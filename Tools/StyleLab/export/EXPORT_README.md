## Export

The Style Lab draws Ransom's Rest from the game's own sources: the scripted Blender models, the texture sets, the
area generator's terrain and maps, and the level build's placements. `Tools/StyleLab/export/` turns them into
`Saved/StyleLab/export/` (generated, never committed: binaries stay out of the repository):

    Tools/StyleLab/export/export_all.sh [--force]

| Step | Script | What it does | Time |
|---|---|---|---|
| 1 | `terrain_build.py` | Builds the terrain with `Art/Levels/area_model.py` (as `Art/Models/Terrain/RansomsRest.py` does) into `Saved/StyleLab/work/terrain.blend` | ~3.5 min, skipped when the blend exists (`--force` rebuilds) |
| 2 | `run_models.py` | Runs every `Art/Models/*/*.py` through `model_export.py`, one process each, 3 at a time; cached by the script's and the kits' hashes | ~7 min cold |
| 3 | `terrain_export.py` | `terrain_near.glb` (the game's core mesh inside the region), `terrain_far.glb` (the rest of the core, water, ring, canyon wall, backdrop), `heights.bin`, the macro map and masks | ~1 min |
| 4 | `scene_build.py` | Runs `Tools/Unreal/build_area.py RansomsRest` (the full build, every `build_area_*.py`) under `mock_unreal.py` and records what it spawns | ~10 s |
| 5 | `lab_export.py` | `scene.json`, `manifest.json`, `models/pack_N.glb` (< 13 MB each), the texture sets as webp | ~35 s |
| 6 | `check.py [--blender]` | The contract's checks (packs parse, nodes = manifest, every material has a record, every texture exists, Main Street and the chapel where they belong) | seconds |

Python is the bpy 4.5 module: `/home/user/bpyenv/bin/python`. Outputs are deterministic (rerun, same bytes).

**Models** (`model_export.py`). Each model script runs in an empty scene as `Tools/models.ps1` would; every top-level
mesh or armature is a model, its child meshes merged with modifiers applied and its pivot at the origin, `UCX_`/`USP_`/
`UCP_` and `_` objects left out. UVs, custom normals, the `Col` vertex colours (glTF `COLOR_0`, linear: A is baked
occlusion; vegetation R wind, G phase) and the real material names are kept; textures are not embedded (the manifest
points at the sets). Rigs keep their skin and bones (`bones`, `boneRest`); sockets come in three's axes and Unreal's.
`Gun_Bullpup` and `Gun_Ranchhand` are each script's `DEFAULT` parts on the body's sockets, as the game assembles a common
gun. Model scripts write into the repository as they run (the spider's texture set, `DenDressing.placement.json`, the
screen weave): a write guard (`labcommon.install_write_guard`) sends every such write to `Saved/StyleLab/work/sandbox`,
so the export never changes a tracked file. The kits' Windows font paths are pointed at `Art/Fonts/Rye-Regular.ttf`.

**Placements** (`mock_unreal.py`, `scene_build.py`). The level build's own Python runs unchanged against a stand-in
`unreal` module: actors, components, attachments, sockets, instanced meshes and material slots behave as in the
editor, and every trace (`line_trace_component`, `SystemLibrary.line_trace_single`, `capsule_overlap_components`)
meets the real triangles of the exported models and terrain pieces through mathutils BVH trees. So cliffs, panels,
talus, abutments, walkway clearing, platforms, dressing (fences plumb on slopes, grave rows, yard props), effects,
house lights and every story script (`build_area_story/chapel/sink/den/deck/depot/caches/farm/whitlock/posters`) place
what they place in Unreal; only the light, sky and fog step is skipped. What C++ constructors add is filled in from
the C++'s asset paths (the Keeper's Lantern and its snare, the depot's building, the train's cars at their layout
spots, wanted posters as cards over `T_Posters_BC`'s cells). PCG's scatter is exported as rules (`scene.scatter`, from
`Tools/Unreal/build_island_scatter.py`: cell, keep threshold, slope band, scale, models, masks), with the generator's
listed scrub (pit tufts and sage, rim and crest junipers, sage groups) as instances. `scene.noTrees` holds the boxes
the tree layers keep out of (build_area.py's no_tree_zones from `level.noTreeZones`), and the rules built with
`trees=True` (trees, meadow trees, crease pines) carry `noTrees: true`.

**Terrain.** `Terrain_Near`/`Terrain_Far` are the game's own adaptive core mesh (143k triangles over 400 m, the same
triangles Unreal draws at every quality), cut along the region with shared seam vertices; `heights.bin` is ray-cast
from them. Every lab map is row 0 at z0, column 0 at x0 (load with flipY = false), and RGB or grayscale: a map's alpha
(the macro map's detail selector, the masks' last channel) is a grayscale file of its own (`<map>_A.webp`), since a
browser loses the colour under alpha 0. `manifest.terrain.rules` gives M_Terrain's blend with Ransom's Rest's numbers.

**Textures.** RGB and alpha are always resized apart (Pillow resizes RGBA premultiplied, which wipes the colour under
alpha 0); cut-out colour maps (leaves, needles, webs, posters) are lossless RGBA webp keeping the source's dilated
colour under alpha 0, which `check.py` compares with the source.

**Materials.** One record per material name, with the area's swaps applied in the GLBs (`MI_RockCliff_Ransom`,
`LeavesOak_Ransom`, the dark cottages' `WindowGlow_Dark`, the Sink's own webs and granite, the windmill's steel):
`master` (the game's `gameMaster` mapped onto the contract's list), `set`, `tint`/`tintLinear`, representative `color`,
`roughness`/`metallic` (the set's ORM means), `glow`, `twoSided`, `alphaMask`, `wind`, `role`, and `params` (moss,
tint variation, opacity). Normal maps are DirectX.

**Not exported / approximated.** The light, sky and fog (each style brings its own); the Gravewind's dusk-only wisps and
canyon fog; the cold open's set. Creatures stand at their spawners (spread over the spawn radius, seeded); the
Gravemother is `Spider__Pale` at 1.8x. House lamps use the game's candelas with a warm colour (the C++ light's colour is
not in the data). Posters are cards, not decals. Smoke is listed as points (`SmokePlume` is in the packs).
