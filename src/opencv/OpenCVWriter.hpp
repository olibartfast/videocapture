#pragma once

#include <opencv2/videoio.hpp>

#include "VideoWriterInterface.hpp"

class OpenCVWriter : public VideoWriterInterface {
public:
    bool initialize(const std::string& destination,
                    const videocapture::VideoWriterConfig& config) override;
    bool writeFrame(const videocapture::Frame& frame) override;
    [[nodiscard]] bool isOpen() const override;
    void release() override;

private:
    cv::VideoWriter writer_;
    videocapture::VideoWriterConfig config_{};
    bool initialized_ = false;
};
