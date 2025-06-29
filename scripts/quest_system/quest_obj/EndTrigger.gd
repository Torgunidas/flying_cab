# res://scripts/quest_system/EndTrigger.gd
extends Area2D
class_name EndTrigger

# 1) ID questa
@export var quest_id: String

# 2) Ścieżka do sceny nagrody (jedna uniwersalna RewardBox.tscn)
@export var reward_scene: PackedScene

# 3) Tekstura, którą chcesz pokazać w tym triggerze
@export var reward_texture: Texture

# 4) Możesz przesunąć pozycję nagrody względem triggera
@export var reward_offset: Vector2 = Vector2.ZERO

# Jeśli niepusty, gracz lub pojazd o podanym ID musi wejść w trigger,
# aby zaliczyć questa
@export var required_car_id: String = ""

# Decides if quest should be completed upon entering this trigger.
# Useful when the trigger is only a map hint.
@export var complete_on_enter: bool = true



func _ready() -> void:
	body_entered.connect(_on_enter)

func _on_enter(body: Node) -> void:
	var valid := false
	if required_car_id == "":
				valid = body.is_in_group("player")
	else:
			if body.has_variable("car_id") and body.car_id == required_car_id:
						valid = true
			elif body.is_in_group("player"):
					var pc := body as PlayerCharacter
					if pc.in_vehicle and pc.vehicle.has_variable("car_id") and pc.vehicle.car_id == required_car_id:
							valid = true
	if not valid:
				return
	if not complete_on_enter:
				return

	var q = QuestSys.get_quest(quest_id)
	if q and q.status == QuestData.Status.ACTIVE:
				# ► ukończ questa
		QuestSys.complete(quest_id)

		# ► utwórz instancję RewardBox
		if reward_scene:
			var box = reward_scene.instantiate()

			# 1) dodajemy do głównej sceny, by nie zniknęło po queue_free()
			get_tree().current_scene.add_child(box)

			# 2) ustawiamy pozycję
			box.global_position = global_position + reward_offset

			# 3) podmieniamy teksturę Sprite2D
			var spr = box.get_node("Sprite2D") as Sprite2D
			if spr and reward_texture:
				spr.texture = reward_texture

		# ► usuń trigger (box już w scenie)
		queue_free()
