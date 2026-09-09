@tool
extends EditorPlugin
class_name DialogEditorPlugin

var dock : VBoxContainer
var file_dialog : FileDialog

var current_book : DialogBook
var current_page : DialogPage
var current_answer : DialogAnswer

func _enter_tree() -> void:
	dock = VBoxContainer.new()
	dock.name = "Dialog Editor"
	add_control_to_dock(DOCK_SLOT_RIGHT_UL, dock)

	var new_book_btn := Button.new()
	new_book_btn.text = "New DialogBook"
	new_book_btn.pressed.connect(_on_new_book)
	dock.add_child(new_book_btn)

	var add_page_btn := Button.new()
	add_page_btn.text = "Add Page"
	add_page_btn.pressed.connect(_on_add_page)
	dock.add_child(add_page_btn)

	var add_answer_btn := Button.new()
	add_answer_btn.text = "Add Answer"
	add_answer_btn.pressed.connect(_on_add_answer)
	dock.add_child(add_answer_btn)

	var add_action_btn := Button.new()
	add_action_btn.text = "Add Action"
	add_action_btn.pressed.connect(_on_add_action)
	dock.add_child(add_action_btn)

	var add_req_btn := Button.new()
	add_req_btn.text = "Add Requirement"
	add_req_btn.pressed.connect(_on_add_requirement)
	dock.add_child(add_req_btn)

	var save_btn := Button.new()
	save_btn.text = "Save Book..."
	save_btn.pressed.connect(_on_save_book)
	dock.add_child(save_btn)

	file_dialog = FileDialog.new()
	file_dialog.access = FileDialog.ACCESS_FILESYSTEM
	file_dialog.file_mode = FileDialog.FILE_MODE_SAVE_FILE
	file_dialog.filters = PackedStringArray(["*.tres"])
	file_dialog.file_selected.connect(_on_file_selected)
	dock.add_child(file_dialog)

func _exit_tree() -> void:
	remove_control_from_docks(dock)
	dock.free()

func _on_new_book() -> void:
	current_book = DialogBook.new()
	current_page = null
	current_answer = null
	get_editor_interface().edit_resource(current_book)

func _on_add_page() -> void:
	if current_book == null:
		push_error("Create a DialogBook first")
		return
	var page := DialogPage.new()
	current_book.pages.append(page)
	current_page = page
	current_answer = null
	get_editor_interface().edit_resource(page)

func _on_add_answer() -> void:
	if current_page == null:
		push_error("Add a page first")
		return
	var ans := DialogAnswer.new()
	current_page.answers.append(ans)
	current_answer = ans
	get_editor_interface().edit_resource(ans)

func _on_add_action() -> void:
	if current_answer == null:
		push_error("Add an answer first")
		return
	var act := DialogAction.new()
	current_answer.actions.append(act)
	get_editor_interface().edit_resource(act)

func _on_add_requirement() -> void:
	if current_answer == null:
		push_error("Add an answer first")
		return
	var req := DialogRequirement.new()
	current_answer.requirements.append(req)
	get_editor_interface().edit_resource(req)

func _on_save_book() -> void:
	if current_book == null:
		push_error("Nothing to save")
		return
	file_dialog.popup_centered()

func _on_file_selected(path: String) -> void:
	if current_book:
		var err := ResourceSaver.save(current_book, path)
		if err != OK:
			push_error("Failed to save dialog: %s" % [error_string(err)])
