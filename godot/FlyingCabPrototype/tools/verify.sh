#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/godot-env.sh"
if [[ "${1:-}" == "taxi-guidance" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/taxi_guidance_tests.gd --log-file "$FC_PROJECT_DIR/build/taxi-guidance-tests.log"
fi
if [[ "${1:-}" == "thrust" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/thrust_response_tests.gd --log-file "$FC_PROJECT_DIR/build/thrust-response-tests.log"
fi
if [[ "${1:-}" == "taxi-city" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/taxi_city_population_tests.gd --log-file "$FC_PROJECT_DIR/build/taxi-city-tests.log"
fi
if [[ "${1:-}" == "taxi-ui" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/taxi_ui_tests.gd --log-file "$FC_PROJECT_DIR/build/taxi-ui-tests.log"
fi
if [[ "${1:-}" == "taxi-interaction" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/taxi_interaction_tests.gd --log-file "$FC_PROJECT_DIR/build/taxi-interaction-tests.log"
fi
if [[ "${1:-}" == "import" ]]; then
    exec python3 "$FC_PROJECT_DIR/tools/check_clean_import.py" --engine "$FC_GODOT_BIN"
fi
if [[ "${1:-}" == "taxi-route" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 60 --script tests/taxi_route_tests.gd --log-file "$FC_PROJECT_DIR/build/taxi-route-tests.log"
fi
if [[ "${1:-}" == "taxi" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/taxi_tests.gd --log-file "$FC_PROJECT_DIR/build/taxi-tests.log"
fi
if [[ "${1:-}" == "vehicles" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/vehicle_tests.gd --log-file "$FC_PROJECT_DIR/build/vehicle-tests.log"
fi
if [[ "${1:-}" == "compilation" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/city_compilation_tests.gd --log-file "$FC_PROJECT_DIR/build/compilation-tests.log"
fi
if [[ "${1:-}" == "architecture" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/architecture_tests.gd --log-file "$FC_PROJECT_DIR/build/architecture-tests.log"
fi
if [[ "${1:-}" == "boot" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --disable-vsync --script tests/boot_tests.gd --log-file "$FC_PROJECT_DIR/build/boot-tests.log" -- --quality=balanced
fi
if [[ "${1:-}" == "fx" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/flight_fx_tests.gd --log-file "$FC_PROJECT_DIR/build/flight-fx-tests.log"
fi
if [[ "${1:-}" == "lanes" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --fixed-fps 120 --disable-vsync --script tests/lane_render_tests.gd --log-file "$FC_PROJECT_DIR/build/lane-render-tests.log"
fi
if [[ "${1:-}" == "smog" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/smog_tests.gd --log-file "$FC_PROJECT_DIR/build/smog-tests.log"
fi
if [[ "${1:-}" == "city" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/city_tests.gd --log-file "$FC_PROJECT_DIR/build/city-tests.log"
fi
if [[ "${1:-}" == "airspace" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/airspace_tests.gd --log-file "$FC_PROJECT_DIR/build/airspace-tests.log"
fi
if [[ "${1:-}" == "idle" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/idle_tests.gd --log-file "$FC_PROJECT_DIR/build/idle-tests.log"
fi
if [[ "${1:-}" == "presentation" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --fixed-fps "${2:-120}" --disable-vsync --script tests/presentation_tests.gd --log-file "$FC_PROJECT_DIR/build/presentation-tests.log" -- "--physics-hz=${3:-60}"
fi
exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 60 --script tests/flight_tests.gd --log-file "$FC_PROJECT_DIR/build/tests.log"
