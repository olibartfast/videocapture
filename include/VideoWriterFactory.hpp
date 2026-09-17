#pragma once

#ifndef VIDEOCAPTURE_WITH_WRITER
#error "VideoWriterFactory.hpp requires a videocapture build configured with -DUSE_VIDEOWRITER=ON"
#endif

#include <memory>
#include "VideoWriterInterface.hpp"

// Creates a writer for the backend videocapture was built with, following the
// same priority as createVideoInterface(): FFmpeg, then GStreamer, then OpenCV.
// The writer therefore reuses the capture backend's dependency and introduces
// none of its own.
//
// The returned writer encodes on a thread of its own, so writeFrame() costs the
// caller a validation and a hand-off rather than an encode; pass frames by
// rvalue to avoid copying their pixels. See VideoWriterInterface for the
// ordering, back-pressure, and failure contract.
std::unique_ptr<VideoWriterInterface> createVideoWriter();
