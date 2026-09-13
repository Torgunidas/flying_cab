class_name TaxiNetwork
extends RefCounted
## A small authored corridor graph. Geometry queries validate vehicle clearance.
var stops: Dictionary = {}
var points: Array[Vector3] = []
var edges: Dictionary = {}
var terminals: Dictionary = {}
var space: PhysicsDirectSpaceState3D
var excluded: Array[RID] = []
var edge_cache: Dictionary = {}
var clearance := Vector3(2.5, 0.85, 1.1)

func bind(world: WorldRegistry) -> void:
	edge_cache.clear()
	stops.clear()
	points.clear()
	edges.clear()
	terminals.clear()
	var level := world.map_root() as Node3D
	space = level.get_world_3d().direct_space_state
	excluded.clear()
	for cab: FlightCab in world.vehicles:
		excluded.append(cab.get_rid())
	var spine: Array[int] = []
	for stop: TaxiStop in level.find_children("*", "TaxiStop", true, false):
		if stops.has(stop.stop_id):
			push_error("Duplicate taxi stop: " + stop.stop_id)
			continue
		stops[stop.stop_id] = stop
		var p := stop.global_position
		var terminal := _point(stop.landing_point())
		terminals[stop.stop_id] = terminal
		var approach := _point(Vector3(p.x, p.y + stop.approach_height, 0))
		var junction := _point(Vector3(stop.corridor_x, p.y + stop.approach_height, 0))
		_connect(terminal, approach)
		_connect(approach, junction)
		spine.append(junction)
	spine.sort_custom(func(a: int, b: int): return points[a].y < points[b].y)
	for i in range(1, spine.size()):
		_connect(spine[i - 1], spine[i])

func _point(p: Vector3) -> int:
	points.append(p)
	edges[points.size() - 1] = []
	return points.size() - 1

func _connect(a: int, b: int) -> void:
	edges[a].append(b)
	edges[b].append(a)

func clear_segment(a: Vector3, b: Vector3) -> bool:
	if space == null:
		return false
	var shape := BoxShape3D.new()
	shape.size = clearance
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = shape
	query.transform = Transform3D(Basis.IDENTITY, a)
	query.collision_mask = 1
	query.exclude = excluded
	# cast_motion ignores initial overlaps, so check them explicitly as well.
	if not space.intersect_shape(query, 1).is_empty():
		return false
	query.motion = b - a
	var result := space.cast_motion(query)
	return result.size() == 2 and result[0] >= 0.9999

func path_between(origin: String, destination: String) -> PackedVector3Array:
	if not stops.has(origin) or not stops.has(destination) or not stops[origin].unlocked or not stops[destination].unlocked:
		return []
	return _search(terminals[origin], terminals[destination])

func path_from(position: Vector3, destination: String) -> PackedVector3Array:
	if not stops.has(destination) or not stops[destination].unlocked:
		return []
	# The cab at rest touches its pad. Start guidance just above the surface.
	var start := position + Vector3(0, 0.18, 0)
	var nearest := -1
	var best := INF
	for i in range(points.size()):
		var distance := start.distance_squared_to(points[i])
		if distance < best and clear_segment(start, points[i]):
			var candidate := _search(i, terminals[destination])
			if not candidate.is_empty():
				nearest = i
				best = distance
	if nearest == -1:
		return []
	var route := _search(nearest, terminals[destination])
	# Remove already-passed corners only when the full cab has clear space.
	while route.size() > 1 and clear_segment(start, route[1]):
		route.remove_at(0)
	route.insert(0, position)
	return route

func _search(start: int, target: int) -> PackedVector3Array:
	var open: Array[int] = [start]
	var distance := {start: 0.0}
	var previous := {}
	while not open.is_empty():
		open.sort_custom(func(a: int, b: int): return distance[a] < distance[b])
		var current: int = open.pop_front()
		if current == target:
			var path := PackedVector3Array([points[target]])
			while previous.has(current):
				current = previous[current]
				path.insert(0, points[current])
			return path
		for next: int in edges[current]:
			var key := Vector2i(mini(current, next), maxi(current, next))
			if not edge_cache.has(key):
				edge_cache[key] = clear_segment(points[current], points[next])
			if not edge_cache[key]:
				continue
			var cost: float = distance[current] + points[current].distance_to(points[next])
			if cost < distance.get(next, INF):
				distance[next] = cost
				previous[next] = current
				if not open.has(next):
					open.append(next)
	return []

static func length(path: PackedVector3Array) -> float:
	var total := 0.0
	for i in range(1, path.size()):
		total += path[i - 1].distance_to(path[i])
	return total
