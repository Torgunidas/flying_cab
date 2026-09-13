class_name CampaignState
extends RefCounted
## Inactive until an authored campaign supplies balance and time policy.
signal medicine_delivered(delivery_id: StringName, seconds_added: float)
signal expired
var active := false
var remaining_seconds := 0.0
var credits := 0.0
var receipts: Dictionary = {}
var medicine_doses := 0
var flags: Dictionary = {}
var completed_deliveries: Dictionary = {}

func advance(dt: float) -> void:
	if not active or dt <= 0 or not is_finite(dt):
		return
	remaining_seconds = maxf(0, remaining_seconds - dt)
	if remaining_seconds == 0:
		active = false
		expired.emit()

func deliver_medicine(id: StringName, seconds: float) -> bool:
	if not active or id == &"" or completed_deliveries.has(String(id)) or medicine_doses < 1 or seconds <= 0 or not is_finite(seconds):
		return false
	medicine_doses -= 1
	remaining_seconds += seconds
	completed_deliveries[String(id)] = true
	medicine_delivered.emit(id, seconds)
	return true

func snapshot() -> Dictionary:
	return {"active": active, "remaining_seconds": remaining_seconds, "credits": credits, "receipts": receipts.duplicate(true), "medicine_doses": medicine_doses, "flags": flags.duplicate(true), "completed_deliveries": completed_deliveries.duplicate(true)}

static func from_snapshot(data: Dictionary) -> CampaignState:
	if not data.get("active") is bool or not data.get("flags") is Dictionary or not data.get("completed_deliveries") is Dictionary:
		return null
	for key in ["remaining_seconds", "credits", "medicine_doses"]:
		if not (data.get(key) is int or data.get(key) is float) or not is_finite(float(data[key])) or data[key] < 0:
			return null
	if float(data.medicine_doses) != floorf(float(data.medicine_doses)) or (data.active and data.remaining_seconds <= 0):
		return null
	var receipts_data = data.get("receipts", {})
	if not receipts_data is Dictionary:
		return null
	for id in receipts_data:
		if not id is String or id.is_empty() or not (receipts_data[id] is int or receipts_data[id] is float) or not is_finite(receipts_data[id]) or receipts_data[id] < 0:
			return null
	var state := CampaignState.new()
	state.receipts = receipts_data.duplicate(true)
	state.active = data.active
	state.remaining_seconds = data.remaining_seconds
	state.credits = data.credits
	state.medicine_doses = int(data.medicine_doses)
	state.flags = data.flags.duplicate(true)
	state.completed_deliveries = data.completed_deliveries.duplicate(true)
	return state
