class_name PassengerVisual
extends Node3D
## Presentation only: one baked skinned mesh, no passenger AI tick.
## A full left/right cycle covers stride_length metres. Feet move backwards
## at exactly the body's speed during stance; knees and ankles solve the reach.
@export_range(0.6, 1.5, 0.05) var stride_length := 0.95
@export_range(0.35, 0.65, 0.01) var stance_fraction := 0.56
@export_range(0.04, 0.22, 0.01) var step_height := 0.10
@onready var rig: Skeleton3D = $Skeleton3D
var _time := 0.0
var _gait_phase := 0.0
var _gait_weight := 0.0

func reset_gait() -> void:
	_gait_phase = 0.0
	_gait_weight = 0.0
	if is_instance_valid(rig):
		rig.reset_bone_poses()

func pose(dt: float, travel: Vector3, waving: bool, direction: float) -> void:
	_time += dt
	var distance := Vector2(travel.x, travel.z).length()
	var walking := dt > 0.0 and distance > 0.00001
	if walking:
		# Face displacement, including braking and diagonal approaches to doors.
		rotation.y = atan2(travel.x, travel.z)
		var model_scale := maxf(global_basis.get_scale().x, 0.001)
		_gait_phase = fposmod(_gait_phase + distance / (stride_length * model_scale), 1.0)
	else:
		rotation.y = lerp_angle(rotation.y, PI * 0.5 * signf(direction) if absf(direction) > 0.01 else 0.0, minf(1, dt * 16))
	_gait_weight = move_toward(_gait_weight, 1.0 if walking else 0.0, dt * 12.0)
	var jogging := stance_fraction < 0.5
	# Rise over the supporting ankle; lower the pelvis as the feet separate.
	var pulse := pow(sin((_gait_phase - (stance_fraction - 0.5) * 0.5) * TAU), 2.0)
	var moving_hip := 0.515 + pulse * 0.070 if jogging else 0.528 + pulse * 0.068
	var hip_y := lerpf(HumanRig.HIP_Y, moving_hip, _gait_weight)
	rig.set_bone_pose_position(HumanRig.PELVIS, Vector3(0, hip_y, 0))
	_pose_leg(HumanRig.LEFT_THIGH, _gait_phase, hip_y)
	_pose_leg(HumanRig.RIGHT_THIGH, fposmod(_gait_phase + 0.5, 1.0), hip_y)
	var swing := cos(_gait_phase * TAU) * _gait_weight
	rig.set_bone_pose_rotation(HumanRig.LEFT_ARM, Quaternion(Vector3.RIGHT, swing * 0.50))
	rig.set_bone_pose_rotation(HumanRig.RIGHT_ARM, Quaternion(Vector3.RIGHT, -swing * 0.50))
	var elbow := -0.25 - (0.85 if jogging else 0.20) * _gait_weight
	rig.set_bone_pose_rotation(HumanRig.LEFT_FOREARM, Quaternion(Vector3.RIGHT, elbow))
	rig.set_bone_pose_rotation(HumanRig.RIGHT_FOREARM, Quaternion(Vector3.RIGHT, elbow))
	if waving:
		rig.set_bone_pose_rotation(HumanRig.RIGHT_ARM, Quaternion(Vector3.BACK, 2.1))
		rig.set_bone_pose_rotation(HumanRig.RIGHT_FOREARM, Quaternion(Vector3.BACK, 0.55 + sin(_time * 5) * 0.18))
	var lean := (0.11 if jogging else 0.04) * _gait_weight
	rig.set_bone_pose_rotation(HumanRig.SPINE, Quaternion(Vector3.RIGHT, lean))
	rig.set_bone_pose_rotation(HumanRig.HEAD, Quaternion.from_euler(Vector3(-lean * 0.5, sin(_time * 0.8) * 0.06, 0)))

func _pose_leg(thigh: int, phase: float, hip_y: float) -> void:
	var reach := stride_length * stance_fraction * 0.5
	var forward: float
	var lift := 0.0
	var pitch := 0.0
	if phase < stance_fraction:
		forward = lerpf(reach, -reach, phase / stance_fraction)
	else:
		var t := (phase - stance_fraction) / (1.0 - stance_fraction)
		forward = lerpf(-reach, reach, smoothstep(0.0, 1.0, t))
		lift = sin(PI * t) * step_height
		pitch = sin(PI * t) * 0.20
	_solve_leg(thigh, hip_y, Vector2(HumanRig.ANKLE_Y + lift * _gait_weight, forward * _gait_weight), pitch * _gait_weight)

func _solve_leg(thigh: int, hip_y: float, ankle: Vector2, foot_pitch := 0.0) -> void:
	# Coordinates are (vertical, forward). Choose the forward-bending knee.
	var delta := ankle - Vector2(hip_y, 0)
	var upper := HumanRig.THIGH_LENGTH
	var lower := HumanRig.SHIN_LENGTH
	var distance := clampf(delta.length(), 0.001, upper + lower - 0.000001)
	var aim := atan2(-delta.y, -delta.x)
	var hip := aim - acos(clampf((upper * upper + distance * distance - lower * lower) / (2.0 * upper * distance), -1.0, 1.0))
	var knee := PI - acos(clampf((upper * upper + lower * lower - distance * distance) / (2.0 * upper * lower), -1.0, 1.0))
	rig.set_bone_pose_rotation(thigh, Quaternion(Vector3.RIGHT, hip))
	rig.set_bone_pose_rotation(thigh + 1, Quaternion(Vector3.RIGHT, knee))
	# Cancel both leg rotations: the entire sole, not just the heel, contacts ground.
	rig.set_bone_pose_rotation(thigh + 2, Quaternion(Vector3.RIGHT, foot_pitch - hip - knee))
