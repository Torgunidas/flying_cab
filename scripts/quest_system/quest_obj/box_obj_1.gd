extends Area2D

func _ready() -> void:
	body_entered.connect(_on_body_entered)

func _on_body_entered(body: Node) -> void:
	if not body.is_in_group("player"):
		return
	print("[DEBUG] PickupArea:_on_body_entered by", body.name)
	QuestSys.trigger_quests_from_pickup("Pickup_Crate")
	queue_free()
