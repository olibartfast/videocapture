#include "OpenCVCapture.hpp"
#include <cctype>
#include <algorithm>

bool OpenCVCapture::initialize(const std::string& source) {
    // Check if source is a numeric camera index
    bool isNumeric = !source.empty() && std::all_of(source.begin(), source.end(), ::isdigit);
    
    if (isNumeric) {
        // Treat as camera device index
        int deviceId = std::stoi(source);
        if (!capture.open(deviceId)) {
            initialized = false;
            return false;
        }
    } else {
        // Treat as file path or URL
        if (!capture.open(source)) {
            initialized = false;
            return false;
        }
    }

    // Verify that the capture is actually opened
    if (!capture.isOpened()) {
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