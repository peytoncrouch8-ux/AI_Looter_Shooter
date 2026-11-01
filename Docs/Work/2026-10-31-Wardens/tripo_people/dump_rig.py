"""Runs a story person's model script (Art/Models/Creatures/<Name>.py) and dumps its rig, body and props to
Intermediate/TripoPeople/<Name>_script.json (+ .blend). Blender, through Tools\artrun.ps1 -Script ... -ScriptArgs <Name>."""
import json
import os
import sys
import time

import bpy
import numpy as np

name = sys.argv[sys.argv.index('--') + 1]
repo = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
path = os.path.join(repo, 'Art', 'Models', 'Creatures', f'{name}.py')
os.environ['LOOTER_PEOPLE_TRIPO'] = '0'
t0 = time.time()
sys.argv = [sys.argv[0], '--']
ns = {'__name__': '__main__', '__file__': path}
exec(compile(open(path, encoding='utf-8').read(), path, 'exec'), ns)
out = dict(seconds=round(time.time() - t0, 1), bones={}, objects={}, materials={})
rigs = [o for o in bpy.data.objects if o.type == 'ARMATURE' and o.parent is None]
for rig in rigs:
    for b in rig.data.bones:
        out['bones'][b.name] = dict(head=list(b.head_local), tail=list(b.tail_local),
                                    parent=b.parent.name if b.parent else None)
for o in bpy.data.objects:
    if o.type != 'MESH':
        continue
    d = dict(parent=o.parent.name if o.parent else None, location=list(o.location), verts=len(o.data.vertices),
             tris=sum(len(p.vertices) - 2 for p in o.data.polygons), mats=[m.name if m else None for m in o.data.materials],
             uv=[u.name for u in o.data.uv_layers], groups=len(o.vertex_groups), hidden=o.hide_render)
    if o.vertex_groups and o.parent in rigs:
        V = np.array([v.co[:] for v in o.data.vertices])
        names = {g.index: g.name for g in o.vertex_groups}
        best = {}
        for v in o.data.vertices:
            if v.groups:
                g = max(v.groups, key=lambda g: g.weight)
                best.setdefault(names[g.group], []).append(v.index)
        d['bone_boxes'] = {k: [V[i].min(0).tolist(), V[i].max(0).tolist(), len(i)] for k, i in best.items()}
        d['slot_faces'] = np.bincount([p.material_index for p in o.data.polygons]).tolist()
    out['objects'][o.name] = d
for o in bpy.data.objects:
    if o.type == 'EMPTY':
        out['objects'][o.name] = dict(parent=o.parent.name if o.parent else None, location=list(o.matrix_world.translation))
for m in bpy.data.materials:
    out['materials'][m.name] = {k: (v if isinstance(v, (int, float, str)) else str(v)) for k, v in m.items()}
work = os.path.join(repo, 'Intermediate', 'TripoPeople')
os.makedirs(work, exist_ok=True)
json.dump(out, open(os.path.join(work, f'{name}_script.json'), 'w'), indent=1)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(work, f'{name}_script.blend'))
print(f'DUMP {name}: {out["seconds"]} s, {len(out["bones"])} bones')
