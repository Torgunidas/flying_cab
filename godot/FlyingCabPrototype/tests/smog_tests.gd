extends SceneTree

var scene: Node3D
var cab: FlightCab
var lights: Node3D
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
	cab.freeze = false
	cab.spawn_transform.origin = position
	cab.reset_flight()
	await frames(4)

func _run() -> void:
	scene = load("res://scenes/flight_lab.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	scene.set_physics_process(false)
	cab = scene.get_node("Cab")
	lights = cab.get_node("Headlights")
	var profile: Resource = lights.profile
	var depot: StaticBody3D = scene.get_node("City/LandingPads/Pad0")
	var home := cab.spawn_transform.origin
	await frames(120)
	check(cab.grounded and cab.sleeping and absf(cab.position.y-54.35)<0.08 and cab.position.x == -22.0,"new game starts parked at the elevated Ari depot")
	check(depot.get_node(depot.get_meta("building")).name == "WestArcology1", "Ari depot belongs to an existing grounded skyscraper")
	check(cab.position.y > profile.lights_off_height and not lights.lights_on and lights.light_amount == 0.0,"depot is above the fog and automatic headlights start off")
	check(profile.density_at(profile.top_height-profile.feather)==1.0 and profile.density_at(profile.top_height+6.0)==0.0 and profile.density_at(profile.top_height)>0.0,"shared smog profile has a gradual top below Ari's terrace")
	check(get_nodes_in_group("smog_bank").size()==3,"three world-anchored fog layers show the smog below the depot")
	var low_ring: Node3D = scene.get_node("City/Highways/LowRing")
	var road_bottom: float = low_ring.position.y - low_ring.size.y * 0.5
	check(road_bottom - (profile.top_height+8.0) >= 8.0,"even the highest animated smog wisps have at least 8 m clearance below the bottom highway")
	check(profile.density_at(road_bottom)==0.0 and road_bottom > profile.lights_off_height,"the entire bottom highway is outside the smog and headlight band")
	cab.fuel = 30.0
	await frames(60)
	check(cab.fuel > 54.0 and cab.fuel_burn_rate==0.0,"relocated depot still refuels the parked cab without engine burn")
	# Actual gravity-driven crossing, not an isolated call to the lamp controller.
	await place(Vector3(-12,profile.lights_off_height+4.0,0))
	var switched_on := 0
	var previous: bool = lights.lights_on
	var first_on_height := INF
	for i in range(150):
		await physics_frame
		if lights.lights_on != previous:
			switched_on += 1
			first_on_height = cab.position.y
		previous = lights.lights_on
	check(switched_on==1 and lights.lights_on and first_on_height<=profile.lights_on_height+0.25,"descending through the smog automatically switches headlights on exactly once")
	check(lights.light_amount > 0.99 and lights.get_node("Left").visible and lights.get_node("Beam").visible,"full light and visible scattering appear after the gradual fade")
	check(cab.command==Vector2.ZERO and cab.applied_command==Vector2.ZERO and cab.fuel_burn_rate==0.0,"headlights do not add phantom input or fuel consumption while descending")
	var env: Environment = scene.get_node("WorldEnvironment").environment
	check(env.ambient_light_energy < 0.25 and env.fog_enabled,"descent also selects the darker smog atmosphere")
	cab.command = Vector2(0,1)
	var switched_off := 0
	previous = lights.lights_on
	for i in range(360):
		await physics_frame
		if lights.lights_on != previous:
			switched_off += 1
		previous = lights.lights_on
	check(switched_off==1 and not lights.lights_on and lights.light_amount==0.0,"climbing out turns headlights off once after leaving the fog")
	# Boundary hovering must not make the lamps chatter.
	cab.freeze = true
	lights.update_lights(profile.lights_on_height-1.0,1.0,true)
	for offset in [0.1,1.0,-0.1,2.0,4.0,5.9]:
		lights.update_lights(profile.lights_on_height+offset,0.1)
	check(lights.lights_on,"headlights stay on when hovering inside the boundary band")
	lights.update_lights(profile.lights_off_height+0.1,1.0)
	for offset in [-0.1,-2.0,-5.0,-5.9]:
		lights.update_lights(profile.lights_off_height+offset,0.1)
	check(not lights.lights_on,"headlights stay off until re-entering the fog threshold")
	lights.update_lights(profile.top_height-profile.feather,0.15)
	check(lights.light_amount>0.0 and lights.light_amount<1.0,"automatic switching fades instead of flashing at full intensity")
	cab.fuel = 0.0
	lights.update_lights(profile.top_height-profile.feather,1.0)
	check(lights.lights_on and lights.light_amount==1.0,"an empty flight tank does not turn off safety headlights")
	cab.visual.scale.x = -1.0
	lights._physics_process(0.0)
	var direction: Vector3 = -lights.get_node("Left").global_basis.z.normalized()
	check(direction.dot(Vector3.LEFT)>0.99,"actual spotlights follow the cab's left-facing direction")
	cab.visual.scale.x = 1.0
	lights._physics_process(0.0)
	direction = -lights.get_node("Left").global_basis.z.normalized()
	check(direction.dot(Vector3.RIGHT)>0.99,"actual spotlights follow the cab's right-facing direction")
	await place(home)
	check(not lights.lights_on and lights.light_amount==0.0,"reset clears the fog lights immediately at the depot")
	# Use the live scene controller so the old Y=-20 fall recovery cannot hide here.
	await place(Vector3(-12,low_ring.position.y,0))
	scene.set_physics_process(true)
	await frames(480)
	check(cab.grounded and cab.sleeping and absf(cab.position.y-cab.world_definition.ground_height-0.35)<0.08,"a real descent from the bottom highway reaches the deeper ground without an unwanted reset")
	check(lights.lights_on and cab.fuel_burn_rate==0.0 and cab.fuel==cab.definition.fuel_capacity,"parking at the new bottom retains automatic lights and zero engine burn")
	var controls: FlightControls = scene.get_node("HUD/Controls")
	check(controls._altitude=="000" and controls._district_name=="LOWLIFE / STREFA SMOGU","HUD measures altitude from the new floor and recognizes deep low city")
	check(controls._city_bounds.position.y == cab.world_definition.ground_height and controls._city_bounds.has_point(Vector2(0, low_ring.position.y)), "city reference bounds include the added depth below the highway")
	print("SMOG_TESTS: %d/%d passed" % [checks-failures,checks])
	quit(0 if failures==0 else 1)
