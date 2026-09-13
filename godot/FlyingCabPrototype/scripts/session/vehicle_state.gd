class_name VehicleState
extends Resource
## Serializable identity and condition, independent of its current controller.
@export var entity_id: StringName = &"ari_cab"
@export var model_id: StringName = &"basic_cab"
@export var owner_id: StringName = &"ari"
@export var driver_id: StringName = &""
@export var passenger_ids: PackedStringArray = []
var reservations: Dictionary = {}
@export var map_id: StringName = &"city_02"
@export var fuel := 100.0
@export var condition := 1.0
var position := Vector3.ZERO
var velocity := Vector3.ZERO
var has_pose := false

func snapshot() -> Dictionary:
	return {"id": String(entity_id), "model": String(model_id), "owner": String(owner_id), "driver": String(driver_id), "passengers": Array(passenger_ids), "reservations": reservations.duplicate(true), "map": String(map_id), "fuel": fuel, "condition": condition, "position": [position.x, position.y, position.z], "velocity": [velocity.x, velocity.y, velocity.z], "has_pose": has_pose}

static func from_snapshot(data: Dictionary) -> VehicleState:
	if not data.get("id") is String or data.id.is_empty() or not data.get("owner") is String or not data.get("driver") is String or not data.get("map") is String:
		return null
	for field in ["fuel", "condition"]:
		if not (data.get(field) is float or data.get(field) is int) or not is_finite(float(data[field])):
			return null
	if data.fuel < 0 or data.condition < 0 or data.condition > 1:
		return null
	for field in ["position", "velocity"]:
		if not data.get(field) is Array or data[field].size() != 3:
			return null
		for value in data[field]:
			if not (value is float or value is int) or not is_finite(float(value)):
				return null
	if not data.get("passengers") is Array or not data.get("has_pose") is bool:
		return null
	var occupants := {}
	for passenger in data.passengers:
		if not passenger is String or passenger.is_empty() or occupants.has(passenger) or passenger == data.driver:
			return null
		occupants[passenger] = true
	if not data.get("model", "basic_cab") is String or data.get("model", "basic_cab").is_empty():
		return null
	var saved_reservations = data.get("reservations", {})
	if not saved_reservations is Dictionary:
		return null
	for trip in saved_reservations:
		if not trip is String or trip.is_empty() or not saved_reservations[trip] is Array or saved_reservations[trip].is_empty():
			return null
		for id in saved_reservations[trip]:
			if not id is String or id.is_empty() or occupants.has(id) or id == data.driver:
				return null
			occupants[id] = true
	var state := VehicleState.new()
	state.reservations = saved_reservations.duplicate(true)
	state.entity_id = data.id
	state.model_id = data.get("model", "basic_cab")
	state.owner_id = data.owner
	state.driver_id = data.driver
	state.map_id = data.map
	state.passenger_ids = PackedStringArray(data.passengers)
	state.fuel = data.fuel
	state.condition = data.condition
	state.position = Vector3(data.position[0], data.position[1], data.position[2])
	state.velocity = Vector3(data.velocity[0], data.velocity[1], data.velocity[2])
	state.has_pose = data.has_pose
	return state
