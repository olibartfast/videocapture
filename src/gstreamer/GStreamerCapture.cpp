#include "GStreamerCapture.hpp"

#include <iostream>

bool GStreamerCapture::initialize(const std::string& source) {
    try {
        pipeline.initGstLibrary(0, nullptr);
        pipeline.runPipeline(source);
        pipeline.checkError();
        pipeline.getSink();
        pipeline.setBus();
        pipeline.setState(GST_STATE_PLAYING);
        initialized = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GStreamer initialization failed: " << e.what() << std::endl;
        initialized = false;
        return false;
    }
}

bool GStreamerCapture::readFrame(videocapture::Frame& frame) {
    if (!initialized) {
        frame.clear();
        return false;
    }
    pipeline.setMainLoopEvent(false);

    {
        std::unique_lock<std::mutex> lock(GStreamerPipeline::frameMutex_);
        GStreamerPipeline::frameAvailable_.wait(lock, [] {
            return GStreamerPipeline::isFrameReady_ || GStreamerPipeline::isEndOfStream();
        });
        if (!GStreamerPipeline::isFrameReady_) {
            frame.clear();
            return false;
        }
        frame = pipeline.takeFrame();
        GStreamerPipeline::isFrameReady_ = false;
    }
    return !frame.empty();
}

void GStreamerCapture::release() {
    // Release GStreamer resources
    pipeline.setState(GST_STATE_NULL);

    // Reset the initialization status
    initialized = false;
}
