extends SceneTree

# Run with a real renderer and --fixed-fps (e.g. 120 or 144).
# The fixture removes gravity and collisions to isolate uninterrupted cruising.
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(condition: bool, label: String) -> void:
	if not condition:
		failures += 1
		push_error("FAIL: " + label)
	else:
		print("PASS: " + label)

func displayed_position(cab: FlightCab) -> Vector3:
	if is_physics_interpolation_enabled():
		return cab.get_global_transform_interpolated().origin
	return cab.global_position

func _run() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--physics-hz="):
			Engine.physics_ticks_per_second = int(argument.get_slice("=", 1))
	var scene: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	var cab: FlightCab = scene.get_node("Cab")
	cab.definition = cab.definition.duplicate()
	cab.definition.gravity = 0.0
	cab.definition.airspace_enabled = false
	cab.definition.fuel_enabled = false
	cab.definition.highway_enabled = false
	cab.collision_layer = 0
	cab.collision_mask = 0
	cab.position = Vector3(-55, 40, 0)
	root.add_child(scene)
	current_scene = scene
	await scene.prepare_gameplay()
	# Drive the body directly so OS focus changes cannot cancel the test input.
	# The regular flight suite separately exercises the actual input path.
	scene.set_physics_process(false)
	var camera: Camera3D = scene.get_node("Camera3D")
	await RenderingServer.frame_post_draw
	cab.command = Vector2.RIGHT
	var elapsed := 0.0
	var previous_x := 0.0
	var samples := 0
	var delta_sum := 0.0
	var delta_squared_sum := 0.0
	var peak_step := 0.0
	var min_speed := INF
	var max_speed := 0.0
	while elapsed < 10.0:
		# Model held input throughout the capture, including after a window-focus event.
		cab.command = Vector2.RIGHT
		await RenderingServer.frame_post_draw
		elapsed += root.get_process_delta_time()
		var screen_x := camera.unproject_position(displayed_position(cab)).x
		if elapsed > 3.0:
			if samples > 0:
				var step := screen_x - previous_x
				delta_sum += step
				delta_squared_sum += step * step
				peak_step = maxf(peak_step, absf(step))
			samples += 1
			min_speed = minf(min_speed, cab.linear_velocity.x)
			max_speed = maxf(max_speed, cab.linear_velocity.x)
		previous_x = screen_x
	var mean := delta_sum / maxf(1.0, samples - 1)
	var jitter := sqrt(maxf(0.0, delta_squared_sum / maxf(1.0, samples - 1) - mean * mean))
	print("PRESENTATION: physics=%d Hz, samples=%d, speed=%.5f..%.5f m/s, screen step stddev=%.5f px, peak=%.5f px" % [Engine.physics_ticks_per_second, samples, min_speed, max_speed, jitter, peak_step])
	check(samples > 100 and min_speed > 10.49 and max_speed < 10.51, "sustained input maintains cruise speed without interruptions")
	check(jitter < 0.15 and peak_step < 0.3, "cab stays steady relative to the following camera during cruise")
	scene._reset()
	var reset_error := 0.0
	var camera_reset_error := 0.0
	var reset_seen := false
	for i in range(40):
		await RenderingServer.frame_post_draw
		if not cab.resetting:
			reset_seen = true
			reset_error = maxf(reset_error, displayed_position(cab).distance_to(cab.spawn_transform.origin))
			camera_reset_error = maxf(camera_reset_error, camera.unproject_position(displayed_position(cab)).distance_to(root.get_visible_rect().size * 0.5))
	check(reset_seen and reset_error < 0.01, "reset displays the spawn immediately without interpolating across the map")
	check(reset_seen and camera_reset_error < 0.1, "camera snaps with the body on reset without a frame of separation")
	print("PRESENTATION_TESTS: %s" % ("PASS" if failures == 0 else "FAIL"))
	quit(0 if failures == 0 else 1)
