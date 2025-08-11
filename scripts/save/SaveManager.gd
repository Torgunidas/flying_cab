extends Node
class_name SaveManager

const SAVE_VERSION := 1
const SAVE_DIR := "user://saves"
const SAVE_PATH_FMT := SAVE_DIR + "/slot_%02d.fcsave"

# Ile trzymamy slotów (1..MAX_SLOTS)
const MAX_SLOTS := 5
const AUTO_SLOT := -1

func _ready() -> void:
	DirAccess.make_dir_recursive_absolute(SAVE_DIR)

# ─────────────────────────────────────────────────────────────
# PUBLIC API
# ─────────────────────────────────────────────────────────────

# Gdy slot = AUTO_SLOT (domyślnie), wybiera pierwszy wolny 1..MAX_SLOTS,
# a jeśli wszystkie istnieją – nadpisuje najstarszy (po mtime).
func save_min(slot: int = AUTO_SLOT) -> void:
	var player: Node = _get_player()

	var cur_scene: Node = get_tree().current_scene
	var cur_scene_path: String = cur_scene.scene_file_path if cur_scene != null else ""

	var level_ps: PackedScene = GameState.get_selected_level()
	var level_path: String = level_ps.resource_path if level_ps != null else ""

	# ── INVENTORY: eksport listy ItemData → [{id, res}] ──
	var inv_array: Array[Dictionary] = []
	if GameState.has_method("get_items"):
		var items: Array = GameState.get_items()
		for it in items:
			if it is ItemData:
				var id: String = (it as ItemData).id
				if id == "":
					continue
				var res_path: String = ""
				if it is Resource:
					res_path = (it as Resource).resource_path
				inv_array.append({
					"id": id,
					"res": res_path,  # może być pusty, jeśli item był runtime-only
				})

	var save_data: Dictionary = {
		"version": SAVE_VERSION,
		"timestamp": Time.get_datetime_string_from_system(),
		"current_scene": cur_scene_path,
		"current_level_scene": level_path,
		"maya_seconds_left": GameState.get_maya_seconds_left(),
		"car_access": {
			"models": GameState.allowed_models,
			"instances": GameState.allowed_instances
		},
		"player": {
			"hp": _safe_get(player, "current_hp", 0),
			"max_hp": _safe_get(player, "max_hp", 0),
			"money": GameState.get_money()
		},
		"quests": QuestSys.get_save_dict(),
		"last_car": VehPers.export_last_state(), # zawiera world_id
		"inventory": inv_array                   #  ← NEW
	}

	if slot == AUTO_SLOT:
		slot = _pick_slot_for_auto_save()

	var path: String = _slot_path(slot)
	if FileAccess.file_exists(path):
		DirAccess.copy_absolute(path, "%s.bak" % path)

	DirAccess.make_dir_recursive_absolute(SAVE_DIR)
	var f: FileAccess = FileAccess.open(path, FileAccess.WRITE)
	f.store_var(save_data, true)  # NATYWNY zapis (zachowuje Vector2 itd.)
	f.close()

	if MessageSystem and MessageSystem.has_method("show_message"):
		MessageSystem.show_message("Game saved (slot %d)" % slot)

func load_save(slot: int = 1) -> void:
	var path: String = _slot_path(slot)
	if not FileAccess.file_exists(path):
		push_error("No save file: %s" % path)
		return

	var f: FileAccess = FileAccess.open(path, FileAccess.READ)
	var data: Dictionary = f.get_var(true) as Dictionary  # NATYWNY odczyt
	f.close()

	# przywróć wybrany level w GameState (zanim przełączymy scenę)
	if data.has("current_level_scene"):
		var lvl_path: String = str(data.get("current_level_scene", ""))
		if lvl_path != "" and ResourceLoader.exists(lvl_path):
			var lvl_ps_any: Variant = load(lvl_path)
			if lvl_ps_any is PackedScene:
				GameState.set_selected_level(lvl_ps_any as PackedScene)
		else:
			GameState.set_selected_level(null)

	# przełącz scenę na zapisany root
	var scene_path: String = str(data.get("current_scene", ""))
	if scene_path == "" or not ResourceLoader.exists(scene_path):
		push_error("Saved root scene missing or empty: %s" % scene_path)
		return

	get_tree().change_scene_to_file(scene_path)
	await get_tree().process_frame

	# ── przywracanie stanu gry ────────────────────────────────
	GameState.set_maya_seconds_left(int(data.get("maya_seconds_left", 0)))

	var player_dict: Dictionary = data.get("player", {}) as Dictionary
	GameState.set_money(int(player_dict.get("money", 0)))

	var player: Node = _get_player()
	if player:
		var hp_val: int = int(player_dict.get("hp", 0))
		var max_hp_val: int = int(player_dict.get("max_hp", 0))
		if _has_property(player, "max_hp"):
			player.set("max_hp", max_hp_val)
		if _has_property(player, "current_hp"):
			player.set("current_hp", hp_val)

	# questy
	var quests_data: Dictionary = data.get("quests", {}) as Dictionary
	QuestSys.load_from_save_dict(quests_data)

	# dostęp do pojazdów
	var car_access: Dictionary = data.get("car_access", {}) as Dictionary
	GameState.allowed_models.clear()
	GameState.allowed_models.append_array(car_access.get("models", []))
	GameState.allowed_instances.clear()
	GameState.allowed_instances.append_array(car_access.get("instances", []))
	for id_model in GameState.allowed_models:
		GameState.emit_signal("vehicle_access_changed", true, id_model)
	for id_inst in GameState.allowed_instances:
		GameState.emit_signal("vehicle_access_changed", true, id_inst)

	# ── pojazd: rejestrujemy stan w VehPers; odtworzenie zrobi GameRoot po załadowaniu levelu ──
	var last_car: Dictionary = data.get("last_car", {}) as Dictionary
	if not last_car.is_empty():
		VehPers.import_last_state(last_car)

	# ── INVENTORY: czyścimy i odtwarzamy z listy [{id, res}] ──
	var inv_array: Array = data.get("inventory", []) as Array

	# 1) Wyczyść aktualne itemy (jeśli są)
	if GameState.has_method("get_items") and GameState.has_method("revoke_item"):
		var to_remove: Array = GameState.get_items()
		for it in to_remove:
			if it is ItemData:
				var item_id: String = (it as ItemData).id
				if item_id != "":
					GameState.revoke_item(item_id)

	# 2) Dodaj z save’a
	for entry in inv_array:
		if typeof(entry) != TYPE_DICTIONARY:
			continue
		var e: Dictionary = entry as Dictionary
		var id: String = str(e.get("id", ""))
		var res_path: String = str(e.get("res", ""))

		var item_res: ItemData = null
		if res_path != "" and ResourceLoader.exists(res_path):
			var any_res: Variant = load(res_path)
			if any_res is ItemData:
				item_res = any_res as ItemData

		if item_res != null and GameState.has_method("grant_item"):
			GameState.grant_item(item_res)
		else:
			# Nie znaleziono resource_path – pominiesz ten item (bezpiecznie)
			if id != "":
				push_warning("SaveManager: cannot restore item '%s' (res='%s')" % [id, res_path])

