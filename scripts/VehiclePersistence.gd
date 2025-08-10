extends Node
class_name VehiclePersistence     # Autoload o nazwie VehPers

const WORLD_LEVEL_IDS: Array[String] = ["HiCity"]   # dopisz kolejne jeśli potrzebujesz

var _saved: Dictionary = {}     # klucz = nazwa root levelu
var current_car: Car = null
var last_world_id: String = ""

# ---------- Helpery właściwości ----------
func _get_prop(obj: Object, name: String) -> Variant:
	if obj == null:
		return null
	for p in obj.get_property_list():
		if p.name == name:
			return obj.get(name)
	return null

func _set_prop(obj: Object, name: String, value) -> void:
	if obj == null:
		return
	for p in obj.get_property_list():
		if p.name == name:
			obj.set(name, value)
			return

# ---------- Public API ----------
func is_world_level(level_root: Node) -> bool:
	return level_root != null and level_root.name in WORLD_LEVEL_IDS

func has_state(world_name: String) -> bool:
	return _saved.has(world_name)

func clear_car_state(world_name: String) -> void:
	_saved.erase(world_name)
	if last_world_id == world_name:
		last_world_id = ""

func set_current_car(car: Car) -> void:
	current_car = car
	
func export_last_state() -> Dictionary:
		if last_world_id == "" or not _saved.has(last_world_id):
				return {}
		var data: Dictionary = _saved[last_world_id].duplicate(true)
		data.world_id = last_world_id
		return data

func import_last_state(data: Dictionary) -> void:
		if data.is_empty():
				return
		var world_id: String = data.get("world_id", "")
		if world_id == "":
				return
		var copy := data.duplicate(true)
		copy.erase("world_id")
		_saved[world_id] = copy
		last_world_id = world_id

# ---------- Save ----------
func save_car_state(world_root: Node) -> void:
	if current_car == null or world_root == null:
		return
	if not is_world_level(world_root):
		return

	var path: String = current_car.scene_file_path
	var hp_val       = _get_prop(current_car, "current_hp")
	var max_hp_val   = _get_prop(current_car, "max_hp")
	var fuel_val     = _get_prop(current_car, "current_fuel")
	var max_fuel_val = _get_prop(current_car, "max_fuel")
	var car_id_val = _get_prop(current_car, "car_id")

	_saved[world_root.name] = {
		"car_scene_path": path,
		"pos": current_car.global_position,
		"rot": current_car.rotation,
		"vel": current_car.velocity,
		"hp": hp_val,
		"max_hp": max_hp_val,
		"fuel": fuel_val,
		"max_fuel": max_fuel_val,
		"car_id": car_id_val
	}
	last_world_id = world_root.name
	_dbg("saved", "root="+world_root.name+" pos="+str(current_car.global_position)
		+" hp="+str(hp_val)+"/"+str(max_hp_val)+" fuel="+str(fuel_val)+"/"+str(max_fuel_val))

# ---------- Restore ----------
func restore_car_state(world_root: Node2D, parent: Node) -> Car:
	if world_root == null or parent == null:
		return null
	if not is_world_level(world_root):
		return null
	if not _saved.has(world_root.name):
		return null

	var data: Dictionary = _saved[world_root.name]
	var saved_car_id = data.get("car_id", null)
	var path: String = data.get("car_scene_path", "")
	if path == "" or not ResourceLoader.exists(path):
		push_warning("VehPers: brak sceny pojazdu: %s" % path)
		return null

	var packed: PackedScene = load(path)
	var car := packed.instantiate() as Car
	parent.add_child(car)

	# Transform
	car.global_position = data.pos
	car.rotation = data.rot

	# Velocity (po _ready)
	if data.has("vel") and typeof(data.vel) == TYPE_VECTOR2:
		call_deferred("_apply_velocity", car, data.vel)

	# Deferred stats (aby nie nadpisało ich _ready() w car.gd)
	call_deferred("_restore_stats_after_ready", car, data)
	
		# po zainstancjowaniu:
	if saved_car_id != null:
		_set_prop(car, "car_id", saved_car_id)   # jeśli chcesz zachować identyfikator
	car.set_meta("player_owned", true)
	car.add_to_group("player_owned_vehicle")
	car.set_meta("restored_from_save", true)

	current_car = car
	return car

func _apply_velocity(car: Car, vel: Vector2) -> void:
	if car and car.is_inside_tree():
		car.velocity = vel

func _restore_stats_after_ready(car: Car, data: Dictionary) -> void:
	if car == null or not car.is_inside_tree():
		return
	var saved_max_hp   = data.get("max_hp", null)
	var saved_hp       = data.get("hp", null)
	var saved_max_fuel = data.get("max_fuel", null)
	var saved_fuel     = data.get("fuel", null)

	if saved_max_hp != null:
		_set_prop(car, "max_hp", saved_max_hp)
	if saved_hp != null:
		var cap_hp = saved_max_hp if saved_max_hp != null else saved_hp
		_set_prop(car, "current_hp", clamp(saved_hp, 0, cap_hp))

	if saved_max_fuel != null:
		_set_prop(car, "max_fuel", saved_max_fuel)
	if saved_fuel != null:
		var cap_fuel = saved_max_fuel if saved_max_fuel != null else saved_fuel
		_set_prop(car, "current_fuel", clamp(saved_fuel, 0.0, float(cap_fuel)))

	_dbg("restored_stats", "hp="+str(saved_hp)+"/"+str(saved_max_hp)+" fuel="+str(saved_fuel)+"/"+str(saved_max_fuel))

# ---------- Debug ----------
func _dbg(tag: String, extra := "") -> void:
	print("[VehPers]", tag, extra, " keys=", _saved.keys())
	
	
	# -----------------------------------------------------------------------
#  RESET – kasuje cały runtime’owy zapis pojazdu
# -----------------------------------------------------------------------
func reset_vehicle_persistence() -> void:
	_saved.clear()
	current_car = null
	last_world_id = ""
	_dbg("reset", "vehicle state cleared")
	
func spawn_car_in_garage(car_id:String) -> Car:
		var scene_path := _get_scene_for_car(car_id)
		var packed: PackedScene = load(scene_path)
		var car := packed.instantiate() as Car
		var garage := get_tree().get_current_scene().get_node_or_null("GarageSpawn")
		if garage:
				garage.add_child(car)
				car.global_position = garage.global_position
		else:
				get_tree().get_current_scene().add_child(car)
		return car

func _get_scene_for_car(car_id:String) -> String:
		var map := {
				"cab_neo": "res://scenes/cars/cab_neo.tscn",
				"taxi": "res://scenes/cars/car_yellow_cab.tscn"
		}
		return map.get(car_id, "res://scenes/cars/car_yellow_cab.tscn")
