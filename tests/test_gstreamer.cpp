#ifdef USE_GSTREAMER

#include <algorithm>
#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>
#include "gstreamer/GStreamerCapture.hpp"

class GStreamerCaptureTest : public ::testing::Test {
protected:
    std::unique_ptr<GStreamerCapture> capture;

    void SetUp() override { capture = std::make_unique<GStreamerCapture>(); }

    void TearDown() override {
        if (capture) {
            capture->release();
        }
    }
};

// GStreamerCapture Tests
TEST_F(GStreamerCaptureTest, InitializeWithInvalidPipeline) {
    // GStreamer should return false for invalid pipeline
    EXPECT_FALSE(capture->initialize("invalid ! pipeline ! elements"));
}

TEST_F(GStreamerCaptureTest, ReadFrameBeforeInitialize) {
    videocapture::Frame frame;
    EXPECT_FALSE(capture->readFrame(frame));
    EXPECT_TRUE(frame.empty());
}

TEST_F(GStreamerCaptureTest, ReleaseWithoutInitialize) {
    EXPECT_NO_THROW(capture->release());
}

TEST_F(GStreamerCaptureTest, ValidTestPipeline) {
    std::string pipeline = "videotestsrc num-buffers=1 ! "
                           "video/x-raw,format=BGR,width=64,height=48 ! appsink";

    try {
        bool result = capture->initialize(pipeline);
        EXPECT_TRUE(result);
        if (result) {
            videocapture::Frame frame;
            ASSERT_TRUE(capture->readFrame(frame));
            EXPECT_EQ(frame.width(), 64);
            EXPECT_EQ(frame.height(), 48);
            EXPECT_EQ(frame.channelCount(), 3);
            EXPECT_EQ(frame.format(), videocapture::PixelFormat::BGR8);
            EXPECT_EQ(frame.rowStride(), 192U);
            EXPECT_EQ(frame.sizeBytes(), 9216U);
            EXPECT_EQ(frame.sequence(), 0U);
        }
    } catch (const std::exception& e) {
        // Pipeline construction may fail in some environments
        GTEST_SKIP() << "GStreamer pipeline construction failed: " << e.what();
    }
}

TEST_F(GStreamerCaptureTest, DrainsBufferedFinalFrameBeforeEndOfStream) {
    std::string pipeline = "videotestsrc num-buffers=1 ! "
                           "video/x-raw,format=BGR,width=64,height=48 ! appsink";

    GStreamerPipeline instance;
    instance.initGstLibrary(0, nullptr);
    instance.runPipeline(pipeline);
    instance.getSink();
    instance.setBus();
    instance.setState(GST_STATE_PLAYING);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!instance.isEndOfStream() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_TRUE(instance.isEndOfStream());

    videocapture::Frame frame;
    ASSERT_TRUE(instance.readFrame(frame));
    EXPECT_EQ(frame.sequence(), 0U);
    EXPECT_FALSE(frame.empty());

    EXPECT_FALSE(instance.readFrame(frame));
    EXPECT_TRUE(frame.empty());
}

TEST_F(GStreamerCaptureTest, ReportsEndOfStreamWithoutAnExternalMainLoop) {
    // Nothing in the demo application iterates the GLib main context while it
    // waits for a frame, so end of stream has to reach the reader on its own.
    std::string pipeline = "videotestsrc num-buffers=2 ! "
                           "video/x-raw,format=BGR,width=64,height=48 ! appsink";

    ASSERT_TRUE(capture->initialize(pipeline));

    auto drain = std::async(std::launch::async, [this] {
        videocapture::Frame frame;
        std::size_t frameCount = 0;
        while (capture->readFrame(frame) && !frame.empty()) {
            ++frameCount;
        }
        return frameCount;
    });

    ASSERT_EQ(drain.wait_for(std::chrono::seconds(10)), std::future_status::ready)
        << "readFrame() never observed end of stream";
    EXPECT_EQ(drain.get(), 2U);
}

namespace {
std::string testSource(int width, const std::string& pattern, int buffers = 1) {
    return "videotestsrc is-live=true pattern=" + pattern +
           " num-buffers=" + std::to_string(buffers) +
           " ! video/x-raw,format=BGR,width=" + std::to_string(width) +
           ",height=24,framerate=30/1 ! appsink";
}

void expectSolidFrame(const videocapture::Frame& frame, int width, std::uint8_t pixel) {
    EXPECT_EQ(frame.width(), width);
    EXPECT_EQ(frame.height(), 24);
    EXPECT_EQ(frame.format(), videocapture::PixelFormat::BGR8);
    EXPECT_EQ(frame.rowStride(), static_cast<std::size_t>(width) * 3);
    EXPECT_EQ(frame.sizeBytes(), static_cast<std::size_t>(width) * 24 * 3);
    EXPECT_TRUE(std::all_of(frame.data(), frame.data() + frame.sizeBytes(),
                            [pixel](std::uint8_t value) { return value == pixel; }));
}
}  // namespace

