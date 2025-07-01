extends Area2D
class_name EndTrigger

@export var quest_id           : String
@export var reward_scene       : PackedScene
@export var reward_texture     : Texture
@export var reward_offset      : Vector2 = Vector2.ZERO
@export var required_car_id    : String  = ""   # puste → wystarczy sam gracz
@export_range(0.0, 100.0, 1.0) var min_fuel_percent : float = -1.0   # <0 → brak wymogu
@export var complete_on_enter  : bool    = true

func _ready() -> void:
	body_entered.connect(_on_enter)

func _on_enter(body : Node) -> void:
	var valid := false

	# ─────────────────────────────────────────────────────────────
	# 1)  NIE wymagamy konkretnego auta – wystarczy gracz na piechotę
	# ─────────────────────────────────────────────────────────────
	if required_car_id == "":
		valid = body.is_in_group("player")

	# ─────────────────────────────────────────────────────────────
	# 2)  WYMAGAMY wjazdu graczem w pojeździe o zadanym ID
	# ─────────────────────────────────────────────────────────────
	else:
		# Trigger zwraca kolizję *samochodu* (PlayerCharacter ma wyłączoną kolizję
		# albo jest ukryty), więc sprawdzamy czy:
		#   a) to naprawdę Car o poprawnym ID i
		#   b) siedzi w nim PlayerCharacter.
		if body is Car:
			var car := body as Car
			if car.car_id == required_car_id:
				var pc := get_tree().get_first_node_in_group("player") as PlayerCharacter
				if pc and pc.in_vehicle and pc.vehicle == car:
					valid = true

	# ─────────────────────────────────────────────────────────────
	# 3)  Reagujemy tylko, gdy warunek spełniony *i* flaga pozwala
	# ─────────────────────────────────────────────────────────────
	if not valid or not complete_on_enter:
		return
		
	if min_fuel_percent >= 0.0:
		var pc := get_tree().get_first_node_in_group("player") as PlayerCharacter
		if pc and pc.in_vehicle and pc.vehicle:
			var car := pc.vehicle
			if car.max_fuel > 0.0:
				var percent := 100.0 * car.current_fuel / car.max_fuel
				if percent < min_fuel_percent:
					return
			else:
				return
		else:
			return

	_complete_quest_and_spawn_reward()


# ==========  P R I V A T E  ==========

func _complete_quest_and_spawn_reward() -> void:
	var q := QuestSys.get_quest(quest_id)
	if q and q.status == QuestData.Status.ACTIVE:
		QuestSys.complete(quest_id)

		if reward_scene:
			var box := reward_scene.instantiate()
			get_tree().current_scene.add_child(box)
			box.global_position = global_position + reward_offset

			var spr := box.get_node("Sprite2D") as Sprite2D
			if spr and reward_texture:
				spr.texture = reward_texture

		queue_free()   # trigger spełniony → usuń
