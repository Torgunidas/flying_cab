extends SceneTree
var checks := 0
var failures := 0
var context: RuntimeContext
var level: Node3D
var foot: OnFootInteraction
var cab: FlightCab

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok: print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count := 4) -> void:
	for i in count:
		await physics_frame
		context.player.resume(&"application_focus")
	await process_frame

func capture(label: String) -> void:
	if DisplayServer.get_name() != "headless":
		await process_frame
		RenderingServer.force_draw()
		root.get_texture().get_image().save_png("res://build/access-" + label + ".png")

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	level.living_world_enabled = false
	root.add_child(level)
	await level.prepare_gameplay()
	level.set_physics_process(false)
	foot = level.on_foot
	cab = level.cab
	await frames(90)
	_highway_sides()
	# Verify the actual driver cabin, including mirrored long vehicles.
	var body := StaticBody3D.new()
	var collision := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(60, 2, 6)
	collision.shape = shape
	body.add_child(collision)
	body.position = Vector3(0, 349, 0)
	level.add_child(body)
	for model: VehicleDefinition in context.vehicle_catalog.models:
		var car: FlightCab = load("res://scenes/cab.tscn").instantiate()
		car.definition = model.duplicate()
		car.definition.airspace_enabled = false
		car.entity_id = StringName("access/" + String(model.model_id))
		car.position = Vector3(0, 350 + model.collision_size.y * 0.5 - model.collision_offset.y + 0.03, 0)
		level.add_child(car)
		await frames(70)
		for facing in [1.0, -1.0]:
			car.get_node("Visual").scale.x = facing
			context.player.take_control(foot.actor, &"on_foot")
			foot.actor.set_active(true)
			var door := foot.entry_point(car)
			foot.actor.place(Vector3(car.global_position.x - facing * model.collision_size.x * 0.48, door.y, door.z))
			await frames(12)
			check(not foot.try_enter(car), "%s facing %.0f: rear does not allow entry" % [model.model_id, facing])
			foot.actor.place(door)
			await frames(12)
			check(foot.try_enter(car), "%s facing %.0f: cabin allows entry" % [model.model_id, facing])
			check(foot.try_exit(car), "%s facing %.0f: cabin exit remains available" % [model.model_id, facing])
		car.queue_free()
		await frames()
	# Every car keeps its velocity when Ari releases control; he inherits it.
	cab.freeze = true
	cab.global_position = Vector3(0, 365, 0)
	cab.grounded = false
	cab.linear_velocity = Vector3(5, -3, 0)
	context.player.take_control(cab, &"flight")
	foot.actor.set_active(false)
	cab.command = Vector2(1, 1)
	cab.airspace.returning = true
	foot.refresh_target()
	check(level.controls.interaction_label == "WYSIĄDŹ / Q", "exit is offered during flight, thrust and forced return")
	check(foot.try_exit(cab) and foot.actor.velocity == Vector3(5, -3, 0) and cab.linear_velocity == Vector3(5, -3, 0), "airborne exit transfers momentum without stopping the car")
	check(not foot.recover(), "recovery cannot teleport Ari to safety during a fall")
	var saved := context.snapshot()
	var restored := RuntimeContext.new()
	check(restored.restore(saved) and restored.player_state.velocity == Vector3(5, -3, 0), "airborne save preserves velocity and on-foot mode")
	restored.free()
	cab.airspace.returning = false
	# Start each drop above the isolated platform, with actual character physics.
	foot.actor.place(Vector3(0, 351.18, WorldLayers.PEDESTRIAN_Z))
	await frames(75)
	check(foot.actor.is_on_floor() and context.player_state.health == 100, "short drops are harmless")
	foot.actor.place(Vector3(0, 355.58, WorldLayers.PEDESTRIAN_Z))
	await frames(120)
	var injured := context.player_state.health
	check(foot.actor.is_on_floor() and injured > 0 and injured < 100, "medium drop removes health at real ground impact")
	level._snap_camera()
	await capture("injured")
	saved = context.snapshot()
	restored = RuntimeContext.new()
	check(restored.restore(saved) and is_equal_approx(restored.player_state.health, injured), "injuries persist across a saved session")
	var legacy := saved.duplicate(true)
	legacy.player.erase("health")
	check(restored.restore(legacy) and restored.player_state.health == 100, "older saves without health load at full health")
	for invalid in [-1, 101, INF, NAN, "bad", true]:
		var bad := saved.duplicate(true)
		bad.player.health = invalid
		check(not restored.restore(bad) and restored.player_state.health == 100, "invalid health is rejected atomically: " + str(invalid))
	restored.free()
	foot.actor.place(Vector3(0, 350.62, WorldLayers.PEDESTRIAN_Z), Vector3(0, -20, 0))
	await frames(3)
	check(context.player_state.health == 0 and paused and context.player.is_suspended(), "fast downward ejection is lethal even just above the ground")
	check(level.narrative_panel._mode == "game_over" and level.narrative_panel.heading.text == "ARI NIE ŻYJE", "fatal landing opens the death screen")
	check(not foot.recover() and not foot.try_enter(cab), "death cannot be bypassed by recovery or entering a car")
	await capture("death")
	saved = context.snapshot()
	restored = RuntimeContext.new()
	check(restored.restore(saved) and restored.player_state.health == 0 and restored.player.is_suspended(), "loading a dead character does not resurrect or unlock control")
	restored.free()
	level.queue_free()
	await process_frame
	context.queue_free()
	await process_frame
	check(not paused, "unloading death screen releases the scene pause")
	print("VEHICLE_ACCESS_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)

func _highway_sides() -> void:
	var profile: LivingWorldProfile = load("res://resources/living_world/city_02.tres")
	for route in profile.routes:
		if route.route_id not in ["spine", "crosstown", "outer_clockwise", "outer_counterclockwise"]: continue
		for i in route.points.size():
			var start := route.points[i]
			var end := route.points[(i + 1) % route.points.size()]
			var middle := (start + end) * 0.5
			var delta := end - start
			var valid := false
			for lane: Node3D in context.world.lanes:
				if not lane.contains_point(middle): continue
				if lane.size.y > lane.size.x and absf(delta.x) < 0.01:
					valid = (middle.x - lane.global_position.x) * delta.y > 0
				elif lane.size.x > lane.size.y and absf(delta.y) < 0.01:
					valid = (middle.y - lane.global_position.y) * delta.x < 0
			check(valid, "%s segment %d uses the right-hand side of its highway" % [route.route_id, i])
