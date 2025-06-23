extends CharacterBody2D

@export var thrust_force: float = 200.0
@export var horizontal_thrust: float = 100.0
@export var max_climb_speed: float = 300.0
@export var max_altitude_y: float = 100.0
@export var soft_altitude_range: float = 200.0
@export var soft_ceiling_friction_strength: float = 5.0
@export var ceiling_penalty: float = 10.0
@export var max_horizontal_speed: float = 200.0

@export var max_hp: int = 100
var current_hp: int = 100

@export var collision_damage_factor: float = 0.5
@export var damage_tolerance: float = 1.0
# np. 1.0 oznacza, że jeśli dmg < 1.0, nie odejmujemy HP

func _ready() -> void:
	current_hp = max_hp
	GameState.current_car = self
	# Jeśli UI ma się aktualizować już na starcie, możesz wywołać update HP 
	var mobile_controls = get_parent().get_node("MobileControls")
	if mobile_controls:
		mobile_controls.update_hp_display(current_hp, max_hp)


func _physics_process(delta: float) -> void:
	var level = get_parent()
	if not level:
		print("Warning: No parent with level.gd found.")
		return

	var gravity_force = level.gravity_force
	var air_resistance = level.air_resistance

	# Zapamiętujemy prędkość sprzed ruchu
	var prev_velocity = velocity

	# --- Logika sterowania ---
	velocity.y += gravity_force * delta

	if Input.is_action_pressed("hover"):
		velocity = Vector2.ZERO
	else:
		# THRUST UP (miękkie ograniczenie wysokości)
		if Input.is_action_pressed("thrust_up"):
			var altitude_diff = max_altitude_y - global_position.y
			var factor = clamp(altitude_diff / soft_altitude_range, 0.0, 1.0)

			var desired_final = gravity_force + ceiling_penalty
			var effective_thrust = thrust_force - factor * (thrust_force - desired_final)
			velocity.y -= effective_thrust * delta

			if velocity.y < -max_climb_speed:
				velocity.y = -max_climb_speed

		# THRUST LEFT / RIGHT
		if Input.is_action_pressed("thrust_left"):
			velocity.x -= horizontal_thrust * delta
		if Input.is_action_pressed("thrust_right"):
			velocity.x += horizontal_thrust * delta

		# Ograniczenie prędkości poziomej
		velocity.x = clamp(velocity.x, -max_horizontal_speed, max_horizontal_speed)

		# Tłumienie
		velocity.x = lerp(velocity.x, 0.0, air_resistance * delta)

	# Ruch z wykrywaniem kolizji
	move_and_slide()

	# Sprawdzamy, czy wystąpiła kolizja
	var collision_count = get_slide_collision_count()
	if collision_count > 0:
		var delta_v = (prev_velocity - velocity).length()
		var dmg = int(delta_v * collision_damage_factor)
		_apply_damage(dmg)

	# Wytracanie prędkości w strefie powyżej max_altitude_y
	var altitude_diff2 = max_altitude_y - global_position.y
	if altitude_diff2 > 0.0 and velocity.y < 0.0:
		var factor2 = clamp(altitude_diff2 / soft_altitude_range, 0.0, 1.0)
		velocity.y = lerp(velocity.y, 0.0, factor2 * soft_ceiling_friction_strength * delta)

	# Debug info
	print("Altitude: ", global_position.y,
		  " | Velocity: ", velocity,
		  " | HP: ", current_hp)

func _apply_damage(dmg: int) -> void:
	# Jeśli obrażenia poniżej progu, to ignorujemy
	if dmg < damage_tolerance:
		print("Minor collision! Dmg=", dmg, " < tolerance=", damage_tolerance)
		return

	current_hp -= dmg
	print("Collision! Dmg=", dmg, " HP=", current_hp)

	# Znajdź MobileControls i zaktualizuj wyświetlanie
	var mobile_controls = get_parent().get_node("MobileControls")
	if mobile_controls:
		mobile_controls.update_hp_display(current_hp, max_hp)

	if current_hp <= 0:
		_on_car_death()

func _on_car_death() -> void:
	print("Car destroyed - restarting scene.")
	get_tree().reload_current_scene()
