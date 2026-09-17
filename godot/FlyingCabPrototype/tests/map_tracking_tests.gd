extends SceneTree
var checks := 0
var failures := 0
var context: RuntimeContext
var level: Node3D
var hud: TaxiHud
var map: CityReferenceMap

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok: print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count := 3) -> void:
	for i in count:
		await process_frame
		context.player.resume(&"application_focus")

func pointer(point: Vector2, pressed: bool, id := -1) -> void:
	if id == -1:
		var event := InputEventMouseButton.new()
		event.button_index = MOUSE_BUTTON_LEFT
		event.position = point
		event.pressed = pressed
		root.push_input(event, true)
	else:
		var event := InputEventScreenTouch.new()
		event.index = id
		event.position = point
		event.pressed = pressed
		root.push_input(event, true)

func move(point: Vector2, id := -1) -> void:
	if id == -1:
		var event := InputEventMouseMotion.new()
		event.position = point
		root.push_input(event, true)
	else:
		var event := InputEventScreenDrag.new()
		event.index = id
		event.position = point
		root.push_input(event, true)

func hold(point: Vector2, id := -1) -> void:
	pointer(point, true, id)
	map._process(map.HOLD_SECONDS)
	pointer(point, false, id)

func capture(label: String) -> void:
	if DisplayServer.get_name() != "headless":
		await frames(4)
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/tracking-" + label + ".png")

