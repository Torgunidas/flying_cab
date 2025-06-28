extends CanvasLayer
class_name MapOverlay

@export var toggle_map := "toggle_map"
@export var map_size      := Vector2i(512, 512)
@export var camera_zoom   := Vector2(1, 1)
@export var interior_map_zoom := Vector2(1, 1)

# --- PAN / ZOOM ---
@export var zoom_step      := 1.1
@export var min_zoom       := 0.5
@export var max_zoom       := 3.0
@export var drag_sensitivity := 1.0

var _is_dragging := false
var _drag_start_px : Vector2
var _cam_start_pos : Vector2

var _pinch_start_dist := 0.0
var _pinch_start_zoom := Vector2.ZERO
@export var marker_scale : float = 1.0
var _pinch_active := false
var _touches : Dictionary = {}    # index -> position

# --- SCENES & RESOURCES ---
# const QuestMarkerScene := preload("res://scenes/ui_scenes/QuestMarkers.tscn")
const MARKER_CONFIG := { "player": preload("res://Assets/ui/button.png"),
						"quest_giver": preload("res://Assets/ui/quest_start.png"),
						"quest_goal": preload("res://Assets/ui/quest_obj.png") }

# --- NODES ---
@onready var vp_container    : TextureRect = $MapRoot/MapViewport
@onready var vp              : SubViewport = $MapRoot/MapViewport/MiniVP
@onready var mini_cam        : Camera2D    = $MapRoot/MapViewport/MiniVP/MiniCamera
@onready var player_root     : Control     = $MapRoot/MarkerPlayer
@onready var quest_root      : Control     = $MapRoot/QuestMarkers
@onready var info_panel      : Panel       = $QuestInfoPanel
@onready var info_title      : Label       = $QuestInfoPanel/VBox/TitleLabel
@onready var info_desc       : Label       = $QuestInfoPanel/VBox/DescLabel

var active_goal_marker : Control = null
# --- MARKER STORAGE ---
var player_markers      := {}  # Node2D -> TextureRect
var quest_giver_markers := {}  # Node2D -> TextureRect
var quest_markers       := {}  # quest_id -> QuestMarker
var goal_markers : Dictionary = {}   # Node2D → TextureRect

var current_player_target: Node2D = null
var world_set := false

func _get_pinch_distance() -> float:
	var keys = _touches.keys()
	if keys.size() < 2:
			return 0.0
	return (_touches[keys[0]] as Vector2).distance_to(_touches[keys[1]] as Vector2)


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	print("GameRoot in group gameroot:", get_tree().get_first_node_in_group("gameroot"))
	print("quest_root:", quest_root, "type:", typeof(quest_root))
		# Configure SubViewport & camera
	vp.world_2d = get_viewport().world_2d
	vp.size    = get_viewport().get_visible_rect().size
	mini_cam.make_current()
	mini_cam.zoom = camera_zoom

	# Stretch marker layers fullscreen
	player_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	player_root.offset_left = 0
	player_root.offset_top = 0
	player_root.offset_right = 0
	player_root.offset_bottom = 0

	quest_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	quest_root.offset_left = 0
	quest_root.offset_top = 0
	quest_root.offset_right = 0
	quest_root.offset_bottom = 0

	# Add full-screen map texture
	var tex = TextureRect.new()
	tex.name = "MapTexture"
	tex.texture = vp.get_texture()
	tex.stretch_mode = TextureRect.STRETCH_SCALE
	tex.mouse_filter = Control.MOUSE_FILTER_PASS
	$MapRoot.add_child(tex)
	$MapRoot.move_child(tex, 0)
	tex.set_anchors_preset(Control.PRESET_FULL_RECT)
	tex.offset_left = 0; tex.offset_top = 0; tex.offset_right = 0; tex.offset_bottom = 0

	visible = false
	set_process_input(true)
	set_process(true)
	set_process_unhandled_input(true)

	# Connect level_loaded
	var gr = get_tree().get_first_node_in_group("gameroot")
	if gr:
		gr.connect("level_loaded", Callable(self, "_on_level_loaded"))
		
	QuestSys.quest_activated.connect(func(_id): _build_goal_markers())
	QuestSys.quest_completed.connect(func(_id): _build_goal_markers())

	_build_goal_markers()
	info_panel.visible = false
	
