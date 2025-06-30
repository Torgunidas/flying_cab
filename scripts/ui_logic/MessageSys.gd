extends CanvasLayer
class_name MessageSys

# Prosty system do wyświetlania krótkich komunikatów na środku ekranu.

@export var default_time: float = 2.0

@onready var label: Label = $Panel/Label

var _queue: Array = []
var _timer: Timer

func _ready() -> void:
	label.visible = false
	_timer = Timer.new()
	_timer.one_shot = true
	add_child(_timer)
	_timer.timeout.connect(_show_next)

func show_message(text: String, duration: float = -1.0) -> void:
	var t = duration if duration > 0.0 else default_time
	_queue.append({"text": text, "time": t})
	if label.visible:
		return
	_show_next()

func _show_next() -> void:
	if _queue.is_empty():
		label.visible = false
		return
	var msg = _queue.pop_front()
	label.text = msg["text"]
	label.visible = true
	_timer.start(msg["time"])
