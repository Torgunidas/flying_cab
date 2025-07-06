extends CharacterBody2D
class_name Car         # jeżeli jeszcze nie miałeś class_name
# Sygnał emitowany po zniszczeniu auta
signal died



# potrzebne żeby kompilator znał typy
const NPCPassenger  = preload("res://scripts/npc_system/npc_passanger_male.gd")
const TaxiMarker    = preload("res://scripts/taxi_syst/TaxiMarker.gd")

# ----- warstwy używane w projekcie -----
const LAYER_ENV       := 1      # tu masz ściany / budynki
const LAYER_PLAYER    := 2      # np. death_zone_bottom, kolce itp.
const LAYER_HAZARDS   := 3      # sama taksówka
const SLIDE_H := 40.0     # px/s  – poziome zsuwanie
const SLIDE_V := 30.0     # px/s  – pionowe „osuwanie się”

# Unikalny identyfikator auta wykorzystywany w questach
@export var car_id: String = ""
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
@export var max_fuel: float = 2000.0               # pojemność baku
@export_range(0.0, 2000.0, 1.0)
var current_fuel: float = 2000.0                   # faktyczne paliwo – teraz widoczne w Inspectorze
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

# -------------------------------------------------------------

# -----------------------
# Parametry zatrzymywania
# -----------------------
@export var ground_stop_delay: float = 0.6
var _no_thrust_timer: float = 0.0
@export var ground_friction_multiplier: float = 4.0

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

# --- Drzwi pasażerskie ---

@onready var door_open_sprite: Node2D = $car_body/door_open
# --- Zamknięte nadwozie (chassis) ---
@onready var chassis_sprite   : Node2D = $car_body/chasis   # upewnij się, że nazwa w drzewie brzmi „chasis”
var _door_opened: bool = false  
@onready var rear_thrust_anim  : AnimatedSprite2D = $car_body/rear_thrust_anim
@onready var front_thrust_anim : AnimatedSprite2D = $car_body/front_thrust_anim
@onready var horizontal_thrust_anim : AnimatedSprite2D = $car_body/horizontal_thrust_anim
@onready var car_body : Node2D = $car_body
@onready var rc_front: RayCast2D = $car_body/PivotCheck_front
@onready var rc_rear : RayCast2D = $car_body/PivotCheck_rear

signal fuel_percent_changed(percent: float)
var _prev_fuel_pct: float = -1.0       # pamiętamy ostatnią wartość


# --- Control & state flags ------------------------------------------
var player_input_enabled : bool = true       # ◆ NOWE

func set_player_input_enabled(v: bool) -> void:   # ◆ NOWE
	player_input_enabled = v

func set_controls_enabled(enabled: bool) -> void:
	controls_enabled = enabled
func set_inertial_fall(active: bool) -> void:
	inertial_fall = active
	
# ---------- AI-override (wirtualne „klawisze”) ----------
var ai_input  : Vector2 = Vector2.ZERO   # analog −1…1  (x=←/→, y=↓/↑)
var ai_thrust : bool    = false          # “thrust_up”
var ai_hover  : bool    = false          # “hover”
var _balance_state      : int  = 0      # 0=OK, 1=rear off, 2=front off
var _balance_detached   : bool = false  # czy już oderwaliśmy się w tym stanie

# Cached reference to the level node providing gravity and air resistance.
var _level: Node = null

# Helper: returns true if the given object exposes a property with the provided name.
func _has_property(obj: Object, prop_name: String) -> bool:
		for info in obj.get_property_list():
				if "name" in info and info.name == prop_name:
						return true
		return false

func _find_level_node() -> Node:
		var n: Node = self
		while n:
				if _has_property(n, "gravity_force") and _has_property(n, "air_resistance"):
						return n
				n = n.get_parent()
		return null


# -------- PARAMETRY KASY -----------
@export var fare_rate          : float = 0.04   # 0.04 $ za każdy piksel zbliżenia
@export var fare_penalty_ratio : float = 0.5    # kara 50 % gdy się oddalamy
var _fare_accum        : float = 0.0            # policzona opłata
var _fare_last_dist    : float = 0.0            # dystans z poprzedniej klatki
var _fare_active       : bool  = false          # czy liczymy kurs?


