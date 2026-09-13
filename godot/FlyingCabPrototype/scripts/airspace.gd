extends RefCounted
## City control: soft altitude limit and a physical return along clear edge lanes.

var returning := false
var return_target := Vector3.ZERO

func reset() -> void:
	returning = false
	return_target = Vector3.ZERO

func ceiling_pressure(height: float, world: CityDefinition) -> float:
	return smoothstep(world.max_altitude - world.soft_ceiling_range, world.max_altitude, height)

func command_for(position: Vector3, velocity: Vector3, pilot: Vector2, tuning: VehicleDefinition, world: CityDefinition) -> Vector2:
	if not tuning.airspace_enabled:
		reset()
		return pilot
	if not returning and (position.x < world.city_left or position.x > world.city_right):
		returning = true
		return_target = Vector3(world.city_left + world.return_margin if position.x < world.city_left else world.city_right - world.return_margin, clampf(position.y, world.ground_height + world.return_min_height, world.max_altitude - world.soft_ceiling_range - 3.0), 0.0)
	if not returning:
		return pilot
	var error := return_target - position
	if absf(error.x) < 0.3 and absf(error.y) < 0.4 and absf(velocity.x) < 0.6 and absf(velocity.y) < 0.6:
		returning = false
		# Never replay a held pilot input on the handover frame.
		return Vector2.ZERO
	var target_vx := signf(error.x) * minf(world.return_speed, sqrt(2.0 * tuning.horizontal_acceleration * 0.7 * absf(error.x)))
	if absf(error.x) < 0.1:
		target_vx = 0.0
	var target_vy := clampf(error.y * 1.5, -4.0, 4.0)
	return Vector2(clampf((target_vx - velocity.x) * 3.0 / tuning.horizontal_acceleration, -1.0, 1.0), clampf(((target_vy - velocity.y) * 4.0 + tuning.gravity + velocity.y * tuning.linear_damping) / tuning.vertical_acceleration, 0.0, 1.0))

func limit_climb(velocity: Vector3, position: Vector3, dt: float, tuning: VehicleDefinition, world: CityDefinition, speed_multiplier: float = 1.0) -> Vector3:
	var pressure := ceiling_pressure(position.y, world) if tuning.airspace_enabled else 0.0
	if pressure <= 0.0:
		return velocity
	if velocity.y > 0.0:
		# The old Godot progressively damped ascent. A shrinking ascent limit also
		# prevents an indefinitely held thruster from creeping through the ceiling.
		velocity.y *= exp(-pressure * world.ceiling_drag * dt)
		velocity.y = minf(velocity.y, tuning.max_climb_speed * speed_multiplier * (1.0 - pressure))
		velocity.y = minf(velocity.y, maxf(0.0, (world.max_altitude - position.y) / dt))
	if position.y > world.max_altitude:
		velocity.y = minf(velocity.y, -minf(3.0, (position.y - world.max_altitude) * 1.5))
	return velocity
