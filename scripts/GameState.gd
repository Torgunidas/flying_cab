extends Node               # ⇦ NIE dawaj tutaj `class_name`

@export var starting_money: int = 100    # kwota początkowa (Inspector)
@export var default_level_scene: PackedScene
var _selected_level : PackedScene = null

signal money_changed(new_amount: int)
signal maya_timer_updated(seconds_left: int)
signal maya_time_expired

# --- Items --------------------------------------------------------------
signal items_changed(item_list: Array)
var _items: Dictionary = {}       # id -> ItemData


# ——————————————————————————
#  POJAZDY: model vs instancja
# ——————————————————————————
var allowed_models    : Array[String] = []     # „taxi”, „police_suv”...
var allowed_instances : Array[String] = []     # „taxi_42”, „police_suv_A1”...

signal vehicle_access_changed               # (emitter, added: bool, id: String)

# --- pola wewnętrzne -------------------------------------------------------
var _money: int       = 0
var _initialized: bool = false           # ⇦ TA zmienna zapobiega ponownemu resetowi
var _maya_timer: Timer
var _maya_seconds_left: int = 0
@export var maya_timer_minutes: int = 6
var _maya_timer_active: bool = false

# --- lifecycle -------------------------------------------------------------
func _ready() -> void:
	if not _initialized:                 # tylko przy pierwszym starcie
			_money       = starting_money
			_initialized = true
			print("[GS _ready] init =", _money)
			_maya_timer = Timer.new()
			_maya_timer.wait_time = 1.0
			_maya_timer.one_shot = false
			_maya_timer.autostart = false
			add_child(_maya_timer)
			_maya_timer.timeout.connect(_on_maya_timer_tick)
	else:
			print("[GS _ready] hot-reload, saldo zostaje =", _money)

	emit_signal("money_changed", _money)
	emit_signal("items_changed", get_items())

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
	
# --- RESET GAME ------------------------------------------------------------
# --- RESET GAME ------------------------------------------------------------
func reset_game() -> void:
		# 1) wyczyść zasoby gracza
		_money                = 100
		allowed_models.clear()
		allowed_instances.clear()
		_items.clear()

		# 2) spawn / level
		_selected_level       = null
		_spawn_point_name     = ""

		# 3) globalny timer
		stop_maya_timer()
		_maya_seconds_left    = 0

		# 4) questy
		var qm := get_node_or_null("/root/QuestSys")
		if qm and qm.has_method("reset_all"):
				qm.reset_all()

		# 5) pojazd
		var vp := get_node_or_null("/root/VehPers")
		if vp and vp.has_method("reset_vehicle_persistence"):
				vp.reset_vehicle_persistence()

		# 6) pozwól _ready() ustawić starting_money przy następnym wejściu do gry
		_initialized = false

		# 7) sygnały UI
		emit_signal("money_changed", _money)
		emit_signal("items_changed", get_items())



	
# --- ITEM MANAGEMENT -------------------------------------------------------
func grant_item(item: ItemData) -> void:
		if item == null or item.id == "":
				return
		if _items.has(item.id):
				return
		_items[item.id] = item
		emit_signal("items_changed", get_items())

func revoke_item(item_id: String) -> void:
		if _items.has(item_id):
				_items.erase(item_id)
				emit_signal("items_changed", get_items())

func has_item(item_id: String) -> bool:
		return _items.has(item_id)

func get_items() -> Array:
		return _items.values()
# --- MAYA GLOBAL TIMER -----------------------------------------------------
func start_maya_timer(minutes := maya_timer_minutes) -> void:
		if _maya_timer_active:
				return
		_maya_seconds_left = minutes * 60
		_maya_timer.start()
		_maya_timer_active = true
		emit_signal("maya_timer_updated", _maya_seconds_left)

func stop_maya_timer() -> void:
		if not _maya_timer_active:
				return
		_maya_timer.stop()
		_maya_timer_active = false

func get_maya_seconds_left() -> int:
		return _maya_seconds_left
		
func add_maya_time(minutes: int) -> void:
		if minutes <= 0:
				return
		_maya_seconds_left += minutes * 60
		if not _maya_timer_active and _maya_seconds_left > 0:
				_maya_timer.start()
				_maya_timer_active = true
		emit_signal("maya_timer_updated", _maya_seconds_left)

func _on_maya_timer_tick() -> void:
		if not _maya_timer_active:
				return
		_maya_seconds_left -= 1
		emit_signal("maya_timer_updated", _maya_seconds_left)
		if _maya_seconds_left <= 0:
				_maya_timer.stop()
				_maya_timer_active = false
				emit_signal("maya_time_expired")
