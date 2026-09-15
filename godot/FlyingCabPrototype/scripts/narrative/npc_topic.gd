@tool
class_name NpcTopic
extends Resource
@export var id: StringName
@export var title := "Porozmawiajmy"
@export var dialogue: DialogueDefinition
@export var conditions: Array[NarrativeCondition] = []
@export var hide_unavailable := false
