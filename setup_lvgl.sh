#!/bin/bash
# BJJ Timer - LVGL GUI Setup
# Run this once to add the LVGL submodule

set -e
cd "$(dirname "$0")"

if [ -d "lvgl" ] && [ -f "lvgl/lvgl.h" ]; then
    echo "LVGL already present."
    git submodule update --init --recursive 2>/dev/null || true
    exit 0
fi

echo "Adding LVGL submodule..."
git submodule add https://github.com/lvgl/lvgl.git lvgl
git submodule update --init --recursive

echo "Done. Build with: mkdir -p build && cd build && cmake .. && make"
