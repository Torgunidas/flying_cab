class_name NarrativePanel
extends CanvasLayer
## Portrait conversation and journal. Gameplay state belongs to the session.
const ANSWER_HEIGHT := 52.0
const ANSWER_GAP := 8
const THREE_ANSWER_ROWS := ANSWER_HEIGHT * 3 + ANSWER_GAP * 2
@export_range(0, 120, 1) var characters_per_second := 55.0
@export_range(0, 0.5, 0.01) var answer_delay := 0.085
@export_range(16, 32, 1) var text_size := 20
## NPC text viewport in UI pixels. 172 equals three standard reply rows with gaps.
@export_range(52, 360, 1) var npc_text_height := THREE_ANSWER_ROWS
var context: RuntimeContext
var controls: FlightControls
var session: DialogueSession
var screen: Control
var panel: PanelContainer
var heading: Label
var line: RichTextLabel
var answers: VBoxContainer
var scroll: ScrollContainer
var line_scroll: ScrollContainer
var scroll_hint: Label
var _column: VBoxContainer
var _header: HBoxContainer
var journal: Button
var clock_label: Label
var close_button: Button
var portrait: TextureRect
var _typing_hold := 0.0
var options: Array[Button] = []
var _view: Array = []
var _revision := 0
var _elapsed := 0.0
var _visible_chars := 0.0
var _selected := -1
var _mode := ""
var _owns_pause := false
var _controls_visible := true
var _camera: Camera3D
var _camera_transform := Transform3D.IDENTITY
var _touch_index := -1
var _touch_origin := Vector2.ZERO
var _touch_last := Vector2.ZERO
var _touch_dragged := false
var _touch_scroll: ScrollContainer
var _touch_button: Button
var _touch_text := false
var _touch_revision := 0
var _line_minimum := Vector2(-1, -1)

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	layer = 70
	session = context.dialogue
	_build()
	session.started.connect(_dialogue_started)
	session.line_changed.connect(_line_changed)
	session.finished.connect(_dialogue_finished)
	context.narrative.changed.connect(_state_changed)
	get_viewport().size_changed.connect(_layout)
	_layout()
	_state_changed()

func _style(color: Color) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = color
	style.corner_radius_top_left = 12
	style.corner_radius_top_right = 12
	style.corner_radius_bottom_left = 12
	style.corner_radius_bottom_right = 12
	style.content_margin_left = 16
	style.content_margin_right = 16
	style.content_margin_top = 12
	style.content_margin_bottom = 12
	return style

func _build() -> void:
	journal = Button.new()
	journal.text = "≡"
	journal.tooltip_text = "Zadania i przedmioty / J"
	journal.pressed.connect(open_journal)
	add_child(journal)
	clock_label = Label.new()
	clock_label.add_theme_font_size_override("font_size", 16)
	clock_label.add_theme_color_override("font_color", Color("ffca76"))
	add_child(clock_label)
	screen = Control.new()
	add_child(screen)
	screen.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var dim := ColorRect.new()
	dim.color = Color(0.01, 0.02, 0.04, 0.3)
	screen.add_child(dim)
	dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel = PanelContainer.new()
	panel.add_theme_stylebox_override("panel", _style(Color("091727f7")))
	screen.add_child(panel)
	_column = VBoxContainer.new()
	_column.add_theme_constant_override("separation", 8)
	panel.add_child(_column)
	_header = HBoxContainer.new()
	_column.add_child(_header)
	portrait = TextureRect.new()
	portrait.custom_minimum_size = Vector2(44, 44)
	portrait.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	portrait.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_header.add_child(portrait)
	var name_column := VBoxContainer.new()
	name_column.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	name_column.add_theme_constant_override("separation", 0)
	_header.add_child(name_column)
	heading = Label.new()
	heading.add_theme_font_size_override("font_size", 22)
	heading.add_theme_color_override("font_color", Color("ffc176"))
	heading.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	heading.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	name_column.add_child(heading)
	scroll_hint = Label.new()
	scroll_hint.add_theme_font_size_override("font_size", 12)
	scroll_hint.add_theme_color_override("font_color", Color("a8d6e9"))
	scroll_hint.mouse_filter = Control.MOUSE_FILTER_IGNORE
	scroll_hint.text = "↓ Przesuń tekst"
	scroll_hint.hide()
	name_column.add_child(scroll_hint)
	close_button = Button.new()
	close_button.text = "×"
	close_button.custom_minimum_size = Vector2(44, 44)
	close_button.pressed.connect(close)
	_header.add_child(close_button)
	line_scroll = ScrollContainer.new()
	line_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	line_scroll.scroll_deadzone = 8
	line_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_column.add_child(line_scroll)
	line = RichTextLabel.new()
	line.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	line.fit_content = true
	line.scroll_active = false
	line.visible_characters_behavior = TextServer.VC_CHARS_AFTER_SHAPING
	line.add_theme_font_size_override("normal_font_size", text_size)
	line.mouse_filter = Control.MOUSE_FILTER_PASS
	line.gui_input.connect(func(event: InputEvent):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			skip())
	line_scroll.add_child(line)
	scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	scroll.scroll_deadzone = 8
	_column.add_child(scroll)
	answers = VBoxContainer.new()
	answers.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	answers.add_theme_constant_override("separation", ANSWER_GAP)
	scroll.add_child(answers)
	screen.hide()

