extends SceneTree
const CAB := preload("res://scenes/cab.tscn")
const CATALOG := preload("res://resources/vehicle_catalog.tres")
const EXPECTED := {"basic_cab": Vector2(1, 1), "heavy_shuttle": Vector2(1, 1), "lorry": Vector2(1.5, 2), "supercar": Vector2(1, 1), "limousine": Vector2(2, 1), "luxury_car": Vector2(1, 1), "police_car": Vector2(1, 1), "poor_car": Vector2(0.5, 1), "tow_car": Vector2(1.5, 1.5), "normal_car_1": Vector2(1, 1), "normal_car_2": Vector2(1, 1), "normal_car_3": Vector2(1, 1)}
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

func frames(count := 3) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func bounds(n: Node3D, transform := Transform3D.IDENTITY) -> AABB:
	var box := AABB()
	if n.name == &"Plume":
		return box
	var pose := transform * n.transform
	if n is MeshInstance3D:
		box = pose * n.get_aabb()
	for child in n.get_children():
		if child is Node3D:
			var next := bounds(child, pose)
			if next.has_volume():
				box = box.merge(next) if box.has_volume() else next
	return box

func spawn(host: Node, definition: VehicleDefinition, id: String, position: Vector3) -> FlightCab:
	var actor: FlightCab = CAB.instantiate()
	actor.entity_id = id
	actor.definition = definition
	actor.position = position
	actor.freeze = true
	host.add_child(actor)
	return actor

