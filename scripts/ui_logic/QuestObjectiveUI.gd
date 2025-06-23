extends CanvasLayer
class_name QuestObjectiveUI

@export var quest_id: String = ""    # aktualnie wyświetlany quest

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

# przy starcie – znajdź istniejący active quest (jeśli jest)
func _initialize_current_active() -> void:
	for q in QuestSys.get_all():
		if q.status == QuestData.Status.ACTIVE:
			set_quest_id(q.id)
			return

# główna logika aktualizacji UI
func _update_objective() -> void:
	# brak ID → chowaj
	if quest_id == "":
		visible = false
		return

	var q = QuestSys.get_quest(quest_id)
	# quest nie istnieje lub nie jest active → chowaj
	if q == null or q.status != QuestData.Status.ACTIVE:
		visible = false
		return

	# ok, mamy valid + active → pokaż i wypełnij
	visible = true
	title_label.text     = q.title
	objective_label.text = q.objective
