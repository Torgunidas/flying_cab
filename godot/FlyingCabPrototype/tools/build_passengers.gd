extends SceneTree
## Explicit authoring tool: baked skinned people; never runs during gameplay.
var material := StandardMaterial3D.new()
var bones := [Vector3.ZERO, Vector3(-0.14, 0.82, 0), Vector3(0.14, 0.82, 0), Vector3(-0.31, 1.35, 0), Vector3(0.31, 1.35, 0), Vector3(0, 1.5, 0)]

func _initialize() -> void:
	material.vertex_color_use_as_albedo = true
	material.roughness = 0.9
	for look in range(4):
		_build(look)
	quit()

func _build(look: int) -> void:
	var node := Node3D.new()
	node.name = "Passenger"
	node.set_script(load("res://scripts/taxi/passenger_visual.gd"))
	var rig := Skeleton3D.new()
	rig.name = "Skeleton3D"
	node.add_child(rig)
	rig.owner = node
	for i in range(bones.size()):
		rig.add_bone("part_%d" % i)
		if i > 0:
			rig.set_bone_parent(i, 0)
		rig.set_bone_rest(i, Transform3D(Basis.IDENTITY, bones[i]))
	rig.reset_bone_poses()
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var coat: Color = [Color("d79742"), Color("4b9caa"), Color("bf718d"), Color("aaa786")][look]
	var skin: Color = [Color("bd835b"), Color("e2b28d"), Color("825941"), Color("c58a67")][look]
	var dark := Color("263747")
	_part(st, Vector3(0, 1.15, 0), Vector3(0.46, 0.57, 0.28), coat, 0)
	_part(st, Vector3(0, 0.83, 0), Vector3(0.39, 0.20, 0.25), dark, 0)
	_part(st, Vector3(0, 1.48, 0), Vector3(0.14, 0.13, 0.15), skin, 5)
	_part(st, Vector3(0, 1.67, 0), Vector3(0.28, 0.31, 0.27), skin, 5)
	_part(st, Vector3(0, 1.82, -0.025), Vector3(0.30, 0.09, 0.28), dark.darkened(0.3), 5)
	_part(st, Vector3(0, 1.64, 0.155), Vector3(0.08, 0.09, 0.07), skin.darkened(0.1), 5)
	for i in range(2):
		var side := -1.0 if i == 0 else 1.0
		_part(st, Vector3(side * 0.14, 0.45, 0), Vector3(0.16, 0.7, 0.20), dark, i + 1)
		_part(st, Vector3(side * 0.14, 0.08, 0.05), Vector3(0.20, 0.14, 0.30), Color("13212a"), i + 1)
		_part(st, Vector3(side * 0.31, 1.09, 0), Vector3(0.14, 0.53, 0.18), coat.darkened(0.12), i + 3)
		_part(st, Vector3(side * 0.31, 0.79, 0), Vector3(0.13, 0.13, 0.14), skin, i + 3)
	if look % 2 == 0:
		_part(st, Vector3(0, 1.19, -0.21), Vector3(0.34, 0.43, 0.20), coat.darkened(0.45), 0)
	else:
		_part(st, Vector3(0, 1.22, 0.15), Vector3(0.055, 0.36, 0.025), Color("dde8dc"), 0)
	var mesh := MeshInstance3D.new()
	mesh.name = "Body"
	mesh.mesh = st.commit()
	mesh.material_override = material
	mesh.skin = rig.create_skin_from_rest_transforms()
	mesh.skeleton = NodePath("..")
	mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	mesh.custom_aabb = AABB(Vector3(-1, -0.3, -0.7), Vector3(2, 2.7, 1.4))
	rig.add_child(mesh)
	mesh.owner = node
	var scene := PackedScene.new()
	assert(scene.pack(node) == OK)
	assert(ResourceSaver.save(scene, "res://scenes/people/passenger_%d.tscn" % look) == OK)
	node.free()

func _part(st: SurfaceTool, center: Vector3, extent: Vector3, color: Color, bone: int) -> void:
	var box := BoxMesh.new()
	box.size = extent
	var arrays := box.get_mesh_arrays()
	for index: int in arrays[Mesh.ARRAY_INDEX]:
		st.set_color(color)
		st.set_bones(PackedInt32Array([bone, 0, 0, 0]))
		st.set_weights(PackedFloat32Array([1, 0, 0, 0]))
		st.set_normal(arrays[Mesh.ARRAY_NORMAL][index])
		st.add_vertex(arrays[Mesh.ARRAY_VERTEX][index] + center)
