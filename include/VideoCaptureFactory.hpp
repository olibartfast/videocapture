#pragma once

#include <memory>
#include "VideoCaptureInterface.hpp"

std::unique_ptr<VideoCaptureInterface> createVideoInterface();
