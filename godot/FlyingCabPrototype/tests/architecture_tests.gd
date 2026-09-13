extends SceneTree
var failures := 0
var checks := 0

class RoomFixture extends Node3D:
	var context: RuntimeContext
	var definition := MapDefinition.new()
	var visited := 0
	var arrival: StringName
	func _init() -> void:
		definition.map_id = &"test_room"
		definition.entry_points = {"default": Vector3.ZERO, "door": Vector3(2, 0, 0)}
	func capture_map_state() -> Dictionary:
		return {"visited": visited}
	func _ready() -> void:
		context.world.bind(self, definition)
	func restore_map_state(data: Dictionary) -> void:
		visited = data.get("visited", 0)
	func arrive_at(entry: StringName) -> void:
		arrival = entry
		visited += 1

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if not ok:
		failures += 1
		push_error("FAIL: " + label)
	else:
		print("PASS: ", label)

func frames(count := 4) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func _run() -> void:
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	root.add_child(level)
	level.set_physics_process(false)
	var context: RuntimeContext = level.context
	var first: FlightCab = level.cab
	var second: FlightCab = load("res://scenes/cab.tscn").instantiate()
	second.entity_id = &"npc_cab_01"
	second.name = "UnrelatedVehicleName"
	second.position = Vector3(22, -25, 0)
	second.freeze = true
	level.add_child(second)
	await frames()
	second.state.owner_id = &"resident_01"
	second.state.passenger_ids = PackedStringArray(["passenger_01"])
	second.fuel = 37.0
	second.state.condition = 0.65
	var npc_revision := second.assign_driver(&"resident_01")
	check(second.receive_command(Vector2.RIGHT, &"resident_01", npc_revision), "NPC uses the same vehicle command contract")
	var identity := second.get_instance_id()
	check(context.player.take_control(second, &"flight"), "Ari can take control of the actual NPC vehicle")
	check(second.get_instance_id() == identity and second.state.owner_id == &"resident_01" and second.state.driver_id == &"ari" and second.fuel == 37.0 and second.state.passenger_ids.size() == 1, "takeover retains identity, owner, fuel and occupants")
	check(not second.receive_command(Vector2.LEFT, &"resident_01", npc_revision) and second.command == Vector2.ZERO, "stale NPC commands are rejected after takeover")
	check(context.vehicles.has(&"npc_cab_01") and context.world.vehicles.has(second), "late spawned vehicles join the world and persistent session")
	check(level.get_node("Atmosphere").focus == second and level._camera_target.is_equal_approx(second.global_position), "camera and atmosphere follow the active vehicle despite its node name")
	context.player.dispatch(Vector2.RIGHT)
	check(second.command == Vector2.RIGHT and first.command == Vector2.ZERO, "only the newly controlled vehicle receives player input")
	context.player.suspend(&"dialogue")
	context.player.suspend(&"map_transition")
	context.player.resume(&"dialogue")
	context.player.dispatch(Vector2.RIGHT)
	check(second.command == Vector2.ZERO and context.player.is_suspended(), "independent control locks cannot release each other")
	context.player.resume(&"map_transition")
	check(not context.player.is_suspended(), "control resumes when the last lock is removed")
	var foot := WalkingActor.new()
	foot.position = Vector3(0, 20, 0)
	level.add_child(foot)
	foot.set_physics_process(false)
	check(context.player.take_control(foot, &"on_foot"), "on-foot actor plugs into the same session and camera contract")
	check(second.state.driver_id == &"" and second.state.owner_id == &"resident_01" and level.get_node("Atmosphere").focus == foot, "leaving a vehicle clears its driver without changing its owner")
	var support := StaticBody3D.new()
	var floor_shape := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(24, 2, 4)
	floor_shape.shape = box
	support.add_child(floor_shape)
	support.position = Vector3(130, -1, 0)
	level.add_child(support)
	var collider := CollisionShape3D.new()
	var capsule := CapsuleShape3D.new()
	capsule.radius = 0.3
	capsule.height = 1.6
	collider.shape = capsule
	foot.add_child(collider)
	foot.global_position = Vector3(130, 0.85, 0)
	foot.set_physics_process(true)
	context.player.dispatch(Vector2.RIGHT)
	await frames(120)
	check(foot.is_on_floor() and foot.position.x > 135 and foot.position.x < 139 and foot.position.z == 0, "on-foot adapter walks on a real collider while retaining the 2.5D plane")
	context.player.take_control(second, &"flight")
	await frames(30)
	check(is_zero_approx(foot.velocity.x), "entering a car releases the walking actor's input and brings it to rest")
	second.assign_driver(&"someone_else")
	check(context.player.focus == null and second.state.driver_id == &"someone_else", "losing control detaches player focus without stealing the new driver's claim")
	context.player.take_control(first, &"flight")
	var lane: Node3D = load("res://scripts/highway.gd").new()
	lane.name = "AddedAfterReady"
	lane.add_to_group("highway")
	lane.set_meta("entity_id", "test/lane/new")
	lane.position = Vector3(30, 80, 0)
	level.add_child(lane)
	await frames()
	check(context.world.lanes.has(lane) and level.controls._city_lanes.has(lane), "runtime lane registration updates vehicle queries and minimap")
	first.fuel = 100
	first._update_highway(Vector3(30, 80, 0), 0.5)
	check(first.highway_speed == 1.5, "a lane spawned after ready actually affects vehicle assist")
	level.remove_child(lane)
	check(not context.world.lanes.has(lane) and not level.controls._city_lanes.has(lane), "removing a sector entity removes stale minimap and gameplay references")
	lane.free()
	var campaign := context.campaign
	campaign.active = true
	campaign.remaining_seconds = 30
	campaign.medicine_doses = 2
	campaign.credits = 100
	check(campaign.deliver_medicine(&"dose_01", 60) and campaign.remaining_seconds == 90 and campaign.medicine_doses == 1, "delivering medicine extends campaign time and consumes one dose")
	check(not campaign.deliver_medicine(&"dose_01", 60) and campaign.remaining_seconds == 90, "the same delivery cannot be credited twice")
	var service := VehicleService.new()
	check(service.repair(second.state, second.definition, campaign, 20, 1) > 0 and is_equal_approx(second.state.condition, 0.85) and is_equal_approx(campaign.credits, 80), "repair atomically restores condition and charges only restored units")
	var before := second.state.condition
	check(not service.repair(second.state, second.definition, campaign, 0, 10000) and second.state.condition == before and is_equal_approx(campaign.credits, 80), "empty repair request changes neither condition nor money")
	check(not second.apply_damage(-1) and second.state.condition == before, "future damage capability rejects negative damage")
	var dialogue := DialogueSession.new()
	var graph := {"hello": {"speaker": "Mechanik", "text": "Sprawdzić auto?", "choices": [{"id": "yes", "text": "Tak", "next": "done"}]}, "done": {"speaker": "Mechanik", "text": "Gotowe.", "choices": [{"id": "leave", "text": "Dzięki", "next": ""}]}}
	check(dialogue.begin(&"mechanic", graph, &"hello", context.player) and context.player.is_suspended(), "dialogue graph owns a control lock independently of its future UI")
	check(not dialogue.choose(&"missing") and dialogue.is_active(), "invalid dialogue choice cannot advance the conversation")
	check(dialogue.choose(&"yes") and dialogue.choose(&"leave") and not context.player.is_suspended(), "dialogue transitions release only their own lock on completion")
	context.player.release_control()
	second.capture_state()
	var saved := context.snapshot()
	var restored := RuntimeContext.new()
	check(restored.restore(saved) and restored.vehicles[&"npc_cab_01"].fuel == 37 and restored.campaign.remaining_seconds == 90, "versioned session snapshot restores vehicles and campaign independently of scene nodes")
	check(not restored.campaign.deliver_medicine(&"dose_01", 60), "delivery idempotency survives a saved-session round trip")
	var malformed := saved.duplicate(true)
	malformed.vehicles[0].fuel = -5
	check(not restored.restore(malformed) and restored.campaign.remaining_seconds == 90, "invalid save fails before mutating live session state")
	var path := "res://build/session-round-trip.json"
	check(context.save_to(path) == OK and restored.load_from(path), "JSON save and load preserve the versioned state through disk")
	check(is_equal_approx(RenderPolicy.scale_for(Vector2i(1440, 2560), 921600), 0.5) and RenderPolicy.scale_for(Vector2i(540, 960), 921600) == 1.0, "3D pixel budget scales high-DPI rendering without upscaling small canvases")
	var clock_context := RuntimeContext.new()
	clock_context.campaign.active = true
	clock_context.campaign.remaining_seconds = 10
	clock_context._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
	clock_context._process(1000)
	check(clock_context.campaign.remaining_seconds == 10, "unfocused application cannot advance active campaign time")
	clock_context._notification(NOTIFICATION_APPLICATION_FOCUS_IN)
	clock_context._process(0.25)
	check(clock_context.campaign.remaining_seconds == 9.75, "focus return advances only the new active interval without offline catch-up")
	var expiry_count := [0]
	clock_context.campaign.expired.connect(func(): expiry_count[0] += 1)
	clock_context.campaign.advance(20)
	clock_context.campaign.advance(20)
	clock_context.campaign.medicine_doses = 1
	check(expiry_count[0] == 1 and not clock_context.campaign.deliver_medicine(&"too_late", 60), "campaign expiration fires once and cannot be undone by a late dose")
	clock_context.free()
	# Use real PackedScenes and a persistent context across city/interior changes.
	var host := Node.new()
	root.add_child(host)
	var persistent := RuntimeContext.new()
	host.add_child(persistent)
	persistent.campaign.credits = 42
	var router := MapRouter.new()
	router.context = persistent
	router.host = host
	router.register_map(&"city_02", load("res://scenes/flight_lab.tscn"))
	var fixture := RoomFixture.new()
	var room := PackedScene.new()
	room.pack(fixture)
	fixture.free()
	router.register_map(&"test_room", room)
	check(await router.enter(&"city_02"), "map router enters a catalogued city scene")
	var old_vehicle: FlightCab = router.current.cab
	old_vehicle.freeze = true
	old_vehicle.fuel = 23
	old_vehicle.global_position = Vector3(12, 70, 0)
	check(await router.enter(&"test_room", Callable(), &"door"), "map router supports a non-vehicle interior and a named entry")
	check(router.current.arrival == &"door" and persistent.world.definition.map_id == &"test_room" and persistent.campaign.credits == 42 and persistent.vehicles[&"ari_cab"].fuel == 23, "interior arrival retains campaign and parked vehicle state")
	check(not await router.enter(&"unknown") and not await router.enter(&"test_room", Callable(), &"missing"), "invalid map/entry leaves the active map intact")
	var invalid_root := Node.new()
	var invalid_scene := PackedScene.new()
	invalid_scene.pack(invalid_root)
	invalid_root.free()
	router.register_map(&"invalid_root", invalid_scene)
	check(not await router.enter(&"invalid_root") and persistent.current_map == &"test_room" and not persistent.player.is_suspended(), "invalid scene protocol is rejected before unloading the current map")
	var room_snapshot := persistent.snapshot()
	check(room_snapshot.maps.test_room.visited == 1, "saving inside an active room captures its latest local state without requiring an exit")
	check(await router.enter(&"city_02") and router.current.cab.fuel == 23 and router.current.cab.global_position.is_equal_approx(Vector3(12, 70, 0)), "return to city restores the same persistent vehicle pose and fuel")
	check(await router.enter(&"test_room") and router.current.visited == 2, "map-local state is restored when re-entering an interior")
	persistent.player.release_control()
	persistent.world.unbind()
	host.queue_free()
	context.player.release_control()
	level.queue_free()
	restored.free()
	await process_frame
	print("ARCHITECTURE_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)
