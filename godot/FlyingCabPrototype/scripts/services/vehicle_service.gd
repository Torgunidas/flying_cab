class_name VehicleService
extends RefCounted
## Atomic purchase in HP, shared by workshops and future mechanics/dialogues.
signal repaired(vehicle_id: StringName, restored: float, cost: float)

func repair(state: VehicleState, model: VehicleDefinition, campaign: CampaignState, amount: float, cost_per_unit: float, ledger: EconomyLedger = null) -> float:
	if state == null or model == null or campaign == null or amount <= 0 or cost_per_unit < 0 or not is_finite(amount) or not is_finite(cost_per_unit):
		return 0.0
	if not is_finite(model.max_hull) or model.max_hull <= 0.0 or not is_finite(campaign.credits) or campaign.credits < 0.0:
		return 0.0
	var restored := minf(amount, (1.0 - state.condition) * model.max_hull)
	if cost_per_unit > 0.0:
		restored = minf(restored, maxf(0.0, campaign.credits) / cost_per_unit)
	var cost := restored * cost_per_unit
	if restored <= 0.0:
		return 0.0
	if ledger:
		if not ledger.spend(cost):
			return 0.0
	else:
		campaign.credits = maxf(0.0, campaign.credits - cost)
	state.condition = minf(1.0, state.condition + restored / model.max_hull)
	repaired.emit(state.entity_id, restored, cost)
	return restored
