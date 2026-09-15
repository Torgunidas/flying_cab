extends SceneTree
var checks := 0
var failures := 0
var context: RuntimeContext
var level: Node3D
var world: LivingWorldDirector
var phases: Dictionary = {}

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok:
		print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
		if world:
			for id in context.living.agents:
				if not phases.has(id):
					phases[id] = {}
				phases[id][context.living.agents[id].phase] = true
	await process_frame

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	world = level.living_world
	await frames(90)
	check(world.initialized and world.cars.size() == 50 and world.people.size() == 37, "bounded population: 38 loop vehicles, 12 resident cars, 37 independent pedestrians")
	check(context.world.vehicles.size() == 63 and context.living.agents.size() == 75, "population and twelve test-fleet cars have unique persistent identities")
	check(context.rides.offers.size() == 25, "living pedestrians remain separate from one taxi waiter per platform")
	_clearance()
	await _pedestrian_plane()
	await _pause_population()
	var earlier_laps := {}
	for segment in range(6):
		await frames(3600)
		if segment == 2:
			for id in world.cars:
				earlier_laps[id] = context.living.agents[id].laps
		print("SIMULATED: ", (segment + 1) * 60, " seconds")
	for id in world.cars:
		var item: Dictionary = context.living.agents[id]
		var car: FlightCab = world.cars[id]
		if item.kind == "loop":
			check(item.laps > earlier_laps[id] and car.hull > 0, id + " keeps completing physical loops after six minutes, without destruction")
		elif item.kind == "resident":
			check(item.laps >= 1 and phases[id].has("boarding") and phases[id].has("flying") and phases[id].has("alighting") and phases[id].has("inside"), id + " walks, boards, flies, lands, exits and visits the building")
		print("AGENT: ", id, " phase=", item.phase, " laps=", item.laps, " target=", item.waypoint, " position=", car.global_position, " velocity=", car.linear_velocity, " hull=", car.hull)
	await _takeover()
	await _commuter_takeover()
	await _reload()
	await _legacy_upgrade()
	print("LIVING_WORLD_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)

func _clearance() -> void:
	var query := PhysicsShapeQueryParameters3D.new()
	query.collision_mask = WorldLayers.GEOMETRY
	for route in world.profile.routes:
		var clear := true
		for model_id in route.models:
			var model := context.vehicle_catalog.find_model(StringName(model_id))
			var shape := BoxShape3D.new()
			shape.size = model.collision_size + Vector3(0.3, 0.3, 0.1)
			query.shape = shape
			for i in range(route.points.size()):
				query.transform = Transform3D(Basis.IDENTITY, route.points[i] + model.collision_offset)
				query.motion = route.points[(i + 1) % route.points.size()] - route.points[i]
				var result := world.network.space.cast_motion(query)
				clear = clear and world.network.space.intersect_shape(query, 1).is_empty() and result[0] >= 0.9999
		check(clear, route.route_id + " has real clearance for every assigned model")

func _pedestrian_plane() -> void:
	var foot: OnFootInteraction = level.on_foot
	check(foot.try_exit(level.cab), "Ari exits next to the resident's parked car")
	await frames(20)
	var ari := foot.actor
	var stop: TaxiStop = world.network.stops.depot
	check(is_equal_approx(ari.global_position.z, stop.waiting_point().z), "Ari and NPCs use the same pedestrian plane")
	ari.place(Vector3(-25.5, 54.60, 0))
	await frames(15)
	context.player.dispatch(Vector2.RIGHT)
	await frames(175)
	context.player.dispatch(Vector2.ZERO)
	check(ari.global_position.x > -19 and ari.is_on_floor(), "Ari walks across the cab and resident car without jumping or getting blocked")
	check(ari.collision_mask == WorldLayers.GEOMETRY and (level.cab.collision_mask & WorldLayers.PEOPLE) == 0 and (level.cab.collision_mask & WorldLayers.VEHICLES) != 0, "people do not collide with cars or people; vehicles still collide with vehicles")
	foot.recover()
	await frames(15)
	check(foot.try_enter(level.cab), "normal cab entry still works from the pedestrian strip")

func _takeover() -> void:
	var car: FlightCab = world.cars["resident/hotel_owner"]
	var foot: OnFootInteraction = level.on_foot
	# Move only the player fixture, leaving the NPC car and its actual state intact.
	context.player.take_control(foot.actor, &"on_foot")
	foot.actor.set_active(true)
	var points := foot.exit_points(car)
	check(not points.is_empty(), "parked NPC car has a reachable entry on the pedestrian strip")
	if points.is_empty():
		return
	foot.actor.place(points[0])
	await frames(20)
	var identity := car.get_instance_id()
	var fuel := car.fuel
	var hull := car.hull
	check(foot.try_enter(car), "Q interaction takes a parked NPC-owned car")
	check(context.player.focus == car and car.get_instance_id() == identity and car.fuel == fuel and car.hull == hull and car.state.owner_id == &"resident/hotel_owner", "takeover preserves the exact car, owner, tank and condition")
	check(context.living.agents["resident/hotel_owner"].claimed and car.definition.fuel_enabled, "resident loses the plan and normal player fuel consumption applies")
	var snapshot := context.snapshot()
	var restored := RuntimeContext.new()
	var json: Dictionary = JSON.parse_string(JSON.stringify(snapshot))
	check(restored.restore(json) and restored.player_state.vehicle_id == car.entity_id and restored.living.agents["resident/hotel_owner"].claimed, "JSON save preserves population, stolen car and player control")
	var invalid := snapshot.duplicate(true)
	invalid.living_world.agents["resident/hotel_owner"].person[0] = "bad"
	var rejected := RuntimeContext.new()
	check(not rejected.restore(invalid) and rejected.vehicles.is_empty(), "invalid living-world save fails before mutating the session")
	rejected.free()
	restored.free()
	for field in ["route", "vehicle"]:
		invalid = snapshot.duplicate(true)
		invalid.living_world.agents["traffic/outer_clockwise/00"][field] = "unknown"
		rejected = RuntimeContext.new()
		check(not rejected.restore(invalid) and rejected.vehicles.is_empty(), "invalid population " + field + " fails transactionally")
		rejected.free()

func _pause_population() -> void:
	var car: FlightCab = world.cars["traffic/outer_clockwise/00"]
	context.player.suspend(&"test_menu")
	context.player.suspend(&"test_focus")
	var plans := context.living.snapshot()
	# Physics-server body freezing takes effect at the next synchronization.
	await frames(2)
	var position := car.global_position
	await frames(60)
	context.player.resume(&"test_menu")
	await frames(60)
	check(car.freeze and car.global_position.is_equal_approx(position) and context.living.snapshot() == plans, "nested control locks pause NPC bodies and pedestrian schedules")
	context.player.resume(&"test_focus")
	await frames(60)
	check(not car.freeze and car.global_position.distance_to(position) > 0.5, "population resumes only after the final control lock is released")

func _commuter_takeover() -> void:
	var id := "resident/velvet_commuter"
	var car: FlightCab = world.cars[id]
	var foot: OnFootInteraction = level.on_foot
	var ticks := 0
	while not world.can_take_vehicle(car) and ticks < 12000:
		await frames(1)
		ticks += 1
	check(world.can_take_vehicle(car), "a commuting resident leaves its real vehicle available after landing")
	if not world.can_take_vehicle(car):
		return
	context.player.take_control(foot.actor, &"on_foot")
	foot.actor.set_active(true)
	var points := foot.exit_points(car)
	if points.is_empty():
		check(false, "commuter's berth is reachable")
		return
	foot.actor.place(points[0])
	await frames(15)
	check(foot.try_enter(car), "player takes a car used in an actual resident journey")
	var before := car.global_position
	await frames(1800)
	check(context.player.focus == car and not world.drivers.has(id) and context.living.agents[id].claimed and car.global_position.distance_to(before) < 0.1, "resident abandons its departure plan and cannot take control back")
	check(car.definition.fuel_enabled and car.state.owner_id == StringName(id), "stolen commuter keeps its owner and restores ordinary fuel rules")

func _reload() -> void:
	var snapshot := context.snapshot()
	var selected := context.player_state.vehicle_id
	var at: Vector3 = context.player.focus.global_position
	world = null
	level.queue_free()
	context.queue_free()
	await process_frame
	context = RuntimeContext.new()
	check(context.restore(JSON.parse_string(JSON.stringify(snapshot))), "full population snapshot is accepted in a fresh session")
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	world = level.living_world
	check(world.cars.size() == 50 and context.world.vehicles.size() == 63 and context.living.agents.size() == 75, "map reconstruction does not duplicate residents or vehicles")
	check(context.player.focus is FlightCab and context.player.focus.entity_id == selected and context.player.focus.global_position.distance_to(at) < 0.1, "map reconstruction returns control to the stolen car at its saved position")
	await frames(60)
	var progress := {}
	for id: String in world.drivers:
		progress[id] = context.living.agents[id].laps
	await frames(18000)
	var moving := true
	for id: String in progress:
		moving = moving and context.living.agents[id].laps > progress[id]
	check(moving, "every remaining driver continues its loop or resident journey after reloading")
	check(context.player.focus.entity_id == selected and not world.drivers.has(String(selected)), "theft remains effective after reload and another five minutes of NPC activity")
	var car: FlightCab = context.player.focus
	check(level.taxi.request_recovery(true), "recovery finds a vacant berth for the stolen model")
	await frames(180)
	check(car.grounded and car.sleeping and car.support_body == world.network.stops.depot.get_parent(), "recovery parks beside existing depot cars instead of overlapping them")

func _legacy_upgrade() -> void:
	var snapshot := context.snapshot()
	snapshot.schema_version = 3
	snapshot.erase("living_world")
	snapshot.vehicles = snapshot.vehicles.filter(func(item: Dictionary): return item.id == "ari_cab")
	snapshot.vehicles[0].position = [-19.2, 54.4, 0]
	snapshot.vehicles[0].velocity = [0, 0, 0]
	snapshot.vehicles[0].driver = ""
	snapshot.player.mode = "on_foot"
	snapshot.player.vehicle = "ari_cab"
	snapshot.player.position = [-22, 54.60, 0]
	snapshot.player.velocity = [0, 0, 0]
	snapshot.player.exit_position = [-22, 54.60, 0]
	world = null
	level.queue_free()
	context.queue_free()
	await process_frame
	context = RuntimeContext.new()
	check(context.restore(snapshot), "schema 3 save migrates without requiring a new game")
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	world = level.living_world
	await frames(90)
	check(world.cars.size() == 50 and context.player.focus == level.on_foot.actor and is_equal_approx(level.on_foot.actor.position.z, WorldLayers.PEDESTRIAN_Z), "old pedestrian position migrates onto the shared strip and population seeds once")
	var resident: FlightCab = world.cars["resident/velvet_commuter"]
	check(resident.grounded and resident.global_position.x < -23 and level.cab.hull == level.cab.definition.max_hull, "new NPC uses the other berth when an old save already parks in its usual space")
