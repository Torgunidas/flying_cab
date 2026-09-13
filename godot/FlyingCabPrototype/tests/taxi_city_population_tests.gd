extends SceneTree
## City-wide population, save migration and physical pickup/dropoff at every pad.
var context: RuntimeContext
var level: Node3D
var director: TaxiDirector
var cab: FlightCab
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

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
		context.player.resume(&"application_focus")
		director.advance(1.0 / 60)
	await process_frame

func place(stop: TaxiStop, offset: float) -> void:
	cab.freeze = true
	cab.command = Vector2.ZERO
	cab.global_position = stop.landing_point() + Vector3(offset, 1, 0)
	cab.linear_velocity = Vector3.ZERO
	cab._previous_velocity = Vector3.ZERO
	cab._pre_contact_velocity = Vector3.ZERO
	cab.state.condition = 1
	await physics_frame
	await physics_frame
	cab.reset_physics_interpolation()
	cab.freeze = false
	cab.sleeping = false

func capture(label: String) -> void:
	if DisplayServer.get_name() != "headless":
		for i in range(4):
			await process_frame
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/population-" + label + ".png")

func unique_waiters() -> bool:
	var seen := {}
	for offer: Dictionary in context.rides.offers:
		if seen.has(offer.origin) or offer.party.size() != 1:
			return false
		seen[offer.origin] = true
	return true

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	# Earlier versions generated waiting groups; migrate without discarding the save.
	var legacy := context.rides.create_offer("depot", "foundry_market", 141.71, 3, 150)
	legacy.walk = 1.0
	var first_id: String = legacy.party[0].id
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	director = level.taxi
	cab = level.cab
	var rides := context.rides
	check(director.network.stops.size() == 25 and rides.offers.size() == 25 and unique_waiters(), "all 25 platforms have exactly one waiting passenger at initialization")
	check(legacy.party.size() == 1 and legacy.party[0].id == first_id and is_equal_approx(legacy.fare, 83.36), "legacy waiting group keeps one person and receives the single-person quote")
	check(rides.create_offer("depot", "eden_01", 90, 1, 150).is_empty(), "another caller cannot add a second waiting fare at an occupied platform")
	var times := {}
	for offer: Dictionary in rides.offers:
		times[int(offer.remaining)] = true
	check(times.size() > 10, "waiting lifetimes are staggered instead of expiring together")
	var snapshot := context.snapshot()
	var loaded := RuntimeContext.new()
	check(loaded.restore(snapshot) and loaded.rides.offers.size() == 25, "the entire city population including new stop IDs survives a save round trip")
	var malformed := snapshot.duplicate(true)
	var duplicate: Dictionary = malformed.rides.offers[0].duplicate(true)
	duplicate.id = "ride_1000"
	duplicate.party[0].id = "ride_1000/person_0"
	malformed.rides.serial = 1000
	malformed.rides.offers[1] = duplicate
	check(not loaded.restore(malformed), "save validation rejects two independent waiting fares at the same stop")
	loaded.free()
	# Stress the whole shared pool with one appearance, not just a balanced mix.
	for offer: Dictionary in rides.offers:
		offer.party[0].look = 0
	director._assigned.clear()
	director._render_people(0)
	var visible := true
	for offer: Dictionary in rides.offers:
		var slot: int = director._assigned.get(offer.party[0].id, -1)
		visible = visible and slot >= 0
		if slot >= 0:
			visible = visible and director._pool[slot].visible and director._pool[slot].get_node("Skeleton3D/Body").mesh == director._appearances[0]
	check(visible and director._pool.size() == 37, "all 25 people remain visible even when they share one coat variant; pool stays bounded")
	# Let dispatch cycle repeatedly without a player collecting anyone.
	cab.freeze = true
	context.player.release_control()
	var start_serial := rides.serial
	var consistent := true
	for step in range(4200):
		director.advance(0.1)
		consistent = consistent and unique_waiters() and rides.offers.size() <= 25
	check(consistent and rides.serial > start_serial + 40, "seven minutes of expiry and replacement keep one waiter per platform and bounded population")
	context.player.take_control(cab, &"flight")
	director.rules = director.rules.duplicate()
	director.rules.max_offers = 0 # Explicit per-stop transport fixture below.
	rides.offers.clear()
	director.departures.clear()
	var ids: Array = director.network.stops.keys()
	var sample := OS.get_cmdline_user_args().has("--visual-sample")
	for i in range(ids.size()):
		if sample and not ids[i] in ["velvet_06", "eden_06", "foundry_06", "aurelia_06"]:
			continue
		var origin: TaxiStop = director.network.stops[ids[i]]
		var destination: TaxiStop = director.network.stops[ids[(i + 1) % ids.size()]]
		var fare := director.rules.quote(TaxiNetwork.length(director.network.path_between(origin.stop_id, destination.stop_id)), 1)
		var offer := rides.create_offer(origin.stop_id, destination.stop_id, fare, 1, 10000)
		offer.walk = 1.0
		var party_id: String = offer.party[0].id
		var wallet := context.campaign.credits
		var offset := origin.half_width - 1.2
		await place(origin, offset)
		await frames(130)
		await capture(origin.stop_id + "-pickup")
		await frames(390)
		check(not rides.active.is_empty() and rides.active.phase == "riding" and cab.state.passenger_ids == PackedStringArray([party_id]), origin.stop_id + ": one passenger boards automatically at the real edge berth")
		await place(destination, -(destination.half_width - 1.2))
		await frames(240)
		check(rides.active.is_empty() and cab.state.passenger_ids.is_empty() and is_equal_approx(context.campaign.credits, wallet + fare), destination.stop_id + ": the passenger exits automatically and the fare pays exactly once")
	# Active legacy groups are preserved across map reconstruction (not truncated).
	var active := rides.create_offer("depot", "foundry_market", 141.71, 3, 10000)
	active.walk = 1
	rides.select(active.id)
	check(rides.begin_boarding(cab.state, cab.definition) and rides.board(cab.state, cab.definition), "legacy active group fixture has a complete physical vehicle manifest")
	snapshot = context.snapshot()
	var restored := RuntimeContext.new()
	check(restored.restore(snapshot), "an already boarded legacy group remains loadable")
	level.queue_free()
	await process_frame
	context.queue_free()
	await process_frame
	context = restored
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	check(context.rides.active.party.size() == 3 and level.cab.state.passenger_ids.size() == 3 and context.rides.active.fare == 141.71 and unique_waiters(), "opening the new city preserves an existing group's people and price while new waiters are single")
	level.queue_free()
	await process_frame
	context.queue_free()
	await process_frame
	print("TAXI_CITY_POPULATION_TESTS: ", checks - failures, "/", checks, " visual_sample=", sample)
	quit(1 if failures else 0)
