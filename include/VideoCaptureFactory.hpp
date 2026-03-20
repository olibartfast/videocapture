#pragma once

#include <memory>

#include "VideoCaptureInterface.hpp"

#if defined(USE_GSTREAMER)
#include "GStreamerCapture.hpp"
#else
#include "OpenCVCapture.hpp"
#endif

std::unique_ptr<VideoCaptureInterface> createVideoInterface();
