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

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src/opencv
    ${OpenCV_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Link against GStreamer libraries if USE_GSTREAMER is ON
message(STATUS "USE_GSTREAMER value: ${USE_GSTREAMER}")
if (USE_GSTREAMER)
    include(GStreamer)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GST_VIDEO_LIBRARIES}
        ${GLIB_LIBRARIES}
    )
    
    # Test GStreamer compilation
    test_gstreamer_compilation()
endif()

# Link against FFmpeg libraries if USE_FFMPEG is ON
message(STATUS "USE_FFMPEG value: ${USE_FFMPEG}")
if (USE_FFMPEG)
    include(FFmpeg)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/src/ffmpeg
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FFMPEG_LIBRARIES}
    )
endif()

# Link against other required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
)

# Add subdirectory for the application
add_subdirectory(app)