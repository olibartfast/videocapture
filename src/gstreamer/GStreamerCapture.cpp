#include "GStreamerCapture.hpp"

#include <iostream>

bool GStreamerCapture::initialize(const std::string& source) {
    release();
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
        pipeline.reset();
        initialized = false;
        return false;
    }
}

bool GStreamerCapture::readFrame(videocapture::Frame& frame) {
    if (!initialized) {
        frame.clear();
        return false;
    }
    return pipeline.readFrame(frame);
}

void GStreamerCapture::release() {
    // Release GStreamer resources
    pipeline.reset();

    // Reset the initialization status
    initialized = false;
}
