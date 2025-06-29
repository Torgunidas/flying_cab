# res://scripts/vehicle_base.gd
extends CharacterBody2D
class_name VehicleBase      # <- każde auto dziedziczy po tym skrypcie

@export var model_id     : String = ""   # „żółta_taksówka”, „limuzyna”, itp.
@export var instance_id  : String = ""   # wypełnimy automatycznie

# Tajny rejestr wszystkich pojazdów w grze
static var _registry : Dictionary = {}   # { instance_id: self }

func _ready() -> void:
	# 1) upewnij się, że model_id nie jest pusty
	if model_id == "":
		push_error("%s: model_id jest pusty!" % name)

	# 2) nadaj unikalny instance_id, jeśli nie wpisano ręcznie
	if instance_id == "":
		instance_id = "%s_%d" % [model_id, get_instance_id()]

	# 3) sprawdź duplikaty
	if _registry.has(instance_id):
		push_error("Duplicate instance_id: %s!" % instance_id)
	else:
		_registry[instance_id] = self
		add_to_group("vehicles")           # tu, żeby wszystkie typy dziedziczące trafiły do grupy

# Zwraca pojazd po instance_id
static func get_vehicle(id: String) -> VehicleBase:
	return _registry.get(id, null)

# Zwraca listę pojazdów danego modelu
static func get_all_of_model(model: String) -> Array[VehicleBase]:
	return _registry.values().filter(func(v): return v.model_id == model)
