# FFmpeg Configuration for VideoCapture
# Uses centralized version management from cmake/versions.cmake

# Find FFmpeg packages
find_package(PkgConfig REQUIRED)

# Find FFmpeg libraries
pkg_check_modules(AVFORMAT REQUIRED libavformat>=${FFMPEG_VERSION})
pkg_check_modules(AVCODEC REQUIRED libavcodec>=${FFMPEG_VERSION})
pkg_check_modules(AVUTIL REQUIRED libavutil>=${FFMPEG_VERSION})
pkg_check_modules(SWSCALE REQUIRED libswscale>=${FFMPEG_VERSION})

# Combine all FFmpeg include directories and libraries
set(FFMPEG_INCLUDE_DIRS
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWSCALE_INCLUDE_DIRS}
)

set(FFMPEG_LIBRARIES
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWSCALE_LIBRARIES}
)

# Print the include directories and libraries for debugging
message(STATUS "FFmpeg version: ${FFMPEG_VERSION}")
message(STATUS "FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "FFMPEG_LIBRARIES: ${FFMPEG_LIBRARIES}")

# Define a compile definition to indicate FFmpeg usage
add_compile_definitions(USE_FFMPEG)
