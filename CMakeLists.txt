cmake_minimum_required(VERSION 3.10)
project(VideoCapture)

# Unset cache variables
unset(GSTREAMER_INCLUDE_DIRS CACHE)
unset(GST_APP_INCLUDE_DIRS CACHE)
unset(GST_VIDEO_INCLUDE_DIRS CACHE)
unset(GSTREAMER_LIBRARIES CACHE)
unset(GST_APP_LIBRARIES CACHE)
unset(GST_VIDEO_LIBRARIES CACHE)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenCV REQUIRED)

# Define an option to enable or disable GStreamer support
if(NOT DEFINED USE_GSTREAMER)
    unset(USE_GSTREAMER CACHE)
endif()
set(USE_GSTREAMER OFF CACHE BOOL "Use GStreamer for video capture (optional)")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
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
endif()

# Link against other required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
)

# Add subdirectory for the application
add_subdirectory(app)