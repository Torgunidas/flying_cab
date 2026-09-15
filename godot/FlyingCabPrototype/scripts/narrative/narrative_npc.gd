@tool
class_name NarrativeNpc
extends Node3D
## Instance scenes/narrative/npc.tscn, assign a profile, place its feet on a terrace.
@export var profile: NpcDefinition:
	set(value):
		profile = value
		if is_inside_tree():
			_refresh()
@export_range(0.5, 5, 0.1) var interaction_distance := 1.8
var _session: DialogueSession

func _ready() -> void:
	_refresh()
	if not Engine.is_editor_hint():
		add_to_group("narrative_npc")
		if profile:
			set_meta("entity_id", "npc/" + String(profile.id))

func _refresh() -> void:
	var label := get_node_or_null("Name") as Label3D
	if label:
		label.text = profile.display_name if profile else "ASSIGN NPC PROFILE"
	update_configuration_warnings()

func _get_configuration_warnings() -> PackedStringArray:
	return PackedStringArray(["Assign an NpcDefinition from the session's narrative catalog."]) if profile == null else PackedStringArray()

func available(context: RuntimeContext, actor: WalkingActor) -> bool:
	if profile == null or context.narrative.catalog == null or context.narrative.catalog.npc(profile.id) != profile or context.player.is_suspended() or context.player.focus != actor or not actor.is_on_floor():
		return false
	var head := global_position + Vector3.UP * HumanRig.HEIGHT * 0.5
	if head.distance_to(actor.global_position) > interaction_distance:
		return false
	var query := PhysicsRayQueryParameters3D.create(actor.global_position, head, WorldLayers.GEOMETRY)
	return get_world_3d().direct_space_state.intersect_ray(query).is_empty()

func interact(context: RuntimeContext, actor: WalkingActor) -> bool:
	if not available(context, actor):
		return false
	if not context.dialogue.start(profile, context.narrative, context.player):
		return false
	# Removal / map transition cannot leave a conversation holding an obsolete speaker.
	_session = context.dialogue
	return true

func _exit_tree() -> void:
	if _session and _session.profile == profile:
		_session.end()
