#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "Frame.hpp"
#include "VideoWriterConfig.hpp"

// Frame checks shared by the writer backends. Every backend accepts the same
// packed 8-bit layouts and rejects the same frames, so the accepted input is a
// property of the writer contract rather than of the selected backend.
namespace videocapture::writer {

// Bytes per pixel for the packed 8-bit layouts a writer accepts. Planar and
// semi-planar layouts return zero: their plane strides are backend-specific and
// the writers do not take them.
[[nodiscard]] inline int packedBytesPerPixel(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::Gray8:
            return 1;
        case PixelFormat::RGB8:
        case PixelFormat::BGR8:
            return 3;
        case PixelFormat::RGBA8:
        case PixelFormat::BGRA8:
            return 4;
        case PixelFormat::NV12:
        case PixelFormat::YUV420P:
            return 0;
    }
    return 0;
}

// Validates a frame against the configuration the writer was opened with and
// reports the reason on the error stream, so every backend refuses the same
// input with the same diagnostic.
[[nodiscard]] inline bool validateFrame(const Frame& frame, const VideoWriterConfig& config,
                                        const char* backend) {
    if (frame.empty()) {
        std::cerr << backend << ": refusing to write an empty frame" << std::endl;
        return false;
    }
    if (packedBytesPerPixel(frame.format()) == 0) {
        std::cerr << backend << ": planar pixel formats are not accepted by the writer"
                  << std::endl;
        return false;
    }
    if (frame.width() != config.width || frame.height() != config.height) {
        std::cerr << backend << ": frame is " << frame.width() << "x" << frame.height()
                  << " but the writer was opened for " << config.width << "x" << config.height
                  << std::endl;
        return false;
    }
    return true;
}

// Lowercased file extension of a destination, including the leading dot, or an
// empty string when it has none. Backends that pick a container from the file
// name share this so they agree on what ".MP4" means.
[[nodiscard]] inline std::string destinationExtension(const std::string& destination) {
    const std::size_t dot = destination.find_last_of('.');
    if (dot == std::string::npos) {
        return {};
    }
    std::string extension = destination.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return extension;
}

}  // namespace videocapture::writer
