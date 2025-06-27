extends CanvasLayer
class_name DialogManager

# ─────────────────────────────────────────────────────────────
#  SYGNAŁY
# ─────────────────────────────────────────────────────────────
signal mission_start(mission_id)

# ─────────────────────────────────────────────────────────────
#  WĘZŁY UI
# ─────────────────────────────────────────────────────────────
@export var toggle_action : String = "toggle_action"

var _book   : DialogBook   = null    # cały dialog
var _page_i : int          = 0       # indeks bieżącej strony
@onready var panel       : PanelContainer = $DialogPanel
@onready var npc_label   : Label          = %NPCText
@onready var portrait_rect : TextureRect   = %Portrait
@onready var answer_btns : Array[Button]  = [%AnswerA, %AnswerB, %AnswerC]    # 3 przyciski



# ─────────────────────────────────────────────────────────────
#  ZMIENNE ROBOCZE
# ─────────────────────────────────────────────────────────────
var _current_interactable : Node      = null  # obiekt, który otworzył okno
var _gs                   : Node      = null  # autoload GameState
var _visible_answers      : Array[DialogAnswer] = []

# Helper: returns true if the given object exposes a property with the provided name.
func _has_property(obj: Object, prop_name: String) -> bool:
		for info in obj.get_property_list():
				if "name" in info and info.name == prop_name:
						return true
		return false
# ─────────────────────────────────────────────────────────────
#  READY
# ─────────────────────────────────────────────────────────────
func _ready() -> void:
	panel.visible = false
	portrait_rect.visible = false
	_gs = get_node("/root/GameState")
	for i in answer_btns.size():
		answer_btns[i].pressed.connect(_on_answer_pressed.bind(i))
	set_process_input(true)

# ─────────────────────────────────────────────────────────────
#  INPUT – klawisz zamknięcia
# ─────────────────────────────────────────────────────────────
func _input(event : InputEvent) -> void:
	if panel.visible and event.is_action_pressed(toggle_action):
		close_dialog()

# ─────────────────────────────────────────────────────────────
#  API PUBLICZNE
# ─────────────────────────────────────────────────────────────
func start_dialog(data, from : Node = null) -> void:
	# jeśli przekazano pojedynczy DialogData → zrób z niego 1-stronicową książkę
	if data is DialogData:
			_book = DialogBook.new()
			var page := DialogPage.new()
			page.npc_text = data.npc_text
			page.answers  = data.answers
			_book.pages.append(page)
	elif data is DialogBook:
			_book = data
	else:
			push_error("start_dialog(): expected DialogBook or DialogData")
			return

	_current_interactable = from
	if from and _has_property(from, "portrait_texture"):
			var tex = from.portrait_texture
			portrait_rect.texture = tex
			portrait_rect.visible = tex != null
	else:
			portrait_rect.texture = null
			portrait_rect.visible = false
	_page_i = 0
	_show_page()


func close_dialog() -> void:
	panel.visible = false
	portrait_rect.texture = null
	portrait_rect.visible = false
	set_process_input(false)

func is_open() -> bool:
	return panel.visible
	
func _on_answer_pressed(idx : int) -> void:
	if idx >= _visible_answers.size():
				return
	var ans : DialogAnswer = _visible_answers[idx]
	for act in ans.actions: _apply_action(act)

	if ans.goto_page >= 0 and ans.goto_page < _book.pages.size():
		_page_i = ans.goto_page
		_show_page()                      # przełącz stronę
	else:
		close_dialog()

# ─────────────────────────────────────────────────────────────
#  REQUIREMENTS
# ─────────────────────────────────────────────────────────────
func _requirements_met(ans : DialogAnswer) -> bool:
	for req : DialogRequirement in ans.requirements:
		match req.type:
			"money_req":
				if not _check_money(req):        # brak kasy → disabled
					return false
			"active_quest":
				if not QuestSys.is_active(req.quest_id):
					return false
	return true

func _check_money(req : DialogRequirement) -> bool:
	var wallet : int = _gs.get_money()
	match req.cmp:
		"<":  return wallet <  req.amount
		"=":  return wallet == req.amount
		">":  return wallet >  req.amount
		_:    return false

# ─────────────────────────────────────────────────────────────
#  AKCJE
# ─────────────────────────────────────────────────────────────
func _apply_action(act : DialogAction) -> void:
	var pc : PlayerCharacter = get_tree().get_first_node_in_group("player") \
							   as PlayerCharacter

	match act.type:
		"add_money":        _gs.add_money(   act.amount)
		"deduct_money":     _gs.spend_money( act.amount)
		"add_life":         if pc: pc.heal(act.amount)

		"start_quest":      QuestSys.start_quest(  act.quest_id)
		"finish_quest":     QuestSys.finish_quest( act.quest_id)

		"start_animation":
			if _current_interactable:
		# 1) jeśli to stacja benzynowa → wołamy play_refuel_animation
				if _current_interactable.has_method("play_refuel_animation"):
					_current_interactable.play_refuel_animation(
						act.anim_name,
						max(1, act.loop_count)
					)
		# 2) w pozostałych przypadkach (np. QuestGiver) → stara metoda
				elif _current_interactable.has_method("play_dialog_animation"):
					_current_interactable.play_dialog_animation(
						act.anim_name,
						max(1, act.loop_count)
					)

		"npc_text":
			var branch := DialogData.new()
			branch.npc_text = act.npc_text_id
			start_dialog(branch, _current_interactable)

		"close":
			pass   # nic – samo zamknięcie już nastąpi w _on_answer_pressed
			
			
func _show_page() -> void:
	var page : DialogPage = _book.pages[_page_i]
	npc_label.text = page.npc_text

	_visible_answers.clear()
	var btn_i := 0
	for ans in page.answers:
				var enabled := _requirements_met(ans)
				if not enabled and not ans.show_if_unavailable:
						continue
				if btn_i >= answer_btns.size():
						break
				var btn := answer_btns[btn_i]
				btn.text     = ans.text
				btn.disabled = not enabled
				btn.visible  = true
				_visible_answers.append(ans)
				btn_i += 1

	for i in range(btn_i, answer_btns.size()):
				answer_btns[i].visible = false

	panel.visible = true
