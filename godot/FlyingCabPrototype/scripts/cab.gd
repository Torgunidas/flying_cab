class_name FlightCab
extends RigidBody3D

signal flight_reset
signal autopilot_changed(active: bool)
signal driver_changed(previous: StringName, current: StringName)

const Airspace = preload("res://scripts/airspace.gd")

const REST_SPEED := 0.1
const REST_DELAY := 0.1

## Expand this resource in the Inspector or assign another model .tres.
@export var definition: VehicleDefinition = preload("res://resources/vehicles/basic_cab.tres")
@export var world_definition: CityDefinition = preload("res://resources/city_02.tres")
@export var entity_id: StringName = &"ari_cab"
@export var initial_owner_id: StringName = &"ari"
var state := VehicleState.new()
var fuel_system := VehicleFuel.new()
var thrust_response := ThrustResponse.new()
var vitals := VehicleVitals.new()
var support_body: Node
var hull: float:
	get: return state.condition * definition.max_hull
var disabled: bool:
	get: return state.condition <= 0.0
@export var presentation: VehiclePresentation = VehiclePresentation.new()
var world_registry: WorldRegistry
var _control_revision := 0
var _standalone_lanes: Array[Node] = []
var command := Vector2.ZERO:
	set(value):
		command = Vector2(clampf(value.x, -1.0, 1.0), clampf(value.y, 0.0, 1.0))
		if not command.is_zero_approx() and sleeping:
			sleeping = false
var grounded := false
var airspace := Airspace.new()
var applied_command := Vector2.ZERO
var fuel: float:
	get: return state.fuel
	set(value): state.fuel = value
var fuel_burn_rate := 0.0
var refueling := false
var _on_refuel_pad := false
var resetting := false
var free_refueling := true
var reset_fuel_amount := -1.0
var reset_condition := 1.0
var spawn_transform: Transform3D
var _previous_velocity := Vector3.ZERO
var _pre_contact_velocity := Vector3.ZERO
var _visual_acceleration := 0.0
var _rest_time := 0.0
var highway_speed := 1.0
var highway_fuel := 1.0
@onready var visual: Node3D = $Visual
@onready var flight_fx: Node3D = $FlightFX

func _enter_tree() -> void:
	if definition and definition.validation_errors().is_empty():
		_apply_body()

func _apply_body() -> void:
	var previous := get_node("Visual") as Node3D
	if definition.visual_scene and previous.scene_file_path != definition.visual_scene.resource_path:
		var index := previous.get_index()
		remove_child(previous)
		previous.free()
		var body := definition.visual_scene.instantiate() as Node3D
		body.name = "Visual"
		add_child(body)
		move_child(body, index)
	var collider := get_node("Collision") as CollisionShape3D
	var shape := BoxShape3D.new()
	shape.size = definition.collision_size
	collider.shape = shape
	collider.position = definition.collision_offset

func apply_model(model: VehicleDefinition) -> bool:
	# Used when restoring a saved model onto an authored vehicle spawn.
	if model == null or not model.validation_errors().is_empty():
		return false
	definition = model.duplicate()
	mass = definition.mass_kg
	state.model_id = definition.model_id
	fuel = minf(fuel, definition.fuel_capacity)
	_apply_body()
	visual = get_node("Visual")
	if is_node_ready():
		clear_control_input()
		presentation.reset(visual)
		get_node("Headlights").bind_visual()
		flight_fx.bind_visual()
	return true

func _ready() -> void:
	if definition == null or not definition.validation_errors().is_empty():
		push_error("Invalid vehicle definition on " + String(name))
		freeze = true
		return
	# Editing a runtime instance must not mutate the shared model or other cars.
	definition = definition.duplicate()
	presentation = presentation.duplicate()
	_standalone_lanes = get_tree().get_nodes_in_group("highway")
	spawn_transform = global_transform
	_previous_velocity = linear_velocity
	_pre_contact_velocity = linear_velocity
	state.entity_id = entity_id
	state.owner_id = initial_owner_id
	state.model_id = definition.model_id
	state.map_id = world_definition.map_id
	fuel = minf(definition.starting_fuel, definition.fuel_capacity)
	set_meta("entity_id", String(entity_id))
	add_to_group("vehicle")
	custom_integrator = true
	can_sleep = true
	continuous_cd = true
	mass = definition.mass_kg
	axis_lock_linear_z = true
	axis_lock_angular_x = true
	axis_lock_angular_y = true
	axis_lock_angular_z = true
	max_contacts_reported = 8
	contact_monitor = true
	body_exited.connect(_on_body_exited)
	# Initialize the camera's interpolation history at the spawn, not the origin.
	get_global_transform_interpolated()
	reset_physics_interpolation()

