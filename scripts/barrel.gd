extends Area2D

const FUEL_AMOUNT := 500.0

func _ready() -> void:
	# Upewniamy się, że monitoring jest włączony
	monitoring = true
	# Tworzymy Callable raz
	var cb = Callable(self, "_on_body_entered")
	# Podłączamy tylko, jeśli jeszcze nie ma połączenia
	if not body_entered.is_connected(cb):
		body_entered.connect(cb)

func _on_body_entered(body: Node) -> void:
	# tylko pojazdy zbierają baryłki
	if not body.is_in_group("vehicles"):
		return

	var car = body
	car.current_fuel = min(car.current_fuel + FUEL_AMOUNT, car.max_fuel)
	print("Zebrano baryłkę paliwa: +%d (teraz car.current_fuel=%d)" % [FUEL_AMOUNT, car.current_fuel])

	# odśwież UI
	var ui = get_tree().get_current_scene().get_node_or_null("MobileControls")
	if ui:
		ui.update_fuel_display(car.current_fuel, car.max_fuel)

	# usuń baryłkę
	queue_free()
