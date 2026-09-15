class_name LivingWorldState
extends RefCounted
## Persistent identities and plans. All scene nodes and steering live on the map.
var agents: Dictionary = {}
var initialized_maps := PackedStringArray()

func snapshot() -> Dictionary:
	return {"maps": Array(initialized_maps), "agents": agents.duplicate(true)}

static func from_snapshot(data: Dictionary, vehicles: Dictionary) -> LivingWorldState:
	if not data.get("maps", []) is Array or not data.get("agents", {}) is Dictionary or data.get("agents", {}).size() > 512:
		return null
	var result := LivingWorldState.new()
	for map in data.get("maps", []):
		if not map is String or map.is_empty() or result.initialized_maps.has(map):
			return null
		result.initialized_maps.append(map)
	for id in data.get("agents", {}):
		var item = data.agents[id]
		if not id is String or id.is_empty() or not item is Dictionary:
			return null
		for key in ["kind", "map", "vehicle", "route", "phase"]:
			if not item.get(key) is String:
				return null
		if item.kind not in ["loop", "resident", "parked", "walker"] or item.phase not in ["flying", "parked", "boarding", "alighting", "inside", "walking", "stranded"] or not result.initialized_maps.has(item.map) or not item.get("claimed") is bool:
			return null
		for key in ["waypoint", "stop", "laps", "look"]:
			var value = item.get(key)
			if not (value is int or value is float) or not is_finite(float(value)) or value < 0 or value > 1000000 or float(value) != floorf(float(value)):
				return null
		if item.look > 3 or item.waypoint > 4096 or item.stop > 100:
			return null
		if not (item.get("timer") is int or item.get("timer") is float) or not is_finite(float(item.timer)) or item.timer < 0 or item.timer > 1000000:
			return null
		for key in ["person", "target"]:
			if not item.get(key) is Array or item[key].size() != 3:
				return null
			for value in item[key]:
				if not (value is int or value is float) or not is_finite(float(value)):
					return null
			if not is_equal_approx(float(item[key][2]), WorldLayers.PEDESTRIAN_Z):
				return null
		if item.kind == "walker":
			if not item.vehicle.is_empty() or item.claimed or item.route.is_empty() or not id.begins_with("walker/" + item.route + "/") or item.phase not in ["walking", "parked", "inside"]:
				return null
		else:
			if item.vehicle != id or not vehicles.has(StringName(item.vehicle)) or vehicles[StringName(item.vehicle)].map_id != StringName(item.map):
				return null
			var driver: StringName = vehicles[StringName(item.vehicle)].driver_id
			if not item.claimed and driver not in [&"", StringName(id)]:
				return null
		result.agents[id] = item.duplicate(true)
	return result

func valid_plans(profiles: Array[LivingWorldProfile]) -> bool:
	var maps := {}
	for profile in profiles:
		maps[profile.map_id] = profile
	for map_id in initialized_maps:
		if not maps.has(map_id):
			return false
	for id: String in agents:
		var item: Dictionary = agents[id]
		var profile: LivingWorldProfile = maps[item.map]
		if item.kind == "loop":
			var found := false
			for route in profile.routes:
				if route.route_id == item.route:
					found = item.waypoint < route.points.size() and id.begins_with("traffic/" + item.route + "/") and item.phase == "flying"
			if not found:
				return false
		elif item.kind in ["resident", "parked"]:
			var found := false
			for schedule in profile.residents:
				if schedule.resident_id == item.route:
					found = id == "resident/" + item.route and item.stop < schedule.stops.size() and item.waypoint < 5 and (item.kind == "parked") == schedule.parked_only
			if not found:
				return false
			if item.claimed or item.kind == "parked":
				if item.phase not in ["stranded", "walking", "parked", "inside"]:
					return false
			elif item.phase not in ["inside", "boarding", "flying", "alighting"]:
				return false
	return true
