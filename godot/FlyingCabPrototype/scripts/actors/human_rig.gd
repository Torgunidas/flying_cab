class_name HumanRig
extends RefCounted
## Shared authored anatomy, in the same world units as VehicleDefinition.
## Slightly taller than a standard cab roof; models are baked at this scale.
const SCALE := 0.64
const HEIGHT := 1.16
const RADIUS := 0.17
const HIP_Y := 0.94 * SCALE
const ANKLE_Y := 0.10 * SCALE
const THIGH_LENGTH := 0.42 * SCALE
const SHIN_LENGTH := 0.42 * SCALE
const PELVIS := 0
const SPINE := 1
const HEAD := 2
const LEFT_THIGH := 3
const LEFT_SHIN := 4
const LEFT_FOOT := 5
const RIGHT_THIGH := 6
const RIGHT_SHIN := 7
const RIGHT_FOOT := 8
const LEFT_ARM := 9
const LEFT_FOREARM := 10
const RIGHT_ARM := 11
const RIGHT_FOREARM := 12
const NAMES := ["pelvis", "spine", "head", "thigh_l", "shin_l", "foot_l", "thigh_r", "shin_r", "foot_r", "upper_arm_l", "forearm_l", "upper_arm_r", "forearm_r"]
const PARENTS := [-1, 0, 1, 0, 3, 4, 0, 6, 7, 1, 9, 1, 11]

static func rest_positions() -> Array[Vector3]:
	var positions: Array[Vector3] = [
		Vector3(0, 0.94, 0), Vector3(0, 1.21, 0), Vector3(0, 1.50, 0),
		Vector3(-0.115, 0.94, 0), Vector3(-0.115, 0.52, 0), Vector3(-0.115, 0.10, 0),
		Vector3(0.115, 0.94, 0), Vector3(0.115, 0.52, 0), Vector3(0.115, 0.10, 0),
		Vector3(-0.28, 1.40, 0), Vector3(-0.28, 1.13, 0),
		Vector3(0.28, 1.40, 0), Vector3(0.28, 1.13, 0)]
	for i in positions.size():
		positions[i] *= SCALE
	return positions

static func clearance_shape() -> CapsuleShape3D:
	var shape := CapsuleShape3D.new()
	shape.height = HEIGHT
	shape.radius = RADIUS
	return shape
