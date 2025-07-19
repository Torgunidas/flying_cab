extends Node
class_name VehiclePersistence

const WORLD_LEVEL_IDS: Array[String] = ["HiCity"]

var _saved: Dictionary = {}               # world_name -> dict
var current_car: Car = null
var last_world_id: String = ""

func is_world_level(level_root: Node) -> bool:
	if level_root == null:
		return false
	return level_root.name in WORLD_LEVEL_IDS


func save_car_state(world_root: Node) -> void:
	_dbg("save_enter", "root="+world_root.name+" cur="+str(current_car))
	if current_car == null or world_root == null:
		return
	if not is_world_level(world_root):
		return
	_dbg("saved", "root="+world_root.name+" pos="+str(current_car.global_position))

	# ── WARIANT A (jeżeli rzeczywiście potrzebujesz PackedScene) ──
	# var car_scene: PackedScene = current_car.get_scene() as PackedScene
	# var path: String = car_scene.resource_path if car_scene != null else ""

	# ── WARIANT B (PROSTSZY) używa wbudowanej właściwości node'a (Godot 4):
	var path: String = current_car.scene_file_path

	_saved[world_root.name] = {
		"car_scene_path": path,
		"pos": current_car.global_position,
		"rot": current_car.rotation,
		"vel": current_car.velocity,
	}
	last_world_id = world_root.name


func clear_car_state(world_name: String) -> void:
	_saved.erase(world_name)
	if last_world_id == world_name:
		last_world_id = ""


func has_state(world_name: String) -> bool:
	return _saved.has(world_name)


func restore_car_state(world_root: Node2D, parent: Node) -> Car:
	_dbg("restore_try", "root="+world_root.name)
	if world_root == null or parent == null:
		return null
	if not is_world_level(world_root):
		return null
	if not _saved.has(world_root.name):
		return null

	var data: Dictionary = _saved[world_root.name]
	var path: String = data.get("car_scene_path", "")
	if path == "" or not ResourceLoader.exists(path):
		push_warning("VehPers: brak sceny pojazdu: %s" % path)
		return null
	_dbg("restore_found", "path="+path)

	var packed: PackedScene = load(path)
	var car: Car = packed.instantiate() as Car
	parent.add_child(car)
	car.global_position = data.pos
	car.rotation = data.rot
	call_deferred("_apply_velocity", car, data.vel)
	current_car = car
	_dbg("restored", "pos="+str(car.global_position))
	return car


func _apply_velocity(car: Car, vel: Vector2) -> void:
	if car and car.is_inside_tree():
		car.velocity = vel


func set_current_car(car: Car) -> void:
	current_car = car
	_dbg("set_current_car", "car="+str(car))

func _dbg(tag: String, extra := "") -> void:
	print("[VehPers]", tag, " world_keys=", _saved.keys(), " current_car=", current_car, " last_world=", last_world_id, " ", extra)
