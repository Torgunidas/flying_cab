class_name TaxiHud
extends Control
## Minimal flight UI; map and options own a temporary simulation pause.
signal save_requested
signal new_game_requested
signal foot_recovery_requested
var director: TaxiDirector
var controls: FlightControls
var overlay := ""
var reference_map: CityReferenceMap
var guidance: TaxiGuidanceArrow
var tracking_guidance: TaxiGuidanceArrow
var notice_bubble: TaxiNoticeBubble
var _font: Font
var _map_button := Rect2()
var _gear := Rect2()
var _close := Rect2()
var _resume := Rect2()
var _save := Rect2()
var _tow := Rect2()
var _cancel := Rect2()
var _new_game := Rect2()
var _fuel := Rect2()
var _fuel_pointers: Dictionary = {}
var _key_blocked := true
var _clock := 0.0
var _owns_pause := false
var _controls_visible := true
var _tow_armed := false
var _new_game_armed := false
var _last_warning := ""
var _styles: Dictionary = {}

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	_font = ThemeDB.fallback_font
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	reference_map = CityReferenceMap.new()
	reference_map.director = director
	add_child(reference_map)
	reference_map.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	guidance = TaxiGuidanceArrow.new()
	guidance.director = director
	add_child(guidance)
	guidance.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	tracking_guidance = TaxiGuidanceArrow.new()
	tracking_guidance.director = director
	tracking_guidance.tracked_map = reference_map
	tracking_guidance.color = CityReferenceMap.TRACK_COLOR
	add_child(tracking_guidance)
	tracking_guidance.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	notice_bubble = TaxiNoticeBubble.new()
	notice_bubble.director = director
	notice_bubble.guidance = guidance
	add_child(notice_bubble)
	notice_bubble.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	resized.connect(_layout)
	_layout()
	director.context.player.focus_changed.connect(_focus_changed)
	if not InputMap.has_action("taxi_fuel"):
		InputMap.add_action("taxi_fuel")
		var key := InputEventKey.new()
		key.physical_keycode = KEY_F
		InputMap.action_add_event("taxi_fuel", key)

func _layout() -> void:
	_map_button = Rect2(size.x - 108, 20, 44, 44)
	_gear = Rect2(size.x - 58, 20, 44, 44)
	_close = _gear
	_fuel = Rect2(size.x * 0.5 + 6, controls._footer_y - 50, (size.x - 56) * 0.5, 44)
	var width := minf(340, size.x - 48)
	var x := (size.x - width) * 0.5
	var y := maxf(130, size.y * 0.30)
	_resume = Rect2(x, y, width, 48)
	_save = Rect2(x, y + 60, width, 48)
	_tow = Rect2(x, y + 120, width, 48)
	_cancel = Rect2(x, y + 180, width, 48)
	_new_game = Rect2(x, y + 240, width, 48)
	_clear()
	queue_redraw()

func _clear() -> void:
	_fuel_pointers.clear()
	_key_blocked = true
	if director:
		director.fuel_requested = false

func _focus_changed(_previous: Node3D, current: Node3D) -> void:
	_clear()
	if current is WalkingActor:
		director.notice_remaining = 0
	queue_redraw()

func _notification(what: int) -> void:
	if what in [NOTIFICATION_APPLICATION_FOCUS_OUT, NOTIFICATION_WM_WINDOW_FOCUS_OUT]:
		_clear()
		Input.action_release("taxi_fuel")

func open_overlay(kind: String) -> void:
	if kind not in ["map", "options"] or director.context.player.is_suspended():
		return
	overlay = kind
	guidance.hide()
	tracking_guidance.hide()
	notice_bubble.hide()
	_tow_armed = false
	_new_game_armed = false
	_clear()
	controls.clear_controls()
	_controls_visible = controls.visible
	controls.hide()
	director.context.player.suspend(&"taxi_overlay")
	_owns_pause = not get_tree().paused
	get_tree().paused = true
	if kind == "map":
		reference_map.open()
	queue_redraw()

func close_overlay() -> void:
	if overlay.is_empty():
		return
	overlay = ""
	reference_map.hide()
	_clear()
	controls.clear_controls()
	controls.visible = _controls_visible
	if _owns_pause:
		get_tree().paused = false
	_owns_pause = false
	director.context.player.resume(&"taxi_overlay")
	queue_redraw()

func _exit_tree() -> void:
	if director and director.context.player.focus_changed.is_connected(_focus_changed):
		director.context.player.focus_changed.disconnect(_focus_changed)
	if not overlay.is_empty():
		close_overlay()

