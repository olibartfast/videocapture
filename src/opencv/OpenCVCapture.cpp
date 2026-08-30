#include "OpenCVCapture.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>

#include <opencv2/imgproc.hpp>

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

    nextSequence = 0;
    initialized = true;
    return true;
}

bool OpenCVCapture::readFrame(videocapture::Frame& frame) {
    if (!initialized) {
        frame.clear();
        return false;
    }

    cv::Mat decoded;
    if (!capture.read(decoded) || decoded.empty()) {
        frame.clear();
        return false;
    }

    cv::Mat bgr;
    if (decoded.type() == CV_8UC3) {
        bgr = decoded;
    } else if (decoded.type() == CV_8UC1) {
        cv::cvtColor(decoded, bgr, cv::COLOR_GRAY2BGR);
    } else if (decoded.type() == CV_8UC4) {
        cv::cvtColor(decoded, bgr, cv::COLOR_BGRA2BGR);
    } else {
        frame.clear();
        return false;
    }

    frame.resize(bgr.cols, bgr.rows, videocapture::PixelFormat::BGR8);
    for (int row = 0; row < bgr.rows; ++row) {
        std::memcpy(frame.data() + static_cast<std::size_t>(row) * frame.rowStride(), bgr.ptr(row),
                    frame.rowStride());
    }

    frame.setSequence(nextSequence++);
    const double positionMs = capture.get(cv::CAP_PROP_POS_MSEC);
    if (std::isfinite(positionMs) && positionMs >= 0.0) {
        frame.setTimestamp(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::duration<double, std::milli>(positionMs)));
    }
    return true;
}

void OpenCVCapture::release() {
    // Release OpenCV video capture resources
    capture.release();

    // Reset the initialization status
    nextSequence = 0;
    initialized = false;
}
