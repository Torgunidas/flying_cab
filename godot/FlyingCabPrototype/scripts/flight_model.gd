class_name FlightModel
extends RefCounted

static func step_velocity(v: Vector3, command: Vector2, dt: float, tuning: VehicleDefinition, speed_multiplier: float = 1.0) -> Vector3:
	var horizontal := clampf(command.x, -1.0, 1.0)
	var thrust := clampf(command.y, 0.0, 1.0)
	# Blend coast braking as power fades, avoiding a second on/off step when
	# the engine reaches zero. Full-power and unpowered damping stay unchanged.
	v.x = lerpf(v.x, 0.0, clampf(dt * tuning.horizontal_coast_damping * (1.0 - absf(horizontal)), 0.0, 1.0))
	if v.y > 0.0:
		v.y = lerpf(v.y, 0.0, clampf(dt * tuning.upward_coast_damping * (1.0 - thrust), 0.0, 1.0))
	v *= maxf(0.0, 1.0 - dt * tuning.linear_damping)
	v.x += horizontal * tuning.horizontal_acceleration * dt
	v.y += (thrust * tuning.vertical_acceleration - tuning.gravity) * dt
	v.x = clampf(v.x, -tuning.max_horizontal_speed * speed_multiplier, tuning.max_horizontal_speed * speed_multiplier)
	v.y = clampf(v.y, -tuning.max_fall_speed * speed_multiplier, tuning.max_climb_speed * speed_multiplier)
	v.z = 0.0
	return v
