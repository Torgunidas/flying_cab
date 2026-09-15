class_name FlightCameraTuning
extends Resource
@export_group("Flight")
@export var camera_distance := 19.0
@export var camera_fov := 55.0
@export var camera_angle_degrees := 5.0
@export var camera_follow_speed := 5.0
@export var horizontal_look_ahead := 3.8
@export var vertical_look_ahead := 1.6
@export var look_ahead_speed := 3.5
@export_group("Platform approach")
## Distance from the edge/top of the deck, not its centre (also covers long decks).
@export_range(1.0, 20.0) var approach_start_distance := 8.0
@export_range(0.0, 3.0) var approach_close_distance := 0.8
@export_range(3.0, 19.0) var landing_distance := 6.0
## Keep visibility at cruise speed; the close frame opens as the pilot slows down.
@export_range(0.0, 12.0) var approach_slow_speed := 3.0
@export_range(1.0, 20.0) var approach_fast_speed := 9.0
@export_group("On foot")
@export_range(2.0, 12.0) var on_foot_distance := 4.2
@export var on_foot_horizontal_look_ahead := 0.75
@export var on_foot_target_height := 0.65
@export_range(0.0, 2.0) var on_foot_jump_dead_zone := 0.65
@export var on_foot_follow_speed := 8.0
@export var on_foot_angle_degrees := 0.0
@export_group("Transitions")
@export var framing_speed := 4.0
