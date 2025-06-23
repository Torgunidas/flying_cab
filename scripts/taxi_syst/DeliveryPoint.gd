extends Area2D
class_name DeliveryPoint
"""
DeliveryPoint:
• przechowuje punkt docelowy dostawy oraz (opcjonalnie) ikonkę,
  która staje się widoczna tylko dla aktywnego celu;
• udostępnia waypoint (Marker2D), do którego idzie NPC;
• eksportuje flagę `unspawn` – NPC znika po dojściu, jeśli true.
"""

# ───────── EXPORT ────────────────────────────────────────────────────────
@export var unspawn  : bool     = true                # znika po dojściu?
@export var marker   : NodePath = "Sprite2D"         # ikonka celu (może być pusty)
@export var waypoint : NodePath = "PassengerWaypoint"  # Marker2D dla NPC

# ───────── RUNTIME REF ───────────────────────────────────────────────────
@onready var _marker_ref : Node2D = null              # uzupełnimy w _ready()

# ───────── LIFECYCLE ─────────────────────────────────────────────────────
func _ready() -> void:
	# łatwe wyszukiwanie w kodzie (np. do losowania celu)
	add_to_group("taxi_dest")

	# pobierz referencję ikonki (jeśli istnieje) i ukryj ją
	if not marker.is_empty() and has_node(marker):
		_marker_ref = get_node(marker) as Node2D
		_marker_ref.visible = false

# ───────── PUBLICZNE API ────────────────────────────────────────────────
func activate_marker() -> void:
	if _marker_ref:
		_marker_ref.visible = true

func deactivate_marker() -> void:
	if _marker_ref:
		_marker_ref.visible = false

func get_waypoint() -> Node2D:
	# zwróć waypoint, a jeśli nie ustawiono / nie znaleziono — sam DeliveryPoint
	if not waypoint.is_empty() and has_node(waypoint):
		return get_node(waypoint) as Node2D
	return self
