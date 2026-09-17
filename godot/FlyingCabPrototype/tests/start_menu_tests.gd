extends SceneTree
var checks := 0
var failures := 0
var game: Node
var save_path := ""

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

func wait_ready() -> void:
	var deadline := Time.get_ticks_msec() + 60000
	while game._launching and Time.get_ticks_msec() < deadline: await process_frame
	check(not game._launching and is_instance_valid(game.maps.current), "session finishes preparation")

func open_game(path: String) -> void:
	game = load("res://scenes/game.tscn").instantiate()
	game.save_path = path
	root.add_child(game)
	current_scene = game
	await frames(3)

func close_game() -> void:
	game.queue_free()
	await process_frame

func capture(name: String) -> void:
	await frames(6)
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/start-menu-" + name + ".png")

func fresh_state() -> void:
	var c: RuntimeContext = game.context
	check(c.player_state.health == PlayerState.MAX_HEALTH, "new game restores Ari's health")
	check(c.campaign.credits == 120 and not c.campaign.active and c.campaign.remaining_seconds == 0 and c.campaign.medicine_doses == 0 and c.campaign.flags.is_empty(), "new game resets wallet, sister timer, doses and facts")
	check(c.narrative.data == NarrativeService.empty_state(), "new game removes quests, inventory, access and narrative receipts")
	check(c.rides.completed == 0 and c.rides.income == 0 and c.rides.recovery_debt == 0 and c.rides.active.is_empty(), "new game resets taxi progress and debt")
	check(c.player.focus == game.maps.current.cab and c.player_state.mode == "flight" and game.maps.current.cab.fuel == 100 and game.maps.current.cab.state.condition == 1, "new game restores Ari and a fully serviced starting cab")
	check(c.living.agents.size() == 75 and c.vehicles.size() == 63 and not c.map_states.has("old_room"), "new game recreates the population and fleet without old map state")
	check(not paused and not c.player.is_suspended(), "old modal pauses and control locks do not survive reset")
	var saved := RuntimeContext.new()
	check(saved.load_from(save_path) and saved.campaign.credits == 120 and saved.narrative.data.quests.is_empty(), "fresh state replaces autosave immediately")
	saved.free()

