set(VIDEOCAPTURE_APP_RENDERER "AUTO" CACHE STRING
    "Sample app renderer: AUTO, OPENCV, SDL2, GLFW, or SOKOL")
set_property(CACHE VIDEOCAPTURE_APP_RENDERER PROPERTY STRINGS
             AUTO OPENCV SDL2 GLFW SOKOL)
option(VIDEOCAPTURE_FETCH_APP_DEPENDENCIES
       "Fetch missing sample-app-only renderer dependencies" OFF)
set(SOKOL_ROOT "" CACHE PATH "Path containing the Sokol headers")

string(TOUPPER "${VIDEOCAPTURE_APP_RENDERER}" VIDEOCAPTURE_APP_RENDERER_RESOLVED)
if(VIDEOCAPTURE_APP_RENDERER_RESOLVED STREQUAL "AUTO")
    if(VIDEOCAPTURE_BACKEND STREQUAL "OpenCV")
        set(VIDEOCAPTURE_APP_RENDERER_RESOLVED "OPENCV")
    else()
        set(VIDEOCAPTURE_APP_RENDERER_RESOLVED "SDL2")
    endif()
endif()

set(VIDEOCAPTURE_APP_RENDERERS OPENCV SDL2 GLFW SOKOL)
if(NOT VIDEOCAPTURE_APP_RENDERER_RESOLVED IN_LIST VIDEOCAPTURE_APP_RENDERERS)
    message(FATAL_ERROR
        "Invalid VIDEOCAPTURE_APP_RENDERER='${VIDEOCAPTURE_APP_RENDERER}'. "
        "Choose AUTO, OPENCV, SDL2, GLFW, or SOKOL.")
endif()
message(STATUS "VideoCapture sample app renderer: ${VIDEOCAPTURE_APP_RENDERER_RESOLVED}")

