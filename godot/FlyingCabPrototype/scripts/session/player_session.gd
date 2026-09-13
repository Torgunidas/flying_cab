class_name PlayerSession
extends RefCounted
## Control and camera focus follow a capability, not a node named Cab.
signal focus_changed(previous: Node3D, current: Node3D)
signal control_suspended(suspended: bool)
var actor_id: StringName = &"ari"
var focus: Node3D
var mode: StringName = &"none"
var _control_revision := 0
var _locks: Dictionary = {}

func take_control(actor: Node3D, control_mode: StringName) -> bool:
	if not is_instance_valid(actor) or not actor.has_method("assign_driver") or not actor.has_method("receive_command"):
		return false
	var previous := focus
	if is_instance_valid(previous):
		_disconnect_driver(previous)
		if previous != actor:
			previous.assign_driver(&"")
	focus = actor
	mode = control_mode
	_control_revision = actor.assign_driver(actor_id)
	if actor.has_signal("driver_changed"):
		actor.driver_changed.connect(_driver_changed)
	focus_changed.emit(previous, focus)
	return true

func release_control() -> void:
	var previous := focus
	if is_instance_valid(previous):
		_disconnect_driver(previous)
		previous.assign_driver(&"")
	focus = null
	mode = &"none"
	focus_changed.emit(previous, null)

func dispatch(command: Vector2) -> void:
	if is_instance_valid(focus):
		focus.receive_command(Vector2.ZERO if is_suspended() else command, actor_id, _control_revision)

func suspend(reason: StringName) -> void:
	var before := is_suspended()
	_locks[reason] = true
	dispatch(Vector2.ZERO)
	if is_instance_valid(focus) and focus.has_method("clear_control_input"):
		focus.clear_control_input()
	if not before:
		control_suspended.emit(true)

func resume(reason: StringName) -> void:
	var before := is_suspended()
	_locks.erase(reason)
	if before and not is_suspended():
		control_suspended.emit(false)

func is_suspended() -> bool:
	return not _locks.is_empty()

func _disconnect_driver(actor: Node) -> void:
	if actor.has_signal("driver_changed") and actor.driver_changed.is_connected(_driver_changed):
		actor.driver_changed.disconnect(_driver_changed)

func _driver_changed(_previous: StringName, current: StringName) -> void:
	if current == actor_id:
		return
	var previous := focus
	_disconnect_driver(previous)
	focus = null
	mode = &"none"
	focus_changed.emit(previous, null)
