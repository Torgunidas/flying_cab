class_name RuntimeContext
extends Node
## Session lifetime: maps and presentation may be replaced without resetting this.
signal system_registered(id: StringName)
var player := PlayerSession.new()
var campaign := CampaignState.new()
var ledger := EconomyLedger.new()
var rides := RideService.new()
var world := WorldRegistry.new()
var vehicles: Dictionary = {}
var map_states: Dictionary = {}
var current_map: StringName = &"city_02"
var systems: Dictionary = {}
var _focused := true
@export var vehicle_catalog: VehicleCatalog = preload("res://resources/vehicle_catalog.tres")

func _init() -> void:
	# Demo allowance, not campaign balance. Restoring a save replaces this wallet.
	campaign.credits = 250.0
	ledger.state = campaign
	register_system(&"economy", ledger)
	register_system(&"rides", rides)
	register_system(&"repair", VehicleService.new())

func _process(dt: float) -> void:
	# Campaign is disabled in the flight lab. The initial active-time policy pauses
	# while control is suspended or the application is unfocused; never catch up
	# using wall time on resume. Authored dialogue/pause policy can replace this.
	if _focused and not player.is_suspended():
		campaign.advance(dt)

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		_focused = false
		player.suspend(&"application_focus")
	elif what == NOTIFICATION_APPLICATION_FOCUS_IN:
		_focused = true
		player.resume(&"application_focus")

func register_system(id: StringName, system: Object) -> bool:
	if id == &"" or not is_instance_valid(system) or systems.has(id):
		return false
	systems[id] = system
	system_registered.emit(id)
	return true

func bind_vehicle(vehicle: Node) -> void:
	var id: StringName = vehicle.state.entity_id
	if vehicles.has(id):
		var model_id: StringName = vehicles[id].model_id
		if vehicle.definition.model_id != model_id:
			var model := vehicle_catalog.find_model(model_id)
			if model == null:
				push_error("Cannot restore unknown vehicle model: " + String(model_id))
				return
			vehicle.apply_model(model)
		vehicle.restore_state(vehicles[id])
	else:
		vehicles[id] = vehicle.state

func snapshot() -> Dictionary:
	var level := world.map_root()
	if level and level.has_method("capture_map_state"):
		map_states[String(current_map)] = level.capture_map_state()
	for actor in world.vehicles:
		if is_instance_valid(actor) and actor.has_method("capture_state"):
			actor.capture_state()
	var saved: Array = []
	for state: VehicleState in vehicles.values():
		saved.append(state.snapshot())
	return {"schema_version": 2, "rides": rides.snapshot(), "map": String(current_map), "campaign": campaign.snapshot(), "vehicles": saved, "maps": map_states.duplicate(true)}

func restore(data: Dictionary) -> bool:
	# Validate the complete payload before committing any part of it.
	if (data.get("schema_version") != 1 and data.get("schema_version") != 2) or not data.get("map") is String or not data.get("campaign") is Dictionary or not data.get("vehicles") is Array or not data.get("maps") is Dictionary:
		return false
	var restored_campaign := CampaignState.from_snapshot(data.campaign)
	if restored_campaign == null:
		return false
	var restored_vehicles := {}
	for item in data.vehicles:
		if not item is Dictionary:
			return false
		var state := VehicleState.from_snapshot(item)
		if state == null or restored_vehicles.has(state.entity_id):
			return false
		var model := vehicle_catalog.find_model(state.model_id)
		if model == null or not model.validation_errors().is_empty() or state.passenger_ids.size() + VehicleOccupancy.reserved_count(state) > model.max_passengers or state.fuel > model.fuel_capacity:
			return false
		restored_vehicles[state.entity_id] = state
	if not data.get("rides", {}) is Dictionary:
		return false
	var restored_rides := RideService.from_snapshot(data.get("rides", {}), restored_vehicles, restored_campaign.receipts)
	if restored_rides == null:
		return false
	# Load into a detached session, then let the map router bind live actors.
	if is_instance_valid(player.focus):
		return false
	campaign = restored_campaign
	ledger.state = campaign
	rides = restored_rides
	systems[&"rides"] = rides
	vehicles = restored_vehicles
	current_map = data.map
	map_states = data.maps.duplicate(true)
	return true

func save_to(path: String) -> Error:
	var temporary := path + ".tmp"
	var file := FileAccess.open(temporary, FileAccess.WRITE)
	if file == null:
		return FileAccess.get_open_error()
	file.store_string(JSON.stringify(snapshot()))
	file.flush()
	var result := file.get_error()
	file.close()
	if result != OK:
		return result
	return DirAccess.rename_absolute(temporary, path)

func load_from(path: String) -> bool:
	if not FileAccess.file_exists(path):
		return false
	var parsed = JSON.parse_string(FileAccess.get_file_as_string(path))
	return parsed is Dictionary and restore(parsed)

func _exit_tree() -> void:
	world.unbind()
