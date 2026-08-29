#pragma once

#include <opencv2/videoio.hpp>
#include "VideoCaptureInterface.hpp"

class OpenCVCapture : public VideoCaptureInterface {
private:
    cv::VideoCapture capture;
    bool initialized = false;  // Track initialization status

public:
    bool initialize(const std::string& source) override;

    bool readFrame(VideoFrame& frame) override;

    void release() override;
};
