cmake_minimum_required(VERSION 3.10)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_RAW)
string(STRIP "${PROJECT_VERSION_RAW}" PROJECT_VERSION_RAW)
# Extract semver (strip -dev or other suffixes)
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" PROJECT_SEMVER "${PROJECT_VERSION_RAW}")

project(VideoCapture VERSION ${PROJECT_SEMVER} LANGUAGES CXX)

option(USE_GSTREAMER "Use GStreamer for video capture (optional)" OFF)

if(WIN32 AND USE_GSTREAMER)
    message(WARNING "GStreamer support is not configured for Windows in this repository; building without it.")
    set(USE_GSTREAMER OFF CACHE BOOL "Use GStreamer for video capture (optional)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

# Add source files for video capture
set(VIDEOCAPTURE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/VideoCaptureFactory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/opencv/OpenCVCapture.cpp
)
if (USE_GSTREAMER)
    list(APPEND VIDEOCAPTURE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer/GStreamerCapture.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer/GStreamerOpenCV.cpp
    )
endif()

# Create the video capture library
add_library(${PROJECT_NAME} SHARED ${VIDEOCAPTURE_SOURCES})

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src/opencv
    ${OpenCV_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Link against required libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${OpenCV_LIBS}
)

# Link against GStreamer libraries if USE_GSTREAMER is ON
message(STATUS "USE_GSTREAMER value: ${USE_GSTREAMER}")
if(USE_GSTREAMER)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/GStreamer.cmake)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USE_GSTREAMER)
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC
        PkgConfig::GSTREAMER
    )
endif()

# Add subdirectory for the application
add_subdirectory(app)
