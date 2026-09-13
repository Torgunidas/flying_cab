extends SceneTree

var scene: Node3D
var cab: FlightCab
var controls: FlightControls
var checks := 0
var failures := 0
var resets := 0

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

func release_inputs() -> void:
	for action in ["flight_left", "flight_right", "flight_up"]:
		Input.action_release(action)

func place(position: Vector3) -> void:
	release_inputs()
	cab.spawn_transform.origin = position
	scene._reset()
	await frames(4)

func test_return(side: float, empty: bool, height: float) -> void:
	var edge := cab.world_definition.city_right if side > 0.0 else cab.world_definition.city_left
	await place(Vector3(edge - side * 0.25, height, 0))
	cab.fuel = 0.0 if empty else cab.definition.fuel_capacity
	cab.linear_velocity = Vector3(side * cab.definition.max_horizontal_speed, 0, 0)
	var action := "flight_right" if side > 0.0 else "flight_left"
	Input.action_press(action)
	Input.action_press("flight_up")
	var reset_count := resets
	var previous := cab.position
	var max_step := 0.0
	var activated := false
	var completed := false
	var crossed := false
	var input_overridden := true
	for i in range(900):
		await physics_frame
		max_step = maxf(max_step, cab.position.distance_to(previous))
		previous = cab.position
		crossed = crossed or (cab.position.x - edge) * side > 0.1
		if cab.airspace.returning:
			activated = true
			input_overridden = input_overridden and controls._autopilot and controls.command == Vector2.ZERO
		elif activated:
			completed = true
			break
	var label := "%s return%s at %.1f m" % ["right" if side > 0.0 else "left", " with empty tank" if empty else "", height]
	print("RETURN: %s, position=%s, velocity=%s, max_step=%.5f" % [label, cab.position, cab.linear_velocity, max_step])
	check(crossed and activated and completed, label + ": crosses the boundary and completes autopilot")
	check(completed and (edge - cab.position.x) * side > cab.world_definition.return_margin - 0.4, label + ": hands over safely inside the city")
	check(max_step < 0.5 and resets == reset_count, label + ": uses continuous physical flight without teleport/reset")
	check(input_overridden and not controls._autopilot, label + ": locks pilot input only during return")
	await frames(8)
	check(controls.command == Vector2.ZERO and cab.applied_command == Vector2.ZERO, label + ": held keys are not replayed after handover")
	Input.action_release("flight_up")
	Input.action_release(action)
	await frames(2)
	Input.action_press("flight_up")
	await frames(4)
	check(controls.command.y > 0.0 and (cab.applied_command.y == 0.0 if empty else cab.applied_command.y > 0.0), label + ": fresh input works and emergency reserve ends at handover")
	release_inputs()

