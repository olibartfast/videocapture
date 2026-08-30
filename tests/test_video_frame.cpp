#include <gtest/gtest.h>

#include "VideoCaptureInterface.hpp"

TEST(FrameTest, ResizeCreatesPackedBGRStorage) {
    videocapture::Frame frame;
    frame.resize(8, 6, videocapture::PixelFormat::BGR8);

    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.width(), 8);
    EXPECT_EQ(frame.height(), 6);
    EXPECT_EQ(frame.format(), videocapture::PixelFormat::BGR8);
    EXPECT_EQ(frame.channelCount(), 3);
    EXPECT_EQ(frame.planeCount(), 1U);
    EXPECT_EQ(frame.planeWidth(), 8);
    EXPECT_EQ(frame.planeHeight(), 6);
    EXPECT_EQ(frame.rowStride(), 24U);
    EXPECT_EQ(frame.sizeBytes(), 144U);
    EXPECT_EQ(frame.storageSizeBytes(), 144U);
    EXPECT_NE(frame.data(), nullptr);
}

TEST(FrameTest, PlanarFormatsExposeEveryPlane) {
    videocapture::Frame frame(5, 3, videocapture::PixelFormat::YUV420P);

    EXPECT_EQ(frame.channelCount(), 0);
    ASSERT_EQ(frame.planeCount(), 3U);

    EXPECT_EQ(frame.planeWidth(0), 5);
    EXPECT_EQ(frame.planeHeight(0), 3);
    EXPECT_EQ(frame.rowStride(0), 5U);
    EXPECT_EQ(frame.sizeBytes(0), 15U);

    EXPECT_EQ(frame.planeWidth(1), 3);
    EXPECT_EQ(frame.planeHeight(1), 2);
    EXPECT_EQ(frame.rowStride(1), 3U);
    EXPECT_EQ(frame.sizeBytes(1), 6U);
    EXPECT_EQ(frame.sizeBytes(2), 6U);
    EXPECT_EQ(frame.storageSizeBytes(), 27U);
    EXPECT_EQ(frame.data(1), frame.data(0) + frame.sizeBytes(0));
    EXPECT_EQ(frame.data(2), frame.data(1) + frame.sizeBytes(1));
}

TEST(FrameTest, NV12UsesInterleavedChromaSamples) {
    videocapture::Frame frame(5, 3, videocapture::PixelFormat::NV12);

    ASSERT_EQ(frame.planeCount(), 2U);
    EXPECT_EQ(frame.rowStride(0), 5U);
    EXPECT_EQ(frame.sizeBytes(0), 15U);
    EXPECT_EQ(frame.planeWidth(1), 3);
    EXPECT_EQ(frame.planeHeight(1), 2);
    EXPECT_EQ(frame.rowStride(1), 6U);
    EXPECT_EQ(frame.sizeBytes(1), 12U);
}

TEST(FrameTest, PackedFormatsUseCanonicalChannelLayouts) {
    struct ExpectedLayout {
        videocapture::PixelFormat format;
        int channels;
    };
    constexpr ExpectedLayout layouts[] = {
        {videocapture::PixelFormat::Gray8, 1}, {videocapture::PixelFormat::RGB8, 3},
        {videocapture::PixelFormat::BGR8, 3},  {videocapture::PixelFormat::RGBA8, 4},
        {videocapture::PixelFormat::BGRA8, 4},
    };

    for (const auto& layout : layouts) {
        videocapture::Frame frame(7, 2, layout.format);
        EXPECT_EQ(frame.channelCount(), layout.channels);
        EXPECT_EQ(frame.planeCount(), 1U);
        EXPECT_EQ(frame.rowStride(), static_cast<std::size_t>(7 * layout.channels));
        EXPECT_EQ(frame.sizeBytes(), static_cast<std::size_t>(14 * layout.channels));
    }
}

TEST(FrameTest, CopyHasIndependentStorageAndMetadata) {
    videocapture::Frame original(2, 2);
    original.data()[0] = 17;
    original.setTimestamp(std::chrono::nanoseconds(-25));
    original.setSequence(3);

    videocapture::Frame copy = original;
    copy.data()[0] = 42;

    EXPECT_NE(copy.data(), original.data());
    EXPECT_EQ(original.data()[0], 17);
    EXPECT_EQ(copy.data()[0], 42);
    EXPECT_EQ(copy.timestamp(), std::chrono::nanoseconds(-25));
    EXPECT_EQ(copy.sequence(), 3U);
}

TEST(FrameTest, ResizeAndClearResetPerFrameMetadata) {
    videocapture::Frame frame;
    frame.resize(8, 6);
    frame.setTimestamp(std::chrono::nanoseconds(123));
    frame.setSequence(7);

    frame.resize(4, 2);

    EXPECT_FALSE(frame.timestamp().has_value());
    EXPECT_FALSE(frame.sequence().has_value());

    frame.setTimestamp(std::chrono::nanoseconds(456));
    frame.setSequence(8);
    frame.clear();

    EXPECT_TRUE(frame.empty());
    EXPECT_EQ(frame.width(), 0);
    EXPECT_EQ(frame.height(), 0);
    EXPECT_EQ(frame.planeCount(), 0U);
    EXPECT_EQ(frame.rowStride(), 0U);
    EXPECT_EQ(frame.sizeBytes(), 0U);
    EXPECT_EQ(frame.data(), nullptr);
    EXPECT_FALSE(frame.timestamp().has_value());
    EXPECT_FALSE(frame.sequence().has_value());
}

TEST(FrameTest, InvalidResizePreservesExistingFrame) {
    videocapture::Frame frame(8, 6);
    frame.data()[0] = 42;

    EXPECT_THROW(frame.resize(0, 6), std::invalid_argument);

    EXPECT_EQ(frame.width(), 8);
    EXPECT_EQ(frame.height(), 6);
    EXPECT_EQ(frame.data()[0], 42);
}

TEST(FrameTest, OutOfRangePlaneAccessIsSafe) {
    const videocapture::Frame frame(8, 6);

    EXPECT_EQ(frame.data(1), nullptr);
    EXPECT_EQ(frame.sizeBytes(1), 0U);
    EXPECT_EQ(frame.rowStride(1), 0U);
    EXPECT_EQ(frame.planeWidth(1), 0);
    EXPECT_EQ(frame.planeHeight(1), 0);
}
