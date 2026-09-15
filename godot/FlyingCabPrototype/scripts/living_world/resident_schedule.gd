class_name ResidentSchedule
extends Resource
## A parked opportunity or a resident alternating between two terraces.
@export var resident_id := ""
@export var model_id := "normal_car_1"
@export var stops := PackedStringArray()
@export var parked_only := false
@export var berth_x := 2.8
@export var corridor_x := 13.7
@export_range(3, 120, 1) var dwell_seconds := 18.0
@export_range(0, 3, 1) var look := 0
