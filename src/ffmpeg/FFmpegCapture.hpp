#pragma once
#include <memory>
#include <string>
#include "VideoCaptureInterface.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class FFmpegCapture : public VideoCaptureInterface {
private:
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    const AVCodec* codec = nullptr;
    SwsContext* swsContext = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    int videoStreamIndex = -1;
    bool initialized = false;

    void cleanup();

public:
    FFmpegCapture();
    ~FFmpegCapture();

    bool initialize(const std::string& source) override;
    bool readFrame(VideoFrame& frame) override;
    void release() override;
};
