extends Node
## F6 author sandbox. Never reads or writes player saves; uses the same runtime and UI.
@export var catalog: NarrativeCatalog = preload("res://resources/narrative/city_catalog.tres")
@export var initial_credits := 250.0
@export var initial_medicine := 1
@export var campaign_seconds := 600.0
@export var initial_facts: Dictionary[String, float] = {}
var context: RuntimeContext
var presentation: NarrativePanel
var npc_picker: OptionButton
var quest_picker: OptionButton
var status_picker: OptionButton

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	var overlay := CanvasLayer.new()
	overlay.layer = 80
	add_child(overlay)
	var column := VBoxContainer.new()
	column.position = Vector2(16, 16)
	column.custom_minimum_size.x = 490
	column.add_theme_constant_override("separation", 10)
	overlay.add_child(column)
	var label := Label.new()
	label.text = "PODGLĄD AUTORA — osobna sesja bez zapisu"
	column.add_child(label)
	var errors := catalog.validation_errors()
	if not errors.is_empty():
		label.text = "Błędy katalogu — zobacz Output."
		catalog.validate()
		return
	npc_picker = OptionButton.new()
	for npc in catalog.npcs: npc_picker.add_item(npc.display_name)
	column.add_child(npc_picker)
	quest_picker = OptionButton.new()
	for quest in catalog.quests: quest_picker.add_item(quest.title)
	column.add_child(quest_picker)
	status_picker = OptionButton.new()
	for status in ["inactive", "active", "ready", "completed"]: status_picker.add_item(status)
	column.add_child(status_picker)
	var restart := Button.new()
	restart.text = "Zastosuj stan i rozpocznij rozmowę"
	restart.pressed.connect(restart_preview)
	column.add_child(restart)
	var validate := Button.new()
	validate.text = "Sprawdź katalog"
	validate.pressed.connect(catalog.validate)
	column.add_child(validate)
	restart_preview()

func restart_preview() -> void:
	if presentation:
		presentation.free()
	if context:
		context.free()
	context = RuntimeContext.new()
	context.narrative_catalog = catalog
	context.narrative.catalog = catalog
	context.campaign.credits = initial_credits
	context.campaign.medicine_doses = initial_medicine
	context.campaign.active = campaign_seconds > 0
	context.campaign.remaining_seconds = maxf(0, campaign_seconds)
	context.campaign.flags.merge(initial_facts, true)
	if not catalog.quests.is_empty():
		var q := catalog.quests[quest_picker.selected]
		var status := status_picker.get_item_text(status_picker.selected)
		if status != "inactive":
			_set_quest(q, status)
			for id in q.prerequisites:
				_set_quest(catalog.quest(StringName(id)), "completed")
	add_child(context)
	presentation = NarrativePanel.new()
	presentation.context = context
	add_child(presentation)
	if not catalog.npcs.is_empty():
		context.dialogue.start(catalog.npcs[npc_picker.selected], context.narrative, context.player)

func _set_quest(q: QuestDefinition, status: String) -> void:
	if status == "ready" and not q.requires_turn_in:
		status = "completed"
	var progress := {}
	for objective in q.objectives:
		progress[String(objective.id)] = 0.0 if status == "active" else objective.required
	context.narrative.data.quests[String(q.id)] = {"version": q.version, "status": status, "step": String(q.objectives[0].id) if status == "active" else "", "progress": progress}
	if q.has_branches():
		var path: Array[String] = [String(q.objectives[0].id)]
		for objective in q.objectives: progress[String(objective.id)] = 0.0
		if status != "active":
			for iteration in q.objectives.size():
				var current: QuestObjective
				for objective in q.objectives:
					if String(objective.id) == path.back(): current = objective
				if current == null: break
				progress[String(current.id)] = current.required
				var next_id := q.objective_targets(current)[0]
				for transition in current.transitions:
					if context.narrative.reason(transition.conditions).is_empty():
						next_id = String(transition.target)
						break
				if next_id.is_empty(): break
				path.append(next_id)
		context.narrative.data.quests[String(q.id)].path = path
