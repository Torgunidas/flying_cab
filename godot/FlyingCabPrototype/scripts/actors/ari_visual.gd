extends PassengerVisual
## Ari shares the human anatomy; airborne and landing poses use the same knees.
var _landing_offset := 0.0

func reset_gait() -> void:
	super.reset_gait()
	_landing_offset = 0.0

func pose_actor(dt: float, state: StringName, facing: float, travel: Vector3) -> void:
	pose(dt, travel if state in [&"walk", &"land"] else Vector3.ZERO, false, facing)
	_landing_offset = lerpf(_landing_offset, -0.05 if state == &"land" else 0.0, minf(1, dt * 22))
	if state in [&"jump", &"fall"]:
		rig.set_bone_pose_position(HumanRig.PELVIS, Vector3(0, HumanRig.HIP_Y, 0))
		_solve_leg(HumanRig.LEFT_THIGH, HumanRig.HIP_Y, Vector2(0.24 if state == &"jump" else 0.11, 0.16), 0.18)
		_solve_leg(HumanRig.RIGHT_THIGH, HumanRig.HIP_Y, Vector2(0.12, -0.10), 0.12)
		rig.set_bone_pose_rotation(HumanRig.LEFT_ARM, Quaternion(Vector3.RIGHT, -0.65))
		rig.set_bone_pose_rotation(HumanRig.RIGHT_ARM, Quaternion(Vector3.RIGHT, 0.35))
	elif absf(_landing_offset) > 0.0001:
		# Absorb landing in the knees without pushing shoes below the terrace.
		var hip_y := rig.get_bone_pose_position(HumanRig.PELVIS).y + _landing_offset
		rig.set_bone_pose_position(HumanRig.PELVIS, Vector3(0, hip_y, 0))
		_pose_leg(HumanRig.LEFT_THIGH, _gait_phase, hip_y)
		_pose_leg(HumanRig.RIGHT_THIGH, fposmod(_gait_phase + 0.5, 1.0), hip_y)
