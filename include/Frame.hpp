#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace videocapture {

// Pixel layout is explicit so consumers never have to infer channel order
// from a channel count. The capture backends currently normalize their output
// to BGR8; the additional formats define the frame contract for consumers and
// future native-output backends.
enum class PixelFormat : std::uint8_t {
    Gray8,
    RGB8,
    BGR8,
    RGBA8,
    BGRA8,
    NV12,
    YUV420P,
};

class Frame {
public:
    static constexpr std::size_t MaxPlanes = 3;

    Frame() = default;

    explicit Frame(int width, int height, PixelFormat format = PixelFormat::BGR8) {
        resize(width, height, format);
    }

    [[nodiscard]] bool empty() const noexcept {
        return width_ <= 0 || height_ <= 0 || planeCount_ == 0 || storage_.empty();
    }

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] PixelFormat format() const noexcept { return format_; }
    [[nodiscard]] std::size_t planeCount() const noexcept { return planeCount_; }
    [[nodiscard]] std::size_t storageSizeBytes() const noexcept { return storage_.size(); }

    // Returns the number of interleaved channels for packed formats. Planar
    // and semi-planar formats return zero; inspect their individual planes.
    [[nodiscard]] int channelCount() const noexcept {
        switch (format_) {
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

    [[nodiscard]] std::uint8_t* data(std::size_t plane = 0) noexcept {
        if (plane >= planeCount_ || storage_.empty()) {
            return nullptr;
        }
        return storage_.data() + planes_[plane].offset;
    }

    [[nodiscard]] const std::uint8_t* data(std::size_t plane = 0) const noexcept {
        if (plane >= planeCount_ || storage_.empty()) {
            return nullptr;
        }
        return storage_.data() + planes_[plane].offset;
    }

    [[nodiscard]] std::size_t sizeBytes(std::size_t plane = 0) const noexcept {
        return plane < planeCount_ ? planes_[plane].sizeBytes : 0;
    }

    [[nodiscard]] std::size_t rowStride(std::size_t plane = 0) const noexcept {
        return plane < planeCount_ ? planes_[plane].rowStride : 0;
    }

    // Plane dimensions are measured in logical samples. For NV12 the chroma
    // plane contains two bytes (U and V) per logical chroma sample.
    [[nodiscard]] int planeWidth(std::size_t plane = 0) const noexcept {
        return plane < planeCount_ ? planes_[plane].width : 0;
    }

    [[nodiscard]] int planeHeight(std::size_t plane = 0) const noexcept {
        return plane < planeCount_ ? planes_[plane].height : 0;
    }

    // Presentation timestamp on the source's media timeline. This is not a
    // wall-clock timestamp and may be negative when the source timeline is.
    [[nodiscard]] std::optional<std::chrono::nanoseconds> timestamp() const noexcept {
        return timestamp_;
    }

    void setTimestamp(std::chrono::nanoseconds timestamp) noexcept { timestamp_ = timestamp; }
    void clearTimestamp() noexcept { timestamp_.reset(); }

    // Zero-based frame sequence assigned by a capture source. Capture
    // implementations reset the sequence when they are initialized.
    [[nodiscard]] std::optional<std::uint64_t> sequence() const noexcept { return sequence_; }

    void setSequence(std::uint64_t sequence) noexcept { sequence_ = sequence; }
    void clearSequence() noexcept { sequence_.reset(); }

    // Allocates tightly packed host storage using the canonical plane layout
    // for the requested format. Existing capacity is reused where possible.
    // Invalid dimensions and arithmetic overflow are rejected without
    // modifying the existing frame.
    void resize(int width, int height, PixelFormat format = PixelFormat::BGR8) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Frame dimensions must be positive");
        }

        std::array<PlaneLayout, MaxPlanes> nextPlanes{};
        std::size_t nextPlaneCount = 0;
        std::size_t storageSize = 0;

        auto addPlane = [&](int planeWidth, int planeHeight, std::size_t bytesPerSample) {
            const std::size_t stride =
                checkedMultiply(static_cast<std::size_t>(planeWidth), bytesPerSample);
            const std::size_t planeSize =
                checkedMultiply(stride, static_cast<std::size_t>(planeHeight));
            if (storageSize > std::numeric_limits<std::size_t>::max() - planeSize) {
                throw std::length_error("Frame storage size overflow");
            }
            nextPlanes[nextPlaneCount++] = {storageSize, planeSize, stride, planeWidth,
                                            planeHeight};
            storageSize += planeSize;
        };

        switch (format) {
            case PixelFormat::Gray8:
                addPlane(width, height, 1);
                break;
            case PixelFormat::RGB8:
            case PixelFormat::BGR8:
                addPlane(width, height, 3);
                break;
            case PixelFormat::RGBA8:
            case PixelFormat::BGRA8:
                addPlane(width, height, 4);
                break;
            case PixelFormat::NV12: {
                const int chromaWidth = width / 2 + width % 2;
                const int chromaHeight = height / 2 + height % 2;
                addPlane(width, height, 1);
                addPlane(chromaWidth, chromaHeight, 2);
                break;
            }
            case PixelFormat::YUV420P: {
                const int chromaWidth = width / 2 + width % 2;
                const int chromaHeight = height / 2 + height % 2;
                addPlane(width, height, 1);
                addPlane(chromaWidth, chromaHeight, 1);
                addPlane(chromaWidth, chromaHeight, 1);
                break;
            }
            default:
                throw std::invalid_argument("Unsupported pixel format");
        }

        storage_.resize(storageSize);
        width_ = width;
        height_ = height;
        format_ = format;
        planes_ = nextPlanes;
        planeCount_ = nextPlaneCount;
        timestamp_.reset();
        sequence_.reset();
    }

    // Releases the logical frame while retaining vector capacity for the next
    // decoded frame.
    void clear() noexcept {
        width_ = 0;
        height_ = 0;
        format_ = PixelFormat::BGR8;
        planes_ = {};
        planeCount_ = 0;
        storage_.clear();
        timestamp_.reset();
        sequence_.reset();
    }

private:
    struct PlaneLayout {
        std::size_t offset = 0;
        std::size_t sizeBytes = 0;
        std::size_t rowStride = 0;
        int width = 0;
        int height = 0;
    };

    [[nodiscard]] static std::size_t checkedMultiply(std::size_t lhs, std::size_t rhs) {
        if (rhs != 0 && lhs > std::numeric_limits<std::size_t>::max() / rhs) {
            throw std::length_error("Frame storage size overflow");
        }
        return lhs * rhs;
    }

    int width_ = 0;
    int height_ = 0;
    PixelFormat format_ = PixelFormat::BGR8;
    std::array<PlaneLayout, MaxPlanes> planes_{};
    std::size_t planeCount_ = 0;
    std::vector<std::uint8_t> storage_;
    std::optional<std::chrono::nanoseconds> timestamp_;
    std::optional<std::uint64_t> sequence_;
};

}  // namespace videocapture
