#!/usr/bin/env bash
# macOS build script — companion to compile.bat.
#
# Tested against Houdini 21.0 on Apple Silicon. Differs from the Windows/Linux
# build in three places because Houdini 21 ships OpenEXR 3:
#   - half.h moved from .../include/OpenEXR/ to .../include/Imath/
#   - libHalf is gone; Half is now part of libImath_sidefx
#   - The macOS Libraries dir lives at $HFS/../Libraries
#
# Usage:
#   1. Open a Terminal and source Houdini's setup:
#        cd "/Applications/Houdini/Current/Frameworks/Houdini.framework/Versions/Current/Resources"
#        source houdini_setup_bash
#   2. cd back to this repo and run: bash compile_macos.sh
#
# Output: Houdini_Template/PRT_RopDriver.dylib, also auto-installed by hcustom
# to ~/Library/Preferences/houdini/<ver>/dso/.

set -euo pipefail

if [ -z "${HFS:-}" ]; then
    echo "ERROR: \$HFS is not set. Source houdini_setup_bash first." >&2
    echo "  cd /Applications/Houdini/Current/Frameworks/Houdini.framework/Versions/Current/Resources" >&2
    echo "  source houdini_setup_bash" >&2
    exit 1
fi

cd "$(dirname "$0")/Houdini_Template"

hcustom \
    -I "$HFS/toolkit/include" \
    -I "$HFS/toolkit/include/Imath" \
    -I "$HFS/toolkit/include/OpenEXR" \
    -L "$HFS/../Libraries" \
    -l Imath_sidefx \
    -l z \
    PRT_RopDriver.cpp
