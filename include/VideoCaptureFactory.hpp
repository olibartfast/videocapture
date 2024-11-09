#pragma once
#include "VideoCaptureInterface.hpp"
#ifdef USE_GSTREAMER
#include "GStreamerCapture.hpp"
#else
#include "OpenCVCapture.hpp"
#endif

 std::unique_ptr<VideoCaptureInterface> createVideoInterface(); 
