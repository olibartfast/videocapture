#pragma once
#include "VideoCaptureInterface.hpp"
#include <opencv2/highgui/highgui.hpp>


class OpenCVCapture : public VideoCaptureInterface {
private:
    cv::VideoCapture capture;
    bool initialized = false; // Track initialization status

public:
    bool initialize(const std::string& source) override;

    bool readFrame(cv::Mat& frame) override;

    void release() override;
};