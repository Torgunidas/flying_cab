class_name FlightControls
extends Control

signal reset_requested

const INK := Color("0b1724")
const CYAN := Color("71e5e4")
const WHITE := Color("e9f1ee")
const MUTED := Color("8babae")
const AMBER := Color("ffc176")
const SMOG = preload("res://resources/smog.tres")
var taxi_mode := false:
	set(value):
		taxi_mode = value
		if is_node_ready():
			_layout()
var command := Vector2.ZERO
var repair_held := false
var _repair_rect := Rect2()
var _repair_available := false
var _repair_name := ""
var _repair_price := 0.0
var _repairing := false
var _hull_ratio := 1.0
var _credits := 0.0
var _passengers := 0
var _seats := 0
var _damage_flash := false
var _pointers: Dictionary = {}
var _keys_blocked_until_release := false
var _mouse_down := false
var _readout := ""
var _altitude := ""
var _status := "GOTOWY DO LOTU"
var _font: Font
var _controls: Array[Rect2] = []
var _reset_rect := Rect2()
var _footer_y := 0.0
var _safe_bottom := 22.0
var _autopilot := false
var _external_lock := false
var _control_mode: StringName = &"flight"
var _world: WorldRegistry
var _styles: Dictionary = {}
var _fuel_ratio := 1.0
var _ceiling_warning := false
var _refueling := false
var _restore_notice := 0.0
var _district_name := "FOUNDRY / CAB DEPOT"
var _city_position := Vector2.ZERO
var _city_bounds := Rect2(-74.5, -48, 150, 368)
var _low_city_top := 11.0
var _city_lanes: Array[Node] = []
var _city_pads: Array[Node] = []
var _highway_speed := 1.0
var _automatic_lights := false

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	_font = ThemeDB.fallback_font
	_install_keys()
	resized.connect(_layout)
	_layout()

func bind_world(registry: WorldRegistry) -> void:
	if _world and _world.changed.is_connected(_world_changed):
		_world.changed.disconnect(_world_changed)
	_world = registry
	_world.changed.connect(_world_changed)
	_world_changed()

func _world_changed() -> void:
	_city_lanes.assign(_world.lanes)
	_city_pads.assign(_world.pads)
	_city_bounds = _world.definition.bounds()
	var city := _world.definition as CityDefinition
	_low_city_top = city.low_city_top if city else _city_bounds.position.y
	queue_redraw()

func update_city(position: Vector3, definition: MapDefinition, highway_speed: float, headlights_on := false) -> void:
	_city_position = Vector2(position.x, position.y)
	_city_bounds = definition.bounds()
	_highway_speed = highway_speed
	_automatic_lights = headlights_on
	_district_name = definition.district_at(position, SMOG.top_height)
	queue_redraw()

func set_control_suspended(value: bool) -> void:
	_external_lock = value
	clear_controls()

func _install_keys() -> void:
	var actions := {"flight_left": [KEY_A, KEY_LEFT], "flight_right": [KEY_D, KEY_RIGHT], "flight_up": [KEY_W, KEY_UP, KEY_SPACE], "flight_reset": [KEY_R], "vehicle_repair": [KEY_E]}
	for action: String in actions:
		if not InputMap.has_action(action):
			InputMap.add_action(action)
			for key: int in actions[action]:
				var event := InputEventKey.new()
				event.physical_keycode = key
				InputMap.action_add_event(action, event)

func _layout() -> void:
	# Portrait controls retain the old game's two left-thumb arrows + right-thumb thrust.
	var usable := minf(size.x, 660.0)
	var margin := (size.x - usable) * 0.5 + 18.0
	var side := usable * 0.19
	var thrust := usable * 0.25
	var bottom := size.y - _safe_bottom
	_controls = [Rect2(margin, bottom - side, side, side), Rect2(margin + side + 12.0, bottom - side, side, side), Rect2(size.x - margin - thrust, bottom - thrust, thrust, thrust)]
	_reset_rect = Rect2() if taxi_mode else Rect2(size.x - 105.0, 24.0, 85.0, 42.0)
	_footer_y = bottom - thrust - 25.0
	_repair_rect = Rect2(22, _footer_y - 50, (size.x - 56) * 0.5, 44) if taxi_mode else Rect2(22, _footer_y - 115, size.x - 44, 82)
	clear_controls()
	queue_redraw()

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT or what == NOTIFICATION_WM_WINDOW_FOCUS_OUT:
		clear_controls()
		for action in ["flight_left", "flight_right", "flight_up", "vehicle_repair"]:
			Input.action_release(action)