func _run() -> void:
	check(CATALOG.models.size() == EXPECTED.size() and CATALOG.validation_errors().is_empty(), "12 valid models: cab, legacy shuttle and exactly ten new variants")
	var fixture := Node3D.new()
	root.add_child(fixture)
	var session := RuntimeContext.new()
	root.add_child(session)
	var actors: Array[FlightCab] = []
	var shape_ids := {}
	for model: VehicleDefinition in CATALOG.models:
		var id := String(model.model_id)
		var car := spawn(fixture, model, "test_" + id, Vector3(actors.size() * 8, 54, 0))
		actors.append(car)
		session.bind_vehicle(car)
		var measured := bounds(car.visual)
		var requested: Vector2 = EXPECTED[id]
		check(Vector2(measured.size.x, measured.size.y).is_equal_approx(Vector2(2.2, 1.14) * requested), id + " measured stationary silhouette " + str(measured.size))
		check(car.visual.scene_file_path == model.visual_scene.resource_path and car.visual == car.get_node("Visual") and car.flight_fx.rig == car.visual.get_node("Thrusters"), id + " visual and effects bind before first frame")
		var collision: CollisionShape3D = car.get_node("Collision")
		check(collision.shape.size.is_equal_approx(model.collision_size) and collision.position.is_equal_approx(model.collision_offset) and not shape_ids.has(collision.shape.get_instance_id()), id + " owns its collider and uses model geometry")
		shape_ids[collision.shape.get_instance_id()] = true
		if id != "basic_cab" and id != "heavy_shuttle":
			var envelope := AABB(model.collision_offset - model.collision_size / 2, model.collision_size).grow(0.001)
			var navigation := AABB(-model.navigation_clearance / 2, model.navigation_clearance).grow(0.001)
			check(envelope.encloses(measured) and navigation.encloses(envelope), id + " collision and navigation enclose stationary body")
			var authored: FlightCab = load("res://scenes/vehicles/" + id + ".tscn").instantiate()
			check(authored.definition.model_id == model.model_id and authored.get_node("Visual").scene_file_path == model.visual_scene.resource_path, id + " saved actor displays its model without running the game")
			authored.free()
		check(is_equal_approx(car.mass, model.mass_kg) and model.thrust_force > model.mass_kg * model.gravity and model.thrust_rise_seconds == 0.18 and model.thrust_release_seconds == 0.12, id + " flight tuning can lift its mass and retains engine response")
		car.flight_fx.show_warmup_effects()
		check(car.flight_fx.wake.visible and car.flight_fx._plumes.all(func(p): return p.visible), id + " all exhaust effects support preparation")
		car.flight_fx.reset_visuals()
		check(not car.flight_fx.wake.visible and car.flight_fx._plumes.all(func(p): return not p.visible), id + " preparation restores hidden exhaust")
		car.get_node("Headlights").update_lights(-30, 0, true)
		check(car.get_node("Headlights").lights_on and is_equal_approx(car.get_node("Headlights/Left").position.x, model.headlight_origin.x), id + " headlights use body origin and smog trigger")
		var damage := car.apply_damage(20)
		check(is_equal_approx(damage, 20 * (1 - model.damage_resistance)) and is_equal_approx(car.hull, model.max_hull - damage), id + " resistance reduces damage independently of HP")
		car.fuel = model.fuel_capacity * 0.6
		car.board_passenger(StringName("passenger_" + id))
		car.capture_state()
	var twin := spawn(fixture, CATALOG.models[0], "test_twin", Vector3(-8, 54, 0))
	session.bind_vehicle(twin)
	check(twin.fuel == 100 and twin.hull == 100 and twin.state.passenger_ids.is_empty() and twin.get_node("Collision").shape != actors[0].get_node("Collision").shape, "same model instances isolate fuel, damage, passengers and collision resources")
	check(session.save_to("res://build/model-roundtrip.json") == OK, "save every model to real JSON file")
	var restored := RuntimeContext.new()
	check(restored.load_from("res://build/model-roundtrip.json"), "load every model from real JSON file")
	for i in range(actors.size()):
		var original := actors[i]
		var replacement := spawn(fixture, CATALOG.models[0], String(original.entity_id), Vector3(0, 80, 0))
		var old_visual: WeakRef = weakref(replacement.visual)
		restored.bind_vehicle(replacement)
		check(replacement.definition.model_id == original.definition.model_id and replacement.visual.scene_file_path == original.visual.scene_file_path and replacement.mass == original.mass and is_equal_approx(replacement.hull, original.hull) and is_equal_approx(replacement.fuel, original.fuel) and replacement.state.passenger_ids == original.state.passenger_ids and replacement.global_position.is_equal_approx(original.global_position), String(original.definition.model_id) + " restores appearance, mass, HP, fuel, passengers and pose")
		check(replacement.flight_fx.rig == replacement.visual.get_node("Thrusters") and (i < 2 or old_visual.get_ref() == null), String(original.definition.model_id) + " replacement releases old body and rebinds effects")
		replacement.queue_free()
	var police: FlightCab = actors[6]
	var lamps: VehicleBeacons = police.visual.get_node("Beacons")
	lamps.elapsed = 0.0
	lamps.update_lamps()
	check(lamps._materials[0].emission_energy_multiplier == 4 and lamps._materials[1].emission_energy_multiplier == 0, "police starts an alternating roof flash")
	lamps.elapsed = 0.24
	lamps.update_lamps()
	check(lamps._materials[0].emission_energy_multiplier == 0 and lamps._materials[1].emission_energy_multiplier == 4, "police alternates to the second lamp")
	lamps.elapsed = 3
	lamps.update_lamps()
	check(lamps._materials.all(func(m): return m.emission_energy_multiplier == 0), "police has a quiet interval between bursts")
	police.flight_fx.show_warmup_effects()
	check(lamps._materials.all(func(m): return m.emission_energy_multiplier == 4), "police preparation renders both lamps")
	police.flight_fx.reset_visuals()
	check(not lamps.warmup and lamps._materials.all(func(m): return m.emission_energy_multiplier == 0), "police preparation returns immediately to normal interval")
	lamps._process(5.0)
	check(lamps.elapsed == 0 and lamps._materials[0].emission_energy_multiplier == 4, "police burst repeats on the configured cycle")
	var truck: FlightCab = actors[8]
	var socket: Marker3D = truck.visual.get_node("CargoMount")
	var cargo_visual: Node3D = CATALOG.models[0].visual_scene.instantiate()
	var cargo_bounds := Transform3D(Basis.IDENTITY, socket.position) * bounds(cargo_visual)
	check(is_equal_approx(-1.65 - cargo_bounds.position.x, 1.1), "cab on tow socket overhangs rear by exactly half its 2.2 m length")
	check(socket.get_child_count() == 0 and not truck.has_method("load_cargo"), "tow socket ships empty with no loading or transport mechanics")
	cargo_visual.free()
	var invalid: VehicleDefinition = CATALOG.models[0].duplicate()
	invalid.damage_resistance = 1
	var old_model: StringName = twin.definition.model_id
	check(not invalid.validation_errors().is_empty() and not twin.apply_model(invalid) and twin.definition.model_id == old_model, "invalid resistance cannot replace a working model")
	invalid.damage_resistance = NAN
	check(not invalid.validation_errors().is_empty(), "non-finite resistance is rejected")
	# Exercise actual integration and contacts for every new body.
	var floor := StaticBody3D.new()
	var collider := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(250, 2, 10)
	collider.shape = shape
	floor.position = Vector3(50, -1, 0)
	floor.add_child(collider)
	fixture.add_child(floor)
	await frames()
	for i in range(actors.size()):
		var car := actors[i]
		car.definition.airspace_enabled = false
		car.definition.highway_enabled = false
		car.global_position = Vector3(i * 8, 1, 0)
		car.linear_velocity = Vector3.ZERO
		car.state.condition = 1
		car.freeze = false
	await frames(160)
	for car in actors:
		check(car.sleeping and car.hull == car.definition.max_hull and car.fuel_burn_rate == 0, String(car.definition.model_id) + " soft landing settles with full HP and zero idle burn")
		car.command = Vector2(1, 1)
	await frames(50)
	for car in actors:
		check(car.global_position.y > 1 and car.linear_velocity.x > 0 and car.fuel_burn_rate > 0, String(car.definition.model_id) + " actual physics lifts, accelerates and burns its own fuel")
	restored.free()
	session.queue_free()
	fixture.queue_free()
	await frames()
	print("VEHICLE_MODELS_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
