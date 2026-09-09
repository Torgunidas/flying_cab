extends CharacterBody2D


# ----- warstwy używane w projekcie -----
const LAYER_ENV       := 1      # tu masz ściany / budynki
const LAYER_PLAYER    := 2      # np. death_zone_bottom, kolce itp.
const LAYER_HAZARDS   := 3      # sama taksówka

# -----------------------
# Parametry ruchu/fizyki
# -----------------------
@export var thrust_force: float = 200.0
@export var horizontal_thrust: float = 100.0
@export var max_climb_speed: float = 300.0
@export var vertical_damping: float = 3.0   # im większe, tym szybciej tłumi wznoszenie
@export var max_fall_speed: float = 400.0
@export var max_altitude_y: float = -3700.0
@export var soft_altitude_range: float = 200.0
@export var soft_ceiling_friction_strength: float = 5.0
@export var ceiling_penalty: float = 10.0
@export var max_horizontal_speed: float = 200.0

# -----------------------
# Parametry HP
# -----------------------
@export var max_hp: int = 300
var current_hp: int = 300
@export var collision_damage_factor: float = 0.5
@export var damage_tolerance: float = 40

# -----------------------
# Parametry paliwa
# -----------------------
@export var max_fuel: float = 100.0
var current_fuel: float = 100.0
@export var fuel_consumption_rate_up: float = 0.1
@export var fuel_consumption_rate_lr: float = 0.05
@export var fuel_regeneration_rate: float = 20
@export var max_descent_speed_for_regeneration: float = 50.0

# -----------------------
# Parametry zatrzymywania
# -----------------------
@export var ground_stop_delay: float = 0.6
var _no_thrust_timer: float = 0.0

# -----------------------
# Boost przy starcie z podłoża
# -----------------------
@export var ground_boost_multiplier: float = 200.0
@export var ground_boost_duration: float = 0.15
var _is_boosting: bool = false
var _boost_timer: float = 2.0

# Dodatkowe do sterowania i flipowania
var input_direction: Vector2 = Vector2.ZERO

var _is_dead := false               # <-- nowa zmienna na początku skryptu
var controls_enabled: bool = true

func _ready() -> void:
	current_hp = max_hp
	current_fuel = max_fuel

	# Jeśli masz globalny GameState:
	# GameState.current_car = self

	# Inicjalizacja UI (załóżmy, że MobileControls jest rodzeństwem)
	var mobile_controls = get_parent().get_node("MobileControls")
	if mobile_controls:
		mobile_controls.update_hp_display(current_hp, max_hp)
		mobile_controls.update_fuel_display(current_fuel, max_fuel)


func _process(delta: float) -> void:
	if not controls_enabled:
		return          # wrak nie reaguje na wejście
	# Odczyt wejścia do wektora
	input_direction.x = Input.get_action_strength("thrust_right") - Input.get_action_strength("thrust_left")
	input_direction.y = -Input.get_action_strength("thrust_up")

	# Flipowanie sprite'a w zależności od kierunku x
	# (Przykładowo: x > 0 -> flip_h = true, x < 0 -> flip_h = false)
	if input_direction.x > 0:
		$car_body.flip_h = true
	elif input_direction.x < 0:
		$car_body.flip_h = false

