# GStreamer Configuration for VideoCapture
# Uses centralized version management from cmake/versions.cmake

# Find GStreamer using pkg-config
find_package(PkgConfig REQUIRED)

# Find GStreamer libraries as imported targets. Imported targets carry the
# include dirs, library search paths and link flags reported by pkg-config,
# which keeps discovery portable across Linux (/usr) and macOS Homebrew
# (/usr/local on Intel, /opt/homebrew on Apple Silicon).
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0>=${GSTREAMER_VERSION})
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=${GSTREAMER_VERSION})
pkg_check_modules(GST_VIDEO REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=${GSTREAMER_VERSION})
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

# Aggregate include directories (also exposed transitively by the targets below)
set(GSTREAMER_INCLUDE_DIRS
    ${GSTREAMER_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

# Link against the imported targets so library search paths resolve everywhere
set(GSTREAMER_LIBRARIES
    PkgConfig::GSTREAMER
    PkgConfig::GST_APP
    PkgConfig::GST_VIDEO
    PkgConfig::GLIB
)

# GStreamer configuration using centralized version management
message(STATUS "GStreamer version: ${GSTREAMER_VERSION}")
message(STATUS "GSTREAMER_INCLUDE_DIRS: ${GSTREAMER_INCLUDE_DIRS}")
message(STATUS "GSTREAMER_LIBRARIES: ${GSTREAMER_LIBRARIES}")

# Define a compile definition to indicate GStreamer usage
add_compile_definitions(USE_GSTREAMER)
