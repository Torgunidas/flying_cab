extends SceneTree
## Full physical flight through the authored route; no body relocation.
var failures := 0
var checks := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok:
		print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func _run() -> void:
	Engine.max_fps = 60
	var context := RuntimeContext.new()
	context.rides.enabled = true
	context.campaign.credits = 120
	root.add_child(context)
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	level.living_world_enabled = false # Isolated fixture; full population has its own integration suite.
	level.context = context
	root.add_child(level)
	level.set_physics_process(false)
	await level.prepare_gameplay()
	var quality := RenderPolicy.new()
	quality.apply(root, level, "balanced")
	context.player.suspend(&"warmup")
	await GraphicsWarmup.new().prepare(level)
	context.player.resume(&"warmup")
	var cab: FlightCab = level.cab
	var director: TaxiDirector = level.taxi
	var ride: Dictionary = context.rides.offers[0]
	var fuel_start := cab.fuel
	var clock := 0.0
	var index := 1
	var route := director.network.path_between(ride.origin, ride.destination)
	var landed := false
	var telemetry := FrameTelemetry.new()
	telemetry.context = context
	telemetry.profile = "balanced"
	root.add_child(telemetry)
	telemetry.begin()
	for frame in range(6000):
		await physics_frame
		clock += 1.0 / 60
		director.advance(1.0 / 60)
		if context.rides.completed == 1:
			break
		if ride.phase == "riding" and not landed:
			var target: Vector3 = route[index]
			var offset := target - cab.global_position
			if index < route.size() - 1 and offset.length() < 0.7:
				index += 1
				target = route[index]
				offset = target - cab.global_position
			if index == route.size() - 1:
				offset.y -= 0.55
				if absf(offset.x) < 0.25 and offset.y > -0.25 and cab.linear_velocity.length() < 1.0:
					landed = true
			var desired := Vector3(clampf(offset.x * 1.5, -5, 5), clampf(offset.y * 1.5, -3, 4), 0)
			context.player.dispatch(Vector2(clampf((desired.x - cab.linear_velocity.x) * 3 / cab.definition.horizontal_acceleration, -1, 1), clampf((cab.definition.gravity + (desired.y - cab.linear_velocity.y) * 4) / cab.definition.vertical_acceleration, 0, 1)))
		if landed or ride.phase != "riding":
			context.player.dispatch(Vector2.ZERO)
	check(context.rides.completed == 1, "continuous physical flight reaches the fare's actual destination and pays")
	check(cab.support_body == director.network.stops[ride.destination].get_parent() and cab.sleeping, "the car settles on the requested terrace after the route")
	check(cab.hull == 100, "the route can be flown without touching a building or damaging the cab")
	check(cab.fuel < fuel_start and cab.fuel > 0, "the full trip consumes the real vehicle's fuel without refueling en route")
	check(is_equal_approx(context.campaign.credits, 120 + ride.fare), "full physical trip pays exactly the price offered before departure")
	check(cab.state.passenger_ids.is_empty() and not director.departures.is_empty(), "the transported person physically leaves the cab at the destination")
	print("TAXI_ROUTE_RESULT: seconds=", clock, " fuel=", fuel_start - cab.fuel, " fare=", ride.fare, " position=", cab.global_position)
	if DisplayServer.get_name() != "headless":
		telemetry.enabled = false
		for i in range(10):
			await process_frame
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/taxi-delivered.png")
		var file := FileAccess.open("res://build/taxi-route-performance.json", FileAccess.WRITE)
		file.store_string(JSON.stringify(telemetry.report(), "\t"))
	telemetry.queue_free()
	level.queue_free()
	await process_frame
	context.queue_free()
	await process_frame
	print("TAXI_ROUTE_TESTS: ", checks - failures, "/", checks)
	quit(1 if failures else 0)
