extends SceneTree
var checks := 0
var failures := 0
var level: Node3D

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
		if level and level.taxi.initialized:
			level.taxi.advance(1.0 / 60.0)
	await process_frame

func _run() -> void:
	var context := RuntimeContext.new()
	context.rides.enabled = true
	context.campaign.credits = 120
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	var director: TaxiDirector = level.taxi
	var rides := context.rides
	var cab: FlightCab = level.cab
	check(director.network.stops.size() == 25, "all 25 authored taxi stops bind to their real pads")
	print("NETWORK: stops=", director.network.stops.keys(), " points=", director.network.points.size(), " offers=", rides.offers.size())
	check(rides.offers.size() == 25, "initial city has one reachable fare on each of its 25 platforms")
	if rides.offers.size() != 25:
		_finish(context)
		return
	var reachable := true
	for a: String in director.network.stops:
		for b: String in director.network.stops:
			if a != b:
				var path := director.network.path_between(a, b)
				reachable = reachable and not path.is_empty()
	check(reachable, "all 600 stop pairs have clearance-checked corridor routes")
	var depot: TaxiStop = director.network.stops.depot
	depot.unlocked = false
	check(director.network.path_between("depot", "foundry_market").is_empty(), "locked destinations cannot produce usable routes")
	depot.unlocked = true
	check(not director.network.clear_segment(Vector3(-22, 53, 0), Vector3(-22, 60, 0)), "route clearance rejects a segment starting inside a real landing pad")
	# Keep this transaction fixture isolated from the new city-wide population.
	rides.offers = [rides.offers[0]]
	director.rules = director.rules.duplicate()
	director.rules.max_offers = 0
	await frames(280)
	var lifetime: float = rides.offers[0].remaining
	context.player.suspend(&"taxi_test")
	director.fuel_requested = true
	director.advance(10)
	check(rides.offers[0].remaining == lifetime and not director.fuel_requested, "suspension pauses offers and clears held fueling input")
	context.player.resume(&"taxi_test")
	cab.fuel = 99.5
	var fuel_wallet := context.campaign.credits
	director.fuel_requested = true
	director.advance(0.1)
	director.fuel_requested = false
	check(cab.fuel == 100 and is_equal_approx(context.campaign.credits, fuel_wallet - 0.5), "the physical depot sells only the missing fuel and charges its exact quantity")
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/taxi-start.png")
	var original: Dictionary = rides.offers[0]
	var ids := rides.party_ids(original)
	var fare: float = original.fare
	var wallet := context.campaign.credits
	await frames(20)
	check(not rides.active.is_empty() and rides.active.phase == "boarding" and VehicleOccupancy.reserved_count(cab.state) == 1, "stopped taxi reserves the passenger's seat while they walk to the cab")
	check(not cab.board_passenger(StringName(ids[0])), "the public cab API cannot board a person whose seat is still reserved")
	cab.command = Vector2(0, 1)
	await frames(10)
	check(rides.active.is_empty() and cab.state.reservations.is_empty() and cab.state.passenger_ids.is_empty(), "takeoff during boarding cancels the reservation without losing the offer")
	check(original.get("return_remaining", 0.0) > 0 and original.return_points.size() == ids.size(), "interrupted boarding retains the person's current position for their walk back")
	cab.command = Vector2.ZERO
	await frames(480)
	if rides.active.is_empty():
		await frames(180)
	check(not rides.active.is_empty() and rides.active.phase == "riding" and cab.state.passenger_ids.has(ids[0]), "passenger boards the actual settled taxi and retains their ID")
	check(cab.state.reservations.is_empty() and context.campaign.credits == wallet, "boarding converts the reservation without paying prematurely")
	var foot := WalkingActor.new()
	level.add_child(foot)
	foot.set_physics_process(false)
	context.player.take_control(foot, &"on_foot")
	director.advance(0.1)
	check(rides.active.phase == "riding" and rides.active.vehicle == String(cab.state.entity_id) and cab.state.passenger_ids.has(ids[0]), "changing to an on-foot actor leaves the fare and passenger in their actual vehicle")
	context.player.take_control(cab, &"flight")
	foot.queue_free()
	if rides.active.is_empty():
		_finish(context)
		return
	var saved := context.snapshot()
	var restored := RuntimeContext.new()
	check(restored.restore(saved) and restored.rides.active.id == rides.active.id and restored.vehicles[cab.state.entity_id].passenger_ids == cab.state.passenger_ids, "active journey, passenger identity and vehicle manifest survive JSON state restore")
	var malformed := saved.duplicate(true)
	malformed.rides.active.party[0].look = -1
	check(not restored.restore(malformed) and restored.rides.active.id == rides.active.id, "invalid passenger presentation is rejected before mutating restored state")
	malformed = saved.duplicate(true)
	malformed.rides.active.destination = "missing_stop"
	check(not restored.restore(malformed), "a saved journey cannot reference an unknown destination")
	malformed = saved.duplicate(true)
	malformed.rides.serial = 0
	check(not restored.restore(malformed), "invalid saved serial cannot reuse a journey or payment identity")
	var destination: TaxiStop = director.network.stops[rides.active.destination]
	var target := destination.landing_point()
	# A real body relocation is used only to shorten the transport fixture. Fare is fixed.
	cab.freeze = true
	cab.global_position = target + Vector3(0, 10, 0)
	cab.reset_physics_interpolation()
	await frames(30)
	cab.global_position += Vector3(10, 0, 0)
	await frames(30)
	cab.global_position -= Vector3(10, 0, 0)
	await frames(30)
	check(rides.active.fare == fare and context.campaign.credits == wallet, "flying toward and away from the destination cannot raise the agreed fare")
	check(rides.active.phase == "riding", "hovering near the destination does not disembark the passenger")
	cab.global_position = target
	cab.linear_velocity = Vector3.ZERO
	cab._previous_velocity = Vector3.ZERO
	cab._pre_contact_velocity = Vector3.ZERO
	cab.freeze = false
	cab.sleeping = false
	cab.reset_physics_interpolation()
	await frames(220)
	check(rides.active.is_empty() and rides.completed == 1 and cab.state.passenger_ids.is_empty(), "landing on the destination pad completes physical disembarkation")
	check(is_equal_approx(context.campaign.credits, wallet + fare) and context.campaign.receipts.has(original.id), "completed ride pays its exact quote once")
	check(not context.ledger.credit_once(original.id, fare) and not rides.complete(cab.state, context.ledger), "duplicate completion and receipt replay cannot pay twice")
	check(not director.departures.is_empty() and director.departures[0].party[0].id == ids[0], "the delivered person walks away with their original identity and appearance")
	var before_fuel := cab.fuel
	director.fuel_requested = true
	director.advance(0.1)
	director.fuel_requested = false
	check(director.fuel_station == null and cab.fuel == before_fuel and not cab.free_refueling, "ordinary taxi stops do not provide automatic or paid fuel")
	var model := VehicleDefinition.new()
	model.max_passengers = 3
	var state := VehicleState.new()
	state.driver_id = &"driver"
	check(VehicleOccupancy.reserve(state, model, "group", ["a", "b", "c"]), "a party reserves all three seats atomically")
	check(not VehicleOccupancy.reserve(state, model, "other", ["d"]), "another journey cannot steal reserved seats")
	check(VehicleOccupancy.commit(state, model, "group") and state.passenger_ids.size() == 3, "the same occupancy service boards a whole group")
	state.passenger_ids.erase("a")
	check(not VehicleOccupancy.reserve(state, model, "crowd", ["d", "e"]), "remaining passengers count toward capacity at a subsequent stop")
	check(not VehicleOccupancy.reserve(state, model, "duplicate", ["b"]), "one person cannot reserve a second seat")
	state.passenger_ids.clear()
	state.entity_id = &"group_cab"
	var group_service := RideService.new()
	var group_offer := group_service.create_offer("depot", "foundry_market", 80, 3, 100)
	group_offer.walk = 1.0
	group_service.select(group_offer.id)
	var group_wallet := CampaignState.new()
	group_wallet.credits = 0
	var group_ledger := EconomyLedger.new()
	group_ledger.state = group_wallet
	group_service.recovery_debt = 35
	check(group_service.begin_boarding(state, model) and group_service.board(state, model), "a three-person fare uses the same complete boarding transaction")
	group_service.active.phase = "alighting"
	check(group_service.complete(state, group_ledger) and state.passenger_ids.is_empty() and group_wallet.credits == 60 and group_service.recovery_debt == 15, "one group fare pays once and repays exactly 25 percent toward recovery debt")
	var cost: Array[float] = []
	for hz in [60, 120]:
		var campaign := CampaignState.new()
		campaign.credits = 100
		var ledger := EconomyLedger.new()
		ledger.state = campaign
		var fuel := 0.0
		for step in range(hz * 2):
			fuel += ledger.purchase_units(15.0 / hz, 25.3 - fuel, 1.0)
		cost.append(100 - campaign.credits)
	check(is_equal_approx(cost[0], 25.3) and is_equal_approx(cost[0], cost[1]), "fractional fuel costs depend on quantity, not simulation frequency")
	var poor := CampaignState.new()
	poor.credits = 0.23
	var small_wallet := EconomyLedger.new()
	small_wallet.state = poor
	check(is_equal_approx(small_wallet.purchase_units(10, 10, 2), 0.115) and is_zero_approx(poor.credits), "last credits buy precisely the affordable partial service")
	context.campaign.credits = 0
	cab.state.condition = 0
	cab.fuel = 0
	check(director.request_recovery(true), "emergency recovery is available with empty fuel, hull and wallet")
	await frames(60)
	check(cab.hull == 50 and is_equal_approx(cab.fuel, 35) and rides.recovery_debt == 35 and context.campaign.credits == 0, "recovery restores only emergency resources and records the unpaid fee")
	var path := "res://build/taxi-save-test.json"
	check(context.save_to(path) == OK, "ride state and wallet can be saved to disk")
	var loaded := RuntimeContext.new()
	check(loaded.load_from(path) and loaded.rides.recovery_debt == 35 and loaded.campaign.receipts.has(original.id), "receipt and recovery debt survive disk reload")
	loaded.free()
	# Recreate the map around the earlier detached save with a passenger aboard.
	level.queue_free()
	await process_frame
	context.free()
	root.add_child(restored)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = restored
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	await frames(10)
	check(restored.rides.active.phase == "riding" and level.cab.state.passenger_ids.has(ids[0]) and level.taxi.current_stop_id() == original.destination, "recreating the whole city resumes the saved vehicle, onboard passenger and destination")
	_finish(restored)

func _finish(context: RuntimeContext) -> void:
	level.queue_free()
	await process_frame
	context.queue_free()
	await process_frame
	print("TAXI_TESTS: ", checks - failures, "/", checks)
	quit(1 if failures else 0)
