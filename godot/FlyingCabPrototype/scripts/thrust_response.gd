class_name ThrustResponse
extends RefCounted
## Per-vehicle engine power, independent of keyboard/touch and render frame rate.
var power := Vector2.ZERO

func reset() -> void:
	power = Vector2.ZERO

func advance(requested: Vector2, dt: float, tuning: VehicleDefinition) -> Vector2:
	power.x = _axis(power.x, clampf(requested.x, -1.0, 1.0), dt, tuning)
	power.y = _axis(power.y, clampf(requested.y, 0.0, 1.0), dt, tuning)
	return power

static func _axis(current: float, target: float, dt: float, tuning: VehicleDefinition) -> float:
	# A direction reversal first releases the previous engine, then builds the
	# opposite thrust. Carry remaining time across zero, even at a low tick rate.
	if current * target < 0.0:
		var release_time := absf(current) * tuning.thrust_release_seconds
		if dt < release_time:
			return move_toward(current, 0.0, dt / tuning.thrust_release_seconds)
		dt -= release_time
		current = 0.0
	var seconds := tuning.thrust_rise_seconds if absf(target) > absf(current) else tuning.thrust_release_seconds
	return target if seconds <= 0.0 else move_toward(current, target, dt / seconds)
