class_name TaxiDirector
extends Node3D
## Map adapter: automatic curbside pickup, safe exits and local presentation.
signal checkpoint_requested
const PEOPLE = [preload("res://scenes/people/passenger_0.tscn"), preload("res://scenes/people/passenger_1.tscn"), preload("res://scenes/people/passenger_2.tscn"), preload("res://scenes/people/passenger_3.tscn")]
@export var rules: TaxiRules = preload("res://resources/taxi/default_rules.tres")
var context: RuntimeContext
var network := TaxiNetwork.new()
var initialized := false
var notice := "Zatrzymaj się przy pasażerze i puść ciąg."
var notice_remaining := 5.0
var fuel_station: TaxiStop
var fuel_requested := false
var fueling := false
var departures: Array = []
var _pool: Array[Node3D] = []
var _appearances: Array[Mesh] = []
var _assigned: Dictionary = {}
var _shown_last_frame: Dictionary = {}
var _rng := RandomNumberGenerator.new()
var _spawn_time := 0.0
var _recover_armed := 0.0
var _exit_blocked := false
var _last_local_hint := ""
var _hint_time := 0.0

func _ready() -> void:
	set_process(false)
	set_physics_process(false)
	_rng.seed = rules.population_seed
	# Fixed small presentation pool: every mesh variant is rendered during warmup.
	for i in range(maxi(36, rules.max_offers + 12)):
		var person: Node3D = PEOPLE[i % 4].instantiate()
		add_child(person)
		person.position = Vector3(-25 + i * 0.6, 54, 1.2)
		_pool.append(person)
		if i < PEOPLE.size():
			_appearances.append(person.get_node("Skeleton3D/Body").mesh)

func initialize() -> void:
	if initialized:
		return
	if rules == null or not rules.validation_errors().is_empty():
		push_error("Invalid taxi rules")
		return
	network.bind(context.world)
	for pad in context.world.pads:
		var label := pad.get_node_or_null("BerthID") as Label3D
		if label:
			var stop := pad.get_node_or_null("TaxiStop") as TaxiStop
			label.text = String(pad.name).to_upper() + (" / FUEL + REPAIR" if stop and stop.fuel_service else (" / TAXI" if stop else " / LANDING"))
	initialized = true
	# Old waiting groups become one-person fares; already active journeys keep
	# their manifest, identities and agreed price until completion.
	for offer: Dictionary in context.rides.offers:
		if offer.party.size() > 1:
			offer.party = offer.party.slice(0, 1)
			offer.fare = rules.quote(TaxiNetwork.length(network.path_between(offer.origin, offer.destination)), 1)
			if offer.has("return_points"):
				offer.return_points = offer.return_points.slice(0, 1)
	if context.rides.offers.is_empty() and context.rides.active.is_empty():
		_spawn("depot", "foundry_market")
	for id: String in network.stops:
		_spawn(id)
	_render_people(0)

func _spawn(origin := "", destination := "") -> void:
	var rides := context.rides
	if rides.offers.size() >= rules.max_offers or network.stops.size() < 2:
		return
	var available: Array = []
	for id: String in network.stops:
		var occupied := false
		for offer: Dictionary in rides.offers:
			occupied = occupied or offer.origin == id
		if not rides.active.is_empty():
			occupied = occupied or rides.active.destination == id or rides.active.origin == id
		for departure: Dictionary in departures:
			occupied = occupied or departure.stop == id
		if not occupied and network.stops[id].unlocked:
			available.append(id)
	if origin.is_empty():
		if available.is_empty():
			return
		origin = available[_rng.randi_range(0, available.size() - 1)]
	if not available.has(origin):
		return
	if destination.is_empty():
		var choices: Array = network.stops.keys()
		choices.erase(origin)
		destination = choices[_rng.randi_range(0, choices.size() - 1)]
	var focused := context.player.focus as FlightCab
	if focused and focused.definition.navigation_clearance != network.clearance:
		network.clearance = focused.definition.navigation_clearance
		network.edge_cache.clear()
	var path := network.path_between(origin, destination)
	if path.is_empty():
		return
	var lifetime := rules.offer_seconds * _rng.randf_range(1.0 - rules.lifetime_jitter, 1.0 + rules.lifetime_jitter)
	rides.create_offer(origin, destination, rules.quote(TaxiNetwork.length(path), 1), 1, lifetime)

