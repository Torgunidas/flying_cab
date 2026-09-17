extends SceneTree
var checks := 0
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok:
		print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count := 4) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func box(parent: Node, at: Vector3, size: Vector3) -> StaticBody3D:
	var body := StaticBody3D.new()
	var collider := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = size
	collider.shape = shape
	body.add_child(collider)
	body.position = at
	parent.add_child(body)
	return body

func _run() -> void:
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	var context := RuntimeContext.new()
	root.add_child(context)
	context.rides.enabled = true
	level.living_world_enabled = false # Isolated fixture; full population has its own integration suite.
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	var cab: FlightCab = level.cab
	var foot: OnFootInteraction = level.on_foot
	var ari := foot.actor
	var capsule := ari.get_node("Collision").shape as CapsuleShape3D
	check(is_equal_approx(capsule.height, HumanRig.HEIGHT) and is_equal_approx(capsule.radius, HumanRig.RADIUS) and is_equal_approx(ari.get_node("Visual").position.y, -capsule.height * 0.5), "physical capsule and sole offset match the shared human anatomy")
	var controls: FlightControls = level.controls
	await frames(90)
	check(cab.grounded and not ari.active and not ari.visible, "normal game starts driving with Ari hidden and non-colliding")
	check(foot.parked(cab) and foot.exit_points(cab).size() == 3, "cabin and fallback depot exits have capsule clearance")
	var identity := cab.get_instance_id()
	cab.fuel = 43.0
	cab.state.condition = 0.75
	cab.state.passenger_ids.append("test_passenger")
	var cash := context.campaign.credits
	cab.command = Vector2(0, 1)
	check(foot.try_exit(cab), "held thrust no longer blocks leaving the vehicle")
	await frames(12)
	check(foot.try_enter(cab), "Ari re-enters at the cabin after releasing thrust")
	cab.clear_control_input()
	context.player.suspend(&"dialogue")
	check(not foot.try_exit(cab), "dialogue lock blocks interaction")
	context.player.resume(&"dialogue")
	context.rides.active = {"vehicle": String(cab.entity_id), "phase": "boarding"}
	check(foot.try_exit(cab), "passenger boarding does not trap the driver")
	await frames(12)
	check(foot.try_enter(cab), "driver returns while the passenger boarding record is preserved")
	context.rides.active = {}
	check(foot.try_exit(cab), "Ari exits a parked cab with a passenger still aboard")
	await frames(12)
	check(ari.is_on_floor() and context.player.focus == ari and ari.visible and is_equal_approx(ari.global_position.z, WorldLayers.PEDESTRIAN_Z), "visible Ari stands on the actual depot terrace in the city plane")
	check(cab.get_instance_id() == identity and cab.fuel == 43 and cab.state.condition == 0.75 and cab.state.passenger_ids.has("test_passenger") and cab.get_driver_id() == &"", "exit retains vehicle identity, fuel, damage and passengers, releasing only the driver")
	check(controls._control_mode == &"on_foot" and not controls._repair_available, "HUD switches to on-foot controls and hides vehicle service controls")
	var start := ari.global_position
	context.player.dispatch(Vector2(0, 1))
	await frames(8)
	check(ari.global_position.y > start.y + 0.3 and ari.movement_state == &"jump", "a fresh press performs a real upward jump")
	check(not foot.try_enter(cab), "a jumping character cannot teleport into the car")
	await frames(100)
	check(ari.is_on_floor() and absf(ari.global_position.y - start.y) < 0.06, "holding jump through landing does not trigger another jump")
	context.player.dispatch(Vector2.ZERO)
	context.player.dispatch(Vector2(1, 1))
	await frames(12)
	check(ari.global_position.x > start.x + 0.1 and ari.global_position.y > start.y + 0.3, "airborne movement responds to horizontal steering")
	context.player.suspend(&"dialogue")
	var airborne_y := ari.global_position.y
	await frames(90)
	check(ari.is_on_floor() and ari.global_position.y < airborne_y and not ari._jump_pending, "input lock clears jump while gravity and landing continue")
	context.player.resume(&"dialogue")
	check(foot.recover(), "prototype recovery returns Ari to the exit terrace without towing")
	await frames(12)
	context.player.dispatch(Vector2.RIGHT)
	await frames(10)
	context.player.dispatch(Vector2.ZERO)
	await frames(20)
	check(ari.global_position.x > start.x + 0.15 and absf(ari.velocity.x) < 0.01 and ari.facing == 1, "walking accelerates, brakes and preserves facing after release")
	check(context.campaign.credits == cash and cab.fuel == 43, "walking and prototype recovery do not change money or parked fuel")
	# Capture a real on-foot pose through the full save path and restore the map.
	var snapshot := context.snapshot()
	var restored := RuntimeContext.new()
	check(restored.restore(snapshot) and restored.player_state.mode == "on_foot", "schema 4 restores player mode independently of the car")
	var path := "res://build/on-foot-session-test.json"
	check(context.save_to(path) == OK and restored.load_from(path), "on-foot state survives JSON numeric conversion and disk round trip")
	var broken := snapshot.duplicate(true)
	broken.player.position[0] = "invalid"
	check(not restored.restore(broken) and restored.player_state.mode == "on_foot", "malformed player pose is rejected without changing the restored session")
	for version in [1, 2]:
		var legacy := snapshot.duplicate(true)
		legacy.schema_version = version
		legacy.erase("player")
		var old := RuntimeContext.new()
		check(old.restore(legacy) and old.player_state.mode == "flight", "legacy schema %d still loads into the vehicle" % version)
		old.free()
	root.add_child(restored)
	var restored_level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	restored_level.context = restored
	root.add_child(restored_level)
	restored_level.set_physics_process(false)
	check(restored.player.focus == restored_level.on_foot.actor and restored_level.on_foot.actor.global_position.distance_to(ari.global_position) < 0.01 and restored_level.cab.fuel == 43 and restored_level.cab.get_driver_id() == &"", "loading the city recreates Ari at the saved pose and leaves the same car parked")
	restored_level.queue_free()
	restored.queue_free()
	await frames()
	foot.recover()
	await frames(12)
	check(foot.try_enter(cab) and cab.get_instance_id() == identity and cab.state.passenger_ids.has("test_passenger"), "entering returns control to the original car and keeps its occupants")
	check(not ari.visible and ari.collision_layer == 0 and ari.collision_mask == 0 and not ari.is_physics_processing(), "seated Ari has neither a visible duplicate nor a live collider")
	check(context.snapshot().player.mode == "flight", "save tracks return to driving")
	# A blocked door can use a clear ejection point, but is not an entry hotspot.
	var points := foot.exit_points(cab)
	var wall_door := box(level, points[0], Vector3(0.6, 3, 0.6))
	await frames()
	check(foot.exit_points(cab).size() == 2 and foot.try_exit(cab), "blocked cabin falls back to a clear side")
	await frames(12)
	check(not foot.try_enter(cab), "side fallback does not allow entry through a blocked cabin")
	wall_door.queue_free()
	await frames(12)
	ari.place(foot.entry_point(cab))
	await frames(12)
	check(foot.try_enter(cab), "clearing the cabin restores entry")
	var blockers: Array[Node] = []
	for point in foot.exit_points(cab):
		blockers.append(box(level, point, Vector3(0.6, 3, 0.6)))
	await frames()
	check(not foot.try_exit(cab) and context.player.focus == cab, "fully enclosed vehicle cannot spawn Ari inside solid geometry")
	for blocker in blockers: blocker.queue_free()
	await frames(30)
	# Q and touch each request one transfer; held W does not leak into a jump.
	level.set_physics_process(true)
	Input.action_press("flight_up")
	controls._keys_blocked_until_release = true
	controls._update_command()
	foot.refresh_target()
	var touch := InputEventScreenTouch.new()
	touch.index = 7
	touch.position = controls._interaction_rect.get_center()
	touch.pressed = true
	controls._input(touch)
	await frames(20)
	check(context.player.focus == ari and ari.is_on_floor() and ari._command == Vector2.ZERO, "touch exit consumes one press and blocks a pre-held keyboard jump")
	touch.pressed = false
	controls._input(touch)
	Input.action_release("flight_up")
	await frames(5)
	var q := InputEventKey.new()
	q.physical_keycode = KEY_Q
	q.pressed = true
	controls._input(q)
	await frames(20)
	check(context.player.focus == cab, "Q returns to the nearest reachable entry point")
	q.echo = true
	controls._input(q)
	await frames(20)
	check(context.player.focus == cab, "keyboard repeat does not trigger a second transfer")
	level.set_physics_process(false)
	# Every vehicle definition, including lorry and limousine, uses its own bounds.
	var test_floor := box(level, Vector3(0, 349, 0), Vector3(40, 2, 6))
	for model: VehicleDefinition in context.vehicle_catalog.models:
		var other: FlightCab = load("res://scenes/cab.tscn").instantiate()
		other.definition = model.duplicate()
		other.definition.airspace_enabled = false
		other.entity_id = StringName("entry_test_" + String(model.model_id))
		other.position = Vector3(0, 350 + model.collision_size.y * 0.5 - model.collision_offset.y + 0.03, 0)
		level.add_child(other)
		context.player.take_control(other, &"flight")
		await frames(60)
		var out := foot.try_exit(other)
		await frames(12)
		var back := foot.try_enter(other)
		check(out and back and other.get_driver_id() == &"ari", "real exit and re-entry for " + String(model.model_id))
		context.player.take_control(cab, &"flight")
		ari.set_active(false)
		other.queue_free()
		await frames()
	test_floor.queue_free()
	# Leaving a vehicle no longer requires a landing platform.
	cab.freeze = true
	cab.global_position = Vector3(3.5, 120, 0)
	cab.grounded = false
	check(foot.try_exit(cab), "airborne exit is available without ground below the car")
	level.queue_free()
	context.queue_free()
	await process_frame
	print("ON_FOOT_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
