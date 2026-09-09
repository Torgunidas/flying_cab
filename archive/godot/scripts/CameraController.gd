extends Camera2D
class_name CameraController

# The node the camera will follow
@export var follow_target: NodePath
# How quickly to interpolate towards target
@export var follow_speed: float = 5.0

func _ready() -> void:
	# Make this the active camera
	make_current()
	# Disable built-in smoothing (Godot 4)
	position_smoothing_enabled = false

func set_follow_target(path: NodePath) -> void:
	follow_target = path

func _process(delta: float) -> void:
	var tgt = get_node_or_null(follow_target)
	if not tgt:
		return
	# Smoothly interpolate camera position towards the target
	global_position = global_position.lerp(tgt.global_position, follow_speed * delta)
