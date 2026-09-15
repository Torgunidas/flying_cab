class_name TaxiNoticeBubble
extends Control
## Follow the rendered actor after the camera and guidance update, every frame.
## Text wrapping is cached; the rest of the HUD can keep its slower refresh.
var director: TaxiDirector
var guidance: TaxiGuidanceArrow
var anchor := Vector2.ZERO
var bubble_rect := Rect2()
var _lines: Array[String] = []
var _text := ""
var _width := -1.0
var _font: Font
var _panel: StyleBoxFlat

func _ready() -> void:
	mouse_filter = MOUSE_FILTER_IGNORE
	process_priority = 2
	_font = ThemeDB.fallback_font
	_panel = StyleBoxFlat.new()
	_panel.bg_color = Color("09151e")
	_panel.border_color = Color("345663")
	_panel.set_border_width_all(1)
	_panel.set_corner_radius_all(12)
	hide()

func _process(_dt: float) -> void:
	if director == null or not director.initialized or director.notice_remaining <= 0 or director.context.player.is_suspended():
		hide()
		return
	var actor := director.context.player.focus
	var camera := get_viewport().get_camera_3d()
	if not is_instance_valid(actor) or camera == null:
		hide()
		return
	var displayed := actor.get_global_transform_interpolated().origin
	if camera.is_position_behind(displayed):
		hide()
		return
	var width := minf(size.x - 48, 370)
	if _text != director.notice or not is_equal_approx(_width, width):
		_text = director.notice
		_width = width
		_lines = [""]
		for word: String in _text.split(" "):
			var next := (_lines[-1] + " " + word).strip_edges()
			if _font.get_string_size(next, HORIZONTAL_ALIGNMENT_LEFT, -1, 14).x > width - 28 and not _lines[-1].is_empty():
				_lines.append(word)
			else:
				_lines[-1] = next
	anchor = get_global_transform_with_canvas().affine_inverse() * camera.unproject_position(displayed) - Vector2(0, 55)
	if guidance and guidance.visible:
		anchor.y -= 26
	var height := 22 + _lines.size() * 20
	bubble_rect = Rect2(Vector2(clampf(anchor.x - width * 0.5, 24, size.x - width - 24), clampf(anchor.y - height, 90, size.y - height - 200)), Vector2(width, height))
	show()
	queue_redraw()

func _draw() -> void:
	draw_style_box(_panel, bubble_rect)
	var tip_x := clampf(anchor.x, bubble_rect.position.x + 16, bubble_rect.end.x - 16)
	draw_colored_polygon(PackedVector2Array([Vector2(tip_x - 7, bubble_rect.end.y - 1), Vector2(tip_x + 7, bubble_rect.end.y - 1), Vector2(tip_x, bubble_rect.end.y + 8)]), Color("09151e"))
	for i in range(_lines.size()):
		draw_string(_font, bubble_rect.position + Vector2(14, 25 + i * 20), _lines[i], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("e9f1ee"))
