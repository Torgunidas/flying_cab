@tool
class_name NarrativeCondition
extends Resource
## All conditions in an array must pass. Reason is shown on disabled choices.
@export_enum("quest_status", "fact", "credits", "item", "medicine", "campaign_active", "fuel_percent", "vehicle_id", "vehicle_model", "access") var kind := "fact"
@export var key: StringName
@export_enum("inactive", "active", "ready", "completed") var status := "active"
@export_enum(">=", "<=", "==", "!=", ">", "<") var comparison := ">="
@export var amount := 1.0
@export var invert := false
@export_multiline var reason := "Warunek nie został spełniony."
