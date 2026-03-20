#include "VideoCaptureFactory.hpp"

std::unique_ptr<VideoCaptureInterface> createVideoInterface() {
#if defined(USE_GSTREAMER)
    return std::make_unique<GStreamerCapture>();
#else
    return std::make_unique<OpenCVCapture>();
#endif
}
