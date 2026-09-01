#pragma once

#include <cstdint>
#include <string>

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include "Frame.hpp"
#include "VideoWriterInterface.hpp"

class GStreamerWriter : public VideoWriterInterface {
public:
    GStreamerWriter() = default;
    ~GStreamerWriter() override;

    GStreamerWriter(const GStreamerWriter&) = delete;
    GStreamerWriter& operator=(const GStreamerWriter&) = delete;

    // The destination is an output file path, or a complete pipeline
    // description containing an appsrc when it holds a '!', mirroring how
    // GStreamerPipeline treats capture sources.
    bool initialize(const std::string& destination,
                    const videocapture::VideoWriterConfig& config) override;
    bool writeFrame(const videocapture::Frame& frame) override;
    [[nodiscard]] bool isOpen() const override;
    void release() override;

private:
    bool buildPipelineDescription(const std::string& destination, std::string& description) const;
    bool bindElements(bool ownsFileSink, const std::string& destination);
    bool startStream(videocapture::PixelFormat format);
    void cleanup();

    GstElement* pipeline_ = nullptr;
    GstElement* source_ = nullptr;
    videocapture::VideoWriterConfig config_{};
    videocapture::PixelFormat streamFormat_ = videocapture::PixelFormat::BGR8;
    std::uint64_t nextFrameIndex_ = 0;
    bool streaming_ = false;
    bool initialized_ = false;
};
