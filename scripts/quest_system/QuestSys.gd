extends Node
class_name QuestManager        # ← nazwa klasy NIE może być „QuestSys”

## -------------------------------------------------------------------
## 1. Pola eksportowane – listę .tres przeciągasz tu w Inspectorze
## -------------------------------------------------------------------
@export var quests: Array[QuestData] = []

## -------------------------------------------------------------------
## 2. Sygnały – UI będzie ich słuchać
## -------------------------------------------------------------------
signal quest_activated(quest_id: String)
signal quest_completed(quest_id: String)

## -------------------------------------------------------------------
## 3. Prywatny słownik id → QuestData
## -------------------------------------------------------------------
var _dict: Dictionary = {}

func _ready() -> void:
	for q: QuestData in quests:
		_dict[q.id] = q
	_debug_print_all()      # testowo – możesz potem usunąć

## -------------------------------------------------------------------
## 4. Publiczne API
## -------------------------------------------------------------------
func activate(id: String) -> void:
	var q: QuestData = _dict.get(id) as QuestData
	if q and q.status == QuestData.Status.INACTIVE:
		q.status = QuestData.Status.ACTIVE
		quest_activated.emit(id)

func complete(id: String) -> void:
	var q := _dict.get(id) as QuestData
	if not q or q.status == QuestData.Status.DONE:
		return

	q.status = QuestData.Status.DONE

	if q.reward > 0:
		GameState.add_money(q.reward)     # ← wypłata

	quest_completed.emit(id)
	
	if q.next_quest_id != "":
		var nxt: QuestData = _dict.get(q.next_quest_id) as QuestData
		if nxt:
			activate(nxt.id)                      # ► quest ACTIVE
		else:
			push_warning("QuestSys: next_quest_id '" + q.next_quest_id + "' nie istnieje")
			
## ─── Aliasy dla kompatybilności z DialogManager ───

# start_quest() w DialogManager → activate()
func start_quest(id: String) -> void:
	activate(id)

# finish_quest() w DialogManager → complete()
func finish_quest(id: String) -> void:
	complete(id)

# is_active() używane w REQUIREMENTS
func is_active(id: String) -> bool:
	var q := _dict.get(id) as QuestData
	return q != null and q.status == QuestData.Status.ACTIVE



# ----------------------------------------------------------
#  Zwraca WSZYSTKIE misje jako Array[QuestData]
# ----------------------------------------------------------
func get_all() -> Array[QuestData]:
	var arr: Array[QuestData] = []
	for q in _dict.values():
		if q is QuestData:        # dodatkowa asekuracja
			arr.append(q)
	return arr


# ----------------------------------------------------------
#  Zwraca tylko misje o podanym statusie
# ----------------------------------------------------------
func get_by_status(st: QuestData.Status) -> Array[QuestData]:
	var arr: Array[QuestData] = []
	for q in _dict.values():
		if q is QuestData and q.status == st:
			arr.append(q)
	return arr

## -------------------------------------------------------------------
## 5. Pomocniczy wydruk – tylko do testów
## -------------------------------------------------------------------
func _debug_print_all() -> void:
	for q: QuestData in _dict.values():
		print("%s – %s" % [q.id, q.title])
		
		## ------------------  Mały pomocnik dla UI  ------------------
func get_quest(id: String) -> QuestData:
	# Zwraca pojedynczy zasób QuestData albo null, jeśli brak
	return _dict.get(id) as QuestData
	
	## ------------  Kompatybilność ze starym MapOverlay  ------------
# 1) dawny słownik all_quests
var all_quests: Dictionary:
	get:
		return _dict

# 2) słownik aktywnych (wartości = QuestData)
var active_quests: Dictionary:
	get:
		var d: Dictionary = {}
		for q: QuestData in _dict.values():
			if q.status == QuestData.Status.ACTIVE:
				d[q.id] = q
		return d

# 3) dawna funkcja get_quests() – zwraca listę „etapów”
func get_quests() -> Array:
	# Na razie zwracamy sam QuestData, a 'node' ustawiamy na null.
	# MapOverlay sprawdza marker.quest_node – dlatego zwracamy pole dummy.
	var arr: Array = []
	for q: QuestData in _dict.values():
		if q.status == QuestData.Status.ACTIVE:
			arr.append({
				"id": q.id,
				"node": null,      # placeholder – wrócimy do stage-node’ów później
			})
	return arr
