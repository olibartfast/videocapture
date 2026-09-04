#include "FFmpegWriter.hpp"

#include <iostream>
#include <string>

#include "WriterSupport.hpp"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

namespace {

// av_err2str expands to a compound literal, which is not valid C++, so error
// strings are formatted here instead.
std::string ffmpegError(int code) {
    char message[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, message, sizeof(message));
    return message;
}

AVPixelFormat toAVPixelFormat(videocapture::PixelFormat format) {
    switch (format) {
        case videocapture::PixelFormat::Gray8:
            return AV_PIX_FMT_GRAY8;
        case videocapture::PixelFormat::RGB8:
            return AV_PIX_FMT_RGB24;
        case videocapture::PixelFormat::BGR8:
            return AV_PIX_FMT_BGR24;
        case videocapture::PixelFormat::RGBA8:
            return AV_PIX_FMT_RGBA;
        case videocapture::PixelFormat::BGRA8:
            return AV_PIX_FMT_BGRA;
        case videocapture::PixelFormat::NV12:
        case videocapture::PixelFormat::YUV420P:
            break;
    }
    return AV_PIX_FMT_NONE;
}

AVCodecID toAVCodecID(videocapture::VideoCodec codec, const AVOutputFormat* outputFormat) {
    switch (codec) {
        case videocapture::VideoCodec::Auto:
            return outputFormat ? outputFormat->video_codec : AV_CODEC_ID_NONE;
        case videocapture::VideoCodec::H264:
            return AV_CODEC_ID_H264;
        case videocapture::VideoCodec::HEVC:
            return AV_CODEC_ID_HEVC;
        case videocapture::VideoCodec::MJPEG:
            return AV_CODEC_ID_MJPEG;
    }
    return AV_CODEC_ID_NONE;
}

// Returns the encoder's preferred format when it cannot take the requested one.
// The list of supported formats moved to avcodec_get_supported_config() in
// FFmpeg 7.1; AVCodec::pix_fmts is the equivalent on older releases.
AVPixelFormat chooseEncoderPixelFormat(const AVCodec* encoder, AVPixelFormat preferred) {
    const AVPixelFormat* supported = nullptr;
    int count = 0;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 13, 100)
    const void* configs = nullptr;
    if (avcodec_get_supported_config(nullptr, encoder, AV_CODEC_CONFIG_PIX_FORMAT, 0, &configs,
                                     &count) < 0) {
        return preferred;
    }
    supported = static_cast<const AVPixelFormat*>(configs);
#else
    supported = encoder->pix_fmts;
    if (supported) {
        while (supported[count] != AV_PIX_FMT_NONE) {
            ++count;
        }
    }
#endif

    if (!supported || count <= 0) {
        // The encoder does not advertise a list and accepts what it is given.
        return preferred;
    }
    for (int i = 0; i < count; ++i) {
        if (supported[i] == preferred) {
            return preferred;
        }
    }
    return supported[0];
}

}  // namespace

FFmpegWriter::FFmpegWriter() {
    // Allocate the reusable output packet once, mirroring FFmpegCapture.
    packet_ = av_packet_alloc();
}

FFmpegWriter::~FFmpegWriter() {
    release();
    if (packet_) {
        av_packet_free(&packet_);
    }
}

