#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/godot-env.sh"
if [ ! -f "$FC_PROJECT_DIR/build/templates/web_release.zip" ]; then
    echo "Rozpakuj templates/web_release.zip i web_debug.zip z oficjalnych szablonów Godot 4.7.2 do build/templates/."
    exit 1
fi
mkdir -p "$FC_PROJECT_DIR/build/web"
# Compile authored geometry explicitly before export; never generate it at Play.
"$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tools/compile_city.gd --log-file "$FC_PROJECT_DIR/build/compile-city.log"
python3 "$FC_PROJECT_DIR/tools/stamp_build.py"
"$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --export-release Web --log-file "$FC_PROJECT_DIR/build/export-web.log"
# Do not package an incomplete or failed export, even if the editor returned 0.
if [ ! -s "$FC_PROJECT_DIR/build/web/index.html" ] || [ ! -s "$FC_PROJECT_DIR/build/web/index.wasm" ] || [ ! -s "$FC_PROJECT_DIR/build/web/index.pck" ]; then
    echo "Eksport nie jest kompletny. Sprawdź build/export-web.log."
    exit 1
fi
if /usr/bin/grep -E 'SCRIPT ERROR|ERROR:|crashed with signal' "$FC_PROJECT_DIR/build/export-web.log"; then
    echo "Eksport zgłosił błąd; nie utworzono paczki."
    exit 1
fi
# Re-open the actual exported PCK and compare every batched transform with the
# authored geometry. A successful editor export is not proof of valid scene data.
"$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/city_compilation_tests.gd --log-file "$FC_PROJECT_DIR/build/export-geometry-check.log" -- "--pack=$FC_PROJECT_DIR/build/web/index.pck"
python3 "$FC_PROJECT_DIR/tools/package_web.py"
