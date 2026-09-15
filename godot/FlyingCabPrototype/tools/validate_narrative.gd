extends SceneTree
## Local content check, independent of gameplay, renderer and export.
func _initialize() -> void:
	var path := "res://resources/narrative/city_catalog.tres"
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--catalog="):
			path = arg.trim_prefix("--catalog=")
	var catalog := load(path) as NarrativeCatalog
	if catalog == null:
		push_error("Cannot load NarrativeCatalog: " + path)
		quit(1)
		return
	var errors := catalog.validation_errors()
	for error in errors:
		push_error(error)
	print("NARRATIVE VALIDATION: ", errors.size(), " errors; ", catalog.quests.size(), " quests / ", catalog.dialogues.size(), " dialogues / ", catalog.npcs.size(), " NPCs")
	quit(0 if errors.is_empty() else 1)
