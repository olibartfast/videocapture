#include <iostream>

#include "VideoCaptureFactory.hpp"

#ifdef VIDEOCAPTURE_USE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#endif

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <video_source>" << std::endl;
        return 1;
    }

    std::unique_ptr<VideoCaptureInterface> videoInterface = createVideoInterface();
    const std::string source = argv[1];
    if (!videoInterface->initialize(source)) {
        std::cerr << "Failed to initialize video capture for input: " << source << std::endl;
        return 1;
    }

    VideoFrame frame;
    std::size_t frameCount = 0;
    while (true) {
        if (!videoInterface->readFrame(frame) || frame.empty()) {
            break;
        }

        ++frameCount;

#ifdef VIDEOCAPTURE_USE_OPENCV
        cv::Mat displayFrame(frame.height, frame.width, CV_8UC3, frame.data.data(), frame.stride);
        cv::imshow("Frame", displayFrame);
        if (cv::waitKey(10) >= 0) {
            break;
        }
#endif
    }

    videoInterface->release();
    std::cout << "Decoded " << frameCount << " frame(s)." << std::endl;
    return 0;
}
