"""One-time conversion of FlightLab to a saved, editable level (UE editor Python).

Run via UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Refuses an already converted map; it never regenerates over the designer's edits.
"""
import unreal

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert level.load_level('/Game/Maps/FlightLab')
assert not any(isinstance(a, unreal.FlyingCabAuthoredWorld) for a in actors.get_all_level_actors()), 'Map is already editable; conversion refused.'
assert unreal.FlyingCabWorldAuthoring.bake_current_world(), 'World conversion failed; map was not saved.'
all_actors = actors.get_all_level_actors()
assert sum(isinstance(a, unreal.FlyingCabDistrictAnchor) for a in all_actors) == 24
assert sum(isinstance(a, unreal.FlyingCabFuelStation) for a in all_actors) == 4
assert sum(isinstance(a, unreal.FlyingCabRepairStation) for a in all_actors) == 2
assert sum(isinstance(a, unreal.FlyingCabQuestGiver) for a in all_actors) >= 2
assert level.save_current_level()
unreal.log('EDITABLE_WORLD_SAVED: {} actors, 24 taxi stops, 6 services; no runtime geometry generation.'.format(len(all_actors)))
