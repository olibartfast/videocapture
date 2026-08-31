#include "OpenCvRenderer.hpp"

#include <iostream>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

namespace videocapture::app {

OpenCvRenderer::OpenCvRenderer(std::string title) : title_(std::move(title)) {}

bool OpenCvRenderer::present(const Frame& frame) {
    if (!supports(frame)) {
        if (!warned_) {
            warned_ = true;
            std::cerr << "warning: OpenCV preview requires a packed BGR8 frame, preview disabled\n";
        }
        return true;
    }

    cv::Mat displayFrame(frame.height(), frame.width(), CV_8UC3,
                         const_cast<std::uint8_t*>(frame.data()), frame.rowStride());
    cv::imshow(title_, displayFrame);
    return cv::waitKey(10) < 0;
}

}  // namespace videocapture::app