func _run() -> void:
	save_path = "res://build/start-menu-session-%d.json" % Time.get_ticks_usec()
	await open_game(save_path)
	check(game._overlay.visible and game.context == null and game.maps.current == null, "normal startup shows menu before creating world or campaign")
	check(game._overlay.continue_button.disabled and not game._overlay.new_game_button.disabled, "no autosave disables Continue and offers New Game")
	game._process(25)
	game._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
	check(not FileAccess.file_exists(save_path), "idle start screen and focus loss cannot create an autosave")
	if OS.get_cmdline_user_args().has("--capture-only"):
		await _render_menu()
		return
	game._overlay.new_game_button.pressed.emit()
	game._overlay.new_game_button.pressed.emit()
	await wait_ready()
	fresh_state()
	check(game.get_children().filter(func(node): return node is RuntimeContext).size() == 1, "double click creates only one session")
	var c: RuntimeContext = game.context
	c.campaign.credits = 777
	c.player_state.health = 63
	c.campaign.medicine_doses = 3
	c.campaign.active = true
	c.campaign.remaining_seconds = 500
	var e := NarrativeEffect.new()
	e.kind = "start_quest"
	e.key = &"first_dose"
	c.narrative.execute([e], "fixture")
	c.campaign.flags.promised_froggy = 1.0
	c.narrative.data.items.froggy_note = 2
	c.rides.completed = 7
	c.rides.income = 432
	c.rides.recovery_debt = 35
	c.map_states.old_room = {"visited": true}
	game.maps.current.cab.fuel = 32
	game.maps.current.cab.state.condition = 0.4
	game._save(false)
	var original := FileAccess.get_file_as_string(save_path)
	await close_game()
	await open_game(save_path)
	check(not game._overlay.continue_button.disabled and game.context == null, "existing autosave offers Continue without loading it automatically")
	game._overlay.new_game_button.pressed.emit()
	check(game._overlay.cancel_button.visible and not game._launching and FileAccess.get_file_as_string(save_path) == original, "first New Game press asks before replacing existing progress")
	game._overlay.cancel_button.pressed.emit()
	check(game._overlay.continue_button.visible and FileAccess.get_file_as_string(save_path) == original, "cancel preserves the complete autosave")
	game._overlay.continue_button.pressed.emit()
	await wait_ready()
	c = game.context
	check(c.player_state.health == 63, "Continue restores Ari's injuries")
	check(c.campaign.credits == 777 and c.campaign.medicine_doses == 3 and c.narrative.status(&"first_dose") == "active" and c.narrative.data.items.froggy_note == 2 and c.rides.completed == 7 and c.rides.recovery_debt == 35, "Continue restores campaign, quest, item and taxi progress")
	check(game.maps.current.cab.fuel == 32 and game.maps.current.cab.state.condition == 0.4, "Continue restores saved vehicle condition")
	var hud: TaxiHud = game.maps.current.taxi_hud
	hud.open_overlay("options")
	var old_context := c.get_instance_id()
	hud.press(hud._new_game.get_center(), 0)
	check(hud._new_game_armed and paused and game.context.get_instance_id() == old_context, "in-game options exposes a confirmed New Game action")
	hud.close_overlay()
	hud.open_overlay("options")
	check(not hud._new_game_armed, "leaving options cancels reset confirmation")
	hud.press(hud._new_game.get_center(), 0)
	hud.press(hud._new_game.get_center(), 0)
	await wait_ready()
	check(game.context.get_instance_id() != old_context, "confirmed in-game reset replaces the entire runtime session")
	fresh_state()
	# The next launch must read the fresh state, even without waiting for autosave.
	await close_game()
	await open_game(save_path)
	game._overlay.continue_button.pressed.emit()
	await wait_ready()
	check(game.context.campaign.credits == 120 and game.context.narrative.data.quests.is_empty(), "immediate relaunch cannot resurrect the old autosave")
	game.context.campaign.active = true
	game.context.campaign.remaining_seconds = 1
	game.context.campaign.advance(2)
	check(paused and game.maps.current.narrative_panel._mode == "game_over", "campaign end keeps its paused presentation")
	game.maps.current.narrative_panel.answers.get_child(0).pressed.emit()
	await wait_ready()
	fresh_state()
	game.context.damage_player(100)
	await frames(3)
	check(paused and game.maps.current.narrative_panel.heading.text == "ARI NIE ŻYJE", "player death opens its own game-over screen")
	var death_save := RuntimeContext.new()
	check(death_save.load_from(save_path) and death_save.player_state.health == 0, "death immediately reaches autosave despite the pause")
	death_save.free()
	await close_game()
	await open_game(save_path)
	game._overlay.continue_button.pressed.emit()
	await wait_ready()
	check(paused and game.context.player_state.health == 0 and game.maps.current.narrative_panel.heading.text == "ARI NIE ŻYJE", "full Continue from a fatal save keeps the death screen and pause")
	game.maps.current.narrative_panel.answers.get_child(0).pressed.emit()
	await wait_ready()
	fresh_state()
	await close_game()
	var invalid_path := "res://build/start-menu-invalid-%d.json" % Time.get_ticks_usec()
	var file := FileAccess.open(invalid_path, FileAccess.WRITE)
	file.store_string("{}")
	file.close()
	await open_game(invalid_path)
	game._overlay.continue_button.pressed.emit()
	while game._launching: await process_frame
	check(game.context == null and game._overlay.status.visible and FileAccess.get_file_as_string(invalid_path) == "{}", "invalid save remains untouched and returns a visible menu error")
	game._overlay.new_game_button.pressed.emit()
	game._overlay.new_game_button.pressed.emit()
	await wait_ready()
	check(game.context.campaign.credits == 120, "New Game can recover from an incompatible save")
	await close_game()
	await open_game("res://build/does-not-exist/start-menu.json")
	game._overlay.new_game_button.pressed.emit()
	while game._launching: await process_frame
	check(game.context == null and game._overlay.status.visible and not game._overlay.new_game_button.disabled, "failed new-save write stays on menu and allows retry")
	await close_game()
	_finish()

func _render_menu() -> void:
	for dimensions in [Vector2i(540, 960), Vector2i(360, 640), Vector2i(960, 540)]:
		root.content_scale_size = dimensions
		root.size = dimensions
		await frames(8)
		var screen: GameStartScreen = game._overlay
		var bounds := Rect2(Vector2.ZERO, Vector2(dimensions))
		check(bounds.encloses(screen.artwork.get_global_rect()) and bounds.encloses(screen.actions.get_global_rect()), "artwork and buttons fit %s" % dimensions)
		check(screen.artwork.stretch_mode == TextureRect.STRETCH_KEEP_ASPECT_CENTERED, "archive poster retains its complete composition")
		await capture("%dx%d" % [dimensions.x, dimensions.y])
	root.content_scale_size = Vector2i(540, 960)
	root.size = Vector2i(540, 960)
	game._overlay.show_menu(true)
	await capture("continue")
	game._overlay.new_game_button.pressed.emit()
	await capture("confirmation")
	game._overlay.show_menu(false)
	game._overlay.new_game_button.pressed.emit()
	await wait_ready()
	game.maps.current.taxi_hud.open_overlay("options")
	await capture("options")
	game.maps.current.taxi_hud.press(game.maps.current.taxi_hud._new_game.get_center(), 0)
	await capture("options-confirmation")
	await close_game()
	_finish()

func _finish() -> void:
	print("START_MENU_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
