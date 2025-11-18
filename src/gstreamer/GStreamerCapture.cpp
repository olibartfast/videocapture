#include "GStreamerCapture.hpp"


 bool GStreamerCapture::initialize(const std::string& source) {
    try {
        gstocv.initGstLibrary(0, nullptr);
        gstocv.runPipeline(source);
        gstocv.checkError();
        gstocv.getSink();
        gstocv.setBus();
        gstocv.setState(GST_STATE_PLAYING);
        initialized = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GStreamer initialization failed: " << e.what() << std::endl;
        initialized = false;
        return false;
    }
}

bool GStreamerCapture::readFrame(cv::Mat& frame) {
    if (!initialized ||   GStreamerOpenCV::isEndOfStream()) {
        // Handle attempts to read frames without proper initialization
        return false;
    }   
    gstocv.setMainLoopEvent(false);

    {
        std::unique_lock<std::mutex> lock(GStreamerOpenCV::frameMutex_);
        GStreamerOpenCV::frameAvailable_.wait(lock, [this] { return GStreamerOpenCV::isFrameReady_; });
        frame = gstocv.getFrame().clone();
    }   
    return !frame.empty();
}

void GStreamerCapture::release() {
    // Release GStreamer resources
    gstocv.setState(GST_STATE_NULL);

    // Reset the initialization status
    initialized = false;
}
