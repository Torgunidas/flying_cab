extends RefCounted
## Read persisted CPU data: headless instance getters return identity transforms.
static func validate(tree_root: Node, source_path: String, runtime_path: String) -> PackedStringArray:
	var errors := PackedStringArray()
	var source_scene = ResourceLoader.load(source_path, "PackedScene", ResourceLoader.CACHE_MODE_IGNORE)
	var runtime_scene = ResourceLoader.load(runtime_path, "PackedScene", ResourceLoader.CACHE_MODE_IGNORE)
	if not source_scene is PackedScene or not runtime_scene is PackedScene:
		return PackedStringArray(["Could not load both city scenes"])
	var source: Node3D = source_scene.instantiate()
	var runtime: Node3D = runtime_scene.instantiate()
	tree_root.add_child(source)
	tree_root.add_child(runtime)
	var represented := {}
	var batches := runtime.find_children("*", "MultiMeshInstance3D", true, false)
	if batches.is_empty():
		errors.append("Compiled city has no render batches")
	for batch: MultiMeshInstance3D in batches:
		var mm := batch.multimesh
		var paths: PackedStringArray = batch.get_meta("source_nodes", PackedStringArray())
		var buffer := mm.buffer
		if mm.transform_format != MultiMesh.TRANSFORM_3D or mm.use_colors or mm.use_custom_data or buffer.size() != mm.instance_count * 12 or paths.size() != mm.instance_count:
			errors.append("Incomplete transforms/source mapping: " + String(batch.name))
			continue
		for i in range(mm.instance_count):
			var original := source.get_node_or_null(NodePath(paths[i])) as MeshInstance3D
			if original == null or represented.has(paths[i]) or runtime.has_node(NodePath(paths[i])):
				errors.append("Missing or duplicate source detail: " + paths[i])
				continue
			represented[paths[i]] = true
			var offset := i * 12
			var basis := Basis(Vector3(buffer[offset], buffer[offset+4], buffer[offset+8]), Vector3(buffer[offset+1], buffer[offset+5], buffer[offset+9]), Vector3(buffer[offset+2], buffer[offset+6], buffer[offset+10]))
			var pose := Transform3D(basis, Vector3(buffer[offset+3], buffer[offset+7], buffer[offset+11]))
			var actual := batch.global_transform * pose
			if not actual.is_finite() or not actual.is_equal_approx(original.global_transform):
				errors.append("Transform changed during compilation: " + paths[i])
			# With a renderer also verify the GPU-facing instance API. The dummy
			# renderer deliberately does not implement this getter.
			if DisplayServer.get_name() != "headless" and not mm.get_instance_transform(i).is_equal_approx(pose):
				errors.append("Renderer disagrees with persisted transform: " + paths[i])
			if mm.mesh.get_class() != original.mesh.get_class() or not mm.mesh.get_aabb().is_equal_approx(original.mesh.get_aabb()):
				errors.append("Mesh changed during compilation: " + paths[i])
	for node in source.find_children("*", "MeshInstance3D", true, false):
		var path := String(source.get_path_to(node))
		if not runtime.has_node(NodePath(path)) and not represented.has(path):
			errors.append("Source geometry disappeared: " + path)
	source.free()
	runtime.free()
	return errors
