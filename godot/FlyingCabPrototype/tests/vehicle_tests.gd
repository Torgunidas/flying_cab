extends SceneTree
var checks := 0
var failures := 0
var serial := 0
const CAB = preload("res://scenes/cab.tscn")
const BASIC = preload("res://resources/vehicles/basic_cab.tres")
const SHUTTLE = preload("res://resources/vehicles/heavy_shuttle.tres")

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if not ok:
		failures += 1
		push_error("FAIL: " + label)
	else:
		print("PASS: ", label)

func frames(count: int = 4) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func spawn(host: Node, model: VehicleDefinition, pos: Vector3, velocity := Vector3.ZERO) -> FlightCab:
	var cab: FlightCab = CAB.instantiate()
	serial += 1
	cab.entity_id = StringName("test_car_%d" % serial)
	cab.definition = model.duplicate()
	cab.definition.airspace_enabled = false
	cab.definition.highway_enabled = false
	cab.position = pos
	cab.linear_velocity = velocity
	host.add_child(cab)
	return cab

func body(host: Node, pos: Vector3, size: Vector3) -> StaticBody3D:
	var result := StaticBody3D.new()
	var collider := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = size
	collider.shape = shape
	result.add_child(collider)
	result.position = pos
	host.add_child(result)
	return result

