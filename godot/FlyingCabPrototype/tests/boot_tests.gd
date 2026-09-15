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

func _run() -> void:
	var game: Node = load("res://scenes/game.tscn").instantiate()
	game.save_path = "res://build/boot-test-session.json"
	root.add_child(game)
	current_scene = game
	game.restart_game()
	await process_frame
	check(game.maps.busy and game.context.player.is_suspended(), "render preparation locks control before gameplay")
	var start := Time.get_ticks_msec()
	var elapsed := 0
	while game.maps.busy and elapsed < 90000:
		await process_frame
		elapsed = Time.get_ticks_msec() - start
	check(not game.maps.busy, "graphics preparation finishes")
	check(not game._overlay.visible and not game.context.player.is_suspended(), "loading screen releases control after completed rendering")
	var cab: FlightCab = game.maps.current.cab
	check(cab.global_position.distance_to(Vector3(-22,54.4,0)) < 0.1 and cab.fuel == 100 and cab.command == Vector2.ZERO, "warmup retains spawn position, full tank and neutral controls")
	check(not cab.get_node("Headlights").lights_on and not cab.flight_fx.wake.visible, "warmup restores the actual hidden headlight and express states")
	check(game.context.systems.has(&"repair") and game.context.systems.has(&"dialogue") and game.context.systems.has(&"maps"), "composition root wires extensible systems into the session")
	for i in range(90):
		await process_frame
	var report: Dictionary = game.telemetry.report()
	check(report.total_frames > 0 and report.quantile_window_frames <= FrameTelemetry.CAPACITY, "bounded diagnostics measure real gameplay frames")
	check(cab.sleeping and cab.fuel_burn_rate == 0, "normal physical idle resumes after loading")
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/boot-ready.png")
	print("BOOT_TESTS: %d/%d passed; preparation_ms=%d" % [checks-failures,checks,elapsed])
	quit(0 if failures == 0 else 1)
