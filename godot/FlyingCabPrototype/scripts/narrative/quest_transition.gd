@tool
class_name QuestTransition
extends Resource
## First matching transition wins. Empty target finishes the objectives.
@export var target: StringName
@export var conditions: Array[NarrativeCondition] = []
