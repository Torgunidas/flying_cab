extends Camera2D

@export var follow_target: NodePath
@export var follow_speed: float = 5.0
@export var offset_y: float = 0.0
@export var follow_threshold: float = 0.0

func _ready():
	# W Godot 4 nie ma camera.current, jest make_current()
	make_current()
	# Zamiast smoothing_enabled (Godot 3.x)
	position_smoothing_enabled = false

func _process(delta: float) -> void:
	var target_node = get_node_or_null(follow_target)
	if not target_node:
		return
	
	# Prosty przykład: przesuwamy kamerę w osi Y, jeśli obiekt
	# jest powyżej zadanego progu 'follow_threshold'
	var cam_pos = global_position
	var target_y = target_node.global_position.y + offset_y

	# Jeśli pojazd/punkt docelowy jest „wyżej” (np. y < follow_threshold),
	# to płynnie przesuwamy kamerę w górę:
	if target_y <= follow_threshold:
		cam_pos.y = lerp(cam_pos.y, target_y, follow_speed * delta)
	else:
		# W innym wypadku kamera wraca do threshold (lub pozostaje tam)
		cam_pos.y = lerp(cam_pos.y, follow_threshold, follow_speed * delta)

	global_position = cam_pos
