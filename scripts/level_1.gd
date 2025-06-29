extends Node2D

@export var gravity_force: float = 50.0
@export var air_resistance: float = 0.05
@export var player_spawn_position: Vector2 = Vector2(1400, -2271)


func _ready() -> void:
	# 1) Dla grupy "passangers_left" – flip_h = true
	for passenger in get_tree().get_nodes_in_group("passangers_left"):
		if passenger.has_node("AnimatedSprite2D"):
			passenger.get_node("AnimatedSprite2D").flip_h = true

	# 2) Dla grupy "passangers_right" – flip_h = false (domyślnie)
	for passenger in get_tree().get_nodes_in_group("passangers_right"):
		if passenger.has_node("AnimatedSprite2D"):
			passenger.get_node("AnimatedSprite2D").flip_h = false
			
			# 2) ►►  ODTWÓRZ ZAPISANE POZYCJE POJAZDÓW  ◄◄
	for car in get_tree().get_nodes_in_group("vehicles"):
		if not (car is Car):
			continue
		var saved := GameState.get_saved_transform(name, car.instance_id)
		if saved.size() > 0:
			car.global_position = saved["pos"]
			car.rotation       = saved["rot"]
