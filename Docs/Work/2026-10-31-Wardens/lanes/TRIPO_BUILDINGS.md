# Lane TRIPO-BUILDINGS: Tripo main buildings (2026-11-01)

Nothing imported, built or committed; Unreal never opened. Blender only through `Tools\artrun.ps1`. Exports in
`Intermediate/ArtExport_TripoBuildings` (the old ones, for comparison: `_Old`, `_OldHomes`).

## Credits (cap 480): 460 used, 20 left

| Model | Credits | Used? |
|---|---|---|
| FarmhouseRansom | 30 + repaint 20 | **fitted** (SM_Farmhouse_Ransom), repaint is the default |
| BarnRansom | 30 + repaint 20 | **fitted** (SM_Barn_Ransom), repaint |
| CottageRansom | 30 | **fitted** (SM_Cottage_Ransom) |
| WindmillRR | 30 + redo 30 | **fitted** (SM_RR_Windmill + its wheel as SM_RR_WindmillFan). Try 1 was a Dutch smock mill (`WindmillRR_try1.glb`, kept) |
| FrontAssay, FrontBarber | 30 each | fitted, judged worse, **not hooked** (below) |
| DepotRR, ChapelRR, WorkshopBright, BarnWhitlock, SmithyRR, FrontHotel, WatchtowerRR | 30 each | made, not fitted (below) |

Records: `Art/Tripo/<Name>/task.json`. Specs: `Art/Tripo/specs/buildings.json` (prompts <=1024 chars, negative <=255:
the API refuses longer, no charge). Prompts name the town palette; Tripo ignores stated sizes (the barn came out cubic,
the farmhouse/barn first paints bleached white-blotchy and rust-orange).

## Pictures (`Saved/ArtPreviews/TripoBuildings/`)

`Check_Final.png` (old | new, front and back, new with its hulls in red: farmhouse, barn, cottage, windmill),
`Farmhouse_Porch.png`, `Farmhouse_Roof34.png` (3/4 roof close-up), `Farmhouse_PorchUnder.png`, `Farmhouse_DoorSide.png` (sockets as magenta balls), `Farmhouse_FirstPaintGraded.png` (Tripo's first
paint, graded: rusty, derelict), `Fronts_Windmill_FirstFit.png` (assay/barber judged), `Raw_*.png` (Tripo's GLBs as
delivered), `style/` (the style references).

## Style (the user's "match our current art style")

- Prompts: weathered warm honey/grey-brown wood, grey-brown shakes, grey fieldstone, faded teal and oxide-red paint,
  matte hand-painted stylized realism; negatives: glossy, plastic, cartoon, low poly, faceted, grey clay, blue roof.
- Repaints with a style picture (`Tools/tripo.py` now uploads a spec's `paint_style` image as
  `texture_prompt.style_image`): farmhouse (style `Art/Tripo/style/cottage_preview.png`, our Cottage_Ransom render),
  barn (`barn_ingame.png`, an in-game crop). Lesson: an in-game crop carries the game's golden light: the barn came back
  3-4x as saturated as our trim sheet. Use neutral-light preview renders as style pictures.
- Grading (`looter_tripo_buildings.grade`, every export): each Tripo colour map's two colour-difference channels get the
  mean and spread of `T_HouseTrim_BC` (the sheet every scripted building wears), brightness half way to its mean x0.92.
  Measured (rg / yb chroma): trim 0.040/0.076; barn repaint 0.175/0.239 -> 0.040/0.076; cottage 0.072/0.106 -> trim;
  windmill 0.130/0.182 -> trim. Red frames and teal doors stay relatively red/teal.
