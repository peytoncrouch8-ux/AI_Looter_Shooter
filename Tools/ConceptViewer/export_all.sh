#!/bin/bash
# Exports every model the concept viewer uses, one model script per process (a few kits leave state behind), then the
# terrain. Output goes to Saved/ConceptViewer/web (ignored by git) unless a folder is given.
#   Tools/ConceptViewer/export_all.sh [outdir]
set -u
cd "$(dirname "$0")/../.."
OUT="${1:-Saved/ConceptViewer/web}"
mkdir -p "$OUT"
for src in Buildings/Barn.py Buildings/Bridge.py Buildings/Cottage.py Buildings/Farmhouse.py Buildings/GunRack.py Buildings/LogCabin.py Buildings/LookoutTower.py Buildings/Outhouse.py Buildings/Well.py Buildings/Windmill.py Buildings/FalseFronts.py Buildings/Depot.py Props/Fences.py Props/VillageProps.py Props/Containers.py Props/FarmProps.py Props/LanternPost.py Props/SkiffJetty.py Props/Boardwalk.py Props/Ruins.py Rocks/Cliffs.py Rocks/Rocks.py Rocks/Outcrops.py Rocks/SkyIslands.py Vegetation/DeadTree.py Vegetation/Pond.py Creatures/Spider.py Creatures/Slime.py Vehicles/Skiff.py; do
  python3 Tools/ConceptViewer/web_export.py "$OUT" models "$src" 2>&1 | grep "^EXPORT: [A-Za-z_]* *[0-9]* tris\|FAILED\|could not patch"
done
python3 Tools/ConceptViewer/web_export.py "$OUT" terrain 2>&1 | grep "EXPORT:"
echo "ALL DONE"
