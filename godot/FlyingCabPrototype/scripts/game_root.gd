extends Node
## Composition root: session outlives maps; loading and rendering do not own gameplay.
const CITY = preload("res://scenes/flight_lab.tscn")
var context: RuntimeContext
var maps := MapRouter.new()
var quality := RenderPolicy.new()
var telemetry: FrameTelemetry
var _overlay: CanvasLayer
var _message: Label
var _progress: ProgressBar
var _budget_elapsed := 0.0
var _save_elapsed := 0.0
@export var save_path := "user://taxi-session.json"

func _ready() -> void:
	context = RuntimeContext.new()
	var new_game := OS.get_cmdline_user_args().has("--new-game")
	# Return a string across the bridge: the Web template stalled on a bool.
	if OS.has_feature("web"):
		new_game = new_game or JavaScriptBridge.eval("new URLSearchParams(window.location.search).get('new') || ''") == "1"
	if not new_game and FileAccess.file_exists(save_path):
		context.load_from(save_path)
	if not context.rides.enabled:
		context.rides.enabled = true
		context.campaign.credits = preload("res://resources/taxi/default_rules.tres").starting_credits
	context.name = "Session"
	add_child(context)
	context.register_system(&"dialogue", DialogueSession.new())
	context.register_system(&"maps", maps)
	telemetry = FrameTelemetry.new()
	telemetry.context = context
	add_child(telemetry)
	_build_overlay()
	maps.context = context
	maps.host = self
	maps.register_map(&"city_02", CITY)
	quality.profile = "balanced" if OS.has_feature("web") or OS.has_feature("mobile") else "desktop"
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--quality=") and RenderPolicy.PROFILES.has(arg.trim_prefix("--quality=")):
			quality.profile = arg.trim_prefix("--quality=")
	if OS.has_feature("web"):
		var selected = JavaScriptBridge.eval("new URLSearchParams(window.location.search).get('quality') || ''")
		if selected is String and RenderPolicy.PROFILES.has(selected):
			quality.profile = selected
	get_window().size_changed.connect(func(): quality.resize(get_window()))
	# The interactive demo targets stable 60 Hz; offline measurement scripts load
	# the level directly and retain their own frame pacing.
	Engine.max_fps = 60
	call_deferred("_start")

func _start() -> void:
	await maps.enter(&"city_02", prepare_map)
	telemetry.begin()
	print("FLYING_CAB_READY build=", telemetry.build_id, " profile=", quality.profile, " scale_3d=", get_window().scaling_3d_scale)

func prepare_map(level: Node3D) -> void:
	_overlay.show()
	if level.has_method("prepare_gameplay"):
		await level.prepare_gameplay()
		if level.taxi_hud and not level.taxi_hud.save_requested.is_connected(_save):
			level.taxi_hud.save_requested.connect(_save)
			level.taxi.checkpoint_requested.connect(_schedule_save)
			level.on_foot.checkpoint_requested.connect(_schedule_save)
	quality.apply(get_window(), level, quality.profile)
	telemetry.profile = quality.profile
	var warmup := GraphicsWarmup.new()
	warmup.progressed.connect(func(fraction: float): _progress.value = fraction * 100)
	await warmup.prepare(level)
	var controls := level.get_node_or_null("HUD/Controls")
	if controls:
		controls.clear_controls()
	_overlay.hide()

func _process(dt: float) -> void:
	_save_elapsed += dt
	if _save_elapsed >= 20 and not maps.busy and not context.player.is_suspended():
		_save(false)
	_budget_elapsed += dt
	if _budget_elapsed >= 0.2 and is_instance_valid(maps.current) and not maps.busy:
		quality.budget_vehicle_lights(maps.current, context.player.focus)
		_budget_elapsed = 0

func _save(show_notice := true) -> void:
	if maps.busy or not is_instance_valid(maps.current):
		return
	var result := context.save_to(save_path)
	_save_elapsed = 0
	if maps.current.taxi and (show_notice or result != OK):
		maps.current.taxi.message("Zapisano. Po ponownym otwarciu wrócisz do tej gry." if result == OK else "Nie udało się zapisać gry na tym urządzeniu.")

func _schedule_save() -> void:
	# Let the recovery body's next physics step finish before capturing its pose.
	_save_elapsed = 19.5

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT and is_instance_valid(context):
		_save(false)

func _build_overlay() -> void:
	_overlay = CanvasLayer.new()
	_overlay.layer = 100
	add_child(_overlay)
	var backdrop := ColorRect.new()
	backdrop.color = Color("09151e")
	backdrop.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_overlay.add_child(backdrop)
	var layout := VBoxContainer.new()
	layout.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	layout.position = Vector2(-170, -70)
	layout.custom_minimum_size = Vector2(340, 140)
	backdrop.add_child(layout)
	_message = Label.new()
	_message.text = "FLYING CAB\nPrzygotowanie miasta"
	_message.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_message.add_theme_font_size_override("font_size", 22)
	layout.add_child(_message)
	_progress = ProgressBar.new()
	_progress.custom_minimum_size.y = 12
	_progress.show_percentage = false
	layout.add_child(_progress)
	var hint := Label.new()
	hint.text = "Za chwilę ruszamy."
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	layout.add_child(hint)
	var diagnostics := OS.get_cmdline_user_args().has("--diagnostics")
	if OS.has_feature("web"):
		diagnostics = JavaScriptBridge.eval("new URLSearchParams(window.location.search).get('diagnostics') || ''") == "1"
	if diagnostics:
		var layer := CanvasLayer.new()
		layer.layer = 90
		add_child(layer)
		var button := Button.new()
		button.text = "Zapisz pomiar"
		button.position = Vector2(20, 430)
		button.pressed.connect(telemetry.export_report)
		layer.add_child(button)
