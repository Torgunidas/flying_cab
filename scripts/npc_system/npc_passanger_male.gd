extends CharacterBody2D
class_name NPCPassenger

# ───────── PARAMETRY RUCHU ─────────
@export var speed: float             = 60.0
@export var arrive_radius: float     = 4.0

# ───────── PARAMETRY TAXI CALL ─────
@export var call_interval_min: float = 30.0   # minimalny odstęp wywołania taxi (sekundy)
@export var call_interval_max: float = 60.0   # maksymalny odstęp wywołania taxi (sekundy)
@export var taxi_detect_radius: float = 120.0 # promień wykrycia taksówki
@export var board_distance: float     = 20.0  # dystans, w którym NPC wsiada do auta
@export var taxi_wait_time: float     = 20.0  # dodatkowy czas oczekiwania na taksówkę (nowa zmienna)
@export var disappear_after: float    = 4.0   # zniknięcie po “śmierci” (używane wcześniej)
@export var body_mass: float          = 1.0   # do obrażeń auta (jeśli jest)
@export var dynamic_spawn : bool = false   # true = NPC stworzony przez SpawnPoint


# ───────── STANY ────────────────────
enum State {
	PATROL,          # ← istniał
	CALL_TAXI,       # ← istniał (czeka przy krawężniku)
	GO_TO_CAR,       # ← istniał
	GO_TO_DEST,      # ← istniał
	FALL,            # ← istniał (wypadł z auta)

	# ⇩ NOWE, potrzebne tylko dla „cyklu spawnu”
	WALK_TO_WP,      # idzie z punktu spawn do waypointa
	WAIT_TAXI,       # aktywne wołanie taksówki (to samo co CALL_TAXI – alias)
	GO_HOME          # wraca do punktu spawn
}
var _state: State = State.PATROL


# ───────── WEWNĘTRZNE ───────────────
var _points: Array[Vector2] = []    # lista punktów patrolu
var _idx: int = 0                   # indeks aktualnego punktu patrolu
var _target_car: Node = null        # referencja do samochodu, do którego idziemy
var is_dead: bool = false           # flaga, czy NPC “umarl”
var _home_pos  : Vector2            # gdzie wrócić
var _wp_target : Vector2            # punkt waypoint

# ───────── REFERENCJE ───────────────
@onready var _sprite: AnimatedSprite2D = $Sprite
@onready var _call_label: Label         = $CallLabel
@onready var _call_timer: Timer         = $CallTimer
@onready var _wait_timer: Timer         = $WaitTimer  # ← nowy odnośnik do WaitTimer

# ───────── FIZYKA UPADKU ────────────
@export var fall_impulse  : Vector2 = Vector2(0, -200)   # pierwszy “skok” w górę
@export var gravity       : float    = 750.0             # przysp. grawitacyjne
@export var term_velocity : float    = 550.0             # maks prędkość spadania
@export var idle_time      : float = 4.0     # ile stoi przy waypoint
@export var spawn_lifespan : float = 60.0    # max czas życia od spawnu
## Lista węzłów-kontenerów, w których trzymasz DeliveryPointy
@export var delivery_groups : Array[NodePath] = []


func _ready() -> void:
	_gather_waypoints()
	assert(_points.size() >= 2)  # przynajmniej 2 punkty potrzeba, by patrolować

	# Ustawiamy animację chodzenia
	_sprite.animation = "walk"
	_sprite.play()

	# Konfiguracja CallTimer (timeout = losowy interwał do wystawienia telefonu po taksówkę)
	_call_timer.one_shot     = true
	_call_timer.process_mode = Node.PROCESS_MODE_ALWAYS
	_call_timer.timeout.connect(_on_CallTimer_timeout)

	# Konfiguracja WaitTimer (timeout = czas oczekiwania na taksówkę)
	_wait_timer.one_shot     = true
	_wait_timer.process_mode = Node.PROCESS_MODE_ALWAYS
	_wait_timer.timeout.connect(_on_WaitTimer_timeout)  # podpinamy funkcję, która wróci do Patrolu
	#  ▼  IdleTimer – jedno-strzałowy, wywoła przełączenie na czekanie taxi


	


	# Startujemy pierwszy losowy odliczacz do wezwania taksówki
	_schedule_next_call()
	_update_flip()


