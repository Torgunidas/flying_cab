# CarNPC.gd – AI‑sterowany samochód, po którym można chodzić
extends "res://scripts/car.gd"
class_name CarNPC

# ───────────────────────── EXPORTY ───────────────────────────
@export var waypoints_path       : NodePath            # kontener z Node2D ze ścieżką
@export var waypoint_radius      : float   = 16.0
@export var cruise_speed         : float   = 220.0
@export var takeover_distance    : float   = 128.0
@export var loop                 : bool    = true
@export var driver_eject_scene   : PackedScene = preload("res://scenes/npc_passanger_male.tscn")

@export var player_layer_bit     : int = 0      # warstwa gracza (np. Layer‑1)
@export var platform_layer_bit   : int = 5      # warstwa platformy dachu
@export var add_roof_platform    : bool = true
@export var roof_size            : Vector2 = Vector2(64, 20)
@export var roof_offset          : Vector2 = Vector2(0, -10)

@export var interact_offset      : Vector2 = Vector2(32, 0)

# ───────────────────────── ZMIENNE ───────────────────────────
var _waypoints     : Array[Vector2] = []
var _current_wp    : int = 0
var _ai_enabled    : bool = true
var _player        : Node2D
var _eject_spawned : bool = false
var _roof_body     : StaticBody2D

# ───────────────────────── READY ───────────────────────────
func _ready() -> void:
	super._ready()
	add_to_group("vehicles")
	add_to_group("interactables")

	_gather_waypoints()

	set_controls_enabled(true)   # ← AI korzysta z fizyki auta
	set_player_input_enabled(false)
	set_fuel_enabled(false)      # NPC vehicles ignore fuel until taken over 
	if add_roof_platform:
		_spawn_roof_platform()
	_spawn_interact_marker()

	_player = get_tree().get_first_node_in_group("player")


# ────────────────────── PLATFORM AND COLLISION ──────────────────────
# CarNPC uses default Car collision. Roof is separate platform.
func _spawn_roof_platform() -> void:
	if is_instance_valid(_roof_body):
		_roof_body.queue_free()
	var plat := StaticBody2D.new()
	_roof_body = plat
	plat.name = "RoofPlatform"
	add_child(plat)
	plat.position = roof_offset
	var shape := CollisionShape2D.new()
	plat.add_child(shape)
	var rect := RectangleShape2D.new()
	rect.size = roof_size
	shape.shape = rect
	plat.collision_layer = 1 << platform_layer_bit
	plat.collision_mask  = 1 << player_layer_bit

func _remove_roof_platform() -> void:
	if is_instance_valid(_roof_body):
		_roof_body.queue_free()
		_roof_body = null

# ───────────────────────── PHYSICS ───────────────────────────
func _physics_process(delta: float) -> void:
	if _ai_enabled:
		_drive_ai(delta)     # → ustawia „wirtualne klawisze”
	super._physics_process(delta)  # ⇐ wszystką fizykę robi car.gd


func _drive_ai(delta: float) -> void:
	if _waypoints.is_empty():
		ai_input  = Vector2.ZERO
		ai_thrust = false
		ai_hover  = false          # ← na wszelki wypadek zawsze OFF
		return

	var target:  Vector2 = _waypoints[_current_wp]
	var diff:    Vector2 = target - global_position

	# --- waypoint switching ---
	if diff.length() < waypoint_radius:
		_current_wp = (_current_wp + 1) % _waypoints.size() if loop else min(_current_wp + 1, _waypoints.size() - 1)
		target = _waypoints[_current_wp]
		diff   = target - global_position

	# --- poziom (X) ---
	ai_input.x = clamp(diff.x / cruise_speed, -1.0, 1.0)

	# --- pion (Y) ---
	const ALT_TOL: float = 12.0        # martwa strefa
	var dy: float = diff.y             # >0 punkt niżej

	ai_thrust = (dy < -ALT_TOL)        # włącz thrust tylko gdy musimy wznieść się znacząco
	ai_hover  = false                  # całkowicie wyłączamy hover

	# delikatny pitch (opcjonalnie animacyjny)
	ai_input.y = clamp(-velocity.y / max_climb_speed, -0.8, 0.8)



# ───────────── SPAWN NPC DRIVER ─────────────────
func _spawn_ejected_driver() -> void:
	if _eject_spawned or driver_eject_scene == null:
		return
	_eject_spawned = true
	# Instantiate passenger driver above vehicle roof to allow falling
	var npc = driver_eject_scene.instantiate()
	get_parent().add_child(npc)
	# spawn slightly above roof so physics fall is visible
	npc.global_position = global_position + roof_offset + Vector2(0, -10)
	# trigger fall state via deferred call to ensure physics is ready
	if npc.has_method("start_fall"):
		npc.call_deferred("start_fall")
	else:
		# fallback animation if no physics fallback
		if npc.has_variable("_sprite") and npc._sprite:
			npc._sprite.animation = "fall"
			npc._sprite.play()
		if npc.has_variable("velocity"):
			npc.velocity = Vector2(randf_range(-80, 80), randf_range(-80, 80))
	# NPCPassenger script should handle death on floor via its body_entered or floor detection
# ───────────── INTERACTION TOGGLE ─────────────────
func interact() -> void:
	var pc := get_tree().get_first_node_in_group("player")
	if pc == null:
		return

	# — WYJŚCIE GRACZA —
	if pc.in_vehicle and pc.vehicle == self:
		# Twój istniejący kod wysiadania
		return
	# — WEJŚCIE GRACZA —
	if not pc.in_vehicle and pc.vehicle != self:
		# 1) Przełączamy warstwy CarNPC na „Car” (bit 0 tylko):
		self.collision_layer    = 1        # tylko layer 1
		self.collision_mask     = 1        # tylko mask 1
		self.light_mask         = 1        # tylko światła z layer 1
		self.visibility_layer   = 1        # widoczność tylko layer 1

		# (opcjonalnie, jeżeli masz osobne właściwości):
		# self.canvas_item_layer = 1
		# self.canvas_item_mask  = 1

		print("→ CarNPC: warstwy kolizji, light_mask i visibility_layer ustawione na 1")

		# 2) reszta Twojego kodu przejmowania sterowania…
		_remove_roof_platform()
		pc.call_deferred("enter_vehicle", self)
		_ai_enabled = false
		ai_input    = Vector2.ZERO
		ai_thrust   = false
		ai_hover    = false
		set_fuel_enabled(true)
		current_fuel = max_fuel
		set_player_input_enabled(true)
		call_deferred("set_controls_enabled", true)
		return

# ───────────── INTERACT MARKER ─────────────────
func _spawn_interact_marker() -> void:
	var marker := Node2D.new()
	marker.name = "InteractMarker"
	marker.position = interact_offset
	add_child(marker)
	marker.add_to_group("interactables")
	var scr := GDScript.new()
	scr.source_code = "extends Node2D
func interact():
	get_parent().interact()"
	scr.reload()
	marker.set_script(scr)

# ───────────── WAYPOINTS ─────────────────
func _gather_waypoints() -> void:
	# Upewnij się, że ścieżka jest ustawiona i węzeł istnieje
	if waypoints_path == NodePath():
		push_warning("CarNPC: waypoints_path jest pusty – pomijam zbieranie waypointów.")
		return
	var holder := get_node_or_null(waypoints_path)
	if holder == null:
		push_warning("CarNPC: nie znaleziono węzła '" + str(waypoints_path) + "' – pomijam waypointy.")
		return
	# Zbieramy global_position wszystkich Node2D pod holderem
	for c in holder.get_children():
		if c is Node2D:
			_waypoints.append(c.global_position)
