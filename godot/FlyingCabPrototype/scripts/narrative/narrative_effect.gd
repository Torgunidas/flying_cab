@tool
class_name NarrativeEffect
extends Resource
## A list is validated and committed atomically; no method names or executable code.
@export_enum("set_fact", "start_quest", "turn_in", "credit", "spend", "grant_item", "remove_item", "start_campaign", "buy_medicine", "deliver_medicine", "grant_access", "revoke_access") var kind := "set_fact"
@export var key: StringName
@export_range(0, 1000000, 0.1, "or_greater") var amount := 1.0
## Buy medicine: price per dose. Deliver medicine: seconds added per dose.
@export_range(0, 1000000, 0.1, "or_greater") var value := 0.0
