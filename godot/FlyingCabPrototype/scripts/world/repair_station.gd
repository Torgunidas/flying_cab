class_name RepairStation
extends Area3D
## Place as a child of the actual landing-pad body. Overlap alone is insufficient.
@export var station_id: StringName = &"workshop"
@export var service_name := "WARSZTAT"
@export_range(0.0, 1000.0, 0.1, "or_greater") var price_per_hull := 1.0
@export_range(0.1, 1000.0, 0.1, "or_greater") var hull_per_second := 20.0

func _ready() -> void:
	collision_mask = WorldLayers.VEHICLES
	add_to_group("repair_station")
	set_meta("entity_id", "workshop/" + String(station_id))
	set_physics_process(false)

func can_service(vehicle: FlightCab) -> bool:
	return is_instance_valid(vehicle) and overlaps_body(vehicle) and vehicle.support_body == get_parent() and vehicle.grounded and vehicle.sleeping and vehicle.command.is_zero_approx() and not vehicle.airspace.returning and not vehicle.resetting
