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
std::unique_ptr<VideoWriterInterface> createVideoWriter();
