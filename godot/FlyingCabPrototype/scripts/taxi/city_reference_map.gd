class_name CityReferenceMap
extends Control
## On-demand city reference with an optional direct bearing to a held marker.
var director: TaxiDirector
var selected_stop := ""
var selected_npc := ""
var quest_hubs: Array[Dictionary] = []
const HUB_RADIUS := 13.0
const HUB_COLOR := Color("f4deaa")
const TRACK_COLOR := Color("65ed8b")
const HOLD_SECONDS := 0.65
const HOLD_SLOP := 18.0
var tracked_point: Dictionary = {}
var _hold_target: Dictionary = {}
var _hold_pointer := -2
var _hold_origin := Vector2.ZERO
var _hold_elapsed := 0.0
var _hold_completed := false
var _hubs_dirty := false
var _bounds := Rect2()
var _map := Rect2()
var _buildings: Array[Rect2] = []
var _font: Font

func _ready() -> void:
	mouse_filter = MOUSE_FILTER_IGNORE
	_font = ThemeDB.fallback_font
	hide()
	resized.connect(_layout)
	visibility_changed.connect(cancel_hold)
	director.context.narrative.changed.connect(_invalidate_hubs)
	director.context.ledger.changed.connect(_invalidate_hubs)
	director.context.world.changed.connect(_invalidate_hubs)

func _invalidate_hubs() -> void:
	if visible and not _hubs_dirty:
		_hubs_dirty = true
		_refresh_hubs.call_deferred()

func _refresh_hubs() -> void:
	_hubs_dirty = false
	quest_hubs.clear()
	var world := director.context.world.map_root()
	if world:
		for node in get_tree().get_nodes_in_group("narrative_npc"):
			if not node is NarrativeNpc or not world.is_ancestor_of(node) or not node.is_visible_in_tree():
				continue
			var topics := director.context.narrative.available_quest_topics(node.profile)
			if topics.is_empty():
				continue
			var title: String = node.profile.display_name.strip_edges()
			quest_hubs.append({"id": String(node.profile.id), "name": title, "initial": title.left(1).to_upper(), "position": node.global_position, "topics": topics})
	if not selected_npc.is_empty() and _selected_hub().is_empty():
		selected_npc = ""
	queue_redraw()

func _selected_hub() -> Dictionary:
	for hub in _display_hubs():
		if hub.id == selected_npc:
			return hub
	return {}

func _display_hubs() -> Array[Dictionary]:
	var hubs := quest_hubs.duplicate()
	# A tracked location remains removable even after its quest becomes unavailable.
	if tracked_point.get("kind", "") == "npc" and not quest_hubs.any(func(hub): return hub.id == tracked_point.id):
		var marker := tracked_point.duplicate()
		marker.topics = PackedStringArray()
		hubs.append(marker)
	return hubs

func _target_at(point: Vector2) -> Dictionary:
	var result: Dictionary = {}
	var nearest := HUB_RADIUS + 5
	for hub in _display_hubs():
		var distance := hub_point(hub).distance_to(point)
		if distance < nearest:
			nearest = distance
			result = hub.duplicate()
			result.kind = "npc"
	if not result.is_empty():
		return result
	nearest = 28.0
	for stop: TaxiStop in director.network.stops.values():
		var distance := _point(stop.global_position).distance_to(point)
		if distance < nearest:
			nearest = distance
			result = {"kind": "stop", "id": stop.stop_id, "name": stop.display_name, "position": stop.global_position}
	return result

func _marker_point(target: Dictionary) -> Vector2:
	return hub_point(target) if target.kind == "npc" else _point(target.position)

func _same_target(a: Dictionary, b: Dictionary) -> bool:
	return not a.is_empty() and not b.is_empty() and a.kind == b.kind and a.id == b.id

func begin_hold(point: Vector2, pointer: int) -> void:
	if _hold_pointer != -2 or not visible:
		return
	inspect(point)
	_hold_target = _target_at(point)
	if _hold_target.is_empty():
		return
	_hold_pointer = pointer
	_hold_origin = point
	_hold_elapsed = 0.0
	_hold_completed = false
	queue_redraw()

func move_hold(point: Vector2, pointer: int) -> void:
	if pointer == _hold_pointer and (point.distance_to(_hold_origin) > HOLD_SLOP or not _same_target(_target_at(point), _hold_target)):
		cancel_hold()

func end_hold(pointer: int) -> void:
	if pointer == _hold_pointer:
		cancel_hold()

func cancel_hold() -> void:
	_hold_pointer = -2
	_hold_target = {}
	_hold_elapsed = 0.0
	_hold_completed = false
	queue_redraw()

func _notification(what: int) -> void:
	if what in [NOTIFICATION_APPLICATION_FOCUS_OUT, NOTIFICATION_WM_WINDOW_FOCUS_OUT]:
		cancel_hold()
	elif what == NOTIFICATION_WM_MOUSE_EXIT and _hold_pointer == -1:
		cancel_hold()

