# GStreamer Configuration for VideoCapture
# Uses centralized version management from cmake/versions.cmake

# Find GStreamer using pkg-config
find_package(PkgConfig REQUIRED)

# Find GStreamer libraries
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=${GSTREAMER_VERSION})
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=${GSTREAMER_VERSION})
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0>=${GSTREAMER_VERSION})
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Update GStreamer version to found version
set(GSTREAMER_VERSION "${GSTREAMER_VERSION}" PARENT_SCOPE)

# Combine all include directories
set(GSTREAMER_INCLUDE_DIRS 
    ${GSTREAMER_INCLUDE_DIRS} 
    ${GST_APP_INCLUDE_DIRS} 
    ${GST_VIDEO_INCLUDE_DIRS} 
    ${GLIB_INCLUDE_DIRS}
    PARENT_SCOPE
)

# Combine all libraries
set(GSTREAMER_LIBRARIES 
    ${GSTREAMER_LIBRARIES} 
    ${GST_APP_LIBRARIES} 
    ${GST_VIDEO_LIBRARIES} 
    ${GLIB_LIBRARIES}
    PARENT_SCOPE
)

# GStreamer configuration using centralized version management
message(STATUS "GStreamer version: ${GSTREAMER_VERSION}")
message(STATUS "GSTREAMER_INCLUDE_DIRS: ${GSTREAMER_INCLUDE_DIRS}")
message(STATUS "GSTREAMER_LIBRARIES: ${GSTREAMER_LIBRARIES}")

# Define a compile definition to indicate GStreamer usage
add_compile_definitions(USE_GSTREAMER)
