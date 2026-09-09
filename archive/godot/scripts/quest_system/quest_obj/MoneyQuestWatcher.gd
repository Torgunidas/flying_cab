extends Node
class_name MoneyQuestWatcher

# --- Parametry edytowalne ---
@export var quest_id: String
@export var threshold_amount: int = 0
@export_enum("<", ">", "=")
var relation: String = ">"

var _gs: Node = null

func _ready() -> void:
	_gs = get_node_or_null("/root/GameState")
	if _gs:
		_gs.money_changed.connect(_on_money_changed)
		_on_money_changed(_gs.get_money())
	else:
		push_warning("MoneyQuestWatcher: GameState not found")

func _on_money_changed(amount: int) -> void:
	if not _condition_met(amount):
		return
	var q := QuestSys.get_quest(quest_id)
	if q and q.status == QuestData.Status.ACTIVE:
		QuestSys.complete(quest_id)
		if _gs:
			_gs.money_changed.disconnect(_on_money_changed)

func _condition_met(amount: int) -> bool:
	match relation:
		"<":
			return amount < threshold_amount
		">":
			return amount > threshold_amount
		"=":
			return amount == threshold_amount
		_:
			push_warning("MoneyQuestWatcher: unknown operator '" + relation + "'")
			return false
