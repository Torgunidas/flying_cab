class_name RideService
extends RefCounted
## Session-owned journeys. Nodes, animation and the current pilot own no fares.
signal changed
signal journey_event(event: String, payload: Dictionary, receipt: String)
var enabled := false
var offers: Array = []
var active: Dictionary = {}
var selected := ""
var serial := 0
var completed := 0
var income := 0.0
var service_spent := 0.0
var recovery_debt := 0.0

func create_offer(origin: String, destination: String, fare: float, count: int, lifetime: float) -> Dictionary:
	if origin == destination or origin.is_empty() or destination.is_empty() or not is_finite(fare) or fare < 0 or count < 1 or count > 6 or not is_finite(lifetime) or lifetime <= 0:
		return {}
	for waiting: Dictionary in offers:
		if waiting.origin == origin:
			return {}
	serial += 1
	var id := "ride_%d" % serial
	var party: Array = []
	for i in range(count):
		party.append({"id": "%s/person_%d" % [id, i], "look": (serial + i) % 4})
	var offer := {"id": id, "origin": origin, "destination": destination, "map": "city_02", "fare": fare, "remaining": lifetime, "party": party, "walk": 0.0, "phase": "waiting", "vehicle": "", "progress": 0.0, "duration": 1.0}
	offers.append(offer)
	changed.emit()
	return offer

func find_offer(id: String) -> Dictionary:
	for offer: Dictionary in offers:
		if offer.id == id:
			return offer
	return {}

func select(id: String) -> bool:
	if not active.is_empty() or find_offer(id).is_empty():
		return false
	selected = id
	changed.emit()
	return true

func party_ids(ride: Dictionary) -> Array:
	var ids: Array = []
	for person: Dictionary in ride.party:
		ids.append(person.id)
	return ids

func begin_boarding(vehicle: VehicleState, model: VehicleDefinition) -> bool:
	var offer := find_offer(selected)
	if not active.is_empty() or offer.is_empty() or offer.walk < 1.0 or offer.get("return_remaining", 0.0) > 0 or vehicle.map_id != StringName(offer.map):
		return false
	if not VehicleOccupancy.reserve(vehicle, model, offer.id, party_ids(offer)):
		return false
	active = offer
	offers.erase(offer)
	active.vehicle = String(vehicle.entity_id)
	active.phase = "boarding"
	active.progress = 0.0
	changed.emit()
	return true

func interrupt_boarding(vehicle: VehicleState) -> void:
	if active.is_empty() or active.phase != "boarding":
		return
	VehicleOccupancy.release(vehicle, active.id)
	active.phase = "waiting"
	active.vehicle = ""
	active.progress = 0.0
	offers.append(active)
	active = {}
	changed.emit()

func board(vehicle: VehicleState, model: VehicleDefinition) -> bool:
	if active.is_empty() or active.phase != "boarding" or active.vehicle != String(vehicle.entity_id) or not VehicleOccupancy.commit(vehicle, model, active.id):
		return false
	active.phase = "riding"
	active.progress = 0.0
	changed.emit()
	journey_event.emit("passenger_boarded", {"origin": active.origin, "destination": active.destination, "vehicle": active.vehicle}, "board/" + active.id)
	return true

func complete(vehicle: VehicleState, ledger: EconomyLedger) -> bool:
	if active.is_empty() or active.phase != "alighting" or active.vehicle != String(vehicle.entity_id):
		return false
	for id in party_ids(active):
		if not vehicle.passenger_ids.has(id):
			return false
	# The receipt, wallet and manifest mutate synchronously; snapshots see all or none.
	if not ledger.credit_once(active.id, active.fare, false):
		return false
	var completed_trip := active.duplicate(true)
	for id in party_ids(active):
		vehicle.passenger_ids.erase(id)
	completed += 1
	income += active.fare
	# Recovery credit is interest-free and repaid only from new fare income.
	var repayment := minf(recovery_debt, active.fare * 0.25)
	ledger.spend(repayment)
	recovery_debt -= repayment
	active = {}
	selected = ""
	ledger.changed.emit()
	if completed_trip.fare > 0:
		ledger.credited.emit(completed_trip.id, completed_trip.fare, "fare")
	changed.emit()
	journey_event.emit("ride_completed", {"origin": completed_trip.origin, "destination": completed_trip.destination, "vehicle": completed_trip.vehicle}, "delivery/" + completed_trip.id)
	return true

func cancel(vehicles: Dictionary) -> void:
	if not active.is_empty():
		var vehicle: VehicleState = vehicles.get(StringName(active.vehicle))
		if vehicle:
			VehicleOccupancy.release(vehicle, active.id)
			for id in party_ids(active):
				vehicle.passenger_ids.erase(id)
	active = {}
	selected = ""
	changed.emit()

