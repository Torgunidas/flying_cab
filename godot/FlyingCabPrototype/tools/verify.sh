#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/godot-env.sh"
if [[ "${1:-}" == "vehicle-access" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/vehicle_access_tests.gd --log-file "$FC_PROJECT_DIR/build/vehicle-access-tests.log"
fi
if [[ "${1:-}" == "vehicle-access-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --fixed-fps 120 --script tests/vehicle_access_tests.gd --log-file "$FC_PROJECT_DIR/build/vehicle-access-render-tests.log"
fi
if [[ "${1:-}" == "map-tracking" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/map_tracking_tests.gd --log-file "$FC_PROJECT_DIR/build/map-tracking-tests.log"
fi
if [[ "${1:-}" == "map-tracking-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --script tests/map_tracking_tests.gd --log-file "$FC_PROJECT_DIR/build/map-tracking-render-tests.log"
fi
if [[ "${1:-}" == "quest-map" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/quest_map_tests.gd --log-file "$FC_PROJECT_DIR/build/quest-map-tests.log"
fi
if [[ "${1:-}" == "quest-map-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --script tests/quest_map_tests.gd --log-file "$FC_PROJECT_DIR/build/quest-map-render-tests.log"
fi
if [[ "${1:-}" == "story-editor" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/story_editor_tests.gd --log-file "$FC_PROJECT_DIR/build/story-editor-tests.log"
fi
if [[ "${1:-}" == "story-editor-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --script tests/story_editor_render_tests.gd --log-file "$FC_PROJECT_DIR/build/story-editor-render-tests.log"
fi
if [[ "${1:-}" == "platform-camera" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/platform_camera_tests.gd --log-file "$FC_PROJECT_DIR/build/platform-camera-tests.log"
fi
if [[ "${1:-}" == "platform-camera-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --disable-vsync --script tests/platform_camera_tests.gd --log-file "$FC_PROJECT_DIR/build/platform-camera-render-tests.log"
fi
if [[ "${1:-}" == "start-menu" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/start_menu_tests.gd --log-file "$FC_PROJECT_DIR/build/start-menu-tests.log"
fi
if [[ "${1:-}" == "start-menu-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --script tests/start_menu_tests.gd --log-file "$FC_PROJECT_DIR/build/start-menu-render-tests.log" -- --capture-only
fi
if [[ "${1:-}" == "narrative-validate" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tools/validate_narrative.gd --log-file "$FC_PROJECT_DIR/build/narrative-validation.log"
fi
if [[ "${1:-}" == "narrative" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --script tests/narrative_tests.gd --log-file "$FC_PROJECT_DIR/build/narrative-tests.log"
fi
if [[ "${1:-}" == "narrative-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --script tests/narrative_render_tests.gd --log-file "$FC_PROJECT_DIR/build/narrative-render-tests.log"
fi
if [[ "${1:-}" == "test-yard" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/test_yard_tests.gd --log-file "$FC_PROJECT_DIR/build/test-yard-tests.log"
fi
if [[ "${1:-}" == "test-yard-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 1400x850 --disable-vsync --script tests/test_yard_tests.gd --log-file "$FC_PROJECT_DIR/build/test-yard-render.log" -- --capture-only
fi
if [[ "${1:-}" == "living-world-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --disable-vsync --script tests/living_world_render_tests.gd --log-file "$FC_PROJECT_DIR/build/living-world-render-tests.log"
fi
if [[ "${1:-}" == "perimeter-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --fixed-fps "${2:-120}" --disable-vsync --script tests/perimeter_presentation_tests.gd --log-file "$FC_PROJECT_DIR/build/perimeter-presentation-tests.log"
fi
if [[ "${1:-}" == "living-world" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/living_world_tests.gd --log-file "$FC_PROJECT_DIR/build/living-world-tests.log"
fi
if [[ "${1:-}" == "human-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 1400x1100 --disable-vsync --script tests/human_presentation_tests.gd --log-file "$FC_PROJECT_DIR/build/human-presentation.log"
fi
if [[ "${1:-}" == "walking" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/walking_tests.gd --log-file "$FC_PROJECT_DIR/build/walking-tests.log"
fi
if [[ "${1:-}" == "on-foot-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --disable-vsync --script tests/on_foot_render_tests.gd --log-file "$FC_PROJECT_DIR/build/on-foot-render-tests.log"
fi
if [[ "${1:-}" == "on-foot" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/on_foot_tests.gd --log-file "$FC_PROJECT_DIR/build/on-foot-tests.log"
fi
if [[ "${1:-}" == "models" ]]; then
    exec "$FC_GODOT_BIN" --headless --path "$FC_PROJECT_DIR" --fixed-fps 120 --script tests/vehicle_models_tests.gd --log-file "$FC_PROJECT_DIR/build/vehicle-models-tests.log"
fi
if [[ "${1:-}" == "models-render" ]]; then
    exec "$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 1400x1100 --disable-vsync --script tests/vehicle_showroom_render_tests.gd --log-file "$FC_PROJECT_DIR/build/vehicle-showroom-render.log"
fi
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
