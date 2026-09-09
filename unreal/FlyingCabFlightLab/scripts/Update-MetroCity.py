"""UE 5.8 editor script: apply the reviewed four-estate city to tracked Unreal assets.

Run with UnrealEditor-Cmd project.uproject -run=pythonscript -script=<this file>.
Explicit authoring operation, not a startup migration. Re-running resets layout/routes
to the compiled metro defaults. PlayerStart, lighting and existing economy are preserved.
"""
import unreal

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert level.load_level('/Game/Maps/FlightLab')
city = unreal.load_asset('/Game/Data/DA_FlyingCabCityLayout')
assert city
defaults = unreal.get_default_object(unreal.FlyingCabCityLayoutAsset)
for key in ['districts', 'neighborhoods', 'standalone_repair_stations',
            'traffic_routes', 'minimap_world_min', 'minimap_world_max']:
    city.set_editor_property(key, defaults.get_editor_property(key))
assert unreal.EditorAssetLibrary.save_loaded_asset(city)

# City getters cache a layout pointer, so the above update precedes route generation.
path = '/Game/Data/DA_FlyingCabLivingWorldProfile'
profile = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if profile is None:
    profile = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'DA_FlyingCabLivingWorldProfile', '/Game/Data',
        unreal.FlyingCabLivingWorldProfile, unreal.DataAssetFactory())
assert profile
profile.set_editor_property('routes', unreal.FlyingCabLivingWorldProfile.build_city_routes())
assert unreal.EditorAssetLibrary.save_loaded_asset(profile)

# Exact inventoried prototype actors, superseded by the data-driven metro geometry.
# This only edits FlightLab.umap; all previous geometry remains recoverable in Git.
superseded = {
    'Arena_Floor', 'Arena_LeftBoundary', 'Arena_RightBoundary', 'Arena_Ceiling',
    'Platform_Low', 'Platform_Mid', 'Platform_High', 'PrecisionGate_Left', 'PrecisionGate_Right',
    'Platform_AshlineMarket', 'Platform_NeonDocks', 'Platform_ZenithSpire', 'Platform_NightshiftRepair',
    'Landmark_AshlineMarket_Tower', 'Landmark_NeonDocks_Mast', 'Landmark_NeonDocks_Crossbeam',
    'Landmark_ZenithSpire_Tower', 'Location_AshlineMarket_Label', 'Location_NeonDocks_Label',
    'Location_ZenithSpire_Label', 'TrafficLane_Lower', 'TrafficLane_Middle', 'TrafficLane_Upper'
}
all_actors = actors.get_all_level_actors()
backdrops = [a for a in all_actors if a.get_actor_label() == 'Arena_Backdrop']
assert len(backdrops) == 1
for actor in all_actors:
    if actor.get_actor_label() in superseded:
        unreal.log('METRO_REPLACED ' + actor.get_actor_label())
        assert actors.destroy_actor(actor)
backdrops[0].set_actor_location(unreal.Vector(5000, -650, 6500), False, False)
backdrops[0].set_actor_scale3d(unreal.Vector(400, 1, 130))
assert level.save_current_level()
unreal.log('METRO_UPDATED: 40000 x 13000, 12 stops, 4 estates, 4 fuel, 2 repair, 40 vehicles, 8 pedestrians')
