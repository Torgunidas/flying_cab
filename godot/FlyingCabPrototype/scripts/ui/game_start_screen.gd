@tool
class_name GameStartScreen
extends CanvasLayer
## Presentation only. The composition root owns saves and session replacement.
signal continue_requested
signal new_game_requested
@onready var root: Control = $Root
@onready var artwork: TextureRect = $Root/Artwork
@onready var actions: VBoxContainer = $Root/Actions
@onready var status: Label = $Root/Actions/Status
@onready var continue_button: Button = $Root/Actions/Continue
@onready var new_game_button: Button = $Root/Actions/NewGame
@onready var cancel_button: Button = $Root/Actions/Cancel
@onready var progress: ProgressBar = $Root/Actions/Progress
var _has_save := false
var _confirming := false

func _ready() -> void:
	root.resized.connect(_layout)
	actions.minimum_size_changed.connect(_layout)
	continue_button.pressed.connect(func(): continue_requested.emit())
	new_game_button.pressed.connect(_new_game)
	cancel_button.pressed.connect(func(): show_menu(_has_save))
	_layout()

func _layout() -> void:
	if not is_node_ready(): return
	var size := root.size
	var width := minf(360, size.x - 48)
	if size.x > size.y * 1.15:
		width = minf(360, size.x * 0.44 - 32)
		actions.size.x = width
		actions.size.y = actions.get_combined_minimum_size().y
		actions.position = Vector2(size.x * 0.76 - width * 0.5, (size.y - actions.size.y) * 0.5)
		artwork.position = Vector2(16, 16)
		artwork.size = Vector2(size.x * 0.52 - 32, size.y - 32)
	else:
		actions.size.x = width
		actions.size.y = actions.get_combined_minimum_size().y
		actions.position = Vector2((size.x - width) * 0.5, size.y - actions.size.y - 28)
		artwork.position = Vector2(8, 8)
		artwork.size = Vector2(size.x - 16, maxf(0, actions.position.y - 20))

func show_menu(has_save: bool, error := "") -> void:
	_has_save = has_save
	_confirming = false
	show()
	continue_button.show()
	continue_button.disabled = not has_save
	continue_button.tooltip_text = "" if has_save else "Brak zapisanej gry"
	new_game_button.show()
	new_game_button.disabled = false
	new_game_button.text = "Nowa gra"
	cancel_button.hide()
	status.text = error
	status.visible = not error.is_empty()
	progress.hide()
	_layout.call_deferred()
	if not Engine.is_editor_hint():
		(continue_button if has_save else new_game_button).grab_focus.call_deferred()

func _new_game() -> void:
	if new_game_button.disabled: return
	if _has_save and not _confirming:
		_confirming = true
		continue_button.hide()
		status.text = "Rozpocząć od początku?\nObecny zapis i cały postęp zostaną zastąpione."
		status.show()
		new_game_button.text = "Rozpocznij od nowa"
		cancel_button.show()
		cancel_button.grab_focus()
		_layout.call_deferred()
	else:
		new_game_button.disabled = true
		continue_button.disabled = true
		new_game_requested.emit()

func show_loading() -> void:
	show()
	continue_button.hide()
	new_game_button.hide()
	cancel_button.hide()
	status.text = "Przygotowanie miasta…"
	status.show()
	progress.value = 0
	progress.show()
	_layout.call_deferred()

func _unhandled_key_input(event: InputEvent) -> void:
	if visible and _confirming and event is InputEventKey and event.pressed and event.physical_keycode == KEY_ESCAPE:
		show_menu(_has_save)
		get_viewport().set_input_as_handled()
