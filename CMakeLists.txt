cmake_minimum_required(VERSION 3.10)
project(VideoCapture)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)

# Define an option to enable or disable GStreamer support
unset(USE_GSTREAMER CACHE)
option(USE_GSTREAMER "Use GStreamer for video capture (optional)" OFF)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
message(STATUS "Videocapture Cmake module path: ${CMAKE_MODULE_PATH}")

# Add source files for video capture
set(VIDEOCAPTURE_SOURCES
    src/VideoCaptureInterface.cpp
)

# Create the video capture library
add_library(VideoCaptureLib SHARED ${VIDEOCAPTURE_SOURCES})

# Include directories
target_include_directories(VideoCaptureLib PRIVATE
    include
    src
    ${OpenCV_INCLUDE_DIRS}
    ${spdlog_INCLUDE_DIRS}
)

# Link against GStreamer libraries if USE_GSTREAMER is ON
if (USE_GSTREAMER)
# Include GStreamer-related settings and source files if USE_GSTREAMER is ON
    include(GStreamer)
    target_include_directories(VideoCaptureLib PRIVATE
        ${GSTREAMER_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
        src/gstreamer
    )
    target_link_libraries(VideoCaptureLib PRIVATE
        ${GSTREAMER_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GST_VIDEO_LIBRARIES}
    )
endif()

# Link against other required libraries
target_link_libraries(VideoCaptureLib PRIVATE
    ${OpenCV_LIBS}
    spdlog::spdlog_header_only
)
