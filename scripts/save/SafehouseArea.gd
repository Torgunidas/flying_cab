extends Area2D
@export var slot:int = 1

func _on_body_entered(body:Node) -> void:
	if body.is_in_group("player"):
		if UISys and UISys.has_method("show_confirm"):
			UISys.show_confirm("ZAPISAĆ GRĘ?", func(): SaveManager.save_min(slot))
		else:
			SaveManager.save_min(slot)

func _ready() -> void:
	body_entered.connect(_on_body_entered)
