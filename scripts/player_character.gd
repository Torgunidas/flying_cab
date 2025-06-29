class_name PlayerCharacter
extends CharacterBody2D

signal entered_vehicle(vehicle: Car)
signal exited_vehicle()
signal money_changed(new_money: int)
const SPEED := 120.0
const JUMP_VELOCITY := -300
@export var gravity_force: float = 800

# Zoom kamery pojazd/piechota
@export var zoom_in_vehicle: Vector2 = Vector2(1.5, 1.5)
@export var zoom_on_foot: Vector2    = Vector2(3.0, 3.0)

# HP postaci
@export var max_hp: int = 50
var current_hp: int = max_hp

# Urazy od wysokości
@export var fall_damage_threshold: float = 50.0
@export var fall_damage_factor: float    = 0.1

var _fall_start_y: float = 0.0
var _was_on_floor: bool   = true

# Stan pojazdu i sterowania
var vehicle: Car = null
var in_vehicle: bool         = false
var controls_enabled: bool   = true
var allowed_car_ids: Array[String] = []   # identyfikatory pojazdów, do których gracz ma dostęp

# Flagi śmierci i lądowania
var _player_dead: bool        = false
var _death_in_vehicle: bool = false
var _dead_has_landed: bool    = false

# Node'y sceny
@onready var character_anim := $character_anim as AnimatedSprite2D
@onready var char_collision := $CollisionShape2D as CollisionShape2D

func _ready() -> void:
	print("Global pause object =", typeof(pauza))   # musi wypisać 17 (OBJECT)
	add_to_group("player")
	await(get_tree().process_frame)  # czekamy, aż wszystkie pojazdy dodadzą się do grupy

	var cars = get_tree().get_nodes_in_group("vehicles")
	for v: CharacterBody2D in cars:
			add_collision_exception_with(v)
			v.add_collision_exception_with(self)
			
#	enter_vehicle()
	update_hp_ui()
	add_to_group("player")


func _physics_process(delta: float) -> void:
	# Martwy ludzki opadanie + landing dead anim
	if _player_dead:
		# Ruch spadania
		velocity.y += gravity_force * delta
		move_and_slide()
		# Po pierwszym zetknięciu z podłożem, niezależnie od kierunku, odpal animację dead
		if not _dead_has_landed and is_on_floor():
			_dead_has_landed = true
			character_anim.play("dead")
		return

	
	# Tryb pojazdu
	if in_vehicle:
		#if Input.is_action_just_pressed("toggle_vehicle"):
		#	exit_vehicle()
		return

	# Tryb pieszo
	if not controls_enabled:
		return
	var dir := Input.get_axis("thrust_left", "thrust_right")
	# Grawitacja
	if not is_on_floor():
		velocity.y += gravity_force * delta
	# Skok
	if Input.is_action_just_pressed("thrust_up") and is_on_floor():
		velocity.y = JUMP_VELOCITY
	# Ruch poziomy
	if dir != 0:
		velocity.x = dir * SPEED
	else:
		velocity.x = move_toward(velocity.x, 0, SPEED)
	# Animacje priorytet: skok>chodzenie>idle
	if not is_on_floor():
		character_anim.flip_h = dir < 0
		character_anim.play("jump")
	elif dir != 0:
		character_anim.flip_h = dir < 0
		character_anim.play("walk")
	else:
		character_anim.play("idle")
	# Ruch i detekcja upadku
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
	#if Input.is_action_just_pressed("toggle_vehicle") and global_position.distance_to(vehicle.global_position) < 32:
	#	enter_vehicle()

func set_controls_enabled(enabled: bool) -> void:
	controls_enabled = enabled
	
func _set_camera_target(node: Node) -> void:
	var cam := get_tree().get_current_scene().get_node_or_null("CameraController")
	if cam:
		cam.set_follow_target(cam.get_path_to(node))   # ścieżka względna
	
func _set_zoom(v: Vector2) -> void:
	# szukamy głównej kamery tylko raz, cache w static var
	var cam := get_tree().get_current_scene() \
						.get_node_or_null("CameraController")
	if cam:
		cam.zoom = v
		
func _ui() -> CanvasLayer:
	var ui := get_tree().get_current_scene().get_node_or_null("MobileControls")
	return ui
# -- w dowolnym miejscu w PlayerCharacter.gd, ale poza innymi funkcjami -- 
func _find_closest_vehicle() -> Car:
	var closest  : Car   = null
	var min_dist := INF

	for v in get_tree().get_nodes_in_group("vehicles"):
		if not (v is Car):
			continue                         # w grupie znalazł się nie-samochód – pomijamy

		var car := v as Car
		if not can_use_vehicle_id(car.car_id):
			continue                         # brak uprawnień do tego modelu / egzemplarza

		var d := global_position.distance_to(car.global_position)
		if d < min_dist:
			min_dist = d
			closest  = car
	return closest



