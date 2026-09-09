extends Area2D
class_name QuestGiver

# Możesz podać DialogBook (wieloetapowy) lub pojedynczy DialogData
@export var dialog_resource : Resource
@export var portrait_texture : Texture2D = null

signal mission_start(mission_id)

@onready var _anim_player: AnimatedSprite2D = get_node_or_null("AnimationPlayer") as AnimatedSprite2D

func _ready() -> void:
	add_to_group("interactables")

func interact() -> void:
	var dm: DialogManager = get_tree().get_current_scene() \
		.get_node_or_null("DialogManager") as DialogManager
	if dm == null:
		push_warning("DialogManager not found in current scene")
		return
	dm.call_deferred("start_dialog", dialog_resource, self)

# Odtwarzaj animację anim_name tyle razy, ile wskazuje loops
# QuestGiver.gd

func play_dialog_animation(anim_name: String, loops: int = 1) -> void:
	if _anim_player == null:
		return

	print("▶ play_dialog_animation:", anim_name, "loops=", loops)
	_anim_player.play(anim_name)

	if loops > 1:
		_anim_player.connect(
			"animation_finished",
			Callable(self, "_repeat_animation").bind(anim_name, loops - 1),
			CONNECT_ONE_SHOT
		)



func _repeat_animation(anim_name: String, remaining: int) -> void:
	print("↻ _repeat_animation:", anim_name, "remaining=", remaining)

	if remaining <= 0:
		return

	_anim_player.play(anim_name)

	if remaining > 1:
		_anim_player.connect(
			"animation_finished",
			Callable(self, "_repeat_animation").bind(anim_name, remaining - 1),
			CONNECT_ONE_SHOT
		)
