extends Area2D          # ① Działa jako “trigger” – wykrywa wejście ciał w obszar

func _ready() -> void:
	# ② Podłączamy sygnał 'body_entered' programowo, żeby nie robić tego ręcznie w Inspectorze
	connect("body_entered", Callable(self, "_on_body_entered"))

func _on_body_entered(body: Node) -> void:
	# ③ Jeżeli obiekt jest w grupie 'vehicles' i ma metodę śmierci → wywołujemy eksplozję
	if body.is_in_group("vehicles") and body.has_method("_on_car_death"):
		body._on_car_death()
		return

	# ④ Jeżeli to gracz (grupa 'player') i NIE jest w aucie → od razu ginie
	if body.is_in_group("player") \
	and not body.in_vehicle \
	and body.has_method("_on_player_death"):
		body._on_player_death()
		return
