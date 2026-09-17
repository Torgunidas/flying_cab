extends SceneTree
## Explicit authoring only: writes saved meshes, definitions, actors and showroom.
## Never called by the game. Does not touch city or the original cab/shuttle.
const OUT := "res://scenes/vehicles/"
const DATA := "res://resources/vehicles/"
const BASE := preload("res://resources/vehicles/basic_cab.tres")
const CAB := preload("res://scenes/cab.tscn")
const BEACONS := preload("res://scripts/vehicle_beacons.gd")
const SHOWROOM := preload("res://scripts/vehicle_showroom.gd")
const SPECS := [
	["lorry", "Lorry", 1.5, 2.0, "aacbdd", 320, 5900, 2900, 280, 400, 0.40, 1],
	["supercar", "Supercar", 1.0, 1.0, "db363b", 80, 2250, 1560, 90, 85, 0.05, 1],
	["limousine", "Limousine", 2.0, 1.0, "f3f1e8", 190, 4000, 2280, 190, 180, 0.15, 5],
	["luxury_car", "Luxury car", 1.0, 1.0, "d2c6db", 120, 2820, 1740, 130, 150, 0.20, 3],
	["police_car", "Police car", 1.0, 1.0, "2359af", 125, 3100, 2000, 140, 180, 0.30, 3],
	["poor_car", "Poor car", 0.5, 1.0, "638241", 60, 1170, 630, 55, 55, 0.0, 1],
	["tow_car", "Tow car", 1.5, 1.5, "d67b36", 240, 4700, 2400, 220, 300, 0.35, 1],
	["normal_car_1", "Normal / Violet", 1.0, 1.0, "8a68ae", 100, 2350, 1400, 100, 100, 0.05, 3],
	["normal_car_2", "Normal / Graphite", 1.0, 1.0, "505864", 110, 2475, 1430, 115, 120, 0.10, 3],
	["normal_car_3", "Normal / Copper", 1.0, 1.0, "a96953", 95, 2280, 1378, 95, 95, 0.05, 3],
]

func _initialize() -> void:
	if not OS.get_cmdline_user_args().has("--write-models"):
		printerr("Explicit authoring required: --script tools/build_vehicles.gd -- --write-models")
		quit(1)
		return
	call_deferred("_build")

func material(hex: String, metallic := 0.0, roughness := 0.7, glow := false) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = Color(hex)
	m.metallic = metallic
	m.roughness = roughness
	m.emission_enabled = glow
	m.emission = m.albedo_color
	return m

func node(parent: Node, label: String, pos := Vector3.ZERO) -> Node3D:
	var n := Node3D.new()
	n.name = label
	n.position = pos
	parent.add_child(n)
	return n

func mesh(parent: Node, label: String, shape: Mesh, pos: Vector3, mat: Material, shadow := false) -> MeshInstance3D:
	var n := MeshInstance3D.new()
	n.name = label
	n.mesh = shape
	n.position = pos
	n.material_override = mat
	n.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON if shadow else GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	parent.add_child(n)
	return n

func box(parent: Node, label: String, size: Vector3, pos: Vector3, mat: Material, shadow := false) -> MeshInstance3D:
	var shape := BoxMesh.new()
	shape.size = size
	return mesh(parent, label, shape, pos, mat, shadow)

func hull(parent: Node, label: String, profile: Array[Vector2], width: float, mat: Material) -> void:
	# Convex side profile extruded across Z. Baked ArrayMesh, no runtime CSG.
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	var vertices: Array[Vector3] = []
	for z in [-width / 2, width / 2]:
		for p in profile:
			vertices.append(Vector3(p.x, p.y, z))
	var count := profile.size()
	for i in range(1, count - 1):
		for index in [0, i, i + 1, count, count + i + 1, count + i]:
			surface.add_vertex(vertices[index])
	for i in range(count):
		var j := (i + 1) % count
		for index in [i, count + i, j, j, count + i, count + j]:
			surface.add_vertex(vertices[index])
	surface.generate_normals()
	mesh(parent, label, surface.commit(), Vector3.ZERO, mat, true)

func own_children(parent: Node, owner_node: Node) -> void:
	for child in parent.get_children():
		child.owner = owner_node
		if child.scene_file_path.is_empty():
			own_children(child, owner_node)

func save_scene(scene: Node, path: String) -> void:
	own_children(scene, scene)
	var packed := PackedScene.new()
	assert(packed.pack(scene) == OK)
	assert(ResourceSaver.save(packed, path) == OK)