func has_save(slot: int = 1) -> bool:
	return FileAccess.file_exists(_slot_path(slot))

func delete_save(slot: int) -> void:
	var path: String = _slot_path(slot)
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(path)

# Zwraca listę metadanych slotów (tylko te istniejące), posortowaną rosnąco po numerze slotu.
# Każdy element: { "slot", "path", "timestamp", "maya", "money", "mtime" }
func list_saves() -> Array:
	var out: Array[Dictionary] = []
	var dir: DirAccess = DirAccess.open(SAVE_DIR)
	if dir == null:
		return out

	for s in range(1, MAX_SLOTS + 1):
		var path := _slot_path(s)
		if not FileAccess.file_exists(path):
			continue
		var meta: Dictionary = _peek_metadata(path)
		meta["slot"] = s
		meta["path"] = path
		meta["mtime"] = int(FileAccess.get_modified_time(path))
		out.append(meta)

	out.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		return int(a.get("slot", 0)) < int(b.get("slot", 0))
	)
	return out

# ─────────────────────────────────────────────────────────────
# INTERNAL
# ─────────────────────────────────────────────────────────────

func _slot_path(slot: int) -> String:
	return SAVE_PATH_FMT % slot

# Wybór slotu dla AUTO save:
# 1) pierwszy wolny w 1..MAX_SLOTS
# 2) jeśli wszystkie zajęte – najstarszy po mtime
func _pick_slot_for_auto_save() -> int:
	# pierwszy wolny
	for s in range(1, MAX_SLOTS + 1):
		if not FileAccess.file_exists(_slot_path(s)):
			return s
	# wszystkie zajęte – wybierz najstarszy
	var oldest_slot: int = 1
	var oldest_time: int = int(FileAccess.get_modified_time(_slot_path(1)))
	for s in range(2, MAX_SLOTS + 1):
		var t: int = int(FileAccess.get_modified_time(_slot_path(s)))
		if t < oldest_time:
			oldest_time = t
			oldest_slot = s
	return oldest_slot

func _peek_metadata(path: String) -> Dictionary:
	var f: FileAccess = FileAccess.open(path, FileAccess.READ)
	if f == null:
		return {}
	var data: Dictionary = f.get_var(true) as Dictionary
	f.close()

	var player_dict: Dictionary = data.get("player", {}) as Dictionary
	return {
		"timestamp": data.get("timestamp", ""),
		"maya": int(data.get("maya_seconds_left", 0)),
		"money": int(player_dict.get("money", 0)),
	}

func _get_player() -> Node:
	var players: Array = get_tree().get_nodes_in_group("player")
	if players.is_empty():
		return null
	return players[0]

func _has_property(obj: Object, prop_name: String) -> bool:
	for info in obj.get_property_list():
		if "name" in info and info.name == prop_name:
			return true
	return false

func _safe_get(obj: Object, prop: String, def):
	if obj == null:
		return def
	if _has_property(obj, prop):
		return obj.get(prop)
	return def

func _input(event: InputEvent) -> void:
	if event.is_action_pressed("save_debug"):
		save_min() # AUTO slot z rotacją
	elif event.is_action_pressed("load_debug"):
		load_save() # domyślnie slot 1
