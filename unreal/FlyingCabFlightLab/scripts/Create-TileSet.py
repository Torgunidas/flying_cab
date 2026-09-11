"""Create reusable library assets only; never load or save the user's map."""
import unreal

root = '/Game/Tile_Set/Highway'
path = root + '/BP_HighwayTile'
unreal.EditorAssetLibrary.make_directory(root)
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.FlyingCabHighwayTile)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'BP_HighwayTile', root, unreal.Blueprint, factory)
    assert asset, 'Could not create highway tile'
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset), 'Could not save highway tile'
assert unreal.EditorAssetLibrary.load_blueprint_class(path), 'Tile Blueprint must load'
unreal.log('TILE_SET_READY: ' + path)
