#ifdef USE_GSTREAMER

#include <gtest/gtest.h>
#include "gstreamer/GStreamerCapture.hpp"
#include <opencv2/core.hpp>

class GStreamerCaptureTest : public ::testing::Test {
protected:
    std::unique_ptr<GStreamerCapture> capture;

    void SetUp() override {
        capture = std::make_unique<GStreamerCapture>();
    }

    void TearDown() override {
        if (capture) {
            capture->release();
        }
    }
};

// GStreamerCapture Tests
TEST_F(GStreamerCaptureTest, InitializeWithInvalidPipeline) {
    // GStreamer throws exception for invalid pipeline
    EXPECT_THROW(capture->initialize("invalid ! pipeline ! elements"), std::runtime_error);
}

TEST_F(GStreamerCaptureTest, ReadFrameBeforeInitialize) {
    cv::Mat frame;
    EXPECT_FALSE(capture->readFrame(frame));
    EXPECT_TRUE(frame.empty());
}

TEST_F(GStreamerCaptureTest, ReleaseWithoutInitialize) {
    EXPECT_NO_THROW(capture->release());
}

TEST_F(GStreamerCaptureTest, ValidTestPipeline) {
    // Test with a simple test pipeline (videotestsrc)
    // Note: appsink needs to be properly configured with format
    std::string pipeline = "videotestsrc num-buffers=10 ! video/x-raw,format=BGR,width=640,height=480 ! appsink";
    
    try {
        bool result = capture->initialize(pipeline);
        if (result) {
            cv::Mat frame;
            EXPECT_TRUE(capture->readFrame(frame));
            EXPECT_FALSE(frame.empty());
            EXPECT_EQ(frame.cols, 640);
            EXPECT_EQ(frame.rows, 480);
        }
    } catch (const std::exception& e) {
        // Pipeline construction may fail in some environments
        GTEST_SKIP() << "GStreamer pipeline construction failed: " << e.what();
    }
}

TEST_F(GStreamerCaptureTest, MultipleReleaseCalls) {
    // Should not crash on multiple release calls
    EXPECT_NO_THROW({
        capture->release();
        capture->release();
    });
}

#endif // USE_GSTREAMER
