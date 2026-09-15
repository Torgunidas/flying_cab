extends SceneTree

var scene: Node3D
var cab: FlightCab
var checks := 0
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(condition: bool, label: String) -> void:
	checks += 1
	if condition:
		print("PASS: " + label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func place(position: Vector3) -> void:
	cab.freeze = false
	cab.spawn_transform.origin = position
	cab.reset_flight()
	await frames(4)

func _run() -> void:
	scene = load("res://scenes/flight_lab.tscn").instantiate()
	cab = scene.get_node("Cab")
	cab.definition = cab.definition.duplicate()
	root.add_child(scene)
	current_scene = scene
	scene.set_physics_process(false)
	await frames(90)
	check(is_equal_approx(cab.world_definition.city_right - cab.world_definition.city_left, 200.0) and cab.world_definition.max_altitude == 320.0 and cab.world_definition.ground_height == -48.0, "city has 200 m of flight space with the existing ceiling and low-city floor")
	var pads := get_nodes_in_group("refuel_pad")
	var lanes := get_nodes_in_group("highway")
	check(pads.size() == 25 and lanes.size() == 6, "four districts have 24 fuel terraces, a start depot and six express lanes")
	var lane_left := INF
	var lane_right := -INF
	for lane in lanes:
		lane_left = minf(lane_left, lane.global_position.x - lane.size.x * 0.5)
		lane_right = maxf(lane_right, lane.global_position.x + lane.size.x * 0.5)
	check(lane_left - cab.world_definition.city_left >= 28 and cab.world_definition.city_right - lane_right >= 28, "both highway edges have at least 28 metres of maneuvering space before forced return")
	var signs := get_nodes_in_group("perimeter_warning")
	var sign_placement_ok := signs.size() == 30
	var sign_content_ok := sign_placement_ok
	var signs_clear := sign_placement_ok
	for sign: Node3D in signs:
		var edge := cab.world_definition.city_left if sign.global_position.x < 0 else cab.world_definition.city_right
		sign_placement_ok = sign_placement_ok and is_equal_approx(absf(sign.global_position.x - edge), 20.0)
		sign_content_ok = sign_content_ok and sign.get_node("Perimeter").text == "CITY PERIMETER" and sign.get_node("Instruction").text == "TURN BACK"
		signs_clear = signs_clear and sign.global_position.z <= -2.5 and sign.find_children("*", "CollisionObject3D", true, false).is_empty()
	check(sign_placement_ok and sign_content_ok, "thirty municipal signs warn 20 m before both boundaries throughout the city height")
	check(signs_clear, "suspended signs are behind the flight plane and cannot block traffic")
	var space := scene.get_world_3d().direct_space_state
	var attachments_ok := true
	var foundations_ok := true
	for pad: StaticBody3D in pads:
		var building: Node3D = pad.get_node(pad.get_meta("building"))
		var shape: CollisionShape3D = pad.get_node("Collision")
		var rear: float = shape.global_position.z - (shape.shape as BoxShape3D).size.z * 0.5
		var ray := PhysicsRayQueryParameters3D.create(pad.global_position, pad.global_position + Vector3(0,0,-25), 1, [pad.get_rid(), cab.get_rid()])
		var hit := space.intersect_ray(ray)
		attachments_ok = attachments_ok and not hit.is_empty()
		if not hit.is_empty():
			attachments_ok = attachments_ok and building.is_ancestor_of(hit.collider) and absf(hit.position.z - rear) < 0.1
		var has_foundation := false
		for body in building.find_children("*", "StaticBody3D", true, false):
			var support: CollisionShape3D = body.get_node("Collision")
			if support.shape is BoxShape3D:
				var bottom: float = support.global_position.y - support.shape.size.y * 0.5
				has_foundation = has_foundation or (absf(bottom - cab.world_definition.ground_height) < 0.01 and support.global_position.y + support.shape.size.y * 0.5 > pad.position.y)
		foundations_ok = foundations_ok and has_foundation
	check(attachments_ok, "every terrace physically meets its named building facade")
	check(foundations_ok, "every supporting building has a continuous solid foundation down to ground")
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = cab.get_node("Collision").shape
	query.exclude = [cab.get_rid()]
	for lane in lanes:
		var extent: Vector2 = lane.size
		var vertical: bool = extent.y > extent.x
		var length: float = maxf(extent.x, extent.y)
		var across: float = minf(extent.x, extent.y) * 0.5 - (1.1 if vertical else 0.35)
		var clear := true
		for along in range(0, int(length)+1, 4):
			for offset in [-across, 0.0, across]:
				var local := Vector3(offset, along-length*0.5, 0) if vertical else Vector3(along-length*0.5, offset, 0)
				query.transform = Transform3D(Basis.IDENTITY, lane.to_global(local))
				clear = clear and space.intersect_shape(query, 1).is_empty()
		check(clear, "%s: full cab clears the centre and both edges of the visible lane" % lane.name)
	# Land on actual colliders; inspect actual fuel and sleeping body, not HUD labels.
	for pad: StaticBody3D in pads:
		await place(pad.global_position + Vector3(0,4,0))
		cab.fuel = 25.0
		await frames(150)
		check(cab.grounded and cab.sleeping and cab.fuel > 25.0 and cab.fuel_burn_rate == 0.0 and absf(cab.position.y - pad.global_position.y - 0.75) < 0.08, "%s: reachable landing, stable rest and working refuel" % pad.name)
	# Assist timing and overlaps are measured against actual scene lane nodes.
	cab.freeze = true
	cab.fuel = 100.0
	cab.highway_speed = 1.0
	cab.highway_fuel = 1.0
	cab._update_highway(Vector3(0.5, 160, 0), 0.25)
	check(is_equal_approx(cab.highway_speed,1.25) and is_equal_approx(cab.highway_fuel,0.75), "entry ramps halfway after 0.25 seconds")
	cab._update_highway(Vector3(0.5, 160, 0), 0.25)
	check(cab.highway_speed == 1.5 and cab.highway_fuel == 0.5, "crossing lanes give one 1.5x speed / 0.5x fuel benefit, never stacked")
	cab._update_highway(Vector3(12, 200, 0), 0.4)
	check(is_equal_approx(cab.highway_speed,1.25), "exit releases the bonus progressively over 0.8 seconds")
	cab._update_highway(Vector3(12, 200, 0), 0.4)
	check(cab.highway_speed == 1.0 and cab.highway_fuel == 1.0, "outside a highway the original flight limits and costs return")
	cab._update_highway(Vector3(0.5, 160, 0), 0.5)
	cab.fuel = 0.0
	cab._update_highway(Vector3(0.5, 160, 0), 0.01)
	check(cab.highway_speed == 1.0 and cab.highway_fuel == 1.0, "empty fuel immediately removes highway assist")
	cab.fuel = 100.0
	cab._update_highway(Vector3(0.5, 160, 3), 0.5)
	check(cab.highway_speed == 1.0, "visual background depth is outside the playable highway volume")
	await place(Vector3(0.5, 30, 0))
	cab.command = Vector2(0, 1)
	await frames(180)
	print("HIGHWAY FLIGHT: pos=",cab.position," velocity=",cab.linear_velocity," command=",cab.command," applied=",cab.applied_command," fuel=",cab.fuel," speed=",cab.highway_speed," cost=",cab.highway_fuel," resetting=",cab.resetting," freeze=",cab.freeze," sleeping=",cab.sleeping)
	check(absf(cab.linear_velocity.y - 17.25) < 0.01 and cab.position.y > 60.0, "sustained physical flight reaches the highway climb cap without changing thrust")
	check(cab.fuel > 97.0 and cab.fuel < 98.0, "the physical highway climb uses roughly half the ordinary fuel")
	var fuel_before := cab.fuel
	cab.command = Vector2.ZERO
	var release_ticks := ceili(cab.definition.thrust_release_seconds * Engine.physics_ticks_per_second) + 2
	await frames(release_ticks)
	var fuel_after_release := cab.fuel
	await frames(60 - release_ticks)
	print("HIGHWAY RELEASE: velocity=",cab.linear_velocity," fuel=",cab.fuel," before=",fuel_before," applied=",cab.applied_command)
	check(cab.fuel == fuel_after_release and fuel_after_release >= fuel_before - cab.definition.fuel_vertical_rate * cab.definition.thrust_release_seconds and cab.linear_velocity.y < 0.0, "highway engine fade is fuel-accounted, then coasting and falling have no passive burn")
	await frames(720)
	check(cab.sleeping and cab.highway_speed == 1.0 and cab.highway_fuel == 1.0, "landing after highway travel clears the remaining assist before sleeping")
	var gravity := cab.definition.gravity
	cab.definition.gravity = 0.0
	await place(Vector3(-50, 160, 0))
	cab.command = Vector2.RIGHT
	await frames(180)
	check(absf(cab.linear_velocity.x-15.75)<0.01 and absf(cab.linear_velocity.y)<0.01, "physical cross-city cruise reaches 1.5x horizontal speed")
	check(cab.fuel > 98.7 and cab.fuel < 99.1, "horizontal express cruise also receives the half-fuel benefit")
	cab.definition.gravity = gravity
	await place(scene.get_node("City/LandingPads/Pad0").global_position + Vector3(0, 0.8, 0))
	check(cab.highway_speed == 1.0 and cab.applied_command == Vector2.ZERO, "reset clears highway assist and engine command")
	print("CITY_TESTS: %d/%d passed" % [checks-failures,checks])
	quit(0 if failures == 0 else 1)
