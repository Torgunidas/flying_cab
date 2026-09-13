class_name EconomyLedger
extends RefCounted
## Wallet owns receipts. Continuous services retain fractions until display.
signal changed
var state: CampaignState

func credit_once(id: String, amount: float) -> bool:
	if state == null or not is_finite(state.credits) or state.credits < 0 or id.is_empty() or not is_finite(amount) or amount < 0 or state.receipts.has(id) or not is_finite(state.credits + amount):
		return false
	state.receipts[id] = amount
	state.credits += amount
	changed.emit()
	return true

func spend(amount: float) -> bool:
	if state == null or not is_finite(state.credits) or state.credits < 0 or not is_finite(amount) or amount < 0 or amount > state.credits + 0.0000001:
		return false
	state.credits = maxf(0, state.credits - amount)
	changed.emit()
	return true

func purchase_units(requested: float, needed: float, price: float) -> float:
	if state == null or not is_finite(state.credits) or state.credits < 0 or not is_finite(requested) or not is_finite(needed) or not is_finite(price) or requested <= 0 or needed <= 0 or price < 0:
		return 0
	var units := minf(requested, needed)
	if price > 0:
		units = minf(units, state.credits / price)
	return units if spend(units * price) else 0.0
