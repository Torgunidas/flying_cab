class_name TaxiGuidanceArrow
extends Control
## Lightweight screen-space counterpart of Unreal's arrow above the cab.
## Tracks the rendered cab every frame; no pathfinding or physics queries.
var _shape := PackedVector2Array([Vector2(-14, -3), Vector2(3, -3), Vector2(3, -8), Vector2(18, 0), Vector2(3, 8), Vector2(3, 3), Vector2(-14, 3)])
var _outline := PackedVector2Array([Vector2(-14, -3), Vector2(3, -3), Vector2(3, -8), Vector2(18, 0), Vector2(3, 8), Vector2(3, 3), Vector2(-14, 3), Vector2(-14, -3)])
var director: TaxiDirector
var anchor := Vector2.ZERO
var direction := Vector2.ZERO

func _ready() -> void:
	mouse_filter = MOUSE_FILTER_IGNORE
	process_priority = 1 # Follow the camera after the level has updated it.
	hide()

func _process(_dt: float) -> void:
	var target := director.guidance_target() if director else null
	var camera := get_viewport().get_camera_3d()
	if target == null or camera == null:
		hide()
		return
	var cab := director.context.player.focus as FlightCab
	var cab_position := cab.get_global_transform_interpolated().origin
	var delta := target.landing_point() - cab_position
	delta.z = 0
	if delta.length_squared() < 0.0001 or camera.is_position_behind(cab_position):
		hide()
		return
	var cab_screen := camera.unproject_position(cab_position)
	# Project a nearby point along the bearing, so distant targets never jump
	# across the camera plane when the perspective view is tilted.
	direction = (camera.unproject_position(cab_position + delta.normalized()) - cab_screen).normalized()
	anchor = cab_screen - Vector2(0, 40)
	anchor.x = clampf(anchor.x, 24, size.x - 24)
	anchor.y = clampf(anchor.y, 100, size.y - 190)
	show()
	queue_redraw()

func _draw() -> void:
	draw_set_transform(anchor, direction.angle())
	draw_colored_polygon(_shape, Color("ff784a"))
	draw_polyline(_outline, Color("09151e"), 3, true)