func clear_controls() -> void:
	_pointers.clear()
	_mouse_down = false
	command = Vector2.ZERO
	repair_held = false
	_keys_blocked_until_release = true
	queue_redraw()

func _button_at(pos: Vector2) -> int:
	if _repair_available and (not taxi_mode or _hull_ratio < 0.99999) and _repair_rect.has_point(pos):
		return 3
	for i in range(_controls.size()):
		if _controls[i].has_point(pos):
			return i
	return -1

func _input(event: InputEvent) -> void:
	if _external_lock:
		return
	if event is InputEventScreenTouch:
		if event.pressed:
			if _reset_rect.has_point(event.position):
				reset_requested.emit()
			elif _button_at(event.position) >= 0:
				_pointers[event.index] = event.position
		else:
			_pointers.erase(event.index)
	elif event is InputEventScreenDrag:
		if _pointers.has(event.index):
			_pointers[event.index] = event.position
	elif event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		_mouse_down = event.pressed
		if event.pressed and _reset_rect.has_point(event.position):
			reset_requested.emit()
		elif event.pressed and _button_at(event.position) >= 0:
			_pointers[-1] = event.position
		else:
			_pointers.erase(-1)
	elif event is InputEventMouseMotion and _mouse_down and _pointers.has(-1):
		_pointers[-1] = event.position
	elif event.is_action_pressed("flight_reset") and not event.is_echo():
		reset_requested.emit()
	_update_command()

func _process(dt: float) -> void:
	if _restore_notice > 0.0:
		_restore_notice = maxf(0.0, _restore_notice - dt)
		if _restore_notice == 0.0:
			queue_redraw()
	_update_command()

func set_autopilot(active: bool) -> void:
	if _autopilot == active:
		return
	_autopilot = active
	_restore_notice = 0.0 if active else 2.5
	clear_controls()

func update_flight_status(fuel_ratio: float, ceiling_warning: bool, refueling: bool) -> void:
	_fuel_ratio = clampf(fuel_ratio, 0.0, 1.0)
	_ceiling_warning = ceiling_warning
	_refueling = refueling
	queue_redraw()

func _update_command() -> void:
	var old := command
	var pressed := [false, false, false, false]
	for pos: Vector2 in _pointers.values():
		var i := _button_at(pos)
		if i >= 0:
			pressed[i] = true
	var keyboard := Vector2(Input.get_axis("flight_left", "flight_right"), Input.get_action_strength("flight_up"))
	var repair_key := Input.is_action_pressed("vehicle_repair")
	if _keys_blocked_until_release:
		var any_key := Input.is_action_pressed("flight_left") or Input.is_action_pressed("flight_right") or Input.is_action_pressed("flight_up") or repair_key
		if not any_key:
			_keys_blocked_until_release = false
		keyboard = Vector2.ZERO
		repair_key = false
	command = Vector2(clampf(keyboard.x + float(pressed[1]) - float(pressed[0]), -1.0, 1.0), maxf(keyboard.y, float(pressed[2])))
	repair_held = _repair_available and (repair_key or pressed[3])
	if _autopilot or _external_lock:
		command = Vector2.ZERO
		repair_held = false
	if command != old:
		queue_redraw()

func update_readout(speed: float, altitude: float, on_ground: bool) -> void:
	var speed_text := "%02d" % roundi(speed * 3.6)
	var alt_text := "%03d" % maxi(0, roundi(altitude))
	var status := "NA PODŁOŻU" if on_ground else "W LOCIE"
	if speed_text != _readout or alt_text != _altitude or status != _status:
		_readout = speed_text
		_altitude = alt_text
		_status = status
		queue_redraw()

func _text(pos: Vector2, value: String, font_size: int, color: Color) -> void:
	draw_string(_font, pos, value, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color)

