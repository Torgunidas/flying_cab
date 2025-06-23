extends Node               # ⇦ NIE dawaj tutaj `class_name`

@export var starting_money: int = 100    # kwota początkowa (Inspector)
@export var default_level_scene: PackedScene
var _selected_level : PackedScene = null

signal money_changed(new_amount: int)

# --- pola wewnętrzne -------------------------------------------------------
var _money: int       = 0
var _initialized: bool = false           # ⇦ TA zmienna zapobiega ponownemu resetowi

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
