class_name NarrativeService
extends RefCounted
## Session-owned quest state and transactional dialogue commands. Resources never mutate.
signal changed
signal checkpoint_requested(urgent: bool)
signal notice(text: String)
var context: RuntimeContext
var catalog: NarrativeCatalog
var data := empty_state()
var _busy := false
var _work: Dictionary
var _campaign: CampaignState
var _wallet: EconomyLedger
var _pending: Array[Dictionary] = []
var _notices: Array[String] = []
var _deliveries: Array[Dictionary] = []
var _work_error := ""
var _operation := 0

static func empty_state() -> Dictionary:
	return {"quests": {}, "items": {}, "access": {}, "receipts": {}, "events": {}, "serial": 0, "tracked": ""}

func status(id: StringName, state: Dictionary = data) -> String:
	return state.quests.get(String(id), {}).get("status", "inactive")

func reason(conditions: Array[NarrativeCondition], state: Dictionary = data, campaign: CampaignState = null) -> String:
	var source := campaign if campaign else context.campaign
	for condition in conditions:
		if not _matches(condition, state, source):
			return condition.reason if not condition.reason.is_empty() else "Warunek nie został spełniony."
	return ""

func _compare(a: float, op: String, b: float) -> bool:
	match op:
		">=": return a >= b
		"<=": return a <= b
		"==": return is_equal_approx(a, b)
		"!=": return not is_equal_approx(a, b)
		">": return a > b
		"<": return a < b
	return false

func _matches(c: NarrativeCondition, state: Dictionary, campaign: CampaignState) -> bool:
	if c == null:
		return false
	var result := false
	var vehicle: VehicleState
	if context and context.player.focus is FlightCab:
		vehicle = context.player.focus.state
	match c.kind:
		"quest_status": result = status(c.key, state) == c.status
		"fact": result = _compare(float(campaign.flags.get(String(c.key), 0)), c.comparison, c.amount)
		"credits": result = _compare(campaign.credits, c.comparison, c.amount)
		"item": result = _compare(state.items.get(String(c.key), 0), c.comparison, c.amount)
		"medicine": result = _compare(campaign.medicine_doses, c.comparison, c.amount)
		"campaign_active": result = campaign.active
		"access": result = state.access.get(String(c.key), false)
		"vehicle_id": result = vehicle != null and vehicle.entity_id == c.key
		"vehicle_model": result = vehicle != null and vehicle.model_id == c.key
		"fuel_percent":
			if vehicle:
				var model := context.vehicle_catalog.find_model(vehicle.model_id)
				result = model != null and _compare(vehicle.fuel / model.fuel_capacity * 100, c.comparison, c.amount)
	return not result if c.invert else result

func can_start(id: StringName, state: Dictionary = data, campaign: CampaignState = null) -> String:
	var q := catalog.quest(id)
	if q == null:
		return "Nieznane zadanie."
	if status(id, state) != "inactive":
		return "To zadanie zostało już przyjęte."
	for prerequisite in q.prerequisites:
		if status(StringName(prerequisite), state) != "completed":
			return "Najpierw ukończ: " + catalog.quest(StringName(prerequisite)).title
	return reason(q.conditions, state, campaign)

func preview_effects(effects: Array[NarrativeEffect], once_id := "", npc_id := "") -> String:
	return _plan(effects, once_id, npc_id)

## Read-only map query. Follow the actual entry and reachable choices, including
## introductions before an offer. A separate planner never touches live work.
func available_quest_topics(npc: NpcDefinition) -> PackedStringArray:
	var result := PackedStringArray()
	if context == null or catalog == null or npc == null or catalog.npc(npc.id) != npc or context.campaign.flags.get("__campaign_expired", false):
		return result
	for topic in npc.topics:
		if topic == null or topic.dialogue == null or not reason(topic.conditions).is_empty():
			continue
		var entry := topic.dialogue.start_node
		for rule in topic.dialogue.entries:
			if reason(rule.conditions).is_empty():
				entry = rule.node_id
				break
		if _has_quest_action(topic.dialogue, entry, String(npc.id), data, context.campaign, []):
			result.append(topic.title)
	return result

