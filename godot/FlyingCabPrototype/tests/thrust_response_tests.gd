extends SceneTree
var checks := 0
var failures := 0
const BASIC = preload("res://resources/vehicles/basic_cab.tres")

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok:
		print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func pulse(held_seconds: float, hz: int) -> Dictionary:
	var response := ThrustResponse.new()
	var fuel := VehicleFuel.new()
	var state := VehicleState.new()
	state.fuel = 100
	var impulse := Vector2.ZERO
	var peak := 0.0
	for i in range(hz):
		var requested := Vector2.ONE if i < roundi(held_seconds * hz) else Vector2.ZERO
		var power := response.advance(requested, 1.0 / hz, BASIC)
		impulse += power / hz
		peak = maxf(peak, power.y)
		fuel.consume(state, BASIC, power, 0, 1, false, 1.0 / hz)
	return {"impulse": impulse, "peak": peak, "final": response.power, "burn": 100 - state.fuel}

func _run() -> void:
	var invalid: VehicleDefinition = BASIC.duplicate()
	invalid.thrust_rise_seconds = -1
	invalid.thrust_release_seconds = NAN
	check(invalid.validation_errors().size() == 2, "Inspector response times reject negative and non-finite values")
	var response := ThrustResponse.new()
	var first := response.advance(Vector2.ONE, 1.0 / 60, BASIC)
	check(first.x > 0 and first.x < 0.15 and first == Vector2(first.y, first.y), "first physics tick starts both engines gently, without a dead delay")
	for i in range(11):
		response.advance(Vector2.ONE, 1.0 / 60, BASIC)
	check(response.power == Vector2.ONE, "sustained input reaches full power within 0.2 seconds")
	var released := response.advance(Vector2.ZERO, 1.0 / 60, BASIC)
	check(released.x > 0.8 and released.x < 1.0 and released.x == released.y, "release fades power on both axes instead of cutting it instantly")
	for i in range(8):
		response.advance(Vector2.ZERO, 1.0 / 60, BASIC)
	check(response.power == Vector2.ZERO, "release reaches exact zero so a parked engine can sleep")
	response.advance(Vector2.ONE, 0.2, BASIC)
	var reversed := response.advance(Vector2(-1, 1), 0.02, BASIC)
	check(reversed.x > 0 and reversed.x < 1 and reversed.y == 1, "reversal releases the old direction without disturbing vertical power")
	response.advance(Vector2(-1, 1), 0.3, BASIC)
	check(response.power == Vector2(-1, 1), "opposite direction builds to full power after crossing zero")
	var left := ThrustResponse.new()
	var right := ThrustResponse.new()
	var symmetric := true
	for i in range(60):
		var target := 1.0 if i < 6 else (-1.0 if i < 24 else 0.0)
		var a := left.advance(Vector2(-target, 0), 1.0 / 60, BASIC)
		var b := right.advance(Vector2(target, 0), 1.0 / 60, BASIC)
		symmetric = symmetric and is_equal_approx(a.x, -b.x)
	check(symmetric, "left/right taps and reversals are symmetric throughout the manoeuvre")
	var short := pulse(0.05, 120)
	var long := pulse(0.1, 120)
	check(short.peak < 0.4 and short.impulse.y > 0 and short.impulse.y < long.impulse.y * 0.5, "a short tap delivers a proportionally smaller braking impulse")
	check(short.final == Vector2.ZERO and long.final == Vector2.ZERO, "short pulses leave no residual thrust")
	check(absf(long.burn - long.impulse.y * (BASIC.fuel_vertical_rate + BASIC.fuel_horizontal_rate)) < 0.00001, "fuel follows delivered power including the release tail")
	var slower := pulse(0.1, 60)
	check(absf(slower.impulse.y - long.impulse.y) < 0.004, "tap response stays consistent at 60 and 120 physics ticks")
	var instant: VehicleDefinition = BASIC.duplicate()
	instant.thrust_rise_seconds = 0
	instant.thrust_release_seconds = 0
	check(response.advance(Vector2.ONE, 1.0 / 60, instant) == Vector2.ONE and response.advance(Vector2(-1, 0), 1.0 / 60, instant) == Vector2(-1, 0), "zero response times remain a valid Inspector option")
	# Verify actual acceleration continuity, not only the input envelope.
	var nearly_off := FlightModel.step_velocity(Vector3(8, 8, 0), Vector2(0.001, 0.001), 1.0 / 60, BASIC)
	var off := FlightModel.step_velocity(Vector3(8, 8, 0), Vector2.ZERO, 1.0 / 60, BASIC)
	check(nearly_off.distance_to(off) < 0.002, "coast braking has no acceleration jump when the fading engine reaches zero")
	var cab: FlightCab = load("res://scenes/cab.tscn").instantiate()
	cab.definition = BASIC.duplicate()
	cab.definition.airspace_enabled = false
	cab.definition.highway_enabled = false
	cab.position = Vector3(0, 100, 0)
	root.add_child(cab)
	var player := PlayerSession.new()
	player.take_control(cab, &"flight")
	await frames(2)
	player.dispatch(Vector2.ONE)
	await frames(2)
	check(cab.applied_command.x > 0 and cab.applied_command.x < 0.3 and cab.applied_command.y == cab.applied_command.x, "real rigid body receives gradual diagonal power")
	await frames(15)
	check(cab.applied_command == Vector2.ONE, "real rigid body reaches full thrust while input is held")
	player.dispatch(Vector2.ZERO)
	await frames(2)
	check(cab.applied_command.x > 0 and cab.applied_command.x < 1 and cab.fuel_burn_rate > 0, "real release retains a brief, fuel-accounted soft impulse")
	await frames(12)
	check(cab.applied_command == Vector2.ZERO and cab.fuel_burn_rate == 0, "real release ends with no engine force or fuel burn")
	player.dispatch(Vector2.ONE)
	await frames(15)
	player.suspend(&"dialogue")
	check(cab.command == Vector2.ZERO and cab.applied_command == Vector2.ZERO and cab.thrust_response.power == Vector2.ZERO, "a session lock clears engine memory immediately")
	player.resume(&"dialogue")
	await frames(2)
	check(cab.applied_command == Vector2.ZERO, "resuming a session does not replay previously accumulated power")
	player.dispatch(Vector2.ONE)
	await frames(15)
	cab.assign_driver(&"resident")
	check(cab.command == Vector2.ZERO and cab.thrust_response.power == Vector2.ZERO, "a new driver does not inherit the previous driver's throttle")
	cab.command = Vector2.ONE
	await frames(15)
	cab.fuel = 0
	await frames(2)
	check(cab.applied_command == Vector2.ZERO and cab.thrust_response.power == Vector2.ZERO, "empty fuel stops engines immediately even during a powered manoeuvre")
	cab.fuel = 100
	await frames(2)
	check(cab.applied_command.y > 0 and cab.applied_command.y < 0.3, "restoring fuel starts from low power even if input remained held")
	await frames(15)
	cab.apply_damage(100)
	var disabled_fuel := cab.fuel
	await frames(2)
	check(cab.applied_command == Vector2.ZERO and cab.thrust_response.power == Vector2.ZERO and cab.fuel == disabled_fuel, "destroyed engines cannot coast on stored thrust or burn fuel")
	cab.reset_flight()
	await frames(3)
	check(cab.command == Vector2.ZERO and cab.applied_command == Vector2.ZERO and cab.thrust_response.power == Vector2.ZERO, "reset clears accumulated engine power")
	var controls := FlightControls.new()
	controls.size = Vector2(540, 960)
	root.add_child(controls)
	controls._update_command()
	player.take_control(cab, &"flight")
	Input.action_press("flight_right")
	Input.action_press("flight_up")
	controls._update_command()
	player.dispatch(controls.command)
	await frames(4)
	var keyboard_power := cab.applied_command
	Input.action_release("flight_right")
	Input.action_release("flight_up")
	controls.clear_controls()
	controls._update_command()
	cab.clear_control_input()
	for index in [1, 2]:
		var touch := InputEventScreenTouch.new()
		touch.index = index
		touch.pressed = true
		touch.position = controls._controls[index].get_center()
		controls._input(touch)
	player.dispatch(controls.command)
	await frames(4)
	check(keyboard_power.is_equal_approx(cab.applied_command) and keyboard_power.x > 0 and keyboard_power.x < 0.5, "keyboard and two-finger touch feed the same gradual physical response")
	controls.clear_controls()
	cab.clear_control_input()
	# Two identical descents onto real colliders: a 100 ms tap must reduce
	# approach speed, then allow contact and true engine-off rest.
	var floor_body := StaticBody3D.new()
	var collider := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(20, 1, 4)
	collider.shape = shape
	floor_body.add_child(collider)
	floor_body.position.y = 40
	root.add_child(floor_body)
	var baseline: FlightCab = load("res://scenes/cab.tscn").instantiate()
	baseline.definition = BASIC.duplicate()
	baseline.definition.airspace_enabled = false
	baseline.definition.highway_enabled = false
	baseline.position = Vector3(-4, 42.5, 0)
	baseline.linear_velocity = Vector3(0, -2, 0)
	cab.spawn_transform.origin = Vector3(4, 42.5, 0)
	cab.reset_flight()
	await frames(2)
	cab.linear_velocity = Vector3(0, -2, 0)
	root.add_child(baseline)
	cab.command = Vector2(0, 1)
	await frames(6)
	check(cab.applied_command.y > 0 and cab.applied_command.y < 0.7 and cab.linear_velocity.y > baseline.linear_velocity.y + 0.4, "a 100 ms landing tap softens actual descent without demanding full thrust")
	cab.command = Vector2.ZERO
	await frames(180)
	print("TAPPED_LANDING: position=", cab.position, " velocity=", cab.linear_velocity, " grounded=", cab.grounded, " sleeping=", cab.sleeping, " hull=", cab.hull, " power=", cab.applied_command, " burn=", cab.fuel_burn_rate)
	check(cab.grounded and cab.sleeping and cab.hull == 100 and cab.applied_command == Vector2.ZERO and cab.fuel_burn_rate == 0, "tapped landing settles onto its real platform without damage, bounce or idle burn")
	print("THRUST_RESPONSE_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
