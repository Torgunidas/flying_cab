extends Node
class_name PauseSys       


func set_paused(on: bool) -> void:
	get_tree().paused = on

func toggle_pause() -> void:
	set_paused(!get_tree().paused)

func is_paused() -> bool:
	return get_tree().paused
