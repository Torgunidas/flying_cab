extends Node2D
class_name EnterVehicleTrigger

@export var quest_id           : String
@export var reward_scene       : PackedScene
@export var reward_texture     : Texture
@export var reward_offset      : Vector2 = Vector2.ZERO
@export var required_car_id    : String = ""
@export var complete_on_enter  : bool = true

func _ready() -> void:
		call_deferred("_connect_player")

func _connect_player() -> void:
		var player := get_tree().get_first_node_in_group("player") as PlayerCharacter
		if not player:
				push_warning("EnterVehicleTrigger: player not found")
				return
		if player.has_signal("entered_vehicle"):
				player.entered_vehicle.connect(_on_entered_vehicle)
		if player.in_vehicle and _car_matches(player.vehicle):
				_maybe_complete()

func _car_matches(car: Car) -> bool:
		if not car:
				return false
		return required_car_id == "" or car.car_id == required_car_id

func _on_entered_vehicle(vehicle: Car) -> void:
		if _car_matches(vehicle):
				_maybe_complete()

func _maybe_complete() -> void:
		if not complete_on_enter:
				return
		_complete()

func _complete() -> void:
		var q := QuestSys.get_quest(quest_id)
		if q and q.status == QuestData.Status.ACTIVE:
				QuestSys.complete(quest_id)

				if reward_scene:
						var box := reward_scene.instantiate()
						get_tree().current_scene.add_child(box)
						box.global_position = global_position + reward_offset

						var spr := box.get_node("Sprite2D") as Sprite2D
						if spr and reward_texture:
								spr.texture = reward_texture

				queue_free()