func _has_quest_action(dialogue: DialogueDefinition, node_id: StringName, npc_id: String, state: Dictionary, campaign: CampaignState, path: Array) -> bool:
	var node := dialogue.find_node(node_id)
	if node == null or path.size() >= 128:
		return false
	# Ignore the planning serial, but revisit a node when a prior choice changed
	# its conditions. Bound repeatable economic loops in malformed authored data.
	var visit := [node_id, state.quests, state.items, state.access, state.receipts, campaign.snapshot()]
	if path.has(visit):
		return false
	var visited := path.duplicate()
	visited.append(visit)
	var probe := NarrativeService.new()
	probe.context = context
	probe.catalog = catalog
	probe.data = state
	for choice in node.choices:
		if not reason(choice.conditions, state, campaign).is_empty():
			continue
		var once_id := "%s/%s/%s" % [dialogue.id, node.id, choice.id] if choice.once else ""
		if not probe._plan(choice.effects, once_id, npc_id, campaign).is_empty():
			continue
		for effect in choice.effects:
			if effect.kind in ["start_quest", "turn_in", "deliver_medicine"]:
				return true
		# Authored fact/item choices may also advance an active objective.
		if probe._work.quests != state.quests:
			return true
		if choice.next_node != &"" and _has_quest_action(dialogue, choice.next_node, npc_id, probe._work, probe._campaign, visited):
			return true
	return false

func execute(effects: Array[NarrativeEffect], once_id := "", npc_id := "") -> String:
	if _busy:
		return "Poczekaj na zakończenie działania."
	_busy = true
	var error := _plan(effects, once_id, npc_id)
	if error.is_empty():
		_commit(true)
	_busy = false
	return error

func _plan(effects: Array[NarrativeEffect], once_id: String, npc_id: String, campaign_source: CampaignState = null) -> String:
	if context == null or catalog == null:
		return "Brak katalogu narracji."
	var source := campaign_source if campaign_source else context.campaign
	if source.flags.get("__campaign_expired", false):
		return "Czas się skończył."
	if not once_id.is_empty() and data.receipts.has(once_id):
		return "To działanie zostało już wykonane."
	var errors := PackedStringArray()
	for effect in effects:
		catalog._effect(effect, "Action", errors)
	if not errors.is_empty():
		return "\n".join(errors)
	_begin_work(source)
	for effect in effects:
		var effect_error := _effect(effect, npc_id)
		if not effect_error.is_empty():
			return effect_error
	if not once_id.is_empty():
		_work.receipts[once_id] = true
	var error := _settle()
	if error.is_empty() and (not _work_error.is_empty() or CampaignState.from_snapshot(_campaign.snapshot()) == null or not validate_snapshot(_work)):
		return "Nieprawidłowy wynik działania. Sprawdź wartości zasobów."
	return error

func _begin_work(campaign_source: CampaignState = null) -> void:
	_work = data.duplicate(true)
	_work.serial += 1
	_campaign = CampaignState.from_snapshot((campaign_source if campaign_source else context.campaign).snapshot())
	_wallet = EconomyLedger.new()
	_wallet.state = _campaign
	_pending.clear()
	_notices.clear()
	_deliveries.clear()
	_work_error = ""
	_operation = 0

func _receipt(kind: String) -> String:
	_operation += 1
	return "narrative/%d/%s/%d" % [_work.serial, kind, _operation]

func _effect(e: NarrativeEffect, npc_id: String) -> String:
	var key := String(e.key)
	match e.kind:
		"set_fact": _campaign.flags[key] = e.amount
		"start_quest":
			var error := can_start(e.key, _work, _campaign)
			if not error.is_empty(): return error
			_start(catalog.quest(e.key))
		"turn_in":
			var q := catalog.quest(e.key)
			if status(e.key, _work) != "ready": return "Zadanie nie jest gotowe do oddania."
			if q.turn_in_npc != &"" and String(q.turn_in_npc) != npc_id: return "Wróć do właściwego rozmówcy."
			_complete(q)
		"credit": _credit(e.amount)
		"spend":
			if not _wallet.spend(e.amount): return "Brakuje pieniędzy."
		"grant_item": _work.items[key] = _work.items.get(key, 0) + int(e.amount)
		"remove_item":
			if _work.items.get(key, 0) < e.amount: return "Brakuje przedmiotu."
			_work.items[key] -= int(e.amount)
		"start_campaign":
			if _campaign.active or _campaign.flags.get("__campaign_started", false): return "Odliczanie zostało już rozpoczęte."
			_campaign.active = true
			_campaign.remaining_seconds = e.amount
			_campaign.flags["__campaign_started"] = true
		"buy_medicine":
			if not _wallet.spend(e.amount * e.value): return "Brakuje pieniędzy na lek."
			_campaign.medicine_doses += int(e.amount)
		"deliver_medicine":
			if not _campaign.active or _campaign.medicine_doses < int(e.amount): return "Nie możesz teraz podać tej dawki."
			for i in int(e.amount):
				var delivery := _receipt("delivery")
				if not _campaign.deliver_medicine(StringName(delivery), e.value): return "Nie udało się przekazać leku."
				_pending.append({"event": "medicine_delivered", "target": npc_id, "amount": 1.0})
				_deliveries.append({"id": delivery, "seconds": e.value})
		"grant_access": _work.access[key] = true
		"revoke_access": _work.access.erase(key)
	return ""

