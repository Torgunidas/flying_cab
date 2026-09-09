"""Author the vector-nozzle meshes and materials (UE 5.8 editor Python).

Run explicitly with -run=pythonscript -script=<this file>. Only /Game/Effects/Thrusters
is authored. Source geometry and shader code are reproducible here; no startup writes.
"""
from pathlib import Path
import math
import unreal

ROOT = '/Game/Effects/Thrusters'
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def material(name, translucent=False):
    path = ROOT + '/' + name
    mat = unreal.load_asset(path) if ASSETS.does_asset_exist(path) else TOOLS.create_asset(
        name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    LIB.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided', True)
    if translucent:
        mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    return mat


def node(mat, cls, **properties):
    result = LIB.create_material_expression(mat, cls)
    for key, value in properties.items():
        result.set_editor_property(key, value)
    return result


def scalar(mat, name, value):
    return node(mat, unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=value)


def connect(src, dst, pin, output=''):
    assert LIB.connect_material_expressions(src, output, dst, pin), pin


def output(src, prop, channel=''):
    assert LIB.connect_material_property(src, channel, prop)


def finish(mat):
    LIB.layout_material_expressions(mat)
    LIB.recompile_material(mat)
    assert ASSETS.save_loaded_asset(mat)


def custom(mat, code, inputs, kind=unreal.CustomMaterialOutputType.CMOT_FLOAT4):
    pins = []
    for key in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', key)
        pins.append(pin)
    expr = node(mat, unreal.MaterialExpressionCustom, code=code, output_type=kind,
                inputs=pins)
    for key, value in inputs.items():
        connect(value, expr, key)
    return expr


def mask(mat, src, rgb=False, alpha=False):
    result = node(mat, unreal.MaterialExpressionComponentMask, r=rgb, g=rgb, b=rgb, a=alpha)
    connect(src, result, '')
    return result


# Advected multi-scale turbulence, not a static cone or a string of shock diamonds.
PLUME = r'''
float t = Clock * (3.8 + Power * 3.0) + Seed * 7.13;
float u = saturate(UV.x);
float y = (UV.y - 0.5) * 2.0;
float wobble = (sin(u*21.0-t*2.3) + 0.45*sin(u*49.0-t*3.7+Seed))*0.055*u;
float width = 0.22 + 0.43*u;
float edgeNoise = sin(u*45.0-t*4.0+y*9.0)*0.07 + sin(u*79.0-t*5.7-y*17.0)*0.035;
float radius = abs(y-wobble) / max(0.05, width+edgeNoise*u);
float envelope = pow(saturate(1.0-radius*radius), 2.2);
float axial = pow(saturate(1.0-u), 1.9) * smoothstep(0.0,0.018,u);
float eddies = 0.68+0.20*sin(u*53.0-t*5.0+y*12.0)+0.12*sin(u*107.0-t*7.3-y*23.0);
float core = exp(-abs(y-wobble)*17.0) * exp(-u*9.0);
float alpha = saturate((envelope*axial*eddies*0.38+core*0.23)*Power);
float3 gas = lerp(float3(0.23,0.38,0.46),float3(0.44,0.31,0.19),u);
float3 color = gas*(0.6+Power*0.65)+float3(1.7,0.70,0.20)*core*Power;
return float4(color,alpha);
'''
mat = material('M_ThrustPlume', True)
uv = node(mat, unreal.MaterialExpressionTextureCoordinate)
clock = scalar(mat, 'Clock', 0)
power = scalar(mat, 'Power', 0)
seed = scalar(mat, 'Seed', 0)
plume = custom(mat, PLUME, dict(UV=uv, Clock=clock, Power=power, Seed=seed))
output(mask(mat, plume, rgb=True), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
opacity = mask(mat, plume, alpha=True)
fade = node(mat, unreal.MaterialExpressionDepthFade, fade_distance_default=18.0)
connect(opacity, fade, 'Opacity')
output(fade, unreal.MaterialProperty.MP_OPACITY)
finish(mat)

# Pixel normal offset leaves the quiet edges of a flat card undistorted.
mat = material('M_ThrustHeat', True)
mat.set_editor_property('refraction_method', unreal.RefractionMode.RM_PIXEL_NORMAL_OFFSET)
uv = node(mat, unreal.MaterialExpressionTextureCoordinate)
clock = scalar(mat, 'Clock', 0)
power = scalar(mat, 'Power', 0)
seed = scalar(mat, 'Seed', 0)
normal = custom(mat, r'''
float u=saturate(UV.x), y=(UV.y-0.5)*2.0, t=Clock*5.0+Seed*11.0;
float envelope=pow(saturate(1.0-abs(y)/(0.24+u*0.65)),2.0)*sin(u*3.14159265);
float a=sin(u*48.0-t*3.2+y*13.0)+0.4*sin(u*97.0-t*5.1-y*29.0);
float b=cos(u*31.0-t*2.8+y*21.0);
return normalize(float3(a*0.14*envelope*Power,b*0.12*envelope*Power,1.0));
''', dict(UV=uv, Clock=clock, Power=power, Seed=seed), unreal.CustomMaterialOutputType.CMOT_FLOAT3)
output(normal, unreal.MaterialProperty.MP_NORMAL)
output(node(mat, unreal.MaterialExpressionConstant, r=1.045), unreal.MaterialProperty.MP_REFRACTION)
output(node(mat, unreal.MaterialExpressionConstant, r=0.0), unreal.MaterialProperty.MP_OPACITY)
output(node(mat, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0, 0, 0)),
       unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(mat)

mat = material('M_ThrustDust', True)
uv = node(mat, unreal.MaterialExpressionTextureCoordinate)
alpha = scalar(mat, 'Alpha', 0)
seed = scalar(mat, 'Seed', 0)
color = node(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Tint',
             default_value=unreal.LinearColor(0.24,0.22,0.19,1))
opacity = custom(mat, r'''
float2 p=(UV-0.5)*2.0;
float r=dot(p,p);
float swirl=0.7+0.2*sin(p.x*13.0+p.y*9.0+Seed*17.0)+0.1*cos(p.y*23.0-p.x*7.0+Seed);
return pow(saturate(1.0-r),2.5)*swirl*Alpha;
''', dict(UV=uv, Alpha=alpha, Seed=seed), unreal.CustomMaterialOutputType.CMOT_FLOAT1)
fade = node(mat, unreal.MaterialExpressionDepthFade, fade_distance_default=12.0)
connect(opacity, fade, 'Opacity')
output(fade, unreal.MaterialProperty.MP_OPACITY)
output(color, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(mat)

for name, tint, metal, rough in [
    ('M_NozzleMetal',(0.17,0.20,0.23),0.78,0.32),
    ('M_NozzleCeramic',(0.022,0.028,0.033),0.15,0.72),
    ('M_NozzleRim',(0.34,0.26,0.16),0.72,0.40),
]:
    mat = material(name)
    output(node(mat, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(*tint)),
           unreal.MaterialProperty.MP_BASE_COLOR)
    output(node(mat, unreal.MaterialExpressionConstant, r=metal), unreal.MaterialProperty.MP_METALLIC)
    output(node(mat, unreal.MaterialExpressionConstant, r=rough), unreal.MaterialProperty.MP_ROUGHNESS)
    finish(mat)

mat = material('M_NozzleGlow')
output(node(mat, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0.07,0.025,0.008)),
       unreal.MaterialProperty.MP_BASE_COLOR)
glow = custom(mat, 'return float3(5.0,1.1,0.12)*Heat*Heat;',
              dict(Heat=scalar(mat, 'Heat', 0)), unreal.CustomMaterialOutputType.CMOT_FLOAT3)
output(glow, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(mat)

# OBJ authoring is kept in project Build, using centimetres and +X as the exhaust axis.
source = Path(unreal.Paths.project_dir())/'Build'/'Thrusters'
source.mkdir(parents=True, exist_ok=True)
vertices, faces = [], []
sections = [(0,12),(6,16),(13,16),(29,20),(34,20),(34,16.5),(27,16.2),(10,10.0)]
segments = 32
for x, radius in sections:
    for i in range(segments):
        theta=2*math.pi*i/segments
        vertices.append((x,radius*math.cos(theta),radius*math.sin(theta)))
for ring in range(len(sections)-1):
    slot='Metal' if ring<3 else ('Rim' if ring<5 else 'Ceramic')
    for i in range(segments):
        a=ring*segments+i+1
        b=ring*segments+(i+1)%segments+1
        c=b+segments
        d=a+segments
        faces.append((slot,(a,b,c,d)))
(source/'Thruster.mtl').write_text(
    'newmtl Metal\nKd 0.17 0.20 0.23\nnewmtl Rim\nKd 0.34 0.26 0.16\n'
    'newmtl Ceramic\nKd 0.022 0.028 0.033\n')
obj=['mtllib Thruster.mtl','o VectorNozzle']+[f'v {x:.7f} {y:.7f} {z:.7f}' for x,y,z in vertices]
obj += [f'vt {ring/(len(sections)-1):.7f} {i/segments:.7f}'
        for ring in range(len(sections)) for i in range(segments)]
for slot, ids in faces:
    obj += ['usemtl '+slot,'f '+' '.join(f'{i}/{i}' for i in ids)]
(source/'SM_VectorNozzle.obj').write_text('\n'.join(obj)+'\n')
# A unit card has its origin at the nozzle mouth, avoiding texture-coordinate guesses.
(source/'SM_ThrustCard.obj').write_text(
    'o ThrustCard\nv 0 -0.5 0\nv 1 -0.5 0\nv 1 0.5 0\nv 0 0.5 0\n'
    'vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 1\n'
    'f 1/1/1 2/2/1 3/3/1\nf 1/1/1 3/3/1 4/4/1\n')
for name in ['SM_VectorNozzle','SM_ThrustCard']:
    task = unreal.AssetImportTask()
    task.set_editor_property('filename',str(source/(name+'.obj')))
    task.set_editor_property('destination_path',ROOT)
    task.set_editor_property('destination_name',name)
    task.set_editor_property('automated',True)
    task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',True)
    options=unreal.FbxImportUI()
    options.set_editor_property('import_materials',False)
    options.set_editor_property('import_textures',False)
    options.static_mesh_import_data.set_editor_property('combine_meshes',True)
    options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False)
    options.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
    options.static_mesh_import_data.set_editor_property('convert_scene',False)
    options.static_mesh_import_data.set_editor_property('convert_scene_unit',False)
    task.set_editor_property('options',options)
    TOOLS.import_asset_tasks([task])
    mesh=unreal.load_asset(ROOT+'/'+name)
    assert mesh, name
    if name=='SM_VectorNozzle':
        slots=mesh.get_editor_property('static_materials')
        unreal.log('THRUST_SLOTS '+str(slots))
        if not slots:
            slot=unreal.StaticMaterial()
            slot.set_editor_property('material_slot_name','Metal')
            slot.set_editor_property('imported_material_slot_name','Metal')
            slots=[slot]
        updated_slots=[]
        for slot in slots:
            key=str(slot.get_editor_property('material_slot_name'))
            lookup={'Metal':'M_NozzleMetal','Rim':'M_NozzleRim','Ceramic':'M_NozzleCeramic'}
            slot.set_editor_property('material_interface',unreal.load_asset(ROOT+'/'+lookup.get(key,'M_NozzleMetal')))
            updated_slots.append(slot)
        mesh.set_editor_property('static_materials',updated_slots)
        for index, slot in enumerate(updated_slots):
            mesh.set_material(index,slot.get_editor_property('material_interface'))
    assert ASSETS.save_loaded_asset(mesh, False)
    unreal.log('THRUST_SAVED_MATERIALS '+str(mesh.get_editor_property('static_materials')))
    unreal.log('THRUST_MESH '+name+' bounds='+str(mesh.get_bounds()))
unreal.log('THRUST_ASSETS_READY')
