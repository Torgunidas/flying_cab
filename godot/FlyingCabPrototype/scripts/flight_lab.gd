extends Node3D

@export var definition: CityDefinition = preload("res://resources/city_02.tres")
@export var camera_tuning: FlightCameraTuning = FlightCameraTuning.new()
## Height of the NPC text area, not the whole dialogue panel. 172 UI pixels
## matches three standard reply buttons with their gaps. Editable during pause.
@export_range(52, 360, 1) var dialogue_npc_text_height := 172.0:
	set(value):
		dialogue_npc_text_height = value
		if is_instance_valid(narrative_panel): narrative_panel.npc_text_height = value
@export var living_world_enabled := true
var living_world: LivingWorldDirector
var _preparing := false
var _prepared := false
var context: RuntimeContext
@onready var cab: FlightCab = $Cab
@onready var camera: Camera3D = $Camera3D
@onready var controls: FlightControls = $HUD/Controls
var _look_ahead := Vector3.ZERO
var _camera_target := Vector3.ZERO
var _hud_elapsed := 0.0
var workshop := WorkshopInteraction.new()
var taxi: TaxiDirector
var taxi_hud: TaxiHud
var on_foot: OnFootInteraction
var narrative_panel: NarrativePanel
var _camera_distance := 19.0
var _camera_angle := 5.0
var _foot_anchor_y := 0.0
var _platform_approach := 0.0

func _ready() -> void:
	var standalone := context == null
	_fit_desktop_window()
	if context == null:
		context = RuntimeContext.new()
		add_child(context)
	context.world.bind(self, definition)
	context.current_map = definition.map_id
	for vehicle in context.world.vehicles:
		_bind_vehicle(vehicle)
	context.world.vehicle_registered.connect(_bind_vehicle)
	controls.bind_world(context.world)
	controls.reset_requested.connect(_reset)
	context.player.focus_changed.connect(_focus_changed)
	context.player.control_suspended.connect(controls.set_control_suspended)
	context.player.take_control(cab, &"flight")
	_camera_distance = camera_tuning.camera_distance
	_camera_angle = camera_tuning.camera_angle_degrees
	# Follow the rendered cab every frame; automatic camera interpolation would
	# add a second interpolation pass to this manually smoothed camera.
	camera.physics_interpolation_mode = Node.PHYSICS_INTERPOLATION_MODE_OFF
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.fov = camera_tuning.camera_fov
	_snap_camera()
	if context.rides.enabled:
		taxi = TaxiDirector.new()
		taxi.context = context
		add_child(taxi)
		controls.taxi_mode = true
		taxi_hud = TaxiHud.new()
		taxi_hud.director = taxi
		taxi_hud.controls = controls
		$HUD.add_child(taxi_hud)
		taxi_hud.foot_recovery_requested.connect(_reset)
	on_foot = OnFootInteraction.new()
	on_foot.context = context
	on_foot.controls = controls
	add_child(on_foot)
	on_foot.restore_player()
	narrative_panel = NarrativePanel.new()
	narrative_panel.name = "NarrativePanel"
	narrative_panel.npc_text_height = dialogue_npc_text_height
	narrative_panel.context = context
	narrative_panel.controls = controls
	add_child(narrative_panel)
	context.narrative.notice.connect(_narrative_notice)
	if standalone:
		prepare_gameplay.call_deferred()

func prepare_gameplay() -> void:
	if _prepared:
		return
	if _preparing:
		while not _prepared:
			await get_tree().process_frame
		return
	_preparing = true
	await get_tree().physics_frame
	await get_tree().physics_frame
	if taxi:
		taxi.initialize()
	if living_world_enabled:
		living_world = LivingWorldDirector.new()
		living_world.context = context
		add_child(living_world)
		living_world.initialize()
		living_world.vehicle_taken.connect(_narrative_vehicle_taken)
		# Saved player vehicles may be owned by the population, not authored Cab.
		on_foot.restore_player()
	_prepared = true
	_preparing = false

func _narrative_notice(text: String) -> void:
	if taxi:
		taxi.message(text, 5)

func _narrative_vehicle_taken(vehicle: FlightCab, previous_owner: StringName) -> void:
	context.narrative.record_event("vehicle_taken", {"vehicle": String(vehicle.entity_id), "target": String(previous_owner)})

