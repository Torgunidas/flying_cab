# class_name PlayerCharacter
extends CharacterBody2D

const SPEED := 150.0
const JUMP_VELOCITY := -300
@export var gravity_force: float = 800

# Gravity provided by level node
@onready var level := get_parent() as Node2D

# Zoom kamery pojazd/piechota
@export var zoom_in_vehicle: Vector2 = Vector2(1.5, 1.5)
@export var zoom_on_foot: Vector2    = Vector2(2.0, 2.0)

# HP postaci
@export var max_hp: int = 100
var current_hp: int = max_hp

# Urazy od wysokości
@export var fall_damage_threshold: float = 200.0
@export var fall_damage_factor: float    = 0.1
var _fall_start_y: float = 0.0
var _was_on_floor: bool   = true
var _player_dead: bool = false
var _dead_has_landed: bool = false

# Referencja do pojazdu i stan
var vehicle: CharacterBody2D = null
var in_vehicle: bool         = false
var controls_enabled: bool   = true

# Node'y sceny
@onready var character_anim := $character_anim as AnimatedSprite2D
@onready var char_collision := $CollisionShape2D as CollisionShape2D
@onready var player_cam     := $Camera2D         as Camera2D

func _ready() -> void:
	# Poczekaj, aż pojazdy dodadzą się do grupy
	await(get_tree().process_frame)
	var cars := get_tree().get_nodes_in_group("vehicles")
	if cars.size() == 0:
		push_error("Nie znaleziono pojazdu w grupie 'vehicles'")
		return
	vehicle = cars[0] as CharacterBody2D

	# Wyłącz kolizje między postacią a pojazdem
	add_collision_exception_with(vehicle)
	vehicle.add_collision_exception_with(self)

	# Obsługa sygnału śmierci auta
	vehicle.connect("died", Callable(self, "_on_vehicle_died"))

	# Rozpocznij w pojeździe
	enter_vehicle()
	update_hp_ui()

func _physics_process(delta: float) -> void:
	 # jeśli już umarł, nie nadpisuj animacji skokiem i chodzeniem,
	# ale wciąż pilnuj kamery
	if _player_dead:
		if in_vehicle:
			player_cam.global_position = vehicle.global_position
		else:
			player_cam.global_position = global_position
			# nadal dodajemy grawitację i przesuwamy ciało
		# pamiętaj, czy byłeś na ziemi
		var was_on_floor := is_on_floor()
		# nadal działamy pod wpływem grawitacji
		velocity.y += gravity_force * delta
		move_and_slide()

		# wykryj moment zetknięcia z podłożem po śmierci
		if not was_on_floor and is_on_floor() and not _dead_has_landed:
			_dead_has_landed = true
			character_anim.play("dead")
		return
		
	# Tryb pojazdu
	if in_vehicle:
		global_position = vehicle.global_position
		if Input.is_action_just_pressed("exit_vehicle"):
			exit_vehicle()
		return
# Kamera podąża za aktualnym aktorem
	if in_vehicle:
		player_cam.global_position = vehicle.global_position
	else:
		player_cam.global_position = global_position
	# Tryb pieszo
	if not controls_enabled:
		return
	var dir := Input.get_axis("thrust_left", "thrust_right")
	# Grawitacja
	if not is_on_floor():
		velocity.y += gravity_force * delta
		
	

# 2) Skok – nadajemy prędkość
	if Input.is_action_just_pressed("thrust_up") and is_on_floor():
		velocity.y = JUMP_VELOCITY

# 3) Ruch poziomy
	if dir != 0:
		velocity.x = dir * SPEED
	else:
		velocity.x = move_toward(velocity.x, 0, SPEED)

# 4) Wybór animacji (priorytet skoku)
	if not is_on_floor():
		character_anim.flip_h = dir < 0
		character_anim.play("jump")
	elif dir != 0:
		character_anim.flip_h = dir < 0
		character_anim.play("walk")
	else:
		character_anim.play("idle")

	# Ruch i detekcja lądowania
	move_and_slide()
	var now_on_floor := is_on_floor()
	if not _was_on_floor and now_on_floor:
		var fall_dist := global_position.y - _fall_start_y
		if fall_dist > fall_damage_threshold:
			var dmg := int((fall_dist - fall_damage_threshold) * fall_damage_factor)
			_apply_damage(dmg)
	if _was_on_floor and not now_on_floor:
		_fall_start_y = global_position.y
	_was_on_floor = now_on_floor

	# Wsiadanie do pojazdu
	if Input.is_action_just_pressed("exit_vehicle") \
	   and global_position.distance_to(vehicle.global_position) < 32:
		enter_vehicle()

func set_controls_enabled(enabled: bool) -> void:
	controls_enabled = enabled

func enter_vehicle() -> void:
	if in_vehicle:
		return
	in_vehicle = true

	set_controls_enabled(false)
	vehicle.set_inertial_fall(false)
	vehicle.set_controls_enabled(true)

	player_cam.zoom = zoom_in_vehicle

	hide()

func exit_vehicle() -> void:
	if not in_vehicle:
		return
	in_vehicle = false

	vehicle.set_controls_enabled(false)
	vehicle.set_inertial_fall(true)
	set_controls_enabled(true)

	player_cam.zoom = zoom_on_foot

	global_position = vehicle.global_position
	show()

func _apply_damage(dmg: int) -> void:
	current_hp = max(current_hp - dmg, 0)
	update_hp_ui()
	if current_hp <= 0:
		if in_vehicle:
			exit_vehicle()
			# reload będzie robić car.gd przez swój timer
		else:
			_on_player_death()

func _on_player_death() -> void:
	get_tree().reload_current_scene()

func _on_vehicle_died() -> void:
	if in_vehicle:
		exit_vehicle()
		current_hp = 0
		update_hp_ui()
				# Oznacz, że gracz jest martwy
		_player_dead = true
		controls_enabled = false
		# reload robi teraz wyłącznie timer z car.gd
		  # flipujemy postać w kierunku ruchu
		var dir := Input.get_axis("thrust_left", "thrust_right")
		character_anim.flip_h = dir < 0

		# uruchamiamy animację śmierci i pokazujemy fire
		character_anim.play("death_fire")
		

func update_hp_ui() -> void:
	var ui := get_parent().get_node("MobileControls") as CanvasLayer
	if ui:
		ui.update_hp_display(current_hp, max_hp)
		ui.update_life_display(current_hp, max_hp)
