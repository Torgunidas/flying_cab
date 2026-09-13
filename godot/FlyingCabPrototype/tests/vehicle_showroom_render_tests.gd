extends SceneTree
## Saved gallery plus a full-resolution image of each model, using actual renderer.
func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Showroom render verification needs a graphics renderer")
		quit(1)
		return
	var gallery: Node3D = load("res://scenes/vehicle_showroom.tscn").instantiate()
	root.add_child(gallery)
	current_scene = gallery
	while not gallery.ready_for_view:
		await process_frame
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/vehicle-library.png")
	for i in range(gallery.models.get_child_count()):
		gallery.show_model(i)
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		var id: String = gallery.models.get_child(i).name
		root.get_texture().get_image().save_png("res://build/vehicle-" + id + ".png")
		print("RENDERED: ", id)
	# Static attachment proof only, never game cargo mechanics.
	gallery.show_model(8)
	var socket: Marker3D = gallery.models.get_child(8).get_node("Vehicle/Visual/CargoMount")
	var sample: Node3D = load("res://scenes/vehicles/visuals/basic_cab.tscn").instantiate()
	socket.add_child(sample)
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://build/vehicle-tow-fit.png")
	sample.free()
	print("VEHICLE_SHOWROOM_RENDER: 12 models + static tow fit rendered")
	quit()
