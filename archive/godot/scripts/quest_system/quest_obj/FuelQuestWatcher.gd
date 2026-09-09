extends Node
class_name FuelQuestWatcher
# ───────────────────────────────────────────────────────────────
#  Parametry edytowalne
# ───────────────────────────────────────────────────────────────
@export var quest_id: String

@export_range(0.0, 100.0, 1.0)
var threshold_percent: float = 20.0

@export_enum("<", ">", "=")
var relation: String = "<"            # "<"  "<="  ">"  ">="  "="

@export var epsilon: float = 0.1      # tolerancja dla "=" – w %

# ───────────────────────────────────────────────────────────────
var _pc: PlayerCharacter = null
var _connected_car: Car = null

func _ready() -> void:
	_try_attach_player()
	# Jeśli gracza nie ma jeszcze w grupie, próbuj co 0,1 s
	if _pc == null:
		var t := Timer.new()
		t.one_shot = false
		t.wait_time = 0.1
		add_child(t)
		t.timeout.connect(_try_attach_player)
		t.start()

# ───────────────────────────────────────────────────────────────
#  SZUKANIE GRACZA I PODPINANIE DO AUTA
# ───────────────────────────────────────────────────────────────
func _try_attach_player() -> void:
	if _pc:
		return
	var pc := get_tree().get_first_node_in_group("player")
	if pc:
		_pc = pc as PlayerCharacter
		_pc.entered_vehicle.connect(_on_player_entered_vehicle)
		if _pc.in_vehicle and _pc.vehicle:
			_connect_car(_pc.vehicle)

func _on_player_entered_vehicle(car: Car) -> void:
	_connect_car(car)

func _connect_car(car: Car) -> void:
	if _connected_car == car:
		return
	if _connected_car:
		_connected_car.fuel_percent_changed.disconnect(_on_fuel_changed)

	_connected_car = car
	_connected_car.fuel_percent_changed.connect(_on_fuel_changed)
	# natychmiastowa weryfikacja stanu
	_on_fuel_changed(_calc_percent(car))

# ───────────────────────────────────────────────────────────────
#  GŁÓWNA LOGIKA WARUNKU
# ───────────────────────────────────────────────────────────────
func _on_fuel_changed(percent: float) -> void:
	if not _condition_met(percent):
		return
	var q := QuestSys.get_quest(quest_id)
	if q and q.status == QuestData.Status.ACTIVE:
		QuestSys.complete(quest_id)
		if _connected_car:
			_connected_car.fuel_percent_changed.disconnect(_on_fuel_changed)

func _condition_met(pct: float) -> bool:
	match relation:
		"<":
			return pct <  threshold_percent
		">":
			return pct >  threshold_percent
		"=":
			return abs(pct - threshold_percent) <= epsilon
		_:
			push_warning("FuelQuestWatcher: nieznany operator '" + relation + "'")
			return false

func _calc_percent(car: Car) -> float:
	if car.max_fuel <= 0.0:
		return 0.0
	return 100.0 * car.current_fuel / car.max_fuel
