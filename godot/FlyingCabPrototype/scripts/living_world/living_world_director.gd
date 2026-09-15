class_name LivingWorldDirector
extends Node3D
## Map-scoped population. Persistent plans live in RuntimeContext.living; this
## adapter owns only nodes, paths and NPC controllers. No spawning during travel.
signal vehicle_taken(vehicle: FlightCab, owner_id: StringName)
@export var profile: LivingWorldProfile = preload("res://resources/living_world/city_02.tres")
var context: RuntimeContext
var network := TaxiNetwork.new()
var cars: Dictionary = {}
var people: Dictionary = {}
var drivers: Dictionary = {}
var routes: Dictionary = {}
var schedules: Dictionary = {}
var journeys: Dictionary = {}
var initialized := false
var _held: Dictionary = {}
var _obstacle_query := PhysicsShapeQueryParameters3D.new()
var _obstacle_shape := BoxShape3D.new()
var _tick := 0
var _junction_owners: Dictionary = {}
var _junction_wait: Dictionary = {}
var _queue_since: Dictionary = {}

func initialize() -> void:
	if initialized:
		return
	network.bind(context.world)
	var errors := profile.validation_errors(context.vehicle_catalog, network.stops)
	if not errors.is_empty():
		push_error("Cannot prepare living world: " + "; ".join(errors))
		return
	for route in profile.routes:
		routes[route.route_id] = route
	for schedule in profile.residents:
		schedules[schedule.resident_id] = schedule
	if not context.living.initialized_maps.has(profile.map_id):
		_seed()
		context.living.initialized_maps.append(profile.map_id)
	context.world.living_world = self
	for id: String in context.living.agents:
		var item: Dictionary = context.living.agents[id]
		if item.map != profile.map_id:
			continue
		if item.kind != "walker":
			_spawn_car(id, item)
		if item.kind != "loop":
			var person := TaxiDirector.PEOPLE[item.look].instantiate() as PassengerVisual
			add_child(person)
			person.position = vector(item.person)
			people[id] = person
			person.visible = item.phase not in ["inside", "flying"]
		if item.kind == "resident" and item.phase == "flying" and not item.claimed:
			_build_journey(id, item, true)
	initialized = true
	context.player.control_suspended.connect(_suspended)
	_suspended(context.player.is_suspended())

func _seed() -> void:
	for car: FlightCab in context.world.vehicles:
		car.capture_state()
	for route in profile.routes:
		for i in range(route.population):
			var id := "traffic/" + route.route_id + "/%02d" % i
			var sample := route.sample(route.length() * (i + 0.35) / route.population)
			var item := _agent("loop", id, route.route_id, "flying", i % 4)
			item.waypoint = sample.next
			context.living.agents[id] = item
			_seed_vehicle(id, route.models[i % route.models.size()], sample.position)
	for schedule in profile.residents:
		if schedule.stops.is_empty() or not network.stops.has(schedule.stops[0]):
			push_error("Missing resident home: " + schedule.resident_id)
			continue
		var id := "resident/" + schedule.resident_id
		var stop: TaxiStop = network.stops[schedule.stops[0]]
		var model := context.vehicle_catalog.find_model(StringName(schedule.model_id))
		var placement := _initial_berth(stop, schedule, model)
		if placement.is_empty():
			push_warning("No free initial berth for resident: " + id)
			continue
		var item := _agent("parked" if schedule.parked_only else "resident", id, schedule.resident_id, "walking" if schedule.parked_only else "inside", schedule.look)
		item.person = array(stop.door_point())
		item.target = array(stop.waiting_point())
		item.timer = 4.0 + schedule.look * 2.0
		context.living.agents[id] = item
		_seed_vehicle(id, schedule.model_id, placement.position)
	for stop: TaxiStop in network.stops.values():
		for i in range(profile.walkers_per_platform):
			var id := "walker/" + stop.stop_id + "/%02d" % i
			var look := posmod(id.hash(), 4)
			var item := _agent("walker", "", stop.stop_id, "walking", look)
			item.person = array(stop.door_point().lerp(stop.waiting_point(), float(posmod(id.hash(), 80)) / 100.0))
			item.target = array(stop.waiting_point())
			context.living.agents[id] = item

