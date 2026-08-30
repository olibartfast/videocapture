#pragma once

#include "Frame.hpp"

#include <string>

class VideoCaptureInterface {
public:
    virtual ~VideoCaptureInterface() = default;

    // Initialize the video capture from a source (e.g., file, camera, URL).
    virtual bool initialize(const std::string& source) = 0;

    // Read a frame from the video source.
    // Frames currently use packed BGR8 pixels regardless of the selected backend.
    // Timestamp values, when available, are presentation times on the source's
    // media timeline. Sequence values start at zero after initialization.
    virtual bool readFrame(videocapture::Frame& frame) = 0;

    // Release any resources associated with the video capture.
    virtual void release() = 0;
};
