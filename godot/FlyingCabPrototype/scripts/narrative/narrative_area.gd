@tool
class_name NarrativeArea
extends Area3D
## Geometry emits an event; quest requirements and completion belong to the catalog.
@export var area_id: StringName
@export var once := false

func _ready() -> void:
	if Engine.is_editor_hint(): return
	collision_mask = WorldLayers.VEHICLES | WorldLayers.PEOPLE
	body_entered.connect(_entered)

func _get_configuration_warnings() -> PackedStringArray:
	return PackedStringArray(["Assign a stable Area ID used by the objective's Target ID."]) if area_id == &"" else PackedStringArray()

func _entered(body: Node3D) -> void:
	var level := get_parent()
	while level and not level.get("context") is RuntimeContext:
		level = level.get_parent()
	if level == null or area_id == &"": return
	var context: RuntimeContext = level.context
	if body != context.player.focus: return
	var vehicle := String(body.entity_id) if body is FlightCab else ""
	var receipt := "area/%s/%s" % [context.current_map, area_id] if once else ""
	context.narrative.record_event("area_entered", {"target": String(area_id), "vehicle": vehicle}, receipt)
