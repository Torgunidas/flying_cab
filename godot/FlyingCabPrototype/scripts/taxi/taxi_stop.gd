class_name TaxiStop
extends Node3D
## Author under the actual pad; the local path stays on its pedestrian strip.
@export var stop_id := "depot"
@export var display_name := "DEPOT"
@export var district := "Velvet"
@export var unlocked := true
@export var fuel_service := false
@export var half_width := 5.0
@export var waiting_x := -3.8
@export var door_x := 4.0
@export var corridor_x := 0.0
@export var approach_height := 5.0

func _ready() -> void:
	add_to_group("taxi_stop")
	set_meta("entity_id", "taxi_stop/" + stop_id)

func waiting_point(index := 0) -> Vector3:
	return to_global(Vector3(waiting_x + index * 0.65, 0, 1.2))

func door_point() -> Vector3:
	return to_global(Vector3(door_x, 0, 1.2))

func landing_point() -> Vector3:
	return to_global(Vector3(0, 0.9, 0))

func can_board(vehicle: FlightCab) -> bool:
	return boarding_block(vehicle).is_empty()

func boarding_block(vehicle: FlightCab) -> String:
	if not is_instance_valid(vehicle):
		return "WRÓĆ DO TAKSÓWKI"
	if vehicle.disabled or vehicle.resetting or vehicle.airspace.returning:
		return "PRZYWRÓĆ SPRAWNOŚĆ I KONTROLĘ AUTA"
	if not vehicle.grounded or vehicle.support_body != get_parent():
		return "WYLĄDUJ NA WSKAZANYM TARASIE"
	if not vehicle.command.is_zero_approx():
		return "PUŚĆ PRZYCISKI LOTU"
	if not vehicle.sleeping:
		return "ZACZEKAJ, AŻ AUTO SIĘ ZATRZYMA"
	# Use the actual cab collider, not a fixed inset that rejected fully parked
	# cars at the depot. The body may rest on a pad while partly over its edge.
	var collision := vehicle.get_node_or_null("Collision") as CollisionShape3D
	var extent := vehicle.definition.navigation_clearance.x * 0.5
	var center := to_local(vehicle.global_position).x
	if collision and collision.shape is BoxShape3D:
		var box := collision.shape as BoxShape3D
		var local := global_transform.affine_inverse() * collision.global_transform
		center = local.origin.x
		extent = (absf(local.basis.x.x) * box.size.x + absf(local.basis.y.x) * box.size.y + absf(local.basis.z.x) * box.size.z) * 0.5
	if absf(center) + extent > half_width + 0.005:
		return "PRZESUŃ AUTO BLIŻEJ ŚRODKA TARASU"
	return ""

func exit_point(vehicle: FlightCab, index := 0) -> Vector3:
	var x := to_local(vehicle.global_position).x
	# Pick the side with room for the whole party, retaining a clear walking strip.
	var side := -1.0 if x > 0.0 else 1.0
	return to_global(Vector3(clampf(x + side * (1.55 + index * 0.55), -half_width + 0.3, half_width - 0.3), 0, 1.2))
