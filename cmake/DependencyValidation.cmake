# Dependency validation and setup utilities for VideoCapture library
# This module provides functions to validate and setup video processing dependencies

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

# Function to validate a dependency exists
function(validate_dependency dependency_name dependency_path)
    if(NOT EXISTS "${dependency_path}")
        message(FATAL_ERROR "${dependency_name} not found at ${dependency_path}. 
        Please ensure the dependency is properly installed or run the setup script.")
    endif()
    
    message(STATUS "✓ ${dependency_name} found at ${dependency_path}")
endfunction()

# Function to validate OpenCV
function(validate_opencv)
    find_package(OpenCV REQUIRED)
    if(OpenCV_VERSION VERSION_LESS OPENCV_MIN_VERSION)
        message(FATAL_ERROR "OpenCV version ${OpenCV_VERSION} is too old. Minimum required: ${OPENCV_MIN_VERSION}")
    endif()
    message(STATUS "✓ OpenCV ${OpenCV_VERSION} found")
endfunction()

# Function to validate GStreamer
function(validate_gstreamer)
    if(USE_GSTREAMER)
        # Check for GStreamer installation
        if(NOT EXISTS "${GSTREAMER_ROOT_DIR}")
            message(FATAL_ERROR "GStreamer root directory not found at ${GSTREAMER_ROOT_DIR}")
        endif()
        
        # Check for required GStreamer files
        set(required_files
            "${GSTREAMER_INCLUDE_DIRS}"
            "${GSTREAMER_LIBRARIES}"
            "${GST_APP_LIBRARIES}"
            "${GST_VIDEO_LIBRARIES}"
        )
        
        foreach(file ${required_files})
            if(NOT EXISTS "${file}")
                message(FATAL_ERROR "GStreamer installation incomplete. Missing: ${file}")
            endif()
        endforeach()
        
        # Check for GLib
        foreach(file ${GLIB_INCLUDE_DIRS})
            if(NOT EXISTS "${file}")
                message(FATAL_ERROR "GLib include directory not found: ${file}")
            endif()
        endforeach()
        
        if(NOT EXISTS "${GLIB_LIBRARIES}")
            message(FATAL_ERROR "GLib library not found: ${GLIB_LIBRARIES}")
        endif()
        
        message(STATUS "✓ GStreamer ${GSTREAMER_VERSION} found")
    endif()
endfunction()

# Function to validate system dependencies
function(validate_system_dependencies)
    # Validate CMake version
    if(CMAKE_VERSION VERSION_LESS CMAKE_MIN_VERSION)
        message(FATAL_ERROR "CMake version ${CMAKE_VERSION} is too old. Minimum required: ${CMAKE_MIN_VERSION}")
    endif()
    message(STATUS "✓ CMake ${CMAKE_VERSION} found")
    
    # Validate C++ standard support
    set(CMAKE_CXX_STANDARD ${CXX_STANDARD})
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    message(STATUS "✓ C++${CXX_STANDARD} standard set")
endfunction()

# Function to validate all dependencies
function(validate_all_dependencies)
    message(STATUS "=== Validating VideoCapture Dependencies ===")
    
    validate_system_dependencies()
    validate_opencv()
    validate_gstreamer()
    
    message(STATUS "=== All VideoCapture Dependencies Validated Successfully ===")
endfunction()

# Function to check if we're in a Docker environment
function(is_docker_environment result)
    if(EXISTS "/.dockerenv")
        set(${result} TRUE PARENT_SCOPE)
    else()
        set(${result} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Function to provide helpful setup instructions
function(print_setup_instructions)
    message(STATUS "=== Setup Instructions ===")
    message(STATUS "If video processing dependencies are missing, run the following commands:")
    message(STATUS "")
    
    if(USE_GSTREAMER)
        message(STATUS "  ./scripts/setup_gstreamer.sh")
    endif()
    
    message(STATUS "")
    message(STATUS "Or run the unified setup script:")
    message(STATUS "  ./scripts/setup_dependencies.sh --gstreamer")
    message(STATUS "")
endfunction()

# Function to test GStreamer compilation
function(test_gstreamer_compilation)
    if(USE_GSTREAMER)
        set(CMAKE_REQUIRED_INCLUDES ${GSTREAMER_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
        set(CMAKE_REQUIRED_LIBRARIES ${GSTREAMER_LIBRARIES} ${GLIB_LIBRARIES})
        
        check_cxx_source_compiles(
            "#include <gstreamer-1.0/gst/gst.h>
             int main() { gst_init(NULL, NULL); return 0; }"
            GSTREAMER_COMPILES
        )
        
        if(NOT GSTREAMER_COMPILES)
            message(WARNING "GStreamer compilation test failed. GStreamer support may not work correctly.")
        else()
            message(STATUS "✓ GStreamer compilation test passed")
        endif()
    endif()
endfunction() 