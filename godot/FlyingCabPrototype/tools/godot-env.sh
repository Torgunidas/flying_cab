#!/bin/bash
# Source from the project's launchers. Engine installation paths stay local.
FC_PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [ -z "${FC_GODOT_BIN:-}" ] && [ -f "$FC_PROJECT_DIR/build/godot-path.txt" ]; then
    IFS= read -r FC_GODOT_BIN < "$FC_PROJECT_DIR/build/godot-path.txt" || true
    FC_GODOT_BIN="${FC_GODOT_BIN%$'\r'}"
fi
if [ -z "${FC_GODOT_BIN:-}" ]; then
    if [ -x "$FC_PROJECT_DIR/build/tools/Godot.app/Contents/MacOS/Godot" ]; then
        FC_GODOT_BIN="$FC_PROJECT_DIR/build/tools/Godot.app/Contents/MacOS/Godot"
    elif [ -f "$FC_PROJECT_DIR/build/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe" ]; then
        FC_GODOT_BIN="$FC_PROJECT_DIR/build/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
    elif command -v godot >/dev/null 2>&1; then
        FC_GODOT_BIN="$(command -v godot)"
    elif [ -x /Applications/Godot.app/Contents/MacOS/Godot ]; then
        FC_GODOT_BIN=/Applications/Godot.app/Contents/MacOS/Godot
    else
        echo "Zainstaluj Godot 4.7.2 lub ustaw FC_GODOT_BIN na jego plik wykonywalny."
        exit 1
    fi
fi
mkdir -p "$FC_PROJECT_DIR/build"
touch "$FC_PROJECT_DIR/build/.gdignore"
