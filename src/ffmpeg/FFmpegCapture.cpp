#include "FFmpegCapture.hpp"
#include <iostream>

FFmpegCapture::FFmpegCapture() {
    // Allocate packet once
    packet = av_packet_alloc();
}

FFmpegCapture::~FFmpegCapture() {
    release();
    if (packet) {
        av_packet_free(&packet);
    }
}

void FFmpegCapture::cleanup() {
    if (buffer) {
        av_free(buffer);
        buffer = nullptr;
    }
    if (frameRGB) {
        av_frame_free(&frameRGB);
        frameRGB = nullptr;
    }
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    videoStreamIndex = -1;
    initialized = false;
}

bool FFmpegCapture::initialize(const std::string& source) {
    // Clean up any previous initialization
    cleanup();

    // Open input file/stream
    if (avformat_open_input(&formatContext, source.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "FFmpeg: Could not open source: " << source << std::endl;
        return false;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "FFmpeg: Could not find stream information" << std::endl;
        cleanup();
        return false;
    }

    // Find the first video stream
    videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "FFmpeg: Could not find video stream" << std::endl;
        cleanup();
        return false;
    }

    // Get codec parameters
    AVCodecParameters* codecParams = formatContext->streams[videoStreamIndex]->codecpar;

    // Find decoder for the video stream
    codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "FFmpeg: Unsupported codec" << std::endl;
        cleanup();
        return false;
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "FFmpeg: Could not allocate codec context" << std::endl;
        cleanup();
        return false;
    }

    // Copy codec parameters to codec context
    if (avcodec_parameters_to_context(codecContext, codecParams) < 0) {
        std::cerr << "FFmpeg: Could not copy codec parameters" << std::endl;
        cleanup();
        return false;
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "FFmpeg: Could not open codec" << std::endl;
        cleanup();
        return false;
    }

    // Allocate video frames
    frame = av_frame_alloc();
    frameRGB = av_frame_alloc();
    if (!frame || !frameRGB) {
        std::cerr << "FFmpeg: Could not allocate frames" << std::endl;
        cleanup();
        return false;
    }

    // Determine required buffer size and allocate buffer
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, codecContext->width,
                                            codecContext->height, 1);
    buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    if (!buffer) {
        std::cerr << "FFmpeg: Could not allocate buffer" << std::endl;
        cleanup();
        return false;
    }

    // Assign buffer to frameRGB
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_BGR24,
                        codecContext->width, codecContext->height, 1);

    // Initialize SWS context for software scaling
    swsContext = sws_getContext(codecContext->width, codecContext->height,
                               codecContext->pix_fmt, codecContext->width,
                               codecContext->height, AV_PIX_FMT_BGR24,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext) {
        std::cerr << "FFmpeg: Could not initialize SWS context" << std::endl;
        cleanup();
        return false;
    }

    initialized = true;
    return true;
}

bool FFmpegCapture::readFrame(cv::Mat& outFrame) {
    if (!initialized) {
        return false;
    }

    // Read frames until we get a video frame from our stream
    while (av_read_frame(formatContext, packet) >= 0) {
        // Check if the packet belongs to the video stream
        if (packet->stream_index == videoStreamIndex) {
            // Send packet to decoder
            int ret = avcodec_send_packet(codecContext, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                std::cerr << "FFmpeg: Error sending packet to decoder" << std::endl;
                continue;
            }

            // Receive decoded frame
            ret = avcodec_receive_frame(codecContext, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_unref(packet);
                continue;
            } else if (ret < 0) {
                av_packet_unref(packet);
                std::cerr << "FFmpeg: Error receiving frame from decoder" << std::endl;
                continue;
            }

            // Convert the frame from native format to BGR24
            sws_scale(swsContext, frame->data, frame->linesize, 0,
                     codecContext->height, frameRGB->data, frameRGB->linesize);

            // Create cv::Mat from the converted frame
            outFrame = cv::Mat(codecContext->height, codecContext->width, CV_8UC3,
                              frameRGB->data[0], frameRGB->linesize[0]).clone();

            av_packet_unref(packet);
            return true;
        }

        av_packet_unref(packet);
    }

    // End of stream
    return false;
}

void FFmpegCapture::release() {
    cleanup();
}
