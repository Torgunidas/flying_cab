class_name TaxiGuidanceArrow
extends Control
## Lightweight screen-space counterpart of Unreal's arrow above the cab.
## Tracks the rendered cab every frame; no pathfinding or physics queries.
var _shape := PackedVector2Array([Vector2(-14, -3), Vector2(3, -3), Vector2(3, -8), Vector2(18, 0), Vector2(3, 8), Vector2(3, 3), Vector2(-14, 3)])
var _outline := PackedVector2Array([Vector2(-14, -3), Vector2(3, -3), Vector2(3, -8), Vector2(18, 0), Vector2(3, 8), Vector2(3, 3), Vector2(-14, 3), Vector2(-14, -3)])
var director: TaxiDirector
var tracked_map: CityReferenceMap
var color := Color("ff784a")
var anchor := Vector2.ZERO
var direction := Vector2.ZERO

func _ready() -> void:
	mouse_filter = MOUSE_FILTER_IGNORE
	process_priority = 1 # Follow the camera after the level has updated it.
	hide()

func _process(_dt: float) -> void:
	if director == null:
		hide()
		return
	var focus := director.context.player.focus
	var target_position := Vector3.ZERO
	if tracked_map:
		if tracked_map.tracked_point.is_empty() or director.context.player.is_suspended() or not is_instance_valid(focus):
			hide()
			return
		target_position = tracked_map.tracked_point.position
	else:
		var target := director.guidance_target()
		if target == null:
			hide()
			return
		target_position = target.landing_point()
	var camera := get_viewport().get_camera_3d()
	if camera == null:
		hide()
		return
	var cab_position := focus.get_global_transform_interpolated().origin
	var delta := target_position - cab_position
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
	draw_colored_polygon(_shape, color)
	draw_polyline(_outline, Color("09151e"), 3, true)
