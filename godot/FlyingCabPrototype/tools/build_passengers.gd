extends SceneTree
## Explicit authoring only. All five people share one anatomy and skin layout.
## One vertex-coloured skinned mesh per person; no runtime mesh generation.
const Anatomy = preload("res://scripts/actors/human_rig.gd")
var material := StandardMaterial3D.new()

func _initialize() -> void:
	material.vertex_color_use_as_albedo = true
	material.roughness = 0.9
	for look in range(5):
		_build(look)
	print("Baked four passengers and Ari: articulated knees, ankles and elbows.")
	quit()

func _build(look: int) -> void:
	var node := Node3D.new()
	node.name = "AriVisual" if look == 4 else "Passenger"
	node.set_script(load("res://scripts/actors/ari_visual.gd" if look == 4 else "res://scripts/taxi/passenger_visual.gd"))
	if look == 4:
		node.stride_length = 1.25
		node.stance_fraction = 0.40
		node.step_height = 0.16
	var rig := Skeleton3D.new()
	rig.name = "Skeleton3D"
	node.add_child(rig)
	rig.owner = node
	var joints := Anatomy.rest_positions()
	for i in Anatomy.NAMES.size():
		rig.add_bone(Anatomy.NAMES[i])
		var parent: int = Anatomy.PARENTS[i]
		rig.set_bone_parent(i, parent)
		rig.set_bone_rest(i, Transform3D(Basis.IDENTITY, joints[i] - (joints[parent] if parent >= 0 else Vector3.ZERO)))
	rig.reset_bone_poses()
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var coat: Color = [Color("d79742"), Color("4b9caa"), Color("bf718d"), Color("aaa786"), Color("39768c")][look]
	var skin: Color = [Color("bd835b"), Color("e2b28d"), Color("825941"), Color("c58a67"), Color("c58a67")][look]
	var dark := Color("263747")
	# Shoulders narrow at the collar; waist and hips have distinct silhouettes.
	_taper(st, Vector3(0, 1.195, 0), 0.41, Vector2(0.17, 0.13), Vector2(0.25, 0.15), coat, Anatomy.SPINE)
	_taper(st, Vector3(0, 1.43, 0), 0.06, Vector2(0.25, 0.15), Vector2(0.11, 0.10), coat, Anatomy.SPINE)
	_taper(st, Vector3(0, 0.95, 0), 0.16, Vector2(0.18, 0.115), Vector2(0.20, 0.13), dark, Anatomy.PELVIS)
	_taper(st, Vector3(0, 1.49, 0), 0.10, Vector2(0.065, 0.065), Vector2(0.065, 0.065), skin, Anatomy.HEAD)
	_ellipsoid(st, Vector3(0, 1.635, 0), Vector3(0.27, 0.29, 0.255), skin, Anatomy.HEAD)
	_taper(st, Vector3(0, 1.775, -0.015), 0.07, Vector2(0.133, 0.125), Vector2(0.103, 0.10), dark.darkened(0.3), Anatomy.HEAD)
	_box(st, Vector3(0, 1.62, 0.135), Vector3(0.055, 0.06, 0.055), skin.darkened(0.08), Anatomy.HEAD)
	for side_index in range(2):
		var side := -1.0 if side_index == 0 else 1.0
		var thigh := Anatomy.LEFT_THIGH if side_index == 0 else Anatomy.RIGHT_THIGH
		var arm := Anatomy.LEFT_ARM if side_index == 0 else Anatomy.RIGHT_ARM
		_taper(st, Vector3(side * 0.115, 0.735, 0), 0.43, Vector2(0.073, 0.073), Vector2(0.101, 0.105), dark, thigh)
		_ellipsoid(st, Vector3(side * 0.115, 0.52, 0), Vector3(0.146, 0.146, 0.146), dark, thigh + 1)
		_taper(st, Vector3(side * 0.115, 0.31, 0), 0.42, Vector2(0.05, 0.055), Vector2(0.073, 0.075), dark, thigh + 1)
		_shoe(st, side * 0.115, thigh + 2)
		_ellipsoid(st, Vector3(side * 0.28, 1.40, 0), Vector3(0.16, 0.16, 0.17), coat, arm)
		_taper(st, Vector3(side * 0.28, 1.265, 0), 0.27, Vector2(0.058, 0.065), Vector2(0.078, 0.080), coat.darkened(0.07), arm)
		_ellipsoid(st, Vector3(side * 0.28, 1.13, 0), Vector3(0.116, 0.116, 0.13), coat.darkened(0.07), arm + 1)
		_taper(st, Vector3(side * 0.28, 1.015, 0), 0.23, Vector2(0.045, 0.055), Vector2(0.058, 0.065), coat.darkened(0.10), arm + 1)
		_ellipsoid(st, Vector3(side * 0.28, 0.865, 0), Vector3(0.10, 0.13, 0.105), skin, arm + 1)
	if look == 4:
		_box(st, Vector3(0, 1.28, 0.137), Vector3(0.39, 0.085, 0.035), Color("f1bf4c"), Anatomy.SPINE)
		_box(st, Vector3(0, 1.28, -0.137), Vector3(0.39, 0.085, 0.035), Color("f1bf4c"), Anatomy.SPINE)
	elif look % 2 == 0:
		_box(st, Vector3(0, 1.22, -0.18), Vector3(0.28, 0.36, 0.14), coat.darkened(0.45), Anatomy.SPINE)
	else:
		_box(st, Vector3(0, 1.23, 0.14), Vector3(0.04, 0.32, 0.025), Color("dde8dc"), Anatomy.SPINE)
	var mesh := MeshInstance3D.new()
	mesh.name = "Body"
	mesh.mesh = st.commit()
	mesh.material_override = material
	mesh.skin = rig.create_skin_from_rest_transforms()
	mesh.skeleton = NodePath("..")
	mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	mesh.custom_aabb = AABB(Vector3(-0.6, -0.05, -0.55), Vector3(1.2, 1.60, 1.1))
	rig.add_child(mesh)
	mesh.owner = node
	var scene := PackedScene.new()
	assert(scene.pack(node) == OK)
	var path := "res://scenes/people/ari_visual.tscn" if look == 4 else "res://scenes/people/passenger_%d.tscn" % look
	assert(ResourceSaver.save(scene, path) == OK)
	node.free()

