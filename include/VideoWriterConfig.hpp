#pragma once

#include <cmath>
#include <cstdint>

namespace videocapture {

// Codec selection is expressed as intent rather than as a backend-specific
// identifier, so one configuration describes the same output on every writer
// backend. Auto lets the backend use the codec implied by the destination
// container.
enum class VideoCodec : std::uint8_t {
    Auto,
    H264,
    HEVC,
    MJPEG,
};

// Encoding parameters supplied when a writer is initialized. Frame geometry is
// fixed for the life of the destination: writeFrame() rejects frames whose
// dimensions differ from the ones declared here.
struct VideoWriterConfig {
    int width = 0;
    int height = 0;

    // Frames per second on the output timeline. Written frames are placed at
    // this constant rate; a frame's own presentation timestamp describes the
    // capture source's timeline and is not used for output timing.
    double frameRate = 30.0;

    VideoCodec codec = VideoCodec::Auto;

    // Target bit rate. Zero leaves the choice to the backend's own default.
    std::int64_t bitRateBitsPerSecond = 0;

    [[nodiscard]] bool valid() const noexcept {
        return width > 0 && height > 0 && std::isfinite(frameRate) && frameRate > 0.0 &&
               bitRateBitsPerSecond >= 0;
    }
};

}  // namespace videocapture