func enter_vehicle(target_vehicle: Car = null) -> void:
	# 1) wybierz pojazd
	var veh : Car = target_vehicle
	if veh == null:
		veh = _find_closest_vehicle()
	if veh == null:
		return                               # nic w zasięgu

	# 2) autoryzacja
	if not can_use_vehicle_id(veh.car_id):
		print("Car not authorized")
		return

	# 2) Store it and disable collisions
	vehicle = veh
	add_collision_exception_with(vehicle)
	vehicle.add_collision_exception_with(self)

	# 3) Switch into vehicle mode
	in_vehicle = true
	set_controls_enabled(false)
	vehicle.set_inertial_fall(false)
	vehicle.set_controls_enabled(true)
	vehicle.set_player_input_enabled(true)
	# sygnał śmierci — tylko tego pojazdu
	if not vehicle.is_connected("died", Callable(self, "_on_vehicle_died")):
		vehicle.connect("died", Callable(self, "_on_vehicle_died"))

	# 4) Update camera & zoom
	_set_zoom(zoom_in_vehicle)
	_set_camera_target(vehicle)

	# 5) Update UI
	var ui = _ui()
	if ui:
		ui.set_mode("vehicle")
		ui.update_hp_display(vehicle.current_hp, vehicle.max_hp)
		ui.update_fuel_display(vehicle.current_fuel, vehicle.max_fuel)
	
	emit_signal("entered_vehicle", vehicle)
	hide()

	

func exit_vehicle() -> void:
	if not in_vehicle:
		return
	if vehicle and vehicle.is_connected("died", Callable(self, "_on_vehicle_died")):
		vehicle.disconnect("died", Callable(self, "_on_vehicle_died"))
	in_vehicle = false
	vehicle.set_controls_enabled(false)
	vehicle.set_player_input_enabled(false)
	vehicle.set_inertial_fall(true)
	set_controls_enabled(true)
	_set_zoom(zoom_on_foot)
	_set_camera_target(self)
	
	var ui = _ui()
	if ui:
		ui.set_mode("foot")  # <- to ukryje HPContainer, Life zostaje
		ui.update_life_display(current_hp, max_hp)
#		ui.set_interact_visible(false)
	
	emit_signal("exited_vehicle")
	global_position = vehicle.global_position
	show()

func _apply_damage(dmg: int) -> void:
	current_hp = max(current_hp - dmg, 0)
	update_hp_ui()
	if current_hp <= 0:
		if in_vehicle:
			exit_vehicle()
			# reload zrobi sam pojazd
		else:
			_on_player_death()

func _on_vehicle_died() -> void:
	if in_vehicle:
		# Wypadnięcie z auta: przejście w tryb dead
		exit_vehicle()
		_player_dead = true
		_death_in_vehicle = true
		_dead_has_landed = false
		_was_on_floor = false    # reset landing flag to detect landing
		_apply_damage(current_hp)
		# Animacja pożaru przy śmierci w aucie
		character_anim.play("death_fire")

func _on_player_death() -> void:
	# Śmierć poza autem: od razu dead animation
	_player_dead = true
	controls_enabled = false
	_death_in_vehicle = false
	_dead_has_landed = false
	character_anim.play("dead")
		# Schedule scene reload after 1.3s
	var t = Timer.new()
	t.one_shot = true
	t.wait_time = 1.3
	add_child(t)
	t.connect("timeout", Callable(self, "_on_player_reload"))
	t.start()

func _on_player_reload() -> void:
	# Odroczony restart sceny po śmierci gracza
	await get_tree().create_timer(0.6).timeout
	get_tree().reload_current_scene()
	
func update_hp_ui() -> void:
	var ui = get_tree().get_current_scene().get_node_or_null("MobileControls") as CanvasLayer
	if ui:
				ui.update_hp_display(current_hp, max_hp)
				ui.update_life_display(current_hp, max_hp)

# -----------------------------------------------------------------
#  Obsługa uprawnień do pojazdów
# -----------------------------------------------------------------
func can_use_vehicle_id(id: String) -> bool:
		return id == "" or GameState.has_vehicle(id)

func add_vehicle_access(id: String, is_instance := false) -> void:
	id = id.strip_edges()
	if id == "":
		return
	GameState.grant_vehicle(id, is_instance)

func remove_vehicle_access(id: String, is_instance := false) -> void:
	id = id.strip_edges()
	if id == "":
		return
	GameState.revoke_vehicle(id, is_instance)
		
