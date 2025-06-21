# Centralized version management for VideoCapture library
# This file should be the single source of truth for all video processing dependencies

# Video Processing Library Versions
set(GSTREAMER_VERSION "1.20.0" CACHE STRING "GStreamer version")
set(OPENCV_MIN_VERSION "4.6.0" CACHE STRING "Minimum OpenCV version")

# System Dependencies (minimum versions)
set(CMAKE_MIN_VERSION "3.10" CACHE STRING "Minimum CMake version")
set(CXX_STANDARD "20" CACHE STRING "C++ standard version")

# Platform-specific paths (with fallbacks)
if(WIN32)
    set(DEFAULT_DEPENDENCY_ROOT "$ENV{USERPROFILE}/dependencies" CACHE PATH "Default dependency installation root")
    set(GSTREAMER_ROOT_DIR "C:/gstreamer/1.0/msvc_x86_64" CACHE PATH "GStreamer root directory")
else()
    set(DEFAULT_DEPENDENCY_ROOT "$ENV{HOME}/dependencies" CACHE PATH "Default dependency installation root")
    set(GSTREAMER_ROOT_DIR "/usr" CACHE PATH "GStreamer root directory")
endif()

# GStreamer-specific paths
set(GSTREAMER_INCLUDE_DIRS "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0" CACHE PATH "GStreamer include directories")
set(GST_APP_INCLUDE_DIRS "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0" CACHE PATH "GStreamer App include directories")
set(GST_VIDEO_INCLUDE_DIRS "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0" CACHE PATH "GStreamer Video include directories")

# GLib paths
set(GLIB_INCLUDE_DIRS 
    "${GSTREAMER_ROOT_DIR}/include/glib-2.0"
    "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/glib-2.0/include"
    CACHE PATH "GLib include directories"
)

# Library paths (platform-specific)
if(WIN32)
    set(GSTREAMER_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/libgstreamer-1.0.lib" CACHE PATH "GStreamer libraries")
    set(GST_APP_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/libgstapp-1.0.lib" CACHE PATH "GStreamer App libraries")
    set(GST_VIDEO_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/libgstvideo-1.0.lib" CACHE PATH "GStreamer Video libraries")
    set(GLIB_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/libglib-2.0.lib" CACHE PATH "GLib libraries")
else()
    set(GSTREAMER_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libgstreamer-1.0.so" CACHE PATH "GStreamer libraries")
    set(GST_APP_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libgstapp-1.0.so" CACHE PATH "GStreamer App libraries")
    set(GST_VIDEO_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libgstvideo-1.0.so" CACHE PATH "GStreamer Video libraries")
    set(GLIB_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libglib-2.0.so" CACHE PATH "GLib libraries")
endif()

# Print version information for debugging
message(STATUS "=== VideoCapture Dependency Versions ===")
message(STATUS "GStreamer: ${GSTREAMER_VERSION}")
message(STATUS "OpenCV (min): ${OPENCV_MIN_VERSION}")
message(STATUS "CMake (min): ${CMAKE_MIN_VERSION}")
message(STATUS "C++ Standard: ${CXX_STANDARD}")
message(STATUS "GStreamer Root: ${GSTREAMER_ROOT_DIR}")
message(STATUS "Dependency Root: ${DEFAULT_DEPENDENCY_ROOT}") 