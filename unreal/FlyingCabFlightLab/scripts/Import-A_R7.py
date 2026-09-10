"""Run with UE 5.8 -run=pythonscript. Only writes /Game/Vehicles/A_R7."""
from pathlib import Path
import json
import unreal
source = Path(__file__).resolve().parent.parent / 'Build/A_R7'
root = '/Game/Vehicles/A_R7'
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary
materials = {}
for name, tint in json.loads((source/'materials.json').read_text()).items():
    path = root+'/M_A_R7_'+name
    mat = unreal.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset('M_A_R7_'+name,root,unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    paint = name == 'Car'
    color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter if paint else unreal.MaterialExpressionConstant3Vector)
    if paint:
        color.set_editor_property('parameter_name','Color')
        color.set_editor_property('default_value',unreal.LinearColor(*tint))
    else:
        color.set_editor_property('constant',unreal.LinearColor(*tint))
    lib.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant)
    rough.set_editor_property('r', .23 if 'Glass' in name or 'Rims' in name else .6)
    lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(mat)
    assets.save_loaded_asset(mat,False)
    materials[name]=mat
task=unreal.AssetImportTask()
task.set_editor_property('filename',str(source/'SM_A_R7_Supercar.obj'))
task.set_editor_property('destination_path',root)
task.set_editor_property('destination_name','SM_A_R7_Supercar')
task.set_editor_property('automated',True)
task.set_editor_property('replace_existing',True)
task.set_editor_property('save',True)
options=unreal.FbxImportUI()
options.set_editor_property('import_materials',False)
options.set_editor_property('import_textures',False)
options.static_mesh_import_data.set_editor_property('combine_meshes',True)
options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',True)
task.set_editor_property('options',options)
tools.import_asset_tasks([task])
mesh=unreal.load_asset(root+'/SM_A_R7_Supercar')
assert mesh
for i, slot in enumerate(mesh.get_editor_property('static_materials')):
    name=str(slot.get_editor_property('imported_material_slot_name'))
    assert name in materials, name
    mesh.set_material(i,materials[name])
assert assets.save_loaded_asset(mesh,False)
bounds=mesh.get_bounds()
assert 109 < bounds.box_extent.x < 111, bounds
assert 60 < bounds.box_extent.y < 62, bounds
assert 27 < bounds.box_extent.z < 29, bounds
unreal.log('A_R7_IMPORT_VERIFIED '+str(bounds))
