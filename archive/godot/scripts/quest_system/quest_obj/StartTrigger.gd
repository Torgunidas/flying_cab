extends Area2D
@export var quest_id: String         # ← ustaw w Inspectorze

func _ready() -> void:
	body_entered.connect(_on_enter)

func _on_enter(body: Node) -> void:
	if body.is_in_group("player"):
		QuestSys.activate(quest_id)  # ► quest ACTIVE
		queue_free()                 # jednorazowy trigger