func _layout() -> void:
	if panel == null: return
	# Shaping may change the fitted height after the parent has sorted already.
	# Refresh its range on the next layout, outside the child's resize callback.
	var line_minimum := line.get_combined_minimum_size()
	if not line_minimum.is_equal_approx(_line_minimum):
		_line_minimum = line_minimum
		line_scroll.queue_sort()
	var size := get_viewport().get_visible_rect().size
	journal.position = Vector2(size.x - 158, 20)
	journal.size = Vector2(44, 44)
	clock_label.position = Vector2(20, 76)
	var width := minf(620, size.x - 32)
	var fraction := 0.72 if _mode == "journal" else 0.57
	var height := minf(size.y - 100, size.y * fraction)
	# Reserve the hint row even when no scrolling is needed. Showing it must
	# never move the panel or the first line of the next NPC utterance.
	_header.custom_minimum_size.y = heading.get_combined_minimum_size().y + scroll_hint.get_combined_minimum_size().y if _mode == "dialogue" else 0.0
	var chrome_height := 24 + _header.get_combined_minimum_size().y + 16
	var available := maxf(0, height - chrome_height)
	if _mode == "dialogue":
		# Reserve three reply rows regardless of how many choices this line has.
		# Text, answers and the camera framing then stay in the same place.
		# Keep at least one reply reachable if the window is unusually short.
		available = maxf(0, size.y - 100 - chrome_height)
		var text_height := minf(npc_text_height, maxf(0, available - ANSWER_HEIGHT))
		var reply_height := minf(THREE_ANSWER_ROWS, maxf(0, available - text_height))
		scroll.size_flags_vertical = Control.SIZE_FILL
		scroll.custom_minimum_size.y = reply_height
		line_scroll.size_flags_vertical = Control.SIZE_FILL
		line_scroll.custom_minimum_size.y = text_height
		height = chrome_height + text_height + reply_height
	else:
		scroll.custom_minimum_size.y = 0
		scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
		line_scroll.size_flags_vertical = Control.SIZE_FILL
		line_scroll.custom_minimum_size.y = minf(line.get_content_height(), available * 0.25)
	panel.position = Vector2((size.x - width) / 2, size.y - height - 20)
	panel.size = Vector2(width, height)
	_update_scroll_hint()
	if _mode == "dialogue" and is_instance_valid(_camera) and is_instance_valid(context.player.focus):
		# Reframe into the visible world above the compact panel, including resize.
		_camera.transform = _camera_transform
		var subject: Vector3 = context.player.focus.global_position
		var depth := -_camera.to_local(subject).z
		var projected := _camera.unproject_position(subject)
		var desired := Vector2(projected.x, panel.position.y * 0.55)
		_camera.global_position += _camera.project_position(projected, depth) - _camera.project_position(desired, depth)

