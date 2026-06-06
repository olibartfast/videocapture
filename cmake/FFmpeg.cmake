# FFmpeg Configuration for VideoCapture
# Uses centralized version management from cmake/versions.cmake

# Find FFmpeg packages
find_package(PkgConfig REQUIRED)

# Find FFmpeg libraries as imported targets. The targets carry pkg-config's
# include dirs and library search paths, so headers and libs resolve on Linux
# and on macOS Homebrew (/usr/local or /opt/homebrew) without manual paths.
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat>=${FFMPEG_VERSION})
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec>=${FFMPEG_VERSION})
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil>=${FFMPEG_VERSION})
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale>=${FFMPEG_VERSION})

# Combine all FFmpeg include directories (also exposed transitively by targets)
set(FFMPEG_INCLUDE_DIRS
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWSCALE_INCLUDE_DIRS}
)

# Link against the imported targets so library search paths resolve everywhere
set(FFMPEG_LIBRARIES
    PkgConfig::AVFORMAT
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
)

# Print the include directories and libraries for debugging
message(STATUS "FFmpeg version: ${FFMPEG_VERSION}")
message(STATUS "FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "FFMPEG_LIBRARIES: ${FFMPEG_LIBRARIES}")

# Define a compile definition to indicate FFmpeg usage
add_compile_definitions(USE_FFMPEG)