func advance(dt: float) -> void:
	if not initialized or dt <= 0 or not is_finite(dt) or context.player.is_suspended():
		fueling = false
		fuel_requested = false
		return
	dt = minf(dt, 0.1)
	_recover_armed = maxf(0, _recover_armed - dt)
	notice_remaining = maxf(0, notice_remaining - dt)
	var rides := context.rides
	var cab := context.player.focus as FlightCab
	_exit_blocked = false
	_update_services(cab, dt)
	for offer: Dictionary in rides.offers.duplicate():
		var stop: TaxiStop = network.stops.get(offer.origin)
		if stop:
			if offer.walk < 1:
				offer.walk = minf(1, offer.walk + _walk_step(_waiting_distance(stop, offer.party.size()), dt))
			if offer.get("return_remaining", 0.0) > 0:
				offer.return_remaining = maxf(0, offer.return_remaining - _walk_step(_waiting_distance(stop, offer.party.size(), offer.return_points), dt))
		offer.remaining = maxf(0, offer.remaining - dt)
		if offer.remaining == 0:
			_depart(offer, offer.origin)
			rides.offers.erase(offer)
			if rides.selected == offer.id:
				rides.selected = ""
				message("Pasażer odszedł.")
	if rides.active.is_empty() and cab:
		# A saved selection is legacy UI state, never a pickup prerequisite.
		rides.selected = ""
		for offer: Dictionary in rides.offers:
			var stop: TaxiStop = network.stops.get(offer.origin)
			if stop == null or not stop.unlocked or not stop.can_board(cab):
				continue
			if not network.stops.has(offer.destination) or not network.stops[offer.destination].unlocked:
				continue
			if offer.walk < 1 or offer.get("return_remaining", 0.0) > 0:
				continue
			if network.clearance != cab.definition.navigation_clearance:
				network.clearance = cab.definition.navigation_clearance
				network.edge_cache.clear()
			if network.path_between(offer.origin, offer.destination).is_empty():
				continue
			rides.select(offer.id)
			if not rides.begin_boarding(cab.state, cab.definition):
				rides.selected = ""
				continue
			message("Do %s, proszę.  %.2f CR" % [network.stops[offer.destination].display_name, offer.fare], 5)
			break
	if not rides.active.is_empty():
		_update_journey(cab, dt)
	_spawn_time += dt
	if _spawn_time >= 5:
		_spawn()
		_spawn_time = 0
	_update_local_hint(cab, dt)
	_render_people(dt)

func _update_journey(focused: FlightCab, dt: float) -> void:
	var rides := context.rides
	var trip := rides.active
	var state: VehicleState = context.vehicles.get(StringName(trip.vehicle))
	if state == null or state.condition <= 0:
		abort("Kurs przerwany — auto niesprawne. Pasażer wróci do przystanku.")
		return
	if trip.phase != "boarding":
		for id in rides.party_ids(trip):
			if not state.passenger_ids.has(id):
				abort("Kurs przerwany — zmieniła się obsada pojazdu.")
				return
	var cab: FlightCab
	for vehicle: FlightCab in context.world.vehicles:
		if String(vehicle.state.entity_id) == trip.vehicle:
			cab = vehicle
			break
	# Parked car and passengers stay together when Ari changes actor or map.
	if cab == null:
		return
	if trip.phase == "boarding":
		var stop: TaxiStop = network.stops.get(trip.origin)
		if cab != focused or stop == null or not stop.can_board(cab):
			if stop:
				trip.return_points = []
				trip.return_remaining = 1.0
				for i in range(trip.party.size()):
					var p := stop.waiting_point(i)
					if _assigned.has(trip.party[i].id):
						p = _pool[_assigned[trip.party[i].id]].global_position
					trip.return_points.append([p.x, p.y, p.z])
			rides.interrupt_boarding(state)
			message("Wsiadanie przerwane. Zatrzymaj się przy pasażerze.")
			return
		# Recompute from the same endpoints as presentation, also for old saves.
		trip.duration = maxf(0.001, _car_walk_distance(stop, cab, trip.party.size(), true) / rules.walking_speed)
		trip.progress = minf(1, trip.progress + dt / trip.duration)
		if trip.progress >= 1:
			if rides.board(state, cab.definition):
				message("Jedźmy do " + network.stops[trip.destination].display_name + ".", 5)
			else:
				rides.interrupt_boarding(state)
				message("Wsiadanie przerwane — zmieniła się dostępność miejsc.")
	elif trip.phase in ["riding", "alighting"]:
		var stop: TaxiStop = network.stops.get(trip.destination)
		var parked := cab == focused and stop != null and stop.can_board(cab)
		_exit_blocked = parked and not _exit_clear(stop, cab, trip.party.size())
		if not parked or _exit_blocked:
			trip.phase = "riding"
			trip.progress = 0.0
			return
		trip.phase = "alighting"
		trip.duration = maxf(0.001, _car_walk_distance(stop, cab, trip.party.size(), false) / rules.walking_speed)
		trip.progress = minf(1, trip.progress + dt / trip.duration)
		if trip.progress >= 1:
			var finished := trip.duplicate(true)
			var before := context.campaign.credits
			if rides.complete(state, context.ledger):
				_depart(finished, finished.destination, cab)
				message("Kurs zakończony  +%.2f CR" % (context.campaign.credits - before), 6)
				checkpoint_requested.emit()

