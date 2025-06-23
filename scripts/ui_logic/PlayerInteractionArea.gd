extends Node2D
class_name ProximityToggle

# Pokazuje przycisk „toggle_action” gdy gracz
# • znajduje się w zasięgu enter_distance od dowolnego węzła w grupie "interactables"
# • LUB siedzi w pojeździe (PlayerCharacter.in_vehicle)

@export var enter_distance: float = 32.0

@onready var ui: MobileControls = get_tree().get_current_scene().get_node_or_null("MobileControls") as MobileControls
@onready var pc: PlayerCharacter = get_parent() as PlayerCharacter

var _prev_show: bool = false

func _ready() -> void:
	set_process_input(true)

func _input(event: InputEvent) -> void:
	if not event.is_action_pressed("toggle_action"):
		return

	var dm := get_tree().get_current_scene().get_node_or_null("DialogManager") as DialogManager
	if dm and dm.is_open():
		dm.close_dialog()
		return

	var best := INF
	var target: Node2D = null
	for n in get_tree().get_nodes_in_group("interactables"):
		var d = pc.global_position.distance_to(n.global_position)
		if d < enter_distance and d < best:
			best = d
			target = n
	if target and target.has_method("interact"):
		target.interact()

func _process(_delta: float) -> void:
	if ui == null:
		return

	var show := pc.in_vehicle
	if not show:
		for n in get_tree().get_nodes_in_group("interactables"):
			if pc.global_position.distance_to(n.global_position) < enter_distance:
				show = true
				break

	if show != _prev_show:
		ui.set_enter_visible(show)
		_prev_show = show

		if not show and not pc.in_vehicle:
			var dm := get_tree().get_current_scene().get_node_or_null("DialogManager") as DialogManager
			if dm and dm.is_open():
				dm.close_dialog()
