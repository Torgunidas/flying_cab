extends SceneTree
var checks := 0
var failures := 0
var context: RuntimeContext
var level: Node3D

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	print("PASS: " if ok else "FAIL: ", label)
	if not ok:
		failures += 1

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	current_scene = level
	level.set_physics_process(false)
	await level.prepare_gameplay()
	await frames(120)
	var platform: StaticBody3D = level.get_node("City/FoundryTestPlatform")
	var fleet: Node3D = level.get_node("FoundryTestFleet")
	check(fleet.get_child_count() == context.vehicle_catalog.models.size(), "one playable example of every catalog model is present in Foundry")
	var collider: CollisionShape3D = platform.get_node("Collision")
	check(collider.shape.size.x == 53 and platform.global_position.y < 160 and platform.global_position.x > 0, "53 metre platform is in industrial Foundry below Crosstown")
	var geometry := PhysicsShapeQueryParameters3D.new()
	geometry.collision_mask = WorldLayers.GEOMETRY
	geometry.shape = level.cab.get_node("Collision").shape
	geometry.transform = Transform3D(Basis.IDENTITY, Vector3(11.5, 153, 0))
	geometry.motion = Vector3(-7, 0, 0)
	var space := level.get_world_3d().direct_space_state
	check(space.intersect_shape(geometry, 1).is_empty() and space.cast_motion(geometry)[0] >= 0.9999, "arrival bay has a clear side approach from the central corridor")
	var attachment := PhysicsRayQueryParameters3D.create(platform.global_position, platform.global_position + Vector3(0, 0, -15), WorldLayers.GEOMETRY, [platform.get_rid()])
	var hit := space.intersect_ray(attachment)
	check(not hit.is_empty() and platform.get_node(platform.get_meta("building")).is_ancestor_of(hit.collider), "the deck is attached to an existing grounded industrial building")
	# Land the player fixture at the marked arrival bay, using real contacts.
	level.cab.spawn_transform.origin = Vector3(11.5, 150.45, 0)
	level.cab.reset_flight()
	await frames(120)
	var foot: OnFootInteraction = level.on_foot
	check(level.cab.support_body == platform and foot.try_exit(level.cab), "player can land in the separate arrival bay and step onto the deck")
	await frames(20)
	if OS.get_cmdline_user_args().has("--capture-only"):
		await _capture()
		_finish()
		return
	for model in context.vehicle_catalog.models:
		var car: FlightCab = fleet.get_node(String(model.model_id))
		check(car.grounded and car.sleeping and car.hull == car.definition.max_hull and car.support_body == platform, String(model.model_id) + " rests on the real platform without touching neighbors")
		var points := foot.exit_points(car)
		check(points.size() == 2, String(model.model_id) + " has a usable pedestrian entry on both sides")
		if points.is_empty():
			continue
		foot.actor.place(points[0])
		await frames(12)
		check(foot.try_enter(car), String(model.model_id) + " can be entered using the normal interaction")
		var before := car.global_position
		context.player.dispatch(Vector2(0, 1))
		await frames(24)
		context.player.dispatch(Vector2.ZERO)
		check(not car.grounded and car.global_position.y > before.y + 0.1, String(model.model_id) + " takes off with its configured thrust and mass")
		await frames(180)
		check(car.sleeping and car.support_body == platform and foot.try_exit(car), String(model.model_id) + " returns to its berth and allows exit")
		await frames(12)
	# Walk the complete frontage without hopping between cars.
	foot.actor.place(Vector3(10, 150.62, 1.2))
	await frames(12)
	context.player.dispatch(Vector2.RIGHT)
	await frames(1180)
	context.player.dispatch(Vector2.ZERO)
	check(foot.actor.global_position.x > 58 and foot.actor.is_on_floor(), "Ari can walk past the whole fleet without jumping or getting blocked")
	var car: FlightCab = fleet.get_node("supercar")
	foot.actor.place(foot.exit_points(car)[0])
	await frames(12)
	foot.try_enter(car)
	car.apply_damage(10)
	car.fuel = 17
	var snapshot := context.snapshot()
	var saved_id := car.entity_id
	level.queue_free()
	context.queue_free()
	await process_frame
	context = RuntimeContext.new()
	check(context.restore(JSON.parse_string(JSON.stringify(snapshot))), "fleet test session survives a JSON round trip")
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	check(context.player.focus.entity_id == saved_id and context.player.focus.fuel == 17 and context.player.focus.hull < context.player.focus.definition.max_hull and context.world.vehicles.size() == 63, "reloading restores the selected test car, its condition and all unique vehicles")
	_finish()

func _capture() -> void:
	level.set_process(false)
	root.size = Vector2i(1400, 850)
	for view in [{"name": "overview", "at": Vector3(34.5, 151.5, 0), "distance": 57.0}, {"name": "arrival", "at": Vector3(16, 151, 0), "distance": 20.0}, {"name": "models", "at": Vector3(32, 151, 0), "distance": 20.0}]:
		level._camera_target = view.at
		level._camera_distance = view.distance
		level._position_camera()
		await frames(10)
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/test-yard-" + view.name + ".png")
	check(true, "rendered review views saved from the actual city")

func _finish() -> void:
	print("TEST_YARD_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)
