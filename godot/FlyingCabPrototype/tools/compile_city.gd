extends SceneTree
## Explicit offline compilation. The editable city.tscn is never overwritten.
## Small spatial batches retain frustum culling and independent interaction nodes.

const SOURCE := "res://scenes/city.tscn"
const OUTPUT := "res://scenes/city_runtime.tscn"
const PENDING := "res://scenes/city_runtime.pending.tscn"
const CELL := 32.0
const Check = preload("res://tools/city_compilation_check.gd")

func _initialize() -> void:
	call_deferred("_compile")

func _compile() -> void:
	var city: Node3D = load(SOURCE).instantiate()
	root.add_child(city)
	var batches := Node3D.new()
	batches.name = "RenderBatches"
	city.add_child(batches)
	batches.owner = city
	var groups := {}
	var casters := 0
	var merged := 0
	for node: MeshInstance3D in city.find_children("*", "MeshInstance3D", true, false):
		# Large silhouettes and landing decks retain real shadows. Tiny services,
		# neon strips, glyphs and background skyline do not need their own shadow pass.
		var structural := String(node.name) in ["Sump", "Undercity", "ServiceBelt", "UpperCity", "Foundation", "SetbackCrown", "GroundedHousing", "Roof", "Deck"]
		node.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON if structural else GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		casters += int(structural)
		if structural or not _batchable(node) or node.get_child_count() != 0:
			continue
		var p := node.global_position
		var cell := Vector2i(floori(p.x / CELL), floori(p.y / CELL))
		var key := "%s:%s:%s" % [cell, node.mesh.get_instance_id(), node.material_override.get_instance_id()]
		if not groups.has(key):
			groups[key] = []
		groups[key].append(node)
	for key: String in groups:
		var members: Array = groups[key]
		if members.size() < 3:
			continue
		var first: MeshInstance3D = members[0]
		var mesh := MultiMesh.new()
		mesh.transform_format = MultiMesh.TRANSFORM_3D
		mesh.mesh = first.mesh
		mesh.instance_count = members.size()
		var instance := MultiMeshInstance3D.new()
		instance.name = "Details%03d" % batches.get_child_count()
		instance.multimesh = mesh
		instance.material_override = first.material_override
		instance.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		batches.add_child(instance)
		instance.owner = city
		var buffer := PackedFloat32Array()
		var source_nodes := PackedStringArray()
		for i in range(members.size()):
			var pose: Transform3D = instance.global_transform.affine_inverse() * members[i].global_transform
			# Godot's headless dummy renderer ignores set_instance_transform().
			# A complete row-major buffer is retained by both dummy and GPU backends.
			buffer.append_array(PackedFloat32Array([
				pose.basis.x.x, pose.basis.y.x, pose.basis.z.x, pose.origin.x,
				pose.basis.x.y, pose.basis.y.y, pose.basis.z.y, pose.origin.y,
				pose.basis.x.z, pose.basis.y.z, pose.basis.z.z, pose.origin.z,
			]))
			source_nodes.append(String(city.get_path_to(members[i])))
			members[i].free()
		mesh.buffer = buffer
		instance.set_meta("source_nodes", source_nodes)
		merged += members.size()
	for label: Label3D in city.find_children("*", "Label3D", true, false):
		label.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	for pad in get_nodes_in_group("refuel_pad"):
		if not pad.has_meta("entity_id"):
			pad.set_meta("entity_id", "city_02/pad/" + String(pad.name))
		if not pad.has_meta("services"):
			pad.set_meta("services", PackedStringArray(["refuel"]))
	for lane in get_nodes_in_group("highway"):
		if not lane.has_meta("entity_id"):
			lane.set_meta("entity_id", "city_02/lane/" + String(lane.name))
	city.set_meta("source_sha256", FileAccess.get_sha256(SOURCE))
	city.set_meta("compiler_version", 2)
	var packed := PackedScene.new()
	var result := packed.pack(city)
	if result == OK:
		result = ResourceSaver.save(packed, PENDING)
	if result == OK:
		var errors := Check.validate(root, SOURCE, PENDING)
		if not errors.is_empty():
			for problem in errors.slice(0, 8):
				push_error(problem)
			result = ERR_INVALID_DATA
		else:
			result = DirAccess.rename_absolute(PENDING, OUTPUT)
	print("CITY_COMPILE: merged=", merged, " batches=", batches.get_child_count(), " shadow_casters=", casters, " result=", result)
	quit(0 if result == OK else 1)

func _batchable(node: MeshInstance3D) -> bool:
	if node.get_script() != null or not node.get_groups().is_empty() or not node.get_meta_list().is_empty():
		return false
	if not node.mesh or not node.material_override or not node.material_override is StandardMaterial3D:
		return false
	if node.material_override.transparency != BaseMaterial3D.TRANSPARENCY_DISABLED:
		return false
	# Keep all authored objects with gameplay meaning and all effect materials.
	for prefix in ["Vent", "FloorBand", "UpperBand", "UtilityBox", "GoldMullion", "GoldFin", "ContainerSeam", "LandingStripe", "GridBoundary", "Shutter", "ServiceGrille", "Duct", "WindowBrace", "SealedWindow"]:
		if String(node.name).begins_with(prefix) or String(node.name).contains(prefix):
			return true
	return false
