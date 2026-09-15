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
func frames(count: int) -> void:
	for i in count: await process_frame
func capture(name: String) -> void:
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/" + name + ".png")
func fits(p: NarrativePanel) -> bool:
	print("LAYOUT viewport=", root.get_visible_rect(), " panel=",p.panel.get_global_rect()," minimum=",p.panel.get_combined_minimum_size()," scroll=",p.scroll.get_global_rect())
	return Rect2(Vector2.ZERO, root.get_visible_rect().size).encloses(p.panel.get_global_rect())
func text_height(p: NarrativePanel, expected := 172.0) -> bool:
	return absf(p.line_scroll.size.y - expected) < 1.0
func stays_at(p: NarrativePanel, panel_rect: Rect2, text_origin: Vector2) -> bool:
	# Check rendered frames, including typewriter animation and container sorting.
	var stable := true
	for i in 12:
		await RenderingServer.frame_post_draw
		var unchanged := p.panel.get_global_rect().is_equal_approx(panel_rect) and p.line.global_position.is_equal_approx(text_origin)
		if stable and not unchanged:
			print("DIALOGUE_SHIFT frame=", i, " panel=", p.panel.get_global_rect(), " expected=", panel_rect, " text=", p.line.global_position, " expected=", text_origin, " scroll=", p.line_scroll.scroll_vertical)
		stable = stable and unchanged
	return stable
func swipe(area: ScrollContainer) -> void:
	var start := area.get_global_rect().get_center()
	start.y = area.get_global_rect().end.y - 6
	var touch := InputEventScreenTouch.new()
	touch.index = 4
	touch.position = start
	touch.pressed = true
	root.push_input(touch, true)
	await frames(2)
	for step in 5:
		var drag := InputEventScreenDrag.new()
		drag.index = 4
		drag.position = start - Vector2(0, (step + 1) * 8)
		drag.relative = Vector2(0, -8)
		root.push_input(drag, true)
		await frames(2)
	touch = InputEventScreenTouch.new()
	touch.index = 4
	touch.position = start - Vector2(0, 40)
	touch.pressed = false
	root.push_input(touch, true)
	await frames(4)
func tap(at: Vector2) -> void:
	var event := InputEventScreenTouch.new()
	event.index = 7
	event.position = at
	event.pressed = true
	root.push_input(event, true)
	event = InputEventScreenTouch.new()
	event.index = 7
	event.position = at
	event.pressed = false
	root.push_input(event, true)
