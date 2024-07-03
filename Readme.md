# VideoCapture

`VideoCapture` is a C++ library for video capturing, supporting various backends such as OpenCV and optionally GStreamer.

## Features

- Capture video from different sources using OpenCV.
- Optional support for GStreamer to enhance video capturing capabilities.
- Integration with other C++ projects.

## Requirements

- CMake 3.10 or higher
- C++17 compatible compiler
- OpenCV
- spdlog

## Optional

- GStreamer (if `USE_GSTREAMER` is enabled)

## Installation

### Prerequisites

Ensure you have the required dependencies installed:

- [OpenCV](https://opencv.org/)
- [spdlog](https://github.com/gabime/spdlog)
- [GStreamer](https://gstreamer.freedesktop.org/) (optional)

### Build Instructions

1. Clone the repository:

    ```bash
    git clone https://github.com/olibartfast/VideoCapture.git
    cd VideoCapture
    ```

2. Create a build directory:

    ```bash
    mkdir build
    cd build
    ```

3. Configure the project with CMake:

    ```bash
    cmake .. -DUSE_GSTREAMER=ON  # Use -DUSE_GSTREAMER=OFF to disable GStreamer support
    ```

4. Build the project:

    ```bash
    make
    ```

```
