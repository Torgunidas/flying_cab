extends Node2D
class_name TaxiMarker            # musi być! (używa go Car.gd)

var car    : Node2D
var target : Node2D

@onready var pointer   : Node2D = $"Pointer"     # ← NOWE
@onready var fare_label: Label  = $"FareLabel"   # ← już było (jeśli dodałeś)

func update_fare_label(amount: int) -> void:
	fare_label.text = "$ %d" % amount

func _process(_delta):
	# 1) Czy obiekty nadal istnieją?
	if !is_instance_valid(car) or !is_instance_valid(target):
		queue_free(); return

	# 2) Przyklej marker nad autem
	global_position = car.global_position + Vector2(0, -48)

	# 3) Obróć TYLKO `Pointer` w stronę celu
	if is_instance_valid(pointer):
		pointer.look_at(target.global_position)   # root NIE obraca się
