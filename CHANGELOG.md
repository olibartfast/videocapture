# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/olibartfast/videocapture/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/olibartfast/videocapture/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/olibartfast/videocapture/releases/tag/v0.1.0