func _update_scroll_hint() -> void:
	var bar := line_scroll.get_v_scroll_bar()
	var overflow := bar.max_value > bar.page + 1
	scroll_hint.visible = _mode == "dialogue" and overflow
	if not scroll_hint.visible: return
	var up := bar.value > 1
	var down := bar.value + bar.page < bar.max_value - 1
	scroll_hint.text = ("↕" if up and down else ("↑" if up else "↓")) + " Przesuń tekst"

func _open(mode: String) -> void:
	if not _mode.is_empty(): return
	_mode = mode
	context.player.suspend(&"narrative_ui")
	_controls_visible = controls.visible if controls else false
	if controls:
		controls.clear_controls()
		controls.hide()
	_owns_pause = not get_tree().paused
	get_tree().paused = true
	var level := context.world.map_root()
	if mode == "dialogue" and level:
		_camera = level.get_node_or_null("Camera3D") as Camera3D
		if _camera:
			_camera_transform = _camera.transform
	screen.show()
	journal.hide()
	_layout()

func _dialogue_started() -> void:
	_open("dialogue")

func _line_changed(who: String, text: String, choices: Array) -> void:
	if _mode != "dialogue": return
	_clear_touch()
	heading.text = who
	portrait.texture = session.profile.portrait if session.profile else null
	portrait.visible = portrait.texture != null
	line.text = text + ("\n\n" + session.feedback if not session.feedback.is_empty() else "")
	line.visible_characters = 0
	_visible_chars = 0
	_elapsed = 0
	_typing_hold = 0
	_revision = session.revision
	_view = choices.duplicate(true)
	_clear_options()
	for index in choices.size():
		var choice: Dictionary = choices[index]
		var button := _button(choice.text + ("\n" + choice.reason if not choice.enabled else ""))
		button.modulate.a = 0
		button.disabled = true
		button.pressed.connect(_choose.bind(index, _revision))
		button.mouse_entered.connect(_select.bind(index))
		options.append(button)
	_selected = -1
	line_scroll.scroll_vertical = 0
	scroll.scroll_vertical = 0

func _button(text: String) -> Button:
	var button := Button.new()
	button.text = text
	button.tooltip_text = text
	button.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	button.alignment = HORIZONTAL_ALIGNMENT_LEFT
	button.custom_minimum_size.y = ANSWER_HEIGHT
	button.add_theme_font_size_override("font_size", text_size)
	button.add_theme_stylebox_override("normal", _style(Color("17354c")))
	button.add_theme_stylebox_override("hover", _style(Color("245975")))
	button.add_theme_stylebox_override("focus", _style(Color("245975")))
	answers.add_child(button)
	return button

func _clear_options() -> void:
	options.clear()
	for child in answers.get_children():
		answers.remove_child(child)
		child.queue_free()

func _process(dt: float) -> void:
	# Containers can temporarily grow while wrapped text is being measured at a
	# previous width. Reapply the viewport cap after layout, including during pause.
	if not _mode.is_empty(): _layout()
	journal.visible = _mode.is_empty() and not context.player.is_suspended()
	clock_label.visible = context.campaign.active and _mode.is_empty()
	if clock_label.visible:
		var seconds := ceili(context.campaign.remaining_seconds)
		clock_label.text = "Siostra  %02d:%02d" % [floori(seconds / 60.0), seconds % 60]
	if _mode != "dialogue": return
	var length := line.get_total_character_count()
	if _visible_chars < length:
		if _typing_hold > 0:
			_typing_hold -= dt
			return
		var previous := int(_visible_chars)
		_visible_chars = minf(length, _visible_chars + minf(dt, 0.1) * characters_per_second) if characters_per_second > 0 else float(length)
		for index in range(previous, int(_visible_chars)):
			if line.text[index] in [".", "!", "?", ",", ";", ":"]:
				_visible_chars = index + 1
				_typing_hold = 0.13 if line.text[index] in [".", "!", "?"] else 0.05
				break
		line.visible_characters = int(_visible_chars)
		return
	_elapsed += minf(dt, 0.1)
	for i in options.size():
		if _elapsed >= i * answer_delay:
			options[i].modulate.a = minf(1, options[i].modulate.a + dt * 12)
			options[i].disabled = not _view[i].enabled
			if _selected == -1 and not options[i].disabled:
				_select(i)