bool FFmpegWriter::initialize(const std::string& destination,
                              const videocapture::VideoWriterConfig& config) {
    // Reinitialization finalizes the previous destination before replacing it.
    release();

    if (destination.empty()) {
        std::cerr << "FFmpeg writer: destination is empty" << std::endl;
        return false;
    }
    if (!config.valid()) {
        std::cerr << "FFmpeg writer: invalid configuration (" << config.width << "x"
                  << config.height << " @ " << config.frameRate << " fps)" << std::endl;
        return false;
    }

    int result =
        avformat_alloc_output_context2(&formatContext_, nullptr, nullptr, destination.c_str());
    if (result < 0 || !formatContext_) {
        std::cerr << "FFmpeg writer: could not deduce an output format for " << destination << " ("
                  << ffmpegError(result) << ")" << std::endl;
        cleanup();
        return false;
    }

    const AVCodecID codecId = toAVCodecID(config.codec, formatContext_->oformat);
    if (codecId == AV_CODEC_ID_NONE) {
        std::cerr << "FFmpeg writer: container " << formatContext_->oformat->name
                  << " has no default video codec; select one explicitly" << std::endl;
        cleanup();
        return false;
    }

    const AVCodec* encoder = avcodec_find_encoder(codecId);
    if (!encoder) {
        std::cerr << "FFmpeg writer: no encoder available for " << avcodec_get_name(codecId)
                  << std::endl;
        cleanup();
        return false;
    }

    AVPixelFormat encoderFormat =
        codecId == AV_CODEC_ID_MJPEG ? AV_PIX_FMT_YUVJ420P : AV_PIX_FMT_YUV420P;
    encoderFormat = chooseEncoderPixelFormat(encoder, encoderFormat);

    // Chroma-subsampled encoders cannot represent odd dimensions, and the
    // failure otherwise surfaces deep inside the encoder.
    const AVPixFmtDescriptor* descriptor = av_pix_fmt_desc_get(encoderFormat);
    if (descriptor && ((descriptor->log2_chroma_w > 0 && config.width % 2 != 0) ||
                       (descriptor->log2_chroma_h > 0 && config.height % 2 != 0))) {
        std::cerr << "FFmpeg writer: " << avcodec_get_name(codecId) << " needs even dimensions for "
                  << av_get_pix_fmt_name(encoderFormat) << ", got " << config.width << "x"
                  << config.height << std::endl;
        cleanup();
        return false;
    }

    codecContext_ = avcodec_alloc_context3(encoder);
    if (!codecContext_) {
        std::cerr << "FFmpeg writer: could not allocate encoder context" << std::endl;
        cleanup();
        return false;
    }

    // A rational frame rate keeps the stream time base exact for the common
    // fractional rates (for example 30000/1001).
    const AVRational frameRate = av_d2q(config.frameRate, 1000000);
    codecContext_->width = config.width;
    codecContext_->height = config.height;
    codecContext_->pix_fmt = encoderFormat;
    codecContext_->time_base = av_inv_q(frameRate);
    codecContext_->framerate = frameRate;
    if (config.bitRateBitsPerSecond > 0) {
        codecContext_->bit_rate = config.bitRateBitsPerSecond;
    }
    if (formatContext_->oformat->flags & AVFMT_GLOBALHEADER) {
        codecContext_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    result = avcodec_open2(codecContext_, encoder, nullptr);
    if (result < 0) {
        std::cerr << "FFmpeg writer: could not open encoder " << encoder->name << " ("
                  << ffmpegError(result) << ")" << std::endl;
        cleanup();
        return false;
    }

    stream_ = avformat_new_stream(formatContext_, nullptr);
    if (!stream_) {
        std::cerr << "FFmpeg writer: could not create output stream" << std::endl;
        cleanup();
        return false;
    }
    stream_->time_base = codecContext_->time_base;
    stream_->avg_frame_rate = frameRate;
    result = avcodec_parameters_from_context(stream_->codecpar, codecContext_);
    if (result < 0) {
        std::cerr << "FFmpeg writer: could not copy encoder parameters (" << ffmpegError(result)
                  << ")" << std::endl;
        cleanup();
        return false;
    }

    if (!(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        result = avio_open(&formatContext_->pb, destination.c_str(), AVIO_FLAG_WRITE);
        if (result < 0) {
            std::cerr << "FFmpeg writer: could not open " << destination << " ("
                      << ffmpegError(result) << ")" << std::endl;
            cleanup();
            return false;
        }
    }

    result = avformat_write_header(formatContext_, nullptr);
    if (result < 0) {
        std::cerr << "FFmpeg writer: could not write container header (" << ffmpegError(result)
                  << ")" << std::endl;
        cleanup();
        return false;
    }
    headerWritten_ = true;

    encodeFrame_ = av_frame_alloc();
    if (!encodeFrame_) {
        std::cerr << "FFmpeg writer: could not allocate encoder frame" << std::endl;
        cleanup();
        return false;
    }
    encodeFrame_->format = encoderFormat;
    encodeFrame_->width = config.width;
    encodeFrame_->height = config.height;
    result = av_frame_get_buffer(encodeFrame_, 0);
    if (result < 0) {
        std::cerr << "FFmpeg writer: could not allocate encoder frame buffer ("
                  << ffmpegError(result) << ")" << std::endl;
        cleanup();
        return false;
    }

    config_ = config;
    nextPts_ = 0;
    initialized_ = true;
    return true;
}

bool FFmpegWriter::prepareScaler(videocapture::PixelFormat format) {
    if (swsContext_ && format == scalerSourceFormat_) {
        return true;
    }

    const AVPixelFormat sourceFormat = toAVPixelFormat(format);
    if (sourceFormat == AV_PIX_FMT_NONE) {
        std::cerr << "FFmpeg writer: unsupported source pixel format" << std::endl;
        return false;
    }

    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }
    swsContext_ =
        sws_getContext(config_.width, config_.height, sourceFormat, config_.width, config_.height,
                       codecContext_->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext_) {
        std::cerr << "FFmpeg writer: could not initialize colour conversion to "
                  << av_get_pix_fmt_name(codecContext_->pix_fmt) << std::endl;
        return false;
    }
    scalerSourceFormat_ = format;
    return true;
}

bool FFmpegWriter::writeFrame(const videocapture::Frame& frame) {
    if (!initialized_) {
        std::cerr << "FFmpeg writer: writeFrame() called before initialize()" << std::endl;
        return false;
    }
    if (!videocapture::writer::validateFrame(frame, config_, "FFmpeg writer")) {
        return false;
    }
    if (!prepareScaler(frame.format())) {
        return false;
    }

    const int result = av_frame_make_writable(encodeFrame_);
    if (result < 0) {
        std::cerr << "FFmpeg writer: encoder frame is not writable (" << ffmpegError(result) << ")"
                  << std::endl;
        return false;
    }

    const std::uint8_t* const sourceData[4] = {frame.data(), nullptr, nullptr, nullptr};
    const int sourceStride[4] = {static_cast<int>(frame.rowStride()), 0, 0, 0};
    sws_scale(swsContext_, sourceData, sourceStride, 0, config_.height, encodeFrame_->data,
              encodeFrame_->linesize);

    // Output timing is the constant rate the writer was opened with, so the
    // presentation timestamps stay monotonic even for sources that do not
    // report usable ones.
    encodeFrame_->pts = nextPts_++;
    return encodeAndMux(encodeFrame_);
}

bool FFmpegWriter::encodeAndMux(AVFrame* frame) {
    int result = avcodec_send_frame(codecContext_, frame);
    if (result < 0) {
        std::cerr << "FFmpeg writer: could not send frame to encoder (" << ffmpegError(result)
                  << ")" << std::endl;
        return false;
    }

    while (true) {
        result = avcodec_receive_packet(codecContext_, packet_);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return true;
        }
        if (result < 0) {
            std::cerr << "FFmpeg writer: could not receive packet from encoder ("
                      << ffmpegError(result) << ")" << std::endl;
            return false;
        }

        av_packet_rescale_ts(packet_, codecContext_->time_base, stream_->time_base);
        packet_->stream_index = stream_->index;
        // av_interleaved_write_frame takes ownership of the packet's buffer and
        // leaves the packet blank, whether or not it succeeds.
        result = av_interleaved_write_frame(formatContext_, packet_);
        if (result < 0) {
            std::cerr << "FFmpeg writer: could not write packet (" << ffmpegError(result) << ")"
                      << std::endl;
            return false;
        }
    }
}

bool FFmpegWriter::isOpen() const {
    return initialized_;
}

void FFmpegWriter::release() {
    if (initialized_) {
        // Flush the encoder's delayed frames before the trailer, otherwise the
        // tail of the video is silently dropped.
        encodeAndMux(nullptr);
        if (headerWritten_) {
            const int result = av_write_trailer(formatContext_);
            if (result < 0) {
                std::cerr << "FFmpeg writer: could not finalize the container ("
                          << ffmpegError(result) << ")" << std::endl;
            }
        }
    }
    cleanup();
}

void FFmpegWriter::cleanup() {
    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }
    if (encodeFrame_) {
        av_frame_free(&encodeFrame_);
    }
    if (codecContext_) {
        avcodec_free_context(&codecContext_);
    }
    if (formatContext_) {
        if (formatContext_->pb && !(formatContext_->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&formatContext_->pb);
        }
        avformat_free_context(formatContext_);
        formatContext_ = nullptr;
    }
    stream_ = nullptr;
    config_ = {};
    scalerSourceFormat_ = videocapture::PixelFormat::BGR8;
    nextPts_ = 0;
    headerWritten_ = false;
    initialized_ = false;
}
