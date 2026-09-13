extends Resource
## Shared world heights for the smog, lamps and HUD. Units are world metres.
@export var top_height: float = -6.0
@export var feather: float = 12.0
@export var lights_on_height: float = -6.0
@export var lights_off_height: float = 0.0
@export var light_fade_seconds: float = 0.6
@export var fog_color := Color("23454b")

func density_at(height: float) -> float:
	return 1.0 - smoothstep(top_height - feather, top_height + 6.0, height)
