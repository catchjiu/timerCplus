#!/bin/bash
# BJJ Timer - LVGL GUI Build (Raspberry Pi / aarch64)
# Workaround: Helium assembly fails on aarch64 Linux - temporarily disable it

set -e
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_ROOT"

HELIUM_SRC="$PROJECT_ROOT/lvgl/src/draw/sw/blend/helium/lv_blend_helium.S"
NEED_RESTORE=0

# On aarch64, rename helium .S to avoid assembly build failure
if [ -f "$HELIUM_SRC" ] && [ "$(uname -m)" = "aarch64" ]; then
  mv "$HELIUM_SRC" "${HELIUM_SRC}.disabled"
  NEED_RESTORE=1
  echo "Disabled lv_blend_helium.S for aarch64 build"
  restore() { cd "$PROJECT_ROOT" && [ -f "${HELIUM_SRC}.disabled" ] && mv "${HELIUM_SRC}.disabled" "$HELIUM_SRC" && echo "Restored lv_blend_helium.S"; }
  trap restore EXIT
fi

# Build
mkdir -p build
cd build
cmake ..
make "$@"

echo "Done: sudo ./build/bjj_timer_gui"
