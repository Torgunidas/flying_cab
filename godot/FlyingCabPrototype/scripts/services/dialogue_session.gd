class_name DialogueSession
extends RefCounted
## Data-driven conversation controller. UI and domain effects subscribe explicitly.
signal line_changed(speaker: String, text: String, choices: Array)
signal choice_selected(conversation_id: StringName, choice_id: StringName)
signal finished
signal started
var revision := 0
var speaker := ""
var text := ""
var choices: Array = []
var feedback := ""
var profile: NpcDefinition
var service: NarrativeService
var definition: DialogueDefinition
var _node: DialogueNode
var _choosing := false
var _player: PlayerSession
var _lines: Dictionary = {}
var _conversation: StringName
var _current: StringName

func begin(id: StringName, lines: Dictionary, start_id: StringName, player: PlayerSession) -> bool:
	if id == &"" or not lines.has(String(start_id)) or player == null or is_active():
		return false
	# Validate graph edges before locking control. No executable method names or
	# file paths are accepted as dialogue data.
	for key in lines:
		var line = lines[key]
		if not line is Dictionary or not line.get("speaker") is String or not line.get("text") is String or not line.get("choices") is Array:
			return false
		var ids := {}
		for choice in line.choices:
			if not choice is Dictionary or not choice.get("id") is String or choice.id.is_empty() or ids.has(choice.id) or not choice.get("text") is String or not choice.get("next") is String:
				return false
			ids[choice.id] = true
			if not choice.next.is_empty() and not lines.has(choice.next):
				return false
	_player = player
	_conversation = id
	_lines = lines.duplicate(true)
	_player.suspend(&"dialogue")
	_show(start_id)
	return true

func choose(id: StringName, expected_revision := -1) -> bool:
	if profile:
		return _choose_authored(id, expected_revision)
	if not is_active():
		return false
	for choice in _lines[String(_current)].choices:
		if choice.id == String(id):
			var conversation := _conversation
			if choice.next.is_empty():
				end()
			else:
				_show(StringName(choice.next))
			choice_selected.emit(conversation, id)
			return true
	return false

func end() -> void:
	if not is_active():
		return
	_lines.clear()
	_current = &""
	_conversation = &""
	_player.resume(&"dialogue")
	_player = null
	profile = null
	definition = null
	_node = null
	service = null
	revision += 1
	finished.emit()

func is_active() -> bool:
	return _player != null

func _show(id: StringName) -> void:
	_current = id
	var line: Dictionary = _lines[String(id)]
	line_changed.emit(line.speaker, line.text, line.choices.duplicate(true))

func start(npc: NpcDefinition, narrative: NarrativeService, player: PlayerSession) -> bool:
	if is_active() or npc == null or narrative == null or narrative.catalog == null or narrative.catalog.npc(npc.id) != npc or player == null or player.is_suspended() or narrative.context.campaign.flags.get("__campaign_expired", false):
		return false
	profile = npc
	service = narrative
	_player = player
	feedback = ""
	_player.suspend(&"dialogue")
	started.emit()
	_topics()
	return true

func _topics() -> void:
	feedback = ""
	definition = null
	_node = null
	speaker = profile.display_name
	text = profile.greeting
	choices = []
	for topic in profile.topics:
		var unavailable := service.reason(topic.conditions)
		if not unavailable.is_empty() and topic.hide_unavailable:
			continue
		choices.append({"id": "topic/" + String(topic.id), "text": topic.title, "enabled": unavailable.is_empty(), "reason": unavailable})
	choices.append({"id": "__close", "text": "Do zobaczenia.", "enabled": true, "reason": ""})
	_publish()

func _show_authored(node: DialogueNode) -> void:
	_node = node
	speaker = profile.display_name if node.speaker.is_empty() else node.speaker
	text = _format(node.text)
	choices = []
	for choice in node.choices:
		var unavailable := service.reason(choice.conditions)
		if unavailable.is_empty():
			unavailable = service.preview_effects(choice.effects, _once_key(choice), String(profile.id))
		if not unavailable.is_empty() and choice.hide_unavailable:
			continue
		choices.append({"id": String(choice.id), "text": _format(choice.text), "enabled": unavailable.is_empty(), "reason": unavailable})
	# Always offer an escape, including a node whose authored options are all unavailable.
	choices.append({"id": "__topics", "text": "Inny temat.", "enabled": true, "reason": ""})
	_publish()

func _publish() -> void:
	revision += 1
	line_changed.emit(speaker, text, choices.duplicate(true))

func _once_key(choice: DialogueChoice) -> String:
	return "%s/%s/%s" % [definition.id, _node.id, choice.id] if choice.once else ""

func _format(value: String) -> String:
	return value.format({"credits": "%.0f" % service.context.campaign.credits, "medicine": service.context.campaign.medicine_doses, "seconds": ceili(service.context.campaign.remaining_seconds)})

func _choose_authored(id: StringName, expected: int) -> bool:
	if _choosing or not is_active() or expected != revision:
		return false
	if id == &"__close":
		end()
		return true
	if id == &"__topics":
		_topics()
		return true
	if definition == null:
		for topic in profile.topics:
			if "topic/" + String(topic.id) != String(id): continue
			if not service.reason(topic.conditions).is_empty():
				_topics()
				return false
			definition = topic.dialogue
			var entry := definition.start_node
			for rule in definition.entries:
				if service.reason(rule.conditions).is_empty():
					entry = rule.node_id
					break
			_show_authored(definition.find_node(entry))
			return true
		return false
	for choice in _node.choices:
		if choice.id != id: continue
		feedback = service.reason(choice.conditions)
		if feedback.is_empty():
			_choosing = true
			feedback = service.execute(choice.effects, _once_key(choice), String(profile.id))
			_choosing = false
			if not is_active(): return feedback.is_empty()
		if not feedback.is_empty():
			_show_authored(_node)
			return false
		if choice.next_node == &"":
			end()
		else:
			_show_authored(definition.find_node(choice.next_node))
		return true
	return false
