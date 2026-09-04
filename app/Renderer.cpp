#include "Renderer.hpp"

#include <limits>
#include <stdexcept>

namespace videocapture::app {

bool Renderer::supports(const Frame& frame) noexcept {
    return !frame.empty() && frame.format() == PixelFormat::BGR8 && frame.planeCount() == 1 &&
           frame.rowStride() >= static_cast<std::size_t>(frame.width()) * 3U &&
           frame.rowStride() <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

void Renderer::copyBgrToRgba(const Frame& frame, std::vector<std::uint8_t>& pixels) {
    if (!supports(frame)) {
        throw std::invalid_argument("Preview requires a packed BGR8 frame");
    }

    pixels.resize(static_cast<std::size_t>(frame.width()) *
                  static_cast<std::size_t>(frame.height()) * 4U);
    for (int y = 0; y < frame.height(); ++y) {
        const auto* source = frame.data() + static_cast<std::size_t>(y) * frame.rowStride();
        auto* destination = pixels.data() + static_cast<std::size_t>(y) *
                                                static_cast<std::size_t>(frame.width()) * 4U;
        for (int x = 0; x < frame.width(); ++x) {
            const std::size_t sourceOffset = static_cast<std::size_t>(x) * 3U;
            const std::size_t destinationOffset = static_cast<std::size_t>(x) * 4U;
            destination[destinationOffset] = source[sourceOffset + 2U];
            destination[destinationOffset + 1U] = source[sourceOffset + 1U];
            destination[destinationOffset + 2U] = source[sourceOffset];
            destination[destinationOffset + 3U] = 255U;
        }
    }
}

std::size_t PollingRenderer::run(FrameReader readFrame) {
    Frame frame;
    std::size_t frameCount = 0;
    while (readFrame(frame) && !frame.empty()) {
        ++frameCount;
        if (!present(frame)) {
            break;
        }
    }
    return frameCount;
}

}  // namespace videocapture::app