func _box(rect: Rect2, color: Color, border: Color, radius: int = 16) -> void:
	var key := "%s/%s/%d" % [color.to_rgba32(), border.to_rgba32(), radius]
	if not _styles.has(key):
		var created := StyleBoxFlat.new()
		created.bg_color = color
		created.border_color = border
		created.set_border_width_all(1)
		created.set_corner_radius_all(radius)
		_styles[key] = created
	var style: StyleBoxFlat = _styles[key]
	draw_style_box(style, rect)

func _draw() -> void:
	if not _font or _controls.is_empty():
		return
	if taxi_mode:
		_draw_compact_status()
	else:
		_box(Rect2(12, 14, size.x - 24, 184), Color(0.025, 0.04, 0.065, 0.83), Color(1, 0.3, 0.2, 0.85) if _damage_flash else Color(0.4, 0.7, 0.72, 0.15), 12)
		_text(Vector2(22, 38), "FLYING CAB", 20, WHITE)
		_text(Vector2(23, 58), _district_name, 11, CYAN)
		_box(_reset_rect, Color(0.04, 0.09, 0.14, 0.88), Color(0.4, 0.7, 0.72, 0.35), 10)
		_text(_reset_rect.position + Vector2(17, 27), "HOLUJ" if taxi_mode else "RESET", 13, WHITE)
		_text(Vector2(23, 99), _readout, 32, WHITE)
		_text(Vector2(69, 98), "km/h", 12, MUTED)
		_text(Vector2(130, 99), _altitude, 32, WHITE)
		_text(Vector2(191, 98), "m", 12, MUTED)
		_text(Vector2(23, 120), _status, 10, CYAN)
		if _highway_speed > 1.01:
			_text(Vector2(240, 120), "EXPRESS  ×%.1f" % _highway_speed, 11, CYAN)
		_draw_flight_status()
		if _control_mode == &"flight":
			var hull_color := AMBER if _hull_ratio < 0.35 else CYAN
			_text(Vector2(23, 180), "HULL %03d%%    %s CR    PAX %d/%d" % [ceili(_hull_ratio * 100), "%.2f" % _credits if taxi_mode else str(floori(_credits)), _passengers, _seats], 12, hull_color)
			draw_rect(Rect2(23, 187, (size.x - 46) * _hull_ratio, 4), hull_color)
			if _repair_available:
				_box(_repair_rect, Color(0.025, 0.11, 0.10, 0.94), CYAN, 12)
				_text(_repair_rect.position + Vector2(14, 23), _repair_name, 13, CYAN)
				var repair_text := "PRZYTRZYMAJ: NAPRAWA / E · %.1f CR/HP" % _repair_price
				if _hull_ratio >= 0.99999:
					repair_text = "AUTO SPRAWNE"
				elif _repair_price > 0 and _credits <= 0.0:
					repair_text = "BRAK KREDYTÓW NA NAPRAWĘ"
				elif _repairing:
					repair_text = "NAPRAWA W TOKU — TRZYMAJ / E"
				_text(_repair_rect.position + Vector2(14, 56), repair_text, 13, WHITE)
		var prompt := "Przytrzymaj ↑, aby wzlecieć. Puść, aby opadać."
		if _hull_ratio <= 0.0 and _control_mode == &"flight":
			prompt = "Auto unieruchomione. HOLUJ / R: powrót do depotu."
		elif _autopilot:
			prompt = "AUTOPILOT / GRID CONTROL"
		var tw := _font.get_string_size(prompt, HORIZONTAL_ALIGNMENT_LEFT, -1, 12).x
		_text(Vector2((size.x - tw) * 0.5, _footer_y), prompt, 12, WHITE)
	for i in range(3):
		var rect := _controls[i]
		var active := command.x < 0 if i == 0 else (command.x > 0 if i == 1 else command.y > 0)
		_box(rect, Color(0.02, 0.04, 0.06, 0.75) if _autopilot else (Color(0.2, 0.61, 0.63, 0.9) if active else Color(0.035, 0.09, 0.13, 0.86)), CYAN if active else Color(0.3, 0.56, 0.6, 0.55), 22)
		var center := rect.get_center() - Vector2(0, 8)
		var direction := Vector2.LEFT if i == 0 else (Vector2.RIGHT if i == 1 else Vector2.UP)
		var across := direction.orthogonal()
		draw_polyline(PackedVector2Array([center - direction * 6 + across * 12, center + direction * 6, center - direction * 6 - across * 12]), WHITE, 3.0, true)
		var label := "A" if i == 0 else ("D" if i == 1 else "CIĄG / W")
		var width := _font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, 11).x
		_text(Vector2(center.x - width * 0.5, rect.end.y - 18), label, 11, WHITE if active else MUTED)

