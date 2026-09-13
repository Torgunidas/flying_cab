class_name MapDefinition
extends Resource
## Common identity/entry contract for cities, interiors and future maps.
@export var map_id: StringName = &"map"
@export var display_name := ""
@export var map_bounds := Rect2(-10, 0, 20, 20)
@export var entry_points: Dictionary = {"default": Vector3.ZERO}
@export var warmup_points: PackedVector3Array = []

func bounds() -> Rect2:
	return map_bounds

func district_at(_position: Vector3, _smog_top: float) -> String:
	return display_name
