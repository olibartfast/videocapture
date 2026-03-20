#include "GStreamerCapture.hpp"

bool GStreamerCapture::initialize(const std::string& source) {
    try {
        gstocv.initGstLibrary(0, nullptr);
        gstocv.runPipeline(source);
        gstocv.getSink();
        gstocv.setBus();
        gstocv.setState(GST_STATE_PLAYING);
        GStreamerOpenCV::setEndOfStream(false);
        GStreamerOpenCV::isFrameReady_ = false;
        initialized = true;
        return true;
    } catch (...) {
        initialized = false;
        return false;
    }
}

bool GStreamerCapture::readFrame(cv::Mat& frame) {
    if (!initialized) {
        return false;
    }

    gstocv.setMainLoopEvent(false);

    std::unique_lock<std::mutex> lock(GStreamerOpenCV::frameMutex_);
    GStreamerOpenCV::frameAvailable_.wait(lock, [] {
        return GStreamerOpenCV::isFrameReady_ || GStreamerOpenCV::isEndOfStream();
    });

    if (!GStreamerOpenCV::isFrameReady_) {
        return false;
    }

    frame = gstocv.getFrame().clone();
    GStreamerOpenCV::isFrameReady_ = false;
    return !frame.empty();
}

void GStreamerCapture::release() {
    gstocv.setState(GST_STATE_NULL);
    GStreamerOpenCV::setEndOfStream(true);
    initialized = false;
}
