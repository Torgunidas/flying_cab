@tool
class_name NpcDefinition
extends Resource
@export var id: StringName
@export var display_name := "Rozmówca"
@export var portrait: Texture2D
@export_multiline var greeting := "O czym chcesz porozmawiać?"
@export var topics: Array[NpcTopic] = []
