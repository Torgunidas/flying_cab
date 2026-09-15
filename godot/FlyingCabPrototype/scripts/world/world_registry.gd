class_name WorldRegistry
extends RefCounted
## Scoped to the loaded map. IDs persist; node references do not enter saves.
signal changed
signal vehicle_registered(vehicle: Node)
var definition: MapDefinition
var living_world: Node
var lanes: Array[Node] = []
var pads: Array[Node] = []
var vehicles: Array[Node] = []
var workshops: Array[Node] = []
var entities: Dictionary = {}
var _world: Node
var _tree: SceneTree

func map_root() -> Node:
	return _world if is_instance_valid(_world) else null

func bind(world: Node, data: MapDefinition) -> void:
	unbind()
	_world = world
	_tree = world.get_tree()
	definition = data
	for node in world.find_children("*", "", true, false):
		register(node)
	_tree.node_added.connect(_added)
	_tree.node_removed.connect(_removed)
	changed.emit()

func unbind() -> void:
	if is_instance_valid(_tree):
		if _tree.node_added.is_connected(_added):
			_tree.node_added.disconnect(_added)
		if _tree.node_removed.is_connected(_removed):
			_tree.node_removed.disconnect(_removed)
	living_world = null
	lanes.clear()
	pads.clear()
	vehicles.clear()
	workshops.clear()
	entities.clear()
	_world = null
	_tree = null

func register(node: Node) -> bool:
	if not is_instance_valid(node) or not is_instance_valid(_world) or not _world.is_ancestor_of(node):
		return false
	var id := StringName(node.get_meta("entity_id", ""))
	if id != &"":
		if entities.has(id) and entities[id] != node:
			push_error("Duplicate world entity: " + String(id))
			return false
		entities[id] = node
	var altered := false
	if node.is_in_group("repair_station") and not workshops.has(node):
		workshops.append(node)
		altered = true
	if node.is_in_group("highway") and not lanes.has(node):
		lanes.append(node)
		altered = true
	if node.is_in_group("refuel_pad") and not pads.has(node):
		pads.append(node)
		altered = true
	if node.is_in_group("vehicle") and not vehicles.has(node):
		vehicles.append(node)
		vehicle_registered.emit(node)
		altered = true
	if altered or id != &"":
		changed.emit()
	return true

func _added(node: Node) -> void:
	# Wait for _ready() and group registration. A weak reference tolerates objects
	# created and removed within the same frame (e.g. cancelled map transitions).
	_register_later.call_deferred(weakref(node))

func _register_later(node_ref: WeakRef) -> void:
	var node = node_ref.get_ref()
	if node:
		register(node)

func _removed(node: Node) -> void:
	var altered := lanes.has(node) or pads.has(node) or vehicles.has(node) or workshops.has(node)
	workshops.erase(node)
	lanes.erase(node)
	pads.erase(node)
	vehicles.erase(node)
	var id := StringName(node.get_meta("entity_id", ""))
	if entities.get(id) == node:
		entities.erase(id)
		altered = true
	if altered:
		changed.emit()
