extends Node2D

@export var gravity_force: float = 50.0
@export var air_resistance: float = 0.05
@export var player_spawn_position: Vector2 = Vector2(1400, -2271)


func _ready() -> void:
	# 1) Dla grupy "passangers_left" – flip_h = true
	#1) Dla grupy "passangers_left" – flip_h = true
	for passenger in get_tree().get_nodes_in_group("passangers_left"):
				if passenger.has_node("AnimatedSprite2D"):
						passenger.get_node("AnimatedSprite2D").flip_h = true

	# 2) Dla grupy "passangers_right" – flip_h = false (domyślnie)
	for passenger in get_tree().get_nodes_in_group("passangers_right"):
				if passenger.has_node("AnimatedSprite2D"):
						passenger.get_node("AnimatedSprite2D").flip_h = false

	if get_name() == "HiCity":
				GameState.start_maya_timer()
				QuestSys.activate("1_1_talk_to_maya")
