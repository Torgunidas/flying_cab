extends SceneTree
const Check = preload("res://tools/city_compilation_check.gd")
func _initialize() -> void:
	call_deferred("run")
func run() -> void:
	var package := ""
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--pack="):
			package = arg.trim_prefix("--pack=")
	if not package.is_empty() and not ProjectSettings.load_resource_pack(package):
		push_error("Could not load exported package: " + package)
		quit(1)
		return
	var errors := Check.validate(root, "res://scenes/city.tscn", "res://scenes/city_runtime.tscn")
	for problem in errors.slice(0, 8):
		push_error(problem)
	print("CITY_COMPILATION_TESTS: ", "PASS" if errors.is_empty() else "FAIL", " errors=", errors.size(), " package=", package)
	quit(0 if errors.is_empty() else 1)