func _build_goal_markers() -> void:
	# 1) usuń stare wykrzykniki
	_clear_goal_markers()

	# 2) przejrzyj wszystkie nody w grupie quest_goal
	for goal in get_tree().get_nodes_in_group("quest_goal"):

		var qid : String = ""

		# ───── A  quest_id w meta-danych ────────────────────────────────
		if goal.has_meta("quest_id"):
			qid = str(goal.get_meta("quest_id"))

		# ───── B  export var quest_id w skrypcie ────────────────────────
		else:
			# sprawdzamy listę właściwości tego noda
			for prop in goal.get_property_list():
				if prop.name == "quest_id":
					qid = str(goal.get("quest_id"))
					break       # już znaleźliśmy, wychodzimy z pętli

		# jeśli nadal brak ID → pomijamy ten węzeł
		if qid == "":
			continue

		# 3) dodaj marker tylko, gdy quest jest ACTIVE
		var q := QuestSys.get_quest(qid)
		if q and q.status == QuestData.Status.ACTIVE:
				_add_goal_marker(goal, qid)
		
func _add_quest_giver_marker(target: Node2D) -> void:
	print("  ▶ _add_quest_giver_marker called for", target)
	print(">> quest_root instance id (in add):", quest_root.get_instance_id())

	if not target or quest_giver_markers.has(target):
		return
		
	var icon = MARKER_CONFIG["quest_giver"]
	var m = TextureRect.new()
	m.texture       = icon
	m.expand_mode   = TextureRect.EXPAND_IGNORE_SIZE
	m.size          = icon.get_size()            # naturalny rozmiar
	m.scale         = Vector2(marker_scale, marker_scale)  # globalna skala
	m.pivot_offset  = Vector2.ZERO
	m.mouse_filter  = Control.MOUSE_FILTER_IGNORE
	m.z_index       = 100

	quest_root.add_child(m)
	# od razu ustal pozycję
	m.position = world_to_map(target.global_position) - (icon.get_size() * marker_scale * 0.5)
	quest_giver_markers[target] = m
	
func _clear_quest_giver_markers() -> void:
	print("quest_root children before adding:", quest_root.get_child_count())
	for m in quest_giver_markers.values():
		m.queue_free()
	quest_giver_markers.clear()
	
# ───────────── GOAL MARKERS (ikonka „!”) ─────────────
func _clear_goal_markers() -> void:
	for m in goal_markers.values():
			m.queue_free()
	goal_markers.clear()
	_hide_goal_info()

func _add_goal_marker(goal_node: Node2D, quest_id: String) -> void:
	if goal_markers.has(goal_node):
			return
	var icon := MARKER_CONFIG["quest_goal"]
	var m := TextureButton.new()
	m.texture_normal = icon
	m.texture_pressed = icon
	m.texture_hover = icon
	m.ignore_texture_size = true
	m.size          = icon.get_size()
	m.scale         = Vector2(marker_scale, marker_scale)
	m.pivot_offset  = Vector2.ZERO
	m.mouse_filter  = Control.MOUSE_FILTER_PASS
	m.z_index       = 100
	m.set_meta("quest_id", quest_id)
	m.connect("mouse_entered", Callable(self, "_on_goal_marker_mouse_entered").bind(m))
	m.connect("mouse_exited", Callable(self, "_on_goal_marker_mouse_exited"))
	m.connect("pressed", Callable(self, "_on_goal_marker_pressed").bind(m))
	quest_root.add_child(m)
	goal_markers[goal_node] = m

func _on_goal_marker_mouse_entered(marker: Control) -> void:
	_show_goal_info(marker)

func _on_goal_marker_mouse_exited() -> void:
	_hide_goal_info()

func _on_goal_marker_pressed(marker: Control) -> void:
	if info_panel.visible and active_goal_marker == marker:
			_hide_goal_info()
	else:
			_show_goal_info(marker)

func _show_goal_info(marker: Control) -> void:
	var qid = marker.get_meta("quest_id", "")
	var q = QuestSys.get_quest(qid)
	if q == null:
			return
	info_title.text = q.title
	info_desc.text = q.description
	active_goal_marker = marker
	info_panel.position = marker.position - Vector2(info_panel.size.x * 0.5 - marker.size.x * marker.scale.x * 0.5, info_panel.size.y + 5)
	info_panel.visible = true

func _hide_goal_info() -> void:
	active_goal_marker = null
	info_panel.visible = false


