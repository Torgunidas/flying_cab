@tool
class_name FlyingCabStoryImporter
extends RefCounted
## Declarative import. Build and validate before writing; never execute author commands.
var errors := PackedStringArray()
var writes: Dictionary = {}
var project_id := ""
var generated_root := "res://resources/narrative/authored"

func _id(value: Variant) -> bool:
	if not value is String: return false
	var regex := RegEx.new()
	regex.compile("^[a-zA-Z0-9][a-zA-Z0-9_.-]*$")
	return regex.search(value) != null

func _array(data: Dictionary, key: String) -> Array:
	if not data.get(key) is Array:
		errors.append("Brak listy: " + key)
		return []
	return data[key]

func _fields(resource: Resource, data: Dictionary, fields: Dictionary) -> void:
	for key in fields:
		if not data.has(key): continue
		var value: Variant = data[key]
		var expected: int = fields[key]
		var compatible := typeof(value) == expected or (expected in [TYPE_FLOAT, TYPE_INT] and typeof(value) in [TYPE_FLOAT, TYPE_INT])
		if not compatible or (expected in [TYPE_FLOAT, TYPE_INT] and not is_finite(value)):
			errors.append("Nieprawidłowy typ pola " + key)
			continue
		if expected == TYPE_INT and value != floorf(value):
			errors.append("Pole " + key + " wymaga liczby całkowitej")
			continue
		resource.set(key, value)

func _conditions(list: Variant) -> Array[NarrativeCondition]:
	var result: Array[NarrativeCondition] = []
	if not list is Array:
		errors.append("Warunki muszą być listą")
		return result
	for data in list:
		if not data is Dictionary:
			errors.append("Nieprawidłowy warunek")
			continue
		var c := NarrativeCondition.new()
		_fields(c, data, {"kind": TYPE_STRING, "key": TYPE_STRING, "status": TYPE_STRING, "comparison": TYPE_STRING, "amount": TYPE_FLOAT, "invert": TYPE_BOOL, "reason": TYPE_STRING})
		result.append(c)
	return result

func _effects(list: Variant) -> Array[NarrativeEffect]:
	var result: Array[NarrativeEffect] = []
	if not list is Array:
		errors.append("Skutki muszą być listą")
		return result
	for data in list:
		if not data is Dictionary:
			errors.append("Nieprawidłowy skutek")
			continue
		var e := NarrativeEffect.new()
		_fields(e, data, {"kind": TYPE_STRING, "key": TYPE_STRING, "amount": TYPE_FLOAT, "value": TYPE_FLOAT})
		result.append(e)
	return result

func _strings(value: Variant) -> PackedStringArray:
	var result := PackedStringArray()
	if not value is Array:
		errors.append("Oczekiwano listy identyfikatorów")
		return result
	for id in value:
		if not _id(id): errors.append("Nieprawidłowy identyfikator")
		else: result.append(id)
	return result

func _owned(resource: Resource) -> bool:
	return resource != null and resource.get_meta("fc_story_project", "") == project_id

func _path(kind: String, id: String) -> String:
	return generated_root.path_join(project_id).path_join(kind + "_" + id + ".tres")

func _queue(resource: Resource, path: String) -> void:
	resource.set_meta("fc_story_project", project_id)
	writes[path] = resource

func _signature(q: QuestDefinition) -> String:
	var list: Array = []
	for o in q.objectives:
		var data := {"id": o.id, "mode": o.mode, "event": o.event, "required": o.required, "target": o.target_id, "origin": o.origin_id, "destination": o.destination_id, "vehicle": o.vehicle_id, "custom": o.custom_event, "conditions": [], "transitions": []}
		for c in o.conditions: data.conditions.append(_condition_signature(c))
		for t in o.transitions:
			var transition := {"target": t.target, "conditions": []}
			for c in t.conditions: transition.conditions.append(_condition_signature(c))
			data.transitions.append(transition)
		list.append(data)
	return JSON.stringify({"objectives": list, "requires_turn_in": q.requires_turn_in, "turn_in_npc": q.turn_in_npc})

