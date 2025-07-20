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
	# 1) Wyczyść poprzednie środowisko
	if env_parent:
		for c in env_parent.get_children():
			c.queue_free()

	# 2) Instancja nowego levelu
	var level_inst := level_scene.instantiate() as Node2D
	env_parent.add_child(level_inst)

	_d("Loaded level root name = %s" % level_inst.name)
	var is_world := VehPers.is_world_level(level_inst)
	_d("is_world = %s" % is_world)
	VehPers._dbg("after_level_added")

	# 3) (ŚWIAT) Usuń fabryczne auta ZANIM zrobimy restore
	if is_world and VehPers.has_state(level_inst.name):
		var removed := _remove_default_spawn_cars(level_inst)
		_d("Default spawn cars removed count = %d" % removed)

		# 4) Przywróć zapisany pojazd
		var veh_parent: Node = level_inst.get_node_or_null("Vehicles") if level_inst.has_node("Vehicles") else level_inst
		var restored := VehPers.restore_car_state(level_inst, veh_parent)
		if restored:
			restored.add_to_group("player_owned_vehicle")
			restored.set_meta("player_owned", true)
			restored.set_meta("restored_from_save", true)
			_d("Restored vehicle at %s" % str(restored.global_position))
	else:
		if is_world:
			_d("No saved vehicle state for %s" % level_inst.name)

	# 5) Spawn gracza
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

	# 6) Kamera
	var cam := get_node_or_null("CameraController") as Camera2D
	if cam:
		if cam.has_method("set_follow_target"):
			cam.set_follow_target(cam.get_path_to(player))
		cam.zoom = interior_zoom if level_inst.is_in_group("interiors") else exterior_zoom

	# 7) FSM
	var asm := get_node_or_null("ActorStateMachine")
	if asm and asm.has_method("initialize"):
		asm.initialize(player, cam)

	# 8) Sygnał
	emit_signal("level_loaded", level_inst, player)

func _remove_default_spawn_cars(level_inst: Node) -> int:
	var count := 0

	# Strategia:
	# 1. Jeśli jest node "Vehicles" – przeszukaj tylko jego poddrzewo rekurencyjnie
	# 2. W przeciwnym razie – przeszukaj cały level rekurencyjnie
	var roots: Array = []
	if level_inst.has_node("Vehicles"):
		roots.append(level_inst.get_node("Vehicles"))
	else:
		roots.append(level_inst)

	for r in roots:
		# rekurencja DFS
		var stack: Array = [r]
		while stack.size() > 0:
			var n: Node = stack.pop_back()
			# enqueue children
			for child in n.get_children():
				stack.append(child)

			# Kryterium: Car + grupa default_spawn_car
			if n is Car and n.is_in_group("default_spawn_car"):
				_d("Removing default_spawn_car node: %s (path=%s)" % [n.name, n.get_path()])
				n.queue_free()
				count += 1

	return count



func restart_level() -> void:
	if not initial_level:
		push_warning("GameRoot.restart_level: initial_level null")
		return
	_load_level(initial_level)
