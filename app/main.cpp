#include <iostream>
#include <opencv2/opencv.hpp>
#include "VideoCaptureFactory.hpp"

int main() {

    std::unique_ptr<VideoCaptureInterface> videoInterface = createVideoInterface();
    const std::string source = "http://47.51.131.147/-wvhttp-01-/GetOneShot?image_size=1280x720&frame_count=1000000000";
    if (!videoInterface->initialize(source)) 
    {
        std::cerr <<"Failed to initialize video capture for input: " << source << std::endl;
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

    return 0;
}
