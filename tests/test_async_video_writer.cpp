#ifdef VIDEOCAPTURE_WITH_WRITER

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "AsyncVideoWriter.hpp"

// Component tests for the queue in front of the backend encoder. A scripted
// fake stands in for the backend so ordering, back-pressure, failure, and
// shutdown are observed deterministically, without a codec. The round trip
// through a real backend is covered in test_video_writer.cpp.
namespace {

using namespace std::chrono_literals;

constexpr int kWidth = 16;
constexpr int kHeight = 8;

// Upper bound on every wait, so a regression fails instead of hanging.
constexpr auto kWaitLimit = 5s;

// Long enough for a producer that was wrongly left unblocked to get through.
constexpr auto kStillBlocked = 50ms;

// Shared between a test and the fake encoder, so the test can steer and observe
// the encoder after the writer has taken ownership of it.
struct EncoderScript {
    std::mutex mutex;
    std::condition_variable changed;

    bool gateOpen = true;
    bool initializeSucceeds = true;
    bool releaseSucceeds = true;
    bool releaseThrows = false;
    std::optional<std::size_t> failingWrite;
    bool failByThrowing = false;

    std::size_t writesStarted = 0;
    std::vector<std::uint64_t> writtenSequences;
    std::vector<const std::uint8_t*> writtenPixels;
    std::vector<std::thread::id> writerThreads;
    std::vector<std::size_t> framesWrittenAtRelease;

    void setGate(bool open) {
        {
            const std::lock_guard<std::mutex> lock(mutex);
            gateOpen = open;
        }
        changed.notify_all();
    }

    template <typename Predicate>
    bool waitUntil(Predicate predicate) {
        std::unique_lock<std::mutex> lock(mutex);
        return changed.wait_for(lock, kWaitLimit, predicate);
    }
};

class FakeEncoder final : public VideoWriterInterface {
public:
    explicit FakeEncoder(std::shared_ptr<EncoderScript> script) : script_(std::move(script)) {}

    using VideoWriterInterface::writeFrame;

    bool initialize(const std::string& /*destination*/,
                    const videocapture::VideoWriterConfig& /*config*/) override {
        const std::lock_guard<std::mutex> lock(script_->mutex);
        open_ = script_->initializeSucceeds;
        return open_;
    }

    bool writeFrame(const videocapture::Frame& frame) override {
        std::unique_lock<std::mutex> lock(script_->mutex);
        const std::size_t index = script_->writesStarted++;
        script_->changed.notify_all();
        script_->changed.wait(lock, [this] { return script_->gateOpen; });

        if (script_->failingWrite == index) {
            if (script_->failByThrowing) {
                throw std::runtime_error("scripted encoder failure");
            }
            return false;
        }
        script_->writtenSequences.push_back(
            frame.sequence().value_or(std::numeric_limits<std::uint64_t>::max()));
        script_->writtenPixels.push_back(frame.data());
        script_->writerThreads.push_back(std::this_thread::get_id());
        script_->changed.notify_all();
        return true;
    }

    [[nodiscard]] bool isOpen() const override {
        const std::lock_guard<std::mutex> lock(script_->mutex);
        return open_;
    }

    bool release() override {
        const std::lock_guard<std::mutex> lock(script_->mutex);
        script_->framesWrittenAtRelease.push_back(script_->writtenSequences.size());
        open_ = false;
        script_->changed.notify_all();
        if (script_->releaseThrows) {
            throw std::runtime_error("scripted finalization failure");
        }
        return script_->releaseSucceeds;
    }

private:
    std::shared_ptr<EncoderScript> script_;
    bool open_ = false;
};

videocapture::VideoWriterConfig makeConfig() {
    videocapture::VideoWriterConfig config;
    config.width = kWidth;
    config.height = kHeight;
    return config;
}

videocapture::Frame makeFrame(std::uint64_t sequence) {
    videocapture::Frame frame(kWidth, kHeight, videocapture::PixelFormat::BGR8);
    frame.setSequence(sequence);
    return frame;
}

std::vector<std::uint64_t> sequencesUpTo(std::uint64_t count) {
    std::vector<std::uint64_t> sequences;
    for (std::uint64_t sequence = 0; sequence < count; ++sequence) {
        sequences.push_back(sequence);
    }
    return sequences;
}

// For state that has no condition variable to wait on, such as a counter
// updated by a producer thread.
template <typename Predicate>
bool pollUntil(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + kWaitLimit;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

}  // namespace

class AsyncVideoWriterTest : public ::testing::Test {
protected:
    std::shared_ptr<EncoderScript> script = std::make_shared<EncoderScript>();
    std::unique_ptr<AsyncVideoWriter> writer;

