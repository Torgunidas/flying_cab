extends Node3D
## Separate authoring gallery. All geometry and actors are stored in the scene.
var ready_for_view := false
var _selected := -1
@onready var camera: Camera3D = $Camera3D
@onready var models: Node3D = $Models

func _ready() -> void:
	get_window().size = Vector2i(1400, 1100)
	get_viewport().content_scale_size = Vector2i(1400, 1100)
	var overlay := CanvasLayer.new()
	var label := Label.new()
	label.text = "FLYING CAB / VEHICLE LIBRARY     |     0: all    Left / Right: model"
	label.position = Vector2(28, 22)
	label.add_theme_font_size_override("font_size", 20)
	overlay.add_child(label)
	add_child(overlay)
	# Render every authored body and effect before exposing the gallery.
	var cover := ColorRect.new()
	cover.color = Color("101b2b")
	cover.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	overlay.add_child(cover)
	for cell in models.get_children():
		var car: FlightCab = cell.get_node("Vehicle")
		car.set_physics_process(false)
		car.flight_fx.show_warmup_effects()
		car.get_node("Headlights").update_lights(-30, 0, true)
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
	for cell in models.get_children():
		var car: FlightCab = cell.get_node("Vehicle")
		car.flight_fx.reset_visuals()
		car.get_node("Headlights").update_lights(54, 0, true)
	cover.queue_free()
	ready_for_view = true

func show_model(index: int) -> void:
	_selected = index
	for i in range(models.get_child_count()):
		models.get_child(i).visible = index < 0 or i == index
	camera.size = 17.5 if index < 0 else 7.0
	camera.position = Vector3(0, 0, 25) if index < 0 else models.get_child(index).position + Vector3(0, 0.4, 25)

func _unhandled_key_input(event: InputEvent) -> void:
	if not event is InputEventKey or not event.pressed or event.echo:
		return
	if event.keycode == KEY_0:
		show_model(-1)
	elif event.keycode == KEY_RIGHT:
		show_model((_selected + 1) % models.get_child_count())
	elif event.keycode == KEY_LEFT:
		show_model(posmod(_selected - 1, models.get_child_count()))
