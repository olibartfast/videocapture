# Validation

Defined before implementation. Focused development checks may repeat. Final
acceptance command, once per attempt: `./scripts/check_writer_concurrency.sh
/tmp/videocapture-isolation-matrix` (one shell line).

- V-1 -> R-1: two different-size/color pipelines read concurrently, independent
  sequence/PTS; one pipeline reaching EOS or error does not terminate the other.
- V-2 -> R-2: release/reinitialize and failed-initialize recovery; destroy an
  active pipeline while another reads; repeat lifecycle tests with sanitizers
  where available. Same-instance concurrent public calls are not tested/promised.
- V-3 -> R-3: existing final-frame/EOS tests; odd-width BGR rows; known test
  source PTS. Runtime bus error must return false without a GLib main loop.
- V-4 -> R-4/R-5: full format gate and backend matrix (OpenCV/GStreamer/FFmpeg/all),
  writer tests retained; capture-only GStreamer build/tests; downstream CMake
  consumer build. No public interface or dependency change by diff review.

## Evidence Log

| ID | Command/Check | Result | Date | Notes |
|----|---------------|--------|------|-------|
| V-1 | `GStreamer*` filter, GCC 15.2 Debug | pass | 2026-09-17 | 13/13; concurrent captures independent |
| V-2 | lifecycle tests + 8× reinit/partial-failure loops | pass | 2026-09-17 | destroy-while-reading, reopen resets sequence |
| V-3 | final-frame/EOS, odd-width (63), runtime error | pass | 2026-09-17 | error returns `false` without a GLib loop |
| V-4a | `./scripts/check_writer_concurrency.sh` | pass | 2026-09-17 | opencv 55, gstreamer 63, ffmpeg 59, all 72 |
| V-4b | capture-only `USE_GSTREAMER=ON`, `USE_VIDEOWRITER=OFF` | pass | 2026-09-17 | 29/29; no jthread/syncstream needed |
| V-4c | downstream FetchContent consumer (OpenCV + writer) | pass | 2026-09-17 | builds and runs against public headers |
| V-4d | full-tree `clang-format --dry-run -Werror` + `git diff --check` | pass | 2026-09-17 | clean, no output |
| V-2s | ThreadSanitizer, `GStreamer*`, `--gtest_repeat=5` | pass (no real races) | 2026-09-17 | 147 warnings; write side always in GLib, one read side in `pollBus()`; see deviation |

## Deviations

- **[V-2s] TSan over GStreamer is noisy and names project code.** The sanitized
  GStreamer build reports data races whose *write* side is always
  `g_malloc0`/`free`/`g_queue_pop_tail` inside GLib. One report's read side is
  `GStreamerPipeline::pollBus()` reading `message->type` from a message it
  popped, so TSan's `SUMMARY` names project code; no report has both accesses in
  `src/`. GLib implements `GMutex` on raw futexes rather than `pthread_mutex`,
  so TSan cannot see the bus-queue happens-before edge. On the identical
  pre-existing test set, `HEAD` produces 21 warnings and this branch 28 — the
  delta is extra bus polling on the reader thread, not a new hazard. CI's
  sanitizer jobs run the OpenCV backend only, so this is not a CI gate; recorded
  rather than suppressed.
- **[M] macOS writer and platform checks not run** — same toolchain gap as
  Phase 7 (`specs/mission.md` [Q-1]); capture-only clang builds are covered by
  CI.

## Known Behavior

- **Bus polling is timer-driven.** `readFrame()` wakes every 5 ms while idle to
  drain its bus (`GStreamerPipeline.cpp`), so EOS and errors surface without an
  external GLib loop. Frame delivery itself is still immediate via
  `notify_one`; the timer costs roughly six idle wakeups per 30 fps frame. This
  is the mechanism that satisfies R-3, and it is a polling loop by design.
- **Errors are observed only during a read.** Bus errors become visible while
  `readFrame()` is running; `isEndOfStream()` alone does not poll and so will not
  observe an error-induced stop. This matches the stated one-caller contract and
  is not a promise of asynchronous error notification.

Attempt ledger: no worker delegation; no metered cost/context data available.