func _physics_process(delta: float) -> void:
	var level = get_parent()
	if not level:
		return

	var gravity_force = level.gravity_force
	var air_resistance = level.air_resistance
	
	# --- MOD: gdy wrak już „dead”, tylko aplikujemy grawitację i opadamy ---
	if _is_dead:
		velocity.y += gravity_force * delta
		if velocity.y > max_fall_speed:
			velocity.y = max_fall_speed
		move_and_slide()
		return
		
	# —————— normalna logika, tylko gdy alive ——————
	if not controls_enabled:
		return

	# 1. Zapamiętaj starą prędkość (do obrażeń)
	var old_velocity = velocity

	# 2. Grawitacja i limit opadania
	velocity.y += gravity_force * delta
	if velocity.y > max_fall_speed:
		velocity.y = max_fall_speed

	# 3. Pionowy thrust i hover (tylko przy paliwie)
	if current_fuel > 0:
		if Input.is_action_pressed("thrust_up"):
			_apply_vertical_thrust(delta)
		elif Input.is_action_pressed("hover"):
			velocity = Vector2.ZERO
			
		# 3b. Tłumienie pozostałej prędkości w górę  <-- DAMP
	if not Input.is_action_pressed("thrust_up") and velocity.y < 0:
		velocity.y = lerp(velocity.y, 0.0, vertical_damping * delta)   # DAMP

	# 4. Bezpośredni thrust poziomy (tylko przy paliwie)
	if current_fuel > 0:
		if Input.is_action_pressed("thrust_left"):
			var fuel_lr = horizontal_thrust * fuel_consumption_rate_lr * delta
			current_fuel = max(current_fuel - fuel_lr, 0)
			velocity.x -= horizontal_thrust * delta
		elif Input.is_action_pressed("thrust_right"):
			var fuel_lr = horizontal_thrust * fuel_consumption_rate_lr * delta
			current_fuel = max(current_fuel - fuel_lr, 0)
			velocity.x += horizontal_thrust * delta
		else:
			velocity.x = lerp(velocity.x, 0.0, air_resistance * delta)
	else:
		# brak paliwa -> tylko hamowanie poziome
		velocity.x = lerp(velocity.x, 0.0, air_resistance * delta)

	# 5. „Miękki” sufit
	_apply_soft_ceiling(delta)

	# 6. Ruch i kolizje
	move_and_slide()

	# 7. Kolizyjne obrażenia
	for i in range(get_slide_collision_count()):
		var coll = get_slide_collision(i)
		if coll and coll.get_collider().name == "death_zone_bottom":
			_on_car_death()
			return
		var dv = (old_velocity - velocity).length()
		_apply_damage(int(dv * collision_damage_factor))

	# 8. Wykrycie boostu z podłoża
	if is_on_floor():
		if Input.is_action_just_pressed("thrust_left") or Input.is_action_just_pressed("thrust_right"):
			_is_boosting = true
			_boost_timer = 0.0
	else:
		_is_boosting = false

	if _is_boosting:
		_boost_timer += delta
		if _boost_timer >= ground_boost_duration:
			_is_boosting = false

	# 9. Dodatkowe thrusty (boost poziomy + opór + MOD: boost pionowy)
	if current_fuel > 0:
		if _is_boosting:
			_apply_vertical_thrust(delta)     # MOD: dodany vertical thrust przy boostzie
		_apply_horizontal_thrust(delta, air_resistance)
	else:
		velocity.x = lerp(velocity.x, 0.0, air_resistance * delta)

	# 10. Stop na ziemi
	_handle_ground_stop(delta)

	# 11. Regeneracja paliwa przy opadaniu
	_apply_fuel_regeneration(delta)

	# 12. Debug i UI
	print("Altitude:", global_position.y,
		  "| Velocity:", velocity,
		  "| HP:", current_hp, "| Fuel:", current_fuel)
	var ui = get_parent().get_node("MobileControls")
	if ui:
		ui.update_fuel_display(current_fuel, max_fuel)

# --- nowa wersja funkcji thrustu pionowego ---
func _apply_vertical_thrust(delta: float) -> void:
	# FIX: prosty stały ciąg w górę
	velocity.y -= thrust_force * delta
	# FIX: zużycie paliwa
	var fuel_used = thrust_force * fuel_consumption_rate_up * delta
	current_fuel = max(current_fuel - fuel_used, 0)
	# FIX: limit prędkości wznoszenia
	if velocity.y < -max_climb_speed:
		velocity.y = -max_climb_speed



func _apply_soft_ceiling(delta: float) -> void:
	var altitude_diff2 = max_altitude_y - global_position.y
	if altitude_diff2 > 0.0 and velocity.y < 0.0:
		var factor2 = clamp(altitude_diff2 / soft_altitude_range, 0.0, 1.0)
		velocity.y = lerp(velocity.y, 0.0, factor2 * soft_ceiling_friction_strength * delta)


