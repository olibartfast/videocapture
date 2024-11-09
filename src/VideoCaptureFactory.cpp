#include "VideoCaptureFactory.hpp"


 std::unique_ptr<VideoCaptureInterface> createVideoInterface() 
 {
        #ifdef USE_GSTREAMER
            return std::make_unique<GStreamerCapture>();
        #else
            return std::make_unique<OpenCVCapture>();
        #endif
}