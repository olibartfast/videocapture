#include <gtest/gtest.h>
#include "opencv/OpenCVCapture.hpp"
#include <opencv2/core.hpp>

class OpenCVCaptureTest : public ::testing::Test {
protected:
    std::unique_ptr<OpenCVCapture> capture;

    void SetUp() override {
        capture = std::make_unique<OpenCVCapture>();
    }

    void TearDown() override {
        if (capture) {
            capture->release();
        }
    }
};

TEST_F(OpenCVCaptureTest, InitializeWithInvalidSource) {
    EXPECT_FALSE(capture->initialize("/nonexistent/video.mp4"));
}

TEST_F(OpenCVCaptureTest, ReadFrameBeforeInitialize) {
    cv::Mat frame;
    EXPECT_FALSE(capture->readFrame(frame));
    EXPECT_TRUE(frame.empty());
}

TEST_F(OpenCVCaptureTest, ReleaseWithoutInitialize) {
    // Should not crash
    EXPECT_NO_THROW(capture->release());
}

TEST_F(OpenCVCaptureTest, MultipleReleaseCalls) {
    // Should not crash on multiple release calls
    EXPECT_NO_THROW({
        capture->release();
        capture->release();
        capture->release();
    });
}

// Test with camera index (may not be available in CI)
TEST_F(OpenCVCaptureTest, InitializeWithCameraIndex) {
    // Try camera 0, will likely fail in CI environment without camera
    bool result = capture->initialize("0");
    
    // In CI/headless environments, camera access typically fails
    // If it succeeds, verify we can read a frame
    if (result) {
        cv::Mat frame;
        EXPECT_TRUE(capture->readFrame(frame));
        EXPECT_FALSE(frame.empty());
        capture->release();
    } else {
        // Expected to fail in CI - this is not an error
        EXPECT_FALSE(result);
    }
}
