extends SceneTree
var checks := 0
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok: print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count := 8) -> void:
	for i in count: await process_frame

func _run() -> void:
	var preview: Node = load("res://addons/flying_cab_story/preview.tscn").instantiate()
	preview.source_path = "res://../../tools/quest_editor/examples/depot_runs.narrative.json"
	root.add_child(preview)
	current_scene = preview
	await frames()
	check(preview.author_preview != null, "imported author preview starts without writing the game catalog")
	if preview.author_preview == null:
		quit(1)
		return
	var author: Node = preview.author_preview
	var context: RuntimeContext = author.context
	check(author.npc_picker.get_item_text(author.npc_picker.selected) == "FROGGY" and author.catalog.quests[author.quest_picker.selected].id == &"fc_depot_runs", "preview selects the imported speaker and quest")
	check(context.dialogue.choose(&"topic/fc_depot_runs", context.dialogue.revision), "new topic opens in the real dialogue panel")
	author.presentation.skip()
	await frames()
	var panel: NarrativePanel = author.presentation
	check(panel.options.size() == 3 and panel.options[0].text == "Zajmę się tym." and panel.options[1].text == "Nie teraz." and panel.line.text.contains("dwa kursy"), "real panel displays the authored line, both choices and topic navigation")
	check(Rect2(Vector2.ZERO, root.get_visible_rect().size).encloses(panel.panel.get_global_rect()), "imported dialogue fits the portrait viewport")
	check(absf(panel.line_scroll.size.y - 172.0) < 1.0, "imported dialogue uses the fixed 172 px NPC field")
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/story-editor-offer.png")
	check(context.dialogue.choose(&"accept", context.dialogue.revision), "accept button starts the quest in the preview session")
	preview._simulate_event()
	check(context.narrative.status(&"fc_depot_runs") == "active", "first simulated event does not finish a two-ride objective")
	preview._simulate_event()
	check(context.narrative.status(&"fc_depot_runs") == "ready", "second simulated event opens reward state")
	context.dialogue.end()
	context.dialogue.start(author.catalog.npc(&"froggy"), context.narrative, context.player)
	context.dialogue.choose(&"topic/fc_depot_runs", context.dialogue.revision)
	author.presentation.skip()
	await frames()
	check(context.dialogue._node.id == &"ready", "returning to the author topic shows the reward entry")
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/story-editor-reward.png")
	var before := context.campaign.credits
	check(context.dialogue.choose(&"claim", context.dialogue.revision) and context.campaign.credits == before + 75, "preview pays the configured reward through the real service")
	# The generic preview must also build valid completed paths for conditional objectives.
	var quest: QuestDefinition = author.catalog.quest(&"fc_depot_runs")
	var second: QuestObjective = quest.objectives[0].duplicate(true)
	second.id = &"alternative"
	var route := QuestTransition.new()
	route.target = second.id
	quest.objectives[0].transitions.append(route)
	quest.objectives.append(second)
	author.status_picker.select(2)
	author.restart_preview()
	check(author.context.narrative.status(quest.id) == "ready" and author.context.narrative.validate_snapshot(author.context.narrative.snapshot()), "ready-state author shortcut produces a valid branched save")
	preview.queue_free()
	await frames(2)
	print("STORY_EDITOR_RENDER: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
