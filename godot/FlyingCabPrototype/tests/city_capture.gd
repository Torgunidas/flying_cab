extends SceneTree
## Render the actual level, using fixed survey positions only in this tool.
func _initialize() -> void:
	call_deferred("_capture")

func _capture() -> void:
	var scene: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	var cab: FlightCab = scene.get_node("Cab")
	var camera: Camera3D = scene.get_node("Camera3D")
	var shots := {
		"start": Vector3(-22, 54.4, 0),
		"low-ring": Vector3(-12, 16, 0),
		"smog-edge": Vector3(-13, -5, 0),
		"lowlife": Vector3(22, -39, 0),
		"smog-canyon": Vector3(0, -25, 0),
		"velvet": Vector3(-22, 32, 0),
		"foundry": Vector3(22, 36, 0),
		"eden": Vector3(-22, 190, 0),
		"aurelia": Vector3(22, 190, 0),
		"overview": Vector3(0.5, 158, 0)
	}
	DirAccess.make_dir_recursive_absolute("res://build/screenshots")
	for shot: String in shots:
		cab.freeze = true
		cab.global_position = shots[shot]
		cab.linear_velocity = Vector3.ZERO
		cab.reset_physics_interpolation()
		scene._snap_camera()
		if shot == "overview":
			scene.set_process(false)
			scene.get_node("HUD").hide()
			camera.projection = Camera3D.PROJECTION_ORTHOGONAL
			camera.keep_aspect = Camera3D.KEEP_WIDTH
			camera.size = 222.0
			camera.position = Vector3(0.5, 140, 220)
			camera.rotation = Vector3.ZERO
		for i in range(100):
			await process_frame
		await RenderingServer.frame_post_draw
		var output := "res://build/screenshots/city-%s.png" % shot
		var result := root.get_texture().get_image().save_png(output)
		print("CAPTURE: ", output, " result=", result, " draw_calls=", Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
		if result != OK:
			quit(1)
			return
	quit()
