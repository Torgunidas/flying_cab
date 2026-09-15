extends Node
@export var source_path := "res://build/story-preview.json"
var author_preview: Node
var debug_status: Label

func _ready() -> void:
	var importer := FlyingCabStoryImporter.new()
	var payload: Variant = JSON.parse_string(FileAccess.get_file_as_string(source_path))
	var catalog := importer.build(payload, load("res://resources/narrative/city_catalog.tres"))
	if catalog == null:
		var label := Label.new()
		label.text = "Błąd podglądu:\n" + "\n".join(importer.errors)
		add_child(label)
		return
	var preview: Node = load("res://scenes/narrative/preview.tscn").instantiate()
	preview.catalog = catalog
	add_child(preview)
	author_preview = preview
	var target_npc := ""
	for npc in payload.npcs:
		if not npc.topics.is_empty():
			target_npc = npc.id
			break
	for i in catalog.npcs.size():
		if String(catalog.npcs[i].id) == target_npc: preview.npc_picker.select(i)
	if not payload.quests.is_empty():
		for i in catalog.quests.size():
			if String(catalog.quests[i].id) == payload.quests[0].id: preview.quest_picker.select(i)
	preview.restart_preview()
	var column: VBoxContainer = preview.npc_picker.get_parent()
	var event_button := Button.new()
	event_button.text = "Symuluj zdarzenie bieżącego celu (+1)"
	event_button.pressed.connect(_simulate_event)
	column.add_child(event_button)
	debug_status = Label.new()
	debug_status.text = "Osobna sesja autora — zapis gracza jest bezpieczny."
	debug_status.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	column.add_child(debug_status)

func _simulate_event() -> void:
	var context: RuntimeContext = author_preview.context
	if author_preview.catalog.quests.is_empty():
		debug_status.text = "Ta opowieść zawiera wyłącznie rozmowy."
		return
	var q: QuestDefinition = author_preview.catalog.quests[author_preview.quest_picker.selected]
	var state: Dictionary = context.narrative.data.quests.get(String(q.id), {})
	if state.get("status") != "active":
		debug_status.text = "Najpierw przyjmij wybrane zadanie w rozmowie."
		return
	for objective in q.objectives:
		if String(objective.id) != state.step: continue
		if objective.mode != "event":
			debug_status.text = "Ten cel sprawdza stan. Ustaw zasoby w Inspectorze podglądu i uruchom go ponownie."
			return
		context.narrative.record_event(objective.event_id(), {"amount": 1.0, "target": String(objective.target_id), "origin": String(objective.origin_id), "destination": String(objective.destination_id), "vehicle": String(objective.vehicle_id)})
		debug_status.text = "%s: %s. Wróć do tematów rozmowy, aby zobaczyć zmianę." % [q.title, context.narrative.status(q.id)]
		return
