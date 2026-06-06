# Centralized version management for VideoCapture library
# This file should be the single source of truth for all video processing dependencies

# Video Processing Library Versions
set(GSTREAMER_VERSION "1.20.0" CACHE STRING "GStreamer version")
set(OPENCV_MIN_VERSION "4.6.0" CACHE STRING "Minimum OpenCV version")
set(FFMPEG_VERSION "4.4" CACHE STRING "Minimum FFmpeg version")

# System Dependencies (minimum versions)
set(CMAKE_MIN_VERSION "3.10" CACHE STRING "Minimum CMake version")
set(CXX_STANDARD "20" CACHE STRING "C++ standard version")

# Default dependency installation root (used by setup scripts)
if(WIN32)
    set(DEFAULT_DEPENDENCY_ROOT "$ENV{USERPROFILE}/dependencies" CACHE PATH "Default dependency installation root")
else()
    set(DEFAULT_DEPENDENCY_ROOT "$ENV{HOME}/dependencies" CACHE PATH "Default dependency installation root")
endif()

# NOTE: GStreamer / FFmpeg include and library paths are discovered at
# configure time via pkg-config (see cmake/GStreamer.cmake and cmake/FFmpeg.cmake).
# They are intentionally NOT hardcoded here so the build is portable across
# Linux and macOS (Intel /usr/local and Apple Silicon /opt/homebrew).

# Print version information for debugging
message(STATUS "=== VideoCapture Dependency Versions ===")
message(STATUS "GStreamer: ${GSTREAMER_VERSION}")
message(STATUS "OpenCV (min): ${OPENCV_MIN_VERSION}")
message(STATUS "FFmpeg (min): ${FFMPEG_VERSION}")
message(STATUS "CMake (min): ${CMAKE_MIN_VERSION}")
message(STATUS "C++ Standard: ${CXX_STANDARD}")
message(STATUS "Dependency Root: ${DEFAULT_DEPENDENCY_ROOT}") 