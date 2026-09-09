extends Node2D
class_name ActorStateMachine

@export var player_path           : NodePath
@export var vehicle_group         : String   = "vehicles"
@export var camera_controller_path: NodePath
@export var enter_distance        : float    = 32.0

var player     : Node                     = null
var camera_ctrl: Camera2D                 = null

func _ready() -> void:
	set_process_unhandled_input(true)

func initialize(p: Node, cam: Node) -> void:
	player      = p
	camera_ctrl = cam
	_update_camera()

func _unhandled_input(event) -> void:
	if not player or not event.is_action_pressed("toggle_action"):
		return
# najpierw spróbuj dialog z quest_giver
	var closest_qg := _get_closest_quest_giver()
	if closest_qg:
		var dist_q = player.global_position.distance_to(closest_qg.global_position)
		if dist_q < enter_distance:
			closest_qg.interact()
			return
	var veh: Node = _get_closest_vehicle()
	var d: float = INF
	if veh:
		d = player.global_position.distance_to(veh.global_position)

	# foot → enter
	if not player.in_vehicle and veh and d < enter_distance:
		player.enter_vehicle(veh)
	# vehicle → exit
	elif player.in_vehicle:
		player.exit_vehicle()

	_update_camera()
	
#	var ui = get_tree().get_current_scene().get_node_or_null("MobileControls") as MobileControls
#	if ui:
		# show button if in_vehicle, hide otherwise
#		ui.set_interact_visible(player.in_vehicle)

func _update_camera() -> void:
	if not camera_ctrl or not player:
		return

	var target: Node = player
	if player.in_vehicle and player.vehicle != null:
		target = player.vehicle         # ← kamera śledzi aktualny pojazd

	camera_ctrl.set_follow_target(camera_ctrl.get_path_to(target))


func _get_closest_vehicle() -> Node:
	var closest: Node = null
	var min_d := INF
	for v: Node in get_tree().get_nodes_in_group(vehicle_group):
		if is_instance_valid(v) and v.is_inside_tree():
			var d = player.global_position.distance_to(v.global_position)
			if d < min_d:
				min_d = d
				closest = v
	return closest
	
func _get_closest_quest_giver() -> Node:
	var closest: Node = null
	var min_d := INF
	for q in get_tree().get_nodes_in_group("quest_givers"):
		if is_instance_valid(q) and q.is_inside_tree():
			var d = player.global_position.distance_to(q.global_position)
			if d < min_d:
				min_d = d
				closest = q
	return closest
