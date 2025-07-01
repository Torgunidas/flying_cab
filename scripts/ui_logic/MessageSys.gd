extends CanvasLayer
class_name MessageSys

# Czas domyślny (sekundy)
@export var default_time: float = 2.0

@onready var panel: Panel = $SysMessagePanel      # Panel z ramką
@onready var label: Label = %SysMessage           # Label wewnątrz panelu

var _queue: Array = []        # Kolejka komunikatów
var _timer: Timer             # Jednorazowy timer

func _ready() -> void:
	panel.visible = false     # Ukrywamy wszystko na starcie
	_timer = Timer.new()
	_timer.one_shot = true
	add_child(_timer)
	_timer.timeout.connect(_show_next)

func show_message(text: String, duration: float = -1.0) -> void:
	# Jeżeli nie podano czasu, użyj default_time
	var t := duration if duration > 0.0 else default_time
	_queue.push_back({"text": text, "time": t})
	# Jeśli panel jest już widoczny, poczekaj aż zniknie
	if panel.visible:
		return
	_show_next()

func _show_next() -> void:
	# Brak kolejnych wiadomości – chowamy panel
	if _queue.is_empty():
		panel.visible = false
		return

	var msg = _queue.pop_front()
	label.text = msg["text"]
	panel.visible = true          # Pokazujemy panel (label pokaże się automatycznie)
	_timer.start(msg["time"])     # Odliczamy czas wyświetlania