func make_visual(spec: Array) -> Node3D:
	var id: String = spec[0]
	var length: float = 2.2 * spec[2]
	var top: float = -0.38 + 1.14 * spec[3]
	var root_node := Node3D.new()
	root_node.name = "Visual"
	var paint := material(spec[4], 0.25, 0.5)
	var dark := material("202c39", 0.2)
	var glass := material("436e85", 0.35, 0.22)
	var silver := material("91a7b2", 0.6, 0.3)
	var gold := material("d7b362", 0.65, 0.3)
	var trim := gold if id == "luxury_car" or id == "limousine" else silver
	box(root_node, "Chassis", Vector3(length, 0.24, 0.88), Vector3(0, -0.15, 0), dark, true)
	if id == "lorry":
		box(root_node, "CargoBody", Vector3(2.08, top - 0.02, 1.02), Vector3(-0.50, (top + 0.02) / 2, 0), paint, true)
		box(root_node, "Cabin", Vector3(0.88, 1.46, 0.98), Vector3(1.11, 0.76, 0), paint, true)
		for side in [-1, 1]:
			box(root_node, "Window" + str(side), Vector3(0.59, 0.55, 0.015), Vector3(1.17, 1.04, side * 0.498), glass)
			for k in range(4):
				box(root_node, "Rib%d_%d" % [side, k], Vector3(0.045, 1.65, 0.025), Vector3(-1.38 + k * 0.52, 1.0, side * 0.52), silver)
		box(root_node, "Windshield", Vector3(0.02, 0.54, 0.85), Vector3(1.56, 1.04, 0), glass)
		box(root_node, "Grille", Vector3(0.025, 0.38, 0.64), Vector3(1.57, 0.42, 0), dark)
	elif id == "tow_car":
		box(root_node, "Deck", Vector3(2.08, 0.16, 1.08), Vector3(-0.61, 0.06, 0), silver, true)
		for side in [-1, 1]:
			box(root_node, "Rail" + str(side), Vector3(2.08, 0.13, 0.055), Vector3(-0.61, 0.19, side * 0.525), paint)
		box(root_node, "Cabin", Vector3(0.96, top - 0.04, 0.98), Vector3(1.10, (top + 0.04) / 2, 0), paint, true)
		box(root_node, "Windshield", Vector3(0.025, 0.42, 0.81), Vector3(1.59, top - 0.34, 0), glass)
		for side in [-1, 1]:
			box(root_node, "Window" + str(side), Vector3(0.64, 0.42, 0.015), Vector3(1.12, top - 0.34, side * 0.5), glass)
		# Cargo origin: chassis at -0.27 rests on deck y=0.14.
		# Its centre sits at rear edge x=-1.65: exactly half the 2.2 m cab overhangs.
		var socket := Marker3D.new()
		socket.name = "CargoMount"
		socket.position = Vector3(-length / 2, 0.41, 0)
		root_node.add_child(socket)
	elif id == "supercar":
		hull(root_node, "Body", [Vector2(-1.07, -0.03), Vector2(-1.07, 0.28), Vector2(-0.5, 0.34), Vector2(0.85, 0.2), Vector2(1.08, 0.0)], 0.92, paint)
		hull(root_node, "Canopy", [Vector2(-0.65, 0.28), Vector2(-0.4, 0.60), Vector2(0.14, 0.53), Vector2(0.62, 0.24)], 0.72, glass)
		box(root_node, "Spoiler", Vector3(0.20, 0.045, 1.04), Vector3(-0.85, top - 0.0225, 0), dark)
		for side in [-1, 1]:
			box(root_node, "WingSupport" + str(side), Vector3(0.055, top - 0.32, 0.045), Vector3(-0.85, (top + 0.23) / 2, side * 0.30), dark)
			box(root_node, "Vent" + str(side), Vector3(0.35, 0.08, 0.03), Vector3(-0.57, 0.11, side * 0.475), dark)
		box(root_node, "Spine", Vector3(0.75, 0.024, 0.09), Vector3(0.58, 0.238, 0), dark)
	else:
		var poor := id == "poor_car"
		var police := id == "police_car"
		var cabin_length := length * (0.72 if id == "limousine" else 0.55)
		var roof := top - (0.16 if police else (0.01 if id == "luxury_car" else 0.0))
		box(root_node, "Body", Vector3(length - 0.04, 0.34, 0.92), Vector3(0, 0.03, 0), paint, true)
		box(root_node, "Cabin", Vector3(cabin_length, roof - 0.25, 0.78), Vector3(-length * 0.08, (roof + 0.13) / 2, 0), dark, true)
		box(root_node, "Roof", Vector3(cabin_length + 0.04, 0.06, 0.84), Vector3(-length * 0.08, roof - 0.03, 0), paint)
		for side in [-1, 1]:
			box(root_node, "Window" + str(side), Vector3(cabin_length - 0.13, roof - 0.32, 0.016), Vector3(-length * 0.08, (roof + 0.19) / 2, side * 0.399), glass)
			box(root_node, "Pillar" + str(side), Vector3(0.045, roof - 0.20, 0.026), Vector3(-length * 0.15, (roof + 0.19) / 2, side * 0.411), paint)
			box(root_node, "Trim" + str(side), Vector3(length - 0.15, 0.04, 0.02), Vector3(0, 0.05, side * 0.472), trim)
		box(root_node, "Windshield", Vector3(0.02, roof - 0.32, 0.73), Vector3(-length * 0.08 + cabin_length / 2 + 0.012, (roof + 0.19) / 2, 0), glass)
		box(root_node, "Hood", Vector3(length * 0.28, 0.07, 0.82), Vector3(length * 0.35, 0.24, 0), paint)
		if id == "luxury_car":
			box(root_node, "GoldGrille", Vector3(0.02, 0.20, 0.59), Vector3(length / 2 - 0.02, 0.05, 0), gold)
			box(root_node, "GoldRoofInlay", Vector3(0.60, 0.008, 0.36), Vector3(-0.18, top - 0.004, 0), gold)
		if poor:
			var rust := material("855039")
			box(root_node, "RustDoor", Vector3(0.21, 0.15, 0.012), Vector3(-0.11, 0.13, 0.468), rust)
			box(root_node, "RustHood", Vector3(0.18, 0.012, 0.26), Vector3(0.37, 0.282, 0.1), rust)
		if police:
			var pale := material("aad1e2")
			var lime := material("b7ed59", 0.2, 0.4, true)
			for side in [-1, 1]:
				box(root_node, "DoorPanel" + str(side), Vector3(1.1, 0.19, 0.018), Vector3(-0.14, 0.10, side * 0.48), pale)
				box(root_node, "Reflector" + str(side), Vector3(1.85, 0.045, 0.018), Vector3(0, -0.025, side * 0.49), lime)
			box(root_node, "LightbarBase", Vector3(0.24, 0.06, 0.75), Vector3(-0.15, roof + 0.04, 0), dark)
			var beacon := node(root_node, "Beacons")
			beacon.set_script(BEACONS)
			for side in [-1, 1]:
				box(beacon, "Lamp" + str(side), Vector3(0.20, 0.08, 0.30), Vector3(-0.15, top - 0.04, side * 0.21), material("70baff", 0.0, 0.35, true))
	var lens := material("e4fff0", 0.0, 0.7, true)
	var tail := material("ef7657", 0.0, 0.7, true)
	for side in [-1, 1]:
		box(root_node, "Head" + str(side), Vector3(0.025, 0.10, 0.2), Vector3(length / 2 - 0.013, 0.12, side * 0.3), lens)
		box(root_node, "Tail" + str(side), Vector3(0.025, 0.08, 0.18), Vector3(-length / 2 + 0.013, 0.02, side * 0.3), tail)
	var thrusters := node(root_node, "Thrusters")
	var scale_x := minf(spec[2], 1.2)
	var flame_material := material("72e8ec", 0.0, 0.78, true)
	flame_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	var flame := CylinderMesh.new()
	flame.top_radius = 0.11 * scale_x
	flame.bottom_radius = 0.025
	flame.height = 0.65
	flame.radial_segments = 8
	flame.rings = 1
	for x_sign in [-1, 1]:
		for z_sign in [-1, 1]:
			var mount := node(thrusters, ("Rear" if x_sign < 0 else "Front") + ("Far" if z_sign < 0 else "Near"), Vector3(x_sign * length * 0.36, -0.24, z_sign * 0.43))
			box(mount, "Housing", Vector3(0.38 * scale_x, 0.28, 0.3), Vector3.ZERO, dark)
			box(mount, "Rim", Vector3(0.23 * scale_x, 0.14, 0.015), Vector3(0, -0.01, z_sign * 0.155), silver)
			var plume := node(mount, "Plume", Vector3(0, -0.13, 0))
			plume.visible = false
			mesh(plume, "Flame", flame, Vector3(0, -0.325, 0), flame_material)
	return root_node

