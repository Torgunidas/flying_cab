extends SceneTree
## Visual review fixture, using the shipped meshes and gait in the native renderer.
func _initialize() -> void:
	call_deferred("_run")

func capture(file: String) -> void:
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/" + file + ".png")

func _run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Human presentation requires the graphics renderer")
		quit(1)
		return
	var gallery: Node3D = load("res://scenes/vehicle_showroom.tscn").instantiate()
	root.add_child(gallery)
	current_scene = gallery
	while not gallery.ready_for_view:
		await process_frame
	for cell in gallery.models.get_children():
		var car: FlightCab = cell.get_node("Vehicle")
		car.rotation = Vector3.ZERO
		car.reset_physics_interpolation()
		var person := load("res://scenes/people/ari_visual.tscn").instantiate() as PassengerVisual
		cell.add_child(person)
		person.position = Vector3(2.65, -0.38, 0)
		person.rotation.y = 0.65
		person.pose(0, Vector3.ZERO, false, 0)
		_line(cell, Vector3(0, -0.38, 0), 5.6)
	await capture("human-vehicle-scale")
	gallery.queue_free()
	await process_frame
	var stage := Node3D.new()
	root.add_child(stage)
	current_scene = stage
	var environment := WorldEnvironment.new()
	environment.environment = Environment.new()
	environment.environment.background_mode = Environment.BG_COLOR
	environment.environment.background_color = Color("14202c")
	environment.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.environment.ambient_light_color = Color("bccddd")
	environment.environment.ambient_light_energy = 0.8
	stage.add_child(environment)
	var key := DirectionalLight3D.new()
	key.rotation_degrees = Vector3(-35, -25, 0)
	key.light_energy = 1.3
	stage.add_child(key)
	var camera := Camera3D.new()
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.size = 7.2
	camera.position = Vector3(0, 0.75, 20)
	stage.add_child(camera)
	camera.make_current()
	_label(stage, "BODY SCALE / SAME GROUND", Vector3(0, 3.9, 0))
	var cab: Node3D = load("res://scenes/vehicles/visuals/basic_cab.tscn").instantiate()
	stage.add_child(cab)
	cab.position = Vector3(-3.25, 2.38, 0)
	_line(stage, Vector3(0, 2, 0), 8.8)
	for i in range(5):
		var person := load("res://scenes/people/ari_visual.tscn" if i == 4 else "res://scenes/people/passenger_%d.tscn" % i).instantiate() as PassengerVisual
		stage.add_child(person)
		person.position = Vector3(-1.15 + i * 1.1, 2, 0)
		person.rotation.y = 0.6
		person.pose(0, Vector3.ZERO, false, 0)
	for row in range(2):
		var y := 0.0 if row == 0 else -2.0
		_label(stage, "PASSENGERS / BRISK WALK" if row == 0 else "ARI / LIGHT JOG", Vector3(0, y + 1.48, 0))
		_line(stage, Vector3(0, y, 0), 8.8)
		for frame in range(8):
			var person := load("res://scenes/people/passenger_0.tscn" if row == 0 else "res://scenes/people/ari_visual.tscn").instantiate() as PassengerVisual
			stage.add_child(person)
			person.position = Vector3(-3.85 + frame * 1.1, y, 0)
			person._gait_weight = 1.0
			person.pose(0.001, Vector3.RIGHT * person.stride_length * (frame / 8.0 + 0.0001), false, 1)
	await capture("human-gait-poses")
	print("HUMAN_PRESENTATION: 12 vehicle comparisons, 5 human looks and 16 gait poses rendered")
	quit()

func _line(parent: Node3D, at: Vector3, width: float) -> void:
	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(width, 0.012, 0.02)
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	material.albedo_color = Color("65818d")
	box.material = material
	mesh.mesh = box
	mesh.position = at
	parent.add_child(mesh)

func _label(parent: Node3D, value: String, at: Vector3) -> void:
	var label := Label3D.new()
	label.text = value
	label.font_size = 28
	label.pixel_size = 0.004
	label.outline_size = 0
	label.position = at
	parent.add_child(label)
