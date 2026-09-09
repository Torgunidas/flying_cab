extends Area2D
class_name EndTrigger

@export var quest_id        : String
@export var reward_scene    : PackedScene
@export var reward_texture  : Texture
@export var reward_offset   : Vector2 = Vector2.ZERO

@export var required_car_id : String  = ""   # pusty → wystarczy sam gracz
@export_range(-1.0, 100.0, 1.0) var min_fuel_percent : float = -1.0 # <0 → brak wymogu
@export var complete_on_enter : bool = true

func _ready() -> void:
	body_entered.connect(_on_enter)

func _on_enter(body: Node) -> void:
	if not complete_on_enter:
		return

	if not _is_valid_body(body):
		return

	if not _fuel_requirement_met():
		return

	_complete_quest_and_spawn_reward()

# ───────────────────────────────────────────────────────────────
#  P R I V A T E
# ───────────────────────────────────────────────────────────────

func _is_valid_body(body: Node) -> bool:
	# ───── 1) brak wymogu konkretnego auta ─────
	if required_car_id == "":
		# a) gracz na piechotę
		if body.is_in_group("player"):
			return true
		# b) gracz w dowolnym samochodzie
		if body is Car:
			var pc := get_tree().get_first_node_in_group("player") as PlayerCharacter
			return pc and pc.in_vehicle and pc.vehicle == body
		return false

	# ───── 2) wymagamy konkretnego auta ─────
	if body is Car:
		var car := body as Car
		if car.car_id == required_car_id:
			var pc := get_tree().get_first_node_in_group("player") as PlayerCharacter
			return pc and pc.in_vehicle and pc.vehicle == car
	return false


func _fuel_requirement_met() -> bool:
	# brak limitu paliwa → zawsze OK
	if min_fuel_percent < 0.0:
		return true

	var pc := get_tree().get_first_node_in_group("player") as PlayerCharacter
	if not (pc and pc.in_vehicle and pc.vehicle):
		return false

	var car := pc.vehicle
	if car.max_fuel <= 0.0:
		return false             # unikamy dzielenia przez zero

	var percent := 100.0 * car.current_fuel / car.max_fuel
	return percent <= min_fuel_percent   # quest kończy się przy mniejszym LUB równym


func _complete_quest_and_spawn_reward() -> void:
	var q := QuestSys.get_quest(quest_id)
	if q and q.status == QuestData.Status.ACTIVE:
		QuestSys.complete(quest_id)

		if reward_scene:
			var box := reward_scene.instantiate()
			box.global_position = global_position + reward_offset
			get_tree().current_scene.add_child(box)

			var spr := box.get_node("Sprite2D") as Sprite2D
			if spr and reward_texture:
				spr.texture = reward_texture

	queue_free()
