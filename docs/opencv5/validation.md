# OpenCV 5 support

Requirements: support the locally installed OpenCV 5 package while preserving
OpenCV 4.6 compatibility, backend priority, and the dependency-free public frame API.
Use an explicit `OpenCV_DIR` so builds never silently select a machine-specific SDK.

Plan: build against OpenCV 5, address any API incompatibilities, document package
selection, then run capture/writer tests and repeat against system OpenCV 4.

Validation planned before implementation:
- Configure and build the library, OpenCV preview, and tests with writer enabled.
- Run CTest, including generated-video writer/capture round trips.
- Repeat with system OpenCV 4 to check backward compatibility.
- Build an `add_subdirectory` consumer against OpenCV 5 without the sample app.

## Reproduce

From the repository root, pass the directory containing `OpenCVConfig.cmake`
and a separate build directory for each installation:

```bash
bash scripts/check_opencv.sh \
  "$HOME/dependencies/opencv-5.0.0/install/lib/cmake/opencv5" \
  /tmp/videocapture-opencv5

bash scripts/check_opencv.sh \
  /usr/lib/x86_64-linux-gnu/cmake/opencv4 \
  /tmp/videocapture-opencv4
```

The script selects the OpenCV backend and preview explicitly, enables writer
coverage, and requires a working MJPEG encoder so round-trip tests cannot skip
for missing codecs. It does not install or rebuild OpenCV. Use fresh build
directories to avoid cached settings from other configurations.

## Local results (2026-10-26)

- Toolchain: GNU C++ 13.3.0, CMake 3.28.3, C++20, Linux x86_64.
- OpenCV 5.0.0: library, writer, HighGUI renderer, sample app, and tests built;
  all 55 CTest tests passed with codec coverage required.
- OpenCV 4.6.0 (system installation): the same build and all 55 CTest tests
  passed with codec coverage required.
- OpenCV 5.0.0 embedded consumer: a separate CMake project used
  `add_subdirectory`, linked `VideoCapture`, created capture and writer objects,
  and exited successfully without building the sample app.
- No OpenCV API adaptation was needed: existing unversioned package discovery
  and numeric minimum-version validation already accept OpenCV 5.
- Live camera availability, interactive preview, other operating systems, and
  external downstream repositories were not validated.

Local logs (temporary artifacts): `/tmp/videocapture-opencv5-validation.log`,
`/tmp/videocapture-opencv4-validation.log`, and
`/tmp/videocapture-opencv5-consumer.log`.

## Hosted CI dependency repair

Run `40289134758` failed before compiling VideoCapture: Homebrew on macOS 14
built dependencies from source and Python's build rejected the runner compiler's
missing efficient tail-call support. A preceding macOS 14 run (`39499271640`)
took 4 hours 22 minutes. All Linux backend/sanitizer jobs and macOS latest passed.

The repair moves CI and release jobs to macOS 15, reuses runner CMake/Ninja,
and bounds macOS dependency installation to 15 minutes. CI jobs have a 25-minute
limit and release jobs a 30-minute limit. Additional release backend installation
has a separate 10-minute limit. Bottle availability is not forced: if Homebrew
falls back to a slow source build, the timeout prevents another multi-hour run.
This updates the tested/packaged platform matrix, not the library's minimum OS.

Validation: YAML parsing, embedded shell syntax, matrix/timeout assertions, and
`git diff --check` pass locally. Hosted execution is required to validate macOS
package installation and builds; the release workflow runs when the release is tagged.

Hosted result: PR #16 run `40295545071` passed all backend, sanitizer,
quality, coverage, and macOS checks. macOS 15 finished in 5m04s using bottles
(including OpenCV 5.0.0), with 21/21 tests passing; macOS latest finished in
5m16s. Two Ubuntu jobs were retried after slow package downloads, then passed.
