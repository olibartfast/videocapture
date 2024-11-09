#pragma once
#include "VideoCaptureInterface.hpp"
#include "GStreamerOpenCV.hpp"
#include <condition_variable>
#include <mutex>

class GStreamerCapture : public VideoCaptureInterface {
private:
    GStreamerOpenCV gstocv;
    bool initialized = false; // Track initialization status
    std::mutex frameMutex_; // Mutex to protect frame access
    std::condition_variable frameAvailable_; // Condition variable to signal new frames

public:
    bool initialize(const std::string& source);
    bool readFrame(cv::Mat& frame) override;
    void release() override;
};