func _fit_desktop_window() -> void:
	if OS.has_feature("web") or OS.has_feature("mobile") or DisplayServer.get_name() == "headless":
		return
	var window := get_window()
	var arguments := OS.get_cmdline_args()
	# Godot consumes --resolution before exposing its remaining arguments.
	# Scripted tests retain their requested size; manual runs can opt out too.
	if arguments.has("--script") or OS.get_cmdline_user_args().has("--keep-window-size") or arguments.has("--wid") or window.is_embedded() or window.mode != Window.MODE_WINDOWED:
		return
	var usable := DisplayServer.screen_get_usable_rect(window.current_screen)
	# Screen and window sizes use the same pixels, including on Retina displays.
	# Fit the whole decorated window, leaving a small margin around the screen.
	var window_id := window.get_window_id()
	var decorations := DisplayServer.window_get_size_with_decorations(window_id) - window.size
	var title_offset := window.position - DisplayServer.window_get_position_with_decorations(window_id)
	var available := Vector2(usable.size) * 0.98 - Vector2(decorations)
	var unit := floori(minf(available.x / 9.0, available.y / 16.0))
	if unit < 1:
		return
	window.size = Vector2i(9, 16) * unit
	var centered_offset := Vector2i(Vector2(usable.size - window.size - decorations) * 0.5)
	window.position = usable.position + centered_offset + title_offset

func _physics_process(dt: float) -> void:
	context.player.dispatch(controls.command)
	workshop.advance(context, controls.repair_held, dt)
	if taxi:
		taxi.advance(dt)
	var active := context.player.focus as FlightCab
	if active and active.position.y < definition.ground_height - 20.0:
		_reset()
	elif context.player.focus is WalkingActor and context.player.focus.position.y < definition.ground_height - 20.0:
		_reset()

func _process(dt: float) -> void:
	var focus := context.player.focus
	if not is_instance_valid(focus):
		return
	var active := focus as FlightCab
	var walker := focus as WalkingActor
	var velocity: Vector3 = focus.get_control_velocity() if focus.has_method("get_control_velocity") else Vector3.ZERO
	var horizontal_limit := active.definition.max_horizontal_speed if active else (walker.walking_speed if walker else 5.0)
	var vertical_limit := (active.definition.max_climb_speed if velocity.y >= 0.0 else active.definition.max_fall_speed) if active else 13.0
	var horizontal_ahead := camera_tuning.horizontal_look_ahead if active else camera_tuning.on_foot_horizontal_look_ahead
	var vertical_ahead := camera_tuning.vertical_look_ahead if active else 0.0
	var displayed_position := focus.get_global_transform_interpolated().origin
	_platform_approach = PlatformCameraFraming.approach_weight(active, displayed_position, context.world, camera_tuning) if active else 0.0
	if active:
		horizontal_ahead = lerpf(horizontal_ahead, camera_tuning.on_foot_horizontal_look_ahead, _platform_approach)
		vertical_ahead *= 1.0 - _platform_approach
	var target := Vector3(velocity.x / horizontal_limit * horizontal_ahead, velocity.y / vertical_limit * vertical_ahead, 0.0)
	_look_ahead = _look_ahead.lerp(target, minf(1.0, dt * camera_tuning.look_ahead_speed))
	var follow_position := displayed_position
	if walker:
		# A short hop stays within the frame; a fall or a new floor moves it vertically.
		if walker.is_on_floor(): _foot_anchor_y = displayed_position.y
		else: _foot_anchor_y = clampf(_foot_anchor_y, displayed_position.y - camera_tuning.on_foot_jump_dead_zone, displayed_position.y + camera_tuning.on_foot_jump_dead_zone)
		follow_position.y = _foot_anchor_y + camera_tuning.on_foot_target_height
	var follow_speed := camera_tuning.on_foot_follow_speed if walker else camera_tuning.camera_follow_speed
	_camera_target = _camera_target.lerp(follow_position + _look_ahead, minf(1.0, dt * follow_speed))
	if walker:
		# Keep Ari visible even during a long fall; the walking actor has no fall-speed cap.
		var safety_margin := camera_tuning.on_foot_jump_dead_zone + 1.0
		_camera_target.y = clampf(_camera_target.y, displayed_position.y + camera_tuning.on_foot_target_height - safety_margin, displayed_position.y + camera_tuning.on_foot_target_height + safety_margin)
	var distance := lerpf(camera_tuning.camera_distance, camera_tuning.landing_distance, _platform_approach) if active else camera_tuning.on_foot_distance
	var blend := 1.0 - exp(-dt * camera_tuning.framing_speed)
	_camera_distance = lerpf(_camera_distance, distance, blend)
	_camera_angle = lerpf(_camera_angle, camera_tuning.on_foot_angle_degrees if walker else camera_tuning.camera_angle_degrees, blend)
	_position_camera()
	_hud_elapsed += dt
	if _hud_elapsed >= 0.1:
		controls._credits = context.campaign.credits
		controls.update_readout(velocity.length(), focus.position.y - definition.ground_height - 0.35, active.grounded if active else false)
		if active:
			controls.update_flight_status(active.fuel / active.definition.fuel_capacity, active.definition.airspace_enabled and active.airspace.ceiling_pressure(active.position.y, definition) > 0.01, active.refueling)
			controls.update_vehicle_status(active, context.campaign.credits, workshop.station, workshop.repairing)
		controls.update_city(focus.position, definition, active.highway_speed if active else 1.0, active.get_node("Headlights").lights_on if active else false)
		_hud_elapsed = 0.0

