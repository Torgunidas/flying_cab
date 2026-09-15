extends SceneTree
var checks := 0
var failures := 0
var level: Node3D
var context: RuntimeContext
var cab: FlightCab
var director: TaxiDirector
var walk_samples := {"boarding": 0, "alighting": 0}
var walk_speed_ok := {"boarding": true, "alighting": true}
var walk_facing_ok := true

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
		# A deterministic fixture must not depend on which desktop window has
		# focus. Production suspension is covered separately by taxi_tests.
		context.player.resume(&"application_focus")
		var trip := context.rides.active
		var phase: String = trip.get("phase", "")
		var person: PassengerVisual
		var before := Vector3.ZERO
		if walk_samples.has(phase) and trip.party.size() == 1 and trip.progress > 0:
			person = director._pool[director._assigned[trip.party[0].id]] as PassengerVisual
			before = person.global_position
		director.advance(1.0 / 60.0)
		if person and not context.rides.active.is_empty() and context.rides.active.phase == phase:
			var travel := person.global_position - before
			walk_samples[phase] += 1
			walk_speed_ok[phase] = walk_speed_ok[phase] and absf(travel.length() * 60.0 - 1.5) < 0.005
			walk_facing_ok = walk_facing_ok and person.basis.z.dot(travel.normalized()) > 0.999
	await process_frame

func place(stop: TaxiStop, offset: float) -> void:
	# Position only the approach fixture; contact, rest and passenger interaction
	# use the real body and geometry. taxi_route_tests covers continuous flight.
	cab.freeze = true
	cab.command = Vector2.ZERO
	cab.global_position = stop.landing_point() + Vector3(offset, 1.0, 0)
	cab.linear_velocity = Vector3.ZERO
	cab._previous_velocity = Vector3.ZERO
	cab._pre_contact_velocity = Vector3.ZERO
	cab.state.condition = 1
	await physics_frame
	await physics_frame
	cab.reset_physics_interpolation()
	cab.freeze = false
	cab.sleeping = false