func reset_flight() -> void:
	clear_control_input()
	resetting = true
	sleeping = false

func clear_control_input() -> void:
	# Session locks, driver changes and teleports are hard stops; ordinary
	# button release goes through the short engine release ramp instead.
	command = Vector2.ZERO
	thrust_response.reset()
	applied_command = Vector2.ZERO
	fuel_burn_rate = 0.0

func _on_body_exited(_body: Node) -> void:
	# Removing a static support need not wake a sleeping custom-integrated body.
	# Re-evaluate support instead of leaving the cab suspended in the air.
	_rest_time = 0.0
	_on_refuel_pad = false
	if sleeping:
		sleeping = false

func _integrate_forces(state: PhysicsDirectBodyState3D) -> void:
	# One source of mass for both thrust/mass integration and contact resolution.
	if not is_equal_approx(mass, definition.mass_kg):
		mass = maxf(1.0, definition.mass_kg)
	if resetting:
		state.transform = spawn_transform
		state.linear_velocity = Vector3.ZERO
		state.angular_velocity = Vector3.ZERO
		# Synchronize the node before clearing both renderer and camera histories.
		# The body's regular state sync runs after this callback.
		global_transform = spawn_transform
		grounded = false
		_on_refuel_pad = false
		refueling = false
		fuel = definition.fuel_capacity if reset_fuel_amount < 0 else minf(definition.fuel_capacity, reset_fuel_amount)
		self.state.condition = reset_condition
		reset_fuel_amount = -1.0
		reset_condition = 1.0
		vitals.reset()
		support_body = null
		fuel_burn_rate = 0.0
		applied_command = Vector2.ZERO
		highway_speed = 1.0
		highway_fuel = 1.0
		var was_returning := airspace.returning
		airspace.reset()
		if was_returning:
			autopilot_changed.emit(false)
		_visual_acceleration = 0.0
		_previous_velocity = Vector3.ZERO
		_pre_contact_velocity = Vector3.ZERO
		_rest_time = 0.0
		presentation.reset(visual)
		reset_physics_interpolation()
		resetting = false
		flight_reset.emit()
		return
	grounded = false
	_on_refuel_pad = false
	support_body = null
	var still_support := false
	var strongest_impact := 0.0
	for i in range(state.get_contact_count()):
		var normal := state.get_contact_local_normal(i)
		# Godot Physics reports zero impulse on some first-contact frames. Compare
		# recent outgoing velocities with the solver's result along the normal.
		# CCD can brake a fast body one step BEFORE it exposes a contact manifold.
		# Mass is already reflected in that result. One manifold is one impact,
		# and tangential travel/braking along a wall contributes no damage.
		strongest_impact = maxf(strongest_impact, (state.linear_velocity - _previous_velocity).dot(normal))
		strongest_impact = maxf(strongest_impact, (state.linear_velocity - _pre_contact_velocity).dot(normal))
		if normal.y > 0.65:
			grounded = true
			var collider := state.get_contact_collider_object(i)
			if collider is Node:
				support_body = collider
			if collider is Node and collider.is_in_group("refuel_pad"):
				_on_refuel_pad = true
		if normal.y > 0.99 and state.get_contact_collider_velocity_at_position(i).length() < 0.001:
			still_support = true
	vitals.impact(self.state, definition, strongest_impact)
	var was_returning := airspace.returning
	var requested := Vector2.ZERO
	if disabled:
		airspace.reset()
	else:
		requested = airspace.command_for(state.transform.origin, state.linear_velocity, command, definition, world_definition)
	if was_returning != airspace.returning:
		if not airspace.returning:
			thrust_response.reset()
		autopilot_changed.emit(airspace.returning)
	_update_highway(state.transform.origin, state.step)
	if disabled or (definition.fuel_enabled and fuel <= 0.0 and not airspace.returning):
		thrust_response.reset()
		requested = Vector2.ZERO
	else:
		requested = thrust_response.advance(requested, state.step, definition)
	_apply_fuel(requested, state.transform.origin.y, state.step)
	# Zero vertical speed at the ceiling is still powered flight, not parking.
	# Keep airborne bodies active so releasing thrust always restores free fall.
	can_sleep = still_support and applied_command.is_zero_approx()
	# The custom integrator adds next-step gravity after the contact solver.
	# Let a settled, unpowered body sleep before that creates another tiny bounce.
	# Check the solver's actual motion; never infer rest from absent input alone.
	if still_support and applied_command.is_zero_approx() and state.linear_velocity.length() < REST_SPEED:
		_rest_time += state.step
	else:
		_rest_time = 0.0
	if _rest_time >= REST_DELAY:
		highway_speed = 1.0
		highway_fuel = 1.0
		state.linear_velocity = Vector3.ZERO
		state.angular_velocity = Vector3.ZERO
		state.sleeping = true
		_previous_velocity = Vector3.ZERO
		_pre_contact_velocity = Vector3.ZERO
		_visual_acceleration = 0.0
		return
	state.linear_velocity = FlightModel.step_velocity(state.linear_velocity, applied_command, state.step, definition, highway_speed)
	state.linear_velocity = airspace.limit_climb(state.linear_velocity, state.transform.origin, state.step, definition, world_definition, highway_speed)
	_visual_acceleration = (state.linear_velocity.x - _previous_velocity.x) / state.step
	_pre_contact_velocity = _previous_velocity
	_previous_velocity = state.linear_velocity

