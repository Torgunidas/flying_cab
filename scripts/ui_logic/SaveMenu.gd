extends Control

@onready var list := %List
@onready var empty_label := %EmptyLabel
@onready var back_btn := %BackBtn

func _ready() -> void:
	_refresh()
	back_btn.pressed.connect(_on_back_pressed)

func _refresh() -> void:
	for c in list.get_children():
		c.queue_free()
	var saves := SaveManager.list_saves()
	empty_label.visible = saves.is_empty()
	for s in saves:
		list.add_child(_make_entry(s))

func _make_entry(data:Dictionary) -> Control:
	var hb := HBoxContainer.new()
	hb.custom_minimum_size.y = 48

	var lbl := Label.new()
	lbl.text = "Slot %02d | %s | $%d | Maya:%ds" % [data.slot, data.timestamp, data.money, data.maya]
	lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	var load_btn := Button.new()
	load_btn.text = "Wczytaj"
	load_btn.pressed.connect(func(): SaveManager.load(data.slot))

	var del_btn := Button.new()
	del_btn.text = "Usuń"
	del_btn.pressed.connect(func():
		SaveManager.delete_save(data.slot)
		_refresh()
	)

	hb.add_child(lbl)
	hb.add_child(load_btn)
	hb.add_child(del_btn)
	return hb

func _on_back_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/ui_scenes/MainMenu.tscn")
