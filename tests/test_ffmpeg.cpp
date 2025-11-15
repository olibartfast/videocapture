#ifdef USE_FFMPEG

#include <gtest/gtest.h>
#include "ffmpeg/FFmpegCapture.hpp"
#include <opencv2/core.hpp>

class FFmpegCaptureTest : public ::testing::Test {
protected:
    std::unique_ptr<FFmpegCapture> capture;

    void SetUp() override {
        capture = std::make_unique<FFmpegCapture>();
    }

    void TearDown() override {
        if (capture) {
            capture->release();
        }
    }
};

TEST_F(FFmpegCaptureTest, InitializeWithInvalidSource) {
    EXPECT_FALSE(capture->initialize("/nonexistent/video.mp4"));
}

TEST_F(FFmpegCaptureTest, InitializeWithEmptySource) {
    EXPECT_FALSE(capture->initialize(""));
}

TEST_F(FFmpegCaptureTest, ReadFrameBeforeInitialize) {
    cv::Mat frame;
    EXPECT_FALSE(capture->readFrame(frame));
    EXPECT_TRUE(frame.empty());
}

TEST_F(FFmpegCaptureTest, ReleaseWithoutInitialize) {
    // Should not crash
    EXPECT_NO_THROW(capture->release());
}

TEST_F(FFmpegCaptureTest, MultipleReleaseCalls) {
    // Should not crash on multiple release calls
    EXPECT_NO_THROW({
        capture->release();
        capture->release();
        capture->release();
    });
}

TEST_F(FFmpegCaptureTest, InitializeWithInvalidFormat) {
    // Try various invalid inputs
    EXPECT_FALSE(capture->initialize("notaurl://invalid"));
    EXPECT_FALSE(capture->initialize("http://"));
    EXPECT_FALSE(capture->initialize("rtsp://"));
}

// Test with lavfi test source (FFmpeg's virtual test source)
TEST_F(FFmpegCaptureTest, InitializeWithTestSource) {
    // Use FFmpeg's lavfi (Libavfilter) test source
    std::string testSource = "lavfi:testsrc=duration=1:size=640x480:rate=30";
    bool result = capture->initialize(testSource);
    
    if (result) {
        cv::Mat frame;
        EXPECT_TRUE(capture->readFrame(frame));
        EXPECT_FALSE(frame.empty());
        EXPECT_EQ(frame.cols, 640);
        EXPECT_EQ(frame.rows, 480);
        EXPECT_EQ(frame.channels(), 3); // RGB
        
        capture->release();
    }
}

TEST_F(FFmpegCaptureTest, MultipleFrameReads) {
    std::string testSource = "lavfi:testsrc=duration=1:size=320x240:rate=10";
    
    if (capture->initialize(testSource)) {
        cv::Mat frame;
        int successfulReads = 0;
        
        // Try to read multiple frames
        for (int i = 0; i < 5; i++) {
            if (capture->readFrame(frame)) {
                successfulReads++;
                EXPECT_FALSE(frame.empty());
                EXPECT_EQ(frame.cols, 320);
                EXPECT_EQ(frame.rows, 240);
            }
        }
        
        EXPECT_GT(successfulReads, 0);
    }
}

#endif // USE_FFMPEG
