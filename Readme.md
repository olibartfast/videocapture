# VideoCapture

`VideoCapture` is a C++ library for video capturing, supporting various backends such as OpenCV and optionally GStreamer.

## Features

- Capture video from different sources using OpenCV.
- Optional support for GStreamer to enhance video capturing capabilities.
- Integration with other C++ projects (The library is currently mainly used as component of the [Object Detection Inference Project](https://github.com/olibartfast/object-detection-inference)).

## Requirements

- CMake 3.10 or higher
- C++17 compatible compiler
- OpenCV

## Optional

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

### Running the Application

After building the project, you can run the sample application to test the library:

```sh
./app/VideoCaptureApp

```

### Linking to Your Project

To use the `VideoCapture` library in your project, link it with CMake:

1. Include the `VideoCapture` library using `FetchContent` or add it as a submodule.

    ```cmake
    include(FetchContent)
    FetchContent_Declare(
      VideoCapture
      GIT_REPOSITORY https://github.com/olibartfast/VideoCapture.git
      GIT_TAG        main  # or the specific tag/branch you want to use
    )
    FetchContent_MakeAvailable(VideoCapture)
    ```

2. Link the `VideoCapture` library in your CMakeLists.txt:

    ```cmake
    target_link_libraries(your_project_name PRIVATE VideoCapture)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${VideoCapture_SOURCE_DIR}/include 
        ${VideoCapture_SOURCE_DIR}/src)    
    ```
