@tool
class_name NarrativeCatalog
extends Resource
## Single explicit content catalog. Validation never replaces author data with demo content.
@export var quests: Array[QuestDefinition] = []
@export var dialogues: Array[DialogueDefinition] = []
@export var npcs: Array[NpcDefinition] = []
@export var items: Array[NarrativeItem] = []
@export var fact_ids: PackedStringArray = []
@export var access_ids: PackedStringArray = []
@export var custom_events: PackedStringArray = []
@export_tool_button("Validate catalog", "Callable") var validate_button: Callable = validate

func quest(id: StringName) -> QuestDefinition:
	for entry in quests:
		if entry and entry.id == id:
			return entry
	return null

func npc(id: StringName) -> NpcDefinition:
	for entry in npcs:
		if entry and entry.id == id:
			return entry
	return null

func validate() -> void:
	var errors := validation_errors()
	if errors.is_empty():
		print("NARRATIVE: catalog valid — ", quests.size(), " quests, ", dialogues.size(), " dialogues, ", npcs.size(), " NPCs")
	for error in errors:
		push_error(error)

func validation_errors() -> PackedStringArray:
	var errors := PackedStringArray()
	for group in [quests, dialogues, npcs, items]:
		var ids := {}
		for entry in group:
			if entry == null or not _valid_id(entry.id) or ids.has(entry.id):
				errors.append("Catalog: missing resource, empty or duplicate ID")
				continue
			ids[entry.id] = true
	for list in [fact_ids, access_ids, custom_events]:
		var seen := {}
		for id in list:
			if not _valid_id(StringName(id)) or seen.has(id):
				errors.append("Catalog: empty or duplicate dictionary key " + id)
			seen[id] = true
	for q in quests:
		if q == null:
			continue
		var at := "Quest " + String(q.id)
		if q.title.is_empty() or q.objectives.is_empty() or q.version < 1 or not is_finite(q.reward_credits) or q.reward_credits < 0:
			errors.append(at + ": title, objectives, version or reward invalid")
		if q.requires_turn_in and q.turn_in_npc != &"" and npc(q.turn_in_npc) == null:
			errors.append(at + ": unknown turn-in NPC")
		_conditions(q.conditions, at, errors)
		for reward in q.rewards:
			_effect(reward, at + " reward", errors)
			if reward and reward.kind not in ["set_fact", "grant_item", "grant_access", "revoke_access"]:
				errors.append(at + ": unsupported reward effect " + reward.kind)
		var objective_ids := {}
		for objective in q.objectives:
			if objective == null or not _valid_id(objective.id) or objective_ids.has(objective.id):
				errors.append(at + ": missing/duplicate objective ID")
				continue
			objective_ids[objective.id] = true
			var path := at + "/" + String(objective.id)
			if objective.description.is_empty() or not is_finite(objective.required) or objective.required <= 0 or objective.mode not in ["state", "event"]:
				errors.append(path + ": invalid objective")
			if objective.mode == "state" and objective.conditions.is_empty():
				errors.append(path + ": state objective needs conditions")
			if objective.event == "custom" and not custom_events.has(String(objective.custom_event)):
				errors.append(path + ": unknown custom event")
			if objective.event != "custom" and objective.event not in QuestObjective.EVENTS:
				errors.append(path + ": unknown event")
			_conditions(objective.conditions, path, errors)
			for i in objective.transitions.size():
				var transition := objective.transitions[i]
				if transition == null:
					errors.append(path + ": null transition")
					continue
				_conditions(transition.conditions, path + " transition", errors)
				if transition.conditions.is_empty() and i != objective.transitions.size() - 1:
					errors.append(path + ": unconditional transition must be last")
			if not objective.transitions.is_empty() and objective.transitions.back() and not objective.transitions.back().conditions.is_empty():
				errors.append(path + ": transitions need an unconditional fallback")
		if q.has_branches():
			_validate_objective_graph(q, errors)
		_cycle(q, [], errors)
	for d in dialogues:
		if d == null:
			continue
		var at := "Dialogue " + String(d.id)
		if d.start_node == &"" or d.find_node(d.start_node) == null:
			errors.append(at + ": missing start node")
		var ids := {}
		for node in d.nodes:
			if node == null or not _valid_id(node.id) or ids.has(node.id):
				errors.append(at + ": empty or duplicate node")
				continue
			ids[node.id] = true
			var path := at + "/" + String(node.id)
			if node.text.is_empty():
				errors.append(path + ": empty text")
			var choices := {}
			for choice in node.choices:
				if choice == null or not _valid_id(choice.id) or choices.has(choice.id):
					errors.append(path + ": empty or duplicate choice ID")
					continue
				choices[choice.id] = true
				var option_path := path + "/" + String(choice.id)
				if choice.text.is_empty() or (choice.next_node != &"" and d.find_node(choice.next_node) == null):
					errors.append(option_path + ": empty text or missing destination")
				_conditions(choice.conditions, option_path, errors)
				for effect in choice.effects:
					_effect(effect, option_path, errors)
		for entry in d.entries:
			if entry == null or d.find_node(entry.node_id) == null:
				errors.append(at + ": invalid entry rule")
			else:
				_conditions(entry.conditions, at + " entry", errors)
	for profile in npcs:
		if profile == null:
			continue
		if profile.display_name.is_empty(): errors.append("NPC " + String(profile.id) + ": empty name")
		var topic_ids := {}
		for topic in profile.topics:
			if topic == null or not _valid_id(topic.id) or topic_ids.has(topic.id) or topic.title.is_empty() or not dialogues.has(topic.dialogue):
				errors.append("NPC " + String(profile.id) + ": invalid/duplicate topic or unregistered dialogue")
				continue
			topic_ids[topic.id] = true
			_conditions(topic.conditions, "NPC " + String(profile.id) + "/" + String(topic.id), errors)
	return errors

