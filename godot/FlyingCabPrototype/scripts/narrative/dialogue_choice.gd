@tool
class_name DialogueChoice
extends Resource
@export var id: StringName
## Keep the reply short enough for the existing button, ideally one line.
## Check the narrow phone preview; do not enlarge buttons to fit verbose prose.
@export_multiline var text := "Odpowiedź"
## Empty ends the conversation. A next node is reached only after successful effects.
@export var next_node: StringName
@export var conditions: Array[NarrativeCondition] = []
@export var effects: Array[NarrativeEffect] = []
@export var hide_unavailable := false
## For promises / gifts. Purchases and subsequent medicine deliveries should remain repeatable.
@export var once := false
