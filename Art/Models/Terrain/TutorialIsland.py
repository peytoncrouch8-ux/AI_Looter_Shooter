"""The tutorial island's terrain (Docs/TutorialIsland.md): Art/Levels/TutorialIsland/layout.json through the shared area
generator in Art/Levels (area_model.py lists the models it makes and its flags: the tiles, the underside and the
water, and with --macro, --computed and --preview the macro map, the scatter mask, layout_computed.json and previews).
A scripted model (Art/README.md):

    blender -b --factory-startup --python Art/Models/Terrain/TutorialIsland.py -- [--macro] [--computed] [--preview]

models.ps1 runs it without flags: the meshes only.
"""
import os
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
for path in (os.path.join(REPO, 'Art', 'Levels'), os.path.join(REPO, 'Tools', 'Blender')):
    if path not in sys.path:
        sys.path.append(path)

import area_model  # noqa: E402

AREA = area_model.main('TutorialIsland')
