# GStreamer Configuration for VideoCapture
# Uses centralized version management from cmake/versions.cmake

# GStreamer configuration using centralized version management
message(STATUS "GStreamer version: ${GSTREAMER_VERSION}")
message(STATUS "GStreamer root directory: ${GSTREAMER_ROOT_DIR}")

# Print the include directories for debugging
message(STATUS "GSTREAMER_INCLUDE_DIRS: ${GSTREAMER_INCLUDE_DIRS}")
message(STATUS "GST_APP_INCLUDE_DIRS: ${GST_APP_INCLUDE_DIRS}")
message(STATUS "GST_VIDEO_INCLUDE_DIRS: ${GST_VIDEO_INCLUDE_DIRS}")
message(STATUS "GLIB_INCLUDE_DIRS: ${GLIB_INCLUDE_DIRS}")

message(STATUS "GSTREAMER_LIBRARIES: ${GSTREAMER_LIBRARIES}")
message(STATUS "GST_APP_LIBRARIES: ${GST_APP_LIBRARIES}")
message(STATUS "GST_VIDEO_LIBRARIES: ${GST_VIDEO_LIBRARIES}")
message(STATUS "GLIB_LIBRARIES: ${GLIB_LIBRARIES}")

# Define a compile definition to indicate GStreamer usage
add_compile_definitions(USE_GSTREAMER)

# Note: Version management is now handled centrally in cmake/versions.cmake
