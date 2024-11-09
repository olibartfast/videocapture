#include <iostream>
#include <opencv2/opencv.hpp>
#include "VideoCaptureFactory.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <video_source>" << std::endl;
        return 1;
    }

    std::unique_ptr<VideoCaptureInterface> videoInterface = createVideoInterface();
    const std::string source = argv[1];
    if (!videoInterface->initialize(source)) 
    {
        std::cerr << "Failed to initialize video capture for input: " << source << std::endl;
        return 1;
    }    

    cv::Mat frame;
    while (true) {
        if (!videoInterface->readFrame(frame) || frame.empty()) {
            std::cerr << "Error: Could not read a frame from the video capture device." << std::endl;
            break;
        }

        cv::imshow("Frame", frame);

        if (cv::waitKey(10) >= 0) {
            break;
        }
    }

    videoInterface->release();
    return 0;
}