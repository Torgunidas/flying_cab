class_name VehicleOccupancy
extends RefCounted
## All drivers use the same atomic seat reservation and boarding rules.
static func reserved_count(state: VehicleState) -> int:
	var count := 0
	for party in state.reservations.values():
		count += party.size()
	return count

static func reserve(state: VehicleState, model: VehicleDefinition, trip: String, ids: Array) -> bool:
	if trip.is_empty() or ids.is_empty() or state.condition <= 0 or state.reservations.has(trip):
		return false
	if state.passenger_ids.size() + reserved_count(state) + ids.size() > model.max_passengers:
		return false
	var used := Array(state.passenger_ids)
	used.append(String(state.driver_id))
	for party in state.reservations.values():
		used.append_array(party)
	for id in ids:
		if not id is String or id.is_empty() or used.has(id):
			return false
		used.append(id)
	state.reservations[trip] = ids.duplicate()
	return true

static func commit(state: VehicleState, model: VehicleDefinition, trip: String) -> bool:
	if not state.reservations.has(trip) or state.condition <= 0:
		return false
	var ids: Array = state.reservations[trip]
	if state.passenger_ids.size() + reserved_count(state) > model.max_passengers:
		return false
	for id in ids:
		if id == String(state.driver_id) or state.passenger_ids.has(id):
			return false
	state.passenger_ids.append_array(PackedStringArray(ids))
	state.reservations.erase(trip)
	return true

static func release(state: VehicleState, trip: String) -> void:
	state.reservations.erase(trip)
