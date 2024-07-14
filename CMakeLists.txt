cmake_minimum_required(VERSION 3.10)
project(VideoCapture)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

# Define an option to enable or disable GStreamer support
unset(USE_GSTREAMER CACHE)
option(USE_GSTREAMER "Use GStreamer for video capture (optional)" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
message(STATUS "Videocapture Cmake module path: ${CMAKE_MODULE_PATH}")

# Add source files for video capture
set(VIDEOCAPTURE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/VideoCaptureInterface.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/VideoCaptureFactory.cpp
)

# Create the video capture library
add_library(${PROJECT_NAME} SHARED ${VIDEOCAPTURE_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Link against GStreamer libraries if USE_GSTREAMER is ON
if (USE_GSTREAMER)
    include(GStreamer)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/src/gstreamer
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GSTREAMER_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GST_VIDEO_LIBRARIES}
    )
endif()

# Link against other required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
)

# Add subdirectory for the application
add_subdirectory(app)