func _process(dt: float) -> void:
	if not visible or _hold_target.is_empty() or _hold_completed:
		return
	if not _same_target(_target_at(_hold_origin), _hold_target):
		cancel_hold()
		return
	_hold_elapsed = minf(_hold_elapsed + dt, HOLD_SECONDS)
	if _hold_elapsed >= HOLD_SECONDS:
		tracked_point = {} if _same_target(tracked_point, _hold_target) else _hold_target.duplicate()
		_hold_completed = true
	queue_redraw()

func hub_point(hub: Dictionary) -> Vector2:
	# Lift the badge off the berth so passenger/service symbols remain visible.
	var point := _point(hub.position) + Vector2(0, -24)
	var margin := Vector2.ONE * (HUB_RADIUS + 3)
	var city := director.context.world.definition as CityDefinition
	if city:
		var split := map_point(Vector2(0, city.upper_city_height)).y
		var names := ["EDEN", "AURELIA", "VELVET", "FOUNDRY"]
		for i in names.size():
			var origin := Vector2(_map.position.x + (i % 2) * _map.size.x * 0.5 + 8, (_map.position.y if i < 2 else split) + 8)
			var label := Rect2(origin, _font.get_string_size(names[i], HORIZONTAL_ALIGNMENT_LEFT, -1, 10))
			if label.grow(HUB_RADIUS + 3).has_point(point):
				point.x = label.end.x + HUB_RADIUS + 4
	return point.clamp(_map.position + margin, _map.end - margin)

func open() -> void:
	_bounds = director.context.world.definition.bounds()
	selected_stop = ""
	selected_npc = ""
	_buildings.clear()
	# Read authored collision silhouettes only when opening the reference.
	for collision: CollisionShape3D in director.context.world.map_root().find_children("*", "CollisionShape3D", true, false):
		if not collision.get_parent() is StaticBody3D or not collision.shape is BoxShape3D or collision.disabled:
			continue
		var p := collision.global_position
		var extent: Vector3 = collision.shape.size * collision.global_basis.get_scale().abs() * 0.5
		_buildings.append(Rect2(Vector2(p.x - extent.x, p.y - extent.y), Vector2(extent.x, extent.y) * 2).intersection(_bounds))
	_layout()
	_refresh_hubs()
	show()
	queue_redraw()

func _layout() -> void:
	cancel_hold()
	if _bounds.size.y <= 0:
		return
	var available := Vector2(size.x - 190, size.y - 270)
	var scale_factor := minf(available.x / _bounds.size.x, available.y / _bounds.size.y)
	var extent := _bounds.size * scale_factor
	_map = Rect2(Vector2((size.x - extent.x) * 0.5, 125), extent)
	queue_redraw()

func map_point(point: Vector2) -> Vector2:
	var uv := (point - _bounds.position) / _bounds.size
	return _map.position + Vector2(uv.x, 1.0 - uv.y) * _map.size

func _point(point: Vector3) -> Vector2:
	return map_point(Vector2(point.x, point.y))

func inspect(point: Vector2) -> void:
	var target := _target_at(point)
	selected_npc = ""
	if not target.is_empty():
		if target.kind == "npc":
			selected_npc = target.id
			selected_stop = ""
		else:
			selected_stop = target.id
	queue_redraw()

func _text(point: Vector2, value: String, height := 12, color := Color("e9f1ee")) -> void:
	draw_string(_font, point, value, HORIZONTAL_ALIGNMENT_LEFT, -1, height, color)

