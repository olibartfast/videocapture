#pragma once

#include "GStreamerPipeline.hpp"
#include "VideoCaptureInterface.hpp"

class GStreamerCapture : public VideoCaptureInterface {
private:
    GStreamerPipeline pipeline;
    bool initialized = false;  // Track initialization status

public:
    bool initialize(const std::string& source) override;
    bool readFrame(VideoFrame& frame) override;
    void release() override;
};
