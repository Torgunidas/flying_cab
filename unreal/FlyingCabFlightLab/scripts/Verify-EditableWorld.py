"""Editor save/reload regression test on a disposable copy; never saves FlightLab."""
import unreal
from pathlib import Path

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
source = '/Game/Maps/FlightLab'
probe = '/Game/Tests/EditableWorldPersistenceProbe'
assert not unreal.EditorAssetLibrary.does_asset_exist(probe), 'An existing probe must be reviewed before retrying.'
assert unreal.EditorAssetLibrary.duplicate_asset(source, probe)
assert level.load_level(probe)
all_actors = actors.get_all_level_actors()
count = len(all_actors)
assert count >= 500, 'Geometry must exist before Play.'
assert not unreal.FlyingCabWorldAuthoring.bake_current_world(), 'A second conversion must be refused.'
assert len(actors.get_all_level_actors()) == count
stop = next(a for a in all_actors if isinstance(a, unreal.FlyingCabDistrictAnchor) and a.get_actor_label() == 'STOP - ASHLINE MARKET')
fuel = next(a for a in all_actors if isinstance(a, unreal.FlyingCabFuelStation) and a.get_attach_parent_actor() == stop)
tower = next(a for a in all_actors if a.get_actor_label() == 'ResidentialTowerAM')
old_stop, old_fuel, old_tower = stop.get_actor_location(), fuel.get_actor_location(), tower.get_actor_location()
delta = unreal.Vector(275, 0, 125)
stop.set_actor_location(old_stop + delta, False, False)
assert (fuel.get_actor_location() - old_fuel - delta).length() < 0.1
tower.set_actor_location(tower.get_actor_location() + unreal.Vector(75, 0, 0), False, False)
tower.set_editor_property('color', unreal.LinearColor(0.2, 0.3, 0.4, 1.0))
assert level.save_current_level()
# Unload the edited world before reading it again from disk.
assert level.load_level(source)
assert level.load_level(probe)
loaded = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
assert (loaded['STOP - ASHLINE MARKET'].get_actor_location() - old_stop - delta).length() < 0.1
assert (loaded['ASHLINE CHARGE'].get_actor_location() - old_fuel - delta).length() < 0.1
assert (loaded['ResidentialTowerAM'].get_actor_location() - old_tower - delta - unreal.Vector(75, 0, 0)).length() < 0.1
assert loaded['ResidentialTowerAM'].get_attach_parent_actor() == loaded['STOP - ASHLINE MARKET']
assert loaded['ResidentialTowerAM'].get_component_by_class(unreal.StaticMeshComponent).get_material(0) is not None
assert abs(loaded['ResidentialTowerAM'].get_editor_property('color').r - 0.2) < 0.001
assert loaded['Office entrance'].get_editor_property('destination_actor') is not None
assert level.load_level(source)
assert unreal.EditorAssetLibrary.delete_asset(probe)
# Some commandlet versions leave the unloaded map file after reporting successful deletion.
# This exact path was created above, with an initial non-existence assertion.
probe_file = Path(unreal.Paths.project_content_dir()) / 'Tests' / 'EditableWorldPersistenceProbe.umap'
if probe_file.exists():
    probe_file.unlink()
unreal.log('EDITABLE_WORLD_PERSISTENCE_PASS: before-Play geometry, move parent + child, services, save/reload, portal references, regeneration guard.')