func _walk_step(distance: float, dt: float) -> float:
	return dt * rules.walking_speed / maxf(distance, 0.001)

func _waiting_distance(stop: TaxiStop, count: int, return_points: Array = []) -> float:
	var distance := 0.0
	for i in range(count):
		var start := stop.door_point()
		if not return_points.is_empty():
			var p: Array = return_points[i]
			start = Vector3(p[0], p[1], p[2])
		distance = maxf(distance, start.distance_to(stop.waiting_point(i)))
	return distance

func _car_door(stop: TaxiStop, cab: FlightCab) -> Vector3:
	return stop.to_global(Vector3(stop.to_local(cab.global_position).x, 0, 0.7))

func _car_walk_distance(stop: TaxiStop, cab: FlightCab, count: int, boarding: bool) -> float:
	var distance := 0.0
	var door := _car_door(stop, cab)
	for i in range(count):
		distance = maxf(distance, door.distance_to(stop.waiting_point(i) if boarding else stop.exit_point(cab, i)))
	return distance

func _exit_clear(stop: TaxiStop, cab: FlightCab, count: int) -> bool:
	var shape := HumanRig.clearance_shape()
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = shape
	query.collision_mask = WorldLayers.GEOMETRY
	query.exclude = [cab.get_rid()]
	for i in range(count):
		query.transform = Transform3D(Basis.IDENTITY, stop.exit_point(cab, i) + Vector3(0, HumanRig.HEIGHT * 0.5 + 0.02, 0))
		if not network.space.intersect_shape(query, 1).is_empty():
			return false
	return true

func _update_services(cab: FlightCab, dt: float) -> void:
	fuel_station = null
	fueling = false
	if cab == null:
		return
	for stop: TaxiStop in network.stops.values():
		if stop.fuel_service and stop.can_board(cab):
			fuel_station = stop
			break
	if fuel_station and fuel_requested:
		var units := context.ledger.purchase_units(rules.fuel_per_second * dt, cab.definition.fuel_capacity - cab.fuel, rules.fuel_price)
		cab.fuel += units
		context.rides.service_spent += units * rules.fuel_price
		fueling = units > 0
		if units > 0:
			context.narrative.record_event("fuel_purchased", {"amount": units, "target": String(fuel_station.stop_id), "vehicle": String(cab.entity_id)})

func _update_local_hint(cab: FlightCab, dt: float) -> void:
	# Only explain a stable local blockage once; no permanent instruction panel.
	var hint := ""
	var trip := current_ride()
	if cab and cab.grounded and not trip.is_empty():
		var stop: TaxiStop = network.stops.get(current_stop_id())
		if stop and cab.support_body == stop.get_parent():
			hint = interaction_hint()
			if hint in ["ZACZEKAJ NA WSIADANIE", "ZACZEKAJ NA WYSIADANIE", "PASAŻER IDZIE DO MIEJSCA ODBIORU"]:
				hint = ""
	if hint != _last_local_hint:
		_last_local_hint = hint
		_hint_time = 0.0
	var before := _hint_time
	_hint_time += dt
	if not hint.is_empty() and before < 2.0 and _hint_time >= 2.0:
		message(hint)