func _process(dt: float) -> void:
	if director == null or not director.initialized:
		return
	if director.context.player.is_suspended():
		_clear()
		return
	var key := Input.is_action_pressed("taxi_fuel")
	if _key_blocked:
		if not key:
			_key_blocked = false
		key = false
	director.fuel_requested = not _fuel_pointers.is_empty() or key
	_update_warning()
	_clock += dt
	if _clock >= 0.1:
		queue_redraw()
		_clock = 0

func _update_warning() -> void:
	if director.context.player.mode == &"on_foot":
		_last_warning = ""
		return
	var warning := ""
	if controls._autopilot:
		warning = "Non authorized out of grid movement. Forced return"
	elif controls._hull_ratio <= 0:
		warning = "Auto unieruchomione. Holowanie znajdziesz w opcjach."
	elif controls._fuel_ratio <= 0:
		warning = "Brak paliwa. Serwis w depocie i TORQUE."
	elif controls._ceiling_warning:
		warning = "Max alt reached"
	elif controls._fuel_ratio < 0.2:
		warning = "Mało paliwa. Serwis w depocie i TORQUE."
	if warning != _last_warning:
		_last_warning = warning
		if not warning.is_empty():
			director.message(warning, 5)

func _input(event: InputEvent) -> void:
	if director == null or not director.initialized:
		return
	if director.context.player.is_suspended() and overlay.is_empty():
		return
	var modal := not overlay.is_empty()
	if event is InputEventScreenTouch:
		if event.pressed and not event.canceled:
			press(event.position, event.index)
		else:
			_fuel_pointers.erase(event.index)
			reference_map.end_hold(event.index)
	elif event is InputEventScreenDrag:
		reference_map.move_hold(event.position, event.index)
		if _fuel_pointers.has(event.index) and not _fuel.has_point(event.position):
			_fuel_pointers.erase(event.index)
	elif event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed:
			press(event.position, -1)
		else:
			_fuel_pointers.erase(-1)
			reference_map.end_hold(-1)
	elif event is InputEventMouseMotion:
		reference_map.move_hold(event.position, -1)
	elif event is InputEventKey and event.pressed and not event.echo:
		if event.physical_keycode in [KEY_M, KEY_ESCAPE]:
			if modal:
				close_overlay()
			else:
				open_overlay("map" if event.physical_keycode == KEY_M else "options")
			get_viewport().set_input_as_handled()
		elif event.physical_keycode == KEY_F5:
			save_requested.emit()
			close_overlay()
			get_viewport().set_input_as_handled()
	if modal:
		get_viewport().set_input_as_handled()

func press(point: Vector2, pointer: int) -> void:
	var handled := true
	if not overlay.is_empty():
		if _close.has_point(point) or (overlay == "options" and _resume.has_point(point)):
			close_overlay()
		elif overlay == "map":
			reference_map.begin_hold(point, pointer)
		elif _save.has_point(point):
			save_requested.emit()
			close_overlay()
		elif _tow.has_point(point):
			if director.context.player.mode == &"on_foot":
				close_overlay()
				foot_recovery_requested.emit()
			elif _tow_armed:
				close_overlay()
				director.request_recovery(true)
			else:
				_tow_armed = true
		elif _cancel.has_point(point) and not director.context.rides.active.is_empty():
			close_overlay()
			director.abort()
		elif _new_game.has_point(point) and new_game_requested.has_connections():
			if _new_game_armed:
				close_overlay()
				new_game_requested.emit()
			else:
				_new_game_armed = true
	elif director.context.player.is_suspended():
		return
	elif _map_button.has_point(point):
		open_overlay("map")
	elif _gear.has_point(point):
		open_overlay("options")
	elif _can_refuel() and _fuel.has_point(point):
		_fuel_pointers[pointer] = true
	else:
		handled = false
	if handled:
		get_viewport().set_input_as_handled()
	queue_redraw()

func _box(rect: Rect2, color := Color("34515e")) -> void:
	var key := color.to_rgba32()
	if not _styles.has(key):
		var style := StyleBoxFlat.new()
		style.bg_color = Color(0.025, 0.055, 0.075, 0.94)
		style.border_color = color
		style.set_border_width_all(1)
		style.set_corner_radius_all(10)
		_styles[key] = style
	draw_style_box(_styles[key], rect)

func _text(point: Vector2, value: String, height := 13, color := Color("e9f1ee")) -> void:
	draw_string(_font, point, value, HORIZONTAL_ALIGNMENT_LEFT, -1, height, color)

