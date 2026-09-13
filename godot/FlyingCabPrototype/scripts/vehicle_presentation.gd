class_name VehiclePresentation
extends Resource
## Body animation is a read-only projection of the physical result.
@export var max_pitch_degrees := 12.0
@export var pitch_full_acceleration := 14.0
@export var pitch_response := 7.0
@export var pitch_return := 2.5
var _pitch := 0.0

func reset(visual: Node3D) -> void:
	_pitch = 0.0
	visual.rotation = Vector3.ZERO

func update(visual: Node3D, acceleration: float, velocity: Vector3, applied: Vector2, dt: float) -> void:
	var target := -clampf(acceleration / pitch_full_acceleration, -1.0, 1.0) * max_pitch_degrees
	var returning := absf(target) < absf(_pitch) or is_zero_approx(target)
	_pitch = lerpf(_pitch, target, clampf(dt * (pitch_return if returning else pitch_response), 0.0, 1.0))
	visual.rotation.z = deg_to_rad(_pitch)
	var heading := velocity.x if absf(velocity.x) > 0.05 else applied.x
	if absf(heading) > 0.01:
		var facing := 1.0 if heading > 0.0 else -1.0
		if visual.scale.x != facing:
			visual.scale.x = facing
			visual.reset_physics_interpolation()
