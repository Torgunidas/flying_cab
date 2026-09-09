extends Area2D
@export var event_name := ""

func _ready():
	body_entered.connect(_on_enter)

func _on_enter(body):
	if body.is_in_group("player"):
		QuestSys._on_area_entered(event_name)
