extends SceneTree

func _initialize() -> void:
	var input := ""
	var catalog := "res://resources/narrative/city_catalog.tres"
	var dry_run := false
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--input="): input = arg.trim_prefix("--input=")
		elif arg.begins_with("--catalog="): catalog = arg.trim_prefix("--catalog=")
		elif arg == "--dry-run": dry_run = true
	var result := FlyingCabStoryImporter.new().import_file(input, catalog, dry_run)
	print("STORY_IMPORT: ", JSON.stringify(result))
	quit(0 if result.ok else 1)
