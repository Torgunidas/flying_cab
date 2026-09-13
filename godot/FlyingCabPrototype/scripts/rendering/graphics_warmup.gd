class_name GraphicsWarmup
extends RefCounted
## Actually renders the loaded map, including normally hidden effects, behind UI.
## Caller owns the loading overlay and session lock; no scene content is created.
signal progressed(fraction: float)
var completed_views := 0

func prepare(level: Node3D) -> void:
	completed_views = 0
	if DisplayServer.get_name() == "headless":
		return
	if not level.has_node("Cab") or not level.has_method("_snap_camera"):
		if level.has_method("prepare_graphics"):
			await level.prepare_graphics()
		else:
			await RenderingServer.frame_post_draw
			await RenderingServer.frame_post_draw
		return
	var previous_mode := level.process_mode
	level.process_mode = Node.PROCESS_MODE_DISABLED
	var cab: FlightCab = level.get_node("Cab")
	var camera: Camera3D = level.get_node("Camera3D")
	var atmosphere := level.get_node("Atmosphere")
	var lights := cab.get_node("Headlights")
	var saved_transform := cab.global_transform
	var saved_camera := camera.global_transform
	var was_frozen := cab.freeze
	cab.freeze = true
	var points := _points(level.definition)
	# Include the final spawn first and last, so startup shaders are covered and
	# the final lighting/culling combination has already rendered before handover.
	points.insert(0, saved_transform.origin)
	points.append(saved_transform.origin)
	for i in range(points.size()):
		cab.global_position = points[i]
		cab.reset_physics_interpolation()
		level._snap_camera()
		atmosphere.apply_height(points[i].y)
		lights.update_lights(points[i].y, 0.0, true)
		cab.flight_fx.show_warmup_effects()
		# Two completed draws also cover the first frame after visibility changes.
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		completed_views += 1
		progressed.emit(float(i + 1) / points.size())
	cab.global_transform = saved_transform
	cab.reset_physics_interpolation()
	cab.flight_fx.reset_visuals()
	lights.update_lights(saved_transform.origin.y, 0.0, true)
	atmosphere.apply_height(saved_transform.origin.y)
	camera.global_transform = saved_camera
	level._snap_camera()
	await RenderingServer.frame_post_draw
	cab.freeze = was_frozen
	level.process_mode = previous_mode

func _points(definition: CityDefinition) -> PackedVector3Array:
	if not definition.warmup_points.is_empty():
		return definition.warmup_points.duplicate()
	var points := PackedVector3Array()
	# The portrait camera sees roughly 20 x 35 m at the flight plane. Overlapping
	# samples also visit different local light combinations and MultiMesh formats.
	var y := definition.ground_height + 8.0
	while y < definition.max_altitude:
		var x := definition.city_left + 5.0
		while x < definition.city_right:
			points.append(Vector3(x, y, 0))
			x += 18.0
		y += 28.0
	return points
