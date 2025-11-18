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
- OpenCV

### Optional

- GStreamer (if `USE_GSTREAMER` is enabled)
- FFmpeg (if `USE_FFMPEG` is enabled)

## Installation

### Prerequisites

Ensure you have the required dependencies installed:

- [OpenCV](https://opencv.org/) (required)
- [GStreamer](https://gstreamer.freedesktop.org/) (optional)
- [FFmpeg](https://ffmpeg.org/) (optional)

You can use the provided setup scripts to install dependencies:

```bash
# Install base dependencies (OpenCV)
./scripts/setup_dependencies.sh

# Install with GStreamer support
./scripts/setup_dependencies.sh --gstreamer

# Install with FFmpeg support
./scripts/setup_dependencies.sh --ffmpeg

# Install both GStreamer and FFmpeg
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

