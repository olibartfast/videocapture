# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

### Changed

### Deprecated

### Removed

### Fixed

### Security

## [1.0.0] - YYYY-MM-DD

### Added
- Initial release
- Support for OpenCV backend (default)
- Support for GStreamer backend (optional)
- Support for FFmpeg backend (optional)
- Factory pattern for backend selection
- CMake build system
- Unit tests with Google Test
- Example application

[Unreleased]: https://github.com/olibartfast/videocapture/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/olibartfast/videocapture/releases/tag/v1.0.0
