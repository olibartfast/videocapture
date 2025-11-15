cmake_minimum_required(VERSION 3.20)
project(VideoCaptureApp)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(OpenCV REQUIRED)

# Add executable
add_executable(${PROJECT_NAME} main.cpp)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../src
    ${PROJECT_SOURCE_DIR}/../src/opencv
    ${OpenCV_INCLUDE_DIRS}
)

# Add OpenCV include directories from found package
if(OpenCV_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
else()
    target_include_directories(${PROJECT_NAME} PRIVATE /usr/include/opencv4)
endif()

# GStreamer support (if enabled in the main project)
if(USE_GSTREAMER)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
    pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
    pkg_check_modules(GLIB REQUIRED glib-2.0)
    
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/../src/gstreamer
    )
endif()

# FFmpeg support (if enabled in the main project)
if(USE_FFMPEG)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/../src/ffmpeg
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    VideoCapture
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)