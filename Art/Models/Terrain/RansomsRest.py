"""Ransom's Rest's terrain (Docs/Areas/RansomsRest.md, step 4): Art/Levels/RansomsRest/layout.json through the shared
area generator in Art/Levels (area_model.py lists the models it makes and its flags). A grounded valley: the 400 m core
square at 12.5 cm heights in 25 tiles (the valley, Ransom's Point, the farm terrace, the Sink, the Dry Wash, Mill Creek
and its falls into Mill Gorge, Coffin Rock, the Nose, the outcrops' knobs), the surround ring past it (Larkspur Ridge
and the higher ridge behind it, the East Ridge with Stage Gap's saddle, the Hogback, the lowlands past them), the
canyon wall under the Rim and the backdrop's silhouettes. A scripted model (Art/README.md):

    blender -b --factory-startup --python Art/Models/Terrain/RansomsRest.py -- [--macro] [--computed] [--preview]

models.ps1 runs it without flags: the meshes only.
"""
import os
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
for path in (os.path.join(REPO, 'Art', 'Levels'), os.path.join(REPO, 'Tools', 'Blender')):
    if path not in sys.path:
        sys.path.append(path)

import area_model  # noqa: E402

AREA = area_model.main('RansomsRest')
