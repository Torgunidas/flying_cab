class_name RuntimeContext
extends Node
## Session lifetime: maps and presentation may be replaced without resetting this.
signal system_registered(id: StringName)
signal player_damaged(amount: float)
var player := PlayerSession.new()
var player_state := PlayerState.new()
var campaign := CampaignState.new()
var ledger := EconomyLedger.new()
var rides := RideService.new()
var living := LivingWorldState.new()
var world := WorldRegistry.new()
var vehicles: Dictionary = {}
var map_states: Dictionary = {}
var current_map: StringName = &"city_02"
var systems: Dictionary = {}
var narrative := NarrativeService.new()
var dialogue := DialogueSession.new()
var _narrative_elapsed := 0.0
@export var narrative_catalog: NarrativeCatalog = preload("res://resources/narrative/city_catalog.tres")
var _focused := true
@export var vehicle_catalog: VehicleCatalog = preload("res://resources/vehicle_catalog.tres")
@export var living_profiles: Array[LivingWorldProfile] = [preload("res://resources/living_world/city_02.tres")]

func _init() -> void:
	# Demo allowance, not campaign balance. Restoring a save replaces this wallet.
	campaign.credits = 250.0
	ledger.state = campaign
	register_system(&"economy", ledger)
	register_system(&"rides", rides)
	register_system(&"living_world", living)
	register_system(&"repair", VehicleService.new())
	narrative.context = self
	narrative.catalog = narrative_catalog
	register_system(&"narrative", narrative)
	register_system(&"dialogue", dialogue)
	ledger.credited.connect(_credited)
	rides.journey_event.connect(_journey_event)
	(systems[&"repair"] as VehicleService).repaired.connect(_repaired)
	campaign.expired.connect(_campaign_expired)

func _ready() -> void:
	var errors := narrative_catalog.validation_errors()
	for error in errors:
		push_error(error)
	if not errors.is_empty():
		narrative.catalog = null
	else:
		narrative.catalog = narrative_catalog

func _process(dt: float) -> void:
	# An authored choice starts the campaign. Pauses and focus loss never catch up
	# using wall time on resume.
	if _focused and not player.is_suspended():
		campaign.advance(dt)
		_narrative_elapsed += dt
		if _narrative_elapsed >= 0.2:
			_narrative_elapsed = 0.0
			narrative.refresh_state()

func _credited(id: String, amount: float, source: String) -> void:
	narrative.record_event("credits_earned", {"amount": amount, "target": source}, "income/" + id)

func _journey_event(event: String, payload: Dictionary, receipt: String) -> void:
	narrative.record_event(event, payload, receipt)

func _repaired(id: StringName, restored: float, _cost: float) -> void:
	narrative.record_event("vehicle_repaired", {"amount": restored, "vehicle": String(id)})

func _campaign_expired() -> void:
	campaign.flags["__campaign_expired"] = true
	dialogue.end()
	player.suspend(&"campaign_over")
	narrative.changed.emit()
	narrative.checkpoint_requested.emit(true)

func damage_player(amount: float) -> float:
	if not is_finite(amount) or amount <= 0 or player_state.health <= 0:
		return 0.0
	var removed := minf(amount, player_state.health)
	player_state.health -= removed
	if player_state.health <= 0:
		dialogue.end()
		player.suspend(&"player_dead")
	player_damaged.emit(removed)
	narrative.changed.emit()
	narrative.checkpoint_requested.emit(true)
	return removed

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

func capture_player_state() -> void:
	if player.focus is WalkingActor:
		player_state.mode = "on_foot"
		player_state.map_id = current_map
		player_state.position = player.focus.global_position
		player_state.velocity = player.focus.velocity
		player_state.facing = player.focus.facing
	elif player.focus is FlightCab:
		player_state.mode = "flight"
		player_state.map_id = current_map
		player_state.vehicle_id = player.focus.entity_id

func snapshot() -> Dictionary:
	capture_player_state()
	var level := world.map_root()
	if level and level.has_method("capture_map_state"):
		map_states[String(current_map)] = level.capture_map_state()
	for actor in world.vehicles:
		if is_instance_valid(actor) and actor.has_method("capture_state"):
			actor.capture_state()
	var saved: Array = []
	for state: VehicleState in vehicles.values():
		saved.append(state.snapshot())
	return {"schema_version": 5, "narrative": narrative.snapshot(), "living_world": living.snapshot(), "player": player_state.snapshot(), "rides": rides.snapshot(), "map": String(current_map), "campaign": campaign.snapshot(), "vehicles": saved, "maps": map_states.duplicate(true)}

func restore(data: Dictionary) -> bool:
	# Validate the complete payload before committing any part of it.
	var schema = data.get("schema_version")
	# JSON numbers are floats; Array.has(int) does not accept the matching float.
	if not (schema is int or schema is float) or not is_finite(schema) or schema != floorf(schema) or schema < 1 or schema > 5:
		return false
	if not data.get("map") is String or not data.get("campaign") is Dictionary or not data.get("vehicles") is Array or not data.get("maps") is Dictionary:
		return false
	var restored_narrative: Dictionary = NarrativeService.empty_state()
	if data.schema_version >= 5:
		if not data.get("narrative") is Dictionary or not narrative.validate_snapshot(data.narrative):
			return false
		restored_narrative = data.narrative.duplicate(true)
		restored_narrative.serial = int(restored_narrative.serial)
		for id in restored_narrative.items:
			restored_narrative.items[id] = int(restored_narrative.items[id])
		for id in restored_narrative.quests:
			restored_narrative.quests[id].version = int(restored_narrative.quests[id].version)
	var restored_campaign := CampaignState.from_snapshot(data.campaign)
	if restored_campaign == null:
		return false
	for id in narrative_catalog.fact_ids:
		var value = restored_campaign.flags.get(id, 0.0)
		if not (value is int or value is float) or not is_finite(value): return false
	for id in ["__campaign_started", "__campaign_expired"]:
		if restored_campaign.flags.has(id) and not restored_campaign.flags[id] is bool: return false
	if restored_campaign.flags.get("__campaign_expired", false) and (restored_campaign.active or restored_campaign.remaining_seconds != 0): return false
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
	var restored_player := PlayerState.new()
	if data.schema_version >= 3:
		if not data.get("player") is Dictionary:
			return false
		restored_player = PlayerState.from_snapshot(data.player)
		if restored_player == null:
			return false
		if restored_player.mode == "flight" and restored_player.vehicle_id != &"" and not restored_vehicles.has(restored_player.vehicle_id):
			return false
	if not data.get("living_world", {}) is Dictionary:
		return false
	var restored_living := LivingWorldState.from_snapshot(data.get("living_world", {}), restored_vehicles)
	if restored_living == null or not restored_living.valid_plans(living_profiles):
		return false
	# Load into a detached session, then let the map router bind live actors.
	if is_instance_valid(player.focus):
		return false
	campaign = restored_campaign
	campaign.expired.connect(_campaign_expired)
	narrative.data = restored_narrative
	player_state = restored_player
	if player_state.health <= 0:
		player.suspend(&"player_dead")
	else:
		player.resume(&"player_dead")
	ledger.state = campaign
	rides = restored_rides
	rides.journey_event.connect(_journey_event)
	systems[&"rides"] = rides
	living = restored_living
	systems[&"living_world"] = living
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
	dialogue.end()
	world.unbind()
