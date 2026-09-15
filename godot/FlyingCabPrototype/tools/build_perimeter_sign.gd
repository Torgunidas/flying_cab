extends SceneTree
## Offline authoring only: one shared opaque mesh, three static text surfaces.
## Does not rebuild the city or run during gameplay.
var surface := SurfaceTool.new()

func _initialize() -> void:
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	var dark := Color("132631")
	var steel := Color("415761")
	var amber := Color("ffc16a")
	_box(Vector3.ZERO, Vector3(8.2, 3.2, 0.16), dark)
	for side in [-1, 1]:
		_box(Vector3(side * 4.08, 0, 0.04), Vector3(0.10, 3.2, 0.16), amber)
		_box(Vector3(0, side * 1.55, 0.10), Vector3(8.2, 0.10, 0.12), amber)
		_box(Vector3(side * 2.8, 2.13, 0), Vector3(0.055, 1.05, 0.055), steel)
		_box(Vector3(side * 4.0, 2.72, 0), Vector3(0.4, 0.16, 0.3), amber)
	_box(Vector3(0, 2.65, 0), Vector3(8.8, 0.18, 0.3), steel)
	# Fixed hazard blocks read as municipal equipment, without pulsing lights.
	for i in range(10):
		_box(Vector3(-3.65 + i * 0.81, -1.31, 0.14), Vector3(0.42, 0.10, 0.06), amber)
	var sign := Node3D.new()
	sign.name = "PerimeterSign"
	sign.add_to_group("perimeter_warning", true)
	var mesh := MeshInstance3D.new()
	mesh.name = "Housing"
	mesh.mesh = surface.commit()
	var material := StandardMaterial3D.new()
	material.vertex_color_use_as_albedo = true
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mesh.material_override = material
	mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	sign.add_child(mesh)
	mesh.owner = sign
	_label(sign, "Authority", "AIRSPACE CONTROL", 1.06, 26, Color("92a8b0"))
	_label(sign, "Perimeter", "CITY PERIMETER", 0.35, 52, amber)
	_label(sign, "Instruction", "TURN BACK", -0.60, 72, Color("fff0d8"))
	var packed := PackedScene.new()
	var result := packed.pack(sign)
	if result == OK:
		result = ResourceSaver.save(packed, "res://scenes/perimeter_sign.tscn")
	sign.free()
	print("PERIMETER_SIGN: ", result)
	quit(0 if result == OK else 1)

func _label(parent: Node3D, name: String, text: String, y: float, size: int, color: Color) -> void:
	var label := Label3D.new()
	label.name = name
	label.text = text
	label.position = Vector3(0, y, 0.18)
	label.font_size = size
	label.pixel_size = 0.011
	label.modulate = color
	label.outline_size = 0
	label.shaded = false
	label.double_sided = false
	label.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	parent.add_child(label)
	label.owner = parent

func _box(center: Vector3, size: Vector3, color: Color) -> void:
	var box := BoxMesh.new()
	box.size = size
	var arrays := box.get_mesh_arrays()
	for index: int in arrays[Mesh.ARRAY_INDEX]:
		surface.set_color(color)
		surface.set_normal(arrays[Mesh.ARRAY_NORMAL][index])
		surface.add_vertex(arrays[Mesh.ARRAY_VERTEX][index] + center)
