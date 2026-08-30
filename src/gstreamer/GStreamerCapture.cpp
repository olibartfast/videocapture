#include "GStreamerCapture.hpp"

#include <chrono>
#include <iostream>

namespace {
// Upper bound on how long a wait sits idle before the GLib main context is
// pumped again for bus messages.
constexpr std::chrono::milliseconds kBusPollInterval{5};
}  // namespace

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
        while (!GStreamerPipeline::isFrameReady_ && !GStreamerPipeline::isEndOfStream()) {
            // Bus messages are dispatched from the default GLib main context and
            // nothing else iterates it, so pump it between waits. Blocking
            // indefinitely here would hang the caller on a pipeline that stops
            // producing buffers, and would keep the demo application from
            // servicing its window events.
            lock.unlock();
            pipeline.setMainLoopEvent(false);
            lock.lock();
            if (GStreamerPipeline::isFrameReady_ || GStreamerPipeline::isEndOfStream()) {
                break;
            }
            GStreamerPipeline::frameAvailable_.wait_for(lock, kBusPollInterval);
        }
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
