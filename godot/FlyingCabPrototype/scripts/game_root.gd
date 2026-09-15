extends Node
## Composition root: the start screen precedes creation of any live session.
const CITY = preload("res://scenes/flight_lab.tscn")
var context: RuntimeContext
var maps := MapRouter.new()
var quality := RenderPolicy.new()
var telemetry: FrameTelemetry
@onready var _overlay: GameStartScreen = $StartScreen
var _budget_elapsed := 0.0
var _save_elapsed := 0.0
var _launching := false
@export var save_path := "user://taxi-session.json"
@export var narrative_catalog: NarrativeCatalog = preload("res://resources/narrative/city_catalog.tres")

func _ready() -> void:
	_overlay.continue_requested.connect(continue_game)
	_overlay.new_game_requested.connect(restart_game)
	_overlay.show_menu(FileAccess.file_exists(save_path))
	quality.profile = "balanced" if OS.has_feature("web") or OS.has_feature("mobile") else "desktop"
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--quality=") and RenderPolicy.PROFILES.has(arg.trim_prefix("--quality=")):
			quality.profile = arg.trim_prefix("--quality=")
	if OS.has_feature("web"):
		var selected = JavaScriptBridge.eval("new URLSearchParams(window.location.search).get('quality') || ''")
		if selected is String and RenderPolicy.PROFILES.has(selected):
			quality.profile = selected
	get_window().size_changed.connect(func(): quality.resize(get_window()))
	Engine.max_fps = 60
	_build_diagnostics()
	# Explicit development shortcut. Normal F5 always shows the start screen.
	var new_game := OS.get_cmdline_user_args().has("--new-game")
	if OS.has_feature("web"):
		new_game = new_game or JavaScriptBridge.eval("new URLSearchParams(window.location.search).get('new') || ''") == "1"
	if new_game: restart_game()

func continue_game() -> void:
	_request_session(false)

func restart_game() -> void:
	_request_session(true)

func _request_session(fresh: bool) -> void:
	if _launching: return
	_launching = true
	if is_instance_valid(context): context.player.suspend(&"session_restart")
	# Leave the emitting button's input callback before freeing its map.
	_launch_session.call_deferred(fresh)

func _launch_session(fresh: bool) -> void:
	var candidate := RuntimeContext.new()
	candidate.narrative_catalog = narrative_catalog
	candidate.narrative.catalog = narrative_catalog
	if not fresh and not candidate.load_from(save_path):
		candidate.free()
		_launch_error("Nie można wczytać zapisu. Możesz rozpocząć nową grę.")
		return
	if not candidate.rides.enabled:
		candidate.rides.enabled = true
		candidate.campaign.credits = preload("res://resources/taxi/default_rules.tres").starting_credits
	if fresh:
		# Replace the autosave before loading the city, not at the next 20 s tick.
		# A failed write leaves the previous session and save intact.
		var result := candidate.save_to(save_path)
		if result != OK:
			candidate.free()
			_launch_error("Nie udało się zapisać nowej gry. Spróbuj ponownie.")
			return
	_overlay.show_loading()
	_dispose_session()
	context = candidate
	context.name = "Session"
	add_child(context)
	context.narrative.checkpoint_requested.connect(_narrative_checkpoint)
	maps = MapRouter.new()
	context.register_system(&"maps", maps)
	maps.context = context
	maps.host = self
	maps.register_map(&"city_02", CITY)
	telemetry = FrameTelemetry.new()
	telemetry.context = context
	add_child(telemetry)
	_save_elapsed = 0
	_budget_elapsed = 0
	await maps.enter(&"city_02", prepare_map)
	_launching = false
	telemetry.begin()
	_save(false)
	print("FLYING_CAB_READY build=", telemetry.build_id, " profile=", quality.profile, " scale_3d=", get_window().scaling_3d_scale)

func _launch_error(message: String) -> void:
	_launching = false
	if is_instance_valid(context) and is_instance_valid(maps.current):
		context.player.resume(&"session_restart")
		if maps.current.taxi: maps.current.taxi.message(message, 8)
	else:
		_overlay.show_menu(FileAccess.file_exists(save_path), message)

func _dispose_session() -> void:
	if is_instance_valid(context): context.dialogue.end()
	if is_instance_valid(maps.current): maps.current.free()
	if is_instance_valid(telemetry): telemetry.free()
	if is_instance_valid(context): context.free()
	context = null
	telemetry = null
	# Old map modals release their own locks during exit. The new session starts clean.
	get_tree().paused = false

func prepare_map(level: Node3D) -> void:
	if level.has_method("prepare_gameplay"):
		await level.prepare_gameplay()
		if level.taxi_hud and not level.taxi_hud.save_requested.is_connected(_save):
			level.taxi_hud.save_requested.connect(_save)
			level.taxi_hud.new_game_requested.connect(restart_game)
			level.taxi.checkpoint_requested.connect(_schedule_save)
			level.on_foot.checkpoint_requested.connect(_schedule_save)
	quality.apply(get_window(), level, quality.profile)
	telemetry.profile = quality.profile
	var warmup := GraphicsWarmup.new()
	warmup.progressed.connect(func(fraction: float): _overlay.progress.value = fraction * 100)
	await warmup.prepare(level)
	var controls := level.get_node_or_null("HUD/Controls")
	if controls: controls.clear_controls()
	_overlay.hide()

func _process(dt: float) -> void:
	if not is_instance_valid(context) or _launching: return
	_save_elapsed += dt
	if _save_elapsed >= 20 and not maps.busy and not context.player.is_suspended():
		_save(false)
	_budget_elapsed += dt
	if _budget_elapsed >= 0.2 and is_instance_valid(maps.current) and not maps.busy:
		quality.budget_vehicle_lights(maps.current, context.player.focus)
		_budget_elapsed = 0

func _save(show_notice := true) -> void:
	if _launching or not is_instance_valid(context) or maps.busy or not is_instance_valid(maps.current): return
	var result := context.save_to(save_path)
	_save_elapsed = 0
	if maps.current.taxi and (show_notice or result != OK):
		maps.current.taxi.message("Zapisano. Po ponownym otwarciu wrócisz do tej gry." if result == OK else "Nie udało się zapisać gry na tym urządzeniu.")

func _narrative_checkpoint(urgent: bool) -> void:
	if urgent:
		_save.call_deferred(false)
	else:
		_schedule_save()

func _schedule_save() -> void:
	_save_elapsed = maxf(_save_elapsed, 19.5)

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		_save(false)

func _build_diagnostics() -> void:
	var diagnostics := OS.get_cmdline_user_args().has("--diagnostics")
	if OS.has_feature("web"):
		diagnostics = JavaScriptBridge.eval("new URLSearchParams(window.location.search).get('diagnostics') || ''") == "1"
	if not diagnostics: return
	var layer := CanvasLayer.new()
	layer.layer = 90
	add_child(layer)
	var button := Button.new()
	button.text = "Zapisz pomiar"
	button.position = Vector2(20, 430)
	button.pressed.connect(func():
		if is_instance_valid(telemetry): telemetry.export_report())
	layer.add_child(button)
