"""Create the 48-triangle cab body. Run with editor -run=pythonscript.

Native Python with --source-only writes the reproducible OBJ/MTL in Build/LowPolyCab.
Editor execution imports only /Game/Vehicles/LowPolyCab; no map or Blueprint edits.
"""
import math
import sys
from pathlib import Path

PROJECT = Path(__file__).resolve().parent.parent
SOURCE = PROJECT / 'Build' / 'LowPolyCab'
ROOT = '/Game/Vehicles/LowPolyCab'
FACES = []


def prism(profile, half_width, side_material, edge_materials):
    left = [(x, -half_width, z) for x, z in profile]
    right = [(x, half_width, z) for x, z in profile]
    FACES.extend([(side_material, left), (side_material, right[::-1])])
    for i in range(len(profile)):
        j = (i + 1) % len(profile)
        FACES.append((edge_materials[i], [left[i], right[i], right[j], left[j]]))


def sources():
    FACES.clear()
    prism([(-110, -35), (98, -35), (110, -24), (110, -6), (52, 0), (-110, 0)],
          45, 'Body', ['Body'] * 6)
    prism([(-78, 0), (40, 0), (3, 35), (-55, 35)],
          34, 'Glass', ['Body', 'Glass', 'Body', 'Glass'])
    for side in [-1, 1]:
        y0, y1 = sorted((side * 22, side * 36))
        FACES.append(('Headlight', [(110.15, y0, -21), (110.15, y1, -21),
                                    (110.15, y1, -9), (110.15, y0, -9)]))
        FACES.append(('Taillight', [(-110.15, y0, -21), (-110.15, y0, -9),
                                    (-110.15, y1, -9), (-110.15, y1, -21)]))
        # Wraparound flat light panels remain legible from the side camera.
        for x0, x1, slot in [(92, 108, 'Headlight'), (-108, -94, 'Taillight')]:
            points = [(x0, side * 45.15, -21), (x0, side * 45.15, -9),
                      (x1, side * 45.15, -9), (x1, side * 45.15, -21)]
            FACES.append((slot, points if side > 0 else points[::-1]))
    lines = ['mtllib LowPolyCab.mtl', 'o LowPolyCab']
    vertex = 1
    triangles = 0
    for normal_id, (slot, points) in enumerate(FACES, 1):
        a, b, c = points[:3]
        u = [b[i] - a[i] for i in range(3)]
        v = [c[i] - a[i] for i in range(3)]
        normal = [u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]]
        length = math.sqrt(sum(n*n for n in normal))
        assert length > 0
        normal = [n/length for n in normal]
        length = math.sqrt(sum(n*n for n in u))
        tangent = [n/length for n in u]
        bitangent = [normal[1]*tangent[2]-normal[2]*tangent[1],
                     normal[2]*tangent[0]-normal[0]*tangent[2],
                     normal[0]*tangent[1]-normal[1]*tangent[0]]
        for p in points:
            lines.append('v ' + ' '.join(f'{n:.6f}' for n in p))
            offset = [p[i]-a[i] for i in range(3)]
            lines.append('vt %.6f %.6f' % (sum(offset[i]*tangent[i] for i in range(3))/220,
                                          sum(offset[i]*bitangent[i] for i in range(3))/220))
        lines.append('vn ' + ' '.join(f'{n:.6f}' for n in normal))
        lines.append('usemtl ' + slot)
        lines.append('f ' + ' '.join(f'{i}/{i}/{normal_id}' for i in range(vertex, vertex+len(points))))
        vertex += len(points)
        triangles += len(points)-2
    SOURCE.mkdir(parents=True, exist_ok=True)
    (SOURCE/'SM_LowPolyCab.obj').write_text('\n'.join(lines)+'\n')
    (SOURCE/'LowPolyCab.mtl').write_text(
        'newmtl Body\nKd 0.85 0.65 0.16\nnewmtl Glass\nKd 0.025 0.055 0.08\n'
        'newmtl Headlight\nKd 1 0.94 0.7\nnewmtl Taillight\nKd 0.8 0.015 0.008\n')
    return triangles


def import_assets(triangles):
    import unreal
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    assets = unreal.EditorAssetLibrary
    lib = unreal.MaterialEditingLibrary
    materials = {}
    for name, tint, roughness, glow in [
        ('Body', (.85, .65, .16), .55, 0),
        ('Glass', (.025, .055, .08), .2, 0),
        ('Headlight', (1, .94, .7), .35, 1.8),
        ('Taillight', (.8, .015, .008), .35, 1.2),
    ]:
        asset_name = 'M_LowPolyCab' + name
        path = ROOT + '/' + asset_name
        mat = unreal.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(
            asset_name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
        lib.delete_all_material_expressions(mat)
        # Only paint responds to the existing vehicle identity/damage Color parameter.
        cls = unreal.MaterialExpressionVectorParameter if name == 'Body' else unreal.MaterialExpressionConstant3Vector
        color = lib.create_material_expression(mat, cls)
        if name == 'Body':
            color.set_editor_property('parameter_name', 'Color')
            color.set_editor_property('default_value', unreal.LinearColor(*tint))
        else:
            color.set_editor_property('constant', unreal.LinearColor(*tint))
        assert lib.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
        rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant)
        rough.set_editor_property('r', roughness)
        assert lib.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        if glow:
            emission = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
            emission.set_editor_property('constant', unreal.LinearColor(*(x*glow for x in tint)))
            assert lib.connect_material_property(emission, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        lib.recompile_material(mat)
        assert assets.save_loaded_asset(mat, False)
        materials[name] = mat
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(SOURCE/'SM_LowPolyCab.obj'))
    task.set_editor_property('destination_path', ROOT)
    task.set_editor_property('destination_name', 'SM_LowPolyCab')
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    options = unreal.FbxImportUI()
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)
    data = options.static_mesh_import_data
    data.set_editor_property('combine_meshes', True)
    data.set_editor_property('generate_lightmap_u_vs', False)
    data.set_editor_property('auto_generate_collision', False)
    data.set_editor_property('convert_scene', False)
    data.set_editor_property('convert_scene_unit', False)
    data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.set_editor_property('options', options)
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(ROOT+'/SM_LowPolyCab')
    assert mesh
    slots = []
    for slot in mesh.get_editor_property('static_materials'):
        key = str(slot.get_editor_property('material_slot_name'))
        assert key in materials, key
        slot.set_editor_property('material_interface', materials[key])
        slots.append(slot)
    assert len(slots) == 4
    mesh.set_editor_property('static_materials', slots)
    for index, slot in enumerate(slots):
        mesh.set_material(index, slot.get_editor_property('material_interface'))
    assert assets.save_loaded_asset(mesh, False)
    unreal.log(f'LOW_POLY_CAB_READY triangles={triangles} bounds={mesh.get_bounds()}')


triangle_count = sources()
if '--source-only' not in sys.argv:
    import_assets(triangle_count)
else:
    print(f'Low-poly cab source: {triangle_count} triangles')
