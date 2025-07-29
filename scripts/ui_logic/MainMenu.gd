extends Control

@export var level1_scene    : PackedScene
@export var level2_scene    : PackedScene
@export var level3_scene    : PackedScene
@export var game_root_scene : PackedScene

# ① Złap node AudioStreamPlayer
@onready var menu_music := $MenuMusic

func _ready() -> void:
	# Hard reset za każdym razem, gdy pojawia się menu
	if GameState and GameState.has_method("reset_game"):
		GameState.reset_game()

	if not menu_music.playing:
		menu_music.play()




func _on_Quit_pressed() -> void:
	if menu_music.playing:
		menu_music.stop()
	get_tree().quit()
	
func _on_LoadGame_pressed() -> void:
		get_tree().change_scene_to_file("res://scenes/ui_scenes/SaveMenu.tscn")

func _on_Continue_pressed() -> void:
		var saves := SaveMgr.list_saves()
		if saves.size() > 0:
				SaveMgr.load(saves.back().slot)


func _on_level_1_button_pressed() -> void:
	if menu_music.playing:
		menu_music.stop()
	GameState.set_selected_level(level1_scene)
	get_tree().change_scene_to_packed(game_root_scene)


func _on_level_2_button_pressed() -> void:
	if menu_music.playing:
		menu_music.stop()
	GameState.set_selected_level(level2_scene)
	get_tree().change_scene_to_packed(game_root_scene)
	
func _on_level_3_button_pressed() -> void:
	if menu_music.playing:
		menu_music.stop()
	GameState.set_selected_level(level3_scene)
	get_tree().change_scene_to_packed(game_root_scene)
