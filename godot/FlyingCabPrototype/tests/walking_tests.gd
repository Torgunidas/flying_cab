extends SceneTree
## Distance-driven gait, real character collisions, and pooled passenger paths.
var checks := 0
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok:
		print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func frames(count: int) -> void:
	for i in range(count):
		await physics_frame
	await process_frame

func box(at: Vector3, size: Vector3) -> StaticBody3D:
	var body := StaticBody3D.new()
	var collider := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = size
	collider.shape = shape
	body.add_child(collider)
	body.position = at
	root.add_child(body)
	return body

func _run() -> void:
	for scene: PackedScene in TaxiDirector.PEOPLE:
		var person := scene.instantiate() as PassengerVisual
		root.add_child(person)
		for i in range(30):
			person.pose(1.0 / 60.0, Vector3(1.5 / 60.0, 0, 0), false, 1)
		var phase := person._gait_phase
		var leg := person.rig.get_bone_pose_rotation(HumanRig.LEFT_THIGH)
		person.reset_gait()
		for i in range(30):
			person.pose(1.0 / 30.0, Vector3(0.75 / 30.0, 0, 0), false, 1)
		check(is_equal_approx(phase, person._gait_phase) and leg.is_equal_approx(person.rig.get_bone_pose_rotation(HumanRig.LEFT_THIGH)), scene.resource_path + ": same distance, same step at different speeds and frame rates")
		for i in range(30):
			person.pose(1.0 / 60.0, Vector3.ZERO, false, 1)
		check(person._gait_phase == phase and person._gait_weight == 0 and person.rig.get_bone_global_pose(HumanRig.LEFT_FOOT).origin.is_equal_approx(HumanRig.rest_positions()[HumanRig.LEFT_FOOT]), "idle freezes the step and returns the feet to rest")
		person.pose(0.1, Vector3(-0.2, 0, -0.05), false, 1)
		check(person.basis.z.dot(Vector3(-0.2, 0, -0.05).normalized()) > 0.999, "motion overrides stale facing, including diagonal car-door paths")
		person.reset_gait()
		check(person._gait_phase == 0 and person._gait_weight == 0, "pool reuse discards the previous person's gait")
		_anatomy(person)
		_gait_contacts(person)
		person.free()
	var ari_model := load("res://scenes/people/ari_visual.tscn").instantiate() as PassengerVisual
	root.add_child(ari_model)
	_anatomy(ari_model)
	_gait_contacts(ari_model)
	ari_model.reset_gait()
	var landing_flat := true
	for frame in range(60):
		ari_model.pose_actor(1.0 / 60.0, &"land" if frame < 8 else &"idle", 1, Vector3.ZERO)
		for foot in [HumanRig.LEFT_FOOT, HumanRig.RIGHT_FOOT]:
			var foot_pose := ari_model.rig.get_bone_global_pose(foot)
			landing_flat = landing_flat and absf(foot_pose.origin.y - HumanRig.ANKLE_Y) < 0.001 and foot_pose.basis.y.dot(Vector3.UP) > 0.9999
	check(landing_flat, "landing bends the knees while both soles remain flat on the terrace")
	ari_model.free()
	await _actor()
	_passengers()
	print("WALKING_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)

func _anatomy(person: PassengerVisual) -> void:
	var body := person.get_node("Skeleton3D/Body") as MeshInstance3D
	var bounds := body.mesh.get_aabb()
	check(absf(bounds.end.y - HumanRig.HEIGHT) < 0.005 and absf(bounds.position.y) < 0.001 and person.scale == Vector3.ONE, "human geometry is baked at 1.16 units with soles at its origin")
	var cab := load("res://scenes/vehicles/visuals/basic_cab.tscn").instantiate() as Node3D
	var roof := cab.get_node("Roof") as MeshInstance3D
	var roof_height := roof.position.y + roof.mesh.get_aabb().end.y + 0.38
	cab.free()
	check(bounds.end.y / roof_height > 1.15 and bounds.end.y / roof_height < 1.25, "person is about 20% taller than the parked basic cab roof")
	check(person.rig.get_bone_count() == 13 and body.skin.get_bind_count() == 13 and body.mesh.get_surface_count() == 1, "one skinned mesh has articulated hips, knees, ankles and elbows")
	var rest := HumanRig.rest_positions()
	var shared := true
	for i in range(rest.size()):
		shared = shared and person.rig.get_bone_global_rest(i).origin.is_equal_approx(rest[i])
	check(shared, "Ari and all passenger looks share the same rest anatomy")

func _gait_contacts(person: PassengerVisual) -> void:
	var flat := true
	var planted := true
	var clearance := true
	var max_knee := 0.0
	var max_lift := 0.0
	var previous := [Vector3.ZERO, Vector3.ZERO]
	var was_stance := [false, false]
	var speed := person.stride_length * 2.0
	var step := speed / 240.0
	person.reset_gait()
	person._gait_weight = 1.0
	# Two complete cycles at 240 Hz. Body translation and foot travel must cancel.
	for frame in range(240):
		person.position.z += step
		person.pose(1.0 / 240.0, Vector3(0, 0, step), false, 0)
		var ground_contacts := 0
		for side in range(2):
			var foot := HumanRig.LEFT_FOOT if side == 0 else HumanRig.RIGHT_FOOT
			var pose := person.rig.get_bone_global_pose(foot)
			var phase := fposmod(person._gait_phase + side * 0.5, 1.0)
			var stance := phase < person.stance_fraction - 0.0001
			var heel := pose * Vector3(0, -HumanRig.ANKLE_Y, -0.10 * HumanRig.SCALE)
			var toe := pose * Vector3(0, -HumanRig.ANKLE_Y, 0.21 * HumanRig.SCALE)
			clearance = clearance and minf(heel.y, toe.y) > -0.001
			max_lift = maxf(max_lift, minf(heel.y, toe.y))
			max_knee = maxf(max_knee, absf(person.rig.get_bone_pose_rotation(foot - 1).get_angle()))
			var world_foot := person.to_global(pose.origin)
			if stance:
				ground_contacts += 1
				flat = flat and absf(heel.y) < 0.001 and absf(toe.y) < 0.001 and pose.basis.y.dot(Vector3.UP) > 0.9999
				if was_stance[side] and phase > step / person.stride_length + 0.0001:
					planted = planted and world_foot.distance_to(previous[side]) < 0.001
			previous[side] = world_foot
			was_stance[side] = stance
		if person.stance_fraction >= 0.5:
			clearance = clearance and ground_contacts >= 1
	check(flat, "both heel and toe are flat on the ground during every support phase")
	check(planted, "support feet stay fixed in world space while the body passes over them")
	check(clearance and max_lift > person.step_height * 0.7, "swing feet clear the ground; walking always has a support foot")
	check(max_knee > 0.8, "knees visibly flex instead of rotating a rigid leg")

func _actor() -> void:
	var ground := box(Vector3(0, -0.5, 0), Vector3(40, 1, 4))
	var wall := box(Vector3(6, 2, 0), Vector3(1, 4, 4))
	var ari := load("res://scenes/people/ari.tscn").instantiate() as WalkingActor
	root.add_child(ari)
	ari.place(Vector3(0, 0.59, 0))
	var visual := ari.get_node("Visual") as PassengerVisual
	var revision := ari.assign_driver(&"test")
	await frames(30)
	var start := ari.global_position
	ari.receive_command(Vector2.RIGHT, &"test", revision)
	await frames(30)
	var walked := ari.global_position.x - start.x
	check(walked > 0.85 and is_equal_approx(ari.velocity.x, 2.5), "Ari accelerates to the energetic 2.5 m/s jog at the corrected body scale")
	check(absf(visual._gait_phase - fposmod(walked / visual.stride_length, 1.0)) < 0.002, "Ari's accelerating steps follow actual ground displacement")
	ari.receive_command(Vector2.LEFT, &"test", revision)
	await frames(2)
	check(ari.velocity.x > 0 and ari.facing == 1 and visual.basis.z.x > 0.99, "reversing input does not turn Ari backwards while he is still braking")
	await frames(20)
	check(ari.velocity.x < 0 and ari.facing == -1 and visual.basis.z.x < -0.99, "Ari turns when actual movement reverses")
	ari.place(Vector3(4.7, 0.59, 0))
	check(visual._gait_phase == 0, "teleport clears gait instead of counting the relocation as steps")
	ari.receive_command(Vector2.RIGHT, &"test", revision)
	await frames(60)
	var stopped_phase := visual._gait_phase
	var stopped_at := ari.global_position
	await frames(30)
	check(ari.global_position.is_equal_approx(stopped_at) and visual._gait_phase == stopped_phase and visual._gait_weight == 0, "pushing against a real wall stops the feet along with the body")
	ari.receive_command(Vector2(0, 1), &"test", revision)
	await frames(5)
	check(ari.movement_state == &"jump" and visual._gait_phase == stopped_phase, "jump keeps its airborne pose without imaginary ground steps")
	ari.set_active(false)
	check(visual._gait_phase == 0 and not ari.visible, "entering a vehicle resets and hides the gait")
	ari.free()
	wall.free()
	ground.free()

func _passengers() -> void:
	var context := RuntimeContext.new()
	root.add_child(context)
	var director := TaxiDirector.new()
	director.context = context
	director.rules = TaxiRules.new()
	director.rules.max_offers = 0 # Only explicitly created path fixtures.
	root.add_child(director)
	director.initialized = true
	var stop := TaxiStop.new()
	root.add_child(stop)
	director.network.stops = {"depot": stop}
	for length in [1.5, 8.0, 11.0]:
		context.rides.offers.clear()
		stop.waiting_x = -length * 0.5
		stop.door_x = length * 0.5
		var offer := context.rides.create_offer("depot", "velvet_club", 30, 1, 1000)
		director._render_people(0)
		var person := director._pool[director._assigned[offer.party[0].id]] as PassengerVisual
		var previous := person.global_position
		director.advance(0.1)
		check(absf(previous.distance_to(person.global_position) - 0.15) < 0.0001 and person.basis.z.x < -0.99, "%.1f m approach uses 1.5 m/s and faces the waiting point" % length)
		for side in [-1, 1]:
			offer.walk = 1.0
			var from := stop.waiting_point() + Vector3(side * 1.2, 0, 0)
			offer.return_points = [[from.x, from.y, from.z]]
			offer.return_remaining = 1.0
			director._render_people(0)
			director.advance(0.1)
			check(absf(from.distance_to(person.global_position) - 0.15) < 0.0001 and person.basis.z.x * -side > 0.99, "return from side %d keeps walking speed and faces forward" % side)
			for i in range(8):
				director.advance(0.1)
			check(person.global_position.distance_to(stop.waiting_point()) < 0.0001 and offer.return_remaining == 0, "short return ends at the curb without a fixed two-second crawl")
		var departed_from := person.global_position
		director._depart(offer, "depot")
		context.rides.offers.clear()
		director._render_people(0)
		director.advance(0.1)
		check(absf(departed_from.distance_to(person.global_position) - 0.15) < 0.0001 and person.basis.z.x > 0.99, "departure keeps the same speed and faces the building")
		director.departures.clear()
	# A new actor reusing a pooled mesh must not inherit a cross-city stride.
	context.rides.offers.clear()
	director._render_people(0.1)
	stop.position.x = 100
	var newcomer := context.rides.create_offer("depot", "velvet_club", 30, 1, 1000)
	director.advance(0.1)
	var reused := director._pool[director._assigned[newcomer.party[0].id]] as PassengerVisual
	check(reused._gait_phase == 0 and reused._gait_weight == 0, "reused passenger slot does not animate the jump across the city")
	context.player.suspend(&"test")
	var at := reused.global_position
	director.advance(0.1)
	check(reused.global_position == at and reused._gait_phase == 0, "paused passenger simulation freezes movement and animation together")
	director.free()
	stop.free()
	context.free()
