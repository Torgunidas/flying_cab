extends Node3D
## Ground smog opens into the softly lit upper city. Geometry is saved in the scene.
@export var profile: Resource = preload("res://resources/smog.tres")
var focus: Node3D
@onready var environment: Environment = get_parent().get_node("WorldEnvironment").environment
@onready var key: DirectionalLight3D = get_parent().get_node("Moonlight")
var _elapsed := 0.0

func _ready() -> void:
	environment.fog_height = profile.top_height - 9.0
	environment.fog_height_density = 0.007
	environment.fog_mode = Environment.FOG_MODE_DEPTH
	for bank in get_tree().get_nodes_in_group("smog_bank"):
		bank.material_override.set_shader_parameter("top_height",profile.top_height)
		bank.material_override.set_shader_parameter("feather",profile.feather)
		bank.material_override.set_shader_parameter("tint",profile.fog_color)

func _process(dt: float) -> void:
	_elapsed += dt
	if _elapsed < 0.1:
		return
	_elapsed = 0.0
	if is_instance_valid(focus):
		apply_height(focus.global_position.y)

func set_focus(target: Node3D) -> void:
	focus = target
	if is_instance_valid(focus):
		apply_height(focus.global_position.y)

func apply_height(height: float) -> void:
	var upper := smoothstep(140.0, 190.0, height)
	environment.background_color = Color("101726").lerp(Color("596e8a"), upper)
	environment.ambient_light_color = Color("747caf").lerp(Color("cbdde8"), upper)
	environment.ambient_light_energy = lerpf(0.40, 0.60, upper)
	environment.fog_light_color = Color("292039").lerp(Color("8294ad"), upper)
	environment.fog_density = lerpf(0.65,0.35,upper)
	key.light_color = Color("c3badb").lerp(Color("ffe1cc"), upper)
	key.light_energy = lerpf(0.45, 0.65, upper)
	var smog: float = profile.density_at(height)
	environment.background_color = environment.background_color.lerp(Color("08181e"),smog)
	environment.ambient_light_color = environment.ambient_light_color.lerp(Color("759da2"),smog)
	environment.ambient_light_energy = lerpf(environment.ambient_light_energy,0.19,smog)
	environment.fog_light_color = environment.fog_light_color.lerp(profile.fog_color,smog)
	# The side camera sits 19 m away: keep nearby surfaces readable while distant
	# silhouettes vanish, instead of fogging the cab as if it were far scenery.
	environment.fog_depth_begin = lerpf(0.0,16.0,smog)
	environment.fog_depth_end = lerpf(160.0,60.0,smog)
	environment.fog_depth_curve = lerpf(0.8,1.35,smog)
	environment.fog_density = lerpf(environment.fog_density,0.97,smog)
	key.light_color = key.light_color.lerp(Color("779fa5"),smog)
	key.light_energy = lerpf(key.light_energy,0.14,smog)
