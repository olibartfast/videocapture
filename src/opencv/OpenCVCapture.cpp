#include "OpenCVCapture.hpp"

bool OpenCVCapture::initialize(const std::string& source) {
    // Initialize OpenCV video capture
    if (!capture.open(source)) {
        // Handle initialization errors
        initialized = false;
        return false;
    }

    initialized = true;
    return true;
}

bool OpenCVCapture::readFrame(cv::Mat& frame) {
    if (!initialized) {
        // Handle attempts to read frames without proper initialization
        return false;
    }

    return capture.read(frame);
}

void OpenCVCapture::release() {
    // Release OpenCV video capture resources
    capture.release();

    // Reset the initialization status
    initialized = false;
}