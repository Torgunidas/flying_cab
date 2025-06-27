extends Resource
class_name DialogAction
@export_enum("add_money","deduct_money","add_life",
						"start_quest","finish_quest",
						"start_animation",
						"close",
						"npc_text") var type := "close"
@export var amount     : int    = 0
@export var quest_id   : String = ""
@export var anim_name  : String = ""
@export var loop_count  : int    = 1
@export var npc_text_id : String = ""
