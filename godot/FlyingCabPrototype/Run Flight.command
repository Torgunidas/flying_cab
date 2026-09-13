#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/tools/godot-env.sh"
exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --log-file "$FC_PROJECT_DIR/build/last-run.log"
