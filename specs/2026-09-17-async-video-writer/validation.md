# Validation — Asynchronous Video Writer

> **Adoption note.** These criteria were written on 2026-09-17, after the
> implementation. That inverts the skill's validation-before-code rule and is
> recorded here rather than hidden: the criteria were derived from the
> requirements, then executed against the tree, but they cannot claim to have
> constrained the design. Phase 7 onward follow spec-before-code order.

## Automated Checks

Run from the repository root.

```text
./scripts/check_writer_concurrency.sh
# format gate + configure/build/ctest across opencv, gstreamer, ffmpeg, all
# expected: exit 0
```

- [x] [V-1] Full-tree `clang-format --dry-run -Werror` plus `git diff --check`, matching `.github/workflows/format.yml`
- [x] [V-2] Focused tests for [R-1]–[R-7] — `tests/test_async_video_writer.cpp`
- [x] [V-3] Full regression suite across four backend configurations
- [x] [V-4] ThreadSanitizer run over the writer suite — covers [R-1]–[R-5]
- [x] [V-5] Configure-time gate accepts a supporting toolchain and rejects a non-supporting one — [R-8]

## Manual Checks

- [x] [M-1] Reopen cycle: initialize → release → initialize → write → release does not inherit stale stop state — [R-5], [D-4]. Automated as `IdleReleaseWakesEncoderAndReopeningGetsAFreshStopState` (32 iterations).
- [x] [M-2] Back-pressure: with the encoder gated, `writeFrame()` on a full queue does not return — [R-2], [R-3]. Automated via `std::async` + `wait_for` timeout.
- [x] [M-3] Diagnostics: 16 encoder threads logging failures while the caller logs 256 rejections produce only whole, expected lines — [R-7].
- [ ] [M-4] macOS writer build. **Not performed** — no macOS toolchain available. See Deviations.

## Evidence Log

| ID | Command/Check | Result | Date | Notes |
|----|---------------|--------|------|-------|
| V-1 | full-tree `clang-format` + `git diff --check` | pass | 2026-09-17 | clean, no output |
| V-2 | `ctest` filter `AsyncVideoWriter*`, GCC 15.2 Debug | pass | 2026-09-17 | 22/22 |
| V-3 | `./scripts/check_writer_concurrency.sh` | pass | 2026-09-17 | 237/237 across opencv, gstreamer, ffmpeg, all — run by maintainer |
| V-3b | `ctest` full suite, OpenCV + writer, GCC 15.2 Debug | pass | 2026-09-17 | 55/55 |
| V-4 | `-fsanitize=thread`, `AsyncVideoWriter*`, `--gtest_repeat=10` | pass | 2026-09-17 | 0 ThreadSanitizer reports, 22/22 per repeat |
| V-5a | `cmake -DUSE_VIDEOWRITER=ON` on GCC 15.2 | pass | 2026-09-17 | `VIDEOCAPTURE_HAS_WRITER_CONCURRENCY - Success` |
| V-5b | `cmake -DUSE_VIDEOWRITER=ON -DCXX_STANDARD=17` | pass (fails as designed) | 2026-09-17 | `FATAL_ERROR` at `cmake/WriterConcurrency.cmake:34`, exit 1, remediation text shown |
| M-1 | reopen cycle | pass | 2026-09-17 | covered by V-2 |
| M-2 | blocking back-pressure | pass | 2026-09-17 | covered by V-2 |
| M-3 | concurrent diagnostics | pass | 2026-09-17 | covered by V-2 |
| M-4 | macOS writer build | **not run** | — | no toolchain |

## Deviations

- **[M-4] macOS is unvalidated.** libc++ made `std::jthread` non-experimental in
  LLVM 20, later than the SDK on the `macos-14` release runner, so
  `release.yml`'s `USE_VIDEOWRITER=ON` macOS jobs are expected to fail at
  configure with the [R-8] message. [R-8] converts a confusing compile error into
  a clear one; it does not make macOS build. `release.yml` was deliberately left
  unchanged. Tracked as roadmap Phase 7 / [Q-1] — **this is a known release
  risk, not a resolved item.**
- **[A-2] unconfirmed.** Only GCC 15.2 has been probed. The claim that
  libstdc++ ≥ GCC 11 suffices remains an inference; [R-8] makes any gap loud.
- **Validation postdates implementation** — see the adoption note above.

## Specification synchronization — 2026-09-17

- User approved adding `specs/` to `REPO_META.yaml: owned_paths`; ownership is
  now recorded there.
- README and build declarations now agree on C++20 and CMake 3.21. A fresh
  writer-enabled configuration succeeded with GCC 15.2 and CMake 4.3.2; this
  does not constitute a test using CMake 3.21 itself.
- Corrected the inferred no-exceptions claim against `AsyncVideoWriter.cpp`:
  backend write/finalization exceptions are caught, while thread-launch,
  frame-copy, and queue-allocation exceptions can propagate after cleanup.
  This is source-review evidence, not injected-failure test coverage.
- Earlier suite and sanitizer results above remain historical evidence; they
  were not rerun for this metadata/documentation-only synchronization.

## Definition of Done (integration)

- [x] Every automated check executed, with evidence recorded
- [x] Evidence traced to [R-n]; deviations documented
- [ ] Spec, code, roadmap, and changelog agree — merge as one coherent change
      *(requirements, ownership, and exception-policy corrections synchronized;
      integration remains pending, and macOS [M-4] is still unvalidated)*
- [x] Durable discoveries propagated to `tech-stack.md`