func _draw_compact_status() -> void:
	var width := minf(228, size.x - 134)
	_box(Rect2(14, 16, width, 54), Color(0.025, 0.04, 0.065, 0.75), Color(1, 0.3, 0.2, 0.85) if _damage_flash else Color(0.4, 0.7, 0.72, 0.12), 10)
	_text(Vector2(24, 37), "%.0f CR" % _credits, 15, WHITE)
	_text(Vector2(118, 37), "PAX %d/%d" % [_passengers, _seats], 12, MUTED)
	_text(Vector2(24, 58), "PALIWO %d%%" % ceili(_fuel_ratio * 100), 11, AMBER if _fuel_ratio < 0.2 else CYAN)
	_text(Vector2(142, 58), "AUTO %d%%" % ceili(_hull_ratio * 100), 11, AMBER if _hull_ratio < 0.35 else CYAN)
	if _repair_available and _hull_ratio < 0.99999:
		_box(_repair_rect, Color(0.025, 0.07, 0.09, 0.85), CYAN, 10)
		_text(_repair_rect.position + Vector2(12, 18), "NAPRAW / E · %.0f CR/HP" % _repair_price, 11, CYAN)
		_text(_repair_rect.position + Vector2(12, 34), "TRZYMAJ" if _credits > 0 else "BRAK KREDYTÓW", 10, MUTED)

func _draw_flight_status() -> void:
	if _control_mode != &"flight":
		return
	var fuel_color := AMBER if _fuel_ratio < 0.2 else CYAN
	_text(Vector2(23, 143), "FUEL  %03d%%" % ceili(_fuel_ratio * 100.0), 12, fuel_color)
	if _refueling:
		_text(Vector2(140, 143), "REFUELING", 12, CYAN)
	if _automatic_lights:
		_text(Vector2(300, 143), "LIGHTS / AUTO", 11, CYAN)
	var bar := Rect2(23, 151, size.x - 46, 5)
	draw_rect(bar, Color(0.02, 0.04, 0.06, 0.85))
	draw_rect(Rect2(bar.position, Vector2(bar.size.x * _fuel_ratio, bar.size.y)), fuel_color)
	var lines: Array[String] = []
	if _autopilot:
		lines.assign(["Non authorized out of grid movement.", "Forced return"])
	elif _ceiling_warning:
		lines.assign(["Max alt reached", "Climb limited / increased thrust fuel use"])
	elif _fuel_ratio <= 0.0:
		lines.assign(["Fuel empty", "Depot / Torque: paliwo. HOLUJ / R w razie potrzeby." if taxi_mode else "Land on a pad to refuel"])
	elif _restore_notice > 0.0:
		lines.assign(["Pilot control restored", "Release and press controls to resume"])
	elif _fuel_ratio < 0.2:
		lines.assign(["Low fuel", "Depot / Torque: paliwo. HOLUJ / R w razie potrzeby." if taxi_mode else "Land on a pad to refuel"])
	if lines.is_empty():
		return
	var alert_y := _footer_y - 305 if taxi_mode else 211.0
	_box(Rect2(22, alert_y, size.x - 44, 76), Color(0.06, 0.075, 0.09, 0.94), Color(1.0, 0.65, 0.3, 0.55), 12)
	_text(Vector2(36, alert_y + 30), lines[0], 17, AMBER)
	_text(Vector2(36, alert_y + 57), lines[1], 14, WHITE)

func set_control_mode(mode: StringName) -> void:
	_control_mode = mode
	_repair_available = false
	clear_controls()

func update_vehicle_status(vehicle: FlightCab, credits: float, station: RepairStation, repairing: bool) -> void:
	_hull_ratio = vehicle.state.condition
	_credits = credits
	_passengers = vehicle.state.passenger_ids.size()
	_seats = vehicle.definition.max_passengers
	_damage_flash = vehicle.vitals.flash_remaining > 0.0
	_repair_available = is_instance_valid(station)
	_repairing = repairing
	if _repair_available:
		_repair_name = station.service_name
		_repair_price = station.price_per_hull
	queue_redraw()
