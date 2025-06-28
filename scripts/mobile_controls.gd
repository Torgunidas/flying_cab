extends CanvasLayer
class_name MobileControls      # ← pozwala w Inspektorze widzieć klasę

# ───────────────────────────── 1. EXPORTY ─────────────────────────────
@export var hp_label_path      : NodePath      # Label z HP auta
@export var fuel_label_path    : NodePath      # Label z paliwem
@export var money_label_path   : NodePath      # Label z kasą gracza
@export var life_label_path    : NodePath      # Label z życiem gracza

@export var car_status_path    : NodePath      # HBox z HP+Fuel
@export var player_status_path : NodePath      # HBox z Life+Money
@export var enter_button_path  : NodePath      # Przycisk „enter / interact”
@export var maya_time_label_path : NodePath    # Label z minutami gry
@export var death_screen_path    : NodePath    # VBox z ekranem śmierci
@export var retry_button_path    : NodePath    # Przycisk restartu gry
@export var timer_minutes        : int = 5     # Startowy czas gry (minuty)

# ───────────────────────── 2. ON-READY REFERENCJE ─────────────────────
@onready var hp_label      : Label   = get_node_or_null(hp_label_path)
@onready var fuel_label    : Label   = get_node_or_null(fuel_label_path)
@onready var money_label   : Label   = get_node_or_null(money_label_path)
@onready var life_label    : Label   = get_node_or_null(life_label_path)

@onready var car_status_box    : Control = get_node_or_null(car_status_path)
@onready var player_status_box : Control = get_node_or_null(player_status_path)
@onready var enter_button      : TouchScreenButton  = get_node_or_null(enter_button_path)
@onready var maya_time_label   : Label   = get_node_or_null(maya_time_label_path)
@onready var death_screen      : Control = get_node_or_null(death_screen_path)
@onready var retry_button      : Button  = get_node_or_null(retry_button_path)

var _countdown_timer : Timer
var _seconds_left : int = 0

# ───────────────────────────── 3. READY ───────────────────────────────
func _ready() -> void:
	_assert_widgets()
	set_mode("foot")
	if enter_button:
			enter_button.visible = false
	if death_screen:
			death_screen.visible = false
	if retry_button:
			retry_button.pressed.connect(_on_retry_pressed)
	_start_timer()

	# ① spróbuj pobrać autoload o nazwie „GameState”
	var gs : Node = get_node_or_null("/root/GameState")

	# ② jeśli był nazwany inaczej – wypisz ostrzeżenie
	if gs == null:
		push_warning("Nie znaleziono autoloadu 'GameState' w /root/ – sprawdź nazwę w Project Settings → Autoload.")
		return

	# ③ od razu pokaż saldo i podłącz się pod sygnał
	update_money_display(gs.get_money())
	gs.money_changed.connect(update_money_display)   # ← bez self., bo to Callable w Godot 4


# ───────────────────────── 4. PUBLICZNE METODY HUD ────────────────────
func update_hp_display(current: int, max_val: int) -> void:
	if hp_label:
		hp_label.text = "HP: %d / %d" % [current, max_val]

func update_fuel_display(current: float, max_val: float) -> void:
	if fuel_label:
		var pct := int((current / max_val) * 100.0)
		fuel_label.text = "Fuel: %d%%" % pct

func update_life_display(current: int, max_val: int) -> void:
	if life_label:
		life_label.text = "♥ %d / %d" % [current, max_val]

func update_money_display(amount: int) -> void:
	if money_label:
		money_label.text = "$ %d" % amount

func set_enter_visible(visible: bool) -> void:
	if enter_button:
				enter_button.visible = visible

func _start_timer() -> void:
		_seconds_left = timer_minutes * 60
		_update_maya_time_display()
		_countdown_timer = Timer.new()
		_countdown_timer.wait_time = 1.0
		_countdown_timer.autostart = true
		_countdown_timer.one_shot = false
		add_child(_countdown_timer)
		_countdown_timer.timeout.connect(_on_timer_tick)

func _on_timer_tick() -> void:
		_seconds_left -= 1
		_update_maya_time_display()
		if _seconds_left <= 0:
				_countdown_timer.stop()
				_on_time_expired()

func _update_maya_time_display() -> void:
		if maya_time_label:
				maya_time_label.text = str(int(_seconds_left / 60))

func _on_time_expired() -> void:
	if death_screen:
		death_screen.visible = true
	get_tree().paused = true          # zamiast „pauza”

func _on_retry_pressed() -> void:
	if death_screen:
		death_screen.visible = false
	get_tree().paused = false
	get_tree().change_scene_to_file("res://scenes/ui_scenes/MainMenu.tscn")

# ───────────────────────── 5. PRZEŁĄCZANIE TRYBU ─────────────────────
func set_mode(mode: String) -> void:
	match mode:
		"vehicle":
			if car_status_box:
				car_status_box.visible = true
		"foot":
			if car_status_box:
				car_status_box.visible = false
		_:
			push_warning("MobileControls.set_mode(): unknown mode '%s'" % mode)

# ───────────────────────── 6. WALIDACJA ŚCIEŻEK ──────────────────────
func _assert_widgets() -> void:
	assert(hp_label,          "hp_label_path nieprzypisany!")
	assert(fuel_label,        "fuel_label_path nieprzypisany!")
	assert(money_label,       "money_label_path nieprzypisany!")
	assert(life_label,        "life_label_path nieprzypisany!")
	assert(car_status_box,    "car_status_path nieprzypisany!")
	assert(player_status_box, "player_status_path nieprzypisany!")
	assert(enter_button,      "enter_button_path nieprzypisany!")
	assert(maya_time_label,   "maya_time_label_path nieprzypisany!")
	assert(death_screen,      "death_screen_path nieprzypisany!")
	assert(retry_button,      "retry_button_path nieprzypisany!")