- Materials: one slot per building, `MI_Tripo<Name>` on **M_World** (the trim sheet's master), maps
  `Art/Textures/Tripo<Name>/T_Tripo<Name>_BC/_N/_ORM.png` at 2048 (ORM: occlusion 1, Tripo's roughness/metal; normal
  flipped to DirectX); baked vertex AO in colour alpha like every building. The farmhouse's kept scripted parts keep
  their own slots (HouseTrim, WindowGlow, MetalRust, LanternGlow, MourningCrepe).
- Judged beside `01_calm.png`, Tour `AC_HomeA_Porch`/`AC_Undertaker`, `Homes_row.png`: after grading the four sit in the
  homes' palette. Farmhouse repaint is tidy and lived-in but its roof planks follow Tripo's corrugation ridges (reads
  wavy close up); the first paint (rust tin, `LOOTER_FARMHOUSE_PAINT=_grey`) is derelict and keeps bright rust (the
  loot-colour reason Barn_Ransom lost its tin). Cottage walls stay a little streakier/warmer than the homes.

## How it's built

- `Tools/Blender/looter_tripo_buildings.py` (new): `swap()` loads the GLB (`looter_tripo.load`), turns +X front to -Y,
  stretches it onto the scripted model's bounds (or a custom `fit`), bends it piecewise so features land on the
  scripted ones (`anchors`: porch deck height, front wall plane, measured by rays on both), collapse-decimates
  (`faces_limit`), cuts clean box holes (`cut_box`: bisect then delete), refits the scripted UCX hulls to the shell
  (26-direction k-DOP of the shell's vertices in each old hull's box; hulls with no shell there are dropped), adds box
  hulls round porch posts found by rays (`posts`), writes + grades the set, bakes AO, joins kept scripted faces
  (`keep` boxes), replaces the mesh (name, sockets, custom props kept). `apply(obj, name, **parts)` is the hook.
  Switch `LOOTER_BUILDINGS_TRIPO` (unset/1 all, 0 scripted, or a list `FarmhouseRansom,BarnRansom`).
- `Art/Models/Buildings/Tripo/parts_buildings.py` (new; models.ps1 skips parts_*): `FITS` per building.
- Hooks (2-5 lines each): `Farmhouse.py`, `Barn.py`, `Cottage.py` after the Ransom model; `Homestead.py` after `models`.
- `Tools/Blender/tripo_buildings_check.py` (new): old/new renders with hulls (hangs a `Fan` model on its socket).
- `Tools/tripo.py`: `upload()` + spec `paint_style` (additive). CODEMAP: one bullet before "Tripo creatures".

## Per building (Blender m, front -Y; UE = (-y, -x, z) x100)

- **SM_Farmhouse_Ransom** 22,869 tris (was 8,816), 13 hulls, 7 sockets. Deck anchored 0.567->0.55, front wall
  -3.40->-3.10. Shell cut at the door (x +-0.66) and in front of it to x -1.45 (Tripo's own screen door, swung out).
  Kept scripted: the hall (x -0.6..1.25, lamp, runner, shawl, crepe-covered mirror), door casing + crepe + open door,
  the stool under SOCKET_Plate, the porch lantern at SOCKET_Light_Porch, broom and pail, the rocking chair (Abel's hat,
  `build_area_landfall.py HAT_CHAIR`).
  Main's review fixes (2026-11-01, no credits):
  - Posts: Tripo's (stretched white paint) found by rays at x -3.55, -0.68, 1.64, 3.55 (y -5.08), cut out ground to
    roof, replaced by the scripted post (`FARM_POST`, trim-sheet beam wood) stretched from the ground to Tripo's porch
    roof underside (2.93); box hulls 18 cm round each; the scripted post hulls (x +-1.15, +-3.45) dropped.
  - Roofs: `looter_tripo_buildings.flatten` fits planes (least squares, outliers over 0.2 m out) and moves vertices
    onto them: main roof front/back tops (rms off plane 0.05/0.015 m after the pass), porch roof top and underside,
    the porch deck top (z 0.589 - 0.008 y: 0.557 at Delia's spot). Dormer (its slopes face +-X) and stovepipe left.
    The flattening alone still read wavy: Tripo painted the corrugation into the colour map. So the roof and porch-roof
    tops lying on their planes (733 faces) are on the house trim sheet's weathered boards (MI_HouseTrim, strip A,
    running down the slope, world scale, U along the eaves) like the town's board-roofed sheds; walls, dormer, deck,
    fascias keep the graded Tripo repaint. `LOOTER_FARMHOUSE_ROOF=tripo` keeps Tripo's roof paint.
  - Steps: Tripo's (lumpy) cut in front of its deck edge (y -5.24); the scripted two-tread steps set against it
    (moved +0.06 m in y), their two scripted hulls moved with them.
  - A grey lump Tripo left on the porch roof over the left post removed (faces 5 cm-1.2 m proud of the roof plane).
  Main's second review (edges, no credits): `_farm_edges` measures the straightened porch roof (x -3.92..3.92, front
  y -5.37), deck (x -3.72..3.71, front -5.24) and main roof (x -4.27..4.29, y -3.38..3.32, ridge y -0.23 z 7.26) and
  cuts the ragged side faces of their edges (1,505 faces: porch roof front/ends to 0.22 under its top, deck front/ends
  0.25-0.58, main eaves/gables to 0.22 under the planes, the ridge seam), everything hanging off the porch roof's
  front, post stubs poking through it, and every Tripo-painted face left on the porch roof's top (shards).
  `_farm_trim` puts straight trim-sheet boards there (MI_HouseTrim, hewn-beam strip C, `looter_buildings` boards with
  their own AO): porch fascia 0.24 deep along the front and both ends plus a ledger against the wall, a deck rim 0.32
  deep along the front and ends (top flush with the boards), main eave fascias 0.3 deep front and back, rake boards up
  both gables, a ridge board. Left: a few dark knee-brace remnants under the porch roof by the post tops
  (`Farmhouse_PorchUnder.png`), Tripo's dormer roof (small, slightly wavy).
- **SM_Barn_Ransom** 10,999 (was 7,671), 2 hulls refitted. Tripo's barn was nearly cubic: stretched 1.44x more along
  its length than across (boards and side windows read wider).
- **SM_Cottage_Ransom** 9,999 (was 7,036), 9 hulls (stoop posts found at x -2.05, -0.36). Stoop 0.21->0.40, wall
  -2.79->-3.20.
- **SM_RR_Windmill** 12,000 (was 2,780) + **SM_RR_WindmillFan** 2,783 (was 992; Nanite off, LODs 40/12, no collision as
  before). Scaled evenly x2.86 so the hub is on SOCKET_Fan (0, -0.98, 8.38); wheel radius 1.60 (scripted 1.70); tail
  vane stays on the tower. Tower narrower than the scripted (4.65 x 3.54 m incl. tank vs 5.5 x 6.3), 11.8 m tall
  (10.7); tank at x 1.4-3.0 (scripted 1.7-3.8). 4 hulls refitted (tower frustum, platform, head, tank).

## Sockets checked (old vs new manifests)

Same names and count on all five. Moved only: Farmhouse `Smoke` (2.75,-1.25,7.63) -> (0.02,-0.62,7.90) and Cottage
`Smoke` (-1.0,1.9,7.43) -> (-0.89,1.5,7.46): onto the Tripo stovepipe/chimney tops (the old spots are open roof now).
Every story socket (Speaker, ScreenDoor, Handoff, Plate, Light, Light_Porch, Fan) is unchanged, and the scripted parts
under them were kept. Traces other lanes make onto the farmhouse (measured on the new mesh): Delia's spot (0,-4.2)
deck 0.552 (hull 0.584: feet ~3 cm high), barricade spots 0.549-0.555, chair seat 1.048 on the chair's side (the
other side 0.55 deck: the script tries both and takes the 85-125 cm one), **Hob's porch-beam trace (+-3.45, -5.13)
now 2.88 m (was 3.29)**: Tripo's porch roof is lower; he sits on its edge. After the fixes: Delia's spot deck 0.557,
hull 0.585; barricade spots 0.551; chair seat 1.048.

