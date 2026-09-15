@tool
class_name DialogueDefinition
extends Resource
@export var id: StringName
@export var start_node: StringName
## First matching rule wins; no match uses Start Node.
@export var entries: Array[DialogueEntry] = []
@export var nodes: Array[DialogueNode] = []

func find_node(key: StringName) -> DialogueNode:
	for node in nodes:
		if node and node.id == key:
			return node
	return null
