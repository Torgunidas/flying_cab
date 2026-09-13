class_name WalkingActor
extends CharacterBody3D
## Shared actor contract; input is supplied by PlayerSession, never polled here.
signal driver_changed(previous: StringName, current: StringName)
@export var actor_id: StringName = &"ari"
@export var walking_speed := 4.0
@export var acceleration := 18.0
@export var gravity := 12.25
@export var jump_speed := 4.4
@export_range(0.0, 1.0) var air_control := 0.45
var facing := 1.0
var active := true
var movement_state: StringName = &"idle"
var _driver: StringName = &""
var _revision := 0
var _command := Vector2.ZERO
var _jump_held := false
var _jump_pending := false
var _landing_time := 0.0

func assign_driver(id: StringName) -> int:
	var previous := _driver
	_driver = id
	_revision += 1
	clear_control_input()
	driver_changed.emit(previous, id)
	return _revision

func get_driver_id() -> StringName:
	return _driver

func receive_command(value: Vector2, id: StringName, revision: int) -> bool:
	if id != _driver or revision != _revision or id == &"":
		return false
	_command = Vector2(clampf(value.x, -1, 1), clampf(value.y, 0, 1))
	var held := _command.y > 0.5
	if held and not _jump_held:
		_jump_pending = true
	_jump_held = held
	return true

func clear_control_input() -> void:
	_command = Vector2.ZERO
	_jump_pending = false
	_jump_held = false

func set_active(value: bool) -> void:
	active = value
	visible = value
	set_physics_process(value)
	collision_layer = 2 if value else 0
	collision_mask = 1 if value else 0
	clear_control_input()
	velocity = Vector3.ZERO

func place(at: Vector3, motion := Vector3.ZERO) -> void:
	global_position = Vector3(at.x, at.y, 0)
	velocity = Vector3(motion.x, motion.y, 0)
	clear_control_input()
	reset_physics_interpolation()

func get_control_velocity() -> Vector3:
	return velocity

func _physics_process(dt: float) -> void:
	var grounded := is_on_floor()
	velocity.x = move_toward(velocity.x, _command.x * walking_speed, acceleration * (1.0 if grounded else air_control) * dt)
	if absf(_command.x) > 0.01:
		facing = signf(_command.x)
	if not grounded:
		velocity.y -= gravity * dt
	else:
		velocity.y = 0
	if _jump_pending and grounded:
		velocity.y = jump_speed
	_jump_pending = false
	velocity.z = 0
	move_and_slide()
	position.z = 0
	_landing_time = maxf(0, _landing_time - dt)
	if not grounded and is_on_floor():
		_landing_time = 0.12
	movement_state = &"jump" if velocity.y > 0 else &"fall"
	if is_on_floor():
		movement_state = &"land" if _landing_time > 0 else (&"walk" if absf(velocity.x) > 0.1 else &"idle")
	var visual := get_node_or_null("Visual")
	if visual and visual.has_method("pose_actor"):
		visual.pose_actor(dt, movement_state, facing)