func _btn_pressed(action: String) -> bool:
	match action:
		"thrust_up":
			return ai_thrust or (player_input_enabled and Input.is_action_pressed("thrust_up"))
		"hover":
			return ai_hover  or (player_input_enabled and Input.is_action_pressed("hover"))
		"thrust_left":
			return (ai_input.x < -0.1) or (player_input_enabled and Input.is_action_pressed("thrust_left"))
		"thrust_right":
			return (ai_input.x >  0.1) or (player_input_enabled and Input.is_action_pressed("thrust_right"))
		_:
			return player_input_enabled and Input.is_action_pressed(action)

func _btn_just_pressed(action: String) -> bool:
	return player_input_enabled and Input.is_action_just_pressed(action)

func _axis_strength(action: String) -> float:
	match action:
		"thrust_left":
			return max(-ai_input.x, Input.get_action_strength("thrust_left")  if player_input_enabled else 0.0)
		"thrust_right":
			return max( ai_input.x, Input.get_action_strength("thrust_right") if player_input_enabled else 0.0)
		"thrust_up":
			return max(-ai_input.y, Input.get_action_strength("thrust_up")   if player_input_enabled else 0.0)
		_:
			return Input.get_action_strength(action) if player_input_enabled else 0.0



func _ready() -> void:
	print("[car.gd] _ready() on", name)
	_level = _find_level_node()
	add_to_group("vehicles")
	add_to_group("interactables")

	current_hp = max_hp
	current_fuel = clamp(current_fuel, 0.0, max_fuel)   # zachowuje to, co wpiszesz w Inspectorze

		# Domyślnie wyłącz sterowanie
	controls_enabled = false

	# Inicjalizacja UI (MobileControls) ─ aktualizuj tylko dla pojazdu
		# sterowanego przez gracza. Dzięki temu spawnowanie innych aut nie
		# nadpisze wartości w HUD‑zie.
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

	input_direction.x = _axis_strength("thrust_right") - _axis_strength("thrust_left")
	input_direction.y = -_axis_strength("thrust_up")

	_update_car_orientation()