## Not fitted, and why

- Depot, Chapel, Workshop Bright, Whitlock barn, Smithy, Watchtower: walked into or climbed (rooms, cellar, nave,
  bale bay, forge, inside stairs) with story sockets inside. A Tripo building is one closed shell; using these means
  cutting their openings and keeping the scripted interiors (the farmhouse hall method, much bigger). Of the GLBs:
  **Chapel is very good** (white clapboard, centred bell tower, spire, red doors, pointed windows); Whitlock barn and
  Smithy good shapes; Depot right palette but one storey, gable to the front; Workshop and Watchtower read as stone.
- Assay and Barber: fitted (`parts_buildings.assay/barber`, not hooked): they lose what the story reads (PROVIDENCE
  MINING CO. / ASSAY OFFICE, BARBER / SHAVE . BATH . TEETH lettering, the pick and hammer, barred shutters, chained
  door; Tripo's barber has no false front) and would stand among scripted fronts. Hotel: an odd red annex and a 1:3
  stretch; not tried.

## Perf risks

- Medium draws every triangle (fallback 100, decimated in Blender instead): farm view +~17k tris (farmhouse +14.1k,
  barn +3.3k), +3.0k per Cottage_Ransom placed, windmill +9.2k (+1.8k fan). The farm is one of the heaviest views
  (0.2-0.5 ms margin): run `Tools\perf.ps1 -Label "Tripo farm buildings"` before/after; if over, lower `faces_limit` in
  `parts_buildings.py` (farmhouse 17000, barn 11000, cottage 10000, windmill 12000) and re-export.
- Four new 2K sets (BC/N/ORM): ~12 textures, ~16-21 MB of GPU memory with mips.

## Known gaps

- FIXED 2026-11-01 (after import, `Looter.World.HouseLights` failed: no WindowGlow faces, so Unreal dropped the slot):
  `parts_buildings._lit_windows` copies the scripted lit window's glass (WindowGlow) and curtains, stretched, into each
  of Tripo's painted windows the scripted house had lit (farmhouse: both front; cottage: front, front gable, the front
  window on each side), 1.2 cm proud of the painted glass, behind Tripo's own modelled sash bars (scripted bars
  dropped). Rects measured on the colour maps (`FARM_LIT`, `COTTAGE_LIT`). Export: `Intermediate/ArtExport_TripoLitWindows`
  (Farmhouse, Cottage); check `Saved/ArtPreviews/TripoBuildings/LitWindows_Dusk.png`. A re-fit that moves the shell
  needs the rects re-measured. The cottage's SOCKET_Light dusk lamp sits inside a closed shell (unshadowed, so it
  lights nothing outside; drop it or leave it).
