extends Node2D

@export var passenger_scene : PackedScene        # ← przeciągnij NPCPassenger.tscn
@export var spawn_interval  : float = 15.0       # stała przerwa pomiędzy próbami spawnu

#  ─────  NOWE  ─────
@export var wait_time_min  : float = 12.0        # min. ile sekund będzie machać ręką
@export var wait_time_max  : float = 25.0        # max. ile sekund

var _active_passenger : Node2D = null            # referencja na żyjącego pasażera
var _rng := RandomNumberGenerator.new()

@onready var _timer   : Timer    = $SpawnTimer
@onready var _spawn   : Marker2D = $"SpawnPos"
@onready var _waypt   : Marker2D = $"Waypoint"
@export var delivery_groups : Array[NodePath] = []

func _ready() -> void:
	_rng.randomize()
	_timer.wait_time = spawn_interval
	_timer.timeout.connect(_on_SpawnTimer_timeout)


func _on_SpawnTimer_timeout() -> void:
	# 1) jeśli ktoś już stoi przy tym punkcie – nic nowego nie tworzymy
	if is_instance_valid(_active_passenger):
		return

	# 2) instancjonujemy pasażera
	if passenger_scene == null:
		return
	var npc = passenger_scene.instantiate()
	add_child(npc)
	npc.delivery_groups = delivery_groups.duplicate()
	npc.global_position = _spawn.global_position
	_active_passenger  = npc
	npc.tree_exited.connect(_on_passenger_freed)   # zjawi się, gdy queue_free()

	# 3) losowy timeout na machanie ręką
	var rand_wait := _rng.randf_range(wait_time_min, wait_time_max)

	# 4) uruchamiamy sekwencję: idź → idle → czekaj rand_wait
	npc.start_spawn_sequence(
		_waypt.global_position,
		rand_wait,            # ← teraz drugi argument to od razu taxi_wait_time
		_spawn.global_position
	)

func _on_passenger_freed() -> void:
	_active_passenger = null    # pozwoli Timerowi stworzyć kolejnego
