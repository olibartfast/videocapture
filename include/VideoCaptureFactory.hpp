#pragma once
#include "VideoCaptureInterface.hpp"
#ifdef USE_GSTREAMER
#include "GStreamerCapture.hpp"
#endif
#ifdef USE_FFMPEG
#include "FFmpegCapture.hpp"
#endif
#include "OpenCVCapture.hpp"

 std::unique_ptr<VideoCaptureInterface> createVideoInterface(); 
