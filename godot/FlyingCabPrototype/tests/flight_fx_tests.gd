extends SceneTree

var scene: Node3D
var cab: FlightCab
var fx: Node3D
var checks := 0
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(condition: bool, label: String) -> void:
	checks += 1
	if condition:
		print("PASS: ",label)
	else:
		failures += 1
		push_error("FAIL: "+label)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func place(position: Vector3) -> void:
	cab.spawn_transform.origin = position
	cab.reset_flight()
	await frames(4)

func all_plumes(visible: bool) -> bool:
	for mount in fx.rig.get_children():
		if mount.get_node("Plume").visible != visible:
			return false
	return true

func exhaust() -> Vector3:
	return -fx.rig.get_child(0).global_basis.y.normalized()

func _run() -> void:
	scene = load("res://scenes/flight_lab.tscn").instantiate()
	cab = scene.get_node("Cab")
	cab.definition = cab.definition.duplicate()
	root.add_child(scene)
	current_scene = scene
	scene.set_physics_process(false)
	fx = cab.get_node("FlightFX")
	await frames(120)
	check(cab.sleeping and not fx.wake.visible and all_plumes(false) and fx.nozzle_angle==0.0,"parked cab has neutral nozzles and no flame or express wake")
	var mounts: Array[Vector3] = []
	for mount in fx.rig.get_children():
		mounts.append(mount.position)
	cab.definition.gravity = 0.0
	await place(Vector3(-40,160,0))
	check(not fx.wake.visible,"being inside an express lane at rest does not suggest motion")
	cab.command = Vector2.RIGHT
	await frames(120)
	check(cab.linear_velocity.x>15.7 and fx.boost_amount>0.99 and fx.wake.visible,"real highway acceleration activates the wake at express speed")
	check(absf(fx.nozzle_angle+16.0)<0.05 and exhaust().x < -0.20 and all_plumes(true),"rightward thrust tilts each firing assembly left relative to the hull, capped at 16 degrees")
	check(fx.wake.global_basis.x.normalized().dot(cab.linear_velocity.normalized())>0.999,"express wake follows actual travel direction")
	var before: float = fx.nozzle_angle
	cab.command = Vector2.LEFT
	await frames(4)
	check(absf(fx.nozzle_angle-before)>0.1 and absf(fx.nozzle_angle-before)<20.0,"reversal starts turning the nozzles smoothly instead of snapping to the other extreme")
	var bounded := true
	for i in range(180):
		await physics_frame
		bounded = bounded and absf(fx.nozzle_angle)<=16.001
	check(bounded and cab.visual.scale.x<0 and exhaust().x>0.20,"left travel and the body-facing flip preserve the opposite exhaust direction within the small angle limit")
	var anchored := true
	for i in range(fx.rig.get_child_count()):
		anchored = anchored and fx.rig.get_child(i).position.is_equal_approx(mounts[i])
	check(anchored,"four assemblies pivot in place without sliding their attachment points")
	cab.command = Vector2.ZERO
	await frames(100)
	check(all_plumes(false) and absf(fx.nozzle_angle)<0.02 and not fx.wake.visible and cab.fuel_burn_rate==0.0,"releasing input returns nozzles to neutral and coasting clears the wake without fuel burn")
	await place(Vector3(12,200,0))
	cab.command = Vector2(1,1)
	await frames(90)
	check(cab.linear_velocity.length()>14.0 and cab.highway_speed==1.0 and fx.boost_amount==0.0,"ordinary diagonal flight cannot falsely trigger the express effect")
	await place(Vector3(0.5,80,0))
	cab.command = Vector2(0,1)
	await frames(120)
	check(cab.linear_velocity.y>17.2 and fx.boost_amount>0.99 and absf(fx.nozzle_angle)<0.01,"vertical highway ascent shows speed while the nozzles remain straight")
	check(fx.wake.global_basis.x.normalized().dot(Vector3.UP)>0.999,"vertical flight leaves the wake below the cab")
	cab.definition.gravity = 9.8
	await place(Vector3(0.5,130,0))
	await frames(240)
	check(cab.linear_velocity.y < -19.4 and fx.boost_amount>0.99 and all_plumes(false) and cab.fuel_burn_rate==0.0,"fast express descent creates airflow streaks without inventing engine thrust")
	check(fx.wake.global_basis.x.normalized().dot(Vector3.DOWN)>0.999,"falling leaves the wake above the cab")
	cab.fuel = 0.0
	cab.command = Vector2.RIGHT
	await frames(4)
	check(cab.applied_command==Vector2.ZERO and not fx.wake.visible and all_plumes(false),"empty fuel suppresses powered effects despite held input")
	cab.definition.gravity = 0.0
	await place(Vector3(-40,160,0))
	cab.command = Vector2.RIGHT
	await frames(120)
	cab.airspace.returning = true
	fx.update_visuals(1.0/60.0)
	check(fx.boost_amount==0.0 and not fx.wake.visible,"forced return suppresses express presentation immediately")
	await place(Vector3(-40,160,0))
	cab.command = Vector2.RIGHT
	await frames(120)
	cab.definition.highway_enabled = false
	await frames(30)
	check(fx.boost_amount==0.0 and not fx.wake.visible,"leaving highway assist fades the speed effect out")
	cab.definition.highway_enabled = true
	await frames(120)
	await place(Vector3(-22,54.4,0))
	check(not fx.wake.visible and fx.boost_amount==0.0 and fx.nozzle_angle==0.0 and all_plumes(false),"reset clears the complete presentation without a residual trail or firing nozzle")
	print("FLIGHT_FX_TESTS: %d/%d passed" % [checks-failures,checks])
	quit(0 if failures==0 else 1)