func _run() -> void:
	scene = load("res://scenes/flight_lab.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	cab = scene.get_node("Cab")
	cab.definition = cab.definition.duplicate()
	cab.definition.highway_enabled = false
	controls = scene.get_node("HUD/Controls")
	cab.flight_reset.connect(func(): resets += 1)
	await frames(120)
	cab.fuel = 30.0
	await frames(120)
	check(cab.fuel > 79.0 and cab.fuel < 82.0 and cab.fuel_burn_rate == 0.0, "a parked cab refuels at 25 units/s without burning fuel")
	await frames(120)
	check(cab.fuel == cab.definition.fuel_capacity, "refueling stops at tank capacity")
	await place(Vector3(28, cab.world_definition.ground_height + 0.36, 0))
	await frames(120)
	cab.fuel = 30.0
	await frames(120)
	check(cab.fuel == 30.0 and not cab.refueling, "ordinary ground is not a fuel station")
	await place(Vector3(0, 35, 0))
	cab.fuel = 50.0
	await frames(30)
	check(cab.fuel == 50.0 and cab.linear_velocity.y < -1.0 and cab.applied_command == Vector2.ZERO, "unpowered falling does not burn fuel")
	await place(Vector3(0, 35, 0))
	Input.action_press("flight_up")
	Input.action_press("flight_right")
	await frames(60)
	var combined_rate := cab.definition.fuel_vertical_rate + cab.definition.fuel_horizontal_rate
	var startup_burn := combined_rate * (1.0 - cab.definition.thrust_rise_seconds * 0.5)
	check(absf(cab.fuel - (100.0 - startup_burn)) < 0.05, "ordinary diagonal startup burns fuel in proportion to its rising power")
	var steady_fuel := cab.fuel
	await frames(60)
	check(absf(steady_fuel - cab.fuel - combined_rate) < 0.05, "sustained full diagonal thrust keeps both original engine burn rates")
	await place(Vector3(0, 35, 0))
	cab.fuel = 0.0
	Input.action_press("flight_up")
	Input.action_press("flight_right")
	await frames(30)
	check(cab.applied_command == Vector2.ZERO and cab.linear_velocity.y < -1.0 and absf(cab.linear_velocity.x) < 0.01, "empty tank cuts pilot thrust while gravity remains active")
	await place(Vector3(0, cab.world_definition.max_altitude - 12.0, 0))
	Input.action_press("flight_up")
	var top := cab.position.y
	var max_ceiling_burn := 0.0
	var previous_climb_speed := cab.linear_velocity.y
	var max_braking_step := 0.0
	var slept_at_ceiling := false
	var reset_count := resets
	for i in range(900):
		await physics_frame
		top = maxf(top, cab.position.y)
		max_ceiling_burn = maxf(max_ceiling_burn, cab.fuel_burn_rate)
		max_braking_step = maxf(max_braking_step, previous_climb_speed - cab.linear_velocity.y)
		previous_climb_speed = cab.linear_velocity.y
		slept_at_ceiling = slept_at_ceiling or cab.sleeping
	check(top > cab.world_definition.max_altitude - 3.0 and top <= cab.world_definition.max_altitude + 0.01 and absf(cab.linear_velocity.y) < 0.3 and resets == reset_count, "held thrust slows smoothly and cannot creep through the soft ceiling")
	check(controls._ceiling_warning and max_ceiling_burn > cab.definition.fuel_vertical_rate * 2.8, "ceiling warning accompanies increased thrust fuel use")
	check(max_braking_step < 1.0, "soft ceiling brakes progressively instead of stopping ascent in one tick")
	check(not slept_at_ceiling, "near-zero speed at the ceiling never parks the airborne body")
	var before_descent := cab.position.y
	var fuel_before_descent := cab.fuel
	Input.action_release("flight_up")
	var release_ticks := ceili(cab.definition.thrust_release_seconds * Engine.physics_ticks_per_second) + 2
	await frames(release_ticks)
	var fuel_after_release := cab.fuel
	var max_release_burn := cab.definition.fuel_vertical_rate * cab.definition.ceiling_fuel_multiplier * (cab.definition.thrust_release_seconds * 0.5 + 2.0 / Engine.physics_ticks_per_second)
	check(fuel_after_release < fuel_before_descent and fuel_after_release >= fuel_before_descent - max_release_burn, "ceiling release accounts for only the short fading engine impulse")
	await frames(90 - release_ticks)
	check(cab.position.y < before_descent - 5.0 and cab.fuel == fuel_after_release and cab.applied_command == Vector2.ZERO, "free descent after engine release has no passive ceiling fuel penalty")
	await frames(60)
	check(not controls._ceiling_warning, "ceiling warning clears after leaving the restricted altitude")
	await test_return(1.0, false, 30.0)
	await test_return(-1.0, false, 30.0)
	await test_return(1.0, true, cab.world_definition.ground_height + 0.4)
	await test_return(-1.0, true, cab.world_definition.max_altitude - 4.0)
	await place(Vector3(cab.world_definition.city_right + 2, 30, 0))
	await frames(8)
	check(cab.airspace.returning, "outside spawn activates return")
	cab.spawn_transform.origin = scene.get_node("City/LandingPads/Pad0").global_position + Vector3(0, 0.8, 0)
	scene._reset()
	await frames(120)
	check(not cab.airspace.returning and not controls._autopilot and cab.applied_command == Vector2.ZERO and cab.fuel == cab.definition.fuel_capacity, "reset cancels autopilot, clears thrust and restores the prototype tank")
	controls.set_autopilot(true)
	var touch := InputEventScreenTouch.new()
	touch.index = 5
	touch.position = controls._controls[0].get_center()
	touch.pressed = true
	controls._input(touch)
	check(controls.command == Vector2.ZERO, "autopilot also suppresses touch input")
	controls.set_autopilot(false)
	var drag := InputEventScreenDrag.new()
	drag.index = 5
	drag.position = touch.position
	controls._input(drag)
	check(controls.command == Vector2.ZERO and controls._pointers.is_empty(), "a finger held during return is not replayed after handover")
	touch.pressed = false
	controls._input(touch)
	touch.pressed = true
	controls._input(touch)
	check(controls.command.x == -1.0, "a fresh touch resumes pilot control")
	touch.pressed = false
	controls._input(touch)
	print("AIRSPACE_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
