extends PassengerVisual
## Ari shares the human rig, with additional airborne and landing poses.
func pose_actor(dt: float, state: StringName, facing: float) -> void:
	pose(dt, state == &"walk", false, facing)
	if state in [&"jump", &"fall"]:
		var rising := state == &"jump"
		rig.set_bone_pose_rotation(1, Quaternion(Vector3.RIGHT, -0.65 if rising else -0.2))
		rig.set_bone_pose_rotation(2, Quaternion(Vector3.RIGHT, 0.35 if rising else 0.15))
		rig.set_bone_pose_rotation(3, Quaternion(Vector3.FORWARD, -0.35))
		rig.set_bone_pose_rotation(4, Quaternion(Vector3.FORWARD, 0.35))
	rig.position.y = lerpf(rig.position.y, -0.10 if state == &"land" else 0.0, minf(1, dt * 22))
