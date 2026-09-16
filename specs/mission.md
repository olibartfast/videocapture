# Mission — VideoCapture

## Problem

C++ applications that need frames from a camera, file, or stream have to pick a
capture library early and then live with it. OpenCV, GStreamer, and FFmpeg each
impose their own types, lifecycle, and error conventions on the calling code, so
swapping backends — or supporting more than one — means rewriting the call sites.

## Audience

For C++ projects that want frames and encoded output behind one stable interface
and choose the backend at build time. Downstream consumer of record:
[neuriplo-infer](https://github.com/olibartfast/neuriplo-infer) (evidence:
`CHANGELOG.md` links neuriplo-infer#49 as the driver for the async writer).

Explicitly not for: applications that need a full media graph (filter chains,
transcoding pipelines, muxing several streams). Those should use GStreamer or
FFmpeg directly.

## Product Promise

One backend-free public API — `createVideoInterface()` for capture,
`createVideoWriter()` for encoding — over interchangeable backends. The public
headers carry no backend types, so a build that switches from OpenCV to FFmpeg
recompiles without touching consumer code (evidence: `include/` contains only
`Frame.hpp`, the two interfaces, the two factories, and `VideoWriterConfig.hpp`,
none of which include a backend header; enforced by
`REPO_META.yaml: public_frame_api_dependency_free`).

## Success Criteria

- A consumer compiles against the public headers with no backend headers,
  libraries, or types in its own build (evidence: `include/`, `tests/test_factory.cpp`).
- Backend selection is a CMake option, not a code change: `USE_GSTREAMER`,
  `USE_FFMPEG`, `USE_VIDEOWRITER` (evidence: `CMakeLists.txt:34,40,48`).
- Capture semantics are identical across backends — packed BGR8 frames,
  source-timeline timestamps, sequences from zero (evidence:
  `include/VideoCaptureInterface.hpp:12-19`).
- The writer adds no dependency in any configuration; it encodes through the
  backend the build already decodes with (evidence:
  `REPO_META.yaml: writer_adds_no_new_dependency`; `CHANGELOG.md` 0.5.0).

## Product Principles

- Backend priority is fixed and shared by capture and writer: FFmpeg, then
  GStreamer, then OpenCV (evidence: `include/VideoWriterFactory.hpp`,
  `REPO_META.yaml: preserve_backend_priority`).
- Writer validation and backend encode/finalization failures are reported through
  return values and stderr diagnostics. The public interface is not guaranteed
  non-throwing: thread-launch, frame-copy, and queue-allocation failures can
  propagate after cleanup (evidence: `src/AsyncVideoWriter.cpp`).
- Source semantics are preserved across changes; stream behavior changes get
  mandatory review (evidence: `REPO_META.yaml: constraints`).

## Open Questions

- [Q-1] Is macOS a supported platform for writer builds, or capture-only?
  `release.yml:24` builds `macos-14` and `macos-latest` with
  `USE_VIDEOWRITER=ON`, but the writer now needs libc++ features that those
  images may not ship. Owner: maintainer. See `specs/roadmap.md` Phase 6.

_Revision: 2026-09-17 — reconstructed from the existing repository during brownfield spec adoption._