func _gather_waypoints() -> void:
	# Pobiera wszystkie dzieci węzła "Waypoints" i zapisuje ich pozycje
	for c in $"Waypoints".get_children():
		if c is Node2D:
			_points.append(c.global_position)


func _physics_process(delta: float) -> void:
	if is_dead:
		return

	match _state:
		State.PATROL:
			_patrol_move(delta)

		State.WALK_TO_WP:
			_walk_to_wp(delta)
		State.CALL_TAXI, State.WAIT_TAXI:   # oba idą do tej samej funkcji
			_wait_for_taxi(delta)
		State.GO_TO_CAR:
			_walk_to_car(delta)
		State.GO_HOME:
			_go_home(delta)
		State.GO_TO_DEST:
			_go_to_dest(delta)
		State.FALL:
			_fall_move(delta)
		_:
			pass    # istniejące stany
			
func _walk_to_wp(delta: float) -> void:
	var dir := _wp_target - global_position

	if dir.length() <= arrive_radius:
		velocity = Vector2.ZERO
		move_and_slide()

		# ← tu wchodzimy prosto w czekanie na taksówkę
		_enter_wait_for_taxi()
		return

	velocity = dir.normalized() * speed
	move_and_slide()
	_update_flip()


func _stand_idle(_delta: float) -> void:
	velocity = Vector2.ZERO
	move_and_slide()

func _go_home(delta: float) -> void:
	# upewnij się, że chodzimy
	if _sprite.animation != "walk":
		_sprite.animation = "walk"
		_sprite.play()

	var dir := _home_pos - global_position
	if dir.length() <= arrive_radius:
		queue_free()
		return

	velocity = dir.normalized() * speed
	move_and_slide()
	_update_flip()



func start_spawn_sequence(
		wp_pos: Vector2,     # dokąd iść
		wait_sec: float,     # ile machać ręką
		home_pos: Vector2    # gdzie wrócić i zniknąć
) -> void:
	dynamic_spawn  = true
	_home_pos      = home_pos
	taxi_wait_time = wait_sec
	_wp_target     = wp_pos
	_state         = State.WALK_TO_WP


# ────────────────────────────────────
#  1) PATROL: NPC przechodzi przez punkty
# ────────────────────────────────────
func _patrol_move(delta: float) -> void:
	var target := _points[_idx]
	var to     := target - global_position

	if to.length_squared() <= arrive_radius * arrive_radius:
		# Jeśli dotarliśmy do punktu, zmieniamy na kolejny
		_idx = (_idx + 1) % _points.size()
		target = _points[_idx]
		to     = target - global_position

	velocity = to.normalized() * speed
	move_and_slide()
	_update_flip()


