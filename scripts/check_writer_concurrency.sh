#!/usr/bin/env bash
# Acceptance for the C++20 writer modernization; run from any directory.
set -euo pipefail
repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_root=${1:-"${TMPDIR:-/tmp}/videocapture-writer-concurrency"}
cd "$repo_dir"

# Same full-tree check as .github/workflows/format.yml.
while IFS= read -r -d '' file; do
    clang-format --dry-run -Werror "$file"
done < <(find src include tests app -name '*.cpp' -print0 -o -name '*.hpp' -print0 -o -name '*.h' -print0 -o -name '*.c' -print0)
git diff --check

for backend in opencv gstreamer ffmpeg all; do
    ffmpeg=OFF
    gstreamer=OFF
    case "$backend" in
        ffmpeg) ffmpeg=ON ;;
        gstreamer) gstreamer=ON ;;
        all) ffmpeg=ON; gstreamer=ON ;;
    esac
    cmake -S . -B "$build_root/$backend" \
        -DCMAKE_BUILD_TYPE=Debug -DENABLE_CCACHE=OFF \
        -DBUILD_TESTS=ON -DUSE_VIDEOWRITER=ON \
        -DUSE_FFMPEG="$ffmpeg" -DUSE_GSTREAMER="$gstreamer"
    cmake --build "$build_root/$backend" --parallel 4
    ctest --test-dir "$build_root/$backend" --output-on-failure --timeout 60
done