func _build() -> void:
	var catalog: VehicleCatalog = load("res://resources/vehicle_catalog.tres").duplicate()
	catalog.models = [BASE, load(DATA + "heavy_shuttle.tres")]
	for spec in SPECS:
		var id: String = spec[0]
		var visual := make_visual(spec)
		save_scene(visual, OUT + "visuals/" + id + ".tscn")
		visual.free()
		var definition: VehicleDefinition = BASE.duplicate()
		definition.driver_door_x = 1.12 if id in ["lorry", "tow_car"] else (0.6 if id == "limousine" else 0.25)
		definition.model_id = id
		definition.display_name = spec[1]
		definition.visual_scene = load(OUT + "visuals/" + id + ".tscn")
		definition.size_units = Vector2(spec[2], spec[3])
		# New models use their body envelope; original cab collider stays untouched.
		definition.collision_size = Vector3(2.2 * spec[2], 1.14 * spec[3], 1.20)
		definition.collision_offset = Vector3(0, definition.collision_size.y / 2 - 0.38, 0)
		definition.navigation_clearance = Vector3(definition.collision_size.x + 0.3, 2 * maxf(absf(-0.38), definition.collision_offset.y + definition.collision_size.y / 2) + 0.15, 1.40)
		definition.headlight_origin = Vector3(1.1 * spec[2] + 0.02, 0.12, 0)
		definition.mass_kg = spec[5]
		definition.thrust_force = spec[6]
		definition.horizontal_thrust_force = spec[7]
		definition.fuel_capacity = spec[8]
		definition.starting_fuel = spec[8]
		definition.max_hull = spec[9]
		definition.damage_resistance = spec[10]
		definition.max_passengers = spec[11]
		definition.fuel_vertical_rate = 1.4 * sqrt(definition.mass_kg / 100)
		definition.fuel_horizontal_rate = 0.7 * sqrt(definition.mass_kg / 100)
		definition.max_horizontal_speed = 14.0 if id == "supercar" else (8.5 if id == "lorry" or id == "tow_car" else 10.5)
		assert(definition.validation_errors().is_empty())
		assert(ResourceSaver.save(definition, DATA + id + ".tres") == OK)
		catalog.models.append(load(DATA + id + ".tres"))
		# Common actor scene/script, authored visual override also visible in editor.
		var actor := CAB.instantiate() as FlightCab
		actor.scene_file_path = ""
		actor.definition = catalog.models.back()
		actor.entity_id = id + "_01"
		actor.initial_owner_id = &"unassigned"
		actor._apply_body()
		save_scene(actor, OUT + id + ".tscn")
		actor.free()
	assert(ResourceSaver.save(catalog, "res://resources/vehicle_catalog.tres") == OK)
	_build_showroom(catalog)
	print("VEHICLE_AUTHORING: 10 new variants + cab + legacy shuttle")
	quit()

