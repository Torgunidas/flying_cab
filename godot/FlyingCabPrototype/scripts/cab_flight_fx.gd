extends Node3D
## Presentation only: reads actual engine command and velocity, never applies forces.
@export_range(0.0, 30.0) var nozzle_max_angle_degrees := 16.0
@export var nozzle_response := 8.0
@export var nozzle_return := 5.0
@export var wake_length := 5.5
var nozzle_angle := 0.0
var boost_amount := 0.0
var _time := 0.0
var _facing := 1.0
var _plume_material: StandardMaterial3D
var _mounts: Array[Node] = []
var _plumes: Array[Node3D] = []
var _ribbons: Array[Node] = []
var _previous_boost := -1.0
@onready var cab: FlightCab = get_parent()
@onready var rig: Node3D = $"../Visual/Thrusters"
@onready var wake: Node3D = $Wake

func _ready() -> void:
	_ribbons = wake.get_children()
	for ribbon in _ribbons:
		ribbon.material_override = ribbon.material_override.duplicate()
	cab.flight_reset.connect(reset_visuals)
	bind_visual()

func bind_visual() -> void:
	rig = cab.get_node("Visual/Thrusters")
	_mounts = rig.get_children()
	_plumes.clear()
	for mount in _mounts:
		_plumes.append(mount.get_node("Plume"))
	_plume_material = rig.get_child(0).get_node("Plume/Flame").material_override.duplicate()
	for mount in _mounts:
		mount.get_node("Plume/Flame").material_override = _plume_material
	reset_visuals()

func reset_visuals() -> void:
	nozzle_angle = 0.0
	boost_amount = 0.0
	_time = 0.0
	_facing = cab.get_node("Visual").scale.x
	wake.hide()
	wake.rotation = Vector3.ZERO
	for mount in _mounts:
		mount.rotation.z = 0.0
		mount.get_node("Plume").hide()
	reset_physics_interpolation()
	rig.reset_physics_interpolation()
	var beacons := cab.get_node_or_null("Visual/Beacons")
	if beacons:
		beacons.warmup = false

func update_visuals(dt: float) -> void:
	_time += dt
	_update_wake(dt)
	# Mirror the local angle when the body flips, preserving the world exhaust
	# direction during a reversal. Each assembly pivots at its own fixed mount.
	var facing: float = cab.visual.scale.x
	if facing != _facing:
		nozzle_angle = -nozzle_angle
		_facing = facing
	var command: Vector2 = cab.applied_command
	var firing := not command.is_zero_approx() and not cab.sleeping and not cab.resetting
	var target := 0.0
	if firing:
		var demand_angle := rad_to_deg(atan2(command.x * cab.definition.horizontal_acceleration, maxf(command.y * cab.definition.vertical_acceleration, 0.001)))
		target = -facing * clampf(demand_angle * 0.45, -nozzle_max_angle_degrees, nozzle_max_angle_degrees)
		# Direction alone stays constant while a lateral engine spools down.
		# Power also returns its nozzle towards neutral before the reversal.
		target *= maxf(absf(command.x), command.y)
	var response := nozzle_return if is_zero_approx(target) else nozzle_response
	nozzle_angle = lerpf(nozzle_angle, target, 1.0-exp(-dt*response))
	var power := maxf(command.y, absf(command.x)*0.55)
	var length := (0.18 + power*(0.95+0.07*sin(_time*37.0))) * (1.0+boost_amount*0.4)
	for i in range(_mounts.size()):
		_mounts[i].rotation.z = deg_to_rad(nozzle_angle)
		var plume: Node3D = _plumes[i]
		plume.visible = firing
		plume.scale.y = length
	if not is_equal_approx(_previous_boost, boost_amount):
		_previous_boost = boost_amount
		_plume_material.albedo_color = Color("72e8ec").lerp(Color("d4fff8"),boost_amount)
		_plume_material.emission = _plume_material.albedo_color
		_plume_material.emission_energy_multiplier = 1.0+boost_amount*1.4

func _update_wake(dt: float) -> void:
	var tuning := cab.definition
	var velocity := cab.linear_velocity
	var vertical_cap := tuning.max_climb_speed if velocity.y >= 0.0 else tuning.max_fall_speed
	# Compare each axis with its ordinary cap: diagonal base flight must not
	# masquerade as express speed merely because its total velocity is higher.
	var speed_ratio := maxf(absf(velocity.x)/tuning.max_horizontal_speed, absf(velocity.y)/vertical_cap)
	var assisted := cab.highway_speed > 1.01 and tuning.highway_enabled and not cab.airspace.returning
	var available := not cab.disabled and (not tuning.fuel_enabled or cab.fuel > 0.0)
	var target := clampf((speed_ratio-1.0)/0.5,0.0,1.0) if assisted and available else 0.0
	if cab.sleeping or cab.grounded or cab.resetting or not available or cab.airspace.returning:
		boost_amount = 0.0
	else:
		boost_amount = move_toward(boost_amount,target,dt/(0.18 if target>boost_amount else 0.25))
	var was_visible := wake.visible
	wake.visible = boost_amount > 0.01
	if not wake.visible:
		return
	wake.rotation.z = atan2(velocity.y,velocity.x)
	for i in range(_ribbons.size()):
		var ribbon: MeshInstance3D = _ribbons[i]
		var length := lerpf(1.5,wake_length,boost_amount) * (1.0 if i==1 else 0.78)
		ribbon.scale.x = length
		ribbon.position.x = -1.2-length*0.5
		ribbon.material_override.set_shader_parameter("strength",boost_amount)
	if not was_visible:
		wake.reset_physics_interpolation()

func show_warmup_effects() -> void:
	var beacons := cab.get_node_or_null("Visual/Beacons")
	if beacons:
		beacons.warmup = true
		beacons.update_lamps()
	for plume in _plumes:
		plume.show()
		plume.scale.y = 1.0
	wake.show()
	for ribbon in _ribbons:
		ribbon.scale.x = wake_length
		ribbon.position.x = -1.2 - wake_length * 0.5
		ribbon.material_override.set_shader_parameter("strength", 1.0)
