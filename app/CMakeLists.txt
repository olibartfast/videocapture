cmake_minimum_required(VERSION 3.20)
project(VideoCaptureApp)

set(CMAKE_CXX_STANDARD 17)

# Add executable
add_executable(${PROJECT_NAME} main.cpp)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/../include
)

if(VIDEOCAPTURE_BACKEND STREQUAL "OpenCV")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE VIDEOCAPTURE_USE_OPENCV)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    VideoCapture
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
