extends Node3D
## The visible air lane and its assist share the same local rectangle.
@export var size := Vector2(12, 288)
@export var speed_multiplier := 1.5
@export var fuel_multiplier := 0.5
@export var enabled := true

func contains_point(point: Vector3) -> bool:
	var local := to_local(point)
	return enabled and absf(local.x) <= size.x * 0.5 and absf(local.y) <= size.y * 0.5 and absf(local.z) <= 2.0
