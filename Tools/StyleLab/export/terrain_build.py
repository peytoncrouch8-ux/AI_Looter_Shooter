"""Style Lab export, step 1: builds Ransom's Rest's terrain with the game's own generator (Art/Levels/area_model.py,
as Art/Models/Terrain/RansomsRest.py does, meshes only) under the bpy module and keeps what the later steps need:
    Saved/StyleLab/work/terrain.blend   the core's tiles, the water, the ring, canyon wall and backdrop
    Saved/StyleLab/work/terrain.npz     the height raster (m, valley base 0; x north, y east, layout metres), the
                                        core's inside mask, the steepness and the map square's half side
Run: /home/user/bpyenv/bin/python Tools/StyleLab/export/terrain_build.py [workdir]
"""
import os
import sys
import time

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
WORK = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(REPO, 'Saved', 'StyleLab', 'work')
for p in (os.path.join(REPO, 'Art', 'Levels'), os.path.join(REPO, 'Tools', 'Blender')):
    if p not in sys.path:
        sys.path.insert(0, p)
os.makedirs(WORK, exist_ok=True)

import bpy  # noqa: E402
import numpy as np  # noqa: E402

sys.argv = ['blender', '--']
import area_model  # noqa: E402

started = time.time()
bpy.ops.wm.read_factory_settings(use_empty=True)
area = area_model.main('RansomsRest')
extra = {}
for name in ('inside', 'edge', 'road_gap', 'h_natural'):
    value = getattr(area, name, None)
    if isinstance(value, np.ndarray):
        extra[name] = value
water = area.water_surface()
np.savez_compressed(os.path.join(WORK, 'terrain.npz'), h=area.h.astype(np.float32), half=np.float32(area.half),
                    water=water.astype(np.float32), ao=np.asarray(area.ao, np.float32), **extra)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(WORK, 'terrain.blend'), compress=False)
print(f'STYLELAB: terrain built in {time.time() - started:.0f} s; raster {area.h.shape}, half {area.half} m', flush=True)
