#pragma once

#include <cstdint>
#include <opencv2/videoio.hpp>
#include "VideoCaptureInterface.hpp"

class OpenCVCapture : public VideoCaptureInterface {
private:
    cv::VideoCapture capture;
    bool initialized = false;  // Track initialization status
    std::uint64_t nextSequence = 0;

public:
    bool initialize(const std::string& source) override;

    bool readFrame(videocapture::Frame& frame) override;

    void release() override;
};
