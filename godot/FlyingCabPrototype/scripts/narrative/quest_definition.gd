@tool
class_name QuestDefinition
extends Resource
@export var id: StringName
## Changing objective IDs or meaning requires a save migration, not just a title edit.
@export_range(1, 1000) var version := 1
@export var title := "Nowe zadanie"
@export_multiline var description := ""
@export_enum("main", "side") var category := "side"
@export var objectives: Array[QuestObjective] = []
@export var prerequisites: PackedStringArray = []
@export var conditions: Array[NarrativeCondition] = []
@export var auto_start := false
@export var requires_turn_in := false
@export var turn_in_npc: StringName
@export_range(0, 1000000, 0.1) var reward_credits := 0.0
## Rewards allow set_fact, grant_item, grant_access, revoke_access only.
@export var rewards: Array[NarrativeEffect] = []

func has_branches() -> bool:
	for objective in objectives:
		if objective and not objective.transitions.is_empty():
			return true
	return false

func objective_targets(objective: QuestObjective) -> PackedStringArray:
	var result := PackedStringArray()
	if not objective.transitions.is_empty():
		for transition in objective.transitions:
			if transition:
				result.append(String(transition.target))
	else:
		var index := objectives.find(objective) + 1
		result.append(String(objectives[index].id) if index < objectives.size() else "")
	return result