func _agent(kind: String, vehicle: String, route: String, phase: String, look: int) -> Dictionary:
	return {"kind": kind, "map": profile.map_id, "vehicle": vehicle, "route": route, "phase": phase, "waypoint": 0, "stop": 0, "laps": 0, "look": look, "timer": 0.0, "claimed": false, "person": [0.0, 0.0, WorldLayers.PEDESTRIAN_Z], "target": [0.0, 0.0, WorldLayers.PEDESTRIAN_Z]}

func _seed_vehicle(id: String, model_id: String, at: Vector3) -> void:
	if context.vehicles.has(StringName(id)):
		return
	var model := context.vehicle_catalog.find_model(StringName(model_id))
	var state := VehicleState.new()
	state.entity_id = id
	state.owner_id = id
	state.map_id = profile.map_id
	state.model_id = model_id
	state.fuel = model.fuel_capacity * 0.75
	state.position = at
	state.has_pose = true
	context.vehicles[state.entity_id] = state

func _spawn_car(id: String, item: Dictionary) -> void:
	var saved: VehicleState = context.vehicles[StringName(item.vehicle)]
	var car := preload("res://scenes/cab.tscn").instantiate() as FlightCab
	car.entity_id = saved.entity_id
	car.initial_owner_id = saved.owner_id
	car.definition = context.vehicle_catalog.find_model(saved.model_id)
	car.position = saved.position
	add_child(car)
	context.world.register(car) # Bind saved state before any controller or first tick.
	cars[id] = car
	car.get_node("Headlights").set_detail_enabled(false)
	if item.phase == "flying" and not item.claimed and not car.disabled:
		_assign_driver(id, car)

func _assign_driver(id: String, car: FlightCab) -> void:
	if drivers.has(id):
		drivers[id].detach()
	var driver := TrafficDriver.new()
	driver.bind(car, StringName(id))
	drivers[id] = driver

func _physics_process(dt: float) -> void:
	advance(dt)

func advance(dt: float) -> void:
	if not initialized or context.player.is_suspended():
		return
	_tick += 1
	_update_junctions()
	for id: String in context.living.agents:
		var item: Dictionary = context.living.agents[id]
		if item.map != profile.map_id:
			continue
		if item.kind != "walker":
			var car: FlightCab = cars[id]
			if not item.claimed and car.get_driver_id() == context.player.actor_id:
				take_vehicle(car)
			if item.claimed:
				if people.has(id):
					_wander(id, item, dt)
				continue
			if car.disabled:
				if drivers.has(id):
					_release_driver(id)
				continue
		if item.kind == "loop":
			_loop(id, item, dt)
		elif item.kind == "resident":
			_resident(id, item, dt)
		else:
			_wander(id, item, dt)

func _loop(id: String, item: Dictionary, dt: float) -> void:
	if not routes.has(item.route) or not drivers.has(id):
		return
	var route: LivingRoute = routes[item.route]
	var car: FlightCab = cars[id]
	var target: Vector3 = route.points[int(item.waypoint) % route.points.size()]
	if car.global_position.distance_to(target) < 0.65:
		item.waypoint = (int(item.waypoint) + 1) % route.points.size()
		if item.waypoint == 0:
			item.laps += 1
		target = route.points[item.waypoint]
	var driver: TrafficDriver = drivers[id]
	driver.steer(target, route.speed, dt, _junction_wait.has(id) or _blocked(car, target))

