class_name TaxiRules
extends Resource
## Shared, Inspector-editable economy and dispatch tuning. No per-car fares.
@export var stop_ids := PackedStringArray(["depot", "velvet_neon", "velvet_club", "velvet_03", "eden_01", "eden_02", "eden_03", "velvet_04", "velvet_05", "eden_04", "eden_05", "velvet_06", "eden_06", "foundry_market", "foundry_docks", "foundry_torque", "aurelia_01", "aurelia_02", "aurelia_03", "foundry_04", "foundry_05", "aurelia_04", "aurelia_05", "foundry_06", "aurelia_06"])
@export var starting_credits := 120.0
@export var base_fare := 24.0
@export var fare_per_meter := 0.8
@export var extra_passenger_multiplier := 0.35
@export var fuel_price := 1.0
@export var fuel_per_second := 15.0
@export var tow_fee := 35.0
@export var recovery_fuel := 35.0
@export var recovery_condition := 0.5
@export var max_offers := 25
@export_range(0, 0.5) var lifetime_jitter := 0.25
@export var offer_seconds := 150.0
@export var walking_speed := 1.6
@export var population_seed := 1977

func quote(distance: float, count: int) -> float:
	return snappedf((base_fare + maxf(distance, 0.0) * fare_per_meter) * (1.0 + maxf(count - 1, 0) * extra_passenger_multiplier), 0.01)

func validation_errors() -> PackedStringArray:
	var errors := PackedStringArray()
	for field in ["starting_credits", "base_fare", "fare_per_meter", "extra_passenger_multiplier", "fuel_price", "tow_fee", "recovery_fuel", "recovery_condition"]:
		var value: float = get(field)
		if not is_finite(value) or value < 0:
			errors.append(field)
	for field in ["fuel_per_second", "offer_seconds", "walking_speed"]:
		var value: float = get(field)
		if not is_finite(value) or value <= 0:
			errors.append(field)
	if max_offers < 1 or max_offers > stop_ids.size() or recovery_condition > 1:
		errors.append("offer limit / recovery condition")
	if not is_finite(lifetime_jitter) or lifetime_jitter < 0 or lifetime_jitter > 0.5:
		errors.append("lifetime jitter")
	return errors
