class_name PlayerState
extends RefCounted
## Serializable player location, separate from the car and the character node.
var mode := "flight"
var vehicle_id: StringName = &""
var map_id: StringName = &"city_02"
var position := Vector3.ZERO
var velocity := Vector3.ZERO
var exit_position := Vector3.ZERO
var facing := 1.0

func snapshot() -> Dictionary:
	return {"mode": mode, "vehicle": String(vehicle_id), "map": String(map_id), "position": _array(position), "velocity": _array(velocity), "exit_position": _array(exit_position), "facing": facing}

static func _array(v: Vector3) -> Array:
	return [v.x, v.y, v.z]

static func from_snapshot(data: Dictionary) -> PlayerState:
	if data.get("mode") not in ["flight", "on_foot"] or not data.get("vehicle") is String or not data.get("map") is String or data.map.is_empty():
		return null
	for field in ["position", "velocity", "exit_position"]:
		if not data.get(field) is Array or data[field].size() != 3:
			return null
		for value in data[field]:
			if not (value is float or value is int) or not is_finite(float(value)):
				return null
		if field == "velocity" and not is_zero_approx(float(data[field][2])):
			return null
		# Legacy saves put Ari on the flight plane. New saves use the pedestrian strip.
		if field != "velocity" and not (is_zero_approx(float(data[field][2])) or is_equal_approx(float(data[field][2]), WorldLayers.PEDESTRIAN_Z)):
			return null
	if not (data.get("facing") is float or data.get("facing") is int) or absf(float(data.facing)) != 1.0:
		return null
	var state := PlayerState.new()
	state.mode = data.mode
	state.vehicle_id = data.vehicle
	state.map_id = data.map
	state.position = Vector3(data.position[0], data.position[1], WorldLayers.PEDESTRIAN_Z)
	state.velocity = Vector3(data.velocity[0], data.velocity[1], 0)
	state.exit_position = Vector3(data.exit_position[0], data.exit_position[1], WorldLayers.PEDESTRIAN_Z)
	state.facing = data.facing
	return state
