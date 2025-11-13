# Dependency Management for VideoCapture

This document describes the improved dependency management system for the VideoCapture library, which provides a unified approach to managing video processing dependencies across multiple backends (OpenCV, GStreamer, FFmpeg).

## Overview

The VideoCapture library has been enhanced with a centralized dependency management system that provides:

- **Centralized Version Management**: All video processing library versions are managed in a single file
- **Multiple Backend Support**: OpenCV (default), GStreamer, and FFmpeg backends
- **Dependency Validation**: Automatic validation of installed dependencies
- **Unified Setup Scripts**: Easy installation of video processing dependencies
- **Backward Compatibility**: Existing scripts continue to work

## Architecture

### Version Management

All video processing library versions are centrally managed in `cmake/versions.cmake`:

```cmake
# Video Processing Library Versions
set(GSTREAMER_VERSION "1.20.0" CACHE STRING "GStreamer version")
set(OPENCV_MIN_VERSION "4.6.0" CACHE STRING "Minimum OpenCV version")
set(FFMPEG_VERSION "4.0" CACHE STRING "Minimum FFmpeg version")

# System Dependencies (minimum versions)
set(CMAKE_MIN_VERSION "3.10" CACHE STRING "Minimum CMake version")
set(CXX_STANDARD "20" CACHE STRING "C++ standard version")
```

### Dependency Validation

The `cmake/DependencyValidation.cmake` module provides comprehensive validation:

- **System Dependencies**: CMake version, C++ standard support
- **Video Processing Libraries**: OpenCV, GStreamer, FFmpeg
- **Installation Completeness**: Checks for required files and libraries
- **Compilation Testing**: Tests GStreamer compilation if enabled

### Setup Scripts

#### Unified Setup Script

The main setup script `scripts/setup_dependencies.sh` supports all video processing dependencies:

```bash
# Setup basic dependencies (OpenCV only)
./scripts/setup_dependencies.sh

# Setup with GStreamer support
./scripts/setup_dependencies.sh --gstreamer

# Setup with FFmpeg support
./scripts/setup_dependencies.sh --ffmpeg

# Setup with both GStreamer and FFmpeg
./scripts/setup_dependencies.sh --gstreamer --ffmpeg

# Setup with custom dependency root
./scripts/setup_dependencies.sh --gstreamer --ffmpeg --root /opt/dependencies
```

#### Individual Backend Scripts

For backward compatibility, individual scripts are available:

- `scripts/setup_gstreamer.sh`
- `scripts/setup_ffmpeg.sh`

## Usage

### Building with CMake

The CMakeLists.txt automatically includes version management and validation:

```cmake
# Include centralized version management first
include(cmake/versions.cmake)

# Include dependency validation
include(cmake/DependencyValidation.cmake)

# Validate dependencies before proceeding
validate_all_dependencies()
```

### Setting Up Dependencies

1. **Basic setup (OpenCV only)**:
   ```bash
   ./scripts/setup_dependencies.sh
   ```

2. **Setup with GStreamer**:
   ```bash
   ./scripts/setup_dependencies.sh --gstreamer
   ```

3. **Setup with FFmpeg**:
   ```bash
   ./scripts/setup_dependencies.sh --ffmpeg
   ```

4. **Setup with multiple backends**:
   ```bash
   ./scripts/setup_dependencies.sh --gstreamer --ffmpeg
   ```

5. **Set environment variables**:
   ```bash
   source $HOME/dependencies/setup_videocapture_env.sh
   ```

6. **Build the library**:
   ```bash
   mkdir build && cd build
   # With FFmpeg (takes priority over GStreamer)
   cmake .. -DUSE_FFMPEG=ON
   # Or with GStreamer
   cmake .. -DUSE_GSTREAMER=ON
   # Or with both (FFmpeg will be used)
   cmake .. -DUSE_GSTREAMER=ON -DUSE_FFMPEG=ON
   make
   ```

### Configuration Options

#### CMake Variables

- `USE_GSTREAMER`: Enable GStreamer support (ON/OFF)
- `DEPENDENCY_ROOT`: Set custom dependency installation root
- `GSTREAMER_VERSION`: Override GStreamer version
- `OPENCV_MIN_VERSION`: Override minimum OpenCV version
- `CXX_STANDARD`: Override C++ standard version

#### Environment Variables

The setup script creates environment variables for video processing libraries:

```bash
export DEPENDENCY_ROOT="$HOME/dependencies"

# OpenCV paths (if installed in custom location)
if [[ -d "$DEPENDENCY_ROOT/opencv" ]]; then
    export OpenCV_DIR="$DEPENDENCY_ROOT/opencv"
    export LD_LIBRARY_PATH="$DEPENDENCY_ROOT/opencv/lib:$LD_LIBRARY_PATH"
fi

# GStreamer paths (if installed in custom location)
if [[ -d "$DEPENDENCY_ROOT/gstreamer" ]]; then
    export GSTREAMER_ROOT_DIR="$DEPENDENCY_ROOT/gstreamer"
    export LD_LIBRARY_PATH="$DEPENDENCY_ROOT/gstreamer/lib:$LD_LIBRARY_PATH"
fi
```