func _condition_signature(c: NarrativeCondition) -> Array:
	return [c.kind, c.key, c.status, c.comparison, c.amount, c.invert]

func build(payload: Variant, base: NarrativeCatalog) -> NarrativeCatalog:
	errors.clear()
	writes.clear()
	if not payload is Dictionary or payload.get("format") != "flying-cab-narrative" or payload.get("version") != 1 or not _id(payload.get("project_id")) or base == null:
		errors.append("Nieprawidłowy eksport Flying Cab Narrative 1 lub katalog")
		return null
	project_id = payload.project_id
	var catalog := NarrativeCatalog.new()
	for q in base.quests:
		if not _owned(q): catalog.quests.append(q)
	for d in base.dialogues:
		if not _owned(d): catalog.dialogues.append(d)
	for item in base.items:
		if not _owned(item): catalog.items.append(item)
	# Dictionary keys remain registered even when retired, for existing saves.
	for key in ["fact_ids", "access_ids", "custom_events"]:
		var values: PackedStringArray = base.get(key).duplicate()
		for id in _strings(payload.get(key)):
			if not values.has(id): values.append(id)
		catalog.set(key, values)
	for data in _array(payload, "items"):
		if not data is Dictionary or not _id(data.get("id")):
			errors.append("Nieprawidłowy przedmiot")
			continue
		var item := NarrativeItem.new()
		_fields(item, data, {"id": TYPE_STRING, "display_name": TYPE_STRING})
		catalog.items.append(item)
		_queue(item, _path("item", data.id))
	for data in _array(payload, "quests"):
		if not data is Dictionary or not _id(data.get("id")):
			errors.append("Nieprawidłowy quest")
			continue
		var q := QuestDefinition.new()
		_fields(q, data, {"id": TYPE_STRING, "title": TYPE_STRING, "description": TYPE_STRING, "version": TYPE_INT, "category": TYPE_STRING, "auto_start": TYPE_BOOL, "requires_turn_in": TYPE_BOOL, "turn_in_npc": TYPE_STRING, "reward_credits": TYPE_FLOAT})
		q.prerequisites = _strings(data.get("prerequisites"))
		q.conditions = _conditions(data.get("conditions"))
		q.rewards = _effects(data.get("rewards"))
		for entry in _array(data, "objectives"):
			if not entry is Dictionary or not _id(entry.get("id")):
				errors.append("Nieprawidłowy cel")
				continue
			var o := QuestObjective.new()
			_fields(o, entry, {"id": TYPE_STRING, "description": TYPE_STRING, "mode": TYPE_STRING, "event": TYPE_STRING, "custom_event": TYPE_STRING, "required": TYPE_FLOAT, "target_id": TYPE_STRING, "origin_id": TYPE_STRING, "destination_id": TYPE_STRING, "vehicle_id": TYPE_STRING})
			o.conditions = _conditions(entry.get("conditions"))
			for route in _array(entry, "transitions"):
				if not route is Dictionary:
					errors.append("Nieprawidłowe przejście celu")
					continue
				var transition := QuestTransition.new()
				_fields(transition, route, {"target": TYPE_STRING})
				transition.conditions = _conditions(route.get("conditions"))
				o.transitions.append(transition)
			if o.mode == "event" and o.event not in ["credits_earned", "fuel_purchased", "vehicle_repaired"] and o.required != floorf(o.required):
				errors.append("Quest " + data.id + ": liczba zdarzeń musi być całkowita")
			q.objectives.append(o)
		var previous := base.quest(q.id)
		if previous:
			if not _owned(previous): errors.append("Quest " + data.id + " istnieje poza tym projektem; nadaj nowy identyfikator")
			elif q.version < previous.version or (_signature(q) != _signature(previous) and q.version <= previous.version):
				errors.append("Quest " + data.id + ": zmiana celów wymaga zwiększenia wersji i nowej sesji gry lub migracji zapisu")
		catalog.quests.append(q)
		_queue(q, _path("quest", data.id))
	var dialogues := {}
	for data in _array(payload, "dialogues"):
		if not data is Dictionary or not _id(data.get("id")):
			errors.append("Nieprawidłowy dialog")
			continue
		var d := DialogueDefinition.new()
		_fields(d, data, {"id": TYPE_STRING, "start_node": TYPE_STRING})
		for entry in _array(data, "nodes"):
			if not entry is Dictionary or not _id(entry.get("id")):
				errors.append("Nieprawidłowa wypowiedź")
				continue
			var node := DialogueNode.new()
			_fields(node, entry, {"id": TYPE_STRING, "speaker": TYPE_STRING, "text": TYPE_STRING})
			for option in _array(entry, "choices"):
				if not option is Dictionary or not _id(option.get("id")):
					errors.append("Nieprawidłowa odpowiedź")
					continue
				var choice := DialogueChoice.new()
				_fields(choice, option, {"id": TYPE_STRING, "text": TYPE_STRING, "next_node": TYPE_STRING, "once": TYPE_BOOL, "hide_unavailable": TYPE_BOOL})
				choice.conditions = _conditions(option.get("conditions"))
				choice.effects = _effects(option.get("effects"))
				node.choices.append(choice)
			d.nodes.append(node)
		for entry in _array(data, "entries"):
			if not entry is Dictionary:
				errors.append("Nieprawidłowy początek dialogu")
				continue
			var rule := DialogueEntry.new()
			_fields(rule, entry, {"node_id": TYPE_STRING})
			rule.conditions = _conditions(entry.get("conditions"))
			d.entries.append(rule)
		dialogues[data.id] = d
		catalog.dialogues.append(d)
		_queue(d, _path("dialogue", data.id))
	var incoming := {}
	for data in _array(payload, "npcs"):
		if not data is Dictionary or not _id(data.get("id")) or incoming.has(data.id):
			errors.append("Nieprawidłowy lub powtórzony NPC")
			continue
		incoming[data.id] = data
	var npc_ids: Array = incoming.keys()
	for npc in base.npcs:
		if not npc_ids.has(String(npc.id)): npc_ids.append(String(npc.id))
	for id in npc_ids:
		var original := base.npc(StringName(id))
		var data: Dictionary = incoming.get(id, {})
		if original == null and data.get("existing", false):
			errors.append("Nie znaleziono istniejącej postaci: " + id)
			continue
		if original != null and not data.is_empty() and not data.get("existing", false) and not _owned(original):
			errors.append("NPC " + id + " już istnieje; wybierz istniejący profil lub nadaj inne ID")
			continue
		var profile := NpcDefinition.new()
		profile.id = StringName(id)
		if original == null or _owned(original): profile.set_meta("fc_story_project", project_id)
		if original:
			profile.display_name = original.display_name
			profile.greeting = original.greeting
			profile.portrait = original.portrait
			for topic in original.topics:
				if not _owned(topic): profile.topics.append(topic)
		_fields(profile, data, {"display_name": TYPE_STRING, "greeting": TYPE_STRING})
		if data.get("portrait", null) == "": profile.portrait = null
		if data.has("portrait") and data.portrait != "":
			if not data.portrait is String or not data.portrait.begins_with("res://") or data.portrait.get_extension().to_lower() not in ["png", "jpg", "jpeg", "webp", "svg"]:
				errors.append("Portret musi wskazywać teksturę w projekcie Godota")
			else:
				profile.portrait = load(data.portrait) as Texture2D
				if profile.portrait == null: errors.append("Nie znaleziono portretu: " + data.portrait)
		if data.has("topics") and not data.topics is Array:
			errors.append("Tematy NPC muszą być listą")
			continue
		for entry in data.get("topics", []):
			if not entry is Dictionary or not _id(entry.get("id")) or not dialogues.has(entry.get("dialogue")):
				errors.append("Nieprawidłowy temat NPC " + id)
				continue
			var topic := NpcTopic.new()
			_fields(topic, entry, {"id": TYPE_STRING, "title": TYPE_STRING, "hide_unavailable": TYPE_BOOL})
			topic.dialogue = dialogues[entry.dialogue]
			topic.conditions = _conditions(entry.get("conditions"))
			topic.set_meta("fc_story_project", project_id)
			profile.topics.append(topic)
		var changed := original == null or profile.topics != original.topics or profile.display_name != original.display_name or profile.greeting != original.greeting or profile.portrait != original.portrait
		if changed:
			var path := original.resource_path if original else _path("npc", id)
			if original and path.is_empty(): path = original.get_meta("fc_story_path", "")
			if path.is_empty() or not path.begins_with("res://") or path.contains("::"):
				errors.append("NPC " + id + ": zapisz profil jako osobny zasób .tres przed importem")
			else:
				profile.set_meta("fc_story_path", path)
				writes[path] = profile
			catalog.npcs.append(profile)
		else:
			catalog.npcs.append(original)
	errors.append_array(catalog.validation_errors())
	if not errors.is_empty(): return null
	return catalog