func skip() -> void:
	if _mode != "dialogue": return
	_visible_chars = line.get_total_character_count()
	line.visible_characters = -1
	_elapsed = 100
	for i in options.size():
		options[i].modulate.a = 1
		options[i].disabled = not _view[i].enabled

func _select(index: int) -> void:
	if index < 0 or index >= options.size() or options[index].disabled or options[index].modulate.a <= 0: return
	_selected = index
	options[index].grab_focus()

func _choose(index: int, expected: int) -> void:
	if _mode != "dialogue" or index < 0 or index >= options.size() or options[index].modulate.a <= 0 or options[index].disabled: return
	session.choose(StringName(_view[index].id), expected)

func _input(event: InputEvent) -> void:
	if not _mode.is_empty() and (event is InputEventScreenTouch or event is InputEventScreenDrag):
		_touch_input(event)
		return
	if not event is InputEventKey or not event.pressed or event.echo: return
	if _mode.is_empty():
		if event.physical_keycode == KEY_J and not context.player.is_suspended():
			open_journal()
			get_viewport().set_input_as_handled()
		return
	get_viewport().set_input_as_handled()
	if event.physical_keycode == KEY_ESCAPE:
		close()
	elif _mode == "dialogue":
		if event.physical_keycode in [KEY_SPACE, KEY_ENTER]:
			if _visible_chars < line.get_total_character_count() or options.any(func(button: Button): return button.modulate.a <= 0):
				skip()
			else:
				_choose(_selected, _revision)
		elif event.physical_keycode >= KEY_1 and event.physical_keycode <= KEY_9:
			_choose(event.physical_keycode - KEY_1, _revision)
		elif event.physical_keycode in [KEY_W, KEY_S, KEY_UP, KEY_DOWN]:
			var direction := -1 if event.physical_keycode in [KEY_W, KEY_UP] else 1
			for option_offset in range(1, options.size() + 1):
				var next := posmod(_selected + direction * option_offset, options.size())
				if not options[next].disabled and options[next].modulate.a > 0:
					_select(next)
					scroll.ensure_control_visible(options[next])
					break

func _clear_touch() -> void:
	_touch_index = -1
	_touch_scroll = null
	_touch_button = null
	_touch_text = false
	_touch_dragged = false

func _touch_input(event: InputEvent) -> void:
	# The game's multi-touch flight controls disable mouse-from-touch emulation.
	# Handle native touch here, consuming drags before they can activate a reply.
	if event is InputEventScreenTouch and event.pressed:
		if not panel.get_global_rect().has_point(event.position): return
		get_viewport().set_input_as_handled()
		if _touch_index != -1: return
		_touch_index = event.index
		_touch_origin = event.position
		_touch_last = event.position
		_touch_revision = session.revision
		if line_scroll.get_global_rect().has_point(event.position):
			_touch_scroll = line_scroll
			_touch_text = true
		elif scroll.get_global_rect().has_point(event.position):
			_touch_scroll = scroll
			for button: Button in answers.get_children():
				if button.get_global_rect().has_point(event.position) and not button.disabled and button.modulate.a > 0:
					_touch_button = button
		elif close_button.visible and close_button.get_global_rect().has_point(event.position):
			_touch_button = close_button
	elif _touch_index != -1:
		get_viewport().set_input_as_handled()
		if event.index != _touch_index: return
		if event is InputEventScreenDrag:
			_touch_dragged = _touch_dragged or event.position.distance_to(_touch_origin) > 8
			if _touch_dragged and is_instance_valid(_touch_scroll):
				_touch_scroll.scroll_vertical += roundi(_touch_last.y - event.position.y)
			_touch_last = event.position
		elif event is InputEventScreenTouch and not event.pressed:
			var tap: bool = not _touch_dragged and not event.canceled and event.position.distance_to(_touch_origin) <= 8 and _touch_revision == session.revision
			var button := _touch_button
			var on_text := _touch_text
			_clear_touch()
			if tap:
				if is_instance_valid(button) and button.get_global_rect().has_point(event.position) and not button.disabled:
					button.pressed.emit()
				elif on_text and line_scroll.get_global_rect().has_point(event.position):
					skip()

