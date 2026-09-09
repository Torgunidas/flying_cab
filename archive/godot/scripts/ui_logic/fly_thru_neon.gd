# NeonLabelSettings.gd  (Godot 4.4) – z pauzami ON / OFF
extends Label

@export var transition_time: float = 0.3   # czas płynnego przejścia 0 ↔ 5
@export var hold_on:        float = 0.4    # ile sekund stać na 5
@export var hold_off:       float = 0.4    # ile sekund stać na 0

@export var outline_off:    int   = 0      # grubość OFF
@export var outline_on:     int   = 5      # grubość ON
@export var neon_color:     Color = Color("00ffff")

var _ls: LabelSettings                       # referencja na LabelSettings

func _ready() -> void:
	# 1) weź istniejący LabelSettings lub stwórz nowy
	_ls = label_settings
	if _ls == null:
		_ls = LabelSettings.new()
		label_settings = _ls                # przypnij go do Labela

	# 2) startowe parametry
	_ls.outline_color = neon_color
	_ls.outline_size  = outline_off         # zaczynamy od 0 px

	# 3) budujemy nieskończoną sekwencję tweenów
	var tw := create_tween().set_loops()

	# 0 → 5
	tw.tween_property(_ls, "outline_size", outline_on, transition_time)\
	   .set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)

	# pauza na 5
	tw.tween_interval(hold_on)

	# 5 → 0
	tw.tween_property(_ls, "outline_size", outline_off, transition_time)\
	   .set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)

	# pauza na 0
	tw.tween_interval(hold_off)
