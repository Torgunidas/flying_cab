class_name VehicleBeacons
extends Node3D
## Visual lightbar only. No siren, AI or pursuit behavior; no dynamic lights.
@export_range(0.2, 30.0, 0.1) var cycle_seconds := 8.0
@export_range(0.1, 10.0, 0.1) var active_seconds := 2.4
@export_range(0.02, 1.0, 0.01) var flash_seconds := 0.12
var warmup := false
var elapsed := 0.0
var _materials: Array[StandardMaterial3D] = []

func _ready() -> void:
	for lamp: MeshInstance3D in get_children():
		var material: StandardMaterial3D = lamp.material_override.duplicate()
		lamp.material_override = material
		_materials.append(material)
	update_lamps()

func _process(dt: float) -> void:
	elapsed = fmod(elapsed + dt, maxf(cycle_seconds, 0.2))
	update_lamps()

func update_lamps() -> void:
	for i in range(_materials.size()):
		var lit := warmup or (elapsed < active_seconds and int(elapsed / maxf(flash_seconds, 0.02)) % 4 == i * 2)
		_materials[i].emission_energy_multiplier = 4.0 if lit else 0.0
		_materials[i].albedo_color = Color("83cfff") if lit else Color("163958")
