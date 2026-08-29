#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class PixelFormat {
    BGR24,
};

struct VideoFrame {
    int width = 0;
    int height = 0;
    std::size_t stride = 0;
    PixelFormat format = PixelFormat::BGR24;
    std::vector<std::uint8_t> data;

    [[nodiscard]] bool empty() const noexcept { return width <= 0 || height <= 0 || data.empty(); }

    [[nodiscard]] int channels() const noexcept { return format == PixelFormat::BGR24 ? 3 : 0; }

    void resize(int frameWidth, int frameHeight, PixelFormat frameFormat = PixelFormat::BGR24) {
        if (frameWidth <= 0 || frameHeight <= 0) {
            clear();
            return;
        }
        width = frameWidth;
        height = frameHeight;
        format = frameFormat;
        stride = static_cast<std::size_t>(width) * channels();
        data.resize(stride * static_cast<std::size_t>(height));
    }

    void clear() noexcept {
        width = 0;
        height = 0;
        stride = 0;
        data.clear();
    }
};

class VideoCaptureInterface {
public:
    virtual ~VideoCaptureInterface() = default;

    // Initialize the video capture from a source (e.g., file, camera, URL).
    virtual bool initialize(const std::string& source) = 0;

    // Read a frame from the video source.
    // Frames use packed BGR24 pixels regardless of the selected backend.
    virtual bool readFrame(VideoFrame& frame) = 0;

    // Release any resources associated with the video capture.
    virtual void release() = 0;
};
