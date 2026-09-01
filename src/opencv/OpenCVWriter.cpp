#include "OpenCVWriter.hpp"

#include <iostream>
#include <string>

#include <opencv2/imgproc.hpp>

#include "WriterSupport.hpp"

namespace {

// OpenCV selects the encoder through a FourCC rather than a codec identifier,
// so codec intent is translated here. Which FourCCs actually work depends on
// the backend OpenCV's videoio was built against.
int toFourCC(videocapture::VideoCodec codec, const std::string& destination) {
    switch (codec) {
        case videocapture::VideoCodec::H264:
            return cv::VideoWriter::fourcc('a', 'v', 'c', '1');
        case videocapture::VideoCodec::HEVC:
            return cv::VideoWriter::fourcc('h', 'v', 'c', '1');
        case videocapture::VideoCodec::MJPEG:
            return cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
        case videocapture::VideoCodec::Auto:
            break;
    }
    // AVI carries MJPEG everywhere; the ISO base media containers default to
    // MPEG-4 part 2, which OpenCV can write without a system H.264 encoder.
    const std::string extension = videocapture::writer::destinationExtension(destination);
    if (extension == ".avi") {
        return cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    }
    return cv::VideoWriter::fourcc('m', 'p', '4', 'v');
}

// Presents the frame's packed pixels as a cv::Mat header. The frame owns the
// storage and is not modified, so the cast only drops constness that
// cv::Mat's constructor cannot express.
cv::Mat asMatHeader(const videocapture::Frame& frame, int type) {
    return cv::Mat(frame.height(), frame.width(), type, const_cast<std::uint8_t*>(frame.data()),
                   frame.rowStride());
}

}  // namespace

bool OpenCVWriter::initialize(const std::string& destination,
                              const videocapture::VideoWriterConfig& config) {
    release();

    if (destination.empty()) {
        std::cerr << "OpenCV writer: destination is empty" << std::endl;
        return false;
    }
    if (!config.valid()) {
        std::cerr << "OpenCV writer: invalid configuration (" << config.width << "x"
                  << config.height << " @ " << config.frameRate << " fps)" << std::endl;
        return false;
    }
    if (config.bitRateBitsPerSecond > 0) {
        // cv::VideoWriter exposes quality, not a bit rate target.
        std::cerr << "OpenCV writer: bit rate is not configurable through this backend; "
                     "using the encoder default"
                  << std::endl;
    }

    if (!writer_.open(destination, toFourCC(config.codec, destination), config.frameRate,
                      cv::Size(config.width, config.height), true) ||
        !writer_.isOpened()) {
        std::cerr << "OpenCV writer: could not open " << destination << " for writing" << std::endl;
        writer_.release();
        return false;
    }

    config_ = config;
    initialized_ = true;
    return true;
}

bool OpenCVWriter::writeFrame(const videocapture::Frame& frame) {
    if (!initialized_) {
        std::cerr << "OpenCV writer: writeFrame() called before initialize()" << std::endl;
        return false;
    }
    if (!videocapture::writer::validateFrame(frame, config_, "OpenCV writer")) {
        return false;
    }

    // cv::VideoWriter takes BGR, which is what every capture backend produces;
    // the other packed layouts are converted here.
    cv::Mat bgr;
    switch (frame.format()) {
        case videocapture::PixelFormat::BGR8:
            bgr = asMatHeader(frame, CV_8UC3);
            break;
        case videocapture::PixelFormat::RGB8:
            cv::cvtColor(asMatHeader(frame, CV_8UC3), bgr, cv::COLOR_RGB2BGR);
            break;
        case videocapture::PixelFormat::Gray8:
            cv::cvtColor(asMatHeader(frame, CV_8UC1), bgr, cv::COLOR_GRAY2BGR);
            break;
        case videocapture::PixelFormat::RGBA8:
            cv::cvtColor(asMatHeader(frame, CV_8UC4), bgr, cv::COLOR_RGBA2BGR);
            break;
        case videocapture::PixelFormat::BGRA8:
            cv::cvtColor(asMatHeader(frame, CV_8UC4), bgr, cv::COLOR_BGRA2BGR);
            break;
        case videocapture::PixelFormat::NV12:
        case videocapture::PixelFormat::YUV420P:
            // Rejected by validateFrame().
            return false;
    }

    writer_.write(bgr);
    // cv::VideoWriter reports encoder failures by closing itself rather than
    // through write(), so the writer's state is what tells us the frame landed.
    if (!writer_.isOpened()) {
        std::cerr << "OpenCV writer: the encoder closed while writing a frame" << std::endl;
        initialized_ = false;
        return false;
    }
    return true;
}

bool OpenCVWriter::isOpen() const {
    return initialized_ && writer_.isOpened();
}

void OpenCVWriter::release() {
    writer_.release();
    config_ = {};
    initialized_ = false;
}