func _update_junctions() -> void:
	_junction_wait.clear()
	for index in range(profile.junctions.size()):
		var center: Vector3 = profile.junctions[index]
		var junction_owner: String = _junction_owners.get(index, "")
		if not junction_owner.is_empty():
			var car: FlightCab = cars.get(junction_owner)
			if not is_instance_valid(car) or car.disabled or not drivers.has(junction_owner) or car.global_position.distance_to(center) > profile.junction_approach + 3:
				_junction_owners.erase(index)
				_queue_since.erase(junction_owner)
				junction_owner = ""
		var candidates: Array[String] = []
		for id: String in drivers:
			if context.living.agents[id].kind != "loop":
				continue
			var car: FlightCab = cars[id]
			if car.global_position.distance_to(center) <= profile.junction_approach:
				candidates.append(id)
				if not _queue_since.has(id):
					_queue_since[id] = _tick
		if junction_owner.is_empty() and not candidates.is_empty():
			candidates.sort_custom(func(a: String, b: String):
				# Clear anything already within the junction first; otherwise FIFO.
				var a_inside: bool = cars[a].global_position.distance_to(center) < 7
				var b_inside: bool = cars[b].global_position.distance_to(center) < 7
				if a_inside != b_inside:
					return a_inside
				if _queue_since[a] != _queue_since[b]:
					return _queue_since[a] < _queue_since[b]
				return cars[a].global_position.distance_squared_to(center) < cars[b].global_position.distance_squared_to(center))
			junction_owner = candidates[0]
			_junction_owners[index] = junction_owner
		for id in candidates:
			if id != junction_owner:
				_junction_wait[id] = true

func _blocked(car: FlightCab, target: Vector3) -> bool:
	# One short sweep per moving car; geometry was validated at authoring/test.
	# Yield only to something ahead, never to pedestrians on the terrace strip.
	var direction := (target - car.global_position).normalized()
	if direction.length_squared() < 0.1:
		return false
	_obstacle_shape.size = car.definition.collision_size + Vector3(0.3, 0.25, 0)
	_obstacle_query.shape = _obstacle_shape
	_obstacle_query.collision_mask = WorldLayers.VEHICLES
	_obstacle_query.exclude = [car.get_rid()]
	_obstacle_query.transform = Transform3D(Basis.IDENTITY, car.global_position + car.definition.collision_offset)
	_obstacle_query.motion = direction * (1.3 + car.linear_velocity.length() * 0.7)
	var sweep := network.space.cast_motion(_obstacle_query)
	return sweep.size() == 2 and sweep[0] < 0.999

func _resident(id: String, item: Dictionary, dt: float) -> void:
	var schedule: ResidentSchedule = schedules.get(item.route)
	if schedule == null:
		return
	var stop: TaxiStop = network.stops[schedule.stops[int(item.stop) % schedule.stops.size()]]
	var car: FlightCab = cars[id]
	if item.phase == "inside":
		people[id].hide()
		item.timer = maxf(0, item.timer - dt)
		if item.timer <= 0 and car.grounded and car.sleeping:
			item.phase = "boarding"
			item.person = array(stop.door_point())
			item.target = array(Vector3(car.global_position.x, stop.global_position.y, WorldLayers.PEDESTRIAN_Z))
			people[id].reset_gait()
	elif item.phase == "boarding":
		if _walk(id, item, dt):
			people[id].hide()
			_assign_driver(id, car)
			item.phase = "flying"
			item.stop = (int(item.stop) + 1) % schedule.stops.size()
			_build_journey(id, item)
	elif item.phase == "flying":
		var path: PackedVector3Array = journeys.get(id, PackedVector3Array())
		if path.is_empty() or not drivers.has(id):
			return
		var last := int(item.waypoint) >= path.size() - 1
		var target: Vector3 = path[mini(item.waypoint, path.size() - 1)]
		if last and not _berth_clear(car, target):
			drivers[id].steer(target + Vector3.UP * 5, profile.residential_speed, dt, _blocked(car, target + Vector3.UP * 5))
			return
		if last and car.grounded and car.support_body == stop.get_parent():
			drivers[id].stop_on_ground()
			if car.sleeping and car.linear_velocity.length() < 0.1:
				_release_driver(id)
				item.phase = "alighting"
				item.laps += 1
				item.person = array(Vector3(car.global_position.x, stop.global_position.y, WorldLayers.PEDESTRIAN_Z))
				item.target = array(stop.door_point())
				people[id].reset_gait()
			return
		if not last and car.global_position.distance_to(target) < 0.5:
			item.waypoint += 1
			target = path[item.waypoint]
		# Aim a few centimetres into support so contact can settle into sleep.
		drivers[id].steer(target - (Vector3.UP * 0.08 if last else Vector3.ZERO), 1.3 if last else profile.residential_speed, dt, false if last else _blocked(car, target))
	elif item.phase == "alighting":
		if _walk(id, item, dt):
			item.phase = "inside"
			item.timer = schedule.dwell_seconds
			people[id].hide()

