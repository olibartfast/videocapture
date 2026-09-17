# Plan — Asynchronous Video Writer

Reconstructed from the branch history; group order matches how the work actually
landed. Commits: `9ac2a4c` (Group 1–2), `a592023` (Group 3), working tree
(Groups 4–6).

## Group 1 — Interface contract

- [T-1] Widen `VideoWriterInterface` for asynchronous writers — Deliverable: `writeFrame()` documented as accept-not-encode; `release()` returns `bool`; `writeFrame(Frame&&)` added with a forwarding default. Serves [R-1], [R-5], [R-6].
- [T-2] Update the three synchronous backends to the new `release()` signature — Deliverable: FFmpeg, GStreamer, and OpenCV writers compile and report finalization failures.
  - Checks: `cmake --build` with each backend enabled.

## Group 2 — Encoder thread

- [T-3] Add `AsyncVideoWriter`: bounded queue, one encoder thread, two condition variables — Deliverable: encoding off the caller's thread. Serves [R-1], [R-2].
- [T-4] Slot reservation before the frame copy, released on throw — Deliverable: bounded memory for both overloads. Serves [R-3], [D-2].
- [T-5] Sticky failure state: discard the queue, wake blocked producers, fail every later call — Deliverable: a broken destination cannot silently continue. Serves [R-4].
- [T-6] Wire `AsyncVideoWriter` into `createVideoWriter()` — Deliverable: the public factory returns the async writer over the selected backend.

## Group 3 — Review hardening

- [T-7] Address PR review findings — Deliverable: exception-safety on the copy path, finalize-even-after-failure, idempotent `release()`.

## Group 4 — C++20 modernization

- [T-8] Replace `std::thread` + `closing_` with `std::jthread` + `std::stop_token`, and `frameQueued_` with `std::condition_variable_any` — Deliverable: stop-aware wait that still drains accepted frames before exit. Serves [R-5], [D-4].
- [T-9] Declare `encoderThread_` last so its RAII join precedes destruction of the state it uses — Deliverable: no use-after-destroy on teardown.
- [T-10] Wrap writer diagnostics in `std::osyncstream` across `AsyncVideoWriter`, `WriterSupport`, and the three backends — Deliverable: line-atomic writer messages. Serves [R-7].

## Group 5 — Toolchain gate

- [T-11] Add `cmake/WriterConcurrency.cmake`: compile-and-link probe for jthread, stop_token, stop-aware wait, and osyncstream; `FATAL_ERROR` with remediation — Deliverable: unsupported toolchains fail at configure. Serves [R-8], [D-6].
  - Checks: configure with `USE_VIDEOWRITER=ON` succeeds on GCC 15.2.
- [T-12] Document the requirement and the diagnostics-scope limitation in `Readme.md` — Deliverable: the Out of Scope boundary from [R-7] is stated where consumers read it.

## Group 6 — Verification

- [T-13] Tests for [R-1]–[R-7], including blocking behavior, failure stickiness, reopen-gets-fresh-stop-state, and non-interleaved diagnostics — Deliverable: `tests/test_async_video_writer.cpp`.
- [T-14] `scripts/check_writer_concurrency.sh`: format gate plus build and test across four backend configurations — Deliverable: one command reproduces the acceptance run.
- [T-15] Execute everything in `validation.md` and record evidence.

## Notes

- The stop-aware wait returns `pred()`, so a stop request with a non-empty queue
  still drains. The `queue_.empty()` check after the wait is the only exit — this
  is the subtlety that makes [R-5] hold under [D-4].
- `release()` calls `request_stop()` and `join()` explicitly rather than relying
  on the `jthread` destructor, because `release()` must work without destroying
  the writer.
- Deviation from spec-first order: the packet postdates the code (see the
  adoption note in `requirements.md`).
