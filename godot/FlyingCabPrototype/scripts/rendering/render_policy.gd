class_name RenderPolicy
extends RefCounted
## Profiles affect presentation only. UI keeps its full-resolution coordinate space.
const PROFILES := {
	"desktop": {"pixels": 0, "ssao": true, "layers": 3, "headlight_shadows": true},
	"balanced": {"pixels": 921600, "ssao": false, "layers": 3, "headlight_shadows": false},
	"performance": {"pixels": 518400, "ssao": false, "layers": 2, "headlight_shadows": false},
	"quality": {"pixels": 1600000, "ssao": true, "layers": 3, "headlight_shadows": true},
}
var profile := "desktop"

func apply(viewport: Window, level: Node, selected: String) -> void:
	profile = selected if PROFILES.has(selected) else "balanced"
	resize(viewport)
	var settings: Dictionary = PROFILES[profile]
	var environment_node := level.get_node_or_null("WorldEnvironment")
	if environment_node:
		var environment: Environment = environment_node.environment
		environment.ssao_enabled = settings.ssao
		environment.glow_enabled = true
	var banks := level.get_tree().get_nodes_in_group("smog_bank")
	var index := 0
	for bank in banks:
		if level.is_ancestor_of(bank):
			bank.visible = index < int(settings.layers)
			index += 1
	for actor in level.context.world.vehicles:
		if actor is FlightCab:
			actor.get_node("Headlights").set_shadow_quality(settings.headlight_shadows)

func resize(viewport: Window) -> void:
	var pixels: int = PROFILES[profile].pixels
	viewport.scaling_3d_scale = scale_for(viewport.size, pixels)

static func scale_for(size: Vector2i, budget: int) -> float:
	if budget <= 0 or size.x <= 0 or size.y <= 0:
		return 1.0
	return clampf(sqrt(float(budget) / (float(size.x) * size.y)), 0.1, 1.0)

func budget_vehicle_lights(level: Node, focus: Node3D) -> void:
	if not is_instance_valid(focus):
		return
	var candidates: Array[FlightCab] = []
	for actor in level.context.world.vehicles:
		if actor is FlightCab:
			candidates.append(actor)
	candidates.sort_custom(func(a, b): return a.global_position.distance_squared_to(focus.global_position) < b.global_position.distance_squared_to(focus.global_position))
	for i in range(candidates.size()):
		var actor := candidates[i]
		var lights := actor.get_node("Headlights")
		lights.set_shadow_quality(PROFILES[profile].headlight_shadows)
		lights.set_detail_enabled(i < 2 and actor.global_position.distance_squared_to(focus.global_position) <= 1600.0)
