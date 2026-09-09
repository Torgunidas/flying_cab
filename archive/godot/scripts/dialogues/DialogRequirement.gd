extends Resource
class_name DialogRequirement
@export_enum("money_req","active_quest","has_item") var type := "money_req"

@export var amount    : int    = 0        # money_req
@export_enum("<","="," >")      var cmp  := ">"  # <  =  >
@export var quest_id  : String = ""       # active_quest
@export var item      : ItemData
