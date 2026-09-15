class_name TrafficDriver
extends RefCounted
## A controller for the ordinary FlightCab. Never teleports or replaces its body.
## Steering produces the same revision-checked thrust commands as player input.
var car: FlightCab
var id: StringName
var revision := 0
var _fuel_enabled := true
var blocked := false

func bind(vehicle: FlightCab, resident: StringName) -> void:
	car = vehicle
	id = resident
	_fuel_enabled = car.definition.fuel_enabled
	# Ambient operation has a separate, unmodelled fuel budget in this slice.
	# Preserve the actual tank for takeover, and restore normal burn immediately.
	car.definition.fuel_enabled = false
	revision = car.assign_driver(id)
	car.driver_changed.connect(_driver_changed)

func _driver_changed(_previous: StringName, current: StringName) -> void:
	if current != id:
		detach()

func detach() -> void:
	if is_instance_valid(car):
		car.definition.fuel_enabled = _fuel_enabled
		if car.driver_changed.is_connected(_driver_changed):
			car.driver_changed.disconnect(_driver_changed)
	car = null

func steer(target: Vector3, speed: float, _dt: float, obstacle := false) -> void:
	if not is_instance_valid(car) or car.get_driver_id() != id:
		return
	var offset := target - car.global_position
	offset.z = 0
	var distance := offset.length()
	# Brake into authored corners and landing points, retaining thrust inertia.
	var desired := offset.normalized() * minf(speed, sqrt(maxf(0, distance * 5.0)))
	if distance < 1.0:
		desired = offset * 1.5
	blocked = obstacle
	if obstacle:
		desired = Vector3.ZERO
	var velocity := car.linear_velocity
	var tuning := car.definition
	var acceleration := (desired - velocity) * 3.0
	var numerator := acceleration.x + velocity.x * (tuning.horizontal_coast_damping + tuning.linear_damping)
	var denominator := tuning.horizontal_acceleration + signf(numerator) * velocity.x * tuning.horizontal_coast_damping
	var horizontal := clampf(numerator / maxf(denominator, 1), -1, 1)
	var climb_drag := maxf(velocity.y, 0) * tuning.upward_coast_damping
	var vertical := clampf((acceleration.y + tuning.gravity + velocity.y * tuning.linear_damping + climb_drag) / (tuning.vertical_acceleration + climb_drag), 0, 1)
	car.receive_command(Vector2(horizontal, vertical), id, revision)

func stop_on_ground() -> void:
	if is_instance_valid(car):
		car.receive_command(Vector2.ZERO, id, revision)
