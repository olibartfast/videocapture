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
| V-2s | ThreadSanitizer, `GStreamer*`, `--gtest_repeat=5` | pass (no project races) | 2026-09-17 | 147 warnings, all racing frames in GLib/GStreamer alloc paths; none in project code |

## Deviations

- **[V-2s] TSan over GStreamer is noisy.** The sanitized GStreamer build reports
  data races whose racing accesses are `g_malloc`/`free`/`g_queue_pop_tail`
  inside GLib and GStreamer; no report carries a racing access frame in
  `src/`. GLib's internal allocation/queue synchronization is not modelled by
  TSan here. CI's sanitizer jobs run the OpenCV backend only, so this is not a
  CI gate; recorded rather than suppressed.
- **[M] macOS writer and platform checks not run** — same toolchain gap as
  Phase 7 (`specs/mission.md` [Q-1]); capture-only clang builds are covered by
  CI.

Attempt ledger: no worker delegation; no metered cost/context data available.
