class_name LivingWorldProfile
extends Resource
## Map content, not simulation state. Population is allocated once before warmup.
@export var map_id := "city_02"
@export var routes: Array[LivingRoute] = []
@export var residents: Array[ResidentSchedule] = []
@export_range(0, 3, 1) var walkers_per_platform := 1
@export_range(0.5, 3, 0.1) var walking_speed := 1.5
@export_range(1, 10, 0.5) var residential_speed := 5.0

## Shared junctions reserve a crossing for one vehicle at a time.
@export var junctions := PackedVector3Array()
@export_range(10, 25, 1) var junction_approach := 18.0

func validation_errors(catalog: VehicleCatalog, stops: Dictionary) -> PackedStringArray:
	var errors := PackedStringArray()
	var ids := {}
	var count := residents.size() + stops.size() * walkers_per_platform
	if map_id.is_empty() or walkers_per_platform < 0 or walkers_per_platform > 3 or not is_finite(walking_speed) or walking_speed <= 0 or not is_finite(residential_speed) or residential_speed <= 0:
		errors.append("Invalid population settings")
	for route in routes:
		if route == null or route.route_id.is_empty() or ids.has(route.route_id):
			errors.append("Missing or duplicate traffic route")
			continue
		ids[route.route_id] = true
		count += route.population
		if route.points.size() < 3 or route.points.size() > 4096 or route.models.is_empty() or route.population < 0 or route.population > 32 or not is_finite(route.speed) or route.speed <= 0:
			errors.append("Invalid traffic route: " + route.route_id)
		for i in range(route.points.size()):
			var point := route.points[i]
			if not point.is_finite() or not is_zero_approx(point.z) or point.distance_to(route.points[(i + 1) % route.points.size()]) < 0.1:
				errors.append("Invalid traffic waypoint: " + route.route_id)
		for model_id in route.models:
			if catalog.find_model(model_id) == null:
				errors.append("Missing traffic model: " + model_id)
	ids.clear()
	for resident in residents:
		if resident == null or resident.resident_id.is_empty() or ids.has(resident.resident_id):
			errors.append("Missing or duplicate resident")
			continue
		ids[resident.resident_id] = true
		var model := catalog.find_model(resident.model_id)
		if model == null or resident.stops.size() < (1 if resident.parked_only else 2) or resident.stops.size() > 100 or resident.look < 0 or resident.look > 3 or not is_finite(resident.berth_x) or not is_finite(resident.corridor_x) or not is_finite(resident.dwell_seconds) or resident.dwell_seconds < 3:
			errors.append("Invalid resident schedule: " + resident.resident_id)
		for stop_id in resident.stops:
			if not stops.has(stop_id):
				errors.append("Missing resident platform: " + stop_id)
			elif model and absf(resident.berth_x + model.collision_offset.x) + model.collision_size.x * 0.5 > stops[stop_id].half_width:
				errors.append("Resident car does not fit on platform: " + stop_id)
	for point in junctions:
		if not point.is_finite() or not is_zero_approx(point.z):
			errors.append("Invalid traffic junction")
	if count > 512:
		errors.append("Population exceeds the save-state capacity")
	return errors