# ────────────────────────────────────
#  2) CALL_TAXI: NPC stoi, migota napis, czeka na taksówkę
# ────────────────────────────────────
func _wait_for_taxi(_delta: float) -> void:
	# 1) NPC stoi w miejscu (zerujemy velocity i wciąż wyświetlamy napis)
	velocity = Vector2.ZERO
	move_and_slide()

	# 2) Szukamy pojazdu gracza (jeśli gracz wsiadł do auta)
	var car: CharacterBody2D = _find_player_car()
	if car == null:
		return  # jeżeli gracz nie jest w aucie, to nic więcej nie robimy
		
		# 2.5) ►► NOWOŚĆ ◄◄  – pomijamy przepełnione auta
	if car.has_variable("max_passanger") \
	and car.passenger_count >= car.max_passanger:
		return                                   # taxi pełna → czekamy dalej

	# 3) Jeżeli auto było CharacterBody2D, upewniamy się, że stoi na ziemi (można pominąć dla RigidBody2D)
	if car.is_class("CharacterBody2D") and not car.is_on_floor():
		return  # auto jeszcze się nie zatrzymało całkowicie → czekamy

	# 4) Sprawdzamy dystans od NPC do auta gracza
	var dist_to_car := global_position.distance_to(car.global_position)
	if dist_to_car <= taxi_detect_radius:
		# gdy auto jest blisko dostatecznie (we wgranym promieniu):
		_target_car = car

		# Wyłączamy kolizje między NPC a autem, by NPC “wsunął” się do auta
		add_collision_exception_with(_target_car)
		_target_car.add_collision_exception_with(self)

		# Przechodzimy do stanu GO_TO_CAR
		_state = State.GO_TO_CAR
		_sprite.animation = "walk"
		_sprite.play()
		_call_label.visible = false

		# WAŻNE: jeśli znaleźliśmy auto, zatrzymujemy timer WaitTimer,
		# bo nie chcemy wracać do patrolu dopóki idziemy do auta
		if _wait_timer.is_stopped() == false:
			_wait_timer.stop()

		_update_flip()


# ────────────────────────────────────
#  3) GO_TO_CAR: NPC idzie w kierunku auta i wsiada
# ────────────────────────────────────
func _walk_to_car(delta: float) -> void:
	if not is_instance_valid(_target_car):
		# Gdy auto przestaje być ważne (np. zniknęło), resetujemy do patrolu
		_reset_to_patrol()
		return

	var to_car: Vector2 = _target_car.global_position - global_position
	if to_car.length() <= board_distance:
		# Jeśli jesteśmy już wystarczająco blisko samochodu, to wsiadamy
		_board_taxi()
		return
	if _target_car.velocity.length() > 50:
		_reset_to_patrol()
		return

	velocity = to_car.normalized() * speed
	move_and_slide()
	_update_flip()


# ────────────────────────────────────
#  UTILITIES
# ────────────────────────────────────
func _update_flip() -> void:
	if abs(velocity.x) > 1.0:
		_sprite.flip_h = velocity.x < 0


func _find_player_car() -> CharacterBody2D:
	# 1) Pobierz wszystkie węzły w grupie "player"
	var arr := get_tree().get_nodes_in_group("player")
	if arr.size() == 0:
		return null

	# 2) Rzutujemy wynik na PlayerCharacter
	var pc: PlayerCharacter = arr[0] as PlayerCharacter
	if pc == null or not is_instance_valid(pc):
		return null

	# 3) Jeżeli gracz nie jest w aucie, to nic nie robimy
	if not pc.in_vehicle:
		return null

	# 4) Zwracamy referencję do pojazdu
	var car: CharacterBody2D = pc.vehicle
	if car == null or not is_instance_valid(car):
		return null

	return car

func _pick_random_delivery_point() -> DeliveryPoint:
	var pool : Array[DeliveryPoint] = []

	# 1) Zbierz DP z podpiętych kontenerów
	for p in delivery_groups:
		var root := get_node_or_null(p)
		if root == null:        # ścieżka zepsuta → pomijamy
			continue
		# przejrzyj dzieci (bez rekurencji; jeśli masz głębiej zagnieżdżone,
		# użyj root.get_children_recursive())
		for c in root.get_children_recursive():
			if c is DeliveryPoint:
				pool.append(c)
	# 2) Fallback – jeśli lista pusta, wracamy do starego sposobu (najbliższy z grupy)
	if pool.is_empty():
		for d in get_tree().get_nodes_in_group("taxi_dest"):
			if d is DeliveryPoint:
				pool.append(d)

	if pool.is_empty():
		return null

	# 3) Losowy wybór
	var rng := RandomNumberGenerator.new()
	rng.randomize()
	return pool[rng.randi_range(0, pool.size()-1)]


