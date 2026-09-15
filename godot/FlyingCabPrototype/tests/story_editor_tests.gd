extends SceneTree
var checks := 0
var failures := 0
const INPUT := "res://../../tools/quest_editor/examples/depot_runs.narrative.json"
const TEST_ROOT := "res://build/story-editor-test"

func _initialize() -> void:
	call_deferred("_run")

func check(ok: bool, label: String) -> void:
	checks += 1
	if ok: print("PASS: ", label)
	else:
		failures += 1
		push_error("FAIL: " + label)

func context_for(catalog: NarrativeCatalog) -> RuntimeContext:
	var context := RuntimeContext.new()
	context.narrative_catalog = catalog
	context.narrative.catalog = catalog
	return context

func choose(context: RuntimeContext, id: String) -> bool:
	return context.dialogue.choose(StringName(id), context.dialogue.revision)

func talk(context: RuntimeContext, npc := "froggy") -> bool:
	context.dialogue.end()
	return context.dialogue.start(context.narrative.catalog.npc(StringName(npc)), context.narrative, context.player) and choose(context, "topic/fc_depot_runs")

func _run() -> void:
	var payload: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(INPUT))
	var base: NarrativeCatalog = load("res://resources/narrative/city_catalog.tres")
	var original_topics := base.npc(&"froggy").topics.duplicate()
	var importer := FlyingCabStoryImporter.new()
	var catalog := importer.build(payload, base)
	check(catalog != null and importer.errors.is_empty(), "real editor export builds a valid native catalog")
	if catalog == null:
		print(importer.errors)
		quit(1)
		return
	check(base.npc(&"froggy").topics == original_topics, "building import never mutates original NPC resources")
	check(catalog.quests.size() == base.quests.size() + (0 if base.quest(&"fc_depot_runs") else 1), "existing quests survive the merge")
	var context := context_for(catalog)
	check(talk(context) and context.dialogue._node.id == &"offer", "new topic is available on Froggy without manual wiring")
	check(choose(context, "decline") and context.narrative.status(&"fc_depot_runs") == "inactive", "declining does not accept the quest")
	context.narrative.record_event("ride_completed", {"destination": "depot"}, "before-accept")
	check(talk(context) and choose(context, "accept") and context.narrative.status(&"fc_depot_runs") == "active", "accept choice starts the imported quest")
	var objective_id: String = payload.quests[0].objectives[0].id
	check(context.narrative.data.quests.fc_depot_runs.progress[objective_id] == 0, "old rides do not count toward a new objective")
	context.dialogue.end()
	context.narrative.record_event("ride_completed", {"destination": "foundry_torque"}, "wrong-destination")
	check(context.narrative.data.quests.fc_depot_runs.progress[objective_id] == 0, "the selected platform filters actual game events")
	context.narrative.record_event("ride_completed", {"destination": "depot"}, "ride-1")
	check(context.narrative.status(&"fc_depot_runs") == "active", "one ride leaves the objective active")
	check(talk(context) and context.dialogue._node.id == &"active", "return visit selects the active entry")
	context.dialogue.end()
	context.narrative.record_event("ride_completed", {"destination": "depot"}, "ride-2")
	check(context.narrative.status(&"fc_depot_runs") == "ready", "exactly two rides make the quest ready")
	var credits := context.campaign.credits
	check(talk(context) and context.dialogue._node.id == &"ready" and choose(context, "claim") and context.campaign.credits == credits + 75, "reward choice pays exactly 75 CR at Froggy")
	check(talk(context) and context.dialogue._node.id == &"done", "completed entry replaces reward entry")
	context.dialogue.end()
	var saved := context.snapshot()
	var revised := payload.duplicate(true)
	revised.dialogues[0].nodes[0].text = "Poprawiony krótki tekst."
	var revised_catalog := importer.build(revised, catalog)
	check(revised_catalog != null, "text-only reimport needs no objective version bump")
	if revised_catalog == null:
		print(importer.errors)
		quit(1)
		return
	var restored := context_for(revised_catalog)
	check(restored.restore(saved) and restored.narrative.status(&"fc_depot_runs") == "completed", "reimport preserves IDs and completed save state")
	check(talk(restored) and restored.dialogue._node.id == &"done" and not choose(restored, "claim") and restored.campaign.credits == credits + 75, "reimport and reload cannot award a second reward")
	revised.quests[0].objectives[0].required = 3
	check(importer.build(revised, catalog) == null and "\n".join(importer.errors).contains("wersji"), "changing objective meaning requires explicit version bump")
	revised.quests[0].version = 2
	check(importer.build(revised, catalog) != null, "higher authored version allows deliberate objective revision")
	var bad := payload.duplicate(true)
	bad.dialogues[0].nodes[0].choices[0].effects[0].kind = "call_method"
	check(importer.build(bad, base) == null, "native importer rejects unknown executable-style commands")
	bad = payload.duplicate(true)
	bad.quests[0].id = "../escape"
	check(importer.build(bad, base) == null, "IDs cannot escape the authored resource directory")
	var new_npc := payload.duplicate(true)
	for npc in new_npc.npcs:
		if npc.id == "froggy":
			npc.id = "new_dispatcher"
			npc.display_name = "DYSPONENT"
			npc.existing = false
	new_npc.quests[0].turn_in_npc = "new_dispatcher"
	# The author may already have imported the sample into the city catalog.
	# Changing its reward recipient deliberately changes quest semantics.
	if base.quest(&"fc_depot_runs"):
		new_npc.quests[0].version = base.quest(&"fc_depot_runs").version + 1
	var expanded := importer.build(new_npc, base)
	check(expanded != null and expanded.npc(&"new_dispatcher").topics[0].dialogue.id == &"fc_depot_runs_dialogue", "new NPC receives a generated profile with its dialogue already attached")
	check(importer.writes.has("res://resources/narrative/authored/cab_story/npc_new_dispatcher.tres"), "generated NPC profile has a predictable scene-assignable resource path")
	bad = payload.duplicate(true)
	for npc in bad.npcs: npc.existing = false
	check(importer.build(bad, base) == null, "new NPC cannot silently overwrite an unrelated existing profile")
	_disk_roundtrip(payload)
	_branches(payload, base)
	context.free()
	restored.free()
	print("STORY EDITOR: %d/%d passed" % [checks - failures, checks])
	quit(0 if failures == 0 else 1)

