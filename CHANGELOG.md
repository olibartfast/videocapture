# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.5.0] - 2026-09-04

### Added

- Optional video writer module, enabled with `-DUSE_VIDEOWRITER=ON`
  (`include/VideoWriterInterface.hpp`, `include/VideoWriterConfig.hpp`,
  `include/VideoWriterFactory.hpp`). `createVideoWriter()` follows the same
  backend priority as `createVideoInterface()`, so a build encodes through the
  backend it decodes with and the option adds no dependency in any
  configuration: FFmpeg encodes with the already-linked libav* libraries,
  GStreamer pushes through the `appsrc` in the already-linked `libgstapp`, and
  the OpenCV backend uses `cv::VideoWriter` from the already-linked `videoio`.
- Writer backends for FFmpeg, GStreamer, and OpenCV, accepting the packed 8-bit
  `videocapture::Frame` layouts and converting to the encoder's format
- Optional output path and frame rate arguments in the sample application
  (`VideoCaptureApp <source> [output] [fps]`) for writer builds
- Selectable OpenCV HighGUI, SDL2, GLFW, and Sokol preview renderers for the
  sample application, while keeping renderer dependencies out of the capture
  library and its public frame API

## [0.4.0] - 2026-08-30

### Added

- `videocapture::Frame`, a dependency-free frame abstraction carrying explicit
  pixel formats, multi-plane layouts, row strides, timestamps, and sequence
  numbers (`include/Frame.hpp`)
- SDL2 preview window in the sample application for the GStreamer and FFmpeg
  backends, avoiding an OpenCV display dependency (`app/SdlRenderer.hpp`)
- `scripts/check_dependency_updates.sh`, extracting the dependency comparison
  out of the workflow so it can be run and tested locally

### Changed

- **Breaking:** `readFrame()` now returns a `videocapture::Frame` instead of a
  `cv::Mat`. Capture backends currently produce packed BGR8.
- Restricted OpenCV discovery, compilation, and linkage to the OpenCV backend;
  FFmpeg and GStreamer builds no longer require OpenCV
- Renamed `GStreamerOpenCV` to `GStreamerPipeline` now that the pipeline no
  longer depends on OpenCV

### Fixed

- `GStreamerCapture::readFrame()` no longer hangs at end of stream. EOS is
  reported from the appsink callback and the GLib main context is pumped
  between short timed waits, so bus errors also break the wait; the bus watch
  is now removed with the pipeline instead of stacking up on re-initialize.
- CMake accepts OpenCV major versions newer than 4
- The dependency-check workflow no longer opens duplicate issues for the same
  pending updates

## [0.3.0] - 2026-06-07

### Added
- Optional `ccache` support to speed up rebuilds (`cmake/CompileSpeed.cmake`)
- Parallel build jobs in the CI workflow for faster pipelines

### Changed
- Build the sample app and tests only when videocapture is the top-level
  project, reducing build time for FetchContent consumers
- Enabled parallel CI builds for top-level and fetched consumers

### Fixed
- Corrected the CMake module path so FetchContent consumers resolve modules
  reliably

## [0.2.0] - 2026-03-31

### Added
- Comprehensive CI/CD pipeline with GitHub Actions
- Multiple workflows: CI, Release, Coverage, Docker, Documentation
- Code quality checks with clang-format and cppcheck
- Sanitizer builds (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer)
- Cross-platform testing (Ubuntu, macOS)
- Docker images for easy deployment
- Automated dependency update checks
- API documentation generation with Doxygen
- Issue and PR templates
- EditorConfig and clang-format configurations
- repo-local `AGENTS.md` guidance for maintenance agents
- `REPO_META.yaml` with local build, ownership, and automation policy metadata
- `VERSION` file as the canonical project version source

### Changed
- `CMakeLists.txt` now reads the project version from `VERSION`

### Deprecated

### Removed

### Fixed

### Security

## [0.1.0] - 2026-03-02

### Added
- Initial release
- Support for OpenCV backend (default)
- Support for GStreamer backend (optional)
- Support for FFmpeg backend (optional)
- Factory pattern for backend selection
- CMake build system
- Unit tests with Google Test
- Example application

[Unreleased]: https://github.com/olibartfast/videocapture/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/olibartfast/videocapture/compare/v0.4.0...v0.5.0
[0.4.0]: https://github.com/olibartfast/videocapture/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/olibartfast/videocapture/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/olibartfast/videocapture/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/olibartfast/videocapture/releases/tag/v0.1.0
