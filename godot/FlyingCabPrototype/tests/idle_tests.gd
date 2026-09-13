extends SceneTree

# Observe both sides of the custom integrator, rather than trusting the HUD.
class ProbeCab extends FlightCab:
	var solver_velocity := Vector3.ZERO
	var integrated_command := Vector2.ZERO

	func _integrate_forces(state: PhysicsDirectBodyState3D) -> void:
		solver_velocity = state.linear_velocity
		integrated_command = command
		super._integrate_forces(state)

var failures := 0
var checks := 0
var scene: Node3D
var cab: ProbeCab
var controls: FlightControls

func _initialize() -> void:
	call_deferred("_run")

func check(condition: bool, label: String) -> void:
	checks += 1
	if condition:
		print("PASS: " + label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func inspect_idle(label: String) -> void:
	var start := cab.position
	var max_speed := 0.0
	var min_speed := INF
	var max_solver_speed := 0.0
	var max_displacement := 0.0
	var unexpected_inputs := 0
	var ungrounded_frames := 0
	var sleep_frames := 0
	var unwanted_thrust := false
	var readings := {}
	for i in range(600):
		await physics_frame
		var keyboard := Vector2(Input.get_axis("flight_left", "flight_right"), Input.get_action_strength("flight_up"))
		if keyboard != Vector2.ZERO or controls.command != Vector2.ZERO or cab.command != Vector2.ZERO or cab.integrated_command != Vector2.ZERO or not controls._pointers.is_empty():
			unexpected_inputs += 1
		max_speed = maxf(max_speed, cab.linear_velocity.length())
		min_speed = minf(min_speed, cab.linear_velocity.length())
		max_solver_speed = maxf(max_solver_speed, cab.solver_velocity.length())
		max_displacement = maxf(max_displacement, cab.position.distance_to(start))
		ungrounded_frames += int(not cab.grounded)
		sleep_frames += int(cab.sleeping)
		readings[controls._readout] = true
		unwanted_thrust = unwanted_thrust or cab.applied_command != Vector2.ZERO or cab.fuel_burn_rate != 0.0
	print("IDLE %s: inputs=%d/600, raw_speed=%.7f..%.7f m/s, solver_max=%.7f, drift=%.7f m, no_support=%d, asleep=%d, HUD=%s" % [label, unexpected_inputs, min_speed, max_speed, max_solver_speed, max_displacement, ungrounded_frames, sleep_frames, str(readings.keys())])
	check(unexpected_inputs == 0, label + ": no keyboard, pointer or integrated thrust input while parked")
	check(max_speed < 0.001 and max_displacement < 0.001, label + ": physics body actually rests, independently of HUD filtering")
	check(readings.size() == 1 and readings.has("00"), label + ": speedometer remains at zero")
	check(not unwanted_thrust, label + ": parked engines apply no thrust and burn no fuel")

func _run() -> void:
	scene = load("res://scenes/flight_lab.tscn").instantiate()
	var original: FlightCab = scene.get_node("Cab")
	var tuning := original.definition
	original.set_script(ProbeCab)
	cab = original as ProbeCab
	cab.definition = tuning
	root.add_child(scene)
	current_scene = scene
	controls = scene.get_node("HUD/Controls")
	await frames(120)
	await inspect_idle("start")
	var initial_height := cab.position.y
	Input.action_press("flight_up")
	await frames(30)
	check(cab.position.y > initial_height + 0.45, "thrust lifts the cab after a long idle")
	Input.action_release("flight_up")
	await frames(240)
	await inspect_idle("landing after released thrust")
	scene._reset()
	await frames(120)
	await inspect_idle("reset")
	var parked_position := cab.position
	cab.apply_central_impulse(Vector3(200, 0, 0))
	await frames(8)
	check(cab.position.x > parked_position.x + 0.05 and cab.linear_velocity.x > 0.1, "an external impact wakes and moves the parked cab")
	check(cab.command == Vector2.ZERO and cab.integrated_command == Vector2.ZERO, "external motion does not become an engine input")
	scene._reset()
	await frames(120)
	var supported_y := cab.position.y
	scene.get_node("City/LandingPads/Pad0").queue_free()
	await frames(30)
	check(cab.position.y < supported_y - 0.5 and cab.linear_velocity.y < -1.0 and not cab.grounded, "removing support wakes the cab and restores free fall without input")
	print("IDLE_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