- Hob's perch 41 cm lower; Delia's porch hull 3 cm over the boards at the door.

## Main's import steps

1. Commit (LFS for binaries): `Art/Tripo/{FarmhouseRansom,BarnRansom,CottageRansom,WindmillRR}/` (the exports need
   these GLBs; the other nine folders are reference), `Art/Tripo/specs/buildings.json`, `Art/Tripo/style/*.png`,
   `Art/Textures/Tripo{FarmhouseRansom,BarnRansom,CottageRansom,WindmillRR}/`, the scripts above, CODEMAP.
2. Editor open: `Tools\console.ps1 "Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_TripoBuildings"`.
   The folder also holds byte-for-byte rebuilds of models these scripts always export (SM_Farmhouse, SM_Barn, SM_Cottage,
   SM_ScreenDoor, SM_PorchPlate*, SM_RR_Home_A-J, SM_RR_Well, SM_RR_Outhouse): harmless. New: MI_TripoFarmhouseRansom,
   MI_TripoBarnRansom, MI_TripoCottageRansom, MI_TripoWindmillRR on M_World.
3. Re-run Ransom's Rest's gameplay pass (build_area_story / build_area_landfall measure Hob's porch perch and the hat's
   seat by traces at build time).
4. Look in game at the farm (01_calm view) and the windmill; `Tools\perf.ps1` before/after; a short PIE play-check of
   Delia's porch fight (collision on the new posts) and the Heirloom hand-off.
5. Roll back any time: `LOOTER_BUILDINGS_TRIPO=0` (or a list) and re-export.
