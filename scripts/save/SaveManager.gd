extends Node
class_name SaveManager

const SAVE_VERSION := 1
const SAVE_DIR := "user://saves"
const SAVE_PATH_FMT := SAVE_DIR + "/slot_%02d.fcsave"

var _last_car_data: Dictionary = {}

func _ready() -> void:
	DirAccess.make_dir_recursive_absolute(SAVE_DIR)

# ---------- PUBLIC API ----------

func set_last_driven_car(car: Node) -> void:
	if car == null:
		return
	_last_car_data = {
		car_id   = _safe_get(car, "car_id", ""),
		hp       = _safe_get(car, "current_hp", 0),
		max_hp   = _safe_get(car, "max_hp", 0),
		fuel     = _safe_get(car, "current_fuel", 0),
		max_fuel = _safe_get(car, "max_fuel", 0),
		upgrades = _safe_get(car, "upgrades", [])
	}

func save_min(slot: int = 1) -> void:
	var player := _get_player()
	var data := {
		version           = SAVE_VERSION,
		timestamp         = Time.get_datetime_string_from_system(),
		current_scene     = get_tree().current_scene.scene_file_path,
		maya_seconds_left = GameState.get_maya_seconds_left(),
		player = {
			hp     = _safe_get(player, "current_hp", 0),
			max_hp = _safe_get(player, "max_hp", 0),
			money  = GameState.get_money()
		},
		quests   = QuestSys.get_save_dict(),
		last_car = _last_car_data
	}

	var path: String = SAVE_PATH_FMT % slot
	if FileAccess.file_exists(path):
		DirAccess.copy_absolute(path, "%s.bak" % path)

	DirAccess.make_dir_recursive_absolute(SAVE_DIR)
	var f := FileAccess.open(path, FileAccess.WRITE)
	f.store_string(JSON.stringify(data))
	f.close()
	if MessageSystem and MessageSystem.has_method("show_message"):
				MessageSystem.show_message("Game saved")

func load_save(slot: int = 1) -> void:          # ⬅ zmiana nazwy (unikamy kolizji z ResourceLoader.load)
	var path := SAVE_PATH_FMT % slot
	if !FileAccess.file_exists(path):
				push_error("No save file: %s" % path)
				return
	var f := FileAccess.open(path, FileAccess.READ)
	var data: Variant = JSON.parse_string(f.get_as_text())
	f.close()
	if typeof(data) != TYPE_DICTIONARY:
			push_error("Corrupted save file")
			return

	get_tree().change_scene_to_file(data.current_scene)
	await get_tree().process_frame

	# przywróć stan gry
	GameState.set_maya_seconds_left(data.maya_seconds_left)
	GameState.set_money(data.player.money)

	var player := _get_player()
	if player:
		player.current_hp = data.player.hp
		player.max_hp     = data.player.max_hp

	QuestSys.load_from_save_dict(data.quests)

	if data.last_car and data.last_car.get("car_id", "") != "":
		var car := VehPers.spawn_car_in_garage(data.last_car.car_id)
		if car:
			car.current_hp   = data.last_car.hp
			car.max_hp       = data.last_car.max_hp
			car.current_fuel = data.last_car.fuel
			car.max_fuel     = data.last_car.max_fuel
			if data.last_car.upgrades and car.has_method("apply_upgrades"):
				car.apply_upgrades(data.last_car.upgrades)

	_last_car_data = data.last_car


func has_save(slot: int = 1) -> bool:
	return FileAccess.file_exists(SAVE_PATH_FMT % slot)

func delete_save(slot: int) -> void:
	var path: String = SAVE_PATH_FMT % slot
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(path)

func list_saves() -> Array:                      # dokładny typ niepotrzebny
	var out: Array[Dictionary] = []
	var dir := DirAccess.open(SAVE_DIR)
	if dir == null:
		return out
	for file in dir.get_files():
		if file.ends_with(".fcsave"):
			var slot: int = int(file.get_slice("_", 1).get_slice(".", 0))
			var path: String = "%s/%s" % [SAVE_DIR, file]
			var meta: Dictionary = _peek_metadata(path)
			meta.slot = slot
			meta.path = path
			out.append(meta)
	out.sort_custom(func(a, b): return a.slot < b.slot)
	return out

# ---------- INTERNAL ----------

func _peek_metadata(path: String) -> Dictionary:
	var f := FileAccess.open(path, FileAccess.READ)
	if f == null:
			return {}
	var txt := f.get_as_text()
	f.close()
	var data: Variant = JSON.parse_string(txt)
	if typeof(data) != TYPE_DICTIONARY:
				return {}
	return {
				timestamp = data.get("timestamp", ""),
				maya      = data.get("maya_seconds_left", 0),
	}

func _get_player() -> Node:
	var players := get_tree().get_nodes_in_group("player")
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
		save_min()
	elif event.is_action_pressed("load_debug"):
		self.load_save()                               # ⬅ nowa nazwa