func _build_journey(id: String, item: Dictionary, restoring := false) -> void:
	var schedule: ResidentSchedule = schedules[item.route]
	var destination: TaxiStop = network.stops[schedule.stops[int(item.stop) % schedule.stops.size()]]
	var car: FlightCab = cars[id]
	var finish := _berth(destination, schedule, car.definition)
	var corridor := schedule.corridor_x
	var start := car.global_position
	if restoring:
		var origin: TaxiStop = network.stops[schedule.stops[posmod(int(item.stop) - 1, schedule.stops.size())]]
		start = _berth(origin, schedule, car.definition)
	var departure_height := start.y + 5
	journeys[id] = PackedVector3Array([Vector3(start.x, departure_height, 0), Vector3(corridor, departure_height, 0), Vector3(corridor, finish.y + 5, 0), Vector3(finish.x, finish.y + 5, 0), finish])
	item.waypoint = mini(item.waypoint, journeys[id].size() - 1) if restoring else 0

func _berth(stop: TaxiStop, schedule: ResidentSchedule, model: VehicleDefinition) -> Vector3:
	return stop.global_position + Vector3(schedule.berth_x, model.collision_size.y * 0.5 - model.collision_offset.y, 0)

func _initial_berth(stop: TaxiStop, schedule: ResidentSchedule, model: VehicleDefinition) -> Dictionary:
	# When upgrading an older save, Ari may already occupy the authored NPC slot.
	var at := _berth(stop, schedule, model) + Vector3.UP * 0.03
	for x: float in [schedule.berth_x, -schedule.berth_x, 0.0]:
		at.x = stop.global_position.x + x
		var candidate := AABB(at + model.collision_offset - model.collision_size * 0.5, model.collision_size).grow(0.1)
		var clear := true
		for state: VehicleState in context.vehicles.values():
			if not state.has_pose or state.map_id != StringName(profile.map_id):
				continue
			var other := context.vehicle_catalog.find_model(state.model_id)
			var bounds := AABB(state.position + other.collision_offset - other.collision_size * 0.5, other.collision_size)
			if candidate.intersects(bounds):
				clear = false
				break
		if clear:
			return {"position": at}
	return {}

func _berth_clear(car: FlightCab, at: Vector3) -> bool:
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = car.get_node("Collision").shape
	query.transform = Transform3D(Basis.IDENTITY, at + car.definition.collision_offset)
	query.collision_mask = WorldLayers.VEHICLES
	query.exclude = [car.get_rid()]
	return network.space.intersect_shape(query, 1).is_empty()

func _release_driver(id: String) -> void:
	var driver: TrafficDriver = drivers.get(id)
	if driver:
		driver.detach()
		drivers.erase(id)
	var car: FlightCab = cars[id]
	car.assign_driver(&"")

