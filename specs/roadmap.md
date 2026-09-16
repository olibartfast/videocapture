# Roadmap

Status values: `idea` · `planned` · `in progress` · `done` · `blocked` · `deferred`

Phases 1–5 are reconstructed from `CHANGELOG.md` and the release tags during
brownfield adoption; they shipped before specs existed and have no feature
packet. Phase 6 onward follow the packet workflow.

## Phase 1 — Backend-selectable capture
- Status: `done` — v0.1.0 (2026-03-02)
- Outcome: `createVideoInterface()` over OpenCV, GStreamer, and FFmpeg, selected by CMake option.
- Proves: one interface can hide three capture libraries.
- Spec: none (pre-adoption)

## Phase 2 — Release and CI infrastructure
- Status: `done` — v0.2.0 (2026-03-31)
- Outcome: CI, release, coverage, Docker, and docs workflows; sanitizer builds; `VERSION` as the version source; `AGENTS.md` and `REPO_META.yaml`.
- Proves: changes can be validated automatically before merge.
- Spec: none (pre-adoption)

## Phase 3 — Consumer build ergonomics
- Status: `done` — v0.3.0 (2026-06-07)
- Outcome: FetchContent consumers skip app/tests; module path fixed; optional ccache.
- Proves: the library is embeddable without carrying the sample app.
- Spec: none (pre-adoption)

## Phase 4 — Dependency-free frame contract
- Status: `done` — v0.4.0 (2026-08-30)
- Outcome: `videocapture::Frame` replaces `cv::Mat` in the public API; OpenCV confined to its own backend.
- Proves: the public API can carry pixels without a backend type. **Breaking.**
- Spec: none (pre-adoption)

## Phase 5 — Optional video writer
- Status: `done` — v0.5.0 (2026-09-04)
- Outcome: `createVideoWriter()` behind `USE_VIDEOWRITER`, same backend priority, no new dependency.
- Proves: encoding reuses the decode backend.
- Spec: none (pre-adoption)

## Phase 6 — Asynchronous writer and C++20 concurrency
- Status: `in progress`
- Outcome: encoding runs on a writer-owned thread behind a bounded queue, so `writeFrame()` costs a validation and a hand-off; shutdown uses `std::jthread`/`std::stop_token`; writer diagnostics are line-atomic.
- Proves: a downstream capture loop ([neuriplo-infer#49](https://github.com/olibartfast/neuriplo-infer/issues/49)) is no longer paced by encode time. **Breaking:** `release()` returns `bool`.
- Spec: `specs/2026-09-17-async-video-writer/`   Branch: `feat/async-videowriter`

## Phase 7 — Resolve macOS writer support
- Status: `blocked` — pending [Q-1]
- Outcome: either macOS writer builds are verified green, or macOS is declared capture-only and `release.yml` stops configuring `USE_VIDEOWRITER=ON` there.
- Proves: the release matrix reflects what actually builds.
- Blocking evidence: libc++ made `std::jthread` non-experimental in LLVM 20; the `macos-14` runner SDK predates that. `cmake/WriterConcurrency.cmake` now *detects* the gap at configure time but does not close it, and `release.yml` was intentionally left unchanged.
- Requires a feature packet when started.

## Phase 8 — GStreamer instance isolation and C++20 source modernization

- Status: `validated, PR pending` — independent of blocked Phase 7.
- Outcome: concurrent captures keep their frames and lifecycle state isolated;
  source code uses C++20 where it improves correctness or clarity.
- Spec: `specs/2026-09-17-gstreamer-isolation-cxx20/`
- Branch: `fix/gstreamer-isolation-cxx20`
- Evidence: `validation.md` — backend matrix, capture-only build, downstream
  consumer, and format gate pass. TSan over GStreamer reports only
  GLib-internal allocation warnings; no project-code racing frames.

## Deferred / Revisit

- [x] Align documentation and build requirements: C++20 and CMake 3.21 in README and build declarations. Writer standard-library support is checked by the feature gate rather than a compiler-version promise. — [Q-2], resolved 2026-09-17
- [ ] Capture and application diagnostics still write to `std::cerr` unsynchronized (`src/ffmpeg/FFmpegCapture.cpp`, `src/gstreamer/GStreamerCapture.cpp`, `app/`), so they can split a synchronized writer message in the sample app. Scope decision recorded in `tech-stack.md` under Open.
- [x] Add `specs/` to `REPO_META.yaml: owned_paths` with explicit user approval — 2026-09-17.
- [ ] `std::condition_variable_any` with a stop token costs a `stop_callback` registration per dequeued frame. Negligible at video rates; revisit only if a profile says otherwise.

_Revision: 2026-09-17 — reconstructed from CHANGELOG and tags; Phase 6 opened for the in-flight branch._