add_library(VideoCaptureAppRenderer STATIC
    Renderer.cpp
    RendererFactory.cpp
)
target_include_directories(VideoCaptureAppRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(VideoCaptureAppRenderer PUBLIC VideoCapture)
target_compile_features(VideoCaptureAppRenderer PUBLIC cxx_std_17)

if(VIDEOCAPTURE_APP_RENDERER_RESOLVED STREQUAL "OPENCV")
    if(NOT OpenCV_highgui_FOUND)
        find_package(OpenCV REQUIRED COMPONENTS core highgui)
    endif()
    target_sources(VideoCaptureAppRenderer PRIVATE OpenCvRenderer.cpp)
    target_include_directories(VideoCaptureAppRenderer PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(VideoCaptureAppRenderer PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(VideoCaptureAppRenderer PRIVATE
        VIDEOCAPTURE_APP_RENDERER_OPENCV)
elseif(VIDEOCAPTURE_APP_RENDERER_RESOLVED STREQUAL "SDL2")
    find_package(SDL2 CONFIG QUIET)
    add_library(VideoCaptureAppSDL2 INTERFACE)
    if(TARGET SDL2::SDL2)
        target_link_libraries(VideoCaptureAppSDL2 INTERFACE SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
        target_link_libraries(VideoCaptureAppSDL2 INTERFACE SDL2::SDL2-static)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
        target_link_libraries(VideoCaptureAppSDL2 INTERFACE PkgConfig::SDL2)
    endif()
    target_sources(VideoCaptureAppRenderer PRIVATE SdlRenderer.cpp)
    target_link_libraries(VideoCaptureAppRenderer PRIVATE VideoCaptureAppSDL2)
    target_compile_definitions(VideoCaptureAppRenderer PRIVATE
        VIDEOCAPTURE_APP_RENDERER_SDL2)
elseif(VIDEOCAPTURE_APP_RENDERER_RESOLVED STREQUAL "GLFW")
    find_package(glfw3 3.3 CONFIG QUIET)
    if(NOT TARGET glfw AND NOT TARGET glfw3::glfw)
        if(NOT VIDEOCAPTURE_FETCH_APP_DEPENDENCIES)
            message(FATAL_ERROR
                "GLFW was not found. Install GLFW development files or configure with "
                "-DVIDEOCAPTURE_FETCH_APP_DEPENDENCIES=ON.")
        endif()
        include(FetchContent)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.5.1
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(glfw)
    endif()
    find_package(OpenGL REQUIRED)
    add_library(VideoCaptureAppGLFW INTERFACE)
    if(TARGET glfw3::glfw)
        target_link_libraries(VideoCaptureAppGLFW INTERFACE glfw3::glfw)
    else()
        target_link_libraries(VideoCaptureAppGLFW INTERFACE glfw)
    endif()
    target_link_libraries(VideoCaptureAppGLFW INTERFACE OpenGL::GL)
    target_sources(VideoCaptureAppRenderer PRIVATE GlfwRenderer.cpp)
    target_link_libraries(VideoCaptureAppRenderer PRIVATE VideoCaptureAppGLFW)
    target_compile_definitions(VideoCaptureAppRenderer PRIVATE
        VIDEOCAPTURE_APP_RENDERER_GLFW)
elseif(VIDEOCAPTURE_APP_RENDERER_RESOLVED STREQUAL "SOKOL")
    find_path(SOKOL_INCLUDE_DIR sokol_app.h
        HINTS ${SOKOL_ROOT} ${SOKOL_ROOT}/include
        NO_DEFAULT_PATH)
    if(NOT SOKOL_INCLUDE_DIR)
        find_path(SOKOL_INCLUDE_DIR sokol_app.h)
    endif()
    if(NOT SOKOL_INCLUDE_DIR)
        if(NOT VIDEOCAPTURE_FETCH_APP_DEPENDENCIES)
            message(FATAL_ERROR
                "Sokol headers were not found. Set SOKOL_ROOT or configure with "
                "-DVIDEOCAPTURE_FETCH_APP_DEPENDENCIES=ON.")
        endif()
        include(FetchContent)
        FetchContent_Declare(sokol
            GIT_REPOSITORY https://github.com/floooh/sokol.git
            GIT_TAG 1847290135f95e57e6d220b0a41208306aafc0dd
        )
        FetchContent_MakeAvailable(sokol)
        FetchContent_GetProperties(sokol SOURCE_DIR SOKOL_INCLUDE_DIR)
    endif()

    target_sources(VideoCaptureAppRenderer PRIVATE
        SokolImplementation.c
        SokolRenderer.cpp
    )
    target_include_directories(VideoCaptureAppRenderer PRIVATE ${SOKOL_INCLUDE_DIR})
    target_compile_definitions(VideoCaptureAppRenderer PRIVATE
        VIDEOCAPTURE_APP_RENDERER_SOKOL)
    if(APPLE)
        enable_language(OBJC)
        set_source_files_properties(SokolImplementation.c PROPERTIES LANGUAGE OBJC)
        target_compile_definitions(VideoCaptureAppRenderer PRIVATE SOKOL_METAL)
        target_link_libraries(VideoCaptureAppRenderer PRIVATE
            "-framework AppKit" "-framework AudioToolbox" "-framework Metal"
            "-framework MetalKit" "-framework QuartzCore")
    elseif(WIN32)
        target_compile_definitions(VideoCaptureAppRenderer PRIVATE SOKOL_D3D11)
        target_link_libraries(VideoCaptureAppRenderer PRIVATE d3d11 dxgi)
    elseif(UNIX)
        find_package(OpenGL REQUIRED)
        find_package(Threads REQUIRED)
        find_package(X11 REQUIRED)
        target_compile_definitions(VideoCaptureAppRenderer PRIVATE SOKOL_GLCORE)
        target_link_libraries(VideoCaptureAppRenderer PRIVATE
            OpenGL::GL X11::X11 Xi Xcursor Threads::Threads ${CMAKE_DL_LIBS} m)
    else()
        message(FATAL_ERROR "The Sokol sample renderer is unsupported on this platform")
    endif()
endif()

add_executable(VideoCaptureApp main.cpp)
target_link_libraries(VideoCaptureApp PRIVATE VideoCaptureAppRenderer)
set_target_properties(VideoCaptureApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
