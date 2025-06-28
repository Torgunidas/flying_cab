extends CanvasLayer
class_name QuestLog

# Font size for description inside the log
const LOG_DESC_FONT_SIZE := 25
# Font size for quest buttons
const LOG_BTN_FONT_SIZE := 28
# Minimal height for quest buttons (for easier tapping)
const LOG_BTN_MIN_HEIGHT := 60

@onready var list_box  : VBoxContainer = $Panel/List

func _ready() -> void:
	visible = false
	set_process_unhandled_input(true)
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
	btn.add_theme_font_size_override("font_size", LOG_BTN_FONT_SIZE)
	btn.custom_minimum_size.y = LOG_BTN_MIN_HEIGHT
	btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list_box.add_child(btn)

func _add_active_button(qdata: QuestData) -> void:
	var container = VBoxContainer.new()
	container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list_box.add_child(container)

	var btn = Button.new()
	btn.text = qdata.title
	btn.add_theme_font_size_override("font_size", LOG_BTN_FONT_SIZE)
	btn.custom_minimum_size.y = LOG_BTN_MIN_HEIGHT
	btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	container.add_child(btn)

	var desc = Label.new()
	desc.text = qdata.description
	desc.visible = false
	desc.add_theme_font_size_override("font_size", LOG_DESC_FONT_SIZE)
	desc.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(desc)

	# podłączamy kliknięcie, żeby zmieniało objective i rozwijało opis
	var cb = Callable(self, "_on_list_item_pressed").bind(qdata.id, desc)
	btn.connect("pressed", cb)

func _on_list_item_pressed(selected_id: String, desc_label: Label) -> void:
		QuestObjectiveUi.set_quest_id(selected_id)
		desc_label.visible = not desc_label.visible
