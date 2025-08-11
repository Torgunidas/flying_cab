extends Area2D

@export var use_rotation: bool = true       # domyślnie: rotacja slotów 1..MAX_SLOTS
@export var manual_slot: int = 1            # używane tylko gdy use_rotation = false

func _on_body_entered(body: Node) -> void:
	if body.is_in_group("player"):
		var do_save := func():
			if use_rotation:
				# AUTO: SaveManager sam wybierze slot (pierwszy wolny, a jak brak – najstarszy)
				SaveMgr.save_min()
			else:
				# Ręczny zapis do wskazanego slota
				SaveMgr.save_min(manual_slot)

		if UISys and UISys.has_method("show_confirm"):
			UISys.show_confirm("ZAPISAĆ GRĘ?", do_save)
		else:
			do_save.call()

func _ready() -> void:
	body_entered.connect(_on_body_entered)
