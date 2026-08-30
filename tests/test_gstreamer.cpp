#ifdef USE_GSTREAMER

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

    ASSERT_TRUE(capture->initialize(pipeline));

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!GStreamerPipeline::isEndOfStream() && std::chrono::steady_clock::now() < deadline) {
        while (g_main_context_iteration(nullptr, false)) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_TRUE(GStreamerPipeline::isEndOfStream());

    videocapture::Frame frame;
    ASSERT_TRUE(capture->readFrame(frame));
    EXPECT_EQ(frame.sequence(), 0U);
    EXPECT_FALSE(frame.empty());

    EXPECT_FALSE(capture->readFrame(frame));
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

TEST_F(GStreamerCaptureTest, MultipleReleaseCalls) {
    // Should not crash on multiple release calls
    EXPECT_NO_THROW({
        capture->release();
        capture->release();
    });
}

#endif  // USE_GSTREAMER
