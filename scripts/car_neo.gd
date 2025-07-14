extends CharacterBody2D
class_name Carneo         # jeżeli jeszcze nie miałeś class_name
# Sygnał emitowany po zniszczeniu auta
signal died



# potrzebne żeby kompilator znał typy
const NPCPassenger  = preload("res://scripts/npc_system/npc_passanger_male.gd")
const TaxiMarker    = preload("res://scripts/taxi_syst/TaxiMarker.gd")

# ----- warstwy używane w projekcie -----
const LAYER_ENV       := 1      # tu masz ściany / budynki
const LAYER_PLAYER    := 2      # np. death_zone_bottom, kolce itp.
const LAYER_HAZARDS   := 3      # sama taksówka

# -----------------------
# Parametry ruchu/fizyki
# -----------------------
@export var thrust_force: float = 400.0
@export var horizontal_thrust: float = 250.0
@export var max_climb_speed: float = 300.0
@export var vertical_damping: float = 1.5   # im większe, tym szybciej tłumi wznoszenie
@export var max_fall_speed: float = 400.0
@export var max_altitude_y: float = -3700.0
@export var soft_altitude_range: float = 200.0
@export var soft_ceiling_friction_strength: float = 20.0
@export var ceiling_penalty: float = 20.0
@export var max_horizontal_speed: float = 300.0

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
@export var max_fuel: float = 2000.0
var current_fuel: float = 2000.0
@export var fuel_consumption_rate_up: float = 0.1
@export var fuel_consumption_rate_lr: float = 0.05
@export var fuel_regeneration_rate: float = 20
@export var max_descent_speed_for_regeneration: float = 50.0


enum CarState { IDLE, DRIVING, MAN_DELIVERY }
var state : CarState = CarState.IDLE

@export var npc_scene      : PackedScene       # przeciągnij NPCPassenger.tscn
@export var taxi_marker_scene : PackedScene   # TaxiMarker.tscn
@export var stop_radius    : float = 16.0      # kiedy „stoi” wzgl. podłoża

var _delivery_point : DeliveryPoint           # bieżący cel
var _taxi_marker    : TaxiMarker
var passenger_count : int = 0
@export var max_passanger : int = 1

# -------------------------------------------------------------

# -----------------------
# Parametry zatrzymywania
# -----------------------
@export var ground_stop_delay: float = 0.6
var _no_thrust_timer: float = 0.0

# -----------------------
# Boost przy starcie z podłoża
# -----------------------
@export var ground_boost_multiplier: float = 300.0
@export var ground_boost_duration: float = 0.15
@export var ground_boost_vertical_multiplier: float = 3.0
var _is_boosting: bool = false
var _boost_timer: float = 2.0
var _boost_dir: String = ""    # "horizontal" lub "vertical"
@export var disembark_distance : float = 256.0   # ≤ tyle pikseli od celu


# Dodatkowe do sterowania i flipowania
var input_direction: Vector2 = Vector2.ZERO

# -----------------------
# Sterowanie pojazdem (włączenie/wyłączenie)
# -----------------------
var _is_dead := false   
var controls_enabled: bool = false
var inertial_fall: bool = false
@onready var death_timer := $DeathTimer as Timer

func set_controls_enabled(enabled: bool) -> void:
	controls_enabled = enabled
func set_inertial_fall(active: bool) -> void:
	inertial_fall = active

func _ready() -> void:
	print("[car.gd] _ready() on", name)
	add_to_group("vehicles")
	add_to_group("interactables")

	current_hp = max_hp
	current_fuel = max_fuel
	passenger_count = clamp(passenger_count, 0, max_passanger)

		# Domyślnie wyłącz sterowanie
	controls_enabled = false

	# Inicjalizacja UI (MobileControls) – aktualizacja tylko gdy gracz
		# siedzi w tym pojeździe. Zapobiega to nadpisywaniu HUDu przez inne auta.
	var mobile_controls = get_tree().get_current_scene().get_node_or_null("MobileControls")
	var player = get_tree().get_first_node_in_group("player")
	if mobile_controls and player and player.in_vehicle and player.vehicle == self:
			mobile_controls.update_hp_display(current_hp, max_hp)
			mobile_controls.update_fuel_display(current_fuel, max_fuel)
		
			# połącz timeout na start reload
	death_timer.connect("timeout", Callable(self, "_on_death_timer_timeout"))

func _process(_delta: float) -> void:
	if not controls_enabled:
		return
	# Odczyt wejścia do wektora
	input_direction.x = Input.get_action_strength("thrust_right") - Input.get_action_strength("thrust_left")
	input_direction.y = -Input.get_action_strength("thrust_up")

	if input_direction.x > 0:
		$car_body.flip_h = true
	elif input_direction.x < 0:
		$car_body.flip_h = false

