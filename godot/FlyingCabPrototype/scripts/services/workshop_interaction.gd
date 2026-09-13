class_name WorkshopInteraction
extends RefCounted
## Authorizes the focused driver at a real workshop; UI only expresses intent.
var station: RepairStation
var repairing := false

func advance(context: RuntimeContext, requested: bool, dt: float) -> void:
	station = null
	repairing = false
	var vehicle := context.player.focus as FlightCab
	if vehicle == null or context.player.is_suspended() or vehicle.state.driver_id != context.player.actor_id:
		return
	for candidate: RepairStation in context.world.workshops:
		if is_instance_valid(candidate) and candidate.can_service(vehicle):
			station = candidate
			break
	if station == null or not requested or not is_finite(dt) or dt <= 0.0:
		return
	var service: VehicleService = context.systems.get(&"repair")
	if service:
		repairing = service.repair(vehicle.state, vehicle.definition, context.campaign, station.hull_per_second * minf(dt, 0.1), station.price_per_hull, context.ledger) > 0.0