func _run() -> void:
	if DisplayServer.get_name() == "headless":
		quit(1)
		return
	var c := RuntimeContext.new()
	c.rides.enabled = true
	root.add_child(c)
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = c
	root.add_child(level)
	current_scene = level
	await level.prepare_gameplay()
	var exited := false
	for i in 180:
		await physics_frame
		exited = level.on_foot.try_exit(level.cab)
		if exited: break
	check(exited, "rendered world allows cab exit")
	if not exited:
		quit(1)
		return
	var maya: NarrativeNpc = level.get_node("Narrative/Maya")
	level.on_foot.actor.place(maya.global_position + Vector3(0.8, HumanRig.HEIGHT * 0.5 + 0.05, 0))
	var started := false
	for i in 180:
		await physics_frame
		started = maya.interact(c, level.on_foot.actor)
		if started: break
	check(started, "rendered NPC starts dialogue")
	if not started:
		quit(1)
		return
	var p: NarrativePanel = level.narrative_panel
	c.dialogue.choose(&"topic/help", c.dialogue.revision)
	p.skip()
	await frames(8)
	check(fits(p) and p.options.size() >= 3, "portrait dialogue fits 540 by 960 with vertical answers")
	check(text_height(p) and absf(p.line_scroll.size.y - p.answers.size.y) < 1.0 and p.options.all(func(button): return button.size.y <= 52.1), "NPC text area matches the three existing reply rows including gaps")
	check(p.options.all(func(button): return p.scroll.get_global_rect().encloses(button.get_global_rect())), "all three short replies remain visible beside independently scrollable NPC text")
	await capture("narrative-dialogue")
	level.dialogue_npc_text_height = 224
	await frames(8)
	check(paused and text_height(p, 224) and p.options.all(func(button): return button.size.y <= 52.1), "Inspector text-height change applies live during a paused dialogue without resizing reply buttons")
	level.dialogue_npc_text_height = 172
	await frames(8)
	check(text_height(p) and fits(p), "restoring the authored text height also refits the outer panel")
	tap(p.options[0].get_global_rect().get_center())
	check(c.narrative.status(&"first_dose") == "active", "native touch tap selects the intended visible reply")
	p.close()
	p.open_journal()
	await frames(8)
	check(fits(p) and p.answers.get_child_count() > 0, "portrait journal shows accepted quest within viewport")
	await capture("narrative-journal")
	p.close()
	c.set_process(false)
	level.queue_free()
	await process_frame
	c.queue_free()
	await process_frame
	# Exercise long author content using the same panel and an isolated catalog.
	c = RuntimeContext.new()
	var catalog := NarrativeCatalog.new()
	var d := DialogueDefinition.new()
	d.id = &"long_dialogue"
	d.start_node = &"short"
	var node := DialogueNode.new()
	node.id = &"long"
	node.text = "Dłuższa wypowiedź rozmówcy. Świat pozostaje widoczny, a tekst można spokojnie przeczytać. ".repeat(6)
	for i in 10:
		var choice := DialogueChoice.new()
		choice.id = StringName("answer_%d" % i)
		choice.text = "Odpowiedź %d: dłuższa myśl gracza, która powinna zawinąć się bez zmniejszania tekstu." % (i + 1)
		node.choices.append(choice)
	var short_line := DialogueNode.new()
	short_line.id = &"short"
	short_line.text = "Krótka wypowiedź."
	var long_line := DialogueNode.new()
	long_line.id = &"long_text"
	long_line.text = node.text
	var last_line := DialogueNode.new()
	last_line.id = &"last"
	last_line.text = "Do zobaczenia."
	for target in [short_line, long_line]:
		var next := DialogueChoice.new()
		next.id = &"continue"
		next.text = "Dalej."
		next.next_node = &"long_text" if target == short_line else &"last"
		var many := DialogueChoice.new()
		many.id = &"many"
		many.text = "Inna sprawa."
		many.next_node = &"long"
		target.choices.append(next)
		target.choices.append(many)
	d.nodes = [node, short_line, long_line, last_line]
	var profile := NpcDefinition.new()
	profile.id = &"long_npc"
	profile.display_name = "ROZMÓWCA Z DŁUGIM IMIENIEM"
	var topic := NpcTopic.new()
	topic.id = &"test"
	topic.dialogue = d
	profile.topics = [topic]
	catalog.npcs = [profile]
	catalog.dialogues = [d]
	c.narrative_catalog = catalog
	c.narrative.catalog = catalog
	root.add_child(c)
	p = NarrativePanel.new()
	p.text_size = 28
	p.context = c
	root.add_child(p)
	root.content_scale_size = Vector2i(360, 640)
	root.size = Vector2i(360, 640)
	c.dialogue.start(profile, c.narrative, c.player)
	p.skip()
	await frames(12)
	var fixed_panel := p.panel.get_global_rect()
	var fixed_text := p.line.global_position
	c.dialogue.choose(&"topic/test", c.dialogue.revision)
	check(await stays_at(p, fixed_panel, fixed_text), "two-to-three replies keep the panel and NPC text origin fixed through rendered frames")
	c.dialogue.choose(&"continue", c.dialogue.revision)
	check(await stays_at(p, fixed_panel, fixed_text) and p.scroll_hint.visible, "long NPC text reveals the scroll hint without moving the panel or first line")
	p.skip()
	p.line_scroll.scroll_vertical = int(p.line_scroll.get_v_scroll_bar().max_value)
	await frames(6)
	c.dialogue.choose(&"continue", c.dialogue.revision)
	var short_stable := await stays_at(p, fixed_panel, fixed_text)
	check(short_stable and p.options.size() == 1 and p.line_scroll.scroll_vertical == 0 and not p.scroll_hint.visible and not p.line_scroll.get_v_scroll_bar().visible, "short next line resets scrolling and keeps its origin fixed with only one reply")
	c.dialogue.choose(&"__topics", c.dialogue.revision)
	check(await stays_at(p, fixed_panel, fixed_text), "returning to the topic list keeps the same reading position")
	c.dialogue.choose(&"topic/test", c.dialogue.revision)
	c.dialogue.choose(&"many", c.dialogue.revision)
	check(await stays_at(p, fixed_panel, fixed_text), "a long list of wrapped replies preserves the same panel bounds and NPC text origin")
	p.skip()
	await frames(12)
	check(fits(p) and p.options.size() == 11, "narrow viewport and larger font preserve every answer")
	check(text_height(p), "narrow phone preserves the configured NPC text height")
	check(p.line_scroll.get_v_scroll_bar().visible and p.scroll_hint.visible and p.scroll_hint.text.begins_with("↓"), "overflowing NPC text exposes both a scrollbar and a touch hint")
	var answer_position := p.answers.global_position
	await swipe(p.line_scroll)
	check(p.line_scroll.scroll_vertical > 0 and p.answers.global_position.is_equal_approx(answer_position), "native touch swipe scrolls NPC text without displacing reply buttons")
	p.line_scroll.scroll_vertical = int(p.line_scroll.get_v_scroll_bar().max_value)
	await frames(6)
	check(p.scroll_hint.text.begins_with("↑"), "end of NPC text changes the hint to scroll back up")
	var revision := c.dialogue.revision
	await swipe(p.scroll)
	check(p.scroll.scroll_vertical > 0 and c.dialogue.revision == revision, "swiping replies scrolls the list without choosing a dialogue action")
	p.scroll.ensure_control_visible(p.options[9])
	await frames(6)
	check(p.scroll.scroll_vertical > 0 and p.options[9].get_global_rect().intersects(p.scroll.get_global_rect()), "tenth answer can be reached by scrolling")
	await capture("narrative-long-360")
	root.content_scale_size = Vector2i(960, 540)
	root.size = Vector2i(960, 540)
	await frames(12)
	check(fits(p) and p.panel.size.x <= 620.1, "wide window retains a bounded dialogue width")
	check(text_height(p), "landscape preserves the same NPC text height while keeping the panel on screen")
	await capture("narrative-wide")
	p.close()
	p.queue_free()
	c.queue_free()
	await process_frame
	root.content_scale_size = Vector2i(540, 960)
	root.size = Vector2i(540, 960)
	var preview: Node = load("res://scenes/narrative/preview.tscn").instantiate()
	root.add_child(preview)
	current_scene = preview
	await frames(8)
	preview.presentation.skip()
	check(preview.context.dialogue.is_active(), "F6 author preview uses real dialogue in an isolated session")
	await capture("narrative-author-preview")
	preview.npc_picker.select(1)
	preview.quest_picker.select(1)
	preview.status_picker.select(2)
	preview.restart_preview()
	check(preview.context.narrative.status(&"froggy_favor") == "ready" and preview.context.narrative.status(&"first_dose") == "completed", "author preview can simulate turn-in with prerequisite completion")
	preview.queue_free()
	await process_frame
	print("NARRATIVE_RENDER_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
