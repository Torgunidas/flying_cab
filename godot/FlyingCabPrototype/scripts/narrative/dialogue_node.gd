@tool
class_name DialogueNode
extends Resource
@export var id: StringName
## Empty uses the NPC's name. Text also supports {credits}, {medicine}, {seconds}.
@export var speaker := ""
## Write very concisely: one or two short sentences per beat. Scrolling is a
## fallback for exceptional long text, not the target reading experience.
@export_multiline var text := "Nowa kwestia"
@export var choices: Array[DialogueChoice] = []
