extends Resource
class_name QuestStage

enum StageType {           # automatycznie rozwijalna lista w Inspectorze
	DIALOG,                # 0
	PICKUP,                # 1
	AREA_ENTER,            # 2
	AREA_DELIVER           # 3
}

@export var type : StageType = StageType.AREA_ENTER
@export var target_name : String = ""    # dialog_id (jako tekst), item_name, albo nazwa Area2D
@export var count : int = 1              # tylko dla PICKUP, DIALOG nie używa
@export var msg   : String = ""
