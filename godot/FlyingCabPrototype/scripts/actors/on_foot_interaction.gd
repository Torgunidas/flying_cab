class_name OnFootInteraction
extends Node
## Physics queries and transfers of control; the same actor survives every ride.
signal checkpoint_requested
const ARI = preload("res://scenes/people/ari.tscn")
const ENTRY_RANGE := 0.85
const SAFE_FALL_SPEED := 8.0
const FALL_DAMAGE_FACTOR := 1.5
const MAX_PARKED_SPEED := 0.25
var context: RuntimeContext
var controls: FlightControls
var actor: WalkingActor
var target: FlightCab
var conversation_target: NarrativeNpc
var _pending := false
var _refresh := 0.0
var _cooldown := 0.0

func _ready() -> void:
	actor = ARI.instantiate()
	add_child(actor)
	actor.set_active(false)
	actor.landed.connect(_landed)
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
		elif context.player.focus == actor and is_instance_valid(conversation_target):
			conversation_target.interact(context, actor)
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

func _landed(impact_speed: float) -> void:
	if context.player.focus != actor:
		return
	var excess := maxf(0, impact_speed - SAFE_FALL_SPEED)
	context.damage_player(excess * excess * FALL_DAMAGE_FACTOR)

func _capsule() -> CapsuleShape3D:
	return actor.get_node("Collision").shape

func _exclusions() -> Array[RID]:
	return [actor.get_rid()]

func is_clear(at: Vector3) -> bool:
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = _capsule()
	query.transform = Transform3D(Basis.IDENTITY, at)
	query.collision_mask = WorldLayers.GEOMETRY
	query.exclude = _exclusions()
	query.margin = 0.01
	return actor.get_world_3d().direct_space_state.intersect_shape(query, 1).is_empty()

func entry_point(cab: FlightCab) -> Vector3:
	var collider := cab.get_node("Collision") as CollisionShape3D
	var box := collider.shape as BoxShape3D
	var visual := cab.get_node("Visual") as Node3D
	var door := visual.to_global(Vector3(cab.definition.driver_door_x, 0, 0))
	return Vector3(door.x, collider.global_position.y - box.size.y * 0.5 + _capsule().height * 0.5 + 0.04, WorldLayers.PEDESTRIAN_Z)

func exit_points(cab: FlightCab) -> Array[Vector3]:
	var points: Array[Vector3] = []
	var at := entry_point(cab)
	var collider := cab.get_node("Collision") as CollisionShape3D
	var box := collider.shape as BoxShape3D
	var side := 1.0 if cab.get_node("Visual").scale.x > 0 else -1.0
	var candidates: Array[Vector3] = [at]
	for direction in [side, -side]:
		candidates.append(Vector3(collider.global_position.x + direction * (box.size.x * 0.5 + _capsule().radius + 0.22), at.y, WorldLayers.PEDESTRIAN_Z))
	# Prefer the cabin. Other clear exits are fallbacks, never entry hotspots.
	for point in candidates:
		if is_clear(point):
			points.append(point)
	return points

func _supported(at: Vector3) -> bool:
	var bottom := at - Vector3.UP * (_capsule().height * 0.5 - 0.02)
	var ray := PhysicsRayQueryParameters3D.create(bottom, bottom - Vector3.UP * 0.18, WorldLayers.GEOMETRY, _exclusions())
	var hit := actor.get_world_3d().direct_space_state.intersect_ray(ray)
	return not hit.is_empty() and hit.normal.y > 0.7

func refresh_target() -> void:
	target = null
	conversation_target = null
	var label := ""
	var cab := context.player.focus as FlightCab
	if cab:
		if not exit_points(cab).is_empty():
			target = cab
			label = "WYSIĄDŹ / Q"
	elif context.player.focus == actor and actor.is_on_floor():
		var npc_distance := INF
		for candidate in get_tree().get_nodes_in_group("narrative_npc"):
			if candidate is NarrativeNpc and candidate.available(context, actor):
				var distance: float = candidate.global_position.distance_to(actor.global_position)
				if distance < npc_distance:
					npc_distance = distance
					conversation_target = candidate
		if conversation_target:
			controls.set_interaction_label("ROZMOWA / Q")
			return
		var nearest := INF
		for vehicle: FlightCab in context.world.vehicles:
			var distance := entry_distance(vehicle)
			if distance < nearest:
				nearest = distance
				target = vehicle
		if target:
			label = "PRZEJMIJ / Q" if target.state.owner_id not in [&"", context.player.actor_id] else "WSIĄDŹ / Q"
	controls.set_interaction_label(label)

func entry_distance(cab: FlightCab) -> float:
	if not parked(cab) or cab.get_driver_id() != &"":
		return INF
	var population := context.world.living_world
	if cab.state.owner_id not in [&"", context.player.actor_id] and not (is_instance_valid(population) and population.can_take_vehicle(cab)):
		return INF
	var at := entry_point(cab)
	var distance := actor.global_position.distance_to(at)
	var reach := minf(ENTRY_RANGE, cab.definition.collision_size.x * 0.35)
	if distance > reach or absf(actor.global_position.y - at.y) > 0.4 or not is_clear(at):
		return INF
	var ray := PhysicsRayQueryParameters3D.create(actor.global_position, at, WorldLayers.GEOMETRY, _exclusions())
	return distance if actor.get_world_3d().direct_space_state.intersect_ray(ray).is_empty() else INF

func try_exit(cab: FlightCab) -> bool:
	if context.player.is_suspended() or context.player.focus != cab:
		return false
	var points := exit_points(cab)
	if points.is_empty():
		return false
	actor.set_active(true)
	actor.place(points[0], cab.linear_velocity)
	actor.facing = 1.0 if cab.get_node("Visual").scale.x > 0 else -1.0
	actor.get_node("Visual").rotation.y = PI * 0.5 * actor.facing
	if _supported(points[0]):
		context.player_state.exit_position = points[0]
	context.player_state.vehicle_id = cab.entity_id
	context.player.take_control(actor, &"on_foot")
	cab.capture_state()
	capture_state()
	context.narrative.record_event("vehicle_exited", {"vehicle": String(cab.entity_id), "target": String(cab.entity_id)})
	checkpoint_requested.emit()
	return true

func try_enter(cab: FlightCab) -> bool:
	if context.player.is_suspended() or context.player.focus != actor or not actor.is_on_floor() or not is_finite(entry_distance(cab)):
		return false
	if is_instance_valid(context.world.living_world):
		context.world.living_world.take_vehicle(cab)
	context.player.take_control(cab, &"flight")
	actor.set_active(false)
	capture_state()
	context.narrative.record_event("vehicle_entered", {"vehicle": String(cab.entity_id), "target": String(cab.entity_id)})
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
	# Recovery uses the last supported exit, never the airborne ejection point.
	if context.player.focus != actor or context.player.is_suspended() or not actor.is_on_floor() or not _supported(actor.global_position) or not is_clear(context.player_state.exit_position) or not _supported(context.player_state.exit_position):
		return false
	actor.place(context.player_state.exit_position)
	controls.clear_controls()
	capture_state()
	checkpoint_requested.emit()
	return true
