class_name MapRouter
extends RefCounted
## Catalogued scene transitions retain the session. No file paths from save data.
signal transition_started(map_id: StringName)
signal transition_finished(map_id: StringName)
var context: RuntimeContext
var host: Node
var current: Node3D
var busy := false
var catalog: Dictionary = {}

func register_map(id: StringName, scene: PackedScene) -> bool:
	if id == &"" or scene == null or catalog.has(id):
		return false
	catalog[id] = scene
	return true

func enter(id: StringName, prepare: Callable = Callable(), entry: StringName = &"default") -> bool:
	if busy or not catalog.has(id) or not is_instance_valid(host) or context == null:
		return false
	var candidate: Node = catalog[id].instantiate()
	if not candidate is Node3D or not candidate.has_method("capture_map_state") or not candidate.get("definition") is MapDefinition:
		candidate.free()
		return false
	var next := candidate as Node3D
	var has_context := false
	for property in next.get_property_list():
		if property.name == &"context":
			has_context = true
			break
	if not has_context or next.definition.map_id != id or not next.definition.entry_points.has(String(entry)):
		next.free()
		return false
	busy = true
	context.player.suspend(&"map_transition")
	transition_started.emit(id)
	if is_instance_valid(current):
		context.map_states[String(context.current_map)] = current.capture_map_state()
		# Snapshot the driver relation before release; an interior can keep an
		# unoccupied parked vehicle in session state without duplicating its node.
		context.player.release_control()
		context.world.unbind()
		host.remove_child(current)
		current.queue_free()
	next.context = context
	current = next
	host.add_child(current)
	context.current_map = id
	if current.has_method("restore_map_state"):
		current.restore_map_state(context.map_states.get(String(id), {}))
	if current.has_method("arrive_at"):
		current.arrive_at(entry)
	if prepare.is_valid():
		await prepare.call(current)
	context.player.resume(&"map_transition")
	busy = false
	transition_finished.emit(id)
	return true
