extends Node
class_name FuelQuestWatcher

@export var quest_id: String
@export_range(0.0, 100.0, 1.0) var fuel_threshold_percent: float = 15.0

var _connected_car: Car = null
var _pc: PlayerCharacter = null          # ← DEKLARACJA, której brakowało

func _ready() -> void:
	_try_attach_player()

	# jeśli gracza jeszcze nie ma w grupie, ponawiamy próbę co 0.1 s
	if _pc == null:
		var t := Timer.new()
		t.one_shot = false
		t.wait_time = 0.1
		add_child(t)
		t.timeout.connect(_try_attach_player)
		t.start()

# ───────────────────────────────────────────────────────────────
#  SZUKANIE GRACZA I PODPINANIE SYGNAŁÓW
# ───────────────────────────────────────────────────────────────
func _try_attach_player() -> void:
	if _pc:                                 # już podłączony
		return

	var pc := get_tree().get_first_node_in_group("player")
	if pc:
		_pc = pc as PlayerCharacter
		print("FuelQuestWatcher: znalazłem gracza =", _pc)

		# reaguj na przyszłe wsiadanie do pojazdów
		_pc.entered_vehicle.connect(_on_player_entered_vehicle)

		# jeśli gracz już siedzi w pojeździe – podpinamy się natychmiast
		if _pc.in_vehicle and _pc.vehicle:
			_connect_car(_pc.vehicle)

		# można zatrzymać timer, bo cel osiągnięty
		var tim := get_parent() as Timer
		if tim:
			tim.queue_free()

func _on_player_entered_vehicle(car: Car) -> void:
	_connect_car(car)

# ───────────────────────────────────────────────────────────────
#  OBSŁUGA AUTA I PALIWA
# ───────────────────────────────────────────────────────────────
func _connect_car(car: Car) -> void:
	if _connected_car == car:
		return
	if _connected_car:
		_connected_car.fuel_percent_changed.disconnect(_on_fuel_changed)

	_connected_car = car
	_connected_car.fuel_percent_changed.connect(_on_fuel_changed)

	# natychmiast sprawdź stan przy wsiadaniu
	var pct := 100.0 * car.current_fuel / car.max_fuel
	_on_fuel_changed(pct)

func _on_fuel_changed(percent: float) -> void:
	var q := QuestSys.get_quest(quest_id)
	if not (q and q.status == QuestData.Status.ACTIVE):
		return

	if percent <= fuel_threshold_percent:
		print("FuelQuestWatcher: paliwo", percent, "% – zaliczam quest", quest_id)
		QuestSys.complete(quest_id)

		if _connected_car:
			_connected_car.fuel_percent_changed.disconnect(_on_fuel_changed)
