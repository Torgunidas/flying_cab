extends Control

@onready var list        : VBoxContainer = $ScrollContainer/SaveList
@onready var empty_label : Label         = $EmptyLabel
@onready var back_btn    : Button        = $BackBtn

func _ready() -> void:
	back_btn.pressed.connect(_on_back_pressed)
	# upewnij się, że ScrollContainer i List w edytorze mają Layout -> Full Rect
	_refresh()

func _refresh() -> void:
	if list == null:
		push_error("SaveMenu: nie znaleziono VBoxContainer 'List'.")
		return

	# wyczyść stare entry
	for child in list.get_children():
		child.queue_free()

	var saves : Array[Dictionary] = SaveMgr.list_saves()
	empty_label.visible = saves.is_empty()

	for save in saves:
		list.add_child(_make_entry(save))

func _make_entry(data : Dictionary) -> Control:
	# — główny kontener wpisu —
	var hb := HBoxContainer.new()
	hb.custom_minimum_size.y = 48
	hb.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	# — etykieta informacji —
	var slot_num   : int    = int(data["slot"])
	var timestamp  : String = str(data["timestamp"])
	var maya_left  : int    = int(data.get("maya", 0))
	var money      : int    = int(data.get("money", 0))

	var lbl := Label.new()
	lbl.text = "Slot %02d | %s | $%d | Maya:%ds" % [slot_num, timestamp, money, maya_left]
	lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	# — przycisk Wczytaj —
	var load_btn := Button.new()
	load_btn.text = "Wczytaj"
	load_btn.pressed.connect(func():
		SaveMgr.load_save(slot_num)
	)

	# — przycisk Usuń —
	var del_btn := Button.new()
	del_btn.text = "Usuń"
	del_btn.pressed.connect(func():
		SaveMgr.delete_save(slot_num)
		_refresh()
	)

	hb.add_child(lbl)
	hb.add_child(load_btn)
	hb.add_child(del_btn)
	return hb

func _on_back_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/ui_scenes/MainMenu.tscn")
