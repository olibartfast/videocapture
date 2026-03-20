include_guard(GLOBAL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
    glib-2.0
)

message(STATUS "GStreamer backend enabled through pkg-config")
