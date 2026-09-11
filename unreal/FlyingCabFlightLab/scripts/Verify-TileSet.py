"""Verify placement, duplication and persistence on a disposable map, never FlightLab."""
from pathlib import Path
import unreal

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
probe = '/Game/Tests/HighwayTilePersistenceProbe'
assert not unreal.EditorAssetLibrary.does_asset_exist(probe)
assert level.new_level(probe)
cls = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Tile_Set/Highway/BP_HighwayTile')
tile = actors.spawn_actor_from_class(cls, unreal.Vector(80000, 0, 80000))
tile.set_actor_label('Tile original')
tile.set_editor_property('length', 4200.0)
tile.set_editor_property('width', 1100.0)
tile.set_editor_property('speed_multiplier', 1.7)
tile.set_editor_property('fuel_consumption_multiplier', 0.4)
tile.set_actor_scale3d(unreal.Vector(2, 1, 0.5))
tile.set_actor_rotation(unreal.Rotator(90, 0, 0), False)
copy = actors.duplicate_actor(tile, offset=unreal.Vector(10000, 0, 0))
assert copy
copy.set_actor_label('Tile duplicate')
assert level.save_current_level()
assert level.load_level('/Game/Maps/FlightLab')
assert level.load_level(probe)
tiles = {a.get_actor_label(): a for a in actors.get_all_level_actors() if isinstance(a, unreal.FlyingCabHighwayTile)}
assert len(tiles) == 2
for name in ('Tile original', 'Tile duplicate'):
    t = tiles[name]
    assert abs(t.get_editor_property('speed_multiplier') - 1.7) < 0.001
    assert abs(t.get_editor_property('fuel_consumption_multiplier') - 0.4) < 0.001
    assert t.get_editor_property('length') == 4200.0
    zone = t.get_editor_property('zone')
    assert (zone.get_unscaled_box_extent() - unreal.Vector(2100, 200, 550)).length() < 0.1
    lane = t.get_editor_property('lane')
    assert abs(lane.get_editor_property('relative_location').y + 520) < 0.1
    assert t.get_editor_property('markings').get_instance_count() == 6
assert abs(tiles['Tile duplicate'].get_actor_location().x - tiles['Tile original'].get_actor_location().x - 10000) < 0.1
assert level.load_level('/Game/Maps/FlightLab')
assert unreal.EditorAssetLibrary.delete_asset(probe)
probe_file = Path(unreal.Paths.project_content_dir()) / 'Tests' / 'HighwayTilePersistenceProbe.umap'
if probe_file.exists():
    probe_file.unlink()
unreal.log('TILE_SET_PERSISTENCE_PASS')
