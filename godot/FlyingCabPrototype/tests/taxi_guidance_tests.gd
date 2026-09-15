extends SceneTree
var checks := 0
var failures := 0
var context: RuntimeContext
var level: Node3D
var director: TaxiDirector
var cab: FlightCab
var arrow: TaxiGuidanceArrow

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
		await process_frame
		context.player.resume(&"application_focus")

func place(point: Vector3) -> void:
	cab.global_position = point
	cab.reset_physics_interpolation()
	level._snap_camera()
	await frames()

func trip(destination: String, boarded := true) -> void:
	context.rides.cancel(context.vehicles)
	context.rides.offers.clear()
	var offer := context.rides.create_offer("depot" if destination != "depot" else "velvet_neon", destination, 30, 1, 150)
	offer.walk = 1.0
	context.rides.select(offer.id)
	assert(context.rides.begin_boarding(cab.state, cab.definition))
	if boarded:
		assert(context.rides.board(cab.state, cab.definition))
	level.controls.update_vehicle_status(cab, context.campaign.credits, null, false)

func capture(label: String) -> void:
	if DisplayServer.get_name() != "headless":
		await frames(4)
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/guidance-" + label + ".png")

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	context.campaign.credits = 120
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.living_world_enabled = false # Isolated fixture; full population has its own integration suite.
	level.context = context
	root.add_child(level)
	await level.prepare_gameplay()
	level.set_physics_process(false)
	director = level.taxi
	cab = level.cab
	cab.freeze = true
	arrow = level.taxi_hud.guidance
	await frames()
	check(director.guidance_target() == null and not arrow.visible, "waiting offers do not add guidance to free flight")
	trip("velvet_club", false)
	await frames()
	check(director.guidance_target() == null and not arrow.visible, "guidance stays hidden while the passenger is boarding")
	context.rides.board(cab.state, cab.definition)
	await frames()
	check(arrow.visible and director.guidance_target().stop_id == "velvet_club", "a ride within Velvet has guidance immediately after boarding at the depot")
	var city := context.world.definition as CityDefinition
	check(city.district_id_at(Vector3(-22, 54, 0)) == &"velvet", "the depot's local display name does not create a separate gameplay district")
	for stop: TaxiStop in director.network.stops.values():
		trip(stop.stop_id)
		var point := stop.global_position + Vector3(0, 8, 0)
		await place(point)
		var shown := arrow.visible and director.guidance_target() == stop
		point.x = city.district_divider * 2 - point.x
		await place(point)
		check(shown and not arrow.visible and director.guidance_target() == null and String(city.district_id_at(stop.global_position)) == stop.district.to_lower(), stop.stop_id + ": local district enables guidance; crossing to the opposite district hides it")
	trip("aurelia_01")
	await place(Vector3(22, city.upper_city_height - 0.01, 0))
	check(not arrow.visible, "correct side but wrong altitude district does not enable guidance")
	await place(Vector3(22, city.upper_city_height, 0))
	check(arrow.visible, "guidance starts on entry to the target upper district")
	await place(Vector3(city.district_divider - 0.01, 170, 0))
	check(not arrow.visible, "guidance stops on leaving Aurelia for Eden")
	await place(Vector3(city.district_divider, 170, 0))
	check(arrow.visible, "returning to Aurelia enables the same ride's guidance again")
	await capture("district-entry")
	await place(Vector3(city.city_right + 1, 180, 0))
	check(not arrow.visible, "leaving the city never counts as the destination district")
	trip("depot")
	await place(Vector3(-22, city.low_city_top - 0.01, 0))
	check(not arrow.visible, "LowLife does not reveal a destination in Velvet")
	trip("foundry_market")
	await place(Vector3(12, 42, 0))
	check(arrow.visible and arrow.direction.x > 0.5 and arrow.direction.y > 0.5, "arrow points directly down/right to the platform, not towards a route corridor")
	await capture("approach")
	await place(Vector3(32, 42, 0))
	check(arrow.visible and arrow.direction.x < -0.5 and arrow.direction.y > 0.5, "passing the target horizontally updates its bearing to down/left")
	await place(Vector3(12, 24, 0))
	check(arrow.visible and arrow.direction.x > 0.5 and arrow.direction.y < -0.4, "a target above the vehicle produces an upward bearing")
	director.message("Jedźmy do FOUNDRY / OCTANE.", 5)
	await capture("bubble")
	level.taxi_hud.open_overlay("map")
	await frames()
	check(not arrow.visible, "city reference hides the in-flight arrow")
	level.taxi_hud.close_overlay()
	await frames()
	check(arrow.visible, "closing the map restores local guidance without a new trip")
	level.taxi_hud.open_overlay("options")
	await frames()
	check(not arrow.visible, "options hide the in-flight arrow")
	level.taxi_hud.close_overlay()
	context.player.suspend(&"dialogue")
	await frames()
	check(not arrow.visible, "a dialogue lock hides guidance without cancelling the ride")
	context.player.resume(&"dialogue")
	context.player.release_control()
	await frames()
	check(not arrow.visible, "leaving the taxi hides its guidance")
	context.player.take_control(cab, &"flight")
	var original_map: String = context.rides.active.map
	context.rides.active.map = "interior"
	await frames()
	check(not arrow.visible, "a trip in another map cannot guide to a same-ID city stop")
	context.rides.active.map = original_map
	var target: TaxiStop = director.network.stops.foundry_market
	target.unlocked = false
	await frames()
	check(not arrow.visible, "an unavailable destination cannot retain an old arrow")
	target.unlocked = true
	context.rides.active.phase = "alighting"
	await frames()
	check(not arrow.visible, "guidance ends when passengers begin getting out")
	context.rides.complete(cab.state, context.ledger)
	await frames()
	check(not arrow.visible and context.rides.active.is_empty(), "completed trip clears guidance")
	trip("foundry_market")
	await frames()
	context.rides.cancel(context.vehicles)
	await frames()
	check(not arrow.visible, "cancelling a trip clears guidance")
	level.queue_free()
	await frames()
	context.queue_free()
	await process_frame
	print("TAXI_GUIDANCE_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)
