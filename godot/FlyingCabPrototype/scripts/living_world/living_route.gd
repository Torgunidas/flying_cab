class_name LivingRoute
extends Resource
## Authored, directed loop in the flight plane. No player guidance is derived here.
@export var route_id := ""
@export var points := PackedVector3Array()
@export_range(0, 32, 1) var population := 4
@export_range(1, 16, 0.5) var speed := 10.0
@export var models := PackedStringArray(["normal_car_1", "normal_car_2", "normal_car_3"])

func sample(distance: float) -> Dictionary:
	var total := length()
	var remaining := fposmod(distance, maxf(total, 0.001))
	for i in range(points.size()):
		var next := (i + 1) % points.size()
		var segment := points[i].distance_to(points[next])
		if remaining <= segment:
			return {"position": points[i].lerp(points[next], remaining / maxf(segment, 0.001)), "next": next}
		remaining -= segment
	return {"position": points[0], "next": 1}

func length() -> float:
	var total := 0.0
	for i in range(points.size()):
		total += points[i].distance_to(points[(i + 1) % points.size()])
	return total
