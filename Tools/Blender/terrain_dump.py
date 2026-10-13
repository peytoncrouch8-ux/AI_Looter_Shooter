"""Builds an area's terrain with the generator in a given repository tree and saves its rasters, for
Tools/terrain_compare.py. Run in Blender after the usual empty-scene setup:

    blender -b --factory-startup --python Tools/Blender/terrain_dump.py -- --dump-tree TREE --dump-area <Area>
        --save-to DIR [--layout PATH] [--computed] [--macro]

TREE is a repository root (the working tree, or HEAD's generator taken out of git into a temp folder): its Art/Levels
and Tools/Blender are put first on the path, so the build is that tree's. Everything area_model.main() takes after '--'
passes through (--save-to keeps the repository untouched: layout_computed.json, the maps and previews/ go to DIR).
Into DIR it writes rasters.npz: the heights (h), the regional field under them (h_region), the baked occlusion (ao),
the curvature the mesh is sized by (curvature), the distance inside the playable boundary (play_edge), the map
square's half side (half, m), each raster's size, and the top mesh (verts, tris; layout meters).
"""
import os
import sys

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
TREE = os.path.abspath(argv[argv.index('--dump-tree') + 1])
AREA = argv[argv.index('--dump-area') + 1]
OUT = os.path.abspath(argv[argv.index('--save-to') + 1])
for path in (os.path.join(TREE, 'Tools', 'Blender'), os.path.join(TREE, 'Art', 'Levels')):
    sys.path.insert(0, path)

import numpy as np  # noqa: E402

import area_model  # noqa: E402

if os.path.dirname(os.path.abspath(area_model.__file__)) != os.path.join(TREE, 'Art', 'Levels'):
    raise SystemExit(f'terrain_dump: imported {area_model.__file__}, not the tree {TREE}')
area = area_model.main(AREA)
os.makedirs(OUT, exist_ok=True)
top = getattr(area, 'top', {})
arrays = dict(h=area.h.astype(np.float32), ao=np.asarray(area.ao, np.float32),
              curvature=np.asarray(area._curvature, np.float32), half=np.float64(area.half),
              sizes=np.array([area.sizes[k] for k in ('shape', 'curvature', 'ao', 'scatter', 'macro')]),
              verts=np.asarray(top.get('verts', np.zeros((0, 3))), np.float64),
              tris=np.asarray(top.get('tris', np.zeros((0, 3))), np.int64))
if getattr(area, 'h_region', None) is not None:
    arrays['h_region'] = area.h_region.astype(np.float32)
if getattr(area, 'play_edge', None) is not None:
    arrays['play_edge'] = area.play_edge.astype(np.float32)
np.savez(os.path.join(OUT, 'rasters.npz'), **arrays)
print(f'LOOTER: terrain_dump wrote {os.path.join(OUT, "rasters.npz")}', flush=True)