    void create(std::size_t capacity = AsyncVideoWriter::kDefaultCapacity) {
        writer =
            std::make_unique<AsyncVideoWriter>(std::make_unique<FakeEncoder>(script), capacity);
    }

    void open(std::size_t capacity = AsyncVideoWriter::kDefaultCapacity) {
        create(capacity);
        ASSERT_TRUE(writer->initialize("scripted", makeConfig()));
    }

    void TearDown() override {
        // A failed assertion can leave the encoder parked; let it finish so the
        // writer's destructor can join.
        script->setGate(true);
        writer.reset();
    }
};

TEST(AsyncVideoWriterConstruction, RejectsANullEncoder) {
    EXPECT_THROW(AsyncVideoWriter(nullptr), std::invalid_argument);
}

TEST_F(AsyncVideoWriterTest, ReleaseWithoutInitializeSucceedsAndTouchesNoEncoder) {
    create();
    EXPECT_FALSE(writer->isOpen());
    EXPECT_TRUE(writer->release());
    EXPECT_TRUE(script->framesWrittenAtRelease.empty());
}

TEST_F(AsyncVideoWriterTest, InitializeFailureLeavesTheWriterClosed) {
    script->initializeSucceeds = false;
    create();

    EXPECT_FALSE(writer->initialize("scripted", makeConfig()));
    EXPECT_FALSE(writer->isOpen());
    EXPECT_FALSE(writer->writeFrame(makeFrame(0)));
    EXPECT_TRUE(writer->release());
    EXPECT_TRUE(script->framesWrittenAtRelease.empty());
}

TEST_F(AsyncVideoWriterTest, RejectsInvalidFramesWithoutWaitingForTheEncoder) {
    open();
    script->setGate(false);

    // Park the encoder on an accepted frame. Rejections that still come back
    // while it is parked cannot have gone through the queue.
    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));
    ASSERT_TRUE(script->waitUntil([this] { return script->writesStarted == 1; }));

    const videocapture::Frame smaller(kWidth / 2, kHeight / 2, videocapture::PixelFormat::BGR8);
    EXPECT_FALSE(writer->writeFrame(smaller));
    EXPECT_FALSE(writer->writeFrame(videocapture::Frame()));
    EXPECT_FALSE(writer->writeFrame(
        videocapture::Frame(kWidth, kHeight, videocapture::PixelFormat::YUV420P)));
    EXPECT_TRUE(writer->isOpen());

    script->setGate(true);
    EXPECT_TRUE(writer->release());
    EXPECT_EQ(script->writtenSequences, sequencesUpTo(1));
}

TEST_F(AsyncVideoWriterTest, EncodesInSubmissionOrderOffTheCallingThread) {
    constexpr std::uint64_t kFrames = 64;
    open();

    for (std::uint64_t sequence = 0; sequence < kFrames; ++sequence) {
        if (sequence % 2 == 0) {
            const videocapture::Frame frame = makeFrame(sequence);
            ASSERT_TRUE(writer->writeFrame(frame)) << "frame " << sequence;
        } else {
            ASSERT_TRUE(writer->writeFrame(makeFrame(sequence))) << "frame " << sequence;
        }
    }
    EXPECT_TRUE(writer->release());

    EXPECT_EQ(script->writtenSequences, sequencesUpTo(kFrames));
    for (const std::thread::id& thread : script->writerThreads) {
        EXPECT_NE(thread, std::this_thread::get_id());
    }
}

TEST_F(AsyncVideoWriterTest, RvalueFramesReachTheEncoderWithoutCopyingPixels) {
    open();

    videocapture::Frame frame = makeFrame(0);
    const std::uint8_t* const pixels = frame.data();
    ASSERT_TRUE(writer->writeFrame(std::move(frame)));
    EXPECT_TRUE(writer->release());

    ASSERT_EQ(script->writtenPixels.size(), 1u);
    EXPECT_EQ(script->writtenPixels.front(), pixels);
}

