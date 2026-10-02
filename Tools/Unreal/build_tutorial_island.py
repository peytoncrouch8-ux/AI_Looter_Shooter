"""The tutorial island's level build, kept so older docs and commands still work: it is build_area.py TutorialIsland.
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_tutorial_island.py [gameplay]"
review_stage.py imports it for the island's light (environment()) and its level (LEVEL).
"""
import importlib
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area  # noqa: E402

ISLAND = build_area.AreaBuild('TutorialIsland')
LEVEL = ISLAND.level
environment = ISLAND.environment

if __name__ == '__main__':
    # The editor keeps imported modules between runs: reload, so an edited build_area.py takes effect.
    importlib.reload(build_area).run('TutorialIsland', only_gameplay='gameplay' in sys.argv[1:])