func _run() -> void:
	var catalog: VehicleCatalog = load("res://resources/vehicle_catalog.tres")
	check(catalog.validation_errors().is_empty() and catalog.find_model(&"basic_cab") != null and catalog.find_model(&"heavy_shuttle") != null, "catalog retains valid original and legacy models with unique IDs")
	var sample := load("res://scenes/vehicles/heavy_shuttle.tscn").instantiate() as FlightCab
	check(sample.definition.model_id == &"heavy_shuttle" and sample.entity_id == &"shuttle_01", "inherited sample scene selects its model without copying the vehicle script")
	sample.free()
	var fuel_state := VehicleState.new()
	fuel_state.fuel = SHUTTLE.starting_fuel
	var fuel_system := VehicleFuel.new()
	fuel_system.consume(fuel_state, SHUTTLE, Vector2(1, 1), 0, 1, false, 1)
	check(fuel_state.fuel == 147 and fuel_system.burn_rate == 3, "another model burns its own configured rates on both thrust axes")
	fuel_system.refuel(fuel_state, SHUTTLE, true, 5)
	check(fuel_state.fuel == 180, "refuelling uses the selected model's tank capacity")
	var emergency := preload("res://scripts/airspace.gd").new()
	var emergency_world: CityDefinition = load("res://resources/city_02.tres")
	var return_pose := Vector3(emergency_world.city_right + 1, 30, 0)
	var return_velocity := Vector3.ZERO
	for i in range(1200):
		var input: Vector2 = emergency.command_for(return_pose, return_velocity, Vector2.ZERO, SHUTTLE, emergency_world)
		return_velocity = FlightModel.step_velocity(return_velocity, input, 1.0 / 60, SHUTTLE)
		return_pose += return_velocity / 60
		if i > 0 and not emergency.returning:
			break
	check(not emergency.returning and return_pose.x < emergency_world.city_right, "autopilot computes a safe return using another model's force-to-mass ratio")
	var invalid: VehicleDefinition = BASIC.duplicate()
	invalid.mass_kg = 0
	invalid.max_hull = NAN
	check(invalid.validation_errors().size() == 2, "invalid mass and non-finite hull are rejected")
	var fixture := Node3D.new()
	root.add_child(fixture)
	var model: VehicleDefinition = BASIC.duplicate()
	model.gravity = 0
	model.horizontal_coast_damping = 0
	model.linear_damping = 0
	var heavy_model: VehicleDefinition = model.duplicate()
	heavy_model.mass_kg *= 2
	var light := spawn(fixture, model, Vector3(-20, 30, 0))
	var heavy := spawn(fixture, heavy_model, Vector3(20, 30, 0))
	light.command = Vector2(1, 1)
	heavy.command = Vector2(1, 1)
	await frames(12)
	check(is_equal_approx(light.mass, 100) and is_equal_approx(heavy.mass, 200) and absf(light.linear_velocity.x - heavy.linear_velocity.x * 2) < 0.01 and absf(light.linear_velocity.y - heavy.linear_velocity.y * 2) < 0.01, "real physics: doubled mass halves acceleration under identical thrust on both axes")
	light.command = Vector2.ZERO
	heavy.command = Vector2.ZERO
	# Isolate external impulses only after the real engine release has finished.
	await frames(ceili(model.thrust_release_seconds * Engine.physics_ticks_per_second) + 2)
	light.linear_velocity = Vector3.ZERO
	heavy.linear_velocity = Vector3.ZERO
	await frames()
	light.apply_central_impulse(Vector3(100, 0, 0))
	heavy.apply_central_impulse(Vector3(100, 0, 0))
	await frames()
	check(absf(light.linear_velocity.x - 1) < 0.01 and absf(heavy.linear_velocity.x - 0.5) < 0.01, "real physics: identical external impulses respect each body's configured mass")
	light.definition.fuel_vertical_rate = 9
	check(BASIC.fuel_vertical_rate == 1.4 and heavy.definition.fuel_vertical_rate == 1.4, "runtime tuning edits do not leak into shared resources or another car")
	light.assign_driver(&"ari")
	check(light.board_passenger(&"one") and light.board_passenger(&"two") and light.board_passenger(&"three") and not light.board_passenger(&"four"), "boarding enforces passenger capacity excluding driver")
	check(not light.board_passenger(&"ari") and not light.board_passenger(&"one") and not light.board_passenger(&""), "driver, duplicate and empty passenger IDs are rejected")
	check(light.leave_passenger(&"two") and light.board_passenger(&"four") and not light.leave_passenger(&"absent"), "disembarking frees exactly one seat")
	light.assign_driver(&"one")
	check(not light.state.passenger_ids.has("one") and light.state.passenger_ids.size() == 2, "moving a passenger to the driver seat never counts that person twice")
	var disabled_events := [0]
	light.vitals.disabled.connect(func(): disabled_events[0] += 1)
	check(light.apply_damage(25) == 25 and is_equal_approx(light.hull, 75), "generic damage consumes actual HP")
	check(light.apply_damage(-10) == 0 and light.apply_damage(INF) == 0 and light.hull == 75, "invalid damage cannot heal or corrupt hull")
	light.apply_damage(1000)
	light.apply_damage(1000)
	check(light.disabled and disabled_events[0] == 1 and not light.board_passenger(&"five"), "zero hull disables vehicle once and blocks boarding")
	var fuel_before := light.fuel
	light.command = Vector2(1, 1)
	await frames(60)
	check(light.applied_command == Vector2.ZERO and light.fuel == fuel_before, "disabled vehicle applies no thrust and burns no fuel despite held input")
	var vitals := VehicleVitals.new()
	var state := VehicleState.new()
	check(vitals.impact(state, BASIC, 7) == 0 and state.condition == 1, "gentle impact threshold protects ordinary landings")
	check(is_equal_approx(vitals.impact(state, BASIC, 12), 17.5) and vitals.impact(state, BASIC, 12) == 0, "quadratic collision damage applies once within its cooldown")
	vitals.advance(0.2)
	check(vitals.impact(state, BASIC, 12) > 0, "a later independent impact can damage the car again")
	var durable: VehicleDefinition = BASIC.duplicate()
	durable.max_hull = 200
	var durable_state := VehicleState.new()
	vitals.reset()
	check(is_equal_approx(vitals.impact(durable_state, durable, 12), 17.5) and durable_state.condition > 0.9, "more hull increases durability without scaling up incoming damage")
	fixture.queue_free()
	await frames()
	fixture = Node3D.new()
	root.add_child(fixture)
	body(fixture, Vector3(0, -1, 0), Vector3(150, 2, 10))
	var gentle := spawn(fixture, BASIC, Vector3(-15, 1, 0))
	var falling := spawn(fixture, BASIC, Vector3(15, 10, 0))
	await frames(240)
	print("LANDING HULL gentle=", gentle.hull, " hard=", falling.hull)
	check(gentle.sleeping and gentle.hull == 100, "real contacts: soft landing settles without hull loss")
	check(falling.sleeping and falling.hull > 30 and falling.hull < 95, "real contacts: hard landing damages hull once through normal velocity change")
	var landed_hull := falling.hull
	await frames(300)
	check(falling.hull == landed_hull, "resting contact and gravity do not repeatedly damage a parked car")
	gentle.queue_free()
	falling.queue_free()
	await frames()
	var drops: Array[FlightCab] = []
	for i in range(21):
		drops.append(spawn(fixture, BASIC, Vector3(-60 + i * 4, 10 + i * 0.15, 0)))
	await frames(300)
	var missed: Array[int] = []
	for i in range(drops.size()):
		if drops[i].hull >= 99.9:
			missed.append(i)
		drops[i].queue_free()
	check(missed.is_empty(), "CCD landings from varied sub-frame approach distances all cause damage: missed=" + str(missed))
	await frames()
	body(fixture, Vector3(0, 50, 0), Vector3(2, 50, 10))
	var wall_hit := spawn(fixture, model, Vector3(-5, 50, 0), Vector3(10, 0, 0))
	var slide := spawn(fixture, model, Vector3(2.1, 55, 0), Vector3(0, -10, 0))
	await frames(50)
	check(wall_hit.hull < 100 and slide.hull == 100, "real contacts: wall-normal impact damages, parallel wall travel does not")
	fixture.queue_free()
	await frames()
	fixture = Node3D.new()
	root.add_child(fixture)
	light = spawn(fixture, model, Vector3(-3, 30, 0), Vector3(10, 0, 0))
	heavy = spawn(fixture, heavy_model, Vector3(3, 30, 0), Vector3(-10, 0, 0))
	await frames(50)
	print("HEAD-ON HULL light=", light.hull, " heavy=", heavy.hull)
	check(light.hull < heavy.hull and light.hull < 100, "real car-to-car collision uses mass-dependent solver response")
	fixture.queue_free()
	await frames()
	await _workshops_and_saves()
	print("VEHICLE_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)

func _workshops_and_saves() -> void:
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	root.add_child(level)
	var context: RuntimeContext = level.context
	var cab: FlightCab = level.cab
	var controls: FlightControls = level.controls
	await frames(120)
	var station: RepairStation = level.get_node("City/LandingPads/Pad0/Workshop")
	check(context.world.workshops.size() == 2 and station.can_service(cab), "both workshops survive city compilation and the depot recognizes a parked cab")
	controls._update_command()
	Input.action_press("flight_up")
	await frames(120)
	Input.action_release("flight_up")
	await frames(300)
	print("DEPOT DROP HULL ", cab.hull)
	check(cab.sleeping and cab.hull < 95, "real city: unbraked descent onto the depot damages the car")
	level._reset()
	await frames(120)
	cab.apply_damage(50)
	await frames(30)
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/workshop-damaged.png")
	check(cab.hull == 50 and context.campaign.credits == 250, "workshop never repairs or charges without the player's request")
	controls._update_command()
	Input.action_press("vehicle_repair")
	await frames(60)
	Input.action_release("vehicle_repair")
	await frames()
	check(cab.hull > 65 and cab.hull < 75 and is_equal_approx(context.campaign.credits, 300 - cab.hull), "holding E repairs about 20 HP per second and charges the exact restored amount")
	var touch := InputEventScreenTouch.new()
	touch.index = 7
	touch.pressed = true
	touch.position = controls._repair_rect.get_center()
	controls._input(touch)
	await frames(150)
	touch.pressed = false
	controls._input(touch)
	await frames()
	check(is_equal_approx(cab.hull, 100) and is_equal_approx(context.campaign.credits, 200) and not controls.repair_held, "touch hold completes repair, release stops it and full hull cannot be overcharged")
	cab.apply_damage(10)
	context.campaign.credits = 3.5
	Input.action_press("vehicle_repair")
	await frames(60)
	Input.action_release("vehicle_repair")
	check(is_equal_approx(cab.hull, 93.5) and is_zero_approx(context.campaign.credits), "last credits buy a partial repair without negative balance")
	context.campaign.credits = 250
	context.player.suspend(&"dialogue")
	var before := cab.hull
	level.workshop.advance(context, true, 0.1)
	check(cab.hull == before and context.campaign.credits == 250, "dialogue/focus locks reject repair even if an old UI request remains held")
	context.player.resume(&"dialogue")
	controls.clear_controls()
	controls._update_command()
	cab.apply_damage(1000)
	Input.action_press("vehicle_repair")
	await frames(30)
	Input.action_release("vehicle_repair")
	check(cab.hull > 5 and not cab.disabled, "a disabled car at a workshop can be repaired and regain propulsion")
	controls.clear_controls()
	controls._update_command()
	Input.action_press("flight_up")
	await frames(30)
	check(cab.applied_command.y > 0 and not station.can_service(cab), "repaired car lifts off and loses workshop eligibility")
	Input.action_release("flight_up")
	var shop2: RepairStation = level.get_node("City/LandingPads/Foundry03/Workshop")
	cab.spawn_transform.origin = shop2.get_parent().global_position + Vector3(0, 0.8, 0)
	level._reset()
	await frames(120)
	check(shop2.can_service(cab) and level.workshop.station == shop2, "Foundry body shop also recognizes landing on its actual platform")
	cab.apply_damage(20)
	var wallet := context.campaign.credits
	level.workshop.advance(context, true, 0.1)
	check(cab.hull > 80 and context.campaign.credits < wallet, "Foundry body shop performs paid repair")
	var shuttle := spawn(level, SHUTTLE, Vector3(0, 40, 0))
	shuttle.freeze = true
	await frames()
	check(shuttle.fuel == 150 and shuttle.mass == 180 and shuttle.hull == 220 and shuttle.definition.max_passengers == 6, "second model initializes its own tank, mass, hull and seating")
	shuttle.apply_damage(33)
	shuttle.board_passenger(&"traveler")
	shuttle.fuel = 71
	context.player.release_control()
	var saved := context.snapshot()
	var restored := RuntimeContext.new()
	check(restored.restore(saved) and restored.vehicles[shuttle.entity_id].model_id == &"heavy_shuttle" and is_equal_approx(restored.vehicles[shuttle.entity_id].condition * 220, 187), "save round trip retains model ID and its damaged hull")
	var replacement := spawn(level, BASIC, Vector3(0, 50, 0))
	replacement.freeze = true
	replacement.state.entity_id = shuttle.entity_id
	restored.bind_vehicle(replacement)
	check(replacement.definition.model_id == &"heavy_shuttle" and replacement.mass == 180 and replacement.fuel == 71 and replacement.state.passenger_ids.has("traveler"), "restoring an actor resolves its saved model before binding fuel, hull and occupants")
	var malformed := saved.duplicate(true)
	malformed.vehicles[0].model = "unknown_model"
	check(not restored.restore(malformed), "unknown saved vehicle model is rejected atomically")
	malformed = saved.duplicate(true)
	malformed.vehicles[0].passengers = ["a", "b", "c", "d"]
	check(not restored.restore(malformed), "save cannot overbook a model's passenger seats")
	var legacy := cab.state.snapshot()
	legacy.erase("model")
	check(VehicleState.from_snapshot(legacy).model_id == &"basic_cab", "older single-cab snapshots migrate to the basic model")
	shop2.get_parent().remove_child(shop2)
	check(not context.world.workshops.has(shop2), "unloading workshop removes stale registry references")
	shop2.free()
	restored.free()
	level.queue_free()
	await frames()