TEST_F(AsyncVideoWriterTest, LvalueFramesAreCopiedBeforeTheCallReturns) {
    open();
    script->setGate(false);

    videocapture::Frame frame = makeFrame(0);
    ASSERT_TRUE(writer->writeFrame(frame));
    // The caller is free to reuse its frame as soon as the call returns.
    frame.setSequence(99);

    script->setGate(true);
    EXPECT_TRUE(writer->release());
    EXPECT_EQ(script->writtenSequences, sequencesUpTo(1));
    ASSERT_EQ(script->writtenPixels.size(), 1u);
    EXPECT_NE(script->writtenPixels.front(), frame.data());
}

TEST_F(AsyncVideoWriterTest, FullQueueBlocksTheProducerWithoutDroppingFrames) {
    constexpr std::size_t kCapacity = 2;
    constexpr std::size_t kFrames = kCapacity + 3;
    open(kCapacity);
    script->setGate(false);

    std::atomic<std::size_t> accepted{0};
    std::thread producer([&] {
        for (std::size_t sequence = 0; sequence < kFrames; ++sequence) {
            if (writer->writeFrame(makeFrame(sequence))) {
                ++accepted;
            }
        }
    });

    // One frame parked in the encoder plus a full queue; the next write must
    // wait rather than be accepted or dropped.
    const bool encoderParked = script->waitUntil([this] { return script->writesStarted == 1; });
    const bool queueFull = pollUntil([&] { return accepted.load() == kCapacity + 1; });
    std::this_thread::sleep_for(kStillBlocked);
    const std::size_t acceptedWhileFull = accepted.load();

    script->setGate(true);
    producer.join();

    EXPECT_TRUE(encoderParked);
    EXPECT_TRUE(queueFull);
    EXPECT_EQ(acceptedWhileFull, kCapacity + 1);
    EXPECT_EQ(accepted.load(), kFrames);
    EXPECT_TRUE(writer->release());
    EXPECT_EQ(script->writtenSequences, sequencesUpTo(kFrames));
}

TEST_F(AsyncVideoWriterTest, EncoderFailureIsStickyForTheDestination) {
    script->failingWrite = 2;
    open();

    // Accepted writes continue until the failure reaches the producer; the
    // queue bound guarantees that happens within a few frames.
    bool rejected = false;
    for (std::uint64_t sequence = 0; sequence < 100 && !rejected; ++sequence) {
        rejected = !writer->writeFrame(makeFrame(sequence));
    }

    EXPECT_TRUE(rejected);
    EXPECT_FALSE(writer->isOpen());
    EXPECT_FALSE(writer->writeFrame(makeFrame(100)));
    EXPECT_FALSE(writer->release());

    // Nothing behind the failed frame is encoded, and the backend is still
    // finalized so it releases its resources.
    EXPECT_EQ(script->writtenSequences, sequencesUpTo(2));
    EXPECT_EQ(script->framesWrittenAtRelease, std::vector<std::size_t>{2});
}

TEST_F(AsyncVideoWriterTest, EncoderExceptionFailsTheDestinationInsteadOfEscaping) {
    script->failingWrite = 0;
    script->failByThrowing = true;
    open();

    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));
    ASSERT_TRUE(pollUntil([this] { return !writer->isOpen(); }));
    EXPECT_FALSE(writer->writeFrame(makeFrame(1)));
    EXPECT_FALSE(writer->release());
    EXPECT_TRUE(script->writtenSequences.empty());
}

TEST_F(AsyncVideoWriterTest, FailureWakesAProducerBlockedOnAFullQueue) {
    script->failingWrite = 0;
    open(1);
    script->setGate(false);

    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));
    ASSERT_TRUE(script->waitUntil([this] { return script->writesStarted == 1; }));
    ASSERT_TRUE(writer->writeFrame(makeFrame(1)));

    auto blocked =
        std::async(std::launch::async, [this] { return writer->writeFrame(makeFrame(2)); });
    EXPECT_EQ(blocked.wait_for(kStillBlocked), std::future_status::timeout);

    script->setGate(true);
    ASSERT_EQ(blocked.wait_for(kWaitLimit), std::future_status::ready);
    EXPECT_FALSE(blocked.get());
    EXPECT_FALSE(writer->release());
    EXPECT_TRUE(script->writtenSequences.empty());
}