func _position_camera() -> void:
	var angle := deg_to_rad(_camera_angle)
	camera.rotation.x = -angle
	camera.position = _camera_target + Vector3(0, sin(angle), cos(angle)) * _camera_distance

func _reset() -> void:
	controls.clear_controls()
	if on_foot and context.player.focus == on_foot.actor:
		if on_foot.recover():
			_snap_camera()
		return
	var active := context.player.focus as FlightCab
	if active:
		if taxi:
			taxi.request_recovery(active.position.y < definition.ground_height - 20.0)
		else:
			if active != cab and living_world and living_world.initialized:
				var berth := living_world.recovery_berth(active)
				if berth.is_empty():
					return
				active.spawn_transform = Transform3D(Basis.IDENTITY, berth.position)
			active.reset_flight()
	# Snap only when the physics body has actually teleported.

func _snap_camera() -> void:
	_look_ahead = Vector3.ZERO
	var focus := context.player.focus
	if not is_instance_valid(focus):
		return
	_camera_target = focus.global_position
	_foot_anchor_y = focus.global_position.y
	if focus is WalkingActor:
		_camera_target.y += camera_tuning.on_foot_target_height
		_camera_distance = camera_tuning.on_foot_distance
		_camera_angle = camera_tuning.on_foot_angle_degrees
	else:
		_platform_approach = PlatformCameraFraming.approach_weight(focus, focus.global_position, context.world, camera_tuning) if focus is FlightCab else 0.0
		_camera_distance = lerpf(camera_tuning.camera_distance, camera_tuning.landing_distance, _platform_approach)
		_camera_angle = camera_tuning.camera_angle_degrees
	_position_camera()

func _focus_changed(previous: Node3D, current: Node3D) -> void:
	if current is WalkingActor: _foot_anchor_y = current.global_position.y
	if previous is FlightCab:
		if previous.flight_reset.is_connected(_snap_camera):
			previous.flight_reset.disconnect(_snap_camera)
		if previous.autopilot_changed.is_connected(controls.set_autopilot):
			previous.autopilot_changed.disconnect(controls.set_autopilot)
	controls.clear_controls()
	controls.set_control_mode(context.player.mode)
	controls.set_autopilot(current.airspace.returning if current is FlightCab else false)
	if current is FlightCab:
		current.flight_reset.connect(_snap_camera)
		current.autopilot_changed.connect(controls.set_autopilot)
	get_node("Atmosphere").set_focus(current)
	# Nearby entry/exit blends the framing; teleports and map arrival still snap.
	if not is_instance_valid(previous) or not is_instance_valid(current) or previous.global_position.distance_to(current.global_position) > 10:
		_snap_camera()

func capture_map_state() -> Dictionary:
	if on_foot:
		on_foot.capture_state()
	for vehicle in find_children("*", "RigidBody3D", true, false):
		if vehicle is FlightCab:
			vehicle.capture_state()
	return {}

func _exit_tree() -> void:
	if is_instance_valid(context):
		if context.narrative.notice.is_connected(_narrative_notice):
			context.narrative.notice.disconnect(_narrative_notice)
		if context.world.vehicle_registered.is_connected(_bind_vehicle):
			context.world.vehicle_registered.disconnect(_bind_vehicle)
		if context.player.focus_changed.is_connected(_focus_changed):
			context.player.focus_changed.disconnect(_focus_changed)
		if context.player.control_suspended.is_connected(controls.set_control_suspended):
			context.player.control_suspended.disconnect(controls.set_control_suspended)

func _bind_vehicle(vehicle: Node) -> void:
	if vehicle is FlightCab:
		vehicle.world_definition = definition
		vehicle.world_registry = context.world
		vehicle.free_refueling = not context.rides.enabled
		context.bind_vehicle(vehicle)
