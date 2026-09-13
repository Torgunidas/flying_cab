extends SceneTree
## Capture actual physical flight with the gameplay camera; fixture gravity is off.
func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var scene: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	var cab: FlightCab = scene.get_node("Cab")
	cab.definition = cab.definition.duplicate()
	cab.definition.gravity = 0.0
	root.add_child(scene)
	current_scene = scene
	scene.set_physics_process(false)
	DirAccess.make_dir_recursive_absolute("res://build/screenshots")
	var shots := {
		"express-right": [Vector3(-40,160,0),Vector2.RIGHT],
		"express-left": [Vector3(40,160,0),Vector2.LEFT],
		"express-up": [Vector3(0.5,80,0),Vector2(0,1)],
		"ordinary-right": [Vector3(12,120,0),Vector2.RIGHT]
	}
	for shot: String in shots:
		cab.spawn_transform.origin = shots[shot][0]
		cab.reset_flight()
		for i in range(4):
			await physics_frame
		cab.command = shots[shot][1]
		for i in range(120):
			await physics_frame
		await RenderingServer.frame_post_draw
		var output := "res://build/screenshots/flight-%s.png" % shot
		var result := root.get_texture().get_image().save_png(output)
		print("CAPTURE: ",output," result=",result," boost=",cab.flight_fx.boost_amount," nozzle=",cab.flight_fx.nozzle_angle)
		if result != OK:
			quit(1)
			return
	quit()
