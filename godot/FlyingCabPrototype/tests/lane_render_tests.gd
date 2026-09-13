extends SceneTree
## Compare actual pixels over time with the authored materials in an isolated view.
## Isolation removes fog drift and camera motion from the test.

func _initialize() -> void:
	call_deferred("_run")

func snapshot() -> PackedByteArray:
	for i in range(90):
		await process_frame
	await RenderingServer.frame_post_draw
	return root.get_texture().get_image().get_data()

func _run() -> void:
	var city: Node3D = load("res://scenes/city.tscn").instantiate()
	var stage := Node3D.new()
	root.add_child(stage)
	var camera := Camera3D.new()
	stage.add_child(camera)
	camera.position = Vector3(0,0,10)
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.size = 40.0
	var lane := MeshInstance3D.new()
	stage.add_child(lane)
	var passed := 0
	var last: PackedByteArray
	for route in city.get_node("Highways").get_children():
		if not route.is_in_group("highway"):
			continue
		lane.mesh = route.get_node("HolographicLane").mesh
		lane.material_override = route.get_node("HolographicLane").material_override
		var first := await snapshot()
		last = await snapshot()
		var stable := first == last
		passed += int(stable)
		print("PASS: " if stable else "FAIL: ",route.name," markings stay stationary")
	# Positive control: hiding the route must change the actual rendered pixels.
	lane.hide()
	var blank := await snapshot()
	var visible_ok := blank != last
	passed += int(visible_ok)
	print("PASS: captured markings are visible" if visible_ok else "FAIL: captures did not contain the lane")
	city.free()
	print("LANE_RENDER_TESTS: %d/7 passed" % passed)
	quit(0 if passed==7 else 1)
