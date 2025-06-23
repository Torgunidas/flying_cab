extends CanvasLayer
class_name QuestLog

@onready var list_box  : VBoxContainer = $Panel/List
@onready var close_btn : Button        = $Panel/Close

func _ready() -> void:
	visible = false
	set_process_unhandled_input(true)
	close_btn.pressed.connect(Callable(self, "_on_close_pressed"))
	QuestSys.quest_activated.connect(Callable(self, "_refresh"))
	QuestSys.quest_completed.connect(Callable(self, "_refresh"))
	_refresh()

func _on_close_pressed() -> void:
	visible = false

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("quest_log"):
		visible = not visible
		if visible:
			_refresh()

func _refresh(_id := "") -> void:
	# 1) czyścimy poprzednie przyciski
	for child in list_box.get_children():
		child.queue_free()

	# 2) iterujemy tylko po ACTIVE i DONE
	for qdata in QuestSys.get_all():
		match qdata.status:
			QuestData.Status.INACTIVE:
				continue   # pomijamy
			QuestData.Status.DONE:
				_add_done_button(qdata)
			QuestData.Status.ACTIVE:
				_add_active_button(qdata)

func _add_done_button(qdata: QuestData) -> void:
	var btn = Button.new()
	btn.text = "✔ %s" % qdata.title
	btn.disabled = true
	# zielony kolor tekstu
	btn.add_theme_color_override("font_color", Color(0, 1, 0))
	list_box.add_child(btn)

func _add_active_button(qdata: QuestData) -> void:
	var btn = Button.new()
	btn.text = qdata.title
	# podłączamy kliknięcie, żeby zmieniało objective
	var cb = Callable(self, "_on_list_item_pressed").bind(qdata.id)
	btn.connect("pressed", cb)
	list_box.add_child(btn)

func _on_list_item_pressed(selected_id: String) -> void:
	QuestObjectiveUi.set_quest_id(selected_id)
