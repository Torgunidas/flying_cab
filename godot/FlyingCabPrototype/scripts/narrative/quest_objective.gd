@tool
class_name QuestObjective
extends Resource
const EVENTS := ["ride_completed", "passenger_boarded", "credits_earned", "fuel_purchased", "vehicle_repaired", "vehicle_entered", "vehicle_exited", "vehicle_taken", "area_entered", "medicine_delivered"]
@export var id: StringName
@export_multiline var description := "Nowy cel"
@export_enum("event", "state") var mode := "event"
@export_enum("ride_completed", "passenger_boarded", "credits_earned", "fuel_purchased", "vehicle_repaired", "vehicle_entered", "vehicle_exited", "vehicle_taken", "area_entered", "medicine_delivered", "custom") var event := "ride_completed"
## Used when Event = custom; register it in NarrativeCatalog.custom_events.
@export var custom_event: StringName
## Number of events, or total amount for fuel/credits/repair. Use whole numbers for rides.
# Range snaps relative to its minimum; align the step so entering 2 stays exactly 2.
@export_range(0.001, 1000000, 0.001, "or_greater") var required := 1.0
@export_group("Event filters (empty accepts any)")
@export var target_id: StringName
@export var origin_id: StringName
@export var destination_id: StringName
@export var vehicle_id: StringName
@export_group("State / event requirements")
@export var conditions: Array[NarrativeCondition] = []
## Empty list preserves sequential objectives. Otherwise end with an unconditional fallback.
@export var transitions: Array[QuestTransition] = []

func event_id() -> String:
	return String(custom_event) if event == "custom" else event
