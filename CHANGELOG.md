# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- repo-local `AGENTS.md` guidance for maintenance agents
- `REPO_META.yaml` with local build, ownership, and automation policy metadata
- `VERSION` file as the canonical project version source

### Changed

- `CMakeLists.txt` now reads the project version from `VERSION`

## [0.1.0] - 2026-03-02

### Added

- shared `VideoCapture` library target for video input handling
- OpenCV-backed video capture implementation
- optional GStreamer-backed capture implementation
- sample `VideoCaptureApp`

[Unreleased]: https://github.com/olibartfast/videocapture/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/olibartfast/videocapture/releases/tag/v0.1.0