func _physics_process(delta: float) -> void:
	match state:
		CarState.MAN_DELIVERY:
			_update_delivery(delta)
	
	var level = get_parent()
	if not level:
		return
		
		# 8. Wykrycie boostu z podłoża
	# 1) Detekcja boostu z podłoża + rozróżnienie kierunku
	if is_on_floor():
		if Input.is_action_just_pressed("thrust_up"):
			_is_boosting = true
			_boost_dir = "vertical"
			_boost_timer = 0.0
		elif Input.is_action_just_pressed("thrust_left") or Input.is_action_just_pressed("thrust_right"):
			_is_boosting = true
			_boost_dir = "horizontal"
			_boost_timer = 0.0
	else:
		_is_boosting = false

	if _is_boosting:
		_boost_timer += delta
		if _boost_timer >= ground_boost_duration:
			_is_boosting = false

	if not controls_enabled and not _is_dead:
		if inertial_fall:
			var old_velocity = velocity

			# grawitacja + ograniczenia
			velocity.y += level.gravity_force * delta
			if velocity.y > max_fall_speed:
				velocity.y = max_fall_speed
			velocity.x = lerp(velocity.x, 0.0, level.air_resistance * delta)

			# ruch
			move_and_slide()

			# — teraz kolizje i obrażenia tak jak w normalnym locie —
			for i in range(get_slide_collision_count()):
				var coll = get_slide_collision(i)
				if coll and coll.get_collider().name == "death_zone_bottom":
					_on_car_death()
					return
				var dv = (old_velocity - velocity).length()
				_apply_damage(int(dv * collision_damage_factor))

		# blokujemy resztę logiki sterowania
		return
			

	# Dead‑branch
	if _is_dead:
		velocity.y += level.gravity_force * delta
		if velocity.y > max_fall_speed:
			velocity.y = max_fall_speed
		move_and_slide()
		return

	# Normalne sterowanie (gdy controls_enabled == true)
	var old_velocity = velocity
	velocity.y += level.gravity_force * delta
	if velocity.y > max_fall_speed:
		velocity.y = max_fall_speed

	if current_fuel > 0:
		if Input.is_action_pressed("thrust_up"):
			_apply_vertical_thrust(delta)
		elif Input.is_action_pressed("hover"):
			velocity = Vector2.ZERO
	if not Input.is_action_pressed("thrust_up") and velocity.y < 0:
		velocity.y = lerp(velocity.y, 0.0, vertical_damping * delta)

	# Poziomy thrust / hamowanie
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
			velocity.x = lerp(velocity.x, 0.0, level.air_resistance * delta)
	else:
		velocity.x = lerp(velocity.x, 0.0, level.air_resistance * delta)

	_apply_soft_ceiling(delta)
	move_and_slide()

	# ... reszta logiki (obrażenia, boost, regen, UI) bez zmian ...

	# 7. Kolizyjne obrażenia + zabicie NPC
	for i in range(get_slide_collision_count()):
		var coll := get_slide_collision(i)
		if coll == null:
			continue

		var other := coll.get_collider()

		# ── Death-zone ───────────────────────────────────────────
		if other and other.name == "death_zone_bottom":
			_on_car_death()
			return                                  # ← kończy całe _physics_process

		# ── Zderzenie z NPC ─────────────────────────────────────
		if other is NPCPassenger:
			var npc := other as NPCPassenger

			if not npc.is_dead:
				npc.kill(self)                      # zabij NPC i dodaj wyjątek
				var rel_speed := (old_velocity - velocity).length()
				var dmg := int(rel_speed * npc.body_mass * collision_damage_factor)
				_apply_damage(dmg)                  # zadaj obrażenia autu

			add_collision_exception_with(npc)       # auto ignoruje dalsze kolizje
			continue                                # wróć na początek pętli

		# ── Standardowe zderzenie (ściana/budynek) ──────────────
		var dv := (old_velocity - velocity).length()
		_apply_damage(int(dv * collision_damage_factor))



	# ── Standardowe kontakt ze ścianą ─────────
	var dv := (old_velocity - velocity).length()
	_apply_damage(int(dv * collision_damage_factor))



	

	# 9. Dodatkowe thrusty (boost poziomy + opór + MOD: boost pionowy)
	if current_fuel > 0:
		if _is_boosting:
			_apply_vertical_thrust(delta)     # MOD: dodany vertical thrust przy boostzie
		_apply_horizontal_thrust(delta, level.air_resistance)
	else:
		velocity.x = lerp(velocity.x, 0.0, level.air_resistance * delta)

	# 10. Stop na ziemi
	_handle_ground_stop(delta)

	# 11. Regeneracja paliwa przy opadaniu
	_apply_fuel_regeneration(delta)

	# 12. Debug i UI
	print("Altitude:", global_position.y,
		  "| Velocity:", velocity,
		  "| HP:", current_hp, "| Fuel:", current_fuel)
	var ui = get_tree().get_current_scene().get_node_or_null("MobileControls")
	var player = get_tree().get_first_node_in_group("player")
	if ui and player and player.in_vehicle and player.vehicle == self:
			ui.update_fuel_display(current_fuel, max_fuel)

