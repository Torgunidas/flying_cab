extends SceneTree

func _initialize() -> void:
	call_deferred("_capture")

func _capture() -> void:
	var scene: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	var arguments := OS.get_cmdline_user_args()
	if arguments.size() > 1:
		var cab: FlightCab = scene.get_node("Cab")
		cab.spawn_transform.origin = Vector3(cab.world_definition.city_right + 5.0, 30, 0) if arguments[1] == "return" else Vector3(0, cab.world_definition.max_altitude - 3.0, 0)
		scene._reset()
		for i in range(5):
			await physics_frame
		cab.fuel = 65.0
		if arguments[1] == "ceiling":
			Input.action_press("flight_up")
	for i in range(50):
		await process_frame
	await RenderingServer.frame_post_draw
	print("WINDOW: size=", root.size, " position=", root.position, " usable=", DisplayServer.screen_get_usable_rect(root.current_screen), " screen_scale=", DisplayServer.screen_get_scale(root.current_screen))
	var output := "/private/tmp/flyingcab-flight01.png"
	if not OS.get_cmdline_user_args().is_empty():
		output = OS.get_cmdline_user_args()[0]
	var result := root.get_texture().get_image().save_png(output)
	print("CAPTURE: ", output, " result=", result)
	quit(result)