func _draw() -> void:
	if director == null or _bounds.size.y <= 0:
		return
	var extent := _map.size
	var scale_factor := extent.x / _bounds.size.x
	draw_rect(_map, Color("102630"))
	var city := director.context.world.definition as CityDefinition
	if city:
		var split := map_point(Vector2(0, city.upper_city_height)).y
		var low := map_point(Vector2(0, city.low_city_top)).y
		var colors := [Color("254340"), Color("3b3932"), Color("312739"), Color("21363d")]
		for i in range(4):
			var top := _map.position.y if i < 2 else split
			var bottom := split if i < 2 else low
			draw_rect(Rect2(_map.position.x + (i % 2) * extent.x * 0.5, top, extent.x * 0.5, bottom - top), colors[i])
		_text(_map.position + Vector2(8, 20), "EDEN", 10, Color("aeddd3"))
		_text(_map.position + Vector2(extent.x * 0.5 + 8, 20), "AURELIA", 10, Color("c6b88f"))
		_text(Vector2(_map.position.x + 8, split + 20), "VELVET", 10, Color("bc83ad"))
		_text(Vector2(_map.get_center().x + 8, split + 20), "FOUNDRY", 10, Color("71a1ab"))
		_text(Vector2(_map.position.x + 8, _map.end.y - 10), "LOWLIFE / SMOG", 10, Color("8babae"))
	for building in _buildings:
		if building.has_area():
			var top := map_point(Vector2(building.position.x, building.end.y))
			draw_rect(Rect2(top, building.size * scale_factor), Color(0.65, 0.75, 0.76, 0.16))
	for lane: Node3D in director.context.world.lanes:
		var dimensions: Vector2 = lane.size
		var direction := Vector3.UP if dimensions.y > dimensions.x else Vector3.RIGHT
		var half: float = maxf(dimensions.x, dimensions.y) * 0.5
		draw_line(_point(lane.to_global(-direction * half)), _point(lane.to_global(direction * half)), Color(0.6, 0.9, 0.8, 0.35), 2, true)
	for pad: Node3D in director.context.world.pads:
		draw_circle(_point(pad.global_position), 2, Color("627b81"))
	var trip := director.context.rides.active
	var destination: String = trip.get("destination", "")
	for stop: TaxiStop in director.network.stops.values():
		var point := _point(stop.global_position)
		var color := Color("e9f1ee")
		for offer: Dictionary in director.context.rides.offers:
			if offer.origin == stop.stop_id:
				color = Color("ffc176")
		if destination == stop.stop_id:
			color = Color("71e5e4")
			draw_circle(point, 10, color, false, 2, true)
		draw_circle(point, 4, color)
		if stop.stop_id == selected_stop:
			draw_circle(point, 14, Color("e9f1ee"), false, 1, true)
		if stop.fuel_service:
			draw_line(point + Vector2(7, -7), point + Vector2(15, -7), color, 2)
			draw_line(point + Vector2(11, -11), point + Vector2(11, -3), color, 2)
		# With 25 stops the selected place is named below the map. Only the
		# active destination is labelled on the chart, keeping close berths legible.
		if destination == stop.stop_id:
			var label := stop.display_name.split(" / ")[-1]
			var left := stop.global_position.x < 0
			var x := _map.position.x - _font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, 11).x - 10 if left else _map.end.x + 10
			_text(Vector2(x, point.y + 4), label, 11, color)
	var cab := director.context.player.focus as Node3D
	if cab:
		var point := _point(cab.global_position)
		draw_circle(point, 6, Color("ffc176"), false, 2, true)
		draw_circle(point, 2, Color("ffc176"))
	var y := _map.end.y + 28
	_text(Vector2(26, y), "Kropka: pasażer    Turkus: cel    Kółko: Ty    + serwis", 12, Color("8babae"))
	_text(Vector2(26, y + 22), "Litera w kółku: dostępne zadanie u NPC", 12, HUB_COLOR)
	for hub in _display_hubs():
		var point := hub_point(hub)
		var anchor := _point(hub.position).clamp(_map.position, _map.end)
		draw_line(anchor, point, HUB_COLOR.darkened(0.3), 1, true)
		draw_circle(point, HUB_RADIUS + 3, Color("102029"), true, -1, true)
		draw_circle(point, HUB_RADIUS, Color("101820"), true, -1, true)
		draw_circle(point, HUB_RADIUS, HUB_COLOR, false, 2, true)
		if hub.id == selected_npc:
			draw_circle(point, HUB_RADIUS + 4, HUB_COLOR, false, 1, true)
		var dimensions := _font.get_string_size(hub.initial, HORIZONTAL_ALIGNMENT_LEFT, -1, 16)
		var baseline := (_font.get_ascent(16) - _font.get_descent(16)) * 0.5
		_text(point + Vector2(-dimensions.x * 0.5, baseline), hub.initial, 16, HUB_COLOR)
	if not tracked_point.is_empty():
		draw_circle(_marker_point(tracked_point), 20, TRACK_COLOR, false, 2, true)
	if not _hold_target.is_empty():
		var center := _marker_point(_hold_target)
		draw_circle(center, 24, Color("102029"), false, 5, true)
		draw_circle(center, 24, TRACK_COLOR.darkened(0.65), false, 3, true)
		if _hold_elapsed > 0:
			draw_arc(center, 24, -PI * 0.5, -PI * 0.5 + TAU * _hold_elapsed / HOLD_SECONDS, 64, TRACK_COLOR, 3, true)
	var tracking_hint := "Przytrzymaj punkt: śledź / wyłącz"
	if not _hold_target.is_empty():
		tracking_hint = "Śledzenie włączone" if _same_target(tracked_point, _hold_target) else "Śledzenie wyłączone"
		if not _hold_completed:
			tracking_hint = "Trzymaj, aby wyłączyć śledzenie" if _same_target(tracked_point, _hold_target) else "Trzymaj, aby śledzić punkt"
	_text(Vector2(26, y + 94), tracking_hint, 12, TRACK_COLOR)
	var selected_hub := _selected_hub()
	var selected: TaxiStop = director.network.stops.get(selected_stop)
	if not selected_hub.is_empty():
		_text(Vector2(26, y + 50), selected_hub.name, 15, HUB_COLOR)
		var label := "Dostępne tematy: " + ", ".join(selected_hub.topics) if not selected_hub.topics.is_empty() else "Brak dostępnych tematów"
		while _font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, 12).x > size.x - 52 and label.length() > 1:
			label = label.trim_suffix("…").left(-1) + "…"
		_text(Vector2(26, y + 71), label, 12)
	elif selected:
		_text(Vector2(26, y + 50), selected.display_name, 15)
		if selected.fuel_service:
			_text(Vector2(26, y + 71), "Paliwo i naprawa", 12, Color("71e5e4"))
	else:
		_text(Vector2(26, y + 50), "Dotknij przystanku lub litery NPC.", 12, Color("8babae"))