TEST_F(AsyncVideoWriterTest, ReleaseEncodesQueuedFramesBeforeFinalizing) {
    constexpr std::uint64_t kFrames = AsyncVideoWriter::kDefaultCapacity;
    open();
    script->setGate(false);

    for (std::uint64_t sequence = 0; sequence < kFrames; ++sequence) {
        ASSERT_TRUE(writer->writeFrame(makeFrame(sequence)));
    }
    ASSERT_TRUE(script->waitUntil([this] { return script->writesStarted == 1; }));

    auto closing = std::async(std::launch::async, [this] { return writer->release(); });
    EXPECT_EQ(closing.wait_for(kStillBlocked), std::future_status::timeout);
    {
        const std::lock_guard<std::mutex> lock(script->mutex);
        EXPECT_TRUE(script->framesWrittenAtRelease.empty());
    }

    script->setGate(true);
    ASSERT_EQ(closing.wait_for(kWaitLimit), std::future_status::ready);
    EXPECT_TRUE(closing.get());
    EXPECT_EQ(script->writtenSequences, sequencesUpTo(kFrames));
    EXPECT_EQ(script->framesWrittenAtRelease, std::vector<std::size_t>{kFrames});
}

TEST_F(AsyncVideoWriterTest, DestructionFinalizesAnOpenDestination) {
    open();
    for (std::uint64_t sequence = 0; sequence < 3; ++sequence) {
        ASSERT_TRUE(writer->writeFrame(makeFrame(sequence)));
    }

    writer.reset();

    EXPECT_EQ(script->writtenSequences, sequencesUpTo(3));
    EXPECT_EQ(script->framesWrittenAtRelease, std::vector<std::size_t>{3});
}

TEST_F(AsyncVideoWriterTest, ReinitializeFinalizesThePreviousDestination) {
    open();
    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));
    ASSERT_TRUE(writer->writeFrame(makeFrame(1)));

    ASSERT_TRUE(writer->initialize("scripted", makeConfig()));
    ASSERT_TRUE(writer->writeFrame(makeFrame(2)));
    EXPECT_TRUE(writer->release());

    EXPECT_EQ(script->writtenSequences, sequencesUpTo(3));
    EXPECT_EQ(script->framesWrittenAtRelease, (std::vector<std::size_t>{2, 3}));
}

TEST_F(AsyncVideoWriterTest, ReleaseBeforeReopeningReportsTheFailedDestination) {
    script->failingWrite = 0;
    open();
    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));
    EXPECT_FALSE(writer->release());

    ASSERT_TRUE(writer->initialize("scripted", makeConfig()));
    EXPECT_TRUE(writer->writeFrame(makeFrame(1)));
    EXPECT_TRUE(writer->release());
}

TEST_F(AsyncVideoWriterTest, ReopeningDoesNotReportOrInheritThePreviousFailure) {
    script->failingWrite = 0;
    open();
    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));
    ASSERT_TRUE(pollUntil([this] { return !writer->isOpen(); }));

    // The failed destination is finalized without its result being reported,
    // and the next destination starts clean.
    ASSERT_TRUE(writer->initialize("scripted", makeConfig()));
    EXPECT_TRUE(writer->isOpen());
    EXPECT_TRUE(writer->writeFrame(makeFrame(1)));
    EXPECT_TRUE(writer->release());
    EXPECT_EQ(script->framesWrittenAtRelease, (std::vector<std::size_t>{0, 1}));
}

TEST_F(AsyncVideoWriterTest, FinalizationFailureIsReported) {
    script->releaseSucceeds = false;
    open();
    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));
    EXPECT_FALSE(writer->release());
}

TEST_F(AsyncVideoWriterTest, FinalizationExceptionClosesTheWriterAndIsReported) {
    script->releaseThrows = true;
    open();
    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));

    EXPECT_FALSE(writer->release());
    EXPECT_FALSE(writer->isOpen());

    // The encoder thread was already joined; neither a second release nor the
    // destructor may try again.
    EXPECT_TRUE(writer->release());
    writer.reset();
    EXPECT_EQ(script->framesWrittenAtRelease, std::vector<std::size_t>{1});
}

TEST_F(AsyncVideoWriterTest, ReleaseIsIdempotent) {
    open();
    ASSERT_TRUE(writer->writeFrame(makeFrame(0)));

    EXPECT_TRUE(writer->release());
    EXPECT_TRUE(writer->release());
    EXPECT_FALSE(writer->isOpen());
    EXPECT_EQ(script->framesWrittenAtRelease.size(), 1u);
}

#endif  // VIDEOCAPTURE_WITH_WRITER
