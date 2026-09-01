#ifdef VIDEOCAPTURE_WITH_WRITER

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "VideoCaptureFactory.hpp"
#include "VideoWriterFactory.hpp"

namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 48;
constexpr int kFrameCount = 15;

videocapture::VideoWriterConfig makeConfig() {
    videocapture::VideoWriterConfig config;
    config.width = kWidth;
    config.height = kHeight;
    config.frameRate = 25.0;
    // Motion JPEG in AVI is the one combination every backend can encode
    // without an optional system codec, which keeps the round trip meaningful
    // on a machine that has no H.264 encoder installed.
    config.codec = videocapture::VideoCodec::MJPEG;
    return config;
}

videocapture::Frame makeFrame(int index,
                              videocapture::PixelFormat format = videocapture::PixelFormat::BGR8) {
    videocapture::Frame frame(kWidth, kHeight, format);
    for (int row = 0; row < frame.height(); ++row) {
        std::uint8_t* pixels = frame.data() + static_cast<std::size_t>(row) * frame.rowStride();
        for (std::size_t byte = 0; byte < frame.rowStride(); ++byte) {
            pixels[byte] = static_cast<std::uint8_t>((row + index * 8 + byte) % 256);
        }
    }
    return frame;
}

std::string temporaryDestination(const std::string& name) {
    std::string directory = ::testing::TempDir();
    if (!directory.empty() && directory.back() != '/') {
        directory.push_back('/');
    }
    return directory + name;
}

bool writerCodecRequired() {
    return std::getenv("VIDEOCAPTURE_REQUIRE_WRITER_CODEC") != nullptr;
}

}  // namespace

class VideoWriterTest : public ::testing::Test {
protected:
    std::unique_ptr<VideoWriterInterface> writer;

    void SetUp() override { writer = createVideoWriter(); }

    void TearDown() override {
        if (writer) {
            writer->release();
        }
    }
};

TEST_F(VideoWriterTest, FactoryCreatesWriter) {
    ASSERT_NE(writer, nullptr);
}

TEST_F(VideoWriterTest, NotOpenBeforeInitialize) {
    EXPECT_FALSE(writer->isOpen());
}

TEST_F(VideoWriterTest, RejectsEmptyDestination) {
    EXPECT_FALSE(writer->initialize("", makeConfig()));
    EXPECT_FALSE(writer->isOpen());
}

TEST_F(VideoWriterTest, RejectsInvalidConfiguration) {
    const std::string destination = temporaryDestination("videocapture_invalid.avi");

    videocapture::VideoWriterConfig config = makeConfig();
    config.width = 0;
    EXPECT_FALSE(writer->initialize(destination, config));

    config = makeConfig();
    config.height = -1;
    EXPECT_FALSE(writer->initialize(destination, config));

    config = makeConfig();
    config.frameRate = 0.0;
    EXPECT_FALSE(writer->initialize(destination, config));

    config = makeConfig();
    config.bitRateBitsPerSecond = -1;
    EXPECT_FALSE(writer->initialize(destination, config));

    EXPECT_FALSE(writer->isOpen());
}

TEST_F(VideoWriterTest, WriteFrameBeforeInitializeFails) {
    EXPECT_FALSE(writer->writeFrame(makeFrame(0)));
}

TEST_F(VideoWriterTest, ReleaseWithoutInitializeDoesNotThrow) {
    EXPECT_NO_THROW(writer->release());
}

TEST_F(VideoWriterTest, MultipleReleaseCalls) {
    EXPECT_NO_THROW({
        writer->release();
        writer->release();
        writer->release();
    });
}

TEST_F(VideoWriterTest, RejectsFramesThatDoNotMatchTheConfiguration) {
    const std::string destination = temporaryDestination("videocapture_mismatch.avi");
    if (!writer->initialize(destination, makeConfig())) {
        if (writerCodecRequired()) {
            FAIL() << "Motion JPEG encoder required by this validation environment";
        }
        GTEST_SKIP() << "no Motion JPEG encoder available for this backend";
    }

    videocapture::Frame smaller(kWidth / 2, kHeight / 2, videocapture::PixelFormat::BGR8);
    EXPECT_FALSE(writer->writeFrame(smaller));

    const videocapture::Frame empty;
    EXPECT_FALSE(writer->writeFrame(empty));

    // Planar layouts carry backend-specific plane strides, so the writer
    // contract does not accept them.
    const videocapture::Frame planar(kWidth, kHeight, videocapture::PixelFormat::YUV420P);
    EXPECT_FALSE(writer->writeFrame(planar));

    // A rejected frame must not close the destination.
    EXPECT_TRUE(writer->isOpen());
    EXPECT_TRUE(writer->writeFrame(makeFrame(0)));

    writer->release();
    std::remove(destination.c_str());
}