func _physics_process(delta: float) -> void:
	if state == CarState.MAN_DELIVERY:
		_update_delivery(delta)

		if _level == null:
				_level = _find_level_node()
		var level_node = _level
		if level_node == null:
				return

	# -------- BOOST z podłoża --------
	if is_on_floor():
		if _btn_just_pressed("thrust_up"):
			_is_boosting = true
			_boost_dir   = "vertical"
			_boost_timer = 0.0
		elif _btn_just_pressed("thrust_left") or _btn_just_pressed("thrust_right"):
			_is_boosting = true
			_boost_dir   = "horizontal"
			_boost_timer = 0.0
	else:
		_is_boosting = false
	if _is_boosting and _boost_timer < ground_boost_duration:
		_boost_timer += delta
	elif _is_boosting:
		_is_boosting = false

	# -------- INERTIAL FALL / WRACK --------
	if not controls_enabled and not _is_dead:
				if inertial_fall:
						velocity.y = min(velocity.y + _level.gravity_force * delta, max_fall_speed)
						velocity.x = lerp(velocity.x, 0.0, _level.air_resistance * delta)
						move_and_slide()
						_handle_ground_stop(delta)

				return
	if _is_dead:
		velocity.y = min(velocity.y + _level.gravity_force * delta, max_fall_speed)
		move_and_slide()
		return

	# -------- NORMALNE STEROWANIE --------
	var old_velocity = velocity
	velocity.y = min(velocity.y + _level.gravity_force * delta, max_fall_speed)

	# pion
	if current_fuel > 0.0:
		if _btn_pressed("thrust_up"):
			_apply_vertical_thrust(delta)
		elif _btn_pressed("hover"):
			velocity = Vector2.ZERO
	if not _btn_pressed("thrust_up") and velocity.y < 0.0:
		velocity.y = lerp(velocity.y, 0.0, vertical_damping * delta)
	if _is_boosting and _boost_dir == "vertical" and current_fuel > 0.0:
		_apply_vertical_thrust(delta)

	# poziom
	if current_fuel > 0.0:
		_apply_horizontal_thrust(delta, _level.air_resistance)
	else:
		velocity.x = lerp(velocity.x, 0.0, _level.air_resistance * delta)
			# zakładam, że current_fuel maleje gdzieś wcześniej w tym samym _physics_process
	if max_fuel <= 0.0:
		return                         # brak zbiornika? pomijamy
	var pct := 100.0 * current_fuel / max_fuel
	# emitujemy sygnał tylko, gdy realnie się zmieniło (histereza 0.1 %)
	if abs(pct - _prev_fuel_pct) >= 0.1:
		_prev_fuel_pct = pct
		emit_signal("fuel_percent_changed", pct)

	_apply_soft_ceiling(delta)
	move_and_slide()
	_update_balance(delta, _level.gravity_force) 
	_handle_ground_stop(delta)
	_apply_fuel_regeneration(delta)

	# kolizje / obrażenia
	for i in range(get_slide_collision_count()):
		var coll := get_slide_collision(i)
		if coll == null:
			continue

		var other := coll.get_collider()

	# death-zone
		if other and other.name == "death_zone_bottom":
			_on_car_death()
			return

	# potrącenie NPC
		if other is NPCPassenger:
			(other as NPCPassenger).kill(self)
			continue

	# ───── obliczenie obrażeń tylko wzdłuż normalnej ─────
		var normal  : Vector2 = coll.get_normal()                     # prostopadła do powierzchni
		var delta_v : float   = (old_velocity - velocity).dot(normal) # zmiana prędkości wzdłuż normalnej
		delta_v = abs(delta_v)

		if delta_v < damage_tolerance:
			continue                                                  # zbyt małe uderzenie – pomijamy

		_apply_damage(int(delta_v * collision_damage_factor))
	# UI (jeśli masz MobileControls)
	var ui = get_tree().get_current_scene().get_node_or_null("MobileControls")
	var player = get_tree().get_first_node_in_group("player")
	if ui and player and player.in_vehicle and player.vehicle == self:
		ui.update_fuel_display(current_fuel, max_fuel)

	_update_door_visibility()
	_update_thrust_flames()


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
	var thrust = horizontal_thrust
	if _is_boosting and _boost_dir == "horizontal" and is_on_floor():
		thrust *= ground_boost_multiplier

	if _btn_pressed("thrust_left"):
				velocity.x -= thrust * delta
	elif _btn_pressed("thrust_right"):
				velocity.x += thrust * delta
	else:
			var res := air_resistance
			if is_on_floor():
					res *= ground_friction_multiplier
			velocity.x = lerp(velocity.x, 0.0, res * delta)

	velocity.x = clamp(velocity.x, -max_horizontal_speed, max_horizontal_speed)
	current_fuel = clamp(
		current_fuel - thrust * fuel_consumption_rate_lr * delta \
					  * int(_btn_pressed("thrust_left") or _btn_pressed("thrust_right")),
		0.0, max_fuel)



func _handle_ground_stop(delta: float) -> void:
	var no_thrust := not (_btn_pressed("thrust_up")
						  or _btn_pressed("thrust_left")
						  or _btn_pressed("thrust_right"))
	if is_on_floor() and no_thrust and abs(velocity.y) < 5.0:
		_no_thrust_timer += delta
		if _no_thrust_timer >= ground_stop_delay:
			velocity.x = 0.0
	else:
		_no_thrust_timer = 0.0



func _apply_fuel_regeneration(delta: float) -> void:
	var using_thrust := _btn_pressed("thrust_up") \
						or _btn_pressed("thrust_left") \
						or _btn_pressed("thrust_right")
	if not using_thrust and velocity.y > 0.0:
		var regen = clamp(velocity.y / max_descent_speed_for_regeneration, 0.0, 1.0)
		current_fuel = clamp(current_fuel + fuel_regeneration_rate * regen * delta,
							 0.0, max_fuel)



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
	
	# -- start licznika opłaty --
	_fare_accum     = 0.0
	_fare_last_dist = global_position.distance_to(_delivery_point.global_position)
	_fare_active    = true
	
