# Test Configuration for VideoCapture Library
cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)
include(GoogleTest)

# Common test sources
set(TEST_SOURCES
    test_main.cpp
    test_factory.cpp
    test_opencv.cpp
)

# Add backend-specific tests if enabled
if(USE_GSTREAMER)
    list(APPEND TEST_SOURCES test_gstreamer.cpp)
endif()

if(USE_FFMPEG)
    list(APPEND TEST_SOURCES test_ffmpeg.cpp)
endif()

# Create test executable
add_executable(VideoCaptureTests ${TEST_SOURCES})

# Link against the library and GTest
target_link_libraries(VideoCaptureTests
    PRIVATE
        VideoCapture
        GTest::GTest
        GTest::Main
        ${OpenCV_LIBS}
        curl  # Fix for curl library versioning issues with OpenCV dependencies
)

# Add include directories
target_include_directories(VideoCaptureTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/opencv
)

# Add backend-specific include directories
if(USE_GSTREAMER)
    target_include_directories(VideoCaptureTests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/gstreamer
            ${GSTREAMER_INCLUDE_DIRS}
    )
    target_link_libraries(VideoCaptureTests
        PRIVATE
            ${GSTREAMER_LIBRARIES}
    )
    target_compile_definitions(VideoCaptureTests PRIVATE USE_GSTREAMER)
endif()

if(USE_FFMPEG)
    target_include_directories(VideoCaptureTests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/ffmpeg
            ${FFMPEG_INCLUDE_DIRS}
    )
    target_link_libraries(VideoCaptureTests
        PRIVATE
            ${FFMPEG_LIBRARIES}
    )
    target_compile_definitions(VideoCaptureTests PRIVATE USE_FFMPEG)
endif()

# Add OpenCV include directories
if(OpenCV_INCLUDE_DIRS)
    target_include_directories(VideoCaptureTests PRIVATE ${OpenCV_INCLUDE_DIRS})
else()
    target_include_directories(VideoCaptureTests PRIVATE /usr/include/opencv4)
endif()

# Discover tests (skip discovery if binary can't run due to library issues)
gtest_discover_tests(VideoCaptureTests
    DISCOVERY_MODE PRE_TEST
)

# Add custom test target
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS VideoCaptureTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