func open_journal() -> void:
	if context.player.is_suspended() or not _mode.is_empty(): return
	_open("journal")
	_fill_journal()

func _fill_journal() -> void:
	heading.text = "ZADANIA I PRZEDMIOTY"
	portrait.hide()
	line.text = "Dawki leku: %d    Kredyty: %.0f" % [context.campaign.medicine_doses, context.campaign.credits]
	line.visible_characters = -1
	line_scroll.scroll_vertical = 0
	_clear_options()
	var count := 0
	for q in context.narrative_catalog.quests:
		var state: Dictionary = context.narrative.data.quests.get(String(q.id), {})
		if state.is_empty(): continue
		count += 1
		var text := ("★ " if context.narrative.data.tracked == String(q.id) else "") + q.title + ("  ✓" if state.status == "completed" else ("  • do oddania" if state.status == "ready" else "")) + "\n" + q.description
		for objective in q.objectives:
			if q.has_branches() and not state.get("path", []).has(String(objective.id)):
				continue
			text += "\n%s  %.0f / %.0f" % [objective.description, state.progress[String(objective.id)], objective.required]
		var button := _button(text)
		button.disabled = state.status == "completed"
		button.pressed.connect(func(): context.narrative.track(String(q.id)))
	if count == 0:
		_button("Nie masz jeszcze zadań.").disabled = true
	for item in context.narrative_catalog.items:
		var amount: int = context.narrative.data.items.get(String(item.id), 0)
		if amount > 0:
			_button("%s  × %d" % [item.display_name, amount]).disabled = true

func _state_changed() -> void:
	if context.player_state.health <= 0 or context.campaign.flags.get("__campaign_expired", false):
		if _mode == "game_over": return
		if not _mode.is_empty(): close()
		_open("game_over")
		heading.text = "ARI NIE ŻYJE" if context.player_state.health <= 0 else "CZAS SIĘ SKOŃCZYŁ"
		close_button.hide()
		line.text = "Upadek okazał się śmiertelny." if context.player_state.health <= 0 else "Nie udało się dostarczyć leku na czas."
		line.visible_characters = -1
		_clear_options()
		var maps: MapRouter = context.systems.get(&"maps")
		if maps and maps.host.has_method("restart_game"):
			_button("Nowa gra").pressed.connect(maps.host.restart_game)
	elif _mode == "journal":
		_fill_journal()

func _dialogue_finished() -> void:
	if _mode == "dialogue": close()

func close() -> void:
	if _mode.is_empty() or _mode == "game_over": return
	_clear_touch()
	_mode = ""
	if session.is_active(): session.end()
	screen.hide()
	if is_instance_valid(_camera):
		_camera.transform = _camera_transform
	_camera = null
	if controls:
		controls.clear_controls()
		controls.visible = _controls_visible
	if _owns_pause: get_tree().paused = false
	_owns_pause = false
	context.player.resume(&"narrative_ui")

func _exit_tree() -> void:
	if _mode == "game_over": _mode = "journal"
	close()
	if session:
		session.started.disconnect(_dialogue_started)
		session.line_changed.disconnect(_line_changed)
		session.finished.disconnect(_dialogue_finished)
	if context:
		context.narrative.changed.disconnect(_state_changed)
