extends SceneTree
var failures := 0
var checks := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if not ok:
		failures += 1
		push_error("FAIL: " + label)
	else:
		print("PASS: ", label)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func capture(file: String) -> void:
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/" + file + ".png")

func _run() -> void:
	if DisplayServer.get_name() == "headless":
		quit(1)
		return
	var context := RuntimeContext.new()
	context.rides.enabled = true
	root.add_child(context)
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	current_scene = level
	level.set_physics_process(false)
	await level.prepare_gameplay()
	await frames(45)
	var foot: OnFootInteraction = level.on_foot
	check(foot.try_exit(level.cab), "rendered city allows safe depot exit")
	await frames(90)
	check(absf(level._camera_distance - level.camera_tuning.on_foot_distance) < 0.1, "camera blends into the closer on-foot frame")
	check(foot.actor.visible and level.controls.interaction_label == "WSIĄDŹ / Q", "rendered Ari and contextual entry control are visible")
	await capture("on-foot-depot")
	context.player.dispatch(Vector2(1, 1))
	await frames(10)
	check(foot.actor.movement_state == &"jump", "rendered Ari uses the airborne pose")
	await capture("on-foot-jump")
	context.player.dispatch(Vector2.ZERO)
	await frames(60)
	foot.recover()
	await frames(10)
	# Render preparation must keep visiting the city when the saved focus is Ari.
	var at := foot.actor.global_transform
	var fuel: float = level.cab.fuel
	var warmup := GraphicsWarmup.new()
	await warmup.prepare(level)
	check(foot.actor.visible and foot.actor.global_transform.is_equal_approx(at) and context.player.focus == foot.actor and level.cab.fuel == fuel, "warmup restores the on-foot actor, focus and parked vehicle state")
	check(warmup.completed_views > 100, "on-foot warmup still renders the complete city sampling path")
	await frames(12)
	var saved_position := foot.actor.global_position
	context.save_to("res://build/on-foot-boot-session.json")
	check(foot.try_enter(level.cab), "rendered character returns to the same cab")
	await frames(90)
	check(absf(level._camera_distance - level.camera_tuning.camera_distance) < 0.1 and not foot.actor.visible, "driving camera returns and the seated character is hidden")
	await capture("on-foot-back-in-cab")
	level.queue_free()
	context.queue_free()
	await process_frame
	var game: Node = load("res://scenes/game.tscn").instantiate()
	game.save_path = "res://build/on-foot-boot-session.json"
	root.add_child(game)
	current_scene = game
	await process_frame
	var deadline := Time.get_ticks_msec() + 60000
	while game.maps.busy and Time.get_ticks_msec() < deadline:
		await process_frame
	check(not game.maps.busy and game.context.player.focus is WalkingActor and not game.context.player.is_suspended(), "real game entry point restores on-foot save through the loading screen")
	check(game.context.player.focus.global_position.distance_to(saved_position) < 0.1 and game.maps.current.cab.fuel == fuel, "full startup retains Ari's saved location and parked fuel")
	await frames(90)
	await capture("on-foot-restored")
	print("ON_FOOT_RENDER_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