# --- LEVEL LOADED ---
func _on_level_loaded(level: Node2D, player: Node2D) -> void:
	print("▶ MapOverlay._on_level_loaded fired!  player =", player)

	if not world_set:
		vp.world_2d = get_viewport().world_2d
		world_set = true

	# ─── Ustawiamy aktualny target i podłączamy SYGNAŁY ─────────
	current_player_target = player
	# 1) connect sygnału wsiadania
	if player.has_signal("entered_vehicle"):
		player.connect("entered_vehicle", Callable(self, "_on_player_entered_vehicle"))
	# 2) connect sygnału wysiadania
	if player.has_signal("exited_vehicle"):
		player.connect("exited_vehicle", Callable(self, "_on_player_exited_vehicle"))

	# ─── Center mini-map camera na GRACZU ───────────────────────
	mini_cam.position = player.global_position
	var target_zoom = camera_zoom
	if level.is_in_group("interiors"):
			target_zoom = interior_map_zoom
	mini_cam.zoom = Vector2(clamp(target_zoom.x, min_zoom, max_zoom),
							   clamp(target_zoom.y, min_zoom, max_zoom))

	# ─── Odświeżamy markery: używamy current_player_target ───────
	_clear_player_markers()
	_add_player_marker(current_player_target)

	_clear_quest_giver_markers()
	print("Found quest_giver nodes:", get_tree().get_nodes_in_group("quest_giver"))
	for giver in get_tree().get_nodes_in_group("quest_giver"):
		_add_quest_giver_marker(giver)

	_build_quest_markers()
	call_deferred("_setup_camera_limits", level)

	_clear_goal_markers()
	_build_goal_markers()


# --- PLAYER MARKERS ---
func _add_player_marker(target: Node2D) -> void:
	if not target or player_markers.has(target):
		return
		
	var icon = MARKER_CONFIG["player"]
	var m = TextureRect.new()
	m.texture       = icon
	m.expand_mode   = TextureRect.EXPAND_IGNORE_SIZE
	m.size          = icon.get_size()
	m.scale         = Vector2(marker_scale, marker_scale)
	m.pivot_offset  = Vector2.ZERO
	m.mouse_filter  = Control.MOUSE_FILTER_IGNORE
	m.z_index       = 100

	player_root.add_child(m)
	m.position = world_to_map(target.global_position) - (icon.get_size() * marker_scale * 0.5)
	player_markers[target] = m
	
	print("Added player marker. player_root child count:", player_root.get_child_count())
	print("Last child is:", player_root.get_child(player_root.get_child_count() - 1))

func _clear_player_markers() -> void:
	for m in player_markers.values():
		m.queue_free()
	player_markers.clear()

# --- QUEST MARKERS ---
# --- QUEST STAGE MARKERS ---
func _build_quest_markers() -> void:
	# 1) clear old
	for m in quest_markers.values():
		m.queue_free()
	quest_markers.clear()

	# 2) START-icon dla wszystkich non-active quests
	for qdata in QuestSys.all_quests.values():
		var id = qdata.id
		if QuestSys.active_quests.has(id):
			continue

		# start_target może być nazwa grupy quest_giver, jeśli qdata.start_target=="quest_giver"
		var start_node = get_tree().get_first_node_in_group(qdata.start_target) as Node2D
		if not start_node:
			continue

			# 3) ikona etapu dla aktywnych questów
	#for quest in QuestSys.get_quests():
	#	if quest_markers.has(quest.id):
	#		quest_markers[quest.id].queue_free()
	#		quest_markers.erase(quest.id)
	#	var marker = QuestMarkerScene.instantiate()
	#	marker.quest_id = quest.id
	#	marker.icon     = MARKER_CONFIG["quest_giver"]   # lub ["player"] albo odpowiedni
	#	marker.quest_node = quest.node
	#	quest_root.add_child(marker)

	_update_quest_positions()


func _on_quest_selected(quest_id: int) -> void:
	get_node("/root/QuestUI").open_quest(quest_id)

# --- PROCESS & POSITION UPDATE ---
func _process(_dt: float) -> void:
	if not visible:
		return

	var screen_scale = marker_scale * mini_cam.zoom.x  # mniejsze przy oddaleniu

	# Player markers
	for target in player_markers.keys():
		if not is_instance_valid(target):
			player_markers[target].queue_free()
			player_markers.erase(target)
			continue
		var pm = player_markers[target]
		var base = world_to_map(target.global_position)
		pm.scale    = Vector2(screen_scale, screen_scale)
		var sz = pm.size * pm.scale
		pm.position = base - sz * 0.5

	# Quest giver
	for target in quest_giver_markers.keys():
		if not is_instance_valid(target):
			quest_giver_markers[target].queue_free()
			quest_giver_markers.erase(target)
			continue
		var qm = quest_giver_markers[target]
		var base = world_to_map(target.global_position)
		qm.scale    = Vector2(screen_scale, screen_scale)
		var sz2 = qm.size * qm.scale
		qm.position = base - sz2 * 0.5

	# Quest stage markers
	_update_quest_positions()
	