func _reset_to_patrol() -> void:
	# ► 1. Przywróć kolizję w OBU kierunkach
	if is_instance_valid(_target_car):
		# – NPC znowu widzi samochód
		remove_collision_exception_with(_target_car)
		# – samochód znowu widzi NPC-a  ← brakowało TEJ linijki
		_target_car.remove_collision_exception_with(self)

	# ► 2. Wyzeruj referencję, żeby następne wywołanie mogło znaleźć nowy pojazd
	_target_car = null

	# ► 3. Schowaj etykietę „TAXI” (jeśli masz label) i wróć do patrolu
	_call_label.visible = false
	_state  = State.PATROL
	_sprite.animation = "walk"
	_sprite.play()

	# ► 4. Zaplanuj kolejne wezwanie taksówki po losowym czasie
	_schedule_next_call()



func _schedule_next_call() -> void:
	# Losujemy czas do kolejnego wezwania taksówki
	var t := randf_range(call_interval_min, call_interval_max)
	_call_timer.start(t)


# ────────────────────────────────────
#  SLOTY TIMERA i BOARDING
# ────────────────────────────────────

func _on_CallTimer_timeout() -> void:
	# Gdy CallTimer odliczy czas, przechodzimy w stan oczekiwania na taksówkę
	_state = State.CALL_TAXI
	_call_label.visible = true
	_sprite.animation = "idle"
	_sprite.play()

	# Uruchamiamy WaitTimer na określony czas oczekiwania (np. taxi_wait_time sekund)
	_wait_timer.start(taxi_wait_time)
	
func _on_IdleTimer_timeout() -> void:
	# zakończyliśmy krótkie „przystanęcie” przy waypoint
	_state = State.WAIT_TAXI          # (albo State.CALL_TAXI – oba działają)
	_call_label.visible = true        # pokaż znak TAXI
	_sprite.animation = "idle"
	_sprite.play()

	# start licznika machania – jego czas dostałeś z PassengerSpawnPoint
	_wait_timer.start(taxi_wait_time)

func _enter_wait_for_taxi() -> void:
	_state = State.WAIT_TAXI        # użyj CALL_TAXI jeśli wolisz jedną nazwę
	_sprite.animation = "idle"
	_sprite.play()
	_call_label.visible = true
	_wait_timer.start(taxi_wait_time)   # czas przekazany przez spawn-point


func _on_WaitTimer_timeout() -> void:
	if dynamic_spawn:
		# → pasażer nie był zabrany; wraca do punktu startu
		_call_label.visible = false
		_state = State.GO_HOME
	else:
		# → zwykły mieszkaniec – wraca do patrolu jak dotąd
		_reset_to_patrol()


func _board_taxi() -> void:
	# 1) zabezpieczenie
	if !is_instance_valid(_target_car):
		_reset_to_patrol()
		return
	
	# 1b) sprawdź pojemność pojazdu
	if _target_car.has_variable("max_passanger"):
			if _target_car.passenger_count >= _target_car.max_passanger:
					_reset_to_patrol()
					return

	# 2) zwiększ licznik pasażerów
	_target_car.passenger_count += 1

	# 3) losujemy cel z przypisanych delivery_groups (lub fallback)
	var dest : DeliveryPoint = _pick_random_delivery_point()

	if dest == null:
		print("Brak dostępnych DeliveryPoint! Sprawdź 'delivery_groups' w NPC lub SpawnPoincie.")
		_reset_to_patrol()
		return

	# 4) start zlecenia
	_target_car.start_delivery(dest)
	dest.activate_marker()

	# 5) NPC znika – siedzi w aucie
	queue_free()




