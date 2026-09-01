#pragma once

#include "Frame.hpp"
#include "VideoWriterConfig.hpp"

#include <string>

class VideoWriterInterface {
public:
    virtual ~VideoWriterInterface() = default;

    // Open a destination and prepare the encoder described by config. The
    // destination is a file path, or a backend-specific sink description where
    // the backend defines one. Returns false and leaves the writer closed when
    // the destination or the configuration is rejected; a failed call may be
    // followed by another initialize(). Calling initialize() while open first
    // flushes and finalizes the current destination.
    virtual bool initialize(const std::string& destination,
                            const videocapture::VideoWriterConfig& config) = 0;

    // Encode and mux one frame. Frames must carry a packed 8-bit pixel layout
    // (Gray8, RGB8, BGR8, RGBA8, BGRA8) and the dimensions declared in the
    // configuration; planar layouts and mismatched dimensions are rejected.
    // Colour conversion to the encoder's format is performed by the backend.
    virtual bool writeFrame(const videocapture::Frame& frame) = 0;

    // Whether a destination is currently open for writing.
    [[nodiscard]] virtual bool isOpen() const = 0;

    // Flush the encoder, finalize the container, and release resources. Safe to
    // call on a writer that was never initialized, and safe to call twice. The
    // destination is only guaranteed to be a complete, playable file after this
    // returns; finalization problems are reported on the error stream.
    virtual void release() = 0;
};
