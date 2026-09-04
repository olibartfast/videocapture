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
- A sample-app renderer: OpenCV HighGUI, SDL2, GLFW, or Sokol

OpenCV is not required by GStreamer or FFmpeg builds.

## Installation

### Prerequisites

Ensure you have the required dependencies installed:

- [OpenCV](https://opencv.org/) for the default backend
- [GStreamer](https://gstreamer.freedesktop.org/) for the GStreamer backend
- [FFmpeg](https://ffmpeg.org/) for the FFmpeg backend
- [SDL2](https://www.libsdl.org/), [GLFW](https://www.glfw.org/), or
  [Sokol](https://github.com/floooh/sokol) for an explicitly selected sample renderer

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

    **Try GLFW or Sokol in the sample app:**
    ```bash
    cmake -B build-glfw -S . -DUSE_FFMPEG=ON \
      -DVIDEOCAPTURE_APP_RENDERER=GLFW \
      -DVIDEOCAPTURE_FETCH_APP_DEPENDENCIES=ON
    cmake --build build-glfw

    cmake -B build-sokol -S . -DUSE_FFMPEG=ON \
      -DVIDEOCAPTURE_APP_RENDERER=SOKOL \
      -DVIDEOCAPTURE_FETCH_APP_DEPENDENCIES=ON
    cmake --build build-sokol
    ```

    `VIDEOCAPTURE_APP_RENDERER=AUTO` keeps the existing behavior: OpenCV
    HighGUI for the default capture backend and SDL2 for FFmpeg or GStreamer.
    Dependency fetching is off by default. Install GLFW system-wide, or set
    `SOKOL_ROOT`, to use those renderers without configure-time network access.
    The fetch path pins GLFW 3.5.1 and Sokol commit
    `1847290135f95e57e6d220b0a41208306aafc0dd`.

    **With the video writer (combines with any backend):**
    ```bash
    cmake -B build -S . -DUSE_FFMPEG=ON -DUSE_VIDEOWRITER=ON
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

Writer builds accept an output path and an optional output frame rate:

```bash
./build/bin/VideoCaptureApp <path/to/video> out.mp4 25
```

All backends currently return packed BGR8 data through the dependency-free
`videocapture::Frame` API. `Frame` owns reusable host storage and exposes the
pixel format, plane dimensions, row stride, optional timestamp, and sequence
number without leaking backend-specific types. The OpenCV build displays frames
with HighGUI. GStreamer and FFmpeg builds default to SDL2, while
`VIDEOCAPTURE_APP_RENDERER` can select GLFW or Sokol without linking OpenCV.
The renderer interface and factory live only in `app/`; the capture library and
public frame API do not depend on any window toolkit. SDL and GLFW disable their
preview and continue decoding if a window cannot be created. Timestamps,
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

## Writing Video

`USE_VIDEOWRITER=ON` adds a sink that mirrors the capture side: `initialize` /
`writeFrame` / `release` against the same dependency-free `videocapture::Frame`.
`createVideoWriter()` follows the same backend priority as
`createVideoInterface()`, so a build encodes with whatever it decodes with.

```cpp
#include "VideoWriterFactory.hpp"

videocapture::VideoWriterConfig config;
config.width = frame.width();
config.height = frame.height();
config.frameRate = 30.0;              // the caller owns the output timeline
config.codec = videocapture::VideoCodec::Auto;  // or H264, HEVC, MJPEG

auto writer = createVideoWriter();
if (writer->initialize("annotated.mp4", config)) {
    writer->writeFrame(frame);
    writer->release();                // flushes the encoder and closes the file
}
```

### What the writer costs

Enabling the writer drops no dependency and adds none: every backend already
links the library that encodes.

| Build configuration | What `-DUSE_VIDEOWRITER=ON` links |
| --- | --- |
| `USE_FFMPEG=ON` | nothing new — encoding uses the `libavcodec`, `libavformat`, `libswscale` already linked for decoding |
| `USE_GSTREAMER=ON` | nothing new — `appsrc` lives in the `libgstapp` already linked for the capture `appsink` |
| OpenCV (default) | nothing new — `cv::VideoWriter` is in the `videoio` module already linked |

What does vary is what has to be installed at runtime: a container and codec are
only writable if the backend was built with, or can load, that encoder. The
GStreamer writer needs the plugin for the encoder it selects (`jpegenc` from
gst-plugins-good, `x264enc` from gst-plugins-ugly, `x265enc` and `h265parse`
from gst-plugins-bad).

### Writer contract

- Frames must carry a packed 8-bit layout (`Gray8`, `RGB8`, `BGR8`, `RGBA8`,
  `BGRA8`) and the dimensions declared in the configuration. Colour conversion
  to the encoder's format is the backend's job. Planar layouts are rejected,
  because their plane strides are backend-specific.
- Output timing is the constant `frameRate` the writer was opened with. A
  frame's own timestamp describes the *source's* timeline and is not used for
  output timing, so sources with absent or non-monotonic timestamps still
  produce a well-formed file.
- `release()` flushes the encoder and finalizes the container. The destination
  is only a complete, playable file once it returns. Calling `initialize()`
  again performs the same finalization before opening the next destination.
- `VideoWriterConfig::codec` states intent (`Auto`, `H264`, `HEVC`, `MJPEG`);
  each backend maps it to its own encoder. `Auto` follows the destination's
  container.
- The GStreamer writer also accepts a complete pipeline description containing
  an `appsrc` in place of a file path, mirroring how the GStreamer capture
  backend treats sources.

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
