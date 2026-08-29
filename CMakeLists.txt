cmake_minimum_required(VERSION 3.10)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_RAW)
string(STRIP "${PROJECT_VERSION_RAW}" PROJECT_VERSION_RAW)
# Extract semver (strip -dev or other suffixes)
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" PROJECT_SEMVER "${PROJECT_VERSION_RAW}")

project(VideoCapture VERSION ${PROJECT_SEMVER})

include(cmake/CompileSpeed.cmake)

# Include centralized version management first
include(cmake/versions.cmake)

# Include dependency validation
include(cmake/DependencyValidation.cmake)

# Unset cache variables
unset(GSTREAMER_INCLUDE_DIRS CACHE)
unset(GST_APP_INCLUDE_DIRS CACHE)
unset(GST_VIDEO_INCLUDE_DIRS CACHE)
unset(GSTREAMER_LIBRARIES CACHE)
unset(GST_APP_LIBRARIES CACHE)
unset(GST_VIDEO_LIBRARIES CACHE)

# Set C++ standard from centralized version management
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

# Define an option to enable or disable GStreamer support
if(NOT DEFINED USE_GSTREAMER)
    unset(USE_GSTREAMER CACHE)
endif()
set(USE_GSTREAMER OFF CACHE BOOL "Use GStreamer for video capture (optional)")

# Define an option to enable or disable FFmpeg support
if(NOT DEFINED USE_FFMPEG)
    unset(USE_FFMPEG CACHE)
endif()
set(USE_FFMPEG OFF CACHE BOOL "Use FFmpeg for video capture (optional)")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
message(STATUS "Videocapture Cmake module path: ${CMAKE_MODULE_PATH}")

# FFmpeg retains priority over GStreamer, and OpenCV remains the default when
# neither optional backend is enabled.
if(USE_FFMPEG)
    set(VIDEOCAPTURE_BACKEND "FFmpeg")
elseif(USE_GSTREAMER)
    set(VIDEOCAPTURE_BACKEND "GStreamer")
else()
    set(VIDEOCAPTURE_BACKEND "OpenCV")
endif()
message(STATUS "VideoCapture backend: ${VIDEOCAPTURE_BACKEND}")

# Discover only the dependencies used by enabled backends. OpenCV is not part
# of the public frame API and is needed only by the OpenCV backend and demo.
if(USE_GSTREAMER)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/GStreamer.cmake)
endif()
if(USE_FFMPEG)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/FFmpeg.cmake)
endif()
if(VIDEOCAPTURE_BACKEND STREQUAL "OpenCV")
    set(OPENCV_COMPONENTS core imgproc videoio)
    if(PROJECT_IS_TOP_LEVEL)
        list(APPEND OPENCV_COMPONENTS highgui)
    endif()
    find_package(OpenCV ${OPENCV_MIN_VERSION} REQUIRED COMPONENTS ${OPENCV_COMPONENTS})
endif()

# Validate dependencies before proceeding
validate_all_dependencies()

# Add source files for video capture
set(VIDEOCAPTURE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/VideoCaptureFactory.cpp
)
if (USE_GSTREAMER)
    list(APPEND VIDEOCAPTURE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer/GStreamerCapture.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer/GStreamerPipeline.cpp
    )
endif()
if (USE_FFMPEG)
    list(APPEND VIDEOCAPTURE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ffmpeg/FFmpegCapture.cpp
    )
endif()
if(VIDEOCAPTURE_BACKEND STREQUAL "OpenCV")
    list(APPEND VIDEOCAPTURE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/opencv/OpenCVCapture.cpp
    )
endif()

# Create the video capture library
add_library(${PROJECT_NAME} SHARED ${VIDEOCAPTURE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Link against GStreamer libraries if USE_GSTREAMER is ON
message(STATUS "USE_GSTREAMER value: ${USE_GSTREAMER}")
if (USE_GSTREAMER)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_LIBRARIES}
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GSTREAMER)
    
    # Test GStreamer compilation
    test_gstreamer_compilation()
endif()

# Link against FFmpeg libraries if USE_FFMPEG is ON
message(STATUS "USE_FFMPEG value: ${USE_FFMPEG}")
if (USE_FFMPEG)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/src/ffmpeg
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FFMPEG_LIBRARIES}
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_FFMPEG)
endif()

if(VIDEOCAPTURE_BACKEND STREQUAL "OpenCV")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/opencv
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
endif()

# Sample app and tests are only built when videocapture is the top-level project.
if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(app)

    option(BUILD_TESTS "Build unit tests" ON)
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()
