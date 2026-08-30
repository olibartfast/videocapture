# VideoCapture

[![CI](https://github.com/olibartfast/videocapture/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/olibartfast/videocapture/actions/workflows/ci.yml)
[![Coverage](https://github.com/olibartfast/videocapture/actions/workflows/coverage.yml/badge.svg?branch=master)](https://github.com/olibartfast/videocapture/actions/workflows/coverage.yml)
[![Docker](https://github.com/olibartfast/videocapture/actions/workflows/docker.yml/badge.svg?branch=master)](https://github.com/olibartfast/videocapture/actions/workflows/docker.yml)
[![License](https://img.shields.io/github/license/olibartfast/videocapture)](LICENSE)

`VideoCapture` is a C++ library for video capturing, supporting multiple backends: OpenCV (default), GStreamer, and FFmpeg.

## Features

- Capture video from different sources using OpenCV (default backend)
- Optional support for GStreamer for advanced pipeline capabilities
- Optional support for FFmpeg for maximum codec/format compatibility
- Clean interface-based architecture for easy backend switching
- Integration with other C++ projects using modern CMake

## Requirements

- CMake 3.20 or higher
- C++17 compatible compiler

Install the dependency for the backend you select:

- OpenCV for the default backend
- GStreamer if `USE_GSTREAMER=ON`
- FFmpeg if `USE_FFMPEG=ON`
- SDL2 for the sample application's GStreamer and FFmpeg preview window

OpenCV is not required by GStreamer or FFmpeg builds.

## Installation

### Prerequisites

Ensure you have the required dependencies installed:

- [OpenCV](https://opencv.org/) for the default backend
- [GStreamer](https://gstreamer.freedesktop.org/) for the GStreamer backend
- [FFmpeg](https://ffmpeg.org/) for the FFmpeg backend
- [SDL2](https://www.libsdl.org/) for the alternate-backend sample application

You can use the provided setup scripts to install dependencies:

```bash
# Install base dependencies (OpenCV)
./scripts/setup_dependencies.sh

# Install GStreamer dependencies (without OpenCV)
./scripts/setup_dependencies.sh --gstreamer

# Install FFmpeg dependencies (without OpenCV)
./scripts/setup_dependencies.sh --ffmpeg

# Install both GStreamer and FFmpeg dependencies (without OpenCV)
./scripts/setup_dependencies.sh --gstreamer --ffmpeg
```

### Build Instructions

1. Clone the repository:

    ```bash
    git clone https://github.com/olibartfast/VideoCapture.git
    cd VideoCapture
    ```

2. Configure and build the project with CMake:

    **Default (OpenCV only):**
    ```bash
    cmake -B build -S .
    cmake --build build
    ```

    **With GStreamer:**
    ```bash
    cmake -B build -S . -DUSE_GSTREAMER=ON
    cmake --build build
    ```

    **With FFmpeg:**
    ```bash
    cmake -B build -S . -DUSE_FFMPEG=ON
    cmake --build build
    ```

    **With both GStreamer and FFmpeg (FFmpeg takes priority):**
    ```bash
    cmake -B build -S . -DUSE_GSTREAMER=ON -DUSE_FFMPEG=ON
    cmake --build build
    ```

### Backend Priority

When multiple backends are enabled, the library uses the following priority order:
1. **FFmpeg** (if `USE_FFMPEG=ON`) - Maximum format/codec compatibility
2. **GStreamer** (if `USE_GSTREAMER=ON`) - Advanced pipeline capabilities  
3. **OpenCV** (default) - Simple and reliable

### Running the Application

After building the project, you can run the sample application:

```bash
./build/bin/VideoCaptureApp <path/to/video>
```

All backends currently return packed BGR8 data through the dependency-free
`videocapture::Frame` API. `Frame` owns reusable host storage and exposes the
pixel format, plane dimensions, row stride, optional timestamp, and sequence
number without leaking backend-specific types. The OpenCV build displays frames
with HighGUI. GStreamer and FFmpeg builds use an SDL2 window backed by a streaming
BGR24 texture, without linking OpenCV; if SDL cannot create a video window, the
application still decodes the input and reports the frame count. Timestamps,
when a backend can provide them, are presentation times on the source media
timeline rather than wall-clock times; sequence numbers start at zero for each
successful initialization.

```cpp
auto capture = createVideoInterface();
videocapture::Frame frame;
if (capture->initialize(source) && capture->readFrame(frame)) {
    const std::uint8_t* pixels = frame.data();
    const std::size_t stride = frame.rowStride();
    // Process frame.height() rows of packed BGR8 pixels.
}
```

`Frame.hpp` uses only the C++17 standard library. Standalone consumers do not
need OpenCV, FFmpeg, GStreamer, or any Neuriplo project to use the returned
frame. Optional interop layers can adapt its explicit pixel and plane metadata
to framework-specific image objects.

## Using in Your Project

To use `VideoCapture` in your project, you can use CMake's `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  VideoCapture
  GIT_REPOSITORY https://github.com/olibartfast/VideoCapture.git
  GIT_TAG        main  # or the specific tag/branch you want to use
)
FetchContent_MakeAvailable(VideoCapture)

target_link_libraries(your_target PRIVATE VideoCapture)
```

## CI/CD

This project uses GitHub Actions for continuous integration and deployment. See [docs/CI_CD.md](docs/CI_CD.md) for details.

### Workflows

- **CI**: Automated testing across multiple platforms and backend configurations
- **Release**: Automated release builds and artifact publishing
- **Coverage**: Code coverage analysis with Codecov integration
- **Docker**: Automated Docker image builds and publishing
- **Documentation**: API documentation generation and deployment
- **Dependency Check**: Weekly dependency update notifications

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. See our [contributing guidelines](.github/pull_request_template.md) for details.

## License

This project is licensed under the terms specified in the [LICENSE](LICENSE) file.
