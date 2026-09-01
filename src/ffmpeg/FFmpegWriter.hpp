#pragma once

#include <cstdint>
#include <string>

#include "Frame.hpp"
#include "VideoWriterInterface.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class FFmpegWriter : public VideoWriterInterface {
public:
    FFmpegWriter();
    ~FFmpegWriter() override;

    FFmpegWriter(const FFmpegWriter&) = delete;
    FFmpegWriter& operator=(const FFmpegWriter&) = delete;

    bool initialize(const std::string& destination,
                    const videocapture::VideoWriterConfig& config) override;
    bool writeFrame(const videocapture::Frame& frame) override;
    [[nodiscard]] bool isOpen() const override;
    void release() override;

private:
    // Sends one frame to the encoder and muxes everything it produces. A null
    // frame flushes the encoder at end of stream.
    bool encodeAndMux(AVFrame* frame);
    bool prepareScaler(videocapture::PixelFormat format);
    void cleanup();

    AVFormatContext* formatContext_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    AVStream* stream_ = nullptr;
    SwsContext* swsContext_ = nullptr;
    AVFrame* encodeFrame_ = nullptr;
    AVPacket* packet_ = nullptr;
    videocapture::VideoWriterConfig config_{};
    videocapture::PixelFormat scalerSourceFormat_ = videocapture::PixelFormat::BGR8;
    std::int64_t nextPts_ = 0;
    bool headerWritten_ = false;
    bool initialized_ = false;
};
