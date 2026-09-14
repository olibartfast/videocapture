#include "VideoWriterFactory.hpp"

#include "AsyncVideoWriter.hpp"

#if defined(USE_FFMPEG)
#include "ffmpeg/FFmpegWriter.hpp"
#elif defined(USE_GSTREAMER)
#include "gstreamer/GStreamerWriter.hpp"
#else
#include "opencv/OpenCVWriter.hpp"
#endif

namespace {

std::unique_ptr<VideoWriterInterface> createBackendWriter() {
#if defined(USE_FFMPEG)
    return std::make_unique<FFmpegWriter>();
#elif defined(USE_GSTREAMER)
    return std::make_unique<GStreamerWriter>();
#else
    return std::make_unique<OpenCVWriter>();
#endif
}

}  // namespace

std::unique_ptr<VideoWriterInterface> createVideoWriter() {
    return std::make_unique<AsyncVideoWriter>(createBackendWriter());
}
