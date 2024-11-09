# VideoCapture

`VideoCapture` is a C++ library for video capturing, supporting various backends such as OpenCV and optionally GStreamer.

## Features

- Capture video from different sources using OpenCV
- Optional support for GStreamer to enhance video capturing capabilities
- Integration with other C++ projects using modern CMake

## Requirements

- CMake 3.20 or higher
- C++17 compatible compiler
- OpenCV

### Optional

- GStreamer (if `USE_GSTREAMER` is enabled)

## Installation

### Prerequisites

Ensure you have the required dependencies installed:

- [OpenCV](https://opencv.org/)
- [GStreamer](https://gstreamer.freedesktop.org/) (optional)

### Build Instructions

1. Clone the repository:

    ```bash
    git clone https://github.com/olibartfast/VideoCapture.git
    cd VideoCapture
    ```

2. Configure and build the project with CMake:

    ```bash
    cmake -B build -S . -DUSE_GSTREAMER=ON
    cmake --build build
    ```

   Use `-DUSE_GSTREAMER=OFF` to disable GStreamer support.

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