func import_file(input_path: String, catalog_path := "res://resources/narrative/city_catalog.tres", dry_run := false) -> Dictionary:
	if not FileAccess.file_exists(input_path): return {"ok": false, "errors": ["Nie znaleziono pliku eksportu"]}
	var json := JSON.new()
	if json.parse(FileAccess.get_file_as_string(input_path)) != OK:
		return {"ok": false, "errors": ["JSON, linia %d: %s" % [json.get_error_line(), json.get_error_message()]]}
	var base := ResourceLoader.load(catalog_path, "", ResourceLoader.CACHE_MODE_IGNORE) as NarrativeCatalog
	var catalog := build(json.data, base)
	if catalog == null: return {"ok": false, "errors": Array(errors)}
	if dry_run: return {"ok": true, "files": writes.size() + 1, "dry_run": true}
	writes[catalog_path] = catalog
	var originals := {}
	var original_uids := {}
	var uid_pattern := RegEx.new()
	uid_pattern.compile('uid="(uid://[^\"]+)"')
	var backup := "res://build/story-import-backups/" + str(Time.get_unix_time_from_system()).replace(".", "_")
	for path in writes:
		originals[path] = FileAccess.get_file_as_bytes(path) if FileAccess.file_exists(path) else null
		# Read the header, not ResourceLoader: headless UID cache may be absent/stale.
		var uid_match := uid_pattern.search(originals[path].get_string_from_utf8().get_slice("\n", 0)) if originals[path] != null else null
		original_uids[path] = ResourceUID.text_to_id(uid_match.get_string(1)) if uid_match != null else -1
		if originals[path] != null:
			var backup_path: String = backup.path_join(path.trim_prefix("res://"))
			DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(backup_path.get_base_dir()))
			var file := FileAccess.open(backup_path, FileAccess.WRITE)
			if file == null: return {"ok": false, "errors": ["Nie można zapisać kopii bezpieczeństwa; import przerwany"]}
			file.store_buffer(originals[path])
	for path in writes:
		DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(path.get_base_dir()))
		(writes[path] as Resource).take_over_path(path)
	var written: Array[String] = []
	for path in writes:
		written.append(path)
		var save_error := ResourceSaver.save(writes[path], path)
		# Headless ResourceSaver can omit UIDs; existing scene references must survive.
		if save_error == OK and original_uids[path] != -1:
			save_error = ResourceSaver.set_uid(path, original_uids[path])
		if save_error != OK:
			for restore in written:
				if originals[restore] == null: DirAccess.remove_absolute(ProjectSettings.globalize_path(restore))
				else:
					var file := FileAccess.open(restore, FileAccess.WRITE)
					if file: file.store_buffer(originals[restore])
			return {"ok": false, "errors": ["Błąd zapisu; przywrócono pliki. Kopia: " + backup]}
	return {"ok": true, "files": writes.size(), "backup": backup, "catalog": catalog_path}