## Supported Platforms

### Linux (Ubuntu/Debian)
- Full support for OpenCV and GStreamer
- Automatic system dependency installation
- Comprehensive GStreamer plugin support

### Linux (CentOS/RHEL/Fedora)
- Full support for OpenCV and GStreamer
- Automatic system dependency installation

### Windows
- Limited support (requires manual dependency installation)
- Path configuration for Windows-specific installations
- GStreamer installation via MSYS2 recommended

### macOS
- Limited support (requires manual dependency installation)
- Homebrew-based installation recommended
- GStreamer installation via Homebrew

## Video Processing Features

### OpenCV Support

The library provides comprehensive OpenCV support:

- **Video Capture**: File-based and device-based video capture
- **Image Processing**: Frame manipulation and preprocessing
- **Format Support**: Multiple video and image formats
- **Hardware Acceleration**: GPU acceleration when available

### GStreamer Support

When enabled, GStreamer provides additional capabilities:

- **Advanced Video Pipelines**: Complex video processing pipelines
- **Network Streaming**: RTSP, HTTP, and other streaming protocols
- **Hardware Acceleration**: Hardware-accelerated video processing
- **Plugin Ecosystem**: Extensive plugin support for various formats

### FFmpeg Support

When enabled, FFmpeg provides maximum flexibility:

- **Codec Coverage**: Supports virtually every codec and container format
- **Streaming Protocols**: RTSP, RTMP, HLS, and more
- **Direct Control**: Low-level API for fine-grained control
- **Batch Processing**: Excellent for offline video processing
- **Lightweight**: Simpler dependency stack than GStreamer

### Backend Selection Priority

When multiple backends are enabled, the factory uses this priority:
1. **FFmpeg** - Maximum compatibility, best for diverse sources
2. **GStreamer** - Advanced pipelines, hardware acceleration
3. **OpenCV** - Simple and reliable default

## Troubleshooting

### Common Issues

1. **Missing OpenCV**:
   ```bash
   # Reinstall with force flag
   ./scripts/setup_dependencies.sh --force
   ```

2. **GStreamer Compilation Issues**:
   ```bash
   # Check GStreamer installation
   gst-launch-1.0 --version
   pkg-config --modversion gstreamer-1.0
   ```

3. **FFmpeg Not Found**:
   ```bash
   # Check FFmpeg installation
   pkg-config --modversion libavformat
   # Or use standalone script
   ./scripts/setup_ffmpeg.sh
   ```

4. **Version Conflicts**:
   ```bash
   # Override version in CMake
   cmake .. -DGSTREAMER_VERSION=1.18.0 -DFFMPEG_VERSION=4.2
   ```

### Validation Errors

The validation system provides detailed error messages:

```
[ERROR] OpenCV headers not found. Please install OpenCV development packages.
[ERROR] GStreamer development headers not found. Please install GStreamer development packages.
[ERROR] FFmpeg development headers not found. Please install FFmpeg development packages.
```

### Manual Installation

For custom installations:

1. Install OpenCV, GStreamer, and/or FFmpeg manually
2. Set environment variables to point to installation directories
3. Run validation: `./scripts/setup_dependencies.sh --gstreamer --ffmpeg`

## Integration with Main Project

The VideoCapture library is designed to be integrated into the main object-detection-inference project:

1. **FetchContent Integration**: The main project fetches this library using CMake FetchContent
2. **Version Synchronization**: Video processing library versions are managed here, not in the main project
3. **Setup Script Coordination**: Main project setup scripts call this library's setup scripts

## Performance Considerations

### Backend Comparison

- **OpenCV**: Best for simple video capture and image processing
- **GStreamer**: Best for complex pipelines, hardware acceleration, and streaming
- **FFmpeg**: Best for maximum format compatibility and batch processing

### Hardware Acceleration

- **OpenCV**: CUDA, OpenCL support for image processing
- **GStreamer**: Hardware acceleration for video decoding/encoding

### Memory Usage

- **OpenCV**: Lower memory footprint for simple operations
- **GStreamer**: Higher memory usage but better for complex pipelines

## Contributing

When adding new video processing features:

1. **Update versions.cmake**: Add version variables for new libraries
2. **Update DependencyValidation.cmake**: Add validation functions
3. **Update setup_dependencies.sh**: Add installation logic
4. **Create individual script**: Add backward compatibility script if needed
5. **Update documentation**: Document the new features

## Version History

- **v1.0**: Initial centralized version management
- **v1.1**: Added dependency validation system
- **v1.2**: Added unified setup scripts
- **v1.3**: Added GStreamer compilation testing
- **v1.4**: Added backward compatibility scripts 