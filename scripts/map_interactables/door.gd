# -------------------------------------------------------------------------
#  door.gd   –   drzwi z indywidualnym punktem spawnu
# -------------------------------------------------------------------------
extends Area2D
class_name Door

# ── 1) EXPORTY ───────────────────────────────────────────────────────────
@export_file("*.tscn") var target_level_path : String = ""   # plik poziomu
@export var           spawn_point_name : String = ""         # ← NOWOŚĆ
@export var sprite_path : NodePath = "AnimatedSprite2D"
@export var open_anim   : String   = "opening"
@export var close_anim  : String   = "closing"

const GAME_ROOT_PATH := "res://scenes/gameplay_root.tscn"

# ── 2) POLA ───────────────────────────────────────────────────────────────
var _sprite      : AnimatedSprite2D
var _player_near : bool = false
var _is_open     : bool = false

# ── 3) READY ──────────────────────────────────────────────────────────────
func _ready() -> void:
	_sprite = get_node(sprite_path)
	body_entered.connect(_on_body_entered)
	body_exited .connect(_on_body_exited)
	add_to_group("interactables"); add_to_group("door"); monitoring = true

# ── 4) DETEKCJA GRACZA ────────────────────────────────────────────────────
func _on_body_entered(body: Node) -> void:
	if body.is_in_group("player"):
		_player_near = true; _open_door()

func _on_body_exited(body: Node) -> void:
	if body.is_in_group("player"):
		_player_near = false; _close_door()

# ── 5) ANIMACJE ───────────────────────────────────────────────────────────
func _open_door():  if !_is_open: _sprite.play(open_anim);  _is_open = true
func _close_door(): if  _is_open: _sprite.play(close_anim); _is_open = false

# ── 6) INTERAKCJA ─────────────────────────────────────────────────────────
func interact() -> void:
	if not (_player_near and _is_open): return

	# ① sprawdź ścieżkę
	var level_ps := load(target_level_path.strip_edges()) as PackedScene
	if level_ps == null:
		push_error("Door: nie mogę załadować %s" % target_level_path); return

	# ② zapisz w GameState: poziom + punkt spawnu
	GameState.set_selected_level(level_ps)
	GameState.set_spawn_point_name(spawn_point_name.strip_edges())

	# ③ przełącz na GameplayRoot
	get_tree().change_scene_to_file(GAME_ROOT_PATH)
