extends Area2D
@export var reward: int = 100   # edytowalne: ile kasy daje skrzynka

func _ready() -> void:
	body_entered.connect(_on_body_entered)

func _on_body_entered(body):
	if body.is_in_group("player"):
		GameState.add_money(reward)
		print("DEBUG>>", GameState, GameState.get_path(), GameState.get_money())
		print("[PICKUP] saldo-HUD =", GameState.get_money())
		queue_free()
