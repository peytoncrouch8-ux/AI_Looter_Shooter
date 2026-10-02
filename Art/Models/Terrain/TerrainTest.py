"""The terrain generator's grounded test area (Docs/Areas/RansomsRest.md, steps 3b and 3c): Art/Levels/TerrainTest/
layout.json through the shared area generator in Art/Levels (area_model.py lists the models it makes and its flags).
One of each feature type on a 150 m core, ridges on three sides and an escarpment on the fourth, with the surround
ring, the canyon wall and the backdrop past it. Its computed outputs are the generator's grounded fixture, as the
tutorial island's are the island one. A scripted model (Art/README.md):

    blender -b --factory-startup --python Art/Models/Terrain/TerrainTest.py -- [--macro] [--computed] [--preview]

models.ps1 runs it without flags: the meshes only.
"""
import os
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
for path in (os.path.join(REPO, 'Art', 'Levels'), os.path.join(REPO, 'Tools', 'Blender')):
    if path not in sys.path:
        sys.path.append(path)

import area_model  # noqa: E402

AREA = area_model.main('TerrainTest')
