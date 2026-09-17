extends SceneTree
var checks := 0
var failures := 0
var notifications := 0

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok: print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func effect(kind: String, key := "", amount := 1.0) -> NarrativeEffect:
	var result := NarrativeEffect.new()
	result.kind = kind
	result.key = StringName(key)
	result.amount = amount
	return result

func available(c: RuntimeContext, id: StringName) -> bool:
	return not c.narrative.available_quest_topics(c.narrative.catalog.npc(id)).is_empty()

func frames(count := 3) -> void:
	for i in count: await process_frame

func _run() -> void:
	_content()
	_conditions()
	await _map()
	print("QUEST_MAP_TESTS: %d/%d passed" % [checks - failures, checks])
	quit(1 if failures else 0)

func _content() -> void:
	var c := RuntimeContext.new()
	var n := c.narrative
	var bruno: NpcDefinition
	for npc in n.catalog.npcs:
		if npc.display_name == "BRUNO": bruno = npc
	check(bruno != null, "authored Bruno is in the city catalog")
	var before := c.snapshot()
	n.checkpoint_requested.connect(func(_urgent): notifications += 1)
	n.changed.connect(func(): notifications += 1)
	for npc in n.catalog.npcs:
		check(available(c, npc.id), "new game exposes an available quest for " + npc.display_name)
	check(c.snapshot() == before and notifications == 0, "query never starts quests, spends money, emits signals or saves")
	check(n.available_quest_topics(n.catalog.npc(&"froggy")).size() == 1, "Froggy offers the imported quest while the first-dose prerequisite blocks his favor")
	n.execute([effect("start_campaign", "", 600), effect("start_quest", "first_dose")])
	check(not available(c, &"maya"), "Maya goes dark while waiting for a dose")
	c.campaign.medicine_doses = 1
	n.refresh_state()
	check(available(c, &"maya"), "Maya lights up when medicine can be delivered")
	var deliver := effect("deliver_medicine")
	deliver.value = 600
	n.execute([deliver], "", "maya")
	check(not available(c, &"maya"), "Maya goes dark after the dose is delivered")
	check(n.available_quest_topics(n.catalog.npc(&"froggy")).size() == 2, "completing first dose unlocks Froggy's second quest topic")
	c.campaign.medicine_doses = 1
	check(available(c, &"maya"), "repeat medicine delivery remains actionable after the first quest")
	n.execute([deliver], "", "maya")
	n.execute([effect("start_quest", "froggy_favor"), effect("start_quest", "fc_depot_runs")])
	check(not available(c, &"froggy"), "shop and small talk alone do not keep a quest hub lit")
	n.record_event("ride_completed", {"destination": "depot", "amount": 1.0})
	n.record_event("ride_completed", {"destination": "depot", "amount": 1.0})
	check(available(c, &"froggy"), "Froggy lights up again for quest turn-in")
	n.execute([effect("turn_in", "froggy_favor"), effect("turn_in", "fc_depot_runs")], "", "froggy")
	check(not available(c, &"froggy"), "completed quests extinguish Froggy's hub")
	var restored := RuntimeContext.new()
	check(restored.restore(c.snapshot()) and not available(restored, &"froggy") and not available(restored, &"maya") and available(restored, bruno.id), "loading restores hub availability entirely from existing story state")
	restored.free()
	c.campaign.flags["__campaign_expired"] = true
	check(not available(c, bruno.id), "expired campaign exposes no available quest hubs")
	c.free()

func _conditions() -> void:
	var c := RuntimeContext.new()
	var n := c.narrative
	# Independent authored fixture: future NPC, branching introduction and a loop.
	var catalog := NarrativeCatalog.new()
	n.catalog = catalog
	catalog.fact_ids = ["introduced"]
	var quest := QuestDefinition.new()
	quest.id = &"future_job"
	var objective := QuestObjective.new()
	objective.id = &"ride"
	quest.objectives = [objective]
	catalog.quests = [quest]
	var npc := NpcDefinition.new()
	npc.id = &"future_npc"
	npc.display_name = "Łucja"
	var topic := NpcTopic.new()
	var dialogue := DialogueDefinition.new()
	dialogue.id = &"future_dialogue"
	dialogue.start_node = &"intro"
	var intro := DialogueNode.new()
	intro.id = &"intro"
	var introduction := DialogueChoice.new()
	introduction.id = &"introduce"
	introduction.effects = [effect("set_fact", "introduced")]
	introduction.next_node = &"offer"
	introduction.once = true
	intro.choices = [introduction]
	var offer := DialogueNode.new()
	offer.id = &"offer"
	var accept := DialogueChoice.new()
	accept.id = &"accept"
	accept.effects = [effect("start_quest", "future_job")]
	var introduced := NarrativeCondition.new()
	introduced.key = &"introduced"
	accept.conditions = [introduced]
	var loop := DialogueChoice.new()
	loop.id = &"loop"
	loop.next_node = &"intro"
	offer.choices = [loop, accept]
	dialogue.nodes = [intro, offer]
	topic.dialogue = dialogue
	npc.topics = [topic]
	catalog.npcs = [npc]
	catalog.dialogues = [dialogue]
	var before := n.snapshot()
	check(available(c, npc.id) and n.snapshot() == before and c.campaign.flags.is_empty(), "future NPC resolves nested effect-gated offers without changing live facts")
	introduction.next_node = &"intro"
	intro.choices.append(accept)
	check(available(c, npc.id) and c.campaign.flags.is_empty(), "a state-changing dialogue loop can unlock an offer on a revisited node")
	intro.choices.erase(accept)
	introduction.next_node = &"offer"
	var blocked := NarrativeCondition.new()
	blocked.kind = "credits"
	blocked.amount = 1000
	topic.conditions = [blocked]
	check(not available(c, npc.id), "unavailable topic suppresses hub even if its offer is valid")
	topic.conditions = []
	quest.conditions = [blocked]
	check(not available(c, npc.id), "quest prerequisites suppress an otherwise visible offer and loops terminate")
	quest.conditions = []
	accept.conditions.append(blocked)
	check(not available(c, npc.id), "disabled choice suppresses a quest regardless of hide-unavailable")
	accept.conditions.erase(blocked)
	n.data.receipts["future_dialogue/intro/introduce"] = true
	check(not available(c, npc.id), "consumed once-only introduction cannot expose an unreachable offer")
	n.data.receipts.clear()
	accept.effects.append(effect("spend", "", 1000))
	check(not available(c, npc.id) and n.snapshot() == before, "failed atomic effects cannot advertise a quest or partially start it")
	c.free()

