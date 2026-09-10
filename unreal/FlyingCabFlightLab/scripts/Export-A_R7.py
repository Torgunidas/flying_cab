"""Blender background script: derive a centered game mesh from the edited source."""
import bpy, json
from pathlib import Path
from mathutils import Vector
project = Path(__file__).resolve().parent.parent
out = project / 'Build' / 'A_R7'
out.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(project / 'assets/cars/A_R7_FlyingCab/A_R7_FlyingCab.blend'))
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
points = [o.matrix_world @ v.co for o in objects for v in o.data.vertices]
lo = Vector([min(v[k] for v in points) for k in range(3)])
hi = Vector([max(v[k] for v in points) for k in range(3)])
center = (lo+hi)/2
scale = 220 / (hi.x-lo.x)
lines = ['mtllib A_R7.mtl', 'o A_R7_Supercar']
materials = {}
idx = 1
for obj in objects:
    mesh = obj.data
    for mat in mesh.materials:
        name = mat.name.split('.')[0]
        shader = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if mat.use_nodes else None
        color = list(shader.inputs['Base Color'].default_value)[:3] if shader else list(mat.diffuse_color)[:3]
        materials[name] = color
    for poly in mesh.polygons:
        lines.append('usemtl '+mesh.materials[poly.material_index].name.split('.')[0])
        for li in poly.loop_indices:
            p = (obj.matrix_world @ mesh.vertices[mesh.loops[li].vertex_index].co-center)*scale
            n = (obj.matrix_world.to_3x3().inverted().transposed() @ mesh.corner_normals[li].vector).normalized()
            uv = mesh.uv_layers.active.data[li].uv if mesh.uv_layers.active else (0,0)
            lines += ['v %.6f %.6f %.6f' % tuple(p), 'vt %.6f %.6f' % tuple(uv), 'vn %.6f %.6f %.6f' % tuple(n)]
        lines.append('f '+' '.join(f'{i}/{i}/{i}' for i in range(idx, idx+len(poly.loop_indices))))
        idx += len(poly.loop_indices)
(out/'SM_A_R7_Supercar.obj').write_text('\n'.join(lines)+'\n')
(out/'A_R7.mtl').write_text('\n'.join('newmtl '+n+'\nKd '+' '.join(map(str,c)) for n,c in materials.items())+'\n')
(out/'materials.json').write_text(json.dumps(materials, indent=2))
print('A_R7 GAME BOUNDS', tuple((hi-lo)*scale))
