extends Node2D
class_name GameRoot

signal level_loaded(level: Node2D, player: Node2D)   # ← zostaw, jeśli Mapa z niego korzysta

@export var initial_level    : PackedScene
@export var player_scene     : PackedScene
@export var environment_parent: NodePath = "environment"
@export var interior_zoom: Vector2 = Vector2(5, 5)
@export var exterior_zoom: Vector2 = Vector2(5, 5)

@onready var env_parent := get_node_or_null(environment_parent)
func _ready() -> void:
	# … twoje dotychczasowe kontrole env_parent …

	var selected := GameState.get_selected_level()      # ← nowa linia
	var level_to_load := selected if selected else initial_level

	if not level_to_load or not player_scene:
		push_error("GameRoot: level_to_load or player_scene not assigned.")
		return

	_load_level(level_to_load)                          # ← zamiast initial_level


func _load_level(level_scene: PackedScene) -> void:
	# 1) czyścimy poprzedni level
	for c in env_parent.get_children():
		c.queue_free()

	# 2) instancjonujemy nowy
	var level_inst := level_scene.instantiate()
	env_parent.add_child(level_inst)

# 3) SPWAN GRACZA  ───────────────────────────────────────────────────────
	var spawn_name := GameState.get_spawn_point_name()
	var spawn: Node2D = null

	if spawn_name != "":                         # ① szukamy REKURENCYJNIE
		# find_child(name, recursive=true, owned=false) zwraca pierwszy pasujący węzeł
		spawn = level_inst.find_child(spawn_name, true, false) as Node2D

# ② fallback do klasycznego "PlayerSpawn", jeśli nie znaleziono lub pole było puste
	if spawn == null:
		spawn = level_inst.get_node_or_null("PlayerSpawn") as Node2D

# ③ instancjonujemy gracza
	var player := player_scene.instantiate()
	env_parent.add_child(player)
	player.global_position = spawn.global_position if spawn else Vector2.ZERO
	if spawn: spawn.queue_free()   # (opcjonalnie) usuwamy marker po użyciu


	# 4) ustawiamy kamerę
	var cam := get_node_or_null("CameraController") as Camera2D
	if cam:
				cam.set_follow_target(cam.get_path_to(player))
				if level_inst.is_in_group("interiors"):
						cam.zoom = interior_zoom
				else:
						cam.zoom = exterior_zoom

	# 5) FSM
	var asm := get_node_or_null("ActorStateMachine")
	if asm and cam and asm.has_method("initialize"):
		asm.initialize(player, cam)
	else:
		push_warning("GameRoot: ActorStateMachine or CameraController not found; skipping initialization.")

	# 6) daj znać MapOverlay (jeśli podłączona)
	emit_signal("level_loaded", level_inst, player)

func restart_level() -> void:
	_load_level(initial_level)
