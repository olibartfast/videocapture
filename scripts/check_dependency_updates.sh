#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 6 ]]; then
    echo "Usage: $0 <versions.env> <report.md> <opencv> <gstreamer> <ffmpeg> <cmake>" >&2
    exit 2
fi

versions_file=$1
report_file=$2
opencv_candidate=$3
gstreamer_candidate=$4
ffmpeg_candidate=$5
cmake_candidate=$6

# shellcheck disable=SC1090
source "$versions_file"

: "${UBUNTU_RELEASE:?UBUNTU_RELEASE is required}"
: "${OPENCV_REVIEWED_VERSION:?OPENCV_REVIEWED_VERSION is required}"
: "${GSTREAMER_REVIEWED_VERSION:?GSTREAMER_REVIEWED_VERSION is required}"
: "${FFMPEG_REVIEWED_VERSION:?FFMPEG_REVIEWED_VERSION is required}"
: "${CMAKE_REVIEWED_VERSION:?CMAKE_REVIEWED_VERSION is required}"

updates_available=false

append_result() {
    local name=$1
    local reviewed=$2
    local candidate=$3
    local status="Reviewed"

    if [[ -z "$candidate" || "$candidate" == "(none)" ]]; then
        echo "No package candidate found for $name" >&2
        return 1
    fi

    if dpkg --compare-versions "$candidate" gt "$reviewed"; then
        status="Update available"
        updates_available=true
    elif dpkg --compare-versions "$candidate" lt "$reviewed"; then
        status="Candidate older than reviewed baseline"
    fi

    printf "| %s | \`%s\` | \`%s\` | %s |\n" "$name" "$reviewed" "$candidate" "$status"
}

{
    echo "Ubuntu runner release: **${UBUNTU_RELEASE}**"
    echo
    echo '| Dependency | Last reviewed package | Current candidate | Status |'
    echo '| --- | --- | --- | --- |'
    append_result "OpenCV" "$OPENCV_REVIEWED_VERSION" "$opencv_candidate"
    append_result "GStreamer" "$GSTREAMER_REVIEWED_VERSION" "$gstreamer_candidate"
    append_result "FFmpeg" "$FFMPEG_REVIEWED_VERSION" "$ffmpeg_candidate"
    append_result "CMake" "$CMAKE_REVIEWED_VERSION" "$cmake_candidate"
} > "$report_file"

echo "$updates_available"
