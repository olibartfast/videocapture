#!/bin/bash

# Individual GStreamer setup script for VideoCapture
# This script is a convenience wrapper around the unified setup script

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNIFIED_SCRIPT="$SCRIPT_DIR/setup_dependencies.sh"

if [[ ! -f "$UNIFIED_SCRIPT" ]]; then
    echo "Error: Unified setup script not found at $UNIFIED_SCRIPT"
    exit 1
fi

echo "GStreamer Setup for VideoCapture"
echo "================================"
echo "This script will install GStreamer dependencies."
echo ""

# Run the unified script with GStreamer enabled
exec "$UNIFIED_SCRIPT" --gstreamer "$@" 