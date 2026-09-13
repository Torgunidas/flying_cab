extends SceneTree

var failures := 0
var checks := 0
var controls: FlightControls

func _initialize() -> void:
	call_deferred("_run")

func check(condition: bool, label: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		push_error("FAIL: " + label)
	else:
		print("PASS: " + label)

func simulate(command: Vector2, seconds: float, hz: int, initial := Vector3.ZERO) -> Vector3:
	var tuning: VehicleDefinition = load("res://resources/vehicles/basic_cab.tres")
	var velocity := initial
	for i in range(roundi(seconds * hz)):
		velocity = FlightModel.step_velocity(velocity, command, 1.0 / hz, tuning)
	return velocity

func touch(index: int, pressed: bool, pos: Vector2) -> void:
	var event := InputEventScreenTouch.new()
	event.index = index
	event.pressed = pressed
	event.position = pos
	controls._input(event)

func drag(index: int, pos: Vector2) -> void:
	var event := InputEventScreenDrag.new()
	event.index = index
	event.position = pos
	controls._input(event)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func _run() -> void:
	check(simulate(Vector2.ZERO, 3.0, 60).y == -13.0, "free fall has a bounded terminal speed")
	check(simulate(Vector2.UP * -1.0, 3.0, 60).y == 11.5, "sustained thrust climbs to the UE speed cap")
	var right := simulate(Vector2(1, 1), 3.0, 60)
	var left := simulate(Vector2(-1, 1), 3.0, 60)
	check(is_equal_approx(right.x, 10.5) and is_equal_approx(left.x, -10.5), "left and right use symmetric horizontal speed caps")
	check(right.y == 11.5 and left.y == 11.5, "diagonal flight preserves full vertical thrust")
	check(absf(simulate(Vector2.ZERO, 2.0, 60, Vector3(10.5, 0, 0)).x) < 0.4, "release progressively brakes horizontal drift")
	check(simulate(Vector2.ZERO, 0.6, 60, Vector3(0, 11.5, 0)).y < 1.0, "release brakes upward motion before falling")
	check(simulate(Vector2(1, 1), 0.5, 60).distance_to(simulate(Vector2(1, 1), 0.5, 120)) < 0.03, "free-flight response is consistent across step rates")
	check(simulate(Vector2.ZERO, 1.0, 60, Vector3(0, 0, 15)).z == 0.0, "depth velocity cannot leak into planar flight")
	var scene: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	await process_frame
	controls = scene.get_node("HUD/Controls")
	controls.clear_controls()
	controls._update_command()
	var l := controls._controls[0].get_center()
	var r := controls._controls[1].get_center()
	var up := controls._controls[2].get_center()
	touch(0, true, l)
	touch(1, true, up)
	check(controls.command == Vector2(-1, 1), "two fingers combine left and thrust")
	touch(0, false, l)
	check(controls.command == Vector2(0, 1), "releasing direction keeps the other finger's thrust")
	touch(2, true, up)
	touch(1, false, up)
	check(controls.command.y == 1.0, "releasing one of two thrust fingers does not release the other")
	touch(2, false, up)
	touch(0, true, l)
	drag(0, r)
	check(controls.command == Vector2(1, 0), "thumb can slide from left to right")
	drag(0, Vector2(270, 300))
	check(controls.command == Vector2.ZERO, "dragging outside buttons releases thrust")
	touch(0, false, r)
	touch(0, true, l)
	touch(1, true, r)
	check(controls.command.x == 0.0, "opposing touch directions cancel")
	controls._notification(Control.NOTIFICATION_APPLICATION_FOCUS_OUT)
	check(controls.command == Vector2.ZERO and controls._pointers.is_empty(), "focus loss clears all pointers")
	controls._update_command()
	Input.action_press("flight_left")
	touch(0, true, up)
	touch(0, false, up)
	check(controls.command == Vector2(-1, 0), "touch release does not clear held keyboard input")
	Input.action_release("flight_left")
	controls.clear_controls()
	await frames(90)
	var cab: FlightCab = scene.get_node("Cab")
	var depot_rest: Vector3 = scene.get_node("City/LandingPads/Pad0").global_position + Vector3(0, 0.75, 0)
	check(cab.position.distance_to(depot_rest) < 0.08 and cab.grounded, "rigid body settles on the start pad")
	var start_y := cab.position.y
	var engine_start_ticks := ceili(cab.definition.thrust_rise_seconds * Engine.physics_ticks_per_second)
	Input.action_press("flight_up")
	await frames(45 + engine_start_ticks)
	check(cab.position.y > start_y + 2.5, "keyboard thrust lifts the physical cab off the pad")
	Input.action_press("flight_right")
	await frames(35)
	check(cab.position.x > depot_rest.x + 1.0 and absf(cab.position.z) < 0.001, "physical cab moves sideways while remaining in its plane")
	Input.action_release("flight_up")
	Input.action_release("flight_right")
	scene._reset()
	await frames(45)
	check(cab.position.distance_to(depot_rest) < 0.08 and cab.linear_velocity.length() < 0.3, "reset returns to a stationary safe landing")
	check(controls.command == Vector2.ZERO, "reset does not leave a held control")
	# City exits are open now; forced return is covered by airspace_tests.gd.
	cab.spawn_transform.origin = Vector3(cab.world_definition.city_right - 3.0, 7, 0)
	scene._reset()
	await frames(3)
	Input.action_press("flight_right")
	await frames(45 + engine_start_ticks)
	check(cab.position.x > cab.world_definition.city_right, "expanded right boundary permits a physical city exit")
	Input.action_release("flight_right")
	cab.spawn_transform.origin = Vector3(cab.world_definition.city_left + 3.0, 7, 0)
	scene._reset()
	await frames(3)
	Input.action_press("flight_left")
	await frames(45 + engine_start_ticks)
	check(cab.position.x < cab.world_definition.city_left, "expanded left boundary permits a physical city exit")
	Input.action_release("flight_left")
	print("FLIGHT_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
