#include <iostream>
#include <memory>
#include <string>

#include "Renderer.hpp"
#include "VideoCaptureFactory.hpp"

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

    auto renderer = videocapture::app::createRenderer("VideoCapture");
    const std::size_t frameCount = renderer->run(
        [&videoInterface](videocapture::Frame& frame) { return videoInterface->readFrame(frame); });

    videoInterface->release();
    std::cout << "Decoded " << frameCount << " frame(s)." << std::endl;
    return 0;
}
