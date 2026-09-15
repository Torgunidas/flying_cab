extends SceneTree
## Requires a renderer; verifies the bubble's actual draw cadence against the cab.
var checks := 0
var failures := 0
var draws := 0
var context: RuntimeContext

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok:
		print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frame() -> void:
	context.player.resume(&"application_focus")
	await RenderingServer.frame_post_draw

func capture(name: String) -> void:
	await frame()
	root.get_texture().get_image().save_png("res://build/" + name + ".png")

func _run() -> void:
	if DisplayServer.get_name() == "headless":
		quit(1)
		return
	context = RuntimeContext.new()
	context.rides.enabled = true
	root.add_child(context)
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	level.living_world_enabled = false # Isolated fixture; full population has its own integration suite.
	level.context = context
	var cab: FlightCab = level.get_node("Cab")
	cab.definition = cab.definition.duplicate()
	cab.definition.gravity = 0
	cab.definition.highway_enabled = false
	cab.definition.airspace_enabled = false
	cab.definition.fuel_enabled = false
	cab.collision_layer = 0
	cab.collision_mask = 0
	cab.position = Vector3(-55, 100, 0)
	root.add_child(level)
	current_scene = level
	level.set_physics_process(false)
	await level.prepare_gameplay()
	var hud: TaxiHud = level.taxi_hud
	var bubble := hud.notice_bubble
	var camera: Camera3D = level.camera
	level.taxi.message("Do ROOM 09, proszę. Spokojnie, zdążymy.", 100)
	for i in range(5):
		await frame()
	bubble.draw.connect(func(): draws += 1)
	var samples := 0
	var peak_error := 0.0
	var interpolation_gap := 0.0
	# Acceleration, cruise, reversal, diagonal movement and braking.
	for command in [Vector2.RIGHT, Vector2.LEFT, Vector2(1, 1), Vector2.ZERO]:
		cab.command = command
		var elapsed := 0.0
		while elapsed < 1.5:
			await frame()
			elapsed += root.get_process_delta_time()
			var shown := cab.get_global_transform_interpolated().origin
			var expected := camera.unproject_position(shown) - Vector2(0, 55)
			peak_error = maxf(peak_error, bubble.anchor.distance_to(expected))
			interpolation_gap = maxf(interpolation_gap, camera.unproject_position(cab.global_position).distance_to(camera.unproject_position(shown)))
			samples += 1
	check(samples > 100 and draws >= samples - 1, "visible bubble actually redraws every rendered frame, independently of the 10 Hz HUD")
	check(interpolation_gap > 0.5 and peak_error < 0.03, "bubble stays attached to the rendered cab through acceleration, reversal and braking")
	print("BUBBLE: samples=%d draws=%d max_anchor_error=%.5f px raw_physics_gap=%.3f px" % [samples, draws, peak_error, interpolation_gap])
	await capture("bubble-moving")
	context.rides.offers.clear()
	var ride := context.rides.create_offer("depot", "velvet_club", 30, 1, 1000)
	ride.walk = 1
	context.rides.select(ride.id)
	context.rides.begin_boarding(cab.state, cab.definition)
	context.rides.board(cab.state, cab.definition)
	await frame()
	await frame()
	var expected := camera.unproject_position(cab.get_global_transform_interpolated().origin) - Vector2(0, 81)
	check(hud.guidance.visible and bubble.anchor.distance_to(expected) < 0.03, "guidance and bubble use the same frame and retain their vertical clearance")
	await capture("bubble-guidance")
	hud.open_overlay("map")
	await frame()
	check(not bubble.visible and not hud.guidance.visible, "map hides both moving annotations")
	hud.close_overlay()
	await frame()
	await frame()
	check(bubble.visible, "closing the map restores the live bubble")
	level.taxi.notice_remaining = 0
	await frame()
	await frame()
	check(not bubble.visible, "expired bubble disappears without waiting for a slow HUD redraw")
	context.rides.cancel(context.vehicles)
	level.taxi.message("Kontrola powrotu.", 100)
	cab.command = Vector2.ZERO
	cab.reset_flight()
	for i in range(12):
		await frame()
	expected = camera.unproject_position(cab.get_global_transform_interpolated().origin) - Vector2(0, 55)
	check(not cab.resetting and bubble.anchor.distance_to(expected) < 0.03, "reset does not leave the bubble at its previous location")
	level.taxi.notice_remaining = 0
	cab.freeze = true
	for entry in [["west", Vector3(-77.5, 61, 0)], ["east", Vector3(78.5, 157, 0)], ["smog", Vector3(-77.5, -35, 0)]]:
		cab.global_position = entry[1]
		cab.linear_velocity = Vector3.ZERO
		cab.reset_physics_interpolation()
		level._snap_camera()
		for i in range(20):
			await frame()
		await capture("perimeter-" + entry[0])
	check(get_nodes_in_group("perimeter_warning").size() == 30, "compiled city renders the shared warning signs on both edges")
	print("PERIMETER_PRESENTATION_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)
