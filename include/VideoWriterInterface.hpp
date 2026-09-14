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
    // flushes and finalizes the current destination, but does not report the
    // result: call release() first when the outcome of that destination matters.
    virtual bool initialize(const std::string& destination,
                            const videocapture::VideoWriterConfig& config) = 0;

    // Submit one frame for encoding. Frames must carry a packed 8-bit pixel
    // layout (Gray8, RGB8, BGR8, RGBA8, BGRA8) and the dimensions declared in
    // the configuration; planar layouts and mismatched dimensions are rejected
    // immediately. Colour conversion to the encoder's format is performed by
    // the backend.
    //
    // A writer may encode on a thread of its own, in which case this returns
    // once the frame is accepted rather than once it is encoded. Accepted frames
    // are encoded in submission order and never dropped: when the encoder falls
    // behind, this call waits for it. Returns false when the frame is rejected,
    // when no destination is open, or when an earlier frame for the destination
    // failed to encode; after such a failure every later call returns false.
    virtual bool writeFrame(const videocapture::Frame& frame) = 0;

    // Same contract, taking ownership of the frame so that a writer encoding on
    // another thread can hand the pixels over without copying them. The default
    // forwards to the copying overload.
    virtual bool writeFrame(videocapture::Frame&& frame) {
        return writeFrame(static_cast<const videocapture::Frame&>(frame));
    }

    // Whether a destination is open and still accepting frames.
    [[nodiscard]] virtual bool isOpen() const = 0;

    // Encode every accepted frame, flush the encoder, finalize the container,
    // and release resources. The destination is only guaranteed to be a
    // complete, playable file after this returns. Returns false when a frame of
    // the destination failed to encode or the container could not be
    // finalized, and true otherwise, including when nothing was open. Safe to
    // call on a writer that was never initialized, and safe to call twice.
    //
    // A writer is driven from one thread at a time; it may use threads of its
    // own internally, and joins them before this returns.
    virtual bool release() = 0;
};