# --- nowa wersja funkcji thrustu pionowego ---
func _apply_vertical_thrust(delta: float) -> void:
	var mult := 1.0
	if _is_boosting and _boost_dir == "vertical":
		mult = ground_boost_vertical_multiplier
	var thrust_value = thrust_force * mult

	velocity.y -= thrust_value * delta

	# zużycie paliwa
	var fuel_used = thrust_value * fuel_consumption_rate_up * delta
	current_fuel = max(current_fuel - fuel_used, 0)

	# limit prędkości
	var speed_limit = -max_climb_speed * mult
	if velocity.y < speed_limit:
		velocity.y = speed_limit

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

	var mobile_controls = get_tree().get_current_scene().get_node_or_null("MobileControls")
	var player = get_tree().get_first_node_in_group("player")
	if mobile_controls and player and player.in_vehicle and player.vehicle == self:
			mobile_controls.update_hp_display(current_hp, max_hp)

	if current_hp <= 0:
		_on_car_death()
		
		


func _on_car_death() -> void:
	# 1) Guard: jeśli już był dead, od razu wychodzimy
	if _is_dead:
		return
		

	# 2) Ustawiamy flagę dead i blokujemy sterowanie
	_is_dead = true
	_eject_passengers() 
	controls_enabled = false
	print("Car destroyed - showing death sprite.")

	
	   # 1) zapisz ostatnią prędkość pojazdu
	var _last_vel = velocity

	# 3) Wyłączamy wszystkie kolizje wraku
	collision_layer = 0
	collision_mask  = 0
	
	# Emit signal śmierci
	emit_signal("died")

	# 4) Zamieniamy sprite i odtwarzamy dźwięk
	var _car_body_sprite  = get_node("car_body")  as Sprite2D
	var car_death_sprite = get_node("car_death") as AnimatedSprite2D
	var car_death_boom = get_node("car_death/death_boom") as AnimatedSprite2D
	if car_death_sprite:
		car_death_sprite.visible = true
		car_death_boom.play()
		

	var death_sound_player = get_node("death_sound") as AudioStreamPlayer2D
	if death_sound_player:
		death_sound_player.call_deferred("play")
 
func start_delivery(point : DeliveryPoint) -> void:
	_delivery_point = point
	state           = CarState.MAN_DELIVERY
	
	# 1) kompas
	_taxi_marker = taxi_marker_scene.instantiate() as TaxiMarker
	
	if _taxi_marker == null:
		push_error("TaxiMarker scene nieprzypisana lub bez skryptu!")
		return
	
	_taxi_marker.car    = self
	_taxi_marker.target = _delivery_point
	get_tree().current_scene.add_child(_taxi_marker)
	
func _update_delivery(_delta: float) -> void:
	if _delivery_point == null or !is_instance_valid(_delivery_point):
		return

	# ---- NOWE KRYTERIUM ----
	var dist := global_position.distance_to(_delivery_point.global_position)

	if dist <= disembark_distance and is_on_floor() and velocity.length() < 2:
		_unboard_npc()




func _unboard_npc() -> void:
	if passenger_count <= 0:
		return                             # zabezpieczenie
	
	# 1) instancjonujemy NPC przy bocznych drzwiach (po prawej)
	var npc := npc_scene.instantiate() as NPCPassenger
	var spawn_offset := Vector2(24, 0)     # dopasuj do grafiki auta
	npc.global_position = global_position + spawn_offset
	get_tree().current_scene.add_child(npc)
	
	# 2) przekazujemy mu punkt docelowy
	npc.set_delivery_target(_delivery_point)
	npc.ignore_car_for_a_moment(self, 1.5) 
	
	# 3) aktualizujemy licznik
	passenger_count -= 1
	
	# 4) sprzątamy kompas + wracamy do normalnej jazdy
	if is_instance_valid(_taxi_marker):
		_taxi_marker.queue_free()
	if is_instance_valid(_delivery_point):
		_delivery_point.deactivate_marker()
	_delivery_point = null
	state = CarState.DRIVING               # lub inny domyślny stan


# ───────────────────────────────────────────────
#  Spawnuje tylu NPC-ów, ilu siedzi w aucie
# ───────────────────────────────────────────────
func _eject_passengers() -> void:
	if passenger_count <= 0:
		return                              # brak pasażerów – nic nie rób

	if npc_scene == null:
		push_error("Car: npc_scene nieustawione! (Inspector › npc_scene)")
		return

	for i in range(passenger_count):
		var npc := npc_scene.instantiate() as NPCPassenger
		if npc == null:
			continue                       # (brak sceny / błąd instancji)

		# 1) pozycja – lekko rozrzucona wokół auta
		npc.global_position = global_position + Vector2(
			randf_range(-12, 12),
			randf_range(-4,   4)
		)

		# 2) jeśli skrypt NPC ma dedykowaną metodę 'start_fall' → wywołaj ją,
		#    w przeciwnym wypadku ustaw animację ręcznie
		if npc.has_method("start_fall"):
			npc.start_fall()
		else:
			npc._sprite.animation = "fall"
			npc._sprite.play()
			# lekki impet, żeby nie stał w miejscu
			if npc.has_variable("velocity"):
				npc.velocity = Vector2(
					randf_range(-80, 80),
					-160
				)

		get_tree().current_scene.add_child(npc)   # wrzuć do świata

	passenger_count = 0                           # wnętrze auta puste
