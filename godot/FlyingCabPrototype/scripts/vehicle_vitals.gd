class_name VehicleVitals
extends RefCounted
## Collision policy and generic damage entry point; no dependency on a controller/HUD.
signal damaged(amount: float, source: StringName)
signal disabled
var cooldown := 0.0
var flash_remaining := 0.0

func advance(dt: float) -> void:
	cooldown = maxf(0.0, cooldown - dt)
	flash_remaining = maxf(0.0, flash_remaining - dt)

func reset() -> void:
	cooldown = 0.0
	flash_remaining = 0.0

func apply_damage(state: VehicleState, model: VehicleDefinition, amount: float, source: StringName = &"external") -> float:
	if not is_finite(amount) or amount <= 0.0 or state.condition <= 0.0:
		return 0.0
	var removed := minf(amount * (1.0 - model.damage_resistance), state.condition * model.max_hull)
	state.condition = maxf(0.0, state.condition - removed / model.max_hull)
	flash_remaining = 0.25
	damaged.emit(removed, source)
	if state.condition <= 0.0:
		disabled.emit()
	return removed

func impact(state: VehicleState, model: VehicleDefinition, normal_speed_change: float) -> float:
	if not model.collision_damage_enabled or cooldown > 0.0 or not is_finite(normal_speed_change):
		return 0.0
	var excess := maxf(0.0, normal_speed_change - model.safe_impact_speed)
	var amount := excess * excess * model.collision_damage_factor * model.mass_kg / 100.0
	var removed := apply_damage(state, model, amount, &"collision")
	if removed > 0.0:
		cooldown = model.collision_cooldown
	return removed
