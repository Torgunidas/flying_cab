class_name WalkingActor
extends CharacterBody3D
## Reusable on-foot movement adapter. Scene/model/interactions can be authored later.
signal driver_changed(previous: StringName, current: StringName)
@export var actor_id: StringName = &"ari"
@export var walking_speed := 4.0
@export var acceleration := 18.0
@export var gravity := 9.8
var _driver: StringName = &""
var _revision := 0
var _command := Vector2.ZERO

func assign_driver(id: StringName) -> int:
	var previous := _driver
	_driver = id
	_revision += 1
	_command = Vector2.ZERO
	driver_changed.emit(previous, id)
	return _revision

func get_driver_id() -> StringName:
	return _driver

func receive_command(value: Vector2, id: StringName, revision: int) -> bool:
	if id != _driver or revision != _revision or id == &"":
		return false
	_command = Vector2(clampf(value.x, -1, 1), 0)
	return true

func get_control_velocity() -> Vector3:
	return velocity

func _physics_process(dt: float) -> void:
	velocity.x = move_toward(velocity.x, _command.x * walking_speed, acceleration * dt)
	if not is_on_floor():
		velocity.y -= gravity * dt
	else:
		velocity.y = 0
	velocity.z = 0
	move_and_slide()
	position.z = 0
