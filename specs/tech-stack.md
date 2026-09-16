# Tech Stack — VideoCapture

## Fixed (must not be changed per feature)

- **Language:** C++20. Set centrally as `CXX_STANDARD "20"` in
  `cmake/versions.cmake:11`, applied at `CMakeLists.txt:28` and
  `cmake/DependencyValidation.cmake:55-56` with `CMAKE_CXX_STANDARD_REQUIRED ON`
  (evidence). Overridable at configure time via `-DCXX_STANDARD=`, but 20 is the
  supported value.
- **Build:** CMake 3.21 or higher, aligned in `CMakeLists.txt`,
  `cmake/versions.cmake`, and `Readme.md`. The existing `PROJECT_IS_TOP_LEVEL`
  checks require this version.
- **Capture backends:** OpenCV (default), GStreamer (`USE_GSTREAMER`), FFmpeg
  (`USE_FFMPEG`). Selection priority FFmpeg → GStreamer → OpenCV.
- **Writer:** optional, `USE_VIDEOWRITER=ON`, reusing the capture backend's
  libraries only.
- **Testing:** GoogleTest, one binary `VideoCaptureTests`, driven by CTest
  (`tests/CMakeLists.txt`). Entrypoints are recorded in `REPO_META.yaml`.
- **CI:** GitHub Actions on `ubuntu-24.04`; release builds also target
  `macos-14` and `macos-latest` (`.github/workflows/release.yml:24`).
- **Formatting:** full-tree `clang-format` check, gate defined by
  `.github/workflows/format.yml` and required before commit and before push
  (`AGENTS.md`).

## Preferred (follow unless there is a recorded reason not to)

- Concurrency follows *C++ Concurrency in Action* patterns: one mutex guarding
  all shared state, predicate-form condition-variable waits, notify outside the
  lock, never call backend code while holding a lock, thread-owning types join
  in their destructor (evidence: `src/AsyncVideoWriter.cpp`).
- Prefer C++20 concurrency vocabulary (`std::jthread`, `std::stop_token`,
  `std::osyncstream`) over hand-rolled C++11 equivalents, subject to the
  standard-library availability gate below.
- Diagnostics go to `std::cerr`. Any code reachable from a non-caller thread
  wraps them in `std::osyncstream`.

## Open (decide per feature, record the decision)

- Whether capture-side and application-side diagnostics also move to
  `std::osyncstream`. Currently only writer diagnostics are synchronized, so
  capture logs can still interleave with encoder-thread logs
  (evidence: `Readme.md` writer section states this limitation explicitly).

## Explicit Non-Choices

- **No new runtime dependency for the writer** — it must encode through the
  already-linked backend (`REPO_META.yaml: writer_adds_no_new_dependency`).
- **No backend types in public headers** — keeps consumer builds dependency-free
  (`REPO_META.yaml: public_frame_api_dependency_free`).
- **No blanket non-throwing guarantee** — `bool` reports validation and backend
  encode/finalization failures. The async writer catches backend write and
  finalization exceptions, but thread-launch, frame-copy, and queue-allocation
  failures can propagate after cleanup (`src/AsyncVideoWriter.cpp`).
- **No frame dropping in the writer** — back-pressure blocks the producer
  instead (evidence: `include/VideoWriterInterface.hpp`, "never dropped").

## Constraints (cross-feature)

- A C++20 *language* flag is not sufficient. Writer builds require a standard
  library providing `std::jthread`, `std::stop_token`, stop-aware
  `condition_variable_any::wait`, and `std::osyncstream`. This is checked at
  configure time by `cmake/WriterConcurrency.cmake` and fails fast with a
  remediation message (evidence).
  - libstdc++ satisfies this from GCC 11 (inference: `<syncstream>` is the
    latest of the four to land).
  - libc++ made `std::jthread` non-experimental in **LLVM 20** (evidence:
    maintainer, 2026-09-17), which is later than the SDK on the `macos-14`
    release runner. Writer builds on macOS are therefore unverified.
- Public-interface changes are source- and ABI-visible to downstream consumers;
  breaking ones are called out in `CHANGELOG.md` and drive the version bump.
- `specs/` is listed in `REPO_META.yaml: owned_paths`, following explicit user
  approval on 2026-09-17. Agent specification updates are within declared scope.

## Open Questions

- [Q-2] Resolved for build requirements: `Readme.md` states C++20 and CMake
  3.21, matching the build configuration. Writer library support is established
  by the compile-and-link gate rather than an unverified compiler-version
  promise (feature decision [D-6]). macOS verification remains open as [Q-1].

_Revision: 2026-09-17 — reconstructed from the existing repository during brownfield spec adoption._