func _draw() -> void:
	if director == null or not director.initialized or controls == null:
		return
	if not overlay.is_empty():
		_draw_overlay()
		return
	_box(_map_button)
	_box(_gear)
	_draw_icon(_map_button.get_center(), false)
	_draw_icon(_gear.get_center(), true)
	if _can_refuel():
		_box(_fuel, Color("ffc176"))
		_text(_fuel.position + Vector2(12, 18), "PALIWO / F · %.0f CR/j." % director.rules.fuel_price, 11, Color("ffc176"))
		_text(_fuel.position + Vector2(12, 34), "TRZYMAJ" if director.context.campaign.credits > 0 else "BRAK KREDYTÓW", 10, Color("8babae"))

func _can_refuel() -> bool:
	var cab := director.context.player.focus as FlightCab
	return director.fuel_station != null and cab != null and cab.fuel < cab.definition.fuel_capacity - 0.00001

func _draw_icon(center: Vector2, gear: bool) -> void:
	var color := Color("d2e3e3")
	if gear:
		draw_circle(center, 8, color, false, 2, true)
		draw_circle(center, 3, color, false, 2, true)
		for i in range(8):
			var direction := Vector2.from_angle(i * TAU / 8)
			draw_line(center + direction * 9, center + direction * 12, color, 3, true)
	else:
		var points := PackedVector2Array([Vector2(-12, -8), Vector2(-4, -11), Vector2(4, -8), Vector2(12, -11), Vector2(12, 8), Vector2(4, 11), Vector2(-4, 8), Vector2(-12, 11), Vector2(-12, -8)])
		for i in range(points.size()):
			points[i] += center
		draw_polyline(points, color, 1.8, true)
		draw_line(center + Vector2(-4, -11), center + Vector2(-4, 8), color, 1.5, true)
		draw_line(center + Vector2(4, -8), center + Vector2(4, 11), color, 1.5, true)

func _draw_overlay() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), Color(0.025, 0.045, 0.065, 0.98))
	_text(Vector2(26, 49), "MIASTO" if overlay == "map" else "OPCJE", 22)
	_box(_close)
	var center := _close.get_center()
	draw_line(center - Vector2(7, 7), center + Vector2(7, 7), Color("e9f1ee"), 2, true)
	draw_line(center - Vector2(-7, 7), center + Vector2(-7, 7), Color("e9f1ee"), 2, true)
	if overlay == "map":
		var trip := director.context.rides.active
		var destination: TaxiStop = director.network.stops.get(trip.get("destination", ""))
		_text(Vector2(26, 89), "Cel: " + destination.display_name if destination else "Pasażerowie czekają na tarasach.", 14, Color("71e5e4"))
		return
	for rect in [_resume, _save, _tow]:
		_box(rect)
	_text(_resume.position + Vector2(18, 30), "Wróć do gry", 16)
	_text(_save.position + Vector2(18, 30), "Zapisz grę", 16)
	var recovery_text := "Potwierdź holowanie · %.0f CR" % director.rules.tow_fee if _tow_armed else "Holuj do depotu · %.0f CR" % director.rules.tow_fee
	if director.context.player.mode == &"on_foot":
		recovery_text = "Wróć do miejsca wysiadania"
	_text(_tow.position + Vector2(18, 30), recovery_text, 15, Color("ffc176"))
	if not director.context.rides.active.is_empty():
		_box(_cancel)
		_text(_cancel.position + Vector2(18, 30), "Anuluj kurs", 16)
	if new_game_requested.has_connections():
		_box(_new_game, Color("dd64bc"))
		_text(_new_game.position + Vector2(18, 30), "Potwierdź nową grę" if _new_game_armed else "Nowa gra", 16)
	var y := (_new_game.end.y if new_game_requested.has_connections() else _cancel.end.y) + 32
	_text(Vector2(_save.position.x, y), "Kursy: %d    Zarobek: %.2f CR" % [director.context.rides.completed, director.context.rides.income], 12, Color("8babae"))
	if director.context.rides.recovery_debt > 0:
		_text(Vector2(_save.position.x, y + 22), "Dług: %.2f CR" % director.context.rides.recovery_debt, 12, Color("ffc176"))
	if _tow_armed:
		_text(Vector2(_save.position.x, y + 50), "Brakującą kwotę spłacisz z kolejnych kursów.", 12, Color("8babae"))
	elif _new_game_armed:
		_text(Vector2(_save.position.x, y + 50), "Zastąpi bieżący zapis i cały postęp.", 12, Color("ffa8dc"))
