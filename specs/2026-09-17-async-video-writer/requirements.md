# Requirements — Asynchronous Video Writer

Spec: `specs/2026-09-17-async-video-writer/requirements.md` · Branch: `feat/async-videowriter` · Roadmap: Phase 6

> **Adoption note.** This packet was written on 2026-09-17, after the
> implementation on this branch (commits `9ac2a4c`, `a592023`, plus working-tree
> changes). It is a reconstruction from code, tests, and `CHANGELOG.md`, not a
> spec that preceded the code. Validation evidence was likewise executed against
> the finished implementation. Phase 7 onward follow spec-before-code order.

## Goal

A capture loop that also records should not be paced by encode time.
`writeFrame()` hands a frame to a writer-owned thread and returns, while the
file that results is byte-for-byte what the synchronous writer would have
produced.

## In Scope

- [R-1] `createVideoWriter()` returns a writer that encodes on a thread it owns.
  `writeFrame()` returns once the frame is accepted, not once it is encoded.
- [R-2] Accepted frames are encoded in submission order and are never dropped.
  When the encoder falls behind, `writeFrame()` blocks until a slot frees.
- [R-3] Memory is bounded: at most `capacity` queued frames plus the one being
  encoded, for either overload. A producer waiting for room holds no extra frame.
- [R-4] An encode failure is sticky for the destination: queued frames are
  discarded, blocked producers wake, and every later `writeFrame()` and the
  closing `release()` return `false`.
- [R-5] `release()` encodes every accepted frame, finalizes the container, joins
  the encoder thread, and returns whether the destination is whole. It is
  idempotent and safe on a writer that was never initialized.
- [R-6] `writeFrame(Frame&&)` takes ownership so the pixels reach the encoder
  thread without a copy. The default implementation forwards to the copying
  overload, so existing `VideoWriterInterface` implementations still compile.
- [R-7] Writer diagnostics emitted from the encoder thread do not interleave
  with writer diagnostics emitted from the caller's thread.
- [R-8] A build that cannot provide the required standard-library concurrency
  features fails at configure time with a remediation message, not at compile
  time with a template error.

## Out of Scope

- Synchronizing capture-side and application-side diagnostics. Only *writer*
  messages are line-atomic; a capture log can still split one. Tracked in
  `specs/roadmap.md` → Deferred.
- Cancelling an encode already in progress. `release()` waits for the current
  backend call however long it takes.
- Resolving macOS writer support. [R-8] detects an unsupported toolchain; it
  does not make macOS work. Tracked as roadmap Phase 7.
- Changing `.github/workflows/release.yml`. Deliberate — see Phase 7.

## Decisions

- [D-1] **Block on a full queue rather than drop frames.** A dropped frame is a
  silently corrupt recording; a blocked producer is visible back-pressure.
  Default capacity 4 (`AsyncVideoWriter.hpp: kDefaultCapacity`) — a longer queue
  cannot raise sustained encode rate, it only absorbs jitter.
- [D-2] **Reserve a queue slot before copying the frame.** Keeps [R-3] true for
  the copying overload; the copy happens only once there is room, and the
  reservation is released if the copy throws.
- [D-3] **`release()` returns `bool`, breaking source and ABI.** Asynchronous
  encoding makes per-frame failure reporting impossible at the call site, so the
  close is the only honest place to report it. Callers that ignore the result
  compile unchanged; custom implementations must update the override.
- [D-4] **`std::jthread` + `std::stop_token` over a `closing_` flag.** Stop state
  is per-thread, so reopening a writer cannot inherit a stale closing flag, and
  the join is RAII. `encoderThread_` is declared last so it is destroyed — and
  therefore joined — before the mutex, queue, and condition variables it uses.
- [D-5] **`std::osyncstream` over a logging mutex.** No new shared state, and it
  composes with any other synchronized stream on `std::cerr`.
- [D-6] **Gate the C++20 library features at configure time** rather than
  documenting a minimum compiler version. The language flag does not imply the
  library support (libc++ is the case in point), so only a compile-and-link
  probe is conclusive.

## Constraints

- No new dependency: the encoder thread links `Threads::Threads` privately
  (`REPO_META.yaml: writer_adds_no_new_dependency`).
- Backend priority FFmpeg → GStreamer → OpenCV is unchanged
  (`REPO_META.yaml: preserve_backend_priority`).
- The wrapped backend is touched from exactly one thread while a destination is
  open; the writer itself is driven from one thread at a time.
- Returning `bool` does not make the public API non-throwing. Backend write and
  finalization exceptions are caught and reported as failure; thread-launch,
  frame-copy, and queue-allocation exceptions can propagate after cleanup.
- Full-tree `clang-format` must pass before commit and before push (`AGENTS.md`).

## Dependencies

- `VideoWriterInterface` and `VideoWriterConfig` (public, `include/`).
- `videocapture::writer::validateFrame()` (`src/WriterSupport.hpp`), shared with
  the synchronous backends.
- Standard library: `std::jthread`, `std::stop_token`, stop-aware
  `std::condition_variable_any::wait`, `std::osyncstream`.
- Downstream driver: [neuriplo-infer#49](https://github.com/olibartfast/neuriplo-infer/issues/49).

## Assumptions & Open Questions

- [A-1] One frame in flight plus a 4-frame queue is enough to decouple a capture
  loop from encode jitter. Basis: inference from [D-1]; no profile of the
  downstream consumer has been run.
- [A-2] libstdc++ from GCC 11 satisfies [R-8]. Basis: inference — `<syncstream>`
  is the last of the four features to land. Only GCC 15.2 has actually been
  probed here; [R-8] makes the failure loud on anything older.
- [Q-1] Is macOS a supported writer platform? Unresolved; see roadmap Phase 7.

## Definition of Done (requirements level)

- [x] Every [R-n] is implemented or explicitly deferred with a tracked location
- [x] No In Scope behavior silently dropped; no Out of Scope work smuggled in
