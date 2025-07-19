extends Node2D
class_name GameRoot

signal level_loaded(level: Node2D, player: Node2D)

@export var initial_level      : PackedScene
@export var player_scene       : PackedScene
@export var environment_parent : NodePath = "environment"
@export var interior_zoom      : Vector2 = Vector2(5, 5)
@export var exterior_zoom      : Vector2 = Vector2(5, 5)

# ─────────────────────────────────────────────────────────────
# PRYWATNE
@onready var env_parent: Node = get_node_or_null(environment_parent)
var _current_level: Node2D = null

const DEBUG_VEH := true
func _d(msg: String) -> void:
	if DEBUG_VEH:
		print("[GameRoot]", msg)

func _ready() -> void:
	if env_parent == null:
		push_error("GameRoot: environment_parent '%s' nie znaleziony." % environment_parent)
		return

	var selected: PackedScene = GameState.get_selected_level()
	var level_to_load: PackedScene = selected if selected else initial_level

	if not level_to_load or not player_scene:
		push_error("GameRoot: level_to_load lub player_scene nieprzypisane.")
		return

	_load_level(level_to_load)


# Publiczny helper – jeśli gdziekolwiek indziej chcesz zmienić level:
func change_level(new_level: PackedScene) -> void:
	if new_level == null:
		push_warning("GameRoot.change_level: new_level is null")
		return
	_load_level(new_level)


func _save_previous_world_vehicle() -> void:
	if _current_level and VehPers.current_car and VehPers.is_world_level(_current_level):
		_d("Saving vehicle for world '%s' pos=%s" %
			[_current_level.name, str(VehPers.current_car.global_position)])
		VehPers.save_car_state(_current_level)
	else:
		# Debug przyczyny pominięcia
		if DEBUG_VEH:
			_d("Skip save: lvl=%s car=%s is_world=%s" % [
				(_current_level and _current_level.name) if _current_level else "null",
				str(VehPers.current_car),
				_current_level and VehPers.is_world_level(_current_level)
			])


func _load_level(level_scene: PackedScene) -> void:
	# 0) ZAPIS POJAZDU POPRZEDNIEGO ŚWIATA (przed usunięciem sceny)
	_save_previous_world_vehicle()

	# 1) WYCZYŚĆ poprzedni level (bez dotykania playera – bo i tak respawnujemy)
	if env_parent:
		for c in env_parent.get_children():
			# Jeżeli chcesz usuwać tylko poprzedni level a zostawić np. UI w env_parent,
			# można dodać tu filtr po grupie. Na razie usuwamy wszystko.
			c.queue_free()

	# 2) INSTANCJUJ nowy level
	var level_inst := level_scene.instantiate() as Node2D
	env_parent.add_child(level_inst)
	_current_level = level_inst

	_d("Loaded level root name = %s" % level_inst.name)
	_d("is_world = %s" % VehPers.is_world_level(level_inst))
	VehPers._dbg("after_level_added")

	# 2.5) ODTWÓRZ POJAZD jeśli to świat
	if VehPers.is_world_level(level_inst):
		var veh_parent: Node = level_inst.get_node_or_null("Vehicles") if level_inst.has_node("Vehicles") else level_inst
		if VehPers.has_state(level_inst.name):
			_d("Restoring vehicle for %s" % level_inst.name)
			var restored := VehPers.restore_car_state(level_inst, veh_parent)
			if restored:
				# (opcjonalnie) oznacz przywrócony pojazd, żeby inne inicjalizacje go nie ruszały
				restored.add_to_group("restored_vehicle")
				_d("Vehicle restored at %s" % str(restored.global_position))
		else:
			_d("No saved vehicle state for %s" % level_inst.name)

	# 3) SPAWN GRACZA ──────────────────────────────────────────
	var spawn_name := GameState.get_spawn_point_name()
	var spawn: Node2D = null

	if spawn_name != "":
		spawn = level_inst.find_child(spawn_name, true, false) as Node2D

	if spawn == null:
		spawn = level_inst.get_node_or_null("PlayerSpawn") as Node2D

	var player := player_scene.instantiate() as Node2D
	env_parent.add_child(player)
	player.global_position = spawn.global_position if spawn else Vector2.ZERO
	if spawn:
		spawn.queue_free()

	# 4) Kamera
	var cam := get_node_or_null("CameraController") as Camera2D
	if cam:
		if cam.has_method("set_follow_target"):
			cam.set_follow_target(cam.get_path_to(player))
		if level_inst.is_in_group("interiors"):
			cam.zoom = interior_zoom
		else:
			cam.zoom = exterior_zoom
	else:
		push_warning("GameRoot: CameraController nie znaleziony.")

	# 5) FSM (jeśli istnieje)
	var asm := get_node_or_null("ActorStateMachine")
	if asm and asm.has_method("initialize"):
		asm.initialize(player, cam)
	else:
		push_warning("GameRoot: ActorStateMachine lub metoda initialize brak – pomijam.")

	# 6) Sygnał dla UI / overlay
	emit_signal("level_loaded", level_inst, player)


func restart_level() -> void:
	if not initial_level:
		push_warning("GameRoot.restart_level: initial_level null")
		return
	_load_level(initial_level)
