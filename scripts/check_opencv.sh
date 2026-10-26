#!/usr/bin/env bash
# Validate an explicit OpenCV installation, including capture/writer round trips.
set -euo pipefail

if [[ $# -ne 2 || ! -f "$1/OpenCVConfig.cmake" ]]; then
    echo "Usage: $0 <directory-containing-OpenCVConfig.cmake> <build-directory>" >&2
    exit 2
fi

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
opencv_dir=$(cd -- "$1" && pwd)
cmake -S "$repo_root" -B "$2" \
    -DOpenCV_DIR="$opencv_dir" \
    -DUSE_GSTREAMER=OFF -DUSE_FFMPEG=OFF \
    -DUSE_VIDEOWRITER=ON -DBUILD_TESTS=ON \
    -DVIDEOCAPTURE_APP_RENDERER=OPENCV
cmake --build "$2" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
VIDEOCAPTURE_REQUIRE_WRITER_CODEC=1 ctest --test-dir "$2" --output-on-failure
