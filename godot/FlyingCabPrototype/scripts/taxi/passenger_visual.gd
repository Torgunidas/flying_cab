class_name PassengerVisual
extends Node3D
## Presentation only; one skinned mesh, shared resources, no passenger AI tick.
@onready var rig: Skeleton3D = $Skeleton3D
var _time := 0.0

func pose(dt: float, walking: bool, waving: bool, direction: float) -> void:
	_time += dt
	rotation.y = lerp_angle(rotation.y, PI * 0.5 * signf(direction) if absf(direction) > 0.01 else 0.0, minf(1, dt * 8))
	var swing := sin(_time * 8.0) * 0.55 if walking else 0.0
	rig.set_bone_pose_rotation(1, Quaternion(Vector3.RIGHT, swing))
	rig.set_bone_pose_rotation(2, Quaternion(Vector3.RIGHT, -swing))
	rig.set_bone_pose_rotation(3, Quaternion(Vector3.RIGHT, -swing * 0.8))
	rig.set_bone_pose_rotation(4, Quaternion(Vector3.FORWARD, 2.7 + sin(_time * 5) * 0.18) if waving else Quaternion(Vector3.RIGHT, swing * 0.8))
	rig.set_bone_pose_rotation(5, Quaternion(Vector3.UP, sin(_time * 0.8) * 0.12))