func _update_delivery(_delta: float) -> void:
	if _delivery_point == null or !is_instance_valid(_delivery_point):
		return
		
		# --- LICZNIK KASY ---
	if _fare_active:
		var dist      := global_position.distance_to(_delivery_point.global_position)
		var delta_d   := _fare_last_dist - dist          # >0 gdy zbliżamy się
		if abs(delta_d) > 0.1:
			if delta_d > 0:
				_fare_accum += delta_d * fare_rate
			else:
				_fare_accum += delta_d * fare_rate * fare_penalty_ratio
			_fare_accum = max(_fare_accum, 0)            # nigdy ujemnie
			_fare_last_dist = dist
			if is_instance_valid(_taxi_marker):
				_taxi_marker.update_fare_label(int(_fare_accum))


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
	# 5) wypłata pieniędzy i reset licznika
	if _fare_active:
		var gs : Node = get_node("/root/GameState")
		gs.add_money(int(round(_fare_accum)))      # wywoła sygnał money_changed :contentReference[oaicite:2]{index=2}
		_fare_active = false

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

# Pokazuje drzwi, gdy stoimy na podłożu,
# ukrywa, gdy lecimy.  Działa tylko, jeśli
# stan zmienił się od poprzedniej klatki.
func _update_door_visibility() -> void:
	const SPEED_THRESHOLD := 30.0      # px/s ─ zmień tu, gdybyś chciał inne zachowanie

	var on_floor = rc_front.is_colliding() and rc_rear.is_colliding()
	var horizontal_vel : float = abs(velocity.x)

	var should_open := on_floor and horizontal_vel < SPEED_THRESHOLD

	if should_open and not _door_opened:
		# -- POKAŻ DRZWI --
		door_open_sprite.visible = true
		chassis_sprite.visible   = false
		_door_opened = true

	elif not should_open and _door_opened:
		# -- SCHOWAJ DRZWI --
		door_open_sprite.visible = false
		chassis_sprite.visible   = true
		_door_opened = false


var _flames_v_open : bool = false     # pamiętać stan pionowych płomieni
var _flames_h_open : bool = false     # pamiętać stan poziomego płomienia

func _update_thrust_flames() -> void:
	var vertical_on   := _btn_pressed("thrust_up") and controls_enabled and not _is_dead and current_fuel > 0.0
	var horizontal_on := (_btn_pressed("thrust_left") or _btn_pressed("thrust_right")) \
						 and controls_enabled and not _is_dead and current_fuel > 0.0

	if vertical_on != _flames_v_open:
		rear_thrust_anim.visible  = vertical_on
		front_thrust_anim.visible = vertical_on
		if vertical_on:
			rear_thrust_anim.play("thrust")
			front_thrust_anim.play("thrust")
		else:
			rear_thrust_anim.stop()
			front_thrust_anim.stop()
		_flames_v_open = vertical_on

	if horizontal_on != _flames_h_open:
		horizontal_thrust_anim.visible = horizontal_on
		if horizontal_on:
			horizontal_thrust_anim.play("thrust")
		else:
			horizontal_thrust_anim.stop()
		_flames_h_open = horizontal_on


func _update_car_orientation() -> void:
	if _btn_pressed("thrust_right"):
		car_body.scale.x = 1
	elif _btn_pressed("thrust_left"):
		car_body.scale.x = -1

# -----------------------------------------------
func _update_balance(delta: float, g: float) -> void:
	var facing : int = sign(car_body.scale.x)   # +1 prawo, -1 lewo
	var SLIDE  : float = 40.0                   # prędkość zsuwania
	var front_hit : bool = rc_front.is_colliding()
	var rear_hit  : bool = rc_rear.is_colliding()

	# ── Scenariusz 1: stabilnie ─────────────────────────────
	if front_hit and rear_hit:
		_balance_state    = 0
		return

	# ── Scenariusz 2: tył w powietrzu ──────────────────────
	if front_hit and not rear_hit:
		_balance_state = 1
		var sx = -facing * SLIDE_H * delta   # poziomo w stronę zwisu
		var sy =  SLIDE_V * delta            # powoli w dół
		translate(Vector2(sx, sy))
		velocity.x = lerp(velocity.x, sx / delta, 2*delta)  # miękkie wyrównanie
		return

	# ── Scenariusz 3: przód w powietrzu ────────────────────
	if rear_hit and not front_hit:
		_balance_state = 2
		var sx =  facing * SLIDE_H * delta
		var sy =  SLIDE_V * delta
		translate(Vector2(sx, sy))
		velocity.x = lerp(velocity.x, sx / delta, 2*delta)
		return

	# ── ani przód, ani tył ────────────────
	_balance_state    = 0
	_balance_detached = false
