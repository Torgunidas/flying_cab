extends Node               # ⇦ NIE dawaj tutaj `class_name`

@export var starting_money: int = 100    # kwota początkowa (Inspector)
@export var default_level_scene: PackedScene
var _selected_level : PackedScene = null

signal money_changed(new_amount: int)

# ——————————————————————————
#  POJAZDY: model vs instancja
# ——————————————————————————
var allowed_models    : Array[String] = []     # „taxi”, „police_suv”...
var allowed_instances : Array[String] = []     # „taxi_42”, „police_suv_A1”...

signal vehicle_access_changed               # (emitter, added: bool, id: String)

# --- pola wewnętrzne -------------------------------------------------------
var _money: int       = 0
var _initialized: bool = false           # ⇦ TA zmienna zapobiega ponownemu resetowi

# ─────────────────────────────────────────────
#  ZAPAMIĘTANE TRANSFORMY POJAZDÓW
#  { "Level1": { "limo_123": {pos: Vector2, rot: float} } }
# ─────────────────────────────────────────────
var saved_vehicle_tf : Dictionary = {}

# --- lifecycle -------------------------------------------------------------
func _ready() -> void:
	if not _initialized:                 # tylko przy pierwszym starcie
		_money       = starting_money
		_initialized = true
		print("[GS _ready] init =", _money)
	else:
		print("[GS _ready] hot-reload, saldo zostaje =", _money)

	emit_signal("money_changed", _money)

# --- API -------------------------------------------------------------------
func get_money() -> int:
	return _money

func add_money(amount: int) -> void:
	print("[GS add_money] before =", _money, " +", amount)
	_money += amount
	print("[GS add_money] after  =", _money)
	emit_signal("money_changed", _money)

func spend_money(amount: int) -> bool:
	if amount > _money:
		return false
	_money -= amount
	emit_signal("money_changed", _money)
	return true

func set_selected_level(scene: PackedScene) -> void:
	_selected_level = scene 
	
func get_selected_level() -> PackedScene:
	return _selected_level if _selected_level else default_level_scene

# --- SPAWN POINT ----------------------------------------------------------
var _spawn_point_name: String = ""

func set_spawn_point_name(name: String) -> void:
	_spawn_point_name = name

func get_spawn_point_name() -> String:
	return _spawn_point_name

func grant_vehicle(id: String, is_instance := false) -> void:
	if id == "":
		return

	var list : Array[String]
	if is_instance:
		list = allowed_instances
	else:
		list = allowed_models

	if id in list:
		return
	list.append(id)
	emit_signal("vehicle_access_changed", true, id)


func revoke_vehicle(id: String, is_instance := false) -> void:
	var list : Array[String]
	if is_instance:
		list = allowed_instances
	else:
		list = allowed_models

	if id in list:
		list.erase(id)
		emit_signal("vehicle_access_changed", false, id)


func has_vehicle(id: String) -> bool:
	return id in allowed_models or id in allowed_instances

func save_vehicle_transform(level_name: String, car: Car) -> void:
	if car == null:
		return
	if not saved_vehicle_tf.has(level_name):
		saved_vehicle_tf[level_name] = {}
	saved_vehicle_tf[level_name][car.instance_id] = {
		"pos": car.global_position,
		"rot": car.rotation
	}

func get_saved_transform(level_name: String, inst_id: String) -> Dictionary:
	if saved_vehicle_tf.has(level_name):
		return saved_vehicle_tf[level_name].get(inst_id, {})
	return {}

func clear_vehicle_transforms(level_name: String) -> void:
	saved_vehicle_tf.erase(level_name)
