class_name PlatformCameraFraming
extends RefCounted
## Read-only presentation query: deck geometry and speed determine the close frame.
## It never selects a taxi destination or changes the vehicle's controls.
static func approach_weight(cab: FlightCab, position: Vector3, world: WorldRegistry, tuning: FlightCameraTuning) -> float:
	var speed_weight := 1.0 - smoothstep(tuning.approach_slow_speed, maxf(tuning.approach_slow_speed + 0.01, tuning.approach_fast_speed), cab.linear_velocity.length())
	if speed_weight <= 0.0: return 0.0
	var platforms: Array[Node] = world.pads.duplicate()
	var map := world.map_root()
	if is_instance_valid(map):
		for platform in map.get_tree().get_nodes_in_group("vehicle_test_platform"):
			if map.is_ancestor_of(platform): platforms.append(platform)
	var bottom_offset := 0.0
	var hull := cab.get_node_or_null("Collision") as CollisionShape3D
	if hull and hull.shape is BoxShape3D:
		var hull_bounds: AABB = hull.global_transform * AABB(-hull.shape.size * 0.5, hull.shape.size)
		bottom_offset = cab.global_position.y - hull_bounds.position.y
	var weight := 0.0
	for platform in platforms:
		if not is_instance_valid(platform): continue
		var collision := platform.get_node_or_null("Collision") as CollisionShape3D
		if not collision or collision.disabled or not collision.shape is BoxShape3D: continue
		var bounds: AABB = collision.global_transform * AABB(-collision.shape.size * 0.5, collision.shape.size)
		var height := position.y - bottom_offset - bounds.end.y
		# Flying under a terrace does not count as an approach to its top.
		if height < -0.75 or height > tuning.approach_start_distance: continue
		var horizontal := position.x - clampf(position.x, bounds.position.x, bounds.end.x)
		var depth := position.z - clampf(position.z, bounds.position.z, bounds.end.z)
		var distance := Vector3(horizontal, maxf(0.0, height), depth).length()
		var proximity := 1.0 - smoothstep(tuning.approach_close_distance, maxf(tuning.approach_close_distance + 0.01, tuning.approach_start_distance), distance)
		proximity *= 1.0 - smoothstep(0.05, 0.75, -height)
		weight = maxf(weight, proximity)
	return weight * speed_weight
