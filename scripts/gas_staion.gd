extends Area2D
class_name GasStation

# ─────────────────────────────────────────────────────────────
#  KONFIGURACJA
# ─────────────────────────────────────────────────────────────
@export var price_per_10 : int   = 5         # $ za 10 jednostek paliwa
@export var refuel_speed : float = 150.0     # jednostek paliwa na sekundę

# ─────────────────────────────────────────────────────────────
#  ZMIENNE ROBOCZE
# ─────────────────────────────────────────────────────────────
var _player : PlayerCharacter = null
var _car    : CharacterBody2D = null
var _dialog : DialogManager   = null
var _refuel : bool            = false

# ─────────────────────────────────────────────────────────────
#  READY
# ─────────────────────────────────────────────────────────────
func _ready() -> void:
	monitoring = true
	body_entered.connect(_on_body_entered)
	body_exited.connect(_on_body_exited)
	add_to_group("gasstations")

# ─────────────────────────────────────────────────────────────
#  WEJŚCIE / WYJŚCIE AUTA
# ─────────────────────────────────────────────────────────────
func _on_body_entered(body: Node) -> void:
	_player = get_tree().get_first_node_in_group("player") as PlayerCharacter
	if _player and _player.in_vehicle and body == _player.vehicle:
		_car = body as CharacterBody2D
		_open_dialog_dynamic()

func _on_body_exited(body: Node) -> void:
	if body == _car:
		_car    = null
		_refuel = false
		_close_dialog()

# ─────────────────────────────────────────────────────────────
#  DIALOG – dynamiczne budowanie z DialogBook systemem
# ─────────────────────────────────────────────────────────────
func _open_dialog_dynamic() -> void:
	_dialog = get_tree().get_current_scene().get_node_or_null("DialogManager") as DialogManager
	if _dialog == null or _dialog.is_open():
		return
	if _car == null:
		return

	var gs      : Node  = get_node("/root/GameState")
	var money   : int   = gs.get_money()
	var need_fu : float = _car.max_fuel - _car.current_fuel
	var cost_fu : int   = int(ceil(need_fu / 10.0) * price_per_10)

	# -------- budujemy DialogData w locie --------
	var dlg := DialogData.new()
	dlg.npc_text = "Welcome on AutoGas! State your request."

	if money < price_per_10:
		dlg.answers.append(_make_answer("Get some money, babe.", "close"))
	else:
		if need_fu > 0.0:
			dlg.answers.append(_make_answer("Fill her up – Cost: %d$" % cost_fu, "start_animation", "refuel_full"))
		dlg.answers.append(_make_answer("Gas for 10$", "start_animation", "refuel_10"))
		dlg.answers.append(_make_answer("Never mind.", "close"))

	_dialog.call_deferred("start_dialog", dlg, self)

func _make_answer(text: String, action_type: String, anim: String = "") -> DialogAnswer:
	var a := DialogAnswer.new()
	a.text = text
	var act := DialogAction.new()
	act.type = action_type
	if action_type == "start_animation":
		act.anim_name = anim
	a.actions.append(act)
	return a

func _close_dialog() -> void:
	if _dialog and _dialog.is_open():
		_dialog.close_dialog()

# ─────────────────────────────────────────────────────────────
#  AKCJE Z DIALOGU / ANIMACJA
# ─────────────────────────────────────────────────────────────
# anim – nazwa animacji w AnimationPlayer
# loops: 1  → raz (domyślnie)
#        n>1 → n‑krotnie
#        -1 → zapętlaj w nieskończoność
# ─────────────────────────────────────────────────────────────
#  AKCJE Z DIALOGU / ANIMACJA
# ─────────────────────────────────────────────────────────────
# anim – nazwa animacji w AnimationPlayer
# loops: 1  → raz (domyślnie)
#        n>1 → n‑krotnie
#        -1 → zapętlaj w nieskończoność
# ─────────────────────────────────────────────────────────────
#  AKCJE Z DIALOGU / ANIMACJA
# ─────────────────────────────────────────────────────────────
# anim  – nazwa animacji w AnimationPlayer
# loops – 1  → raz (domyślnie)
#          n>1 → dokładnie n‑krotnie
#          -1 → zapętlaj w nieskończoność
# → teraz mamy własną funkcję tankowania
func play_refuel_animation(anim: String, loops: int = 1) -> void:
	match anim:
		"refuel_full":
			_refuel = true                 # płynne tankowanie
		"refuel_10":
			_instant_refuel(10)           # instant +10
		_:
			pass


func _instant_refuel(cost_cash: int) -> void:
	if _car == null:
		return
	var gs : Node = get_node("/root/GameState")
	if gs.get_money() < cost_cash:
		return
	var units : float = cost_cash / price_per_10 * 10.0
	var need  : float = _car.max_fuel - _car.current_fuel
	units = min(units, need)
	if units <= 0.0:
		return

	_car.current_fuel += units
	gs.spend_money(cost_cash)

	var ui : MobileControls = get_tree().get_current_scene().get_node_or_null("MobileControls") as MobileControls
	if ui:
		ui.update_fuel_display(_car.current_fuel, _car.max_fuel)

# ─────────────────────────────────────────────────────────────
#  PŁYNNE TANKOWANIE
# ─────────────────────────────────────────────────────────────
# ─────────────────────────────────────────────────────────────
func _physics_process(delta: float) -> void:
	if _refuel and _car and _player and _player.in_vehicle:
		_refuel_logic(delta)

func _refuel_logic(delta: float) -> void:
	var need : float = _car.max_fuel - _car.current_fuel
	if need <= 0.0:
		_refuel = false
		return

	var fill : float = min(refuel_speed * delta, need)
	var cost : int   = int(fill / 10.0 * price_per_10)

	var gs : Node = get_node("/root/GameState")
	if cost > gs.get_money():
		_refuel = false
		return

	_car.current_fuel += fill
	gs.spend_money(cost)

	var ui : MobileControls = get_tree().get_current_scene().get_node_or_null("MobileControls") as MobileControls
	if ui:
		ui.update_fuel_display(_car.current_fuel, _car.max_fuel)