func snapshot() -> Dictionary:
	return {"enabled": enabled, "offers": offers.duplicate(true), "active": active.duplicate(true), "selected": selected, "serial": serial, "completed": completed, "income": income, "service_spent": service_spent, "recovery_debt": recovery_debt}

static func from_snapshot(data: Dictionary, vehicles: Dictionary, receipts: Dictionary) -> RideService:
	var result := RideService.new()
	if data.is_empty():
		return result
	if not data.get("enabled") is bool or not data.get("offers") is Array or not data.get("active") is Dictionary or not data.get("selected") is String:
		return null
	for key in ["serial", "completed", "income", "service_spent", "recovery_debt"]:
		if not (data.get(key) is int or data.get(key) is float) or not is_finite(data[key]) or data[key] < 0:
			return null
	if data.serial != floorf(data.serial) or data.completed != floorf(data.completed):
		return null
	for id in receipts:
		if id.begins_with("ride_") and int(id.trim_prefix("ride_")) > data.serial:
			return null
	var trips := {}
	var persons := {}
	var all: Array = data.offers.duplicate()
	if not data.active.is_empty():
		all.append(data.active)
	# Read scripted resource defaults at runtime. During a fresh editor scan,
	# folding preload(...).stop_ids.has(...) can see a placeholder null value.
	var rules := load("res://resources/taxi/default_rules.tres") as TaxiRules
	if rules == null:
		return null
	var allowed_stops: PackedStringArray = rules.stop_ids
	if data.offers.size() > allowed_stops.size():
		return null
	var waiting_origins := {}
	for ride in all:
		if not ride is Dictionary:
			return null
		for key in ["id", "origin", "destination", "map", "phase", "vehicle"]:
			if not ride.get(key) is String or (key != "vehicle" and ride[key].is_empty()):
				return null
		if ride.map != "city_02" or not allowed_stops.has(ride.origin) or not allowed_stops.has(ride.destination):
			return null
		if ride.origin == ride.destination or trips.has(ride.id) or receipts.has(ride.id) or not ride.get("party") is Array or ride.party.is_empty() or ride.party.size() > 6:
			return null
		if not ride.id.begins_with("ride_") or not ride.id.trim_prefix("ride_").is_valid_int() or int(ride.id.trim_prefix("ride_")) < 1 or int(ride.id.trim_prefix("ride_")) > data.serial:
			return null
		trips[ride.id] = true
		for key in ["fare", "remaining", "walk", "progress", "duration"]:
			if not (ride.get(key) is int or ride.get(key) is float) or not is_finite(ride[key]) or ride[key] < 0:
				return null
		if ride.walk > 1 or ride.progress > 1 or ride.duration <= 0:
			return null
		if ride.has("return_points") != ride.has("return_remaining"):
			return null
		if ride.has("return_points"):
			if not ride.return_points is Array or ride.return_points.size() != ride.party.size() or not (ride.get("return_remaining") is int or ride.get("return_remaining") is float) or not is_finite(ride.return_remaining) or ride.return_remaining < 0 or ride.return_remaining > 1:
				return null
			for point in ride.return_points:
				if not point is Array or point.size() != 3:
					return null
				for coordinate in point:
					if not (coordinate is float or coordinate is int) or not is_finite(coordinate):
						return null
		var ids: Array = []
		for person in ride.party:
			if not person is Dictionary or not person.get("id") is String or person.id.is_empty() or persons.has(person.id) or not (person.get("look") is int or person.get("look") is float) or person.look < 0 or person.look > 3 or person.look != floorf(person.look):
				return null
			persons[person.id] = true
			ids.append(person.id)
		if ride == data.active:
			var vehicle: VehicleState = vehicles.get(StringName(ride.vehicle))
			if vehicle == null or ride.phase not in ["boarding", "riding", "alighting"] or String(vehicle.map_id) != ride.map:
				return null
			if ride.phase == "boarding":
				if vehicle.reservations.get(ride.id) != ids:
					return null
			else:
				for id in ids:
					if not vehicle.passenger_ids.has(id):
						return null
		else:
			if waiting_origins.has(ride.origin):
				return null
			waiting_origins[ride.origin] = true
			if ride.phase != "waiting" or not ride.vehicle.is_empty():
				return null
	if not data.selected.is_empty() and not trips.has(data.selected):
		return null
	result.enabled = data.enabled
	result.offers = data.offers.duplicate(true)
	result.active = data.active.duplicate(true)
	result.selected = data.selected
	result.serial = int(data.serial)
	result.completed = int(data.completed)
	result.income = data.income
	result.service_spent = data.service_spent
	result.recovery_debt = data.recovery_debt
	return result
