class_name CityReferenceMap
extends Control
## On-demand city reference. No route, next-turn arrow or distance to follow.
var director: TaxiDirector
var selected_stop := ""
var _bounds := Rect2()
var _map := Rect2()
var _buildings: Array[Rect2] = []
var _font: Font

func _ready() -> void:
	mouse_filter = MOUSE_FILTER_IGNORE
	_font = ThemeDB.fallback_font
	hide()
	resized.connect(_layout)

func open() -> void:
	_bounds = director.context.world.definition.bounds()
	selected_stop = ""
	_buildings.clear()
	# Read authored collision silhouettes only when opening the reference.
	for collision: CollisionShape3D in director.context.world.map_root().find_children("*", "CollisionShape3D", true, false):
		if not collision.get_parent() is StaticBody3D or not collision.shape is BoxShape3D or collision.disabled:
			continue
		var p := collision.global_position
		var extent: Vector3 = collision.shape.size * collision.global_basis.get_scale().abs() * 0.5
		_buildings.append(Rect2(Vector2(p.x - extent.x, p.y - extent.y), Vector2(extent.x, extent.y) * 2).intersection(_bounds))
	_layout()
	show()
	queue_redraw()

func _layout() -> void:
	if _bounds.size.y <= 0:
		return
	var available := Vector2(size.x - 190, size.y - 245)
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
	var nearest := 28.0
	for stop: TaxiStop in director.network.stops.values():
		var distance := _point(stop.global_position).distance_to(point)
		if distance < nearest:
			nearest = distance
			selected_stop = stop.stop_id
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
	var selected: TaxiStop = director.network.stops.get(selected_stop)
	if selected:
		_text(Vector2(26, y + 32), selected.display_name, 15)
		if selected.fuel_service:
			_text(Vector2(26, y + 53), "Paliwo i naprawa", 12, Color("71e5e4"))
	else:
		_text(Vector2(26, y + 32), "Dotknij przystanku, aby sprawdzić miejsce.", 12, Color("8babae"))
