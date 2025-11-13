#pragma once
#include "VideoCaptureInterface.hpp"
#include <string>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class FFmpegCapture : public VideoCaptureInterface {
private:
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    const AVCodec* codec = nullptr;
    SwsContext* swsContext = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frameRGB = nullptr;
    AVPacket* packet = nullptr;
    uint8_t* buffer = nullptr;
    int videoStreamIndex = -1;
    bool initialized = false;

    void cleanup();

public:
    FFmpegCapture();
    ~FFmpegCapture();

    bool initialize(const std::string& source) override;
    bool readFrame(cv::Mat& frame) override;
    void release() override;
};
