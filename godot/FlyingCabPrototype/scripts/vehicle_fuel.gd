class_name VehicleFuel
extends RefCounted
## Sole writer for propulsion burn/refuelling. No presentation dependencies.
var applied_command := Vector2.ZERO
var burn_rate := 0.0
var refueling := false

func consume(state: VehicleState, tuning: VehicleDefinition, requested: Vector2, ceiling_pressure: float, cost_multiplier: float, emergency_return: bool, dt: float) -> void:
	applied_command = requested
	burn_rate = 0.0
	if not tuning.fuel_enabled:
		return
	var vertical_cost := tuning.fuel_vertical_rate * lerpf(1.0, tuning.ceiling_fuel_multiplier, ceiling_pressure)
	var requested_rate := (absf(requested.x) * tuning.fuel_horizontal_rate + requested.y * vertical_cost) * cost_multiplier
	if requested_rate <= 0.0:
		return
	var fraction := minf(1.0, state.fuel / (requested_rate * dt))
	if not emergency_return:
		applied_command *= fraction
	burn_rate = requested_rate * fraction
	state.fuel = maxf(0.0, state.fuel - burn_rate * dt)

func refuel(state: VehicleState, tuning: VehicleDefinition, eligible: bool, dt: float) -> void:
	refueling = tuning.fuel_enabled and eligible and state.fuel < tuning.fuel_capacity
	if refueling:
		state.fuel = minf(tuning.fuel_capacity, state.fuel + tuning.refuel_rate * dt)