func capture(name: String) -> void:
	if DisplayServer.get_name() != "headless":
		for i in range(12):
			await process_frame
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/" + name + ".png")

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	context.campaign.credits = 120
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.living_world_enabled = false # Isolated fixture; full population has its own integration suite.
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	director = level.taxi
	cab = level.cab
	director.rules = director.rules.duplicate()
	director.rules.max_offers = 0 # Keep this route fixture free of newly generated fares.
	# Reproduce the reported save: ROOM 09 -> depot is an unselected offer,
	# Ari is already at the depot, and there are no passengers in the cab.
	context.rides.offers.clear()
	var offer := context.rides.create_offer("velvet_club", "depot", 91.36, 1, 1000)
	offer.walk = 1.0
	await frames(120)
	check(context.rides.active.is_empty() and cab.state.passenger_ids.is_empty(), "an offer is not mistaken for an onboard passenger while waiting at its destination")
	check(director.current_stop_id().is_empty(), "waiting at a remote offer's destination does not invent a pickup target")
	await capture("taxi-minimal-idle")
	# A pre-update selected offer must not divert local automatic pickup.
	context.rides.selected = "legacy_selection"
	context.rides.offers.clear()
	var room: TaxiStop = director.network.stops.velvet_club
	var depot: TaxiStop = director.network.stops.depot
	# Cover the reported route, one person and a full party, and both edge berths.
	for count in [1, 3]:
		for offset in [-3.8, 3.8]:
			var trip := context.rides.create_offer("velvet_club", "depot", 91.36, count, 1000)
			trip.walk = 1.0
			var wallet := context.campaign.credits
			await place(room, offset)
			await frames(520)
			check(not context.rides.active.is_empty() and context.rides.active.phase == "riding" and cab.state.passenger_ids.size() == count, "%d passengers board at ROOM 09 with berth offset %.1f" % [count, offset])
			check(director.current_stop_id() == "depot", "the trip destination is the depot after automatic pickup")
			await place(depot, offset)
			await frames(220)
			check(depot.can_board(cab) and context.rides.active.is_empty() and cab.state.passenger_ids.is_empty(), "%d passengers can disembark from a fully parked edge berth %.1f at the depot" % [count, offset])
			check(is_equal_approx(context.campaign.credits, wallet + trip.fare), "the edge-berth trip pays exactly once after disembarkation")
	# A car really overhanging the edge remains blocked with an explicit reason.
	var blocked := context.rides.create_offer("velvet_club", "depot", 91.36, 2, 1000)
	blocked.walk = 1.0
	await place(room, 4.5)
	await frames(120)
	check(cab.grounded and cab.sleeping and not room.can_board(cab) and context.rides.active.is_empty(), "partially overhanging cars still cannot board passengers")
	check(director.interaction_hint() == "PRZESUŃ AUTO BLIŻEJ ŚRODKA TARASU", "a blocked berth tells the player exactly how to unblock it")
	await place(room, 0)
	await frames(520)
	check(context.rides.active.phase == "riding" and cab.state.passenger_ids.size() == 2, "moving fully onto the terrace allows the waiting group to board")
	await place(depot, 0)
	var obstacle := StaticBody3D.new()
	var obstacle_collision := CollisionShape3D.new()
	var obstacle_shape := BoxShape3D.new()
	obstacle_shape.size = Vector3(0.6, 2, 0.5)
	obstacle_collision.shape = obstacle_shape
	obstacle.add_child(obstacle_collision)
	level.add_child(obstacle)
	obstacle.global_position = depot.exit_point(cab, 0) + Vector3(0, 1, 0)
	await frames(150)
	check(context.rides.active.phase == "riding" and cab.state.passenger_ids.size() == 2 and director.interaction_hint() == "ZOSTAW WOLNE MIEJSCE OBOK AUTA", "a real obstacle blocks disembarkation with a temporary explanation")
	obstacle.queue_free()
	for i in range(180):
		await frames(1)
		if not context.rides.active.is_empty() and context.rides.active.phase == "alighting":
			break
	var wallet := context.campaign.credits
	cab.command = Vector2(0, 1)
	director.advance(1.0 / 60)
	check(director.interaction_hint() == "PUŚĆ PRZYCISKI LOTU" and context.rides.active.phase == "riding", "thrust interrupts disembarkation and explains why it stopped")
	await frames(6)
	check(cab.state.passenger_ids.size() == 2 and context.campaign.credits == wallet, "interrupted disembarkation neither loses people nor pays early")
	cab.command = Vector2.ZERO
	await frames(220)
	check(context.rides.active.is_empty() and cab.state.passenger_ids.is_empty() and is_equal_approx(context.campaign.credits, wallet + 91.36), "releasing thrust and landing finishes the same journey")
	await capture("taxi-depot-delivery")
	# Validate geometry-consistent boarding width for every current taxi stop.
	for stop: TaxiStop in director.network.stops.values():
		await place(stop, stop.half_width - 1.2)
		await frames(120)
		check(stop.can_board(cab) and director._exit_clear(stop, cab, 3), stop.stop_id + " accepts a fully supported edge berth and a three-person exit")
	await place(depot, 3.8)
	await frames(120)
	var larger := BoxShape3D.new()
	larger.size = Vector3(3.2, 0.7, 0.9)
	cab.get_node("Collision").shape = larger
	await frames(20)
	check(not depot.can_board(cab), "a wider vehicle uses its actual collision width when checking the edge")
	await place(depot, 0)
	await frames(120)
	check(depot.can_board(cab), "the same wider vehicle can board when fully parked on the terrace")
	for phase: String in walk_samples:
		check(walk_samples[phase] > 5 and walk_speed_ok[phase], phase + " uses 1.5 m/s on actual rendered door paths at both edge berths")
	check(walk_facing_ok, "passengers face their movement through boarding and both left/right exits")
	level.queue_free()
	await process_frame
	context.queue_free()
	await process_frame
	print("TAXI_INTERACTION_TESTS: ", checks - failures, "/", checks)
	quit(1 if failures else 0)