func _validate_objective_graph(q: QuestDefinition, errors: PackedStringArray) -> void:
	var graph := {}
	for objective in q.objectives:
		if objective:
			graph[String(objective.id)] = q.objective_targets(objective)
	var visiting := {}
	var visited := {}
	if not q.objectives.is_empty() and q.objectives[0]:
		_visit_objective(String(q.objectives[0].id), graph, visiting, visited, errors, String(q.id))
	for id in graph:
		if not visited.has(id):
			errors.append("Quest " + String(q.id) + ": unreachable objective " + id)

func _visit_objective(id: String, graph: Dictionary, visiting: Dictionary, visited: Dictionary, errors: PackedStringArray, quest_id: String) -> void:
	if id.is_empty(): return
	if visiting.has(id):
		errors.append("Quest " + quest_id + ": objective cycle at " + id)
		return
	if visited.has(id): return
	if not graph.has(id):
		errors.append("Quest " + quest_id + ": unknown objective " + id)
		return
	visiting[id] = true
	visited[id] = true
	for target in graph[id]:
		_visit_objective(target, graph, visiting, visited, errors, quest_id)
	visiting.erase(id)

func _valid_id(id: StringName) -> bool:
	var value := String(id)
	return not value.is_empty() and not value.begins_with("__") and not value.contains("/") and value == value.strip_edges()

func _cycle(q: QuestDefinition, path: Array, errors: PackedStringArray) -> void:
	if path.has(q.id):
		errors.append("Quest prerequisites form a cycle: " + String(q.id))
		return
	var next := path.duplicate()
	next.append(q.id)
	for id in q.prerequisites:
		var dependency := quest(StringName(id))
		if dependency == null:
			errors.append("Quest " + String(q.id) + ": unknown prerequisite " + id)
		else:
			_cycle(dependency, next, errors)

func _conditions(list: Array[NarrativeCondition], at: String, errors: PackedStringArray) -> void:
	for c in list:
		if c == null or not is_finite(c.amount) or c.kind not in ["quest_status", "fact", "credits", "item", "medicine", "campaign_active", "fuel_percent", "vehicle_id", "vehicle_model", "access"] or c.comparison not in [">=", "<=", "==", "!=", ">", "<"]:
			errors.append(at + ": invalid condition")
			continue
		if c.kind == "quest_status" and (quest(c.key) == null or c.status not in ["inactive", "active", "ready", "completed"]):
			errors.append(at + ": unknown quest/status " + String(c.key))
		_check_key(c.kind, c.key, at, errors)

func _check_key(kind: String, key: StringName, at: String, errors: PackedStringArray) -> void:
	var found := true
	if kind == "fact":
		found = fact_ids.has(String(key))
	elif kind == "access":
		found = access_ids.has(String(key))
	elif kind == "item":
		found = false
		for item in items:
			if item and item.id == key:
				found = true
	elif kind in ["vehicle_id", "vehicle_model"]:
		found = key != &""
	if not found:
		errors.append(at + ": unknown " + kind + " key " + String(key))

func _effect(e: NarrativeEffect, at: String, errors: PackedStringArray) -> void:
	if e == null or not is_finite(e.amount) or not is_finite(e.value) or e.amount < 0 or e.value < 0 or e.kind not in ["set_fact", "start_quest", "turn_in", "credit", "spend", "grant_item", "remove_item", "start_campaign", "buy_medicine", "deliver_medicine", "grant_access", "revoke_access"]:
		errors.append(at + ": invalid effect")
		return
	if e.kind in ["start_quest", "turn_in"] and quest(e.key) == null:
		errors.append(at + ": unknown quest " + String(e.key))
	if e.kind in ["grant_item", "remove_item", "buy_medicine", "deliver_medicine"] and (e.amount < 1 or e.amount != floorf(e.amount)):
		errors.append(at + ": item/dose amount must be a positive integer")
	if e.kind in ["grant_item", "remove_item"] and e.amount > 9007199254740991:
		errors.append(at + ": item amount exceeds save precision")
	if e.kind in ["buy_medicine", "deliver_medicine"] and e.amount > 1000:
		errors.append(at + ": at most 1000 doses per action")
	if (e.kind == "deliver_medicine" and e.value <= 0) or (e.kind == "start_campaign" and e.amount <= 0):
		errors.append(at + ": campaign time must be positive")
	var key_kind := {"set_fact": "fact", "grant_item": "item", "remove_item": "item", "grant_access": "access", "revoke_access": "access"}
	if key_kind.has(e.kind):
		_check_key(key_kind[e.kind], e.key, at, errors)
