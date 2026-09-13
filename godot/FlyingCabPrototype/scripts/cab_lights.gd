extends Node3D
## Separate presentation component: never writes thrust, velocity or fuel.
@export var profile: Resource = preload("res://resources/smog.tres")
var lights_on := false
var light_amount := 0.0
var _lens_material: StandardMaterial3D
var _detail_enabled := true
var _last_height := INF
var _last_rotation := Vector3(INF, INF, INF)
@onready var cab: FlightCab = get_parent()
@onready var beam: MeshInstance3D = $Beam
@onready var spots: Array[SpotLight3D] = [$Left, $Right]
@onready var spill: OmniLight3D = $Spill

func _ready() -> void:
	beam.material_override = beam.material_override.duplicate()
	bind_visual()
	cab.flight_reset.connect(func(): update_lights(cab.global_position.y,0.0,true))

func bind_visual() -> void:
	_lens_material = null
	var origin := cab.definition.headlight_origin
	for i in range(spots.size()):
		spots[i].position = origin + Vector3(0, 0, (1 if i == 0 else -1) * minf(0.3, cab.definition.collision_size.z / 3))
	beam.position = origin + Vector3(8, -0.7, 0)
	spill.position = origin + Vector3(-0.02, 0.02, 0)
	for child in cab.get_node("Visual").get_children():
		if child is MeshInstance3D and child.name.begins_with("Head"):
			if not _lens_material:
				_lens_material = child.material_override.duplicate()
			child.material_override = _lens_material
	update_lights(cab.global_position.y, 0.0, true)
	_last_height = INF

func update_lights(height: float, dt: float, immediate := false) -> void:
	if immediate:
		lights_on = height <= profile.lights_on_height
	elif height <= profile.lights_on_height:
		lights_on = true
	elif height >= profile.lights_off_height:
		lights_on = false
	var target := 1.0 if lights_on else 0.0
	light_amount = target if immediate else move_toward(light_amount,target,dt/maxf(profile.light_fade_seconds,0.01))
	for spot in spots:
		spot.visible = light_amount > 0.001 and _detail_enabled
		spot.light_energy = light_amount * 5.0
	spill.visible = light_amount > 0.001 and _detail_enabled
	spill.light_energy = light_amount * 1.4
	var fog_amount: float = profile.density_at(height)
	beam.visible = light_amount * fog_amount > 0.001
	beam.material_override.set_shader_parameter("intensity",light_amount * fog_amount)
	if _lens_material:
		_lens_material.albedo_color = Color("44565d").lerp(Color("e4fff0"),light_amount)
		_lens_material.emission_energy_multiplier = light_amount * 3.0

func _physics_process(dt: float) -> void:
	var desired := Vector3(0, PI if cab.visual.scale.x < 0 else 0.0, cab.visual.rotation.z)
	var height := cab.global_position.y
	var settled := is_equal_approx(light_amount, 1.0 if lights_on else 0.0)
	if settled and is_equal_approx(_last_height, height) and _last_rotation.is_equal_approx(desired):
		return
	rotation = desired
	_last_rotation = desired
	_last_height = height
	update_lights(height, dt)

func set_shadow_quality(enabled: bool) -> void:
	for spot in spots:
		if spot.shadow_enabled != enabled:
			spot.shadow_enabled = enabled

func set_detail_enabled(enabled: bool) -> void:
	if enabled == _detail_enabled:
		return
	_detail_enabled = enabled
	_last_height = INF
