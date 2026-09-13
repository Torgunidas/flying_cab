class_name FrameTelemetry
extends Node
## Bounded, local diagnostics. No network transmission and no per-frame logging.
const CAPACITY := 7200
var context: RuntimeContext
var build_id := "development"
var profile := "desktop"
var enabled := false
var _samples := PackedFloat64Array()
var _cursor := 0
var _count := 0
var _previous := 0
var _spikes: Array[Dictionary] = []
var _focused := true
var _total_frames := 0
var _over_50 := 0
var _over_100 := 0
var _max_ms := 0.0

func _ready() -> void:
	_samples.resize(CAPACITY)
	if FileAccess.file_exists("res://resources/build_info.json"):
		var info = JSON.parse_string(FileAccess.get_file_as_string("res://resources/build_info.json"))
		if info is Dictionary:
			build_id = str(info.get("build_id", build_id))

func begin() -> void:
	_cursor = 0
	_count = 0
	_total_frames = 0
	_over_50 = 0
	_over_100 = 0
	_max_ms = 0
	_spikes.clear()
	_previous = 0
	enabled = true

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		_focused = false
		_previous = 0
	elif what == NOTIFICATION_APPLICATION_FOCUS_IN:
		_focused = true
		_previous = 0

func _process(_dt: float) -> void:
	if not enabled or not _focused or (context and context.player.is_suspended()):
		_previous = 0
		return
	var now := Time.get_ticks_usec()
	if _previous != 0:
		var ms := float(now - _previous) / 1000.0
		_samples[_cursor] = ms
		_cursor = (_cursor + 1) % CAPACITY
		_count = mini(_count + 1, CAPACITY)
		_total_frames += 1
		_over_50 += int(ms > 50.0)
		_over_100 += int(ms > 100.0)
		_max_ms = maxf(_max_ms, ms)
		if ms > 50:
			if _spikes.size() == 128:
				_spikes.pop_front()
			var focus: Node3D = context.player.focus if context else null
			var position := focus.global_position if is_instance_valid(focus) else Vector3.ZERO
			_spikes.append({"frame": _total_frames, "ms": ms, "position": [position.x, position.y, position.z], "draw_calls": Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME), "map": String(context.current_map) if context else ""})
	_previous = now

func report() -> Dictionary:
	var sorted := _samples.slice(0, _count)
	sorted.sort()
	var data := {
		"build_id": build_id, "profile": profile, "engine": Engine.get_version_info().string,
		"os": OS.get_name(), "renderer": RenderingServer.get_current_rendering_method(),
		"window_pixels": [get_window().size.x, get_window().size.y], "scale_3d": get_window().scaling_3d_scale,
		"total_frames": _total_frames, "quantile_window_frames": _count,
		"p50_ms": _percentile(sorted, 0.5), "p95_ms": _percentile(sorted, 0.95), "p99_ms": _percentile(sorted, 0.99),
		"max_ms": _max_ms, "frames_over_50ms": _over_50, "frames_over_100ms": _over_100, "recent_spikes": _spikes.duplicate(true),
	}
	if OS.has_feature("web"):
		data["browser"] = JSON.parse_string(JavaScriptBridge.eval("JSON.stringify({userAgent:navigator.userAgent,dpr:window.devicePixelRatio,canvasWidth:document.querySelector('canvas').width,canvasHeight:document.querySelector('canvas').height,viewportWidth:window.innerWidth,viewportHeight:window.innerHeight})"))
	return data

func export_report() -> void:
	var json := JSON.stringify(report(), "\t")
	if OS.has_feature("web"):
		JavaScriptBridge.download_buffer(json.to_utf8_buffer(), "flying-cab-performance.json", "application/json")
	else:
		var file := FileAccess.open("user://performance.json", FileAccess.WRITE)
		if file:
			file.store_string(json)
	print("PERFORMANCE_REPORT ", json)

func _percentile(values: PackedFloat64Array, fraction: float) -> float:
	return 0.0 if values.is_empty() else values[mini(values.size() - 1, ceili(values.size() * fraction) - 1)]
