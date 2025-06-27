extends Resource
class_name DialogAnswer
@export var text         : String = "..."
@export var actions      : Array[DialogAction]      = []
@export var goto_page    : int   = -1       #  -1 = brak skoku
@export var requirements : Array[DialogRequirement] = []
# Pokazuje odpowiedź nawet jeśli wymagania nie są spełnione.
# Gdy false - przycisk zostanie ukryty.
@export var show_if_unavailable : bool = true
