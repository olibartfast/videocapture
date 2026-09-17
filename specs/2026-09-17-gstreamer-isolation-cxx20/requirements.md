# Requirements — GStreamer isolation and C++20 source modernization

Written before implementation. Branch: `fix/gstreamer-isolation-cxx20`, from
clean `develop`. User explicitly requested both the isolation fix and source
modernization; this authorizes the bounded refactor described here.

- R-1: Independent GStreamer captures must never share pixels, sequence numbers,
  EOS/error state, or wakeups. Different instances may run on different threads.
- R-2: Release, destruction, failed initialization, and reinitialization stop
  streaming callbacks before destroying their instance state and release owned
  GStreamer references. Reopening resets only that instance's sequence to zero.
- R-3: Preserve BGR packing/stride, source PTS, final-buffer-before-EOS behavior,
  existing latest-frame buffering policy, and file/RTSP/custom-pipeline routing.
  Bus errors must terminate a reader without requiring an external GLib loop.
- R-4: Apply useful C++20 idioms across src: range algorithms and bounded views,
  prefix checks, checked integer conversion, and designated callback setup.
  Keep factories and already-modern writer concurrency when no change helps.
- R-5: Preserve public API, backend priority, dependencies, and capture-only
  toolchain support. Do not require jthread/syncstream for capture-only builds.

Decisions: callback functions remain static C trampolines with instance userdata;
mutable pipeline state is private and owned by one instance. Poll only that
pipeline's bus, never dispatch another object's default-context callbacks.
Use one mutex for the frame, sequence, and EOS state; do not add worker threads.
One caller at a time per capture remains the contract (including release).

Out of scope: concurrent read/release on the same capture, new URL formats,
YouTube, FFmpeg decode-loop redesign, application-wide logging modernization
(roadmap Deferred), macOS writer/toolchain and release changes (Phase 7).
Constitution: mission and dependency/public-interface constraints unchanged.