TEST_F(GStreamerCaptureTest, ConcurrentCapturesKeepPixelsSequenceAndEosIndependent) {
    GStreamerCapture other;
    ASSERT_TRUE(capture->initialize(testSource(64, "black")));
    ASSERT_TRUE(other.initialize(testSource(65, "white")));

    auto read = std::async(std::launch::async, [this] {
        videocapture::Frame frame;
        EXPECT_TRUE(capture->readFrame(frame));
        return frame;
    });
    videocapture::Frame white;
    ASSERT_TRUE(other.readFrame(white));
    ASSERT_EQ(read.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    const auto black = read.get();
    expectSolidFrame(black, 64, 0);
    expectSolidFrame(white, 65, 255);
    EXPECT_EQ(black.sequence(), 0U);
    EXPECT_EQ(white.sequence(), 0U);
    EXPECT_TRUE(black.timestamp().has_value());
    EXPECT_TRUE(white.timestamp().has_value());
    videocapture::Frame eos;
    EXPECT_FALSE(capture->readFrame(eos));
    EXPECT_FALSE(other.readFrame(eos));
}

TEST_F(GStreamerCaptureTest, EosAndDestructionOfOneCaptureLeaveTheOtherRunning) {
    GStreamerCapture other;
    ASSERT_TRUE(capture->initialize(testSource(64, "black")));
    ASSERT_TRUE(other.initialize(testSource(65, "white", 10)));
    videocapture::Frame frame;
    ASSERT_TRUE(other.readFrame(frame));
    const auto firstSequence = frame.sequence();
    ASSERT_TRUE(capture->readFrame(frame));
    EXPECT_FALSE(capture->readFrame(frame));
    capture.reset();
    ASSERT_TRUE(other.readFrame(frame));
    expectSolidFrame(frame, 65, 255);
    EXPECT_GT(frame.sequence(), firstSequence);
}

TEST_F(GStreamerCaptureTest, DestroyingAnActiveCaptureDoesNotDisturbAnotherReader) {
    GStreamerCapture other;
    ASSERT_TRUE(capture->initialize(testSource(64, "black", 100)));
    ASSERT_TRUE(other.initialize(testSource(65, "white", 100)));
    videocapture::Frame initial;
    ASSERT_TRUE(other.readFrame(initial));
    auto read = std::async(std::launch::async, [&other] {
        videocapture::Frame next;
        EXPECT_TRUE(other.readFrame(next));
        return next;
    });
    capture.reset();
    ASSERT_EQ(read.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    const auto next = read.get();
    expectSolidFrame(next, 65, 255);
    EXPECT_GT(next.sequence(), initial.sequence());
}

TEST_F(GStreamerCaptureTest, ReinitializeAndPartialFailureCleanUpOnlyTheirOwnPipeline) {
    GStreamerCapture other;
    ASSERT_TRUE(other.initialize(testSource(65, "white", 100)));
    for (int iteration = 0; iteration < 8; ++iteration) {
        ASSERT_TRUE(capture->initialize(testSource(64, "black", 100)));
        videocapture::Frame frame;
        ASSERT_TRUE(capture->readFrame(frame));
        EXPECT_EQ(frame.sequence(), 0U);
        // A parsed pipeline with no appsink exercises partial setup cleanup.
        EXPECT_FALSE(capture->initialize("videotestsrc ! fakesink"));
        EXPECT_FALSE(capture->readFrame(frame));
        EXPECT_TRUE(frame.empty());
        ASSERT_TRUE(capture->initialize(testSource(63, "black")));
        ASSERT_TRUE(capture->readFrame(frame));
        expectSolidFrame(frame, 63, 0);
        EXPECT_EQ(frame.sequence(), 0U);
        ASSERT_TRUE(other.readFrame(frame));
        expectSolidFrame(frame, 65, 255);
    }
}

TEST_F(GStreamerCaptureTest, RuntimeErrorIsLocalAndDoesNotNeedAMainLoop) {
    GStreamerCapture other;
    ASSERT_TRUE(other.initialize(testSource(65, "white", 100)));
    ASSERT_TRUE(capture->initialize(
        "videotestsrc is-live=true ! video/x-raw,format=BGR,width=64,height=24 ! "
        "identity error-after=1 ! appsink"));
    auto failedRead = std::async(std::launch::async, [this] {
        videocapture::Frame frame;
        return capture->readFrame(frame);
    });
    ASSERT_EQ(failedRead.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_FALSE(failedRead.get());
    videocapture::Frame frame;
    ASSERT_TRUE(other.readFrame(frame));
    expectSolidFrame(frame, 65, 255);
    EXPECT_FALSE(capture->readFrame(frame));
    EXPECT_TRUE(frame.empty());
}

TEST_F(GStreamerCaptureTest, ReinitializeAnOpenCaptureStartsANewSequenceAndTimeline) {
    for (int iteration = 0; iteration < 8; ++iteration) {
        ASSERT_TRUE(capture->initialize("videotestsrc num-buffers=1 pattern=black ! "
                                        "video/x-raw,format=BGR,width=63,height=24 ! appsink"));
        videocapture::Frame frame;
        ASSERT_TRUE(capture->readFrame(frame));
        EXPECT_EQ(frame.sequence(), 0U);
        ASSERT_TRUE(frame.timestamp().has_value());
        EXPECT_EQ(frame.timestamp()->count(), 0);
        expectSolidFrame(frame, 63, 0);
    }
}

TEST_F(GStreamerCaptureTest, MultipleReleaseCalls) {
    // Should not crash on multiple release calls
    EXPECT_NO_THROW({
        capture->release();
        capture->release();
    });
}

#endif  // USE_GSTREAMER
