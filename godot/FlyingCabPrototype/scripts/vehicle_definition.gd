class_name VehicleDefinition
extends Resource
## Shared, authored model data. Runtime fuel, hull and occupants live in VehicleState.

@export_group("Model")
@export var model_id: StringName = &"basic_cab"
@export var display_name := "Ari / Cab"
@export_group("Body")
@export var visual_scene: PackedScene
## Authoring metadata: stationary silhouette relative to the original cab.
## Length 1 = 2.2 m; height 1 = 1.14 m, excluding exhaust flames.
@export_storage var size_units := Vector2.ONE
@export var collision_size := Vector3(2.2, 0.7, 0.9)
@export var collision_offset := Vector3.ZERO
@export var headlight_origin := Vector3(1.12, 0.08, 0)
## Driver's door along the visual's X axis; mirrors with the car's facing.
@export var driver_door_x := 0.25
@export_group("Physics — kg and newtons")
@export_range(1.0, 10000.0, 1.0, "or_greater", "suffix:kg") var mass_kg := 100.0
@export_range(1.0, 100000.0, 1.0, "or_greater", "suffix:N") var thrust_force := 2350.0
@export_range(1.0, 100000.0, 1.0, "or_greater", "suffix:N") var horizontal_thrust_force := 1400.0
var vertical_acceleration: float:
	get: return thrust_force / maxf(1.0, mass_kg)
var horizontal_acceleration: float:
	get: return horizontal_thrust_force / maxf(1.0, mass_kg)
@export_group("Engine response — seconds")
## Time from zero to full power on each axis. Zero restores instant response.
@export_range(0.0, 1.0, 0.01, "or_greater", "suffix:s") var thrust_rise_seconds := 0.18
## Time from full power to zero after release; reversal passes through zero.
@export_range(0.0, 1.0, 0.01, "or_greater", "suffix:s") var thrust_release_seconds := 0.12
@export_group("Navigation clearance — metres")
@export var navigation_clearance := Vector3(2.5, 0.85, 1.1)
@export_group("Hull and passengers")
@export_range(1.0, 10000.0, 1.0, "or_greater", "suffix:HP") var max_hull := 100.0
## Fraction of incoming damage absorbed, independently of total health.
@export_range(0.0, 0.95, 0.01) var damage_resistance := 0.0
## Passenger seats EXCLUDING the driver. Boarding is validated by the actor API.
@export_range(0, 32, 1) var max_passengers := 3
@export_group("Collision damage")
@export var collision_damage_enabled := true
@export_range(0.0, 100.0, 0.1, "suffix:m/s") var safe_impact_speed := 7.0
## HP per squared excess normal speed for a 100 kg vehicle. Independent of max_hull.
@export_range(0.0, 10.0, 0.05, "or_greater") var collision_damage_factor := 0.7
@export_range(0.0, 1.0, 0.01, "suffix:s") var collision_cooldown := 0.15
@export_group("Flight — metres / seconds")
@export var gravity: float = 9.8
@export var max_climb_speed: float = 11.5
@export var max_fall_speed: float = 13.0
@export var max_horizontal_speed: float = 10.5
@export var horizontal_coast_damping: float = 1.7
@export var upward_coast_damping: float = 1.5
@export var linear_damping: float = 0.05

@export var airspace_enabled := true

@export_group("Fuel — prototype units")
@export var fuel_enabled := true
@export_range(1.0, 10000.0, 1.0, "or_greater") var fuel_capacity: float = 100.0
## Initial amount only; fuel is subsequently owned by each vehicle instance.
@export_range(0.0, 10000.0, 1.0, "or_greater") var starting_fuel := 100.0
@export_range(0.0, 100.0, 0.1, "or_greater", "suffix:/s") var fuel_vertical_rate: float = 1.4
@export_range(0.0, 100.0, 0.1, "or_greater", "suffix:/s") var fuel_horizontal_rate: float = 0.7
@export var ceiling_fuel_multiplier: float = 3.0
@export var refuel_rate: float = 25.0

@export_group("Highway assist — Unreal reference")
@export var highway_enabled := true
@export var highway_entry_seconds: float = 0.5
@export var highway_exit_seconds: float = 0.8

func validation_errors() -> PackedStringArray:
	var errors := PackedStringArray()
	if model_id == &"" or display_name.is_empty():
		errors.append("Model ID and display name must not be empty")
	for field in ["mass_kg", "thrust_force", "horizontal_thrust_force", "max_hull", "fuel_capacity", "max_climb_speed", "max_fall_speed", "max_horizontal_speed"]:
		var value: float = get(field)
		if not is_finite(value) or value <= 0.0:
			errors.append(field + " must be finite and positive")
	for field in ["thrust_rise_seconds", "thrust_release_seconds", "gravity", "horizontal_coast_damping", "upward_coast_damping", "linear_damping", "safe_impact_speed", "collision_damage_factor", "collision_cooldown", "starting_fuel", "fuel_vertical_rate", "fuel_horizontal_rate", "ceiling_fuel_multiplier", "refuel_rate", "highway_entry_seconds", "highway_exit_seconds"]:
		var value: float = get(field)
		if not is_finite(value) or value < 0.0:
			errors.append(field + " must be finite and non-negative")
	if not navigation_clearance.is_finite() or navigation_clearance.x <= 0 or navigation_clearance.y <= 0 or navigation_clearance.z <= 0:
		errors.append("Navigation clearance must be finite and positive")
	if max_passengers < 0 or max_passengers > 32:
		errors.append("Passenger capacity must be between 0 and 32")
	if not is_finite(damage_resistance) or damage_resistance < 0 or damage_resistance > 0.95:
		errors.append("Damage resistance must be between 0 and 0.95")
	if not size_units.is_finite() or size_units.x <= 0 or size_units.y <= 0:
		errors.append("Body size units must be finite and positive")
	if not is_finite(driver_door_x) or absf(driver_door_x) > collision_size.x * 0.5:
		errors.append("Driver door must be within the vehicle body")
	if not collision_size.is_finite() or collision_size.x <= 0 or collision_size.y <= 0 or collision_size.z <= 0 or not collision_offset.is_finite() or not headlight_origin.is_finite():
		errors.append("Body collision and light geometry must be finite and valid")
	return errors
