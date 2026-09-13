extends SceneTree
## Opt-in render audit. Does not run in the game or enter the Web export.
## Use real wall time, never --fixed-fps. See docs/ARCHITECTURE_PERFORMANCE_AUDIT_2026-09-12.md.

const SCENE = preload("res://scenes/flight_lab.tscn")
const VIEWS := {
	"depot": Vector3(-22, 54.4, 0),
	"smog": Vector3(0, -25, 0),
	"eden": Vector3(-22, 190, 0),
}
var _scene: Node3D
var _variant := "baseline"
var _duration := 2.0
var _output := "res://build/performance-audit.json"
var _warmup := false
var _profile := "desktop"

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	# Godot consumes --fixed-fps before OS.get_cmdline_args(). Callers must omit
	# it; the documented command uses real-time simulation and wall-clock samples.
	for argument in OS.get_cmdline_user_args():
		if argument == "--warmup":
			_warmup = true
		elif argument.begins_with("--quality="):
			_profile = argument.trim_prefix("--quality=")
		elif argument.begins_with("--variant="):
			_variant = argument.trim_prefix("--variant=")
		elif argument.begins_with("--seconds="):
			_duration = maxf(0.5, argument.trim_prefix("--seconds=").to_float())
		elif argument.begins_with("--output="):
			_output = argument.trim_prefix("--output=")
	if _variant not in ["baseline", "no-smog-banks", "no-headlight-shadows", "no-shadows", "no-post", "half-resolution", "no-local-lights", "inventory"]:
		push_error("Unknown audit variant: " + _variant)
		quit(1)
		return
	if DisplayServer.get_name() == "headless" and _variant != "inventory":
		push_error("Use a real renderer for timings; headless only supports --variant=inventory.")
		quit(1)
		return
	_scene = SCENE.instantiate()
	root.add_child(_scene)
	current_scene = _scene
	var cab: FlightCab = _scene.get_node("Cab")
	var policy := RenderPolicy.new()
	policy.apply(root, _scene, _profile)
	cab.freeze = true
	_scene.set_physics_process(false)
	var result := {
		"variant": _variant,
		"quality": policy.profile,
		"warmup": _warmup,
		"engine": Engine.get_version_info(),
		"os": OS.get_name(),
		"renderer": RenderingServer.get_current_rendering_method(),
		"adapter": RenderingServer.get_video_adapter_name(),
		"window_pixels": [root.size.x, root.size.y],
		"logical_viewport": [root.get_visible_rect().size.x, root.get_visible_rect().size.y],
		"vsync": DisplayServer.window_get_vsync_mode(),
		"physics_hz": Engine.physics_ticks_per_second,
		"inventory": _inventory(),
		"samples": [],
		"limitations": "Desktop frozen-camera render workload; not a phone, WebGL, moving-flight, input-latency or GPU-time benchmark. First visits include transitions; no driver cache was cleared. Effects of propulsion/express are not exercised.",
	}
	if _variant != "inventory":
		if _variant == "half-resolution":
			root.scaling_3d_scale = 0.5
		var environment: Environment = _scene.get_node("WorldEnvironment").environment
		if _variant == "no-post":
			environment.ssao_enabled = false
			environment.glow_enabled = false
		if _variant == "no-shadows":
			for node in _scene.find_children("*", "Light3D", true, false):
				node.shadow_enabled = false
		if _variant == "no-headlight-shadows":
			for node in cab.get_node("Headlights").spots:
				node.shadow_enabled = false
		if _variant == "no-local-lights":
			# Culling masks isolate illumination without being reset by light scripts.
			for node in _scene.find_children("*", "Light3D", true, false):
				if not node is DirectionalLight3D:
					node.light_cull_mask = 0
		if _variant == "no-smog-banks":
			_scene.get_node("City/Smog").hide()
		result["scale_3d"] = root.scaling_3d_scale
		result["msaa_3d"] = root.msaa_3d
		if _warmup:
			var overlay := CanvasLayer.new()
			var cover := ColorRect.new()
			cover.color = Color("09151e")
			cover.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
			root.add_child(overlay)
			overlay.add_child(cover)
			_scene.context.player.suspend(&"benchmark_warmup")
			var preparation := GraphicsWarmup.new()
			var started := Time.get_ticks_usec()
			await preparation.prepare(_scene)
			result["warmup_ms"] = float(Time.get_ticks_usec() - started) / 1000.0
			result["warmup_views"] = preparation.completed_views
			_scene.context.player.resume(&"benchmark_warmup")
			overlay.queue_free()
			await process_frame
		for pass_index in range(2):
			for view_name: String in VIEWS:
				cab.global_position = VIEWS[view_name]
				cab.reset_physics_interpolation()
				_scene._snap_camera()
				var sample: Dictionary = await _measure()
				sample["view"] = view_name
				sample["pass"] = pass_index + 1
				result["samples"].append(sample)
				print("AUDIT ", _variant, " ", view_name, " pass=", pass_index + 1, " p95_ms=", sample["steady_frame_ms"]["p95"], " max_ms=", sample["steady_frame_ms"]["max"], " draws=", sample["draw_calls"]["median"])
	DirAccess.make_dir_recursive_absolute(_output.get_base_dir())
	var file := FileAccess.open(_output, FileAccess.WRITE)
	if file == null:
		push_error("Cannot write audit: " + _output)
		quit(1)
		return
	file.store_string(JSON.stringify(result, "\t") + "\n")
	print("AUDIT_SAVED ", _output, " inventory=", result["inventory"])
	quit()