# ────────────────────────────────────
#  DOTYCHCZASOWA FUNKCJA KILL
# ────────────────────────────────────
func kill(car: CharacterBody2D=null) -> void:
	if is_dead:
		return
	is_dead = true
	set_physics_process(false)
	velocity = Vector2.ZERO
	$CollisionShape2D.disabled = true
	collision_layer = 0
	collision_mask  = 0
	_sprite.animation = "dead"
	_sprite.play()
	if car:
		add_collision_exception_with(car)
		car.add_collision_exception_with(self)
	var t := Timer.new()
	t.one_shot = true
	t.wait_time = disappear_after
	t.process_mode = Node.PROCESS_MODE_ALWAYS
	add_child(t)
	t.connect("timeout", Callable(self, "_on_dead_timeout"))
	t.start()

func _on_dead_timeout() -> void:
	queue_free()
	
@export var walk_speed_to_dest : float = 60.0
var _delivery_target : DeliveryPoint
var _target_wp        : Node2D
var _unspawn_on_arrive: bool = true

func set_delivery_target(point : DeliveryPoint) -> void:
	_target_wp        = point.get_waypoint()
	_unspawn_on_arrive = point.unspawn
	_state            = State.GO_TO_DEST

func _go_to_dest(delta: float) -> void:
	if !is_instance_valid(_target_wp):
		queue_free(); return

	var dir := _target_wp.global_position - global_position
	if dir.length() < 8.0:
		if _unspawn_on_arrive:
			queue_free()                       # znikamy
		else:
			_state = State.PATROL              # lub idle / animacja machania
		return

	velocity = dir.normalized() * walk_speed_to_dest
	move_and_slide()
	_update_flip()
	
# ──────────────────────────────────────────────────────────────
#  IGNOROWANIE KOLIZJI Z SAMOCHODEM PRZEZ KILKA SEKUND
# ──────────────────────────────────────────────────────────────
func ignore_car_for_a_moment(car: Node2D, seconds: float = 1.5) -> void:
	# 1) Wyłącz kolizję w obie strony
	add_collision_exception_with(car)
	car.add_collision_exception_with(self)

	# 2) Odpal jednorazowy Timer
	var timer := Timer.new()
	timer.one_shot     = true
	timer.wait_time    = seconds
	timer.process_mode = Node.PROCESS_MODE_ALWAYS   # tyka nawet w pauzie
	add_child(timer)

	# 3) Gdy Timer wygaśnie → przywróć kolizję
	timer.timeout.connect(_on_ignore_timer_timeout.bind(car, timer))
	timer.start()


func _on_ignore_timer_timeout(car: Node2D, timer: Timer) -> void:
	if is_instance_valid(car):
		remove_collision_exception_with(car)
		car.remove_collision_exception_with(self)
	if is_instance_valid(timer):
		timer.queue_free()

func start_fall() -> void:
	# zabezpieczenie, gdyby _sprite jeszcze nie istniał
	if _sprite == null:
		call_deferred("start_fall")
		return

	_sprite.animation = "fall"
	_sprite.play()

	# mały rozrzut w poziomie
	velocity = Vector2(randf_range(-80, 80), 0) + fall_impulse

	_state = State.FALL

	# na wszelki wypadek włącz fizykę (gdyby była kiedyś wyłączona)
	set_physics_process(true)

	
func _fall_move(delta: float) -> void:
	# 1) grawitacja
	velocity.y = min(velocity.y + gravity * delta, term_velocity)

	# 2) nie zmieniamy velocity.x (zostawiamy początkowy rozrzut)
	move_and_slide()

	# 3) po wylądowaniu przechodzimy do patrolu (lub idling)
	if is_on_floor():
		kill()      # przywraca normalny AI + kolizję
		
# -- Pomocnicze: kompatybilność ze starymi wywołaniami -----------
func has_variable(name: String) -> bool:
	# Przegląda pełną listę właściwości (wbudowanych i ze skryptu)
	for info in get_property_list():
		if "name" in info and info.name == name:
			return true
	return false
