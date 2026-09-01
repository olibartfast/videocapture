#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "Renderer.hpp"
#include "VideoCaptureFactory.hpp"

#ifdef VIDEOCAPTURE_WITH_WRITER
#include "VideoWriterFactory.hpp"
#endif

int main(int argc, char* argv[]) {
#ifdef VIDEOCAPTURE_WITH_WRITER
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <video_source> [output_video] [output_fps]"
                  << std::endl;
        return 1;
    }
    const std::string destination = argc >= 3 ? argv[2] : std::string();
    double outputFrameRate = 30.0;
    if (argc == 4) {
        char* end = nullptr;
        outputFrameRate = std::strtod(argv[3], &end);
        if (end == argv[3] || *end != '\0' || !std::isfinite(outputFrameRate) ||
            outputFrameRate <= 0.0) {
            std::cerr << "Invalid output frame rate: " << argv[3] << std::endl;
            return 1;
        }
    }
    std::unique_ptr<VideoWriterInterface> videoWriter;
#else
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <video_source>" << std::endl;
        return 1;
    }
#endif

    std::unique_ptr<VideoCaptureInterface> videoInterface = createVideoInterface();
    const std::string source = argv[1];
    if (!videoInterface->initialize(source)) {
        std::cerr << "Failed to initialize video capture for input: " << source << std::endl;
        return 1;
    }

    auto renderer = videocapture::app::createRenderer("VideoCapture");
#ifdef VIDEOCAPTURE_WITH_WRITER
    bool writerFailed = false;
    const std::size_t frameCount = renderer->run([&](videocapture::Frame& frame) {
        if (!videoInterface->readFrame(frame) || frame.empty()) {
            return false;
        }

        // Open the destination on the first frame, whose geometry configures
        // the encoder.
        if (!destination.empty() && !videoWriter) {
            videocapture::VideoWriterConfig writerConfig;
            writerConfig.width = frame.width();
            writerConfig.height = frame.height();
            writerConfig.frameRate = outputFrameRate;
            videoWriter = createVideoWriter();
            if (!videoWriter->initialize(destination, writerConfig)) {
                std::cerr << "Failed to open output video: " << destination << std::endl;
                writerFailed = true;
                return false;
            }
        }
        if (videoWriter && !videoWriter->writeFrame(frame)) {
            std::cerr << "Failed to write output frame" << std::endl;
            writerFailed = true;
            return false;
        }
        return true;
    });
#else
    const std::size_t frameCount = renderer->run(
        [&videoInterface](videocapture::Frame& frame) { return videoInterface->readFrame(frame); });
#endif

    videoInterface->release();
#ifdef VIDEOCAPTURE_WITH_WRITER
    if (videoWriter) {
        videoWriter->release();
        if (!writerFailed) {
            std::cout << "Wrote " << frameCount << " frame(s) to " << destination << " at "
                      << outputFrameRate << " fps." << std::endl;
        }
    }
#endif
    std::cout << "Decoded " << frameCount << " frame(s)." << std::endl;
#ifdef VIDEOCAPTURE_WITH_WRITER
    if (writerFailed) {
        return 1;
    }
#endif
    return 0;
}
