extends Node2D
class_name QuestPointer

var target: Node2D
@onready var pointer: Node2D = $"Pointer"

func _process(_delta: float) -> void:
		if target == null or !is_instance_valid(target):
				queue_free(); return
		var player = get_tree().get_first_node_in_group("player")
		if player == null:
				return
		var pos = player.global_position
		if player.in_vehicle and player.vehicle:
				if is_instance_valid(player.vehicle):
						pos = player.vehicle.global_position
		global_position = pos + Vector2(0, -48)
		if is_instance_valid(pointer):
				pointer.look_at(target.global_position)