func _apply_horizontal_thrust(delta: float, air_resistance: float) -> void:
	var final_thrust = horizontal_thrust

	# Boost tylko na podłożu w krótkim okienku
	if _is_boosting and is_on_floor():
		final_thrust *= ground_boost_multiplier

	if Input.is_action_pressed("thrust_left"):
		velocity.x -= final_thrust * delta
	elif Input.is_action_pressed("thrust_right"):
		velocity.x += final_thrust * delta
	else:
		# Jeśli nie wciskamy klawisza w poziomie – hamujemy oporem powietrza
		velocity.x = lerp(velocity.x, 0.0, air_resistance * delta)

	# Ograniczamy prędkość w poziomie
	velocity.x = clamp(velocity.x, -max_horizontal_speed, max_horizontal_speed)


func _handle_ground_stop(delta: float) -> void:
	var no_thrust = (not Input.is_action_pressed("thrust_up")
					 and not Input.is_action_pressed("thrust_left")
					 and not Input.is_action_pressed("thrust_right"))

	if is_on_floor() and no_thrust and abs(velocity.y) < 5:
		_no_thrust_timer += delta
		if _no_thrust_timer >= ground_stop_delay:
			velocity.x = 0
	else:
		_no_thrust_timer = 0.0


func _apply_fuel_regeneration(delta: float) -> void:
	# Regeneracja paliwa tylko jeśli nie wciskamy thrust
	if not (Input.is_action_pressed("thrust_up") 
			or Input.is_action_pressed("thrust_left") 
			or Input.is_action_pressed("thrust_right")):
		# i prędkość w pionie > 0 (czyli spadamy) – można dostroić warunek
		if velocity.y > 0:
			var regen_factor = clamp(velocity.y / max_descent_speed_for_regeneration, 0.0, 1.0)
			current_fuel += fuel_regeneration_rate * regen_factor * delta
			current_fuel = clamp(current_fuel, 0, max_fuel)


func _apply_damage(dmg: int) -> void:
	if _is_dead:          # wrak już nie zbiera obrażeń
		return
	if dmg < damage_tolerance:
		return
	if dmg < damage_tolerance:
		print("Minor collision! Dmg=", dmg, " < tolerance=", damage_tolerance)
		return
	
	current_hp = max(current_hp - dmg, 0)
	print("Collision! Dmg=", dmg, " HP=", current_hp)

	var mobile_controls = get_parent().get_node("MobileControls")
	if mobile_controls:
		mobile_controls.update_hp_display(current_hp, max_hp)

	if current_hp <= 0:
		_on_car_death()


func _on_car_death() -> void:
	# 1) Guard: jeśli już był dead, od razu wychodzimy
	if _is_dead:
		return

	# 2) Ustawiamy flagę dead i blokujemy sterowanie
	_is_dead = true
	controls_enabled = false
	print("Car destroyed - showing death sprite.")
	
	   # 1) zapisz ostatnią prędkość pojazdu
	var last_vel = velocity

	# 3) Wyłączamy wszystkie kolizje wraku
	collision_layer = 0
	collision_mask  = 0
	
	 # 3) odczep i ustaw kamerę
	var cam = get_node("Camera2D") as Camera2D
	if cam:
		cam.get_parent().remove_child(cam)
		get_tree().get_current_scene().add_child(cam)
		cam.global_position = global_position
		cam.make_current()

		# 4) Tween: kamera podąża chwilę za last_vel
		var inertia_factor = 0.3    # zatrzyma się po 30% odcinka
		var inertia_time   = 0.5    # przez 0.5 sekundy
		var target_pos = cam.global_position + last_vel * inertia_factor
		var tw = cam.create_tween()
		tw.tween_property(cam, "global_position", target_pos, inertia_time)\
		  .set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)

	# 4) Zamieniamy sprite i odtwarzamy dźwięk
	var car_body_sprite  = get_node("car_body")  as Sprite2D
	var car_death_sprite = get_node("car_death") as Sprite2D
	if car_body_sprite:
		car_body_sprite.visible = false
	if car_death_sprite:
		car_death_sprite.visible = true

	var death_sound_player = get_node("death_sound") as AudioStreamPlayer2D
	if death_sound_player:
		death_sound_player.play()

	# 5) Po 1 sekundzie przeładuj scenę
	await get_tree().create_timer(1.0).timeout
	get_tree().reload_current_scene()
