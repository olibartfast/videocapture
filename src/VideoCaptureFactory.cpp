#include "VideoCaptureFactory.hpp"

#if defined(USE_FFMPEG)
#include "ffmpeg/FFmpegCapture.hpp"
#elif defined(USE_GSTREAMER)
#include "gstreamer/GStreamerCapture.hpp"
#else
#include "opencv/OpenCVCapture.hpp"
#endif

std::unique_ptr<VideoCaptureInterface> createVideoInterface() {
#if defined(USE_FFMPEG)
    return std::make_unique<FFmpegCapture>();
#elif defined(USE_GSTREAMER)
    return std::make_unique<GStreamerCapture>();
#else
    return std::make_unique<OpenCVCapture>();
#endif
}