func _run() -> void:
	context = RuntimeContext.new()
	context.rides.enabled = true
	root.add_child(context)
	level = load("res://scenes/flight_lab.tscn").instantiate()
	level.living_world_enabled = false
	level.context = context
	root.add_child(level)
	await level.prepare_gameplay()
	level.set_physics_process(false)
	level.cab.freeze = true
	await frames()
	hud = level.taxi_hud
	map = hud.reference_map
	hud.open_overlay("map")
	map.set_process(false) # Advance gesture time deterministically except the paused-clock check.
	var stop: TaxiStop = level.taxi.network.stops.depot
	var point := map._point(stop.global_position)
	var hub: Dictionary = map.quest_hubs.filter(func(item): return item.id == "maya")[0]
	var badge := map.hub_point(hub)
	for id in [-1, 3]:
		pointer(point, true, id)
		map._process(0.2)
		pointer(point, false, id)
		map._process(1.0)
		check(map.selected_stop == "depot" and map.tracked_point.is_empty(), "short press inspects only, pointer " + str(id))
		pointer(point, true, id)
		map._process(0.2)
		move(point + Vector2(40, 0), id)
		move(point, id)
		map._process(1.0)
		pointer(point, false, id)
		check(map.tracked_point.is_empty(), "drag away cancels without restarting on return, pointer " + str(id))
		pointer(point, true, id)
		map._process(map.HOLD_SECONDS * 0.5)
		check(map.tracked_point.is_empty() and map._hold_elapsed > 0, "ring advances before committing, pointer " + str(id))
		await capture("hold-" + str(id))
		map._process(map.HOLD_SECONDS)
		map._process(3.0)
		check(map.tracked_point.get("id") == "depot", "one continuous hold toggles exactly once, pointer " + str(id))
		pointer(point, false, id)
		hold(point, id)
		check(map.tracked_point.is_empty(), "same hold turns tracking off, pointer " + str(id))
	pointer(badge, true, 5)
	map._process(map.HOLD_SECONDS * 0.5)
	await capture("hub-hold")
	pointer(point, true, 6)
	pointer(point, false, 6)
	move(point + Vector2(50, 0), 6)
	map._process(map.HOLD_SECONDS)
	pointer(badge, false, 5)
	check(map.tracked_point.get("id") == "maya" and map.selected_npc == "maya", "second finger cannot replace or release the original hub hold")
	hold(point)
	check(map.tracked_point.get("id") == "depot", "holding another marker replaces the one tracked point")
	hold(Vector2(10, 110))
	check(map.tracked_point.get("id") == "depot", "empty map space does not toggle tracking")
	pointer(point, true, 4)
	map._process(0.2)
	var canceled := InputEventScreenTouch.new()
	canceled.index = 4
	canceled.position = point
	canceled.canceled = true
	root.push_input(canceled, true)
	map._process(1.0)
	check(map.tracked_point.get("id") == "depot" and map._hold_target.is_empty(), "system-canceled touch cannot complete tracking")
	for reason in ["close", "focus", "resize"]:
		pointer(point, true)
		map._process(0.3)
		if reason == "close":
			hud.close_overlay()
			hud.open_overlay("map")
		elif reason == "focus":
			map._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
		else:
			map._layout()
		map._process(1.0)
		pointer(point, false)
		check(map.tracked_point.get("id") == "depot" and map._hold_target.is_empty(), reason + " cancels an incomplete hold and preserves tracking")
	# Real paused process clock, not only direct calls into the timer.
	map.set_process(true)
	pointer(point, true, 2)
	await create_timer(0.8, true).timeout
	pointer(point, false, 2)
	map.set_process(false)
	check(paused and map.tracked_point.is_empty(), "hold timer completes while map pauses the world")
	hold(badge)
	var effect := NarrativeEffect.new()
	effect.kind = "start_quest"
	effect.key = &"first_dose"
	context.narrative.execute([effect])
	await frames()
	check(not map.quest_hubs.any(func(item): return item.id == "maya") and map._display_hubs().any(func(item): return item.id == "maya"), "unavailable tracked hub retains a removable location marker")
	hold(badge)
	check(map.tracked_point.is_empty() and not map._display_hubs().any(func(item): return item.id == "maya"), "same gesture removes an unavailable hub")
	# A far-away point has guidance immediately, independently of district or fares.
	stop = level.taxi.network.stops.aurelia_01
	point = map._point(stop.global_position)
	hold(point)
	await capture("selected")
	for dimensions in [Vector2i(360, 640), Vector2i(960, 540)]:
		root.content_scale_size = dimensions
		root.size = dimensions
		await frames(5)
		check(map._map.end.y + 122 < dimensions.y, "tracking hint fits viewport " + str(dimensions))
		await capture("map-%dx%d" % [dimensions.x, dimensions.y])
	root.content_scale_size = Vector2i(540, 960)
	root.size = Vector2i(540, 960)
	await frames(5)
	hud.close_overlay()
	await frames()
	check(hud.tracking_guidance.visible and not hud.guidance.visible and hud.tracking_guidance.direction.y < 0, "green bearing guides to another district without a taxi trip")
	context.rides.cancel(context.vehicles)
	context.rides.offers.clear()
	var offer := context.rides.create_offer("depot", "velvet_club", 30, 1, 150)
	offer.walk = 1.0
	context.rides.select(offer.id)
	assert(context.rides.begin_boarding(level.cab.state, level.cab.definition))
	assert(context.rides.board(level.cab.state, level.cab.definition))
	level.cab.global_position = level.taxi.network.stops.velvet_club.global_position + Vector3(12, 6, 0)
	level.cab.reset_physics_interpolation()
	level._snap_camera()
	await frames()
	check(hud.tracking_guidance.visible and hud.guidance.visible and hud.tracking_guidance.anchor == hud.guidance.anchor, "both independently directed arrows can share the same anchor")
	check(hud.tracking_guidance.direction.dot(hud.guidance.direction) < 0, "two different destinations produce independently opposed bearings")
	check(hud.tracking_guidance.color == map.TRACK_COLOR and hud.guidance.color == Color("ff784a"), "tracking is green and taxi guidance retains its original color")
	await capture("two-arrows")
	for overlay in ["map", "options"]:
		hud.open_overlay(overlay)
		await frames()
		check(not hud.tracking_guidance.visible and not hud.guidance.visible, overlay + " hides both arrows")
		hud.close_overlay()
	context.player.suspend(&"dialogue")
	await frames()
	check(not hud.tracking_guidance.visible, "dialogue suspension hides tracking")
	context.player.resume(&"dialogue")
	var actor: WalkingActor = level.on_foot.actor
	actor.global_position = level.cab.global_position
	context.player.take_control(actor, &"on_foot")
	level._snap_camera()
	await frames()
	check(hud.tracking_guidance.visible and not hud.guidance.visible, "point tracking follows Ari on foot independently of the taxi")
	context.player.release_control()
	await frames()
	check(not hud.tracking_guidance.visible, "tracking hides when there is no player focus")
	level.queue_free()
	await frames()
	context.queue_free()
	await process_frame
	print("MAP_TRACKING_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)
