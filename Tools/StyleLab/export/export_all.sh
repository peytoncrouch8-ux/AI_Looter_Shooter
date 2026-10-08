#!/bin/bash
# The whole Style Lab export, from the game's own sources into Saved/StyleLab/export (Tools/StyleLab/export/EXPORT_README.md):
#   1. terrain_build.py   Ransom's Rest's terrain with the game's generator (Art/Levels/area_model.py)   ~3.5 min
#   2. run_models.py      every scripted model, one process each, 3 at a time (cached by script + kit)   ~7 min
#   3. terrain_export.py  terrain_near/far.glb, heights.bin, the macro map and the masks                  ~1 min
#   4. scene_build.py     Tools/Unreal/build_area.py RansomsRest, the full build, under mock_unreal.py     ~10 s
#   5. lab_export.py      scene.json, manifest.json, the packs and the texture sets                      ~35 s
#   6. check.py           the contract's checks
# Usage: Tools/StyleLab/export/export_all.sh [--force]   (--force rebuilds every model and the terrain)
set -euo pipefail
cd "$(dirname "$0")/../../.."
PY=/home/user/bpyenv/bin/python
HERE=Tools/StyleLab/export
WORK=Saved/StyleLab/work
mkdir -p "$WORK" Saved/StyleLab/export
export PYTHONDONTWRITEBYTECODE=1
if [[ "${1:-}" == "--force" || ! -f "$WORK/terrain.blend" ]]; then
  $PY $HERE/terrain_build.py > "$WORK/terrain_build.log" 2>&1
  tail -1 "$WORK/terrain_build.log"
fi
$PY $HERE/run_models.py --jobs 3 ${1:-}
$PY $HERE/terrain_export.py 2>&1 | grep '^STYLELAB'
$PY $HERE/scene_build.py 2>&1 | grep '^STYLELAB'
$PY $HERE/lab_export.py 2>&1 | grep '^STYLELAB'
$PY $HERE/check.py | tail -3
echo "ALL DONE"
