@tool
extends EditorPlugin
var picker: EditorFileDialog
var result_dialog: AcceptDialog
var preview_only := false

func _enter_tree() -> void:
	picker = EditorFileDialog.new()
	picker.file_mode = EditorFileDialog.FILE_MODE_OPEN_FILE
	picker.access = EditorFileDialog.ACCESS_FILESYSTEM
	picker.add_filter("*.json", "Eksport Flying Cab Narrative")
	picker.title = "Importuj fabułę Flying Cab"
	picker.file_selected.connect(_import)
	get_editor_interface().get_base_control().add_child(picker)
	result_dialog = AcceptDialog.new()
	get_editor_interface().get_base_control().add_child(result_dialog)
	add_tool_menu_item("Flying Cab — otwórz edytor fabuły", _open_editor)
	add_tool_menu_item("Flying Cab — importuj fabułę", _pick)
	add_tool_menu_item("Flying Cab — podgląd fabuły", _pick_preview)

func _exit_tree() -> void:
	remove_tool_menu_item("Flying Cab — otwórz edytor fabuły")
	remove_tool_menu_item("Flying Cab — importuj fabułę")
	remove_tool_menu_item("Flying Cab — podgląd fabuły")
	picker.queue_free()
	result_dialog.queue_free()

func _open_editor() -> void:
	var path := ProjectSettings.globalize_path("res://../../tools/quest_editor/index.html").simplify_path()
	path = path.replace("\\", "/")
	var uri := "file://" + ("" if path.begins_with("/") else "/") + path.uri_encode().replace("%2F", "/").replace("%3A", ":")
	OS.shell_open(uri)

func _pick() -> void:
	preview_only = false
	picker.popup_centered_ratio(0.7)

func _pick_preview() -> void:
	preview_only = true
	picker.popup_centered_ratio(0.7)

func _import(path: String) -> void:
	var importer := FlyingCabStoryImporter.new()
	var result := importer.import_file(path, "res://resources/narrative/city_catalog.tres", preview_only)
	result_dialog.title = "Flying Cab — import fabuły"
	if result.ok:
		if preview_only:
			DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://build"))
			var error := DirAccess.copy_absolute(path, ProjectSettings.globalize_path("res://build/story-preview.json"))
			if error == OK:
				get_editor_interface().play_custom_scene("res://addons/flying_cab_story/preview.tscn")
				return
			result_dialog.dialog_text = "Nie udało się przygotować podglądu."
			result_dialog.popup_centered()
			return
		get_editor_interface().get_resource_filesystem().scan()
		result_dialog.dialog_text = "Fabuła zapisana (%d plików).\nUruchom grę ponownie, aby wczytać zmiany.\nNowe profile NPC są w resources/narrative/authored/.\nKopia poprzednich plików: %s" % [result.files, result.backup]
	else:
		result_dialog.dialog_text = "Import przerwany:\n" + "\n".join(result.errors)
	result_dialog.popup_centered(Vector2i(760, 440))
