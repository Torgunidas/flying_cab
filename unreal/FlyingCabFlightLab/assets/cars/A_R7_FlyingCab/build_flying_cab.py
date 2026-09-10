"""Run with Blender --background --python; paths stay relative to this script."""
import bpy, math, json
from pathlib import Path
from mathutils import Matrix, Vector

OUT = Path(__file__).resolve().parent
SRC = OUT.parent / 'A_R7'
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def load(path, name):
    bpy.ops.import_scene.fbx(filepath=str(SRC / path))
    obj = next(o for o in bpy.context.selected_objects if o.type == 'MESH')
    obj.name = name
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return obj

body = load('Body/A_R7_Body_1.fbx', 'Body_A_R7_1')
# Keep every body vertex, face, UV and material unchanged.
body_coords = [v.co.copy() for v in body.data.vertices]
parts = [body]
for side, sign, source in [('Right', -1, 'Rims_1_R.fbx'), ('Left', 1, 'Rims_2_L.fbx')]:
    wheel = load('Rims/' + source, 'Nozzle_Front_' + side)
    # Source rims face outward along +/-Y. Rotate about the car's X axis
    # so both exterior rim faces point along world -Z.
    rotation = Matrix.Rotation(-sign * math.pi / 2, 4, 'X')
    wheel.data.transform(rotation)
    assert (rotation.to_3x3() @ Vector((0, sign, 0)) - Vector((0, 0, -1))).length < 1e-6
    wheel.location = (0.96, sign * 0.69, 0.21)
    wheel['source_file'] = 'A_R7/Rims/' + source
    wheel['rotation_degrees'] = -sign * 90
    wheel['nozzle_direction'] = '-Z'
    parts.append(wheel)
    rear = wheel.copy()
    rear.data = wheel.data.copy()
    bpy.context.collection.objects.link(rear)
    rear.name = 'Nozzle_Rear_' + side
    rear.location.x = -1.10
    parts.append(rear)

assert all((v.co - old).length == 0 for v,old in zip(body.data.vertices, body_coords))
bpy.context.view_layer.update()
bpy.ops.object.select_all(action='DESELECT')
for o in parts: o.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0
bpy.ops.export_scene.fbx(filepath=str(OUT / 'A_R7_FlyingCab.fbx'), use_selection=True,
    object_types={'MESH'}, axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
    bake_anim=False, add_leaf_bones=False, mesh_smooth_type='FACE')

# Studio objects are excluded from the FBX export.
studio = bpy.data.collections.new('Preview_Studio')
bpy.context.scene.collection.children.link(studio)
def studio_link(obj):
    for c in list(obj.users_collection): c.objects.unlink(obj)
    studio.objects.link(obj)
def aim(obj, target): obj.rotation_euler = (Vector(target) - obj.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(4.4,-5.3,2.8))
camera=bpy.context.object; camera.name='Preview_Camera'; studio_link(camera)
aim(camera,(0,0,0.32)); camera.data.type='ORTHO'; camera.data.ortho_scale=4.8
scene=bpy.context.scene; scene.camera=camera
for loc,power,size in [((1,-3,5),650,5),((-4,-1,2),450,4),((1,4,4),850,3),((0,-2,-3),200,3)]:
    bpy.ops.object.light_add(type='AREA', location=loc)
    light=bpy.context.object; light.data.energy=power; light.data.shape='DISK'; light.data.size=size
    aim(light,(0,0,0.3)); studio_link(light)
scene.world.color=(0.16,0.16,0.16)
scene.render.engine='CYCLES'; scene.cycles.samples=32
scene.render.resolution_x=1400; scene.render.resolution_y=1000; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.view_settings.view_transform='AgX'
for area in bpy.context.screen.areas:
    if area.type == 'VIEW_3D':
        area.spaces.active.region_3d.view_distance=5
        area.spaces.active.region_3d.view_location=(0,0,0.3)
        area.spaces.active.region_3d.view_rotation=camera.rotation_euler.to_quaternion()
bpy.ops.object.select_all(action='DESELECT')
for o in parts: o.select_set(True)
bpy.context.view_layer.objects.active=body
scene.render.filepath=str(OUT / 'preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'A_R7_FlyingCab.blend'))
bpy.ops.render.render(write_still=True)
camera.location=(4.4,-5.3,-2.6); aim(camera,(0,0,0.25))
scene.render.filepath=str(OUT / 'preview_underside.png')
bpy.ops.render.render(write_still=True)
print('VERIFIED: body unchanged; four source wheels; exterior faces rotated exactly 90 degrees to -Z.')
