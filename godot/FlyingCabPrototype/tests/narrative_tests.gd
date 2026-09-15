extends SceneTree
var checks := 0
var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok: print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func effect(kind: String, key := "", amount := 1.0, value := 0.0) -> NarrativeEffect:
	var e := NarrativeEffect.new()
	e.kind = kind
	e.key = StringName(key)
	e.amount = amount
	e.value = value
	return e

func condition(kind: String, key := "", amount := 1.0) -> NarrativeCondition:
	var c := NarrativeCondition.new()
	c.kind = kind
	c.key = StringName(key)
	c.amount = amount
	return c

func choose(c: RuntimeContext, id: String) -> bool:
	return c.dialogue.choose(StringName(id), c.dialogue.revision)

func talk(c: RuntimeContext, npc: String, topic: String) -> bool:
	c.dialogue.end()
	return c.dialogue.start(c.narrative.catalog.npc(StringName(npc)), c.narrative, c.player) and choose(c, "topic/" + topic)

func _run() -> void:
	var c := RuntimeContext.new()
	var n := c.narrative
	check(n.catalog.validation_errors().is_empty(), "sample content validates")
	var before := c.snapshot()
	check(not n.execute([effect("grant_item", "froggy_note"), effect("spend", "", 1000)]).is_empty() and c.snapshot() == before, "failed payment rolls back all earlier effects and receipts")
	check(not n.execute([effect("buy_medicine", "", 1, 1000)]).is_empty() and c.snapshot() == before, "medicine cannot be obtained without payment")
	var empty_reason := condition("credits", "", 1000)
	empty_reason.reason = ""
	check(not n.reason([empty_reason]).is_empty(), "an empty author message cannot bypass a failed condition")
	check(talk(c, "maya", "help") and n.status(&"first_dose") == "inactive" and not c.campaign.active, "opening the offer does not start a quest or timer")
	check(choose(c, "later") and not c.campaign.active, "declining preserves inactive campaign")
	check(talk(c, "maya", "help"), "offer remains available after declining")
	var revision := c.dialogue.revision
	check(choose(c, "accept") and n.status(&"first_dose") == "active" and c.campaign.remaining_seconds == 600, "accept starts the authored quest and clock together")
	check(not c.dialogue.choose(&"accept", revision) and c.campaign.remaining_seconds == 600, "stale double click cannot repeat effects")
	c.dialogue.end()
	check(talk(c, "froggy", "shop"), "ordinary NPC shop topic opens")
	c.campaign.credits = 100
	check(not choose(c, "buy") and c.campaign.medicine_doses == 0 and c.campaign.credits == 100, "choice rechecks money after its button was displayed")
	c.campaign.credits = 400
	check(choose(c, "buy") and c.campaign.credits == 240 and c.campaign.medicine_doses == 1 and n.data.quests.first_dose.step == "deliver", "purchase atomically charges and advances the possession objective")
	check(talk(c, "maya", "medicine") and choose(c, "give") and n.status(&"first_dose") == "completed" and c.campaign.medicine_doses == 0 and c.campaign.remaining_seconds == 1200, "specific delivery choice consumes dose, extends time and completes first quest")
	check(talk(c, "froggy", "shop") and choose(c, "buy") and talk(c, "maya", "medicine") and choose(c, "give") and c.campaign.remaining_seconds == 1800, "repeat visits with a new dose work after first quest completion")
	n.record_event("ride_completed", {}, "old-ride")
	check(talk(c, "froggy", "work") and choose(c, "accept") and n.data.quests.froggy_favor.progress.rides == 0 and c.campaign.flags.promised_froggy == 1, "new side quest does not count old rides and records the chosen promise")
	c.dialogue.end()
	check(n.record_event("ride_completed", {}, "one") and not n.record_event("ride_completed", {}, "one") and n.data.quests.froggy_favor.progress.rides == 1, "duplicate world event counts only once")
	n.record_event("ride_completed", {}, "two")
	var credits := c.campaign.credits
	check(n.status(&"froggy_favor") == "ready" and c.campaign.credits == credits, "turn-in quest waits for reward collection")
	check(not n.execute([effect("turn_in", "froggy_favor")], "", "maya").is_empty() and n.status(&"froggy_favor") == "ready", "wrong NPC cannot accept a turn-in")
	check(talk(c, "froggy", "work") and c.dialogue._node.id == &"ready" and choose(c, "claim") and c.campaign.credits == credits + 60 and n.data.items.froggy_note == 1, "state entry selects ready dialogue and reward includes money plus item")
	c.dialogue.end()
	var restored := RuntimeContext.new()
	check(c.save_to("res://build/narrative-session.json") == OK and restored.load_from("res://build/narrative-session.json") and restored.narrative.snapshot() == n.snapshot(), "schema 5 survives actual JSON disk round trip with progress, facts, inventory and receipts")
	check(not restored.narrative.execute([effect("turn_in", "froggy_favor")], "froggy_job/ready/claim", "froggy").is_empty() and restored.campaign.credits == credits + 60, "reloaded reward cannot be collected again")
	check(talk(restored, "froggy", "work") and restored.dialogue._node.id == &"done", "reloaded conversation selects completed branch")
	restored.dialogue.end()
	var good := c.snapshot()
	var invalid := good.duplicate(true)
	invalid.narrative.quests.first_dose.version = 99
	check(not restored.restore(invalid) and restored.narrative.snapshot() == n.snapshot(), "incompatible authored version rejects whole save without partial restore")
	invalid = good.duplicate(true)
	invalid.narrative.receipts["forged"] = 1
	check(not restored.restore(invalid), "receipt values must be booleans")
	invalid = good.duplicate(true)
	invalid.campaign.flags.promised_froggy = "invalid"
	check(not restored.restore(invalid), "invalid authored fact rejects save before dialogue evaluation")
	for version in [1, 2, 3, 4]:
		var legacy := good.duplicate(true)
		legacy.schema_version = version
		legacy.erase("narrative")
		var old := RuntimeContext.new()
		check(old.restore(JSON.parse_string(JSON.stringify(legacy))) and old.narrative.data.quests.is_empty() and old.campaign.credits == c.campaign.credits, "schema %d JSON migrates with empty narrative and preserved wallet" % version)
		old.free()
	var stocked := RuntimeContext.new()
	stocked.campaign.medicine_doses = 1
	stocked.narrative.execute([effect("start_quest", "first_dose")])
	check(stocked.narrative.data.quests.first_dose.step == "deliver", "already satisfied state is checked on quest activation")
	var snapshot := n.snapshot()
	for i in 30: n.record_event("fuel_purchased", {"amount": 0.1})
	check(n.snapshot() == snapshot, "irrelevant continuous service events do not commit or autosave")
	_adapters_and_atomic_notifications()
	_validate_content()
	_objective_inspector()
	_sequential_events()
	c.free()
	restored.free()
	stocked.free()
	await _world_and_ui()
	await _boot_persistence()
	print("NARRATIVE_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)

func _adapters_and_atomic_notifications() -> void:
	var c := RuntimeContext.new()
	var catalog := NarrativeCatalog.new()
	c.narrative.catalog = catalog
	c.narrative_catalog = catalog
	for event in ["passenger_boarded", "ride_completed", "credits_earned"]:
		var q := QuestDefinition.new()
		q.id = StringName(event)
		var objective := QuestObjective.new()
		objective.id = &"count"
		objective.event = event
		objective.required = 10 if event == "credits_earned" else 1
		if event != "credits_earned":
			objective.origin_id = &"depot"
			objective.destination_id = &"foundry_market"
		q.objectives = [objective]
		catalog.quests.append(q)
		c.narrative.execute([effect("start_quest", event)])
	var vehicle := VehicleState.new()
	vehicle.entity_id = &"adapter_car"
	vehicle.map_id = &"city_02"
	var model: VehicleDefinition = c.vehicle_catalog.find_model(&"basic_cab")
	var offer := c.rides.create_offer("depot", "foundry_market", 25, 1, 60)
	offer.walk = 1.0
	c.rides.select(offer.id)
	check(c.rides.begin_boarding(vehicle, model) and c.rides.board(vehicle, model) and c.narrative.status(&"passenger_boarded") == "completed", "real boarding adapter emits route context to quests")
	c.rides.active.phase = "alighting"
	check(c.rides.complete(vehicle, c.ledger) and c.narrative.status(&"ride_completed") == "completed" and c.narrative.status(&"credits_earned") == "completed", "real fare completion advances both route and income objectives")
	check(not c.rides.complete(vehicle, c.ledger), "repeated fare completion cannot emit duplicate quest progress")
	c.free()
	c = RuntimeContext.new()
	c.campaign.credits = 500
	c.campaign.active = true
	c.campaign.remaining_seconds = 100
	var observations: Array = []
	c.narrative.changed.connect(func(): observations.append([c.campaign.credits, c.campaign.medicine_doses, c.campaign.remaining_seconds]))
	var deliveries: Array = []
	c.campaign.medicine_delivered.connect(func(id, seconds): deliveries.append([id, seconds]))
	check(c.narrative.execute([effect("buy_medicine", "", 2, 160), effect("deliver_medicine", "", 2, 60)], "", "maya").is_empty() and observations == [[180.0, 0, 220.0]], "multi-effect transaction publishes one complete final state")
	check(deliveries.size() == 2 and deliveries[0][0] != deliveries[1][0] and deliveries[0][1] == 60, "buy and deliver in one transaction publishes each delivery with unique ID")
	c.free()

func _validate_content() -> void:
	var catalog := NarrativeCatalog.new()
	var q := QuestDefinition.new()
	q.id = &"test"
	var objective := QuestObjective.new()
	objective.id = &"step"
	q.objectives = [objective]
	catalog.quests = [q]
	check(catalog.validation_errors().is_empty(), "new editor-style resources need no scripted quest implementation")
	catalog.quests.append(q)
	check(not catalog.validation_errors().is_empty(), "validator rejects duplicate quest ID")
	catalog.quests.pop_back()
	q.prerequisites = ["test"]
	check(not catalog.validation_errors().is_empty(), "validator rejects prerequisite cycle")
	q.prerequisites = []
	objective.event = "misspelled"
	check(not catalog.validation_errors().is_empty(), "validator rejects unknown event names")
	objective.event = "ride_completed"
	q.conditions = [condition("fact", "unknown")]
	check(not catalog.validation_errors().is_empty(), "validator rejects unregistered condition fact")
	q.conditions.clear()
	q.rewards = [effect("start_quest", "test")]
	check(not catalog.validation_errors().is_empty(), "validator rejects unsupported reward cascades")
	q.rewards.clear()
	var d := DialogueDefinition.new()
	d.id = &"test_dialogue"
	d.start_node = &"hello"
	var node := DialogueNode.new()
	node.id = &"hello"
	node.text = "Hello"
	var choice := DialogueChoice.new()
	choice.id = &"answer"
	choice.next_node = &"missing"
	node.choices = [choice]
	d.nodes = [node]
	catalog.dialogues = [d]
	check(not catalog.validation_errors().is_empty(), "validator rejects missing dialogue destination")
	choice.next_node = &""
	choice.id = &"__close"
	check(not catalog.validation_errors().is_empty(), "reserved control IDs cannot shadow authored choices")

func _objective_inspector() -> void:
	var objective := QuestObjective.new()
	objective.id = &"rides"
	# Exercise Godot's real Range snapping using the resource's Inspector metadata.
	var field := SpinBox.new()
	var has_range := false
	for property in objective.get_property_list():
		if property.name == "required" and property.hint == PROPERTY_HINT_RANGE:
			var bounds: PackedStringArray = property.hint_string.split(",")
			field.min_value = float(bounds[0])
			field.max_value = float(bounds[1])
			field.step = float(bounds[2])
			has_range = true
	check(has_range, "objective exposes a numeric Inspector range")
	var preserves_input := true
	for value in [0.001, 0.5, 1.0, 2.0]:
		field.value = value
		preserves_input = preserves_input and is_equal_approx(field.value, value)
	check(preserves_input, "Inspector preserves whole event counts and fractional service amounts")
	objective.required = field.value
	field.free()
	var c := RuntimeContext.new()
	var catalog := NarrativeCatalog.new()
	var q := QuestDefinition.new()
	q.id = &"inspector_rides"
	q.objectives = [objective]
	catalog.quests = [q]
	c.narrative.catalog = catalog
	c.narrative_catalog = catalog
	c.narrative.execute([effect("start_quest", "inspector_rides")])
	c.narrative.record_event("ride_completed")
	check(c.narrative.status(q.id) == "active", "Inspector-authored two-ride quest stays active after one ride")
	c.narrative.record_event("ride_completed")
	check(c.narrative.status(q.id) == "completed", "Inspector-authored two-ride quest completes after exactly two rides")
	c.free()

func _sequential_events() -> void:
	var c := RuntimeContext.new()
	var catalog := NarrativeCatalog.new()
	c.narrative.catalog = catalog
	c.narrative_catalog = catalog
	var q := QuestDefinition.new()
	q.id = &"two_steps"
	for id in ["first", "second"]:
		var objective := QuestObjective.new()
		objective.id = StringName(id)
		objective.event = "vehicle_entered"
		objective.vehicle_id = &"the_car"
		q.objectives.append(objective)
	catalog.quests = [q]
	c.narrative.execute([effect("start_quest", "two_steps")])
	c.narrative.record_event("vehicle_entered", {"vehicle": "wrong_car"})
	check(c.narrative.data.quests.two_steps.step == "first", "wrong vehicle does not advance specific-vehicle objective")
	c.narrative.record_event("vehicle_entered", {"vehicle": "the_car"})
	check(c.narrative.data.quests.two_steps.step == "second", "one event cannot complete two consecutive objectives")
	c.narrative.record_event("vehicle_entered", {"vehicle": "the_car"})
	check(c.narrative.status(&"two_steps") == "completed", "second event completes the sequential quest")
	c.free()

func frames(count: int) -> void:
	for i in count: await physics_frame
	await process_frame

func _world_and_ui() -> void:
	var c := RuntimeContext.new()
	c.rides.enabled = true
	root.add_child(c)
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	level.context = c
	root.add_child(level)
	current_scene = level
	await level.prepare_gameplay()
	await frames(50)
	check(level.on_foot.try_exit(level.cab), "real world permits leaving cab before conversation")
	var maya: NarrativeNpc = level.get_node("Narrative/Maya")
	var froggy: NarrativeNpc = level.get_node("Narrative/Froggy")
	var actor: WalkingActor = level.on_foot.actor
	actor.place(maya.global_position + Vector3(0.8, HumanRig.HEIGHT * 0.5 + 0.05, 0))
	await frames(35)
	level.on_foot.refresh_target()
	check(level.on_foot.conversation_target == maya and level.controls.interaction_label == "ROZMOWA / Q", "nearby NPC is available through contextual on-foot control")
	var presentation: NarrativePanel = level.narrative_panel
	check(maya.interact(c, actor) and paused and c.player.is_suspended() and not level.controls.visible, "dialogue pauses world, clears movement and hides gameplay controls")
	var rev := c.dialogue.revision
	presentation._choose(0, rev)
	check(c.dialogue.revision == rev, "invisible choice cannot accept input")
	presentation.skip()
	check(c.dialogue.revision == rev and presentation.options[0].modulate.a == 1, "skip reveals answers without choosing")
	presentation._choose(0, rev)
	check(c.dialogue.definition != null and c.dialogue.definition.id == &"maya_help", "revealed UI choice enters authored dialogue")
	c.player.suspend(&"test_lock")
	presentation.close()
	check(not paused and c.player.is_suspended() and level.controls.visible, "closing restores presentation while preserving other system locks")
	c.player.resume(&"test_lock")
	actor.place(froggy.global_position + Vector3(0.8, HumanRig.HEIGHT * 0.5 + 0.05, 0))
	await frames(20)
	check(froggy.interact(c, actor), "another NPC can begin a fresh conversation")
	maya.queue_free()
	await process_frame
	check(c.dialogue.is_active() and c.dialogue.profile == froggy.profile, "removing a previous speaker does not close another NPC conversation")
	froggy.queue_free()
	await process_frame
	check(not c.dialogue.is_active() and not paused and not c.player.is_suspended(), "removing current speaker closes UI and releases locks")
	presentation.open_journal()
	check(paused and presentation._mode == "journal", "journal is an independent paused view")
	presentation.close()
	c.campaign.active = true
	c.campaign.remaining_seconds = 0.01
	c.campaign.advance(1)
	check(presentation._mode == "game_over" and paused and c.campaign.flags.get("__campaign_expired", false), "campaign expiration displays end state and persists terminal fact")
	level.queue_free()
	await process_frame
	c.queue_free()
	await process_frame

func _boot_persistence() -> void:
	var save_path := "res://build/narrative-boot-fixture.json"
	var fixture := RuntimeContext.new()
	fixture.rides.enabled = true
	fixture.campaign.credits = 120
	fixture.save_to(save_path)
	fixture.free()
	var game: Node = load("res://scenes/game.tscn").instantiate()
	game.save_path = save_path
	root.add_child(game)
	current_scene = game
	game.continue_game()
	await process_frame
	var deadline := Time.get_ticks_msec() + 15000
	while (game.maps.busy or game.maps.current == null) and Time.get_ticks_msec() < deadline:
		await process_frame
	check(not game.maps.busy and game.maps.current != null, "actual composition root starts a saved narrative session")
	var c: RuntimeContext = game.context
	check(talk(c, "maya", "help") and choose(c, "accept") and paused, "dialogue effect executes while real game is paused")
	for i in 6: await process_frame
	var disk = JSON.parse_string(FileAccess.get_file_as_string(save_path))
	check(disk.narrative.quests.get("first_dose", {}).get("status") == "active" and disk.campaign.remaining_seconds == 600, "committed dialogue effects reach disk before conversation closes")
	c.dialogue.end()
	c.campaign.advance(10000)
	for i in 6: await process_frame
	disk = JSON.parse_string(FileAccess.get_file_as_string(save_path))
	check(disk.campaign.flags.get("__campaign_expired", false), "terminal campaign state is checkpointed during end screen")
	game.queue_free()
	await process_frame
	game = load("res://scenes/game.tscn").instantiate()
	game.save_path = save_path
	root.add_child(game)
	current_scene = game
	game.continue_game()
	await process_frame
	deadline = Time.get_ticks_msec() + 15000
	while (game.maps.busy or game.maps.current == null) and Time.get_ticks_msec() < deadline:
		await process_frame
	check(not game.maps.busy and game.maps.current.narrative_panel._mode == "game_over" and paused, "saved campaign end restores without restarting time or blocking map preparation")
	game.queue_free()
	await process_frame
	var bad_path := "res://build/narrative-invalid-fixture.json"
	var file := FileAccess.open(bad_path, FileAccess.WRITE)
	file.store_string("{invalid save}")
	file.close()
	game = load("res://scenes/game.tscn").instantiate()
	game.save_path = bad_path
	root.add_child(game)
	current_scene = game
	game.continue_game()
	for i in 6: await process_frame
	check(game.maps.current == null and game._overlay.visible and FileAccess.get_file_as_string(bad_path) == "{invalid save}", "invalid save displays a recovery choice without overwriting the original")
	game.queue_free()
	await process_frame
