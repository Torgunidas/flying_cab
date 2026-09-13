class_name DialogueSession
extends RefCounted
## Data-driven conversation controller. UI and domain effects subscribe explicitly.
signal line_changed(speaker: String, text: String, choices: Array)
signal choice_selected(conversation_id: StringName, choice_id: StringName)
signal finished
var _player: PlayerSession
var _lines: Dictionary = {}
var _conversation: StringName
var _current: StringName

func begin(id: StringName, lines: Dictionary, start: StringName, player: PlayerSession) -> bool:
	if id == &"" or not lines.has(String(start)) or player == null or is_active():
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
	_show(start)
	return true

func choose(id: StringName) -> bool:
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
	finished.emit()

func is_active() -> bool:
	return _player != null

func _show(id: StringName) -> void:
	_current = id
	var line: Dictionary = _lines[String(id)]
	line_changed.emit(line.speaker, line.text, line.choices.duplicate(true))
