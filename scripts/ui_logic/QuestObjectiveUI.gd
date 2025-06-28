extends CanvasLayer
class_name QuestObjectiveUI

@export var quest_id: String = ""    # aktualnie wyświetlany quest
@export var pointer_scene: PackedScene

var _pointer: QuestPointer = null

# referencje do Label
@onready var title_label     : Label = $Panel/VBox/TitleLabel
@onready var objective_label : Label = $Panel/VBox/ObjectiveLabel

func _ready() -> void:
	# domyślnie schowaj
	visible = false
	set_process_unhandled_input(true)

	# podłączamy się na activation i completion
	QuestSys.quest_activated.connect(_on_quest_activated)
	QuestSys.quest_completed.connect(_on_quest_completed)

	# jeśli przy starcie jest już jakiś active quest, od razu go pokaż
	_initialize_current_active()

# wywołujemy ręcznie z QuestLog lub wewnętrznie
func set_quest_id(id: String) -> void:
	quest_id = id
	_update_objective()

# gdy QuestSys wyemituje signal quest_activated
func _on_quest_activated(id: String) -> void:
	# ustawiamy i pokazujemy od razu
	set_quest_id(id)

# gdy QuestSys wyemituje signal quest_completed
func _on_quest_completed(id: String) -> void:
	# jeśli ukończono tego, którego właśnie wyświetlamy…
		if id == quest_id:
				# szukamy kolejnego active
				for q in QuestSys.get_all():
						if q.status == QuestData.Status.ACTIVE:
								set_quest_id(q.id)
								return
				# jeśli nie ma więcej active → chowamy
				visible = false
				_remove_pointer()

# przy starcie – znajdź istniejący active quest (jeśli jest)
func _initialize_current_active() -> void:
	for q in QuestSys.get_all():
				if q.status == QuestData.Status.ACTIVE:
						set_quest_id(q.id)
						return

func _remove_pointer() -> void:
	if is_instance_valid(_pointer):
			_pointer.queue_free()
	_pointer = null

func _spawn_pointer(goal_node: Node2D) -> void:
	if pointer_scene == null or goal_node == null:
			return
	_remove_pointer()
	_pointer = pointer_scene.instantiate() as QuestPointer
	_pointer.target = goal_node
	get_tree().current_scene.add_child(_pointer)

func _find_goal_node(id: String) -> Node2D:
	for goal in get_tree().get_nodes_in_group("quest_goal"):
			var qid := ""
			if goal.has_meta("quest_id"):
					qid = str(goal.get_meta("quest_id"))
			else:
					for prop in goal.get_property_list():
							if prop.name == "quest_id":
									qid = str(goal.get("quest_id"))
									break
			if qid == id:
					return goal
	return null

# główna logika aktualizacji UI
func _update_objective() -> void:
	# brak ID → chowaj
	if quest_id == "":
			visible = false
			_remove_pointer()
			return

	var q = QuestSys.get_quest(quest_id)
	# quest nie istnieje lub nie jest active → chowaj
	if q == null or q.status != QuestData.Status.ACTIVE:
				visible = false
				_remove_pointer()
				return

	# ok, mamy valid + active → pokaż i wypełnij
	visible = true
	title_label.text     = q.title
	objective_label.text = q.objective
	_spawn_pointer(_find_goal_node(quest_id))