TEST_F(VideoWriterTest, WritesAndReadsBackTheSameGeometry) {
    const std::string destination = temporaryDestination("videocapture_roundtrip.avi");
    std::remove(destination.c_str());

    if (!writer->initialize(destination, makeConfig())) {
        if (writerCodecRequired()) {
            FAIL() << "Motion JPEG encoder required by this validation environment";
        }
        GTEST_SKIP() << "no Motion JPEG encoder available for this backend";
    }
    EXPECT_TRUE(writer->isOpen());

    for (int index = 0; index < kFrameCount; ++index) {
        ASSERT_TRUE(writer->writeFrame(makeFrame(index))) << "frame " << index;
    }
    writer->release();
    EXPECT_FALSE(writer->isOpen());

    auto capture = createVideoInterface();
    ASSERT_NE(capture, nullptr);
    ASSERT_TRUE(capture->initialize(destination))
        << "written file is not readable: " << destination;

    videocapture::Frame frame;
    int decoded = 0;
    while (capture->readFrame(frame) && !frame.empty()) {
        EXPECT_EQ(frame.width(), kWidth);
        EXPECT_EQ(frame.height(), kHeight);
        ++decoded;
    }
    capture->release();

    EXPECT_EQ(decoded, kFrameCount);
    std::remove(destination.c_str());
}

TEST_F(VideoWriterTest, ReinitializeFinalizesThePreviousDestination) {
    const std::string firstDestination = temporaryDestination("videocapture_first.avi");
    const std::string secondDestination = temporaryDestination("videocapture_second.avi");
    std::remove(firstDestination.c_str());
    std::remove(secondDestination.c_str());

    if (!writer->initialize(firstDestination, makeConfig())) {
        if (writerCodecRequired()) {
            FAIL() << "Motion JPEG encoder required by this validation environment";
        }
        GTEST_SKIP() << "no Motion JPEG encoder available for this backend";
    }
    for (int index = 0; index < 3; ++index) {
        ASSERT_TRUE(writer->writeFrame(makeFrame(index)));
    }

    ASSERT_TRUE(writer->initialize(secondDestination, makeConfig()));
    for (int index = 0; index < 3; ++index) {
        ASSERT_TRUE(writer->writeFrame(makeFrame(index + 3)));
    }
    writer->release();

    const auto decodedFrameCount = [](const std::string& destination) {
        auto capture = createVideoInterface();
        if (!capture || !capture->initialize(destination)) {
            return -1;
        }
        videocapture::Frame frame;
        int decoded = 0;
        while (capture->readFrame(frame) && !frame.empty()) {
            ++decoded;
        }
        capture->release();
        return decoded;
    };

    EXPECT_EQ(decodedFrameCount(firstDestination), 3);
    EXPECT_EQ(decodedFrameCount(secondDestination), 3);
    std::remove(firstDestination.c_str());
    std::remove(secondDestination.c_str());
}

TEST_F(VideoWriterTest, AcceptsEveryPackedPixelLayout) {
    const std::string destination = temporaryDestination("videocapture_layouts.avi");
    if (!writer->initialize(destination, makeConfig())) {
        if (writerCodecRequired()) {
            FAIL() << "Motion JPEG encoder required by this validation environment";
        }
        GTEST_SKIP() << "no Motion JPEG encoder available for this backend";
    }

    // The GStreamer writer negotiates caps from the first frame and keeps them
    // for the rest of the stream, so each layout is written to a freshly
    // reopened destination.
    for (const videocapture::PixelFormat format :
         {videocapture::PixelFormat::BGR8, videocapture::PixelFormat::RGB8,
          videocapture::PixelFormat::Gray8, videocapture::PixelFormat::BGRA8,
          videocapture::PixelFormat::RGBA8}) {
        ASSERT_TRUE(writer->initialize(destination, makeConfig()));
        EXPECT_TRUE(writer->writeFrame(makeFrame(0, format)))
            << "pixel format " << static_cast<int>(format);
        writer->release();
    }
    std::remove(destination.c_str());
}

#endif  // VIDEOCAPTURE_WITH_WRITER
