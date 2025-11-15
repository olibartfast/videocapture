cmake_minimum_required(VERSION 3.10)
project(VideoCapture)

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

# Find OpenCV
find_package(OpenCV REQUIRED)

# Validate dependencies before proceeding
validate_all_dependencies()

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

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake})
message(STATUS "Videocapture Cmake module path: ${CMAKE_MODULE_PATH}")

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
if (USE_FFMPEG)
    list(APPEND VIDEOCAPTURE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ffmpeg/FFmpegCapture.cpp
    )
endif()

# Create the video capture library
add_library(${PROJECT_NAME} SHARED ${VIDEOCAPTURE_SOURCES})

# Include directories - OpenCV4 uses opencv2 directory structure under opencv4/
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src/opencv
)

# Add OpenCV include directories from found package
if(OpenCV_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
else()
    # Fallback for systems where OpenCV doesn't set this variable properly
    target_include_directories(${PROJECT_NAME} PUBLIC /usr/include/opencv4)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Link against GStreamer libraries if USE_GSTREAMER is ON
message(STATUS "USE_GSTREAMER value: ${USE_GSTREAMER}")
if (USE_GSTREAMER)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/GStreamer.cmake)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC
        ${GSTREAMER_LIBRARIES}
    )
    
    # Test GStreamer compilation
    test_gstreamer_compilation()
endif()

# Link against FFmpeg libraries if USE_FFMPEG is ON
message(STATUS "USE_FFMPEG value: ${USE_FFMPEG}")
if (USE_FFMPEG)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/FFmpeg.cmake)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/src/ffmpeg
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC
        ${FFMPEG_LIBRARIES}
    )
endif()

# Link against other required libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${OpenCV_LIBS}
)

# Add subdirectory for the application
add_subdirectory(app)

# Add testing support
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()