func _box(st: SurfaceTool, center: Vector3, extent: Vector3, color: Color, bone: int) -> void:
	var box := BoxMesh.new()
	box.size = extent
	_emit(st, box.get_mesh_arrays(), center, Vector3.ONE, color, bone)

func _ellipsoid(st: SurfaceTool, center: Vector3, extent: Vector3, color: Color, bone: int) -> void:
	var sphere := SphereMesh.new()
	sphere.radius = 0.5
	sphere.height = 1.0
	sphere.radial_segments = 8
	sphere.rings = 3
	_emit(st, sphere.get_mesh_arrays(), center, extent, color, bone)

func _taper(st: SurfaceTool, center: Vector3, height: float, bottom: Vector2, top: Vector2, color: Color, bone: int) -> void:
	var cylinder := CylinderMesh.new()
	cylinder.height = height
	cylinder.bottom_radius = 1.0
	cylinder.top_radius = 1.0
	cylinder.radial_segments = 8
	cylinder.rings = 1
	var arrays := cylinder.get_mesh_arrays()
	var vertices: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
	for i in vertices.size():
		var width := bottom.lerp(top, clampf(vertices[i].y / height + 0.5, 0, 1))
		vertices[i].x *= width.x
		vertices[i].z *= width.y
	arrays[Mesh.ARRAY_VERTEX] = vertices
	_emit(st, arrays, center, Vector3.ONE, color, bone, true)

func _shoe(st: SurfaceTool, x: float, bone: int) -> void:
	var box := BoxMesh.new()
	box.size = Vector3(0.17, 0.10, 0.31)
	var arrays := box.get_mesh_arrays()
	var vertices: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
	for i in vertices.size():
		# Flat outsole, a low toe box and a taller heel/ankle.
		if vertices[i].z > 0:
			vertices[i].x *= 0.86
			if vertices[i].y > 0:
				vertices[i].y -= 0.035
	arrays[Mesh.ARRAY_VERTEX] = vertices
	_emit(st, arrays, Vector3(x, 0.05, 0.055), Vector3.ONE, Color("172832"), bone, true)

func _emit(st: SurfaceTool, arrays: Array, center: Vector3, extent: Vector3, color: Color, bone: int, flat := false) -> void:
	var indices: PackedInt32Array = arrays[Mesh.ARRAY_INDEX]
	var vertices: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
	var normals: PackedVector3Array = arrays[Mesh.ARRAY_NORMAL]
	for triangle in range(0, indices.size(), 3):
		var a := vertices[indices[triangle]] * extent
		var b := vertices[indices[triangle + 1]] * extent
		var c := vertices[indices[triangle + 2]] * extent
		var normal := (c - a).cross(b - a).normalized()
		for corner in range(3):
			var index := indices[triangle + corner]
			st.set_color(color)
			st.set_bones(PackedInt32Array([bone, 0, 0, 0]))
			st.set_weights(PackedFloat32Array([1, 0, 0, 0]))
			st.set_normal(normal if flat else (normals[index] / extent).normalized())
			st.add_vertex((vertices[index] * extent + center) * Anatomy.SCALE)