func _credit(amount: float) -> void:
	if amount == 0: return
	var receipt := _receipt("credit")
	if not _wallet.credit_once(receipt, amount):
		_work_error = "Nie udało się przyznać kredytów."
		return
	if amount > 0:
		_pending.append({"event": "credits_earned", "target": "quest", "amount": amount})

func _start(q: QuestDefinition) -> void:
	var progress := {}
	for objective in q.objectives:
		progress[String(objective.id)] = 0.0
	_work.quests[String(q.id)] = {"version": q.version, "status": "active", "step": String(q.objectives[0].id), "progress": progress}
	if q.has_branches():
		_work.quests[String(q.id)].path = [String(q.objectives[0].id)]
	if _work.tracked.is_empty():
		_work.tracked = String(q.id)
	_notices.append("Nowe zadanie: " + q.title)

func _complete(q: QuestDefinition) -> void:
	_work.quests[String(q.id)].status = "completed"
	_work.quests[String(q.id)].step = ""
	if _work.tracked == String(q.id):
		_work.tracked = ""
	_credit(q.reward_credits)
	for reward in q.rewards:
		_effect(reward, "")
	_notices.append("Ukończono: " + q.title)

func _advance(q: QuestDefinition, objective: QuestObjective, amount: float) -> void:
	var state: Dictionary = _work.quests[String(q.id)]
	var key := String(objective.id)
	state.progress[key] = minf(objective.required, state.progress[key] + amount)
	if state.progress[key] < objective.required:
		return
	var index := q.objectives.find(objective) + 1
	var next_id := String(q.objectives[index].id) if index < q.objectives.size() else ""
	if not objective.transitions.is_empty():
		for transition in objective.transitions:
			if reason(transition.conditions, _work, _campaign).is_empty():
				next_id = String(transition.target)
				break
	if not next_id.is_empty():
		state.step = next_id
		if q.has_branches(): state.path.append(next_id)
		var next_objective := _current(q)
		if next_objective: _notices.append(next_objective.description)
	elif q.requires_turn_in:
		state.step = ""
		state.status = "ready"
		_notices.append("Możesz oddać zadanie: " + q.title)
	else:
		_complete(q)

func _current(q: QuestDefinition) -> QuestObjective:
	var step: String = _work.quests.get(String(q.id), {}).get("step", "")
	for objective in q.objectives:
		if String(objective.id) == step:
			return objective
	return null

func _settle() -> String:
	# Bound malformed auto-start/content cascades; never spin inside an authored graph.
	for iteration in 512:
		var altered := false
		if not _pending.is_empty():
			var event: Dictionary = _pending.pop_front()
			_apply_event(event)
			altered = true
		for q in catalog.quests:
			if q.auto_start and can_start(q.id, _work, _campaign).is_empty():
				_start(q)
				altered = true
			if status(q.id, _work) != "active": continue
			var objective := _current(q)
			if objective and objective.mode == "state" and reason(objective.conditions, _work, _campaign).is_empty():
				_advance(q, objective, objective.required)
				altered = true
		if not altered:
			return ""
	return "Zbyt długi łańcuch automatycznych zmian. Sprawdź katalog."

func _apply_event(event: Dictionary) -> void:
	# Capture the active objective before advancing any quest: an event counts once per quest.
	var targets: Array = []
	for q in catalog.quests:
		if status(q.id, _work) != "active": continue
		var objective := _current(q)
		if objective == null or objective.mode != "event" or objective.event_id() != event.event: continue
		var matches := true
		for pair in [["target", objective.target_id], ["origin", objective.origin_id], ["destination", objective.destination_id], ["vehicle", objective.vehicle_id]]:
			if pair[1] != &"" and String(pair[1]) != event.get(pair[0], ""):
				matches = false
		if matches and reason(objective.conditions, _work, _campaign).is_empty():
			targets.append([q, objective])
	for pair in targets:
		_advance(pair[0], pair[1], event.get("amount", 1.0))

func record_event(event: String, payload: Dictionary = {}, receipt := "") -> bool:
	if _busy or catalog == null or context.campaign.flags.get("__campaign_expired", false): return false
	if event not in QuestObjective.EVENTS and not catalog.custom_events.has(event): return false
	var amount = payload.get("amount", 1.0)
	if not (amount is float or amount is int) or not is_finite(amount) or amount <= 0 or (not receipt.is_empty() and data.events.has(receipt)): return false
	_busy = true
	_begin_work()
	var item := payload.duplicate(true)
	item.event = event
	_pending.append(item)
	if not receipt.is_empty(): _work.events[receipt] = true
	var error := _settle()
	if error.is_empty() and _work_error.is_empty():
		# Continuous fuel/repair input must not save the whole city every frame.
		if _work.quests != data.quests or not receipt.is_empty(): _commit()
	else:
		error = "Nie udało się zapisać postępu."
	_busy = false
	return error.is_empty()