func _depart(trip: Dictionary, stop_id: String, cab: FlightCab = null) -> void:
	if not network.stops.has(stop_id):
		return
	var stop: TaxiStop = network.stops[stop_id]
	var starts: Array[Vector3] = []
	for i in range(trip.party.size()):
		var p := stop.exit_point(cab, i) if cab else stop.waiting_point(i)
		var id: String = trip.party[i].id
		if cab == null and _shown_last_frame.has(id) and _assigned.has(id):
			p = _pool[_assigned[id]].global_position
		starts.append(p)
	departures.append({"party": trip.party.duplicate(true), "stop": stop_id, "starts": starts, "progress": 0.0})

func abort(reason := "Kurs anulowany. Pasażer wróci do przystanku.") -> void:
	if context.player.is_suspended():
		return
	var trip := context.rides.active
	if not trip.is_empty():
		_depart(trip, trip.origin)
	context.rides.cancel(context.vehicles)
	message(reason)

func request_recovery(force := false) -> bool:
	var cab := context.player.focus as FlightCab
	if cab == null or context.player.is_suspended() or cab.resetting:
		return false
	if _recover_armed <= 0 and not force:
		_recover_armed = 4.0
		message("Holowanie %.0f CR — naciśnij ponownie. Brakującą kwotę spłacisz z kursów." % rules.tow_fee, 4)
		return false
	_recover_armed = 0
	var destination := "DEPOT"
	if is_instance_valid(context.world.living_world):
		var berth: Dictionary = context.world.living_world.recovery_berth(cab)
		if berth.is_empty():
			message("Brak wolnego miejsca dla holownika. Spróbuj ponownie za chwilę.")
			return false
		cab.spawn_transform = Transform3D(Basis.IDENTITY, berth.position)
		destination = berth.name
	abort("Holowanie: %s. Awaryjne paliwo i częściowa naprawa." % destination)
	var paid := minf(context.campaign.credits, rules.tow_fee)
	context.ledger.spend(paid)
	context.rides.recovery_debt += rules.tow_fee - paid
	cab.reset_fuel_amount = maxf(cab.fuel, rules.recovery_fuel)
	cab.reset_condition = maxf(cab.state.condition, rules.recovery_condition)
	cab.reset_flight()
	checkpoint_requested.emit()
	return true

func message(value: String, seconds := 4.0) -> void:
	notice = value
	notice_remaining = seconds

func current_ride() -> Dictionary:
	if not context.rides.active.is_empty():
		return context.rides.active
	var cab := context.player.focus as FlightCab
	if cab and cab.grounded:
		for offer: Dictionary in context.rides.offers:
			var stop: TaxiStop = network.stops.get(offer.origin)
			if stop and cab.support_body == stop.get_parent():
				return offer
	return {}

func current_stop_id() -> String:
	var trip := current_ride()
	if trip.is_empty():
		return ""
	return trip.origin if trip.phase in ["waiting", "boarding"] else trip.destination

func guidance_target() -> TaxiStop:
	# Local destination bearing only. Never query the route graph here.
	if not initialized or context.player.is_suspended():
		return null
	var trip := context.rides.active
	var cab := context.player.focus as FlightCab
	var city := context.world.definition as CityDefinition
	if trip.is_empty() or trip.get("phase", "") != "riding" or cab == null or city == null:
		return null
	if trip.get("vehicle", "") != String(cab.state.entity_id) or trip.get("map", "") != String(city.map_id):
		return null
	if cab.disabled or cab.resetting or cab.airspace.returning:
		return null
	var stop: TaxiStop = network.stops.get(trip.get("destination", ""))
	if not is_instance_valid(stop) or not stop.is_inside_tree() or not stop.unlocked:
		return null
	var district := city.district_id_at(cab.global_position)
	if district == &"" or district != city.district_id_at(stop.global_position):
		return null
	return stop

func interaction_hint() -> String:
	var trip := current_ride()
	if trip.is_empty():
		return "ZATRZYMAJ SIĘ PRZY PASAŻERZE"
	var cab := context.player.focus as FlightCab
	if cab == null or (not trip.vehicle.is_empty() and trip.vehicle != String(cab.state.entity_id)):
		return "WRÓĆ DO TAKSÓWKI OBSŁUGUJĄCEJ KURS"
	var stop: TaxiStop = network.stops.get(current_stop_id())
	if stop == null:
		return "PRZYSTANEK JEST NIEDOSTĘPNY"
	var blocked := stop.boarding_block(cab)
	if not blocked.is_empty():
		return blocked
	if _exit_blocked:
		return "ZOSTAW WOLNE MIEJSCE OBOK AUTA"
	if trip.phase == "waiting":
		if trip.walk < 1 or trip.get("return_remaining", 0.0) > 0:
			return "PASAŻER IDZIE DO MIEJSCA ODBIORU"
		if trip.party.size() > cab.definition.max_passengers - cab.state.passenger_ids.size() - VehicleOccupancy.reserved_count(cab.state):
			return "ZA MAŁO WOLNYCH MIEJSC W AUCIE"
		return "ZACZEKAJ NA WSIADANIE"
	return "ZACZEKAJ NA WSIADANIE" if trip.phase == "boarding" else "ZACZEKAJ NA WYSIADANIE"