func _disk_roundtrip(payload: Dictionary) -> void:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(TEST_ROOT))
	var base := NarrativeCatalog.new()
	for id in ["maya", "froggy"]:
		var npc := NpcDefinition.new()
		npc.id = StringName(id)
		npc.display_name = id.to_upper()
		var path: String = TEST_ROOT + "/" + id + ".tres"
		npc.take_over_path(path)
		ResourceSaver.save(npc, path)
		base.npcs.append(npc)
	var catalog_path := TEST_ROOT + "/catalog.tres"
	ResourceSaver.save(base, catalog_path)
	var importer := FlyingCabStoryImporter.new()
	importer.generated_root = TEST_ROOT + "/generated"
	var result := importer.import_file(INPUT, catalog_path)
	check(result.ok, "native importer writes real typed .tres resources")
	if not result.ok:
		print(result)
		return
	var reloaded := ResourceLoader.load(catalog_path, "", ResourceLoader.CACHE_MODE_REPLACE_DEEP) as NarrativeCatalog
	check(reloaded != null and reloaded.validation_errors().is_empty(), "catalog and nested resources validate after disk reload")
	check(reloaded.npc(&"froggy").topics[0].dialogue == reloaded.dialogues[0], "NPC topic and catalog share the same loaded dialogue resource")
	var stable_uid := ResourceUID.create_id()
	check(ResourceSaver.set_uid(catalog_path, stable_uid) == OK, "fixture assigns a persistent resource UID")
	var before := FileAccess.get_file_as_string(catalog_path)
	result = importer.import_file(INPUT, catalog_path)
	check(result.ok and FileAccess.get_file_as_string(catalog_path) == before, "reimport is idempotent and does not duplicate catalog entries")
	check(FileAccess.get_file_as_string(catalog_path).get_slice("\n", 0).contains(ResourceUID.id_to_text(stable_uid)), "reimport preserves existing resource UID even in headless mode")
	var bad := payload.duplicate(true)
	bad.dialogues[0].nodes[0].choices[0].next_node = "missing"
	var invalid_path := TEST_ROOT + "/invalid.json"
	var file := FileAccess.open(invalid_path, FileAccess.WRITE)
	file.store_string(JSON.stringify(bad))
	file.close()
	result = importer.import_file(invalid_path, catalog_path)
	check(not result.ok and FileAccess.get_file_as_string(catalog_path) == before, "invalid export leaves the previous catalog bytes unchanged")

func _branches(payload: Dictionary, base: NarrativeCatalog) -> void:
	var data := payload.duplicate(true)
	data.quests[0].version += 1
	data.fact_ids.append("fc_route")
	var first: Dictionary = data.quests[0].objectives[0]
	var left: Dictionary = first.duplicate(true)
	var right: Dictionary = first.duplicate(true)
	left.id = "north"
	left.required = 1
	right.id = "south"
	right.required = 1
	var condition := {"kind": "fact", "key": "fc_route", "comparison": ">=", "amount": 1, "status": "active", "invert": false, "reason": "Wybierz północ."}
	first.transitions = [{"target": "north", "conditions": [condition]}, {"target": "south", "conditions": []}]
	left.transitions = [{"target": "", "conditions": []}]
	right.transitions = [{"target": "", "conditions": []}]
	data.quests[0].objectives.append(left)
	data.quests[0].objectives.append(right)
	var importer := FlyingCabStoryImporter.new()
	var catalog := importer.build(data, base)
	check(catalog != null, "conditional objective graph imports")
	if catalog == null: return
	for flag in [0, 1]:
		var context := context_for(catalog)
		context.campaign.flags.fc_route = flag
		talk(context)
		choose(context, "accept")
		context.dialogue.end()
		for i in 2: context.narrative.record_event("ride_completed", {"destination": "depot"}, "branch-" + str(i))
		var target := "north" if flag == 1 else "south"
		check(context.narrative.data.quests.fc_depot_runs.step == target, "fact selects " + target + " objective")
		check(context.narrative.data.quests.fc_depot_runs.progress[target] == 0, "same event never counts for both sides of a transition")
		var restored := context_for(catalog)
		check(restored.restore(JSON.parse_string(JSON.stringify(context.snapshot()))), "branched progress survives JSON save roundtrip")
		restored.narrative.record_event("ride_completed", {"destination": "depot"}, "branch-final")
		check(restored.narrative.status(&"fc_depot_runs") == "ready" and restored.narrative.validate_snapshot(restored.narrative.snapshot()), "selected branch ends without executing the skipped goal")
		check(talk(restored) and choose(restored, "claim") and restored.narrative.validate_snapshot(restored.narrative.snapshot()), "branched reward completion remains a valid save")
		var invalid := restored.narrative.snapshot()
		invalid.quests.fc_depot_runs.path.append("forged")
		check(not restored.narrative.validate_snapshot(invalid), "save rejects forged branch paths")
		context.free()
		restored.free()
	data.quests[0].objectives[1].transitions[0].target = first.id
	check(importer.build(data, base) == null, "native validation rejects objective cycles")