func refresh_state() -> void:
	if _busy or catalog == null or context.campaign.flags.get("__campaign_expired", false): return
	_busy = true
	_begin_work()
	if _settle().is_empty() and _work_error.is_empty() and (_work.quests != data.quests or _campaign.flags != context.campaign.flags):
		_commit()
	_busy = false

func _commit(urgent := false) -> void:
	# Publish only after every owner and durable receipt has its final value.
	var live := context.campaign
	live.active = _campaign.active
	live.remaining_seconds = _campaign.remaining_seconds
	live.credits = _campaign.credits
	live.receipts = _campaign.receipts.duplicate(true)
	live.medicine_doses = _campaign.medicine_doses
	live.flags = _campaign.flags.duplicate(true)
	live.completed_deliveries = _campaign.completed_deliveries.duplicate(true)
	data = _work.duplicate(true)
	# Copy notifications before callbacks may query another planned choice.
	var messages := _notices.duplicate()
	var deliveries := _deliveries.duplicate(true)
	context.ledger.changed.emit()
	for delivery in deliveries:
		live.medicine_delivered.emit(StringName(delivery.id), delivery.seconds)
	changed.emit()
	for message in messages:
		notice.emit(message)
	checkpoint_requested.emit(urgent)

func track(id: String) -> void:
	if id.is_empty() or status(StringName(id)) in ["active", "ready"]:
		data.tracked = id
		changed.emit()
		checkpoint_requested.emit(true)

func snapshot() -> Dictionary:
	return data.duplicate(true)

func validate_snapshot(value: Dictionary) -> bool:
	for key in ["quests", "items", "access", "receipts", "events"]:
		if not value.get(key) is Dictionary: return false
	if not value.get("tracked") is String or not _whole(value.get("serial")): return false
	for key in ["access", "receipts", "events"]:
		for id in value[key]:
			if not id is String or id.is_empty() or not value[key][id] is bool or not value[key][id]: return false
	for id in value.access:
		if not catalog.access_ids.has(id): return false
	for id in value.items:
		var known := false
		for item in catalog.items:
			if String(item.id) == id: known = true
		if not known or not _whole(value.items[id]): return false
	for id in value.quests:
		if not id is String: return false
		var q := catalog.quest(StringName(id))
		var state = value.quests[id]
		if q == null or not state is Dictionary or state.get("version") != q.version or state.get("status") not in ["active", "ready", "completed"] or not state.get("step") is String or not state.get("progress") is Dictionary or state.progress.size() != q.objectives.size(): return false
		if q.has_branches():
			if not _validate_branch_snapshot(q, state): return false
			continue
		var expected_step := ""
		for objective in q.objectives:
			var count = state.progress.get(String(objective.id))
			if not (count is float or count is int) or not is_finite(count) or count < 0 or count > objective.required: return false
			if expected_step.is_empty() and count < objective.required:
				expected_step = String(objective.id)
			elif not expected_step.is_empty() and count != 0:
				return false
		if state.step != expected_step or (state.status == "active") != (not expected_step.is_empty()): return false
		if state.status == "ready" and not q.requires_turn_in: return false
	if not value.tracked.is_empty() and status(StringName(value.tracked), value) not in ["active", "ready"]: return false
	return true

func _validate_branch_snapshot(q: QuestDefinition, state: Dictionary) -> bool:
	var path: Variant = state.get("path")
	if not path is Array or path.is_empty() or path[0] != String(q.objectives[0].id) or path.size() > q.objectives.size(): return false
	var by_id := {}
	for objective in q.objectives:
		by_id[String(objective.id)] = objective
		var count: Variant = state.progress.get(String(objective.id))
		if not (count is float or count is int) or not is_finite(count) or count < 0 or count > objective.required: return false
		if not path.has(String(objective.id)) and count != 0: return false
	var seen := {}
	for i in path.size():
		var id: Variant = path[i]
		if not id is String or not by_id.has(id) or seen.has(id): return false
		seen[id] = true
		var objective: QuestObjective = by_id[id]
		if i < path.size() - 1:
			if state.progress[id] != objective.required or not q.objective_targets(objective).has(path[i + 1]): return false
		elif state.status == "active":
			if state.step != id or state.progress[id] >= objective.required: return false
		else:
			if not state.step.is_empty() or state.progress[id] != objective.required or not q.objective_targets(objective).has(""): return false
	if state.status == "ready" and not q.requires_turn_in: return false
	return true

func _whole(value: Variant) -> bool:
	return (value is int or value is float) and is_finite(value) and value >= 0 and value <= 9007199254740991 and float(value) == floorf(float(value))