func _map() -> void:
	var c := RuntimeContext.new()
	c.rides.enabled = true
	root.add_child(c)
	var level: Node3D = load("res://scenes/flight_lab.tscn").instantiate()
	level.living_world_enabled = false
	level.context = c
	root.add_child(level)
	await level.prepare_gameplay()
	c.player.resume(&"application_focus")
	var hud: TaxiHud = level.taxi_hud
	var map := hud.reference_map
	hud.open_overlay("map")
	await frames()
	check(map.visible and paused and map.quest_hubs.size() == 3, "real city map displays all three authored NPC hubs while paused")
	var initials := PackedStringArray()
	for hub in map.quest_hubs:
		initials.append(hub.initial)
		check(map._map.grow(-map.HUB_RADIUS).has_point(map.hub_point(hub)), "badge stays legible inside map bounds: " + hub.name)
	check(initials.has("M") and initials.has("F") and initials.has("B"), "city badges use M, F and B from authored names")
	var maya: NarrativeNpc = level.get_node("Narrative/Maya")
	var marker: Dictionary = map.quest_hubs.filter(func(hub): return hub.id == "maya")[0]
	check(marker.position == maya.global_position, "map marker uses the NPC's actual world position")
	hud.press(map.hub_point(marker), 7)
	check(map.selected_npc == "maya" and map.selected_stop.is_empty() and not c.dialogue.is_active(), "touching a badge inspects its name and topics without beginning a conversation")
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://build/quest-map-active.png")
		for dimensions in [Vector2i(360, 640), Vector2i(960, 540)]:
			root.content_scale_size = dimensions
			root.size = dimensions
			await frames(5)
			var fits := true
			for hub in map.quest_hubs:
				fits = fits and map._map.grow(-map.HUB_RADIUS).has_point(map.hub_point(hub))
			check(fits, "quest badges fit viewport " + str(dimensions))
			await RenderingServer.frame_post_draw
			root.get_texture().get_image().save_png("res://build/quest-map-%dx%d.png" % [dimensions.x, dimensions.y])
		root.content_scale_size = Vector2i(540, 960)
		root.size = Vector2i(540, 960)
		await frames(5)
	c.narrative.execute([effect("start_campaign", "", 600), effect("start_quest", "first_dose")])
	await frames()
	check(map.quest_hubs.size() == 2 and map.selected_npc.is_empty(), "story update extinguishes a selected hub immediately on the paused map")
	var depot: TaxiStop = level.taxi.network.stops.depot
	map.inspect(map._point(depot.global_position))
	check(map.selected_stop == "depot", "taxi stop inspection still works beside quest badges")
	hud.close_overlay()
	c.campaign.medicine_doses = 1
	c.narrative.refresh_state()
	c.player.resume(&"application_focus")
	hud.open_overlay("map")
	await frames()
	check(map.quest_hubs.size() == 3, "reopening map refreshes changed availability")
	maya.hide()
	map.open()
	check(map.quest_hubs.size() == 2, "hidden NPCs do not leak markers onto the map")
	maya.show()
	# A second map/preview in the tree must not contribute its NPC instances.
	var other: Node = load("res://scenes/narrative/city_narrative.tscn").instantiate()
	root.add_child(other)
	map.open()
	check(map.quest_hubs.size() == 3, "only NPCs belonging to the current map appear")
	other.free()
	hud.close_overlay()
	level.queue_free()
	await frames()
	check(not paused, "closing/unloading map releases its pause")
	c.queue_free()
	await frames()