func _update_highway(position: Vector3, dt: float) -> void:
	# Zones adjust limits and cost, never input, acceleration or coast damping.
	# Autopilot uses its own controlled return speed. Empty fuel disables assist.
	if disabled or not definition.highway_enabled or airspace.returning or (definition.fuel_enabled and fuel <= 0.0):
		highway_speed = 1.0
		highway_fuel = 1.0
		return
	var target_speed := 1.0
	var target_fuel := 1.0
	var lanes: Array[Node] = world_registry.lanes if world_registry else _standalone_lanes
	for lane in lanes:
		if is_instance_valid(lane) and lane.contains_point(position):
			target_speed = maxf(target_speed, lane.speed_multiplier)
			target_fuel = minf(target_fuel, lane.fuel_multiplier)
	var seconds := definition.highway_entry_seconds if target_speed > highway_speed else definition.highway_exit_seconds
	var change := 0.5 * dt / maxf(seconds, 0.01)
	highway_speed = move_toward(highway_speed, target_speed, change)
	highway_fuel = move_toward(highway_fuel, target_fuel, change)

func _apply_fuel(requested: Vector2, height: float, dt: float) -> void:
	var pressure := airspace.ceiling_pressure(height, world_definition) if definition.airspace_enabled else 0.0
	fuel_system.consume(state, definition, requested, pressure, highway_fuel, airspace.returning, dt)
	applied_command = fuel_system.applied_command
	fuel_burn_rate = fuel_system.burn_rate

func _physics_process(dt: float) -> void:
	vitals.advance(dt)
	fuel_system.refuel(state, definition, free_refueling and _on_refuel_pad and grounded and sleeping and command.is_zero_approx() and not airspace.returning, dt)
	refueling = fuel_system.refueling
	presentation.update(visual, _visual_acceleration, linear_velocity, applied_command, dt)
	flight_fx.update_visuals(dt)

func assign_driver(id: StringName) -> int:
	var previous := state.driver_id
	state.driver_id = id
	state.passenger_ids.erase(String(id))
	for trip in state.reservations.keys():
		if state.reservations[trip].has(String(id)):
			VehicleOccupancy.release(state, trip)
	_control_revision += 1
	clear_control_input()
	driver_changed.emit(previous, id)
	return _control_revision

func receive_command(value: Vector2, id: StringName, revision: int) -> bool:
	if id != state.driver_id or revision != _control_revision or id == &"":
		return false
	command = value
	return true

func capture_state() -> void:
	state.position = global_position
	state.velocity = linear_velocity
	state.has_pose = true

func restore_state(saved: VehicleState) -> void:
	clear_control_input()
	state = saved
	fuel = clampf(fuel, 0.0, definition.fuel_capacity)
	vitals.reset()
	support_body = null
	if state.has_pose:
		global_position = state.position
		linear_velocity = state.velocity
		_previous_velocity = state.velocity
		_pre_contact_velocity = state.velocity
		reset_physics_interpolation()
	sleeping = false

func apply_damage(amount: float, source: StringName = &"external") -> float:
	return vitals.apply_damage(state, definition, amount, source)

func board_passenger(id: StringName) -> bool:
	if id == &"" or disabled or id == state.driver_id or state.passenger_ids.has(String(id)) or state.passenger_ids.size() + VehicleOccupancy.reserved_count(state) >= definition.max_passengers:
		return false
	for party in state.reservations.values():
		if party.has(String(id)):
			return false
	state.passenger_ids.append(String(id))
	return true

func leave_passenger(id: StringName) -> bool:
	if not state.passenger_ids.has(String(id)):
		return false
	state.passenger_ids.erase(String(id))
	return true

func get_control_velocity() -> Vector3:
	return linear_velocity

func get_driver_id() -> StringName:
	return state.driver_id
