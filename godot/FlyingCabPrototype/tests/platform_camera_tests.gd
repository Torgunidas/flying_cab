extends SceneTree
var checks := 0
var failures := 0
var level: Node3D
var cab: FlightCab
var context: RuntimeContext
var largest_step := 0.0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok: print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func weight(at: Vector3, velocity := Vector3.ZERO) -> float:
	cab.linear_velocity = velocity
	return PlatformCameraFraming.approach_weight(cab, at, context.world, level.camera_tuning)

func place(at: Vector3, velocity := Vector3.ZERO) -> void:
	cab.global_position = at
	cab.linear_velocity = velocity
	cab.reset_physics_interpolation()
	# The renderer caches interpolated transforms per frame. Let it observe the
	# new physics pose before sampling a whole stationary camera transition.
	await physics_frame
	await physics_frame
	await process_frame

func advance_camera(frames := 120) -> void:
	for i in frames:
		var previous: float = level._camera_distance
		level._process(1.0 / 60.0)
		largest_step = maxf(largest_step, absf(level._camera_distance - previous))

func capture(name: String) -> void:
	if DisplayServer.get_name() == "headless": return
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/platform-camera-" + name + ".png")

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = context
	root.add_child(level)
	current_scene = level
	await level.prepare_gameplay()
	for i in 45: await physics_frame
	# Test presentation against the actual city's deck geometry without pilot input.
	level.set_process(false)
	level.set_physics_process(false)
	context.player.suspend(&"camera_fixture")
	cab = level.cab
	cab.freeze = true
	check(is_zero_approx(weight(Vector3(-22, 64, 0))), "open air retains the wide flight view")
	await place(Vector3(-22, 64, 0))
	level._snap_camera()
	check(is_equal_approx(level._camera_distance, 19.0), "open-air camera keeps the established flight scale")
	await capture("flight")
	var previous_weight := 0.0
	var previous_distance: float = level._camera_distance
	var progressive := true
	for altitude in [61.0, 58.0, 55.0, 54.4]:
		await place(Vector3(-22, altitude, 0))
		advance_camera()
		progressive = progressive and level._platform_approach >= previous_weight and level._camera_distance < previous_distance
		if altitude == 61.0: progressive = progressive and level._platform_approach > 0.0 and level._platform_approach < 1.0
		previous_weight = level._platform_approach
		previous_distance = level._camera_distance
		if altitude == 58.0: await capture("approach")
	check(progressive, "descending towards a deck progressively closes the frame before landing")
	check(absf(level._camera_distance - 6.0) < 0.05, "parked cab uses the close landing frame")
	await capture("landed")
	check(weight(Vector3(-15, 55, 0)) > 0.5, "approaching a deck from its side also starts the zoom")
	check(is_zero_approx(weight(Vector3(-22, 51, 0))), "flying underneath a terrace does not zoom into its underside")
	check(is_zero_approx(weight(Vector3(-22, 55, 20))), "distant depth layers cannot trigger a landing zoom")
	check(is_zero_approx(weight(Vector3(-22, 55, 0), Vector3(10.5, 0, 0))), "cruising past a terrace keeps room to see obstacles")
	var medium := weight(Vector3(-22, 55, 0), Vector3(5, 0, 0))
	check(medium > 0 and medium < weight(Vector3(-22, 55, 0), Vector3(2, 0, 0)), "slowing down smoothly adds approach framing")
	await place(Vector3(-22, 54.4, 0))
	advance_camera()
	var before_reversal: float = level._camera_distance
	await place(Vector3(-22, 61, 0), Vector3(0, 5, 0))
	advance_camera(1)
	check(level._camera_distance > before_reversal and level._camera_distance - before_reversal < 1.0, "takeoff begins opening the view without a camera cut")
	await place(Vector3(-22, 66, 0))
	advance_camera()
	check(absf(level._camera_distance - 19.0) < 0.02 and largest_step < 1.0, "leaving the terrace restores flight framing smoothly")
	var full_yard := true
	for x in [10.0, 34.0, 60.0]:
		full_yard = full_yard and weight(Vector3(x, 150.4, 0)) > 0.99
	check(full_yard, "both ends and the centre of the long fleet terrace share landing framing")
	# Authoring a new deck uses the same group and collider contract, without coordinates in code.
	var pad := StaticBody3D.new()
	pad.add_to_group("refuel_pad")
	pad.position = Vector3(70, 230, 0)
	var collision := CollisionShape3D.new()
	collision.name = "Collision"
	collision.shape = BoxShape3D.new()
	collision.shape.size = Vector3(8, 0.8, 6)
	pad.add_child(collision)
	level.add_child(pad)
	context.world.register(pad)
	check(weight(Vector3(70, 230.8, 0)) > 0.99, "newly authored decks participate without a hardcoded platform list")
	collision.disabled = true
	check(is_zero_approx(weight(Vector3(70, 230.8, 0))), "disabled deck geometry does not claim a landing approach")
	pad.free()
	check(is_zero_approx(weight(Vector3(70, 230.8, 0))), "removing a deck also removes its camera influence")
	print("PLATFORM_CAMERA_TESTS: %d/%d passed; max_transition_step=%.3f m" % [checks - failures, checks, largest_step])
	quit(0 if failures == 0 else 1)