func _build_showroom(catalog: VehicleCatalog) -> void:
	var scene := Node3D.new()
	scene.name = "VehicleShowroom"
	scene.set_script(SHOWROOM)
	var environment := WorldEnvironment.new()
	environment.name = "WorldEnvironment"
	environment.environment = Environment.new()
	environment.environment.background_mode = Environment.BG_COLOR
	environment.environment.background_color = Color("101b2b")
	environment.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.environment.ambient_light_color = Color("bacbdd")
	environment.environment.ambient_light_energy = 0.7
	scene.add_child(environment)
	var light := DirectionalLight3D.new()
	light.name = "KeyLight"
	light.rotation_degrees = Vector3(-28, -25, 0)
	light.light_energy = 1.4
	scene.add_child(light)
	var camera := Camera3D.new()
	camera.name = "Camera3D"
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.size = 17.5
	camera.position = Vector3(0, 0, 25)
	scene.add_child(camera)
	var gallery := node(scene, "Models")
	var plinth := material("26374c")
	for i in range(catalog.models.size()):
		var definition := catalog.models[i]
		var cell := node(gallery, String(definition.model_id), Vector3((i % 3 - 1) * 5.8, (1.5 - i / 3) * 3.5, 0))
		var car := CAB.instantiate() as FlightCab
		car.scene_file_path = ""
		car.name = "Vehicle"
		car.definition = definition
		car.entity_id = "showroom_" + String(definition.model_id)
		car.freeze = true
		car.rotation_degrees = Vector3(7, -15, 0)
		car._apply_body()
		cell.add_child(car)
		box(cell, "Plinth", Vector3(5.2, 0.045, 1.5), Vector3(0, -0.55, 0), plinth)
		var label := Label3D.new()
		label.name = "Label"
		label.position = Vector3(0, -0.9, 0.9)
		label.text = "%02d   %s\n%.1f x %.1f cab" % [i + 1, definition.display_name, definition.size_units.x, definition.size_units.y]
		label.font_size = 32
		label.pixel_size = 0.006
		label.outline_size = 0
		cell.add_child(label)
	save_scene(scene, "res://scenes/vehicle_showroom.tscn")
	scene.free()
