# Include GStreamer-related settings and source files
set(GSTREAMER_ROOT_DIR "/usr" CACHE PATH "GStreamer root directory")
set(GSTREAMER_INCLUDE_DIRS "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0")
set(GST_APP_INCLUDE_DIRS "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0")
set(GST_VIDEO_INCLUDE_DIRS "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0")
set(GSTREAMER_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libgstreamer-1.0.so")
set(GST_APP_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libgstapp-1.0.so")
set(GST_VIDEO_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libgstvideo-1.0.so")


# Add GLib settings
set(GLIB_INCLUDE_DIRS 
    "${GSTREAMER_ROOT_DIR}/include/glib-2.0"
    "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/glib-2.0/include"
)
set(GLIB_LIBRARIES "${GSTREAMER_ROOT_DIR}/lib/x86_64-linux-gnu/libglib-2.0.so")

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
