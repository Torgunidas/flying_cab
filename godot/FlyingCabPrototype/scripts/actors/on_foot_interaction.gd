class_name OnFootInteraction
extends Node
## Physics queries and transfers of control; the same actor survives every ride.
signal checkpoint_requested
const ARI = preload("res://scenes/people/ari.tscn")
const ENTRY_RANGE := 1.35
const MAX_PARKED_SPEED := 0.25
var context: RuntimeContext
var controls: FlightControls
var actor: WalkingActor
var target: FlightCab
var _pending := false
var _refresh := 0.0
var _cooldown := 0.0

func _ready() -> void:
	actor = ARI.instantiate()
	add_child(actor)
	actor.set_active(false)
	controls.interaction_requested.connect(request_interaction)
	context.player.control_suspended.connect(_suspended)

func _exit_tree() -> void:
	if context.player.control_suspended.is_connected(_suspended):
		context.player.control_suspended.disconnect(_suspended)

func _suspended(_value: bool) -> void:
	_pending = false

func request_interaction() -> void:
	if not context.player.is_suspended() and _cooldown <= 0:
		_pending = true

func _physics_process(dt: float) -> void:
	if not actor.is_inside_tree():
		return
	_cooldown = maxf(0, _cooldown - dt)
	if context.player.is_suspended():
		_pending = false
		return
	if _pending:
		_pending = false
		if context.player.focus is FlightCab:
			try_exit(context.player.focus)
		elif context.player.focus == actor and is_instance_valid(target):
			try_enter(target)
		_cooldown = 0.25
		_refresh = 0
	_refresh -= dt
	if _refresh <= 0:
		refresh_target()
		_refresh = 0.1

func parked(cab: FlightCab) -> bool:
	return is_instance_valid(cab) and cab.grounded and cab.linear_velocity.length() <= MAX_PARKED_SPEED and cab.command.is_zero_approx() and cab.applied_command.length() < 0.01 and not cab.airspace.returning and not cab.resetting

func _passengers_moving(cab: FlightCab) -> bool:
	var trip := context.rides.active
	return not trip.is_empty() and trip.vehicle == String(cab.entity_id) and trip.phase in ["boarding", "alighting"]

func _capsule() -> CapsuleShape3D:
	return actor.get_node("Collision").shape

func _exclusions() -> Array[RID]:
	return [actor.get_rid()]

func is_clear(at: Vector3) -> bool:
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = _capsule()
	query.transform = Transform3D(Basis.IDENTITY, at)
	query.collision_mask = 1
	query.exclude = _exclusions()
	query.margin = 0.01
	return actor.get_world_3d().direct_space_state.intersect_shape(query, 1).is_empty()

func exit_points(cab: FlightCab) -> Array[Vector3]:
	var points: Array[Vector3] = []
	var shape := _capsule()
	var collider := cab.get_node("Collision") as CollisionShape3D
	var box := collider.shape as BoxShape3D
	if box == null:
		return points
	var center := collider.global_position
	var bottom := center.y - box.size.y * 0.5
	for side: float in [1.0, -1.0]:
		var x := center.x + side * (box.size.x * 0.5 + shape.radius + 0.22)
		# Models may author their own side markers; bounds cover every catalog car.
		var marker := cab.get_node_or_null("Visual/EntryRight" if side > 0 else "Visual/EntryLeft") as Node3D
		if marker:
			x = marker.global_position.x
		var ray := PhysicsRayQueryParameters3D.create(Vector3(x, bottom + 0.5, 0), Vector3(x, bottom - 1.25, 0), 1, _exclusions())
		var hit := actor.get_world_3d().direct_space_state.intersect_ray(ray)
		if hit.is_empty() or hit.normal.y < 0.7 or not hit.collider is StaticBody3D:
			continue
		var at: Vector3 = hit.position + Vector3.UP * (shape.height * 0.5 + 0.04)
		if is_clear(at):
			points.append(at)
	return points

func refresh_target() -> void:
	target = null
	var label := ""
	var cab := context.player.focus as FlightCab
	if cab:
		if parked(cab) and not _passengers_moving(cab) and not exit_points(cab).is_empty():
			target = cab
			label = "WYSIĄDŹ / Q"
	elif context.player.focus == actor and actor.is_on_floor():
		var nearest := INF
		for vehicle: FlightCab in context.world.vehicles:
			var distance := entry_distance(vehicle)
			if distance < nearest:
				nearest = distance
				target = vehicle
		if target:
			label = "WSIĄDŹ / Q"
	controls.set_interaction_label(label)

func entry_distance(cab: FlightCab) -> float:
	if not parked(cab) or cab.get_driver_id() != &"" or cab.state.owner_id not in [&"", context.player.actor_id]:
		return INF
	var nearest := INF
	for at in exit_points(cab):
		var distance := actor.global_position.distance_to(at)
		if distance > ENTRY_RANGE or absf(actor.global_position.y - at.y) > 0.4:
			continue
		var ray := PhysicsRayQueryParameters3D.create(actor.global_position, at, 1, _exclusions())
		if actor.get_world_3d().direct_space_state.intersect_ray(ray).is_empty():
			nearest = minf(nearest, distance)
	return nearest

func try_exit(cab: FlightCab) -> bool:
	if context.player.is_suspended() or context.player.focus != cab or not parked(cab) or _passengers_moving(cab):
		return false
	var points := exit_points(cab)
	if points.is_empty():
		return false
	actor.set_active(true)
	actor.place(points[0])
	actor.facing = signf(points[0].x - cab.global_position.x)
	actor.get_node("Visual").rotation.y = PI * 0.5 * actor.facing
	context.player_state.exit_position = points[0]
	context.player_state.vehicle_id = cab.entity_id
	context.player.take_control(actor, &"on_foot")
	cab.capture_state()
	capture_state()
	checkpoint_requested.emit()
	return true

func try_enter(cab: FlightCab) -> bool:
	if context.player.is_suspended() or context.player.focus != actor or not actor.is_on_floor() or not is_finite(entry_distance(cab)):
		return false
	context.player.take_control(cab, &"flight")
	actor.set_active(false)
	capture_state()
	checkpoint_requested.emit()
	return true

func capture_state() -> void:
	context.capture_player_state()

func restore_player() -> void:
	var saved := context.player_state
	if saved.map_id != context.current_map:
		return
	if saved.mode == "on_foot":
		actor.set_active(true)
		actor.place(saved.position, saved.velocity)
		actor.facing = saved.facing
		actor.get_node("Visual").rotation.y = PI * 0.5 * actor.facing
		context.player.take_control(actor, &"on_foot")
	else:
		for vehicle: FlightCab in context.world.vehicles:
			if vehicle.entity_id == saved.vehicle_id:
				context.player.take_control(vehicle, &"flight")
				break

func recover() -> bool:
	# Prototype recovery, separate from car towing and future health/death rules.
	if context.player.focus != actor or context.player.is_suspended() or not is_clear(context.player_state.exit_position):
		return false
	actor.place(context.player_state.exit_position)
	controls.clear_controls()
	capture_state()
	checkpoint_requested.emit()
	return true