# Quest-goal (“!”) markers
	for g in goal_markers.keys():
				if not is_instance_valid(g):
						goal_markers[g].queue_free()
						goal_markers.erase(g)
						continue
				var m = goal_markers[g]
				var base = world_to_map(g.global_position)
				var sc   = marker_scale * mini_cam.zoom.x
				m.scale = Vector2(sc, sc)
				m.position = base - (m.size * m.scale) * 0.5

	if info_panel.visible and active_goal_marker:
			info_panel.position = active_goal_marker.position - Vector2(info_panel.size.x * 0.5 - active_goal_marker.size.x * active_goal_marker.scale.x * 0.5, info_panel.size.y + 5)




func _update_quest_positions() -> void:
	var screen_scale = marker_scale * mini_cam.zoom.x
	for marker in quest_markers.values():
		if is_instance_valid(marker.quest_node):
			var base = world_to_map(marker.quest_node.global_position)
			marker.scale = Vector2(screen_scale, screen_scale)
			var ssz = marker.size * marker.scale
			marker.position = base - ssz * 0.5




# --- WORLD TO MAP ---
func world_to_map(p: Vector2) -> Vector2:
	return (p - mini_cam.position) * mini_cam.zoom + vp.size * 0.5

# --- CAMERA LIMITS ---
# --- CAMERA LIMITS ----------------------------------------------------
# MapOverlay.gd  ──────────────────────────────────────────────────────────
func _setup_camera_limits(level: Node) -> void:
	 # ❶ Zbierz granice ze wszystkich TileMapLayer-ów oraz węzłów z grupy "level_bounds"
	var world_rect : Rect2
	var found := false

		# Przechodzimy rekurencyjnie po całym drzewie levelu
	for n in level.get_children(true):
		var rect = null

		if n is TileMapLayer:
					var used : Rect2i = n.get_used_rect()
					if used.size != Vector2i.ZERO:
								var tl_local : Vector2 = n.map_to_local(used.position)
								var br_local : Vector2 = n.map_to_local(used.position + used.size)
								rect = Rect2(tl_local, br_local - tl_local)
								rect.position = n.to_global(rect.position)

		elif n.is_in_group("level_bounds") and n is Node2D:
					if n is Sprite2D:
								rect = n.get_rect()
								rect.position = n.to_global(rect.position)
					elif n.has_method("get_rect"):
								rect = n.call("get_rect")
								rect.position = n.to_global(rect.position)

		if rect:
					world_rect = rect if not found else world_rect.merge(rect)
					found = true

	if not found:
		push_warning("MapOverlay: no level bounds found; camera limits unchanged.")
		return

		# ❷ Ustaw limity mini-kamery
		mini_cam.limit_left   = world_rect.position.x
		mini_cam.limit_top    = world_rect.position.y
		mini_cam.limit_right  = world_rect.position.x + world_rect.size.x
		mini_cam.limit_bottom = world_rect.position.y + world_rect.size.y



# --- INPUT HANDLING (zoom/drag/toggle) --- (zoom/drag/toggle) ---
func _input(event: InputEvent) -> void:
	# otwieranie / zamykanie
	if event.is_action_pressed(toggle_map):
		if visible: _hide_map()
		else: show_map()
		return

	# pozostałe zdarzenia interesują nas tylko gdy mapa jest widoczna
	if not visible:
		return

	# ───────────────────── Z O O M ───────────────────────
	# ───────────────────── Z O O M ───────────────────────
	if event is InputEventScreenTouch:
		if event.pressed:
					_touches[event.index] = event.position
					if _touches.size() == 2:
							_pinch_active = true
							_pinch_start_dist = _get_pinch_distance()
							_pinch_start_zoom = mini_cam.zoom
		else:
					_touches.erase(event.index)
					if _touches.size() < 2:
							_pinch_active = false
		return
	elif event is InputEventScreenDrag:
		_touches[event.index] = event.position
		if _pinch_active and _touches.size() >= 2:
						var dist = _get_pinch_distance()
						if dist != 0.0:
							var new_zoom = _pinch_start_zoom.x * (_pinch_start_dist / dist)
							var z = clamp(new_zoom, min_zoom, max_zoom)
							mini_cam.zoom = Vector2(z, z)
		elif _touches.size() == 1:
					mini_cam.position -= event.relative * mini_cam.zoom
		return

	elif event is InputEventMouseButton:
		if event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_UP:
					_apply_zoom(zoom_step)
		elif event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
					_apply_zoom(1.0 / zoom_step)

		# START / STOP drag-a
		elif event.button_index == MOUSE_BUTTON_LEFT:
			_is_dragging = event.pressed
			if _is_dragging:           # zaczynamy przeciąganie
				_drag_start_px = event.position
				_cam_start_pos  = mini_cam.position
		return                         # my obsłużyliśmy, niech inni już nie muszą

	# ──────────────────── P R Z E C I Ą G A N I E ───────────────────
	if event is InputEventMouseMotion and _is_dragging:
		var delta: Vector2 = event.position - _drag_start_px
		# im mniejszy zoom, tym większy mnożnik  (1 / zoom)
		var factor := drag_sensitivity / mini_cam.zoom.x   # zoom.x == zoom.y
		mini_cam.position = _cam_start_pos - delta * factor
		return

