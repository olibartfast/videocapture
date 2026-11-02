# OpenCV 4 to 5 migration (upstream reference copy)

> Reference copy of the upstream OpenCV wiki migration guide, kept offline
> while this repo builds against both OpenCV 4.6 and OpenCV 5.0.
>
> - Source: <https://github.com/opencv/opencv/wiki/OpenCV-4-to-5-migration>
> - Retrieved: 2026-11-02 (upstream note: edited Jun 4, 2026, 16 revisions)
> - Copyright: © 2019-2025 OpenCV team (see source page).
> - Third-party reference: check the upstream wiki for updates rather than
>   editing the guide body below.

This guide covers the breaking changes and API differences introduced in
OpenCV 5.0. Most existing code will require only minor adjustments; the
sections below walk through each affected area with before/after examples.

---

## Table of contents

1. [Build requirements](#1-build-requirements)
2. [Legacy C API removed](#2-legacy-c-api-removed)
3. [Module restructuring](#3-module-restructuring)
4. [Core API changes](#4-core-api-changes)
5. [DNN module](#5-dnn-module)
6. [Imgproc: behavior changes](#6-imgproc-behavior-changes)
7. [Video I/O module](#7-video-io-module)
8. [Python bindings](#8-python-bindings)

---

## 1. Build requirements

OpenCV 5.0 requires **C++17** or later. Make sure your compiler and CMake
toolchain are configured accordingly.

**CMake:**

```cmake
# Before (OpenCV 4.x default)
set(CMAKE_CXX_STANDARD 11)

# After
set(CMAKE_CXX_STANDARD 17)
```

**Compiler minimum versions:** GCC 8 (or 7.x with some caveats), Clang 9,
MSVC 2017 (19.14) or later.

Python 2 is no longer supported. Use Python 3.6 or later.

---

## 2. Legacy C API removed

The OpenCV 1.x C API (`CvMat`, `IplImage`, `cvCreateMat()`,
`cvFindContours()`, etc.) has been fully removed from the codebase. If your
code still uses these types and functions, you need to migrate to the C++
API. Some `CV_` macros such as `CV_8U`, `CV_32F` etc. are still available
and unchanged.

In practice, very little production code written in the last decade uses the
C API directly, so most projects are unaffected. If you do have legacy C API
usage, the
[OpenCV C++ migration guide (2.x era)](https://docs.opencv.org/4.x/db/dfa/tutorial_transition_guide.html)
remains a useful reference for the C→C++ transition.

---

## 3. Module restructuring

### calib3d → geometry / calib / stereo / ptcloud

The large `calib3d` module has been split into three focused modules.
Besides, some previously experimental functionality from
`opencv_contrib/rgbd` module has been moved to the newly formed `ptcloud`
module that is planned to get much more 3D vision functionality in the
further releases. The table below shows where each piece of functionality
now lives.

| Functionality | OpenCV 4.x | OpenCV 5.x |
| --- | --- | --- |
| Camera calibration (`calibrateCamera`, `stereoCalibrate`, etc.) | `calib3d` | `calib` |
| Stereo (`StereoBM`, `StereoSGBM`, `reprojectImageTo3D`, etc.) | `calib3d` | `stereo` |
| Geometry (`findHomography`, `solvePnP`, `estimateAffine*`, `triangulatePoints`, etc.) | `calib3d` | `geometry` |
| Computational geometry (`convHull`, `Delaunay` etc.) | `imgproc` | `geometry` |
| TSDF, Visual odometry, ptcloud I/O ... | partially in `opencv_contrib/rgbd` | `ptcloud` |

**C++ — the easy path:** the legacy header `opencv2/calib3d.hpp` is still
provided and simply includes `opencv2/geometry.hpp`, `opencv2/calib.hpp`
and `opencv2/stereo.hpp`, so **C++ users can keep their existing includes
and nothing will break.** Switching to the individual headers is
recommended for new code only.

```cpp
// Still works in OpenCV 5.x — no changes required
#include <opencv2/calib3d.hpp>

// Recommended for new code — include only what you need
#include <opencv2/geometry.hpp>  // findHomography, solvePnP, estimateAffine* ...
#include <opencv2/calib.hpp>     // calibrateCamera, stereoCalibrate ...
#include <opencv2/stereo.hpp>    // StereoBM, StereoSGBM ...
```

Function signatures are unchanged in all cases:

```cpp
#include <opencv2/geometry.hpp>
cv::Mat H = cv::findHomography(srcPoints, dstPoints);
cv::solvePnP(objectPoints, imagePoints, K, dist, rvec, tvec);

#include <opencv2/calib.hpp>
cv::stereoCalibrate(objectPoints, imagePoints1, imagePoints2,
                    K1, dist1, K2, dist2, imageSize,
                    R, T, E, F);
```

**Python:** no changes required. All functions remain accessible as
`cv2.<funcname>()`.

**Java:** update your imports.

```java
// Before
import org.opencv.calib3d.Calib3d;
Calib3d.findHomography(srcPoints, dstPoints);
Calib3d.solvePnP(objectPoints, imagePoints, K, dist, rvec, tvec);
Calib3d.stereoCalibrate(objectPoints, imagePoints1, imagePoints2, ...);

// After
import org.opencv.geometry.Geometry;
import org.opencv.calib.Calib;
Geometry.findHomography(srcPoints, dstPoints);
Geometry.solvePnP(objectPoints, imagePoints, K, dist, rvec, tvec);
Calib.stereoCalibrate(objectPoints, imagePoints1, imagePoints2, ...);
```

---

### features2d → features

The `features2d` module has been renamed to `features`.

**C++:**

```cpp
// Before
#include <opencv2/features2d.hpp>

// After
#include <opencv2/features.hpp>
```

Function and class names are unchanged (`cv::SIFT`, `cv::ORB`,
`cv::BFMatcher`, etc.).

**Python:** no changes required.

**Java:**

```java
// Before
import org.opencv.features2d.Features2d;
import org.opencv.features2d.SIFT;

// After
import org.opencv.features.Features;
import org.opencv.features.SIFT;
```

The following detectors and descriptors have been **moved to
`opencv_contrib`** (`xfeatures2d`): SURF, BRIEF, FREAK, LUCID, DAISY, and
several others. SIFT, ORB, FAST, GoodFeaturesToTrack and MSER remain in the
main repository.

---

### ML and G-API moved to contrib

The `ml` and `gapi` modules are no longer part of the main OpenCV
repository. To continue using them, add `opencv_contrib` to your build:

```bash
# Clone opencv_contrib alongside your opencv directory
git clone https://github.com/opencv/opencv_contrib.git

# Pass the path to extra modules when configuring
cmake -DOPENCV_EXTRA_MODULES_PATH=<path_to_opencv_contrib>/modules \
      <path_to_opencv_source>
```

Once built with contrib, the APIs are unchanged — just add the appropriate
headers:

```cpp
#include <opencv2/ml.hpp>   // cv::ml::SVM, cv::ml::RTrees, etc.
#include <opencv2/gapi.hpp> // G-API
```

For Python users migrating away from `cv2.ml.*`,
[scikit-learn](https://scikit-learn.org/) is a well-maintained alternative
that covers the same algorithms and more.

---

### Objdetect: Haar and HOG moved to contrib

`cv::CascadeClassifier` (Haar-based) and `cv::HOGDescriptor` (for
pedestrian detection) have been moved to the `xobjdetect` module in
`opencv_contrib`.

```cpp
// Before
#include <opencv2/objdetect.hpp>
cv::CascadeClassifier face_cascade;
face_cascade.load("haarcascade_frontalface_default.xml");

// After — requires opencv_contrib (see above for build instructions)
#include <opencv2/xobjdetect.hpp>
cv::CascadeClassifier face_cascade;  // same API
face_cascade.load("haarcascade_frontalface_default.xml");
```

For new projects, consider using the DNN-based face detector available in
the main `objdetect` module, which is both faster and more accurate:

```python
# Python — DNN-based face detector (no contrib required)
detector = cv2.FaceDetectorYN.create("face_detection_yunet.onnx", "", (320, 320))
_, faces = detector.detect(image)
```

---

### Imgproc: geometry functions moved to geometry

Computational geometry functions that were previously part of `imgproc`
have been moved to the new `geometry` module: `convexHull`,
`convexityDefects`, `isContourConvex`, `minEnclosingCircle`,
`minEnclosingTriangle`, `minAreaRect`, `fitEllipse`, `Subdiv2D` (Delaunay),
and related functions.

**C++:**

```cpp
// Before
#include <opencv2/imgproc.hpp>
std::vector<cv::Point> hull;
cv::convexHull(contour, hull);

// After — add geometry.hpp; imgproc.hpp still needed for findContours etc.
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
std::vector<cv::Point> hull;
cv::convexHull(contour, hull);  // call unchanged
```

**Python:** no changes required — `cv2.convexHull()` etc. work as before.

**Java:**

```java
// Before
import org.opencv.imgproc.Imgproc;
Imgproc.convexHull(contour, hull);

// After
import org.opencv.geometry.Geometry;
Geometry.convexHull(contour, hull);
```

---

## 4. Core API changes

### 1D and 0D array semantics

In OpenCV 4.x, wrapping a `std::vector<T>` into a `Mat` or passing it as
`InputArray`/`OutputArray` produced a 2D matrix of shape `Nx1` (a column
vector). In OpenCV 5.x it produces a true 1D array.

```cpp
std::vector<float> v = {1.f, 2.f, 3.f};
cv::Mat m(v);

// OpenCV 4.x:  m.dims == 2, m.rows == 3, m.cols == 1
// OpenCV 5.x:  m.dims == 1, m.rows == 1, m.cols == 3
```

Code that checks `.rows` or `.cols` directly on a vector-backed Mat may
need to be updated to use `.total()` instead, which returns the number of
elements in both cases.

Also, note that `m.at<float>(i)`, where `0 <= i < m.total()` works
correctly in both 4.x and 5.x.

To check whether a matrix is empty (as opposed to a 0D scalar), use
`mat.empty()` rather than checking `total() == 0` — both are equivalent,
but `empty()` is clearer.

If you explicitly need the 4.x `Nx1` layout, reshape the array:

```cpp
cv::Mat col = cv::Mat(v).reshape(1, (int)v.size());  // Nx1 column vector
```

---

### MatShape replaces MatSize

`MatSize` has been replaced by `MatShape`, which carries shape and data
layout information and does not require dynamic memory allocation. The
legacy `m.size` accessor continues to work as before; `MatSize` is
available as an alias for source compatibility. New code should prefer
`MatShape` directly.

```cpp
// Before and also After
cv::MatSize sz = m.size;

// After — preferred for new code
cv::MatShape shape = m.shape();
```

---

### New data types

Five new element types have been added. If your code contains a `switch` on
`mat.type()` or `mat.depth()`, add cases for the new types — or, if a
`default` branch was missing, add one now to explicitly catch any
unexpected type rather than silently doing the wrong thing:

| Constant | Type |
| --- | --- |
| `CV_16BF` | bfloat16 |
| `CV_32U` | uint32 |
| `CV_64U` | uint64 |
| `CV_64S` | int64 |
| `CV_Bool` | bool (1 byte) |

```cpp
switch (mat.depth()) {
    case CV_8U:  ...; break;
    case CV_32F: ...; break;
    // Add new cases, or at minimum a default to catch surprises:
    default:
        CV_Error(cv::Error::StsUnsupportedFormat, "Unsupported depth");
}
```

`CV_Bool` matrices can now be used as masks wherever a `CV_8U` or `CV_8S`
mask was previously accepted.

`cv::bfloat` is added in addition to `cv::hfloat`. It can be converted to
and from `float`. Also, there are universal intrinsics (that work on any
platform) to load with expansion and store with compression `bfloat`
values:

```cpp
v_float32 vx_load_expand(const bfloat* data);
void v_pack_store(bfloat* data, v_float32 vec);
```

or you can convert the whole mat/tensor to/from bfloat (as well as all the
other newly added data types):

```cpp
Mat bfloat_mat({N, C, H, W}, CV_16BF, data);
Mat fmat;
bfloat_mat.convertTo(fmat, CV_32F);
```

---

## 5. DNN module

### New default engine

OpenCV 5.0 ships a new inference engine alongside the classic one. By
default, `cv::dnn::readNet()` tries the new engine first and falls back to
the classic engine automatically if the model cannot be loaded. **For most
ONNX models the call site is unchanged:**

```python
# Python — unchanged in both 4.x and 5.x
net = cv2.dnn.readNet("model.onnx")
blob = cv2.dnn.blobFromImage(image, 1/255.0, (640, 640))
net.setInput(blob)
outputs = net.forward()
```

If you need to force a specific engine:

```python
# Python — force a specific engine
net = cv2.dnn.readNetFromONNX("model.onnx", engine=cv2.dnn.ENGINE_CLASSIC)
net = cv2.dnn.readNetFromONNX("model.onnx", engine=cv2.dnn.ENGINE_ORT)
```

```cpp
// C++
cv::dnn::Net net = cv::dnn::readNetFromONNX("model.onnx", cv::dnn::ENGINE_CLASSIC);
```

The engine can also be controlled without code changes via the
`OPENCV_FORCE_DNN_ENGINE` environment variable:

```bash
OPENCV_FORCE_DNN_ENGINE=1  # classic engine
OPENCV_FORCE_DNN_ENGINE=2  # new engine (error if model unsupported)
OPENCV_FORCE_DNN_ENGINE=3  # auto / default
OPENCV_FORCE_DNN_ENGINE=4  # ONNX Runtime (ORT)
```

The new engine currently runs on **CPU only**. If you rely on GPU
inference, force the classic engine or build with ONNX Runtime and NVIDIA
execution providers
(`cmake -DWITH_ONNXRUNTIME=ON -DDOWNLOAD_ONNXRUNTIME_GPU=ON ...
<opencv_source_dir>`).

---

### Darknet and Caffe parsers removed

The `readNetFromDarknet()` and `readNetFromCaffe()` functions have been
removed. Convert your models to ONNX using the tools below, then load with
`readNetFromONNX()`.

| Framework | Conversion tool |
| --- | --- |
| Darknet (YOLO) | [ultralytics export](https://docs.ultralytics.com/modes/export/) or [darknet2onnx](https://github.com/Tianxiaomo/pytorch-YOLOv4/tree/master/tool) |
| Caffe | [caffe-onnx](https://github.com/asiryan/caffe-onnx) |

```python
# Before
net = cv2.dnn.readNetFromCaffe("deploy.prototxt", "weights.caffemodel")
net = cv2.dnn.readNetFromDarknet("yolov4.cfg", "yolov4.weights")

# After — convert to ONNX first, then:
net = cv2.dnn.readNetFromONNX("model.onnx")
```

TFLite models continue to work via the classic engine and require no
changes.

---

### Running VLMs

**[TBD: add snippet and link to sample once the VLM samples are
finalised]** (upstream status at retrieval time)

---

## 6. Imgproc: behavior changes

### Nearest-neighbor resize

The nearest-neighbor interpolation algorithm now matches the behavior of
[Pillow](https://pillow.readthedocs.io/). `INTER_NEAREST` and
`INTER_NEAREST_EXACT` are now equivalent and both follow the Pillow
convention. If your pipeline compares OpenCV resize output to Pillow output
pixel-for-pixel, results will now agree. If you have pixel-exact test
baselines generated with OpenCV 4.x, you may need to regenerate them for
images where the old and new rounding rules differ at boundary pixels.

---

### Accelerated warping

`warpAffine`, `warpPerspective` and `remap` now use revised bilinear and
bicubic interpolation that no longer relies on table-based approximations.
Results are more accurate and generally faster. The API is unchanged, but
**numerical output will differ slightly** from OpenCV 4.x for bilinear and
bicubic modes — the new values are closer to the mathematically exact
result. If you have test baselines that compare warp output pixel-by-pixel
against stored references, update those baselines.

---

### Text rendering

OpenCV 5.0 introduces a new text rendering engine based on STB TrueType,
with a new `cv::FontFace` class and overloaded variants of `putText()` and
`getTextSize()`. The new API supports UTF-8 strings, custom `.ttf`/`.otf`
fonts, and several built-in fonts (`"sans"`, `"italic"`, `"uni"`).

**New API:**

```cpp
// C++ — new FontFace-based API
cv::FontFace font;             // default built-in "sans" font
cv::FontFace uni_font("uni");  // built-in font with better Unicode coverage (including CJK)
cv::FontFace custom_font("myfont.ttf"); // or load a custom font

// putText with FontFace: (img, text, org, color, font, size, weight, flags)
cv::putText(img, "Hello, Unicode!", cv::Point(10, 50),
            cv::Scalar(255, 255, 255), uni_font, 32);

// Right-aligned text
cv::putText(img, "right-aligned", cv::Point(640, 50),
            cv::Scalar(255, 255, 255), custom_font, 32, 0,
            cv::PUT_TEXT_ALIGN_RIGHT);
```

```python
# Python — new FontFace-based API
sans_font = cv2.FontFace("sans")  # built-in sans font

cv2.putText(img, "text", (10, 50),
            (255, 255, 255), sans_font, 32)
```

**Legacy API** (`FONT_HERSHEY_*`) **is still available** and requires no
code changes. However, under the hood it now uses the new TrueType engine
with the embedded [Rubik](https://fonts.google.com/specimen/Rubik) font as
the default. As a result:

- Text **size** will differ slightly from OpenCV 4.x.
- Text **appearance** will differ noticeably — Rubik is a modern,
  well-designed font that looks considerably better than the old Hershey
  vector fonts, but it is a different look.

If you specifically want the Hershey font appearance, you can download
Hershey fonts in `.ttf` format and load them explicitly via `FontFace`.
Note that even then the output will not be pixel-identical to OpenCV 4.x,
since the rendering engine itself has changed.

```cpp
// Legacy API — still compiles and runs, but visual output differs from 4.x
cv::putText(img, "Hello", cv::Point(10, 50),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
```

---

## 7. Video I/O module

### VideoCapture::get() return value for unsupported properties

In OpenCV 4.x, `VideoCapture::get()` returned `0` when a property is not
supported by the current backend. This was ambiguous: `0` is also a valid
value for many properties (e.g. `CAP_PROP_AUTO_EXPOSURE` set to off),
making it impossible to reliably distinguish "unsupported" from "supported
but zero".

In OpenCV 5.0, unsupported properties return `-1`. If your code checks the
return value of `get()` to detect unsupported properties, update the check
accordingly. Use `< 0` rather than `== -1` to future-proof against
additional special negative values that may be introduced later.

```cpp
// Before — unreliable, 0 is also a valid property value
double val = cap.get(CAP_PROP_AUTOFOCUS);
if (val == 0) {
    // could be "unsupported" or "autofocus off"
}

// After
double val = cap.get(CAP_PROP_AUTOFOCUS);
if (val < 0) {
    // property is not supported by this backend
}
```

```python
# Python
val = cap.get(cv2.CAP_PROP_AUTOFOCUS)
if val < 0:
    # property is not supported by this backend
    pass
```

Upstream reference: [opencv/opencv#21337](https://github.com/opencv/opencv/pull/21337).

---

## 8. Python bindings

A summary of Python-specific points:

- **Python 2 is no longer supported.** Use Python 3.6 or later.
- **Module restructuring** is fully transparent: all functions remain
  accessible as `cv2.<funcname>()` regardless of which C++ module they now
  live in. No import changes are needed.
- **DNN:** `readNetFromONNX()` is unchanged for ONNX models. Caffe and
  Darknet loaders are gone; convert models to ONNX.
- **1D arrays:** `np.array` passed as `InputArray` now maps to a 1D `Mat`.
  Code that assumed a column-vector layout may need adjustment (see
  [1D and 0D array semantics](#1d-and-0d-array-semantics)).
- **ML module:** no longer available via `cv2.ml.*` unless you build with
  `opencv_contrib`. Consider migrating to scikit-learn.
- **Haar/HOG detectors:** `cv2.CascadeClassifier` and
  `cv2.HOGDescriptor` require `opencv_contrib`. See
  [Objdetect](#objdetect-haar-and-hog-moved-to-contrib).
