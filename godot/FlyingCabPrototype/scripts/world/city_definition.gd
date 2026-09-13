class_name CityDefinition
extends MapDefinition
## World data shared by movement, services, the map and presentation.
@export var city_left := -74.5
@export var city_right := 75.5
@export var ground_height := -48.0
@export var max_altitude := 320.0
@export var soft_ceiling_range := 10.0
@export var ceiling_drag := 4.0
@export var return_margin := 6.0
@export var return_speed := 5.0
@export var return_min_height := 10.0
@export var upper_city_height := 160.0
@export var district_divider := 0.5
@export var low_city_top := 11.0
@export var depot_area := Rect2(-30, 48.4, 16, 12)

func _init() -> void:
	map_id = &"city_02"

func bounds() -> Rect2:
	return Rect2(city_left, ground_height, city_right - city_left, max_altitude - ground_height)

func district_id_at(position: Vector3) -> StringName:
	# Stable gameplay identity, independent of local labels such as Ari's depot
	# and of the visual transition into smog.
	if not bounds().has_point(Vector2(position.x, position.y)):
		return &""
	if position.y < low_city_top:
		return &"lowlife"
	if position.y >= upper_city_height:
		return &"eden" if position.x < district_divider else &"aurelia"
	return &"velvet" if position.x < district_divider else &"foundry"

func district_at(position: Vector3, smog_top: float) -> String:
	if position.y < low_city_top:
		return "LOWLIFE / STREFA SMOGU" if position.y < smog_top else "LOWLIFE / PODMIASTO"
	if depot_area.has_point(Vector2(position.x, position.y)):
		return "ARI / CAB DEPOT"
	if position.y >= upper_city_height:
		return "EDEN / KLINIKI I AUGMENTACJA" if position.x < district_divider else "AURELIA / FINANSE"
	return "VELVET / ROZRYWKA" if position.x < district_divider else "FOUNDRY / BIZNES I WARSZTATY"
