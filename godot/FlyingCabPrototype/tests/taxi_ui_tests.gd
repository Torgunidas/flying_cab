extends SceneTree
## Modal pause and input handoff must be tested against real physics and rides.
var checks := 0
var failures := 0
var context: RuntimeContext
var level: Node3D
var hud: TaxiHud
var cab: FlightCab
var saved := 0

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
		await process_frame
		context.player.resume(&"application_focus")

func capture(label: String) -> void:
	if DisplayServer.get_name() != "headless":
		await frames(4)
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/minimal-" + label + ".png")

func key(code: Key) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.pressed = true
	root.push_input(event, true)
	event = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = false
	root.push_input(event, true)

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	context.campaign.credits = 120
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.living_world_enabled = false # Isolated fixture; full population has its own integration suite.
	level.context = context
	root.add_child(level)
	await level.prepare_gameplay()
	hud = level.taxi_hud
	cab = level.cab
	hud.save_requested.connect(func(): saved += 1)
	await frames(20)
	check(hud.overlay.is_empty() and not hud.reference_map.visible and not paused, "normal play starts with the city reference hidden")
	check(level.controls._reset_rect == Rect2(), "the game has no main-screen tow/reset button competing with options")
	# The old save-button position has no active action in flight.
	hud.press(Vector2(238, 355), 0)
	check(saved == 0 and hud.overlay.is_empty(), "a tap at the removed save action cannot save or open a panel")
	await capture("flight")
	Input.action_press("flight_up")
	await frames(50)
	Input.action_release("flight_up")
	await frames(1)
	hud._fuel_pointers[7] = true
	hud.press(hud._map_button.get_center(), 9)
	var position := cab.global_position
	var fuel := cab.fuel
	var offer: Dictionary = context.rides.offers[0]
	var lifetime: float = offer.remaining
	await frames(40)
	check(hud.overlay == "map" and hud.reference_map.visible and paused, "map icon opens a paused city reference")
	check(cab.global_position.is_equal_approx(position) and cab.fuel == fuel and offer.remaining == lifetime, "map pauses the real airborne body, fuel consumption and waiting passengers")
	check(level.controls.command == Vector2.ZERO and not level.controls.visible and not level.taxi.fuel_requested and hud._fuel_pointers.is_empty(), "opening a modal clears held flight/service inputs and hides flight controls")
	await capture("map")
	var depot: TaxiStop = level.taxi.network.stops.depot
	hud.reference_map.inspect(hud.reference_map._point(depot.global_position))
	check(hud.reference_map.selected_stop == "depot", "reference map lets the player inspect a stop without accepting a fare")
	# Closing must preserve another subsystem's suspension reason.
	context.player.suspend(&"dialogue_test")
	hud.press(hud._close.get_center(), 9)
	check(not paused and context.player.is_suspended() and not hud.reference_map.visible, "closing the map releases only its own pause and control lock")
	context.player.resume(&"dialogue_test")
	Input.action_press("flight_up")
	hud.open_overlay("options")
	await frames(5)
	await capture("options")
	hud.press(hud._save.get_center(), 0)
	check(saved == 1 and hud.overlay.is_empty() and not paused, "save is available inside options and returns to play")
	await frames(3)
	check(level.controls.command == Vector2.ZERO, "a keyboard thrust held through the menu cannot resume automatically")
	Input.action_release("flight_up")
	await frames(3)
	Input.action_press("flight_up")
	await frames(3)
	check(level.controls.command.y == 1, "releasing and pressing again restores normal thrust")
	Input.action_release("flight_up")
	await frames(3)
	key(KEY_M)
	await frames(2)
	check(hud.overlay == "map", "M opens the same reference map")
	# Flight touch coordinates while the map is open must not leak to the cab.
	var touch := InputEventScreenTouch.new()
	touch.index = 4
	touch.position = level.controls._controls[2].get_center()
	touch.pressed = true
	root.push_input(touch, true)
	key(KEY_ESCAPE)
	await frames(2)
	check(hud.overlay.is_empty() and level.controls._pointers.is_empty() and level.controls.command == Vector2.ZERO, "touching the map over a flight button cannot latch thrust after closing")
	key(KEY_ESCAPE)
	await frames(2)
	check(hud.overlay == "options", "Escape opens options during flight")
	key(KEY_ESCAPE)
	await frames(2)
	check(hud.overlay.is_empty() and not context.player.is_suspended(), "Escape closes options and returns input to the player")
	# Save schema remains compatible with offers selected by the previous UI.
	context.rides.selected = context.rides.offers[0].id
	var copy := RuntimeContext.new()
	check(copy.restore(context.snapshot()), "an earlier selected offer still loads under the automatic pickup UI")
	copy.free()
	# Show a full service interaction only when the parked vehicle needs it.
	cab.command = Vector2.ZERO
	cab.reset_flight()
	await frames(200)
	cab.fuel = 75
	cab.state.condition = 0.75
	await frames(15)
	await capture("service")
	var wallet := context.campaign.credits
	var touch_repair := InputEventScreenTouch.new()
	touch_repair.index = 2
	touch_repair.pressed = true
	touch_repair.position = level.controls._repair_rect.get_center()
	root.push_input(touch_repair, true)
	await frames(2)
	hud.press(hud._fuel.get_center(), 3)
	await frames(90)
	check(cab.fuel > 75 and cab.state.condition > 0.75 and context.campaign.credits < wallet, "compact service touch buttons buy real fuel and repair with the shared wallet")
	touch_repair = InputEventScreenTouch.new()
	touch_repair.index = 2
	touch_repair.pressed = false
	root.push_input(touch_repair, true)
	hud._fuel_pointers.clear()
	await frames(2000)
	check(not context.rides.active.is_empty() and context.rides.active.phase == "riding", "waiting at the depot automatically boards its passenger without selecting a card")
	check(level.taxi.notice_remaining == 0, "temporary messages disappear while the journey remains active")
	await capture("quiet-flight")
	hud.open_overlay("map")
	await capture("destination-map")
	var trip := context.rides.active
	var progress: float = trip.progress
	await frames(30)
	check(trip.progress == progress, "opening the reference during a fare preserves its progress")
	hud.close_overlay()
	hud.open_overlay("options")
	level.queue_free()
	await frames(2)
	check(not paused and not context.player.is_suspended(), "unloading a map with options open cannot leave the next map paused")
	context.queue_free()
	await process_frame
	print("TAXI_UI_TESTS: ", checks - failures, "/", checks)
	quit(1 if failures else 0)
