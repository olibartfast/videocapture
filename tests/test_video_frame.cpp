#include <gtest/gtest.h>

#include "VideoCaptureInterface.hpp"

TEST(VideoFrameTest, ResizeCreatesPackedBGRStorage) {
    VideoFrame frame;
    frame.resize(8, 6);

    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.width, 8);
    EXPECT_EQ(frame.height, 6);
    EXPECT_EQ(frame.channels(), 3);
    EXPECT_EQ(frame.stride, 24U);
    EXPECT_EQ(frame.data.size(), 144U);
}

TEST(VideoFrameTest, ClearResetsFrame) {
    VideoFrame frame;
    frame.resize(8, 6);
    frame.clear();

    EXPECT_TRUE(frame.empty());
    EXPECT_EQ(frame.width, 0);
    EXPECT_EQ(frame.height, 0);
    EXPECT_EQ(frame.stride, 0U);
    EXPECT_TRUE(frame.data.empty());
}
