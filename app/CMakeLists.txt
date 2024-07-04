cmake_minimum_required(VERSION 3.20)
project(VideoCaptureApp)

set(CMAKE_CXX_STANDARD 17)

# Add executable
add_executable(${PROJECT_NAME} main.cpp)

# Include directories and link libraries
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    VideoCapture
    ${OpenCV_LIBS}
)