func _wander(id: String, item: Dictionary, dt: float) -> void:
	var stop_id: String = item.route if item.kind == "walker" else schedules[item.route].stops[int(item.stop) % schedules[item.route].stops.size()]
	var stop: TaxiStop = network.stops.get(stop_id)
	if stop == null:
		return
	if item.phase == "stranded":
		item.target = array(stop.door_point())
		if _walk(id, item, dt):
			item.phase = "inside"
			item.timer = 5.0
	elif item.phase in ["inside", "parked"]:
		people[id].visible = item.phase != "inside"
		item.timer = maxf(0, item.timer - dt)
		if item.timer <= 0:
			item.target = array(stop.waiting_point() if item.phase == "inside" else stop.door_point())
			item.phase = "walking"
	else:
		if _walk(id, item, dt):
			var at_door := vector(item.person).distance_to(stop.door_point()) < 0.1
			item.phase = "inside" if at_door else "parked"
			item.timer = 2.0 + item.look * 0.5
			people[id].visible = not at_door
	if item.phase == "parked":
		people[id].pose(dt, Vector3.ZERO, false, 0)

func _walk(id: String, item: Dictionary, dt: float) -> bool:
	var person: PassengerVisual = people[id]
	var before := vector(item.person)
	var target := vector(item.target)
	var at := before.move_toward(target, profile.walking_speed * dt)
	item.person = array(at)
	person.show()
	person.global_position = at
	person.pose(dt, at - before, false, signf(target.x - before.x))
	return at.distance_to(target) < 0.001

func can_take_vehicle(car: FlightCab) -> bool:
	var item: Dictionary = context.living.agents.get(String(car.entity_id), {})
	return not item.is_empty() and item.map == profile.map_id and car.get_driver_id() == &"" and car.grounded and car.sleeping

func recovery_berth(car: FlightCab) -> Dictionary:
	# The authored player spawn may now be occupied by another persistent car.
	# Prefer the depot, then a vacant terrace, with clearance for this model.
	var stops: Array = network.stops.values()
	stops.sort_custom(func(a: TaxiStop, b: TaxiStop):
		return a.stop_id == "depot" and b.stop_id != "depot")
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = car.get_node("Collision").shape
	query.collision_mask = WorldLayers.GEOMETRY | WorldLayers.VEHICLES
	query.exclude = [car.get_rid()]
	for stop: TaxiStop in stops:
		var side := stop.half_width - car.definition.collision_size.x * 0.5 - 0.2
		if side < 0:
			continue
		for x: float in [0.0, -side, side]:
			var point := stop.global_position + Vector3(x, car.definition.collision_size.y * 0.5 - car.definition.collision_offset.y + 0.06, 0)
			query.transform = Transform3D(Basis.IDENTITY, point + car.definition.collision_offset)
			if network.space.intersect_shape(query, 1).is_empty():
				return {"position": point, "name": stop.display_name}
	return {}

func take_vehicle(car: FlightCab) -> void:
	var id := String(car.entity_id)
	var item: Dictionary = context.living.agents.get(id, {})
	if item.is_empty() or item.claimed:
		return
	item.claimed = true
	if drivers.has(id):
		drivers[id].detach()
		drivers.erase(id)
	if item.kind != "loop":
		item.phase = "stranded"
		people[id].show()
	vehicle_taken.emit(car, car.state.owner_id)

func _suspended(value: bool) -> void:
	for id in cars:
		var car: FlightCab = cars[id]
		if value and context.player.focus != car:
			if not _held.has(id):
				_held[id] = {"freeze": car.freeze, "velocity": car.linear_velocity}
			car.freeze = true
		elif not value and _held.has(id):
			car.freeze = _held[id].freeze
			car.linear_velocity = _held[id].velocity
			_held.erase(id)

func _exit_tree() -> void:
	for driver: TrafficDriver in drivers.values():
		driver.detach()
	if context and context.player.control_suspended.is_connected(_suspended):
		context.player.control_suspended.disconnect(_suspended)
	if context and context.world.living_world == self:
		context.world.living_world = null

static func array(v: Vector3) -> Array:
	return [v.x, v.y, v.z]

static func vector(v: Array) -> Vector3:
	return Vector3(v[0], v[1], v[2])
