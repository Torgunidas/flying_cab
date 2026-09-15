extends SceneTree
var checks := 0
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	print("PASS: " if ok else "FAIL: ", label)
	if not ok:
		failures += 1

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
	var game: Node = load("res://scenes/game.tscn").instantiate()
	game.save_path = "res://build/living-render-%d.json" % Time.get_ticks_usec()
	root.add_child(game)
	current_scene = game
	game.restart_game()
	await process_frame
	var deadline := Time.get_ticks_msec() + 90000
	while game.maps.busy and Time.get_ticks_msec() < deadline:
		await process_frame
	var level: Node3D = game.maps.current
	var world: LivingWorldDirector = level.living_world
	check(not game.maps.busy and world.initialized and world.cars.size() == 50, "normal startup prepares all 50 NPC cars before gameplay")
	var at: Vector3 = world.cars["traffic/outer_clockwise/00"].global_position
	await frames(60)
	check(at.distance_to(world.cars["traffic/outer_clockwise/00"].global_position) > 2, "traffic resumes real physics after graphics warmup")
	check(level.on_foot.try_exit(level.cab), "Ari can exit beside the resident's cab in the rendered city")
	await frames(80)
	Input.action_press("flight_left")
	await frames(90)
	Input.action_release("flight_left")
	check(level.on_foot.actor.global_position.x < level.cab.global_position.x - 0.5, "rendered Ari walks in front of the parked cab")
	await capture("living-world-depot")
	# Observer camera for review only; population keeps running on its normal tick.
	level.set_process(false)
	var captured_fuel: float = level.cab.fuel
	for id in ["traffic/crosstown/01", "traffic/outer_clockwise/00", "traffic/velvet/00"]:
		var car: FlightCab = world.cars[id]
		for i in range(120):
			await process_frame
			level._camera_target = car.get_global_transform_interpolated().origin
			level._camera_distance = 19.0
			level._position_camera()
			level.get_node("Atmosphere").apply_height(car.global_position.y)
		await capture("living-world-" + id.split("/")[1])
	var visible_cars := 0
	for car: FlightCab in world.cars.values():
		if car.visible:
			visible_cars += 1
	check(visible_cars == 50 and level.cab.fuel == captured_fuel, "all population bodies persist during observation")
	print("FRAME_TELEMETRY: ", game.telemetry.report())
	print("LIVING_WORLD_RENDER_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)