func _measure() -> Dictionary:
	var transition := PackedFloat64Array()
	var steady := PackedFloat64Array()
	var draws := PackedFloat64Array()
	var primitives := PackedFloat64Array()
	var started := Time.get_ticks_usec()
	var previous := started
	# Separate the first 0.75 s (camera/light transition) from steady rendering.
	while float(Time.get_ticks_usec() - started) / 1e6 < _duration + 0.75:
		await process_frame
		var now := Time.get_ticks_usec()
		# A long interval that starts during transition belongs to transition even
		# if the stall itself crosses the 0.75 s boundary.
		var transition_interval := previous - started < 750000
		var milliseconds := float(now - previous) / 1000.0
		previous = now
		if transition_interval:
			transition.append(milliseconds)
		else:
			steady.append(milliseconds)
			draws.append(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
			primitives.append(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))
	var over_50 := 0
	var over_100 := 0
	for value in steady:
		over_50 += int(value > 50.0)
		over_100 += int(value > 100.0)
	return {
		"transition_frame_ms": _distribution(transition),
		"steady_frame_ms": _distribution(steady),
		"draw_calls": _distribution(draws),
		"primitives": _distribution(primitives),
		"steady_frames_over_50ms": over_50,
		"steady_frames_over_100ms": over_100,
	}

func _distribution(values: PackedFloat64Array) -> Dictionary:
	if values.is_empty():
		return {"count": 0, "median": 0, "p95": 0, "p99": 0, "max": 0}
	values.sort()
	return {
		"count": values.size(),
		"median": values[values.size() / 2],
		"p95": values[mini(values.size() - 1, ceili(values.size() * 0.95) - 1)],
		"p99": values[mini(values.size() - 1, ceili(values.size() * 0.99) - 1)],
		"max": values[-1],
	}

func _inventory() -> Dictionary:
	var counts := {}
	var meshes := {}
	var materials := {}
	var shadow_meshes := 0
	for node in _scene.find_children("*", "", true, false):
		var kind: String = node.get_class()
		counts[kind] = counts.get(kind, 0) + 1
		if node is MeshInstance3D:
			if node.mesh:
				meshes[node.mesh.get_instance_id()] = true
			if node.material_override:
				materials[node.material_override.get_instance_id()] = true
			shadow_meshes += int(node.cast_shadow != GeometryInstance3D.SHADOW_CASTING_SETTING_OFF)
	return {"nodes_by_class": counts, "unique_mesh_resources": meshes.size(), "unique_override_materials": materials.size(), "mesh_shadow_casting_enabled": shadow_meshes}