func _render_people(dt: float) -> void:
	var live := {}
	var journeys: Array = context.rides.offers.duplicate()
	journeys.append_array(departures)
	if not context.rides.active.is_empty():
		journeys.append(context.rides.active)
	for journey: Dictionary in journeys:
		for person: Dictionary in journey.party:
			live[person.id] = true
	for id in _assigned.keys():
		if not live.has(id):
			_assigned.erase(id)
	for person in _pool:
		person.hide()
	var used := {}
	for offer: Dictionary in context.rides.offers:
		var stop: TaxiStop = network.stops.get(offer.origin)
		if stop:
			for i in range(offer.party.size()):
				var p := stop.door_point().lerp(stop.waiting_point(i), offer.walk)
				var returning: bool = offer.get("return_remaining", 0.0) > 0
				if returning:
					var previous: Array = offer.return_points[i]
					p = Vector3(previous[0], previous[1], previous[2]).lerp(stop.waiting_point(i), 1.0 - offer.return_remaining)
				_show_person(offer.party[i], p, offer.walk >= 1 and not returning, -1, dt, used)
	var trip := context.rides.active
	if not trip.is_empty() and trip.phase != "riding":
		var stop: TaxiStop = network.stops.get(trip.origin if trip.phase == "boarding" else trip.destination)
		var cab: FlightCab
		for candidate: FlightCab in context.world.vehicles:
			if String(candidate.state.entity_id) == trip.vehicle:
				cab = candidate
		if stop and cab:
			for i in range(trip.party.size()):
				var entry := stop.exit_point(cab, i)
				var door := _car_door(stop, cab)
				var p := stop.waiting_point(i).lerp(door, trip.progress) if trip.phase == "boarding" else door.lerp(entry, trip.progress)
				_show_person(trip.party[i], p, false, signf(door.x - stop.waiting_point(i).x) if trip.phase == "boarding" else signf(entry.x - door.x), dt, used)
	for departure: Dictionary in departures.duplicate():
		var stop: TaxiStop = network.stops.get(departure.stop)
		if stop:
			var distance := 0.0
			for start: Vector3 in departure.starts:
				distance = maxf(distance, start.distance_to(stop.door_point()))
			departure.progress = minf(1, departure.progress + _walk_step(distance, dt))
			for i in range(departure.party.size()):
				var p: Vector3 = departure.starts[i].lerp(stop.door_point(), departure.progress)
				_show_person(departure.party[i], p, false, signf(stop.door_point().x - p.x), dt, used)
		if departure.progress >= 1:
			departures.erase(departure)
	_shown_last_frame = used

func _show_person(data: Dictionary, p: Vector3, waving: bool, direction: float, dt: float, used: Dictionary) -> void:
	var slot: int = _assigned.get(data.id, -1)
	if slot == -1:
		for i in range(_pool.size()):
			if not _assigned.values().has(i):
				slot = i
				_assigned[data.id] = i
				# Swap a cached mesh on the common skeleton. A rare coat variant
				# cannot exhaust a dedicated sub-pool and make people invisible.
				_pool[i].get_node("Skeleton3D/Body").mesh = _appearances[int(data.look)]
				break
	if slot >= 0:
		used[data.id] = true
		var person := _pool[slot] as PassengerVisual
		var travel := Vector3.ZERO
		var continuous := _shown_last_frame.has(data.id) and dt > 0
		if continuous:
			travel = p - person.global_position
		else:
			# A pool reuse, load or emergence from a car is not a walking step.
			person.reset_gait()
		person.show()
		person.global_position = p
		if not continuous:
			person.reset_physics_interpolation()
		person.set_meta("actor_id", data.id)
		person.pose(dt, travel, waving, direction)