func _unhandled_input(event: InputEvent) -> void:
	# mapa musi być widoczna
	if not visible:
		return
	if event.is_action_pressed("toggle_action"):
		print("[DEBUG] unhandled toggle_action by", self.name)

	# 1. pinch-zoom
	if event is InputEventMagnifyGesture:
		_apply_zoom(1.0 / event.factor)   # factor>1 → przybliżenie

	# 2. drag – jeden lub wiele palców
	elif event is InputEventPanGesture:
		mini_cam.position -= event.delta * mini_cam.zoom

	# 3. mysz: scroll + LPM-drag
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_apply_zoom(zoom_step)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_apply_zoom(1.0/zoom_step)

	if event is InputEventMouseMotion and event.button_mask & MOUSE_BUTTON_MASK_LEFT:
		mini_cam.position -= event.relative * mini_cam.zoom

func show_map():
	visible = true
	mini_cam.position = current_player_target.global_position
	pauza.set_paused(true)
	print("PAUSED =", get_tree().paused)
	print("*** MAP OPEN ***",
		  "  vp.world null? ", vp.world_2d == null,
		  "  children VP:", vp.get_child_count(),
		  "  cam current:", mini_cam.is_current())
	for c in get_tree().get_nodes_in_group("Pickup_Crate"):
		c.visible = false
		
func _hide_map():
	for c in get_tree().get_nodes_in_group("Pickup_Crate"):
			c.visible = true
	pauza.set_paused(false)
	visible = false
	visible = false
	pauza.set_paused(false)
	_hide_goal_info()
	print("tree paused =", get_tree().paused)

func _apply_zoom(mult: float) -> void:
	var z = mini_cam.zoom.x * mult
	z = clamp(z, min_zoom, max_zoom)
	mini_cam.zoom = Vector2(z, z)


# ─────────────────────────────────────────────────────────────────────────
# FUNKCJE OBSŁUGUJĄCE SYGNAŁY z PlayerCharacter.gd
# ─────────────────────────────────────────────────────────────────────────

func _on_player_entered_vehicle(vehicle_node: Node2D) -> void:
	"""
	Kiedy gracz wsiada do pojazdu (PlayerCharacter.emit_signal("entered_vehicle", vehicle)):
	1) usuwamy dotychczasowy marker (gracza),
	2) ustawiamy current_player_target = pojazd,
	3) tworzymy nowy marker na pozycji pojazdu,
	4) przesuwamy kamerę mini-mapki na auto.
	"""
	if not vehicle_node:
		return

	# 1) czyść poprzednie markery
	_clear_player_markers()

	# 2) ustawiamy target na pojazd
	current_player_target = vehicle_node

	# 3) dodaj marker dla pojazdu
	_add_player_marker(current_player_target)

	# 4) kamera mini-mapki za pojazdem
	mini_cam.position = vehicle_node.global_position


func _on_player_exited_vehicle() -> void:
	"""
	Kiedy gracz wysiada z auta (PlayerCharacter.emit_signal("exited_vehicle")):
	1) usuwamy marker auta,
	2) znajdujemy węzeł gracza z grupy "player",
	3) ustawiamy current_player_target = gracz,
	4) tworzymy nowy marker na pozycji gracza,
	5) przestawiamy kamerę mini-mapki na gracza.
	"""
	# 1) usuń istniejący marker (auto)
	_clear_player_markers()

	# 2) znajdź ponownie węzeł gracza
	var player_node = get_tree().get_first_node_in_group("player") as Node2D
	if not player_node:
		return

	# 3) ustawiamy target z powrotem na gracza
	current_player_target = player_node

	# 4) dodaj marker w miejscu, gdzie stoi gracz
	_add_player_marker(current_player_target)

	# 5) kamera mini-mapki wraca na gracza
	mini_cam.position = player_node.global_position
