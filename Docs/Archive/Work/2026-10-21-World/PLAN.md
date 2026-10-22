# Ransom's Rest: world variety plan (WORLD-ART, 2026-10-21 night)

Why: the user played RR and said the world looks samey; tonight "anything goes" for variety (terrain and buildings too).
Goal: each place distinct and lived-in at big-studio quality, 120 fps Medium kept (perf after the gunfeel sounds: 7.6 ms,
p95 8.0, RR 8 spiders; budget 8.3 ms). Look stays today's textured stylized realism (no style change; no faceted low-poly).
Rules kept: variants in the same script as the original, originals export byte-identical (checked, see "Phase 1 done").

## 1. Measured sameness

Source: `Saved/Tour/DrawShares.csv` (2026-10-14, after the expansion; 8 tour views; ISM counts are whole components,
actors per view), `layout.json`, `town.json`, `build_area_dressing.py`, `build_island_scatter.py`, `level.cliffLife`.

| Asset (meshes) | Instances in the level | Worst single view | Where the same few repeat |
|---|---|---|---|
| Grass: GrassClump_A/B/C, TallGrass_A, Clover_A | 53,000 | everywhere | one mix on every zone; only the macro's tone changes |
| Sagebrush_A/B/C, Rabbitbrush_A, DryTuft_A/B | 7,400 | everywhere | slopes, bluffs, ridges: same 6 meshes |
| **CliffFace_A-D** (4 meshes) | **160 actors** | **204 in PointOverValley** (A 63, B 59, D 48, C 34) | every wall: boundary, bluffs, Sink, Hogback, gorge |
| **Bush_A/B/C** | **311** | all views | forest edges, meadows, every zone alike |
| **Juniper_A/B** (+ CliffJuniper_A-C 59) | **249** (+59) | all views | creases, benches, crests, rims |
| **Pine_A/B** | **151** (+ far pines 1,541) | Grave/ValleyNorth 151 | every forest: Widow's, Larkspur, bench stands |
| **FenceRail** (+ FencePost 22, FenceBroken 20) | **147** | Farm, Main St | farm, Amos's, Whitlock, pens: one 3 m section |
| PebbleCluster_A | 577 | all | road edges |
| **Grave headboards OldA-D** (+ Cross_A/B 14, MoundSunken 23) | **72** | ChapelOverTown | churchyard, Boot Hill, family plot |
| **Fence_IronSection** | **55** | Grave | the whole churchyard fence |
| LipDrape_C (A 6, B 11) | 53 | all cliff views | every lip |
| HayBale_Square | 38 | Farm | hay stack + Whitlock |
| **Rock_A-D, Boulder_A-C** | 38 + 6 | all | scatter, talus, the Tumble |
| **Stump_A / Log_A** | 31 / 30 | Farm | forest floor: one stump, one log |
| StoneWall kit (6 pieces) | 60 | Farm, Grave | field wall, pound, glebe, sheep fold |
| **Apple_A** | 18 (24 in Farm view) | Farm, MainStreet | the orchard avenue: one tree |
| **DeadTree_A** | 15 | PointOverValley | churchyard, hanging tree, Webwood, Hogback |
| LampPost 10, Barrel_A 9, FlowerBox 6 | town | MainStreet | town.json dressing |

Not samey: buildings (RR_Home_A-J, the fronts, chapel, depot, mine are each built once); story props; outcrops
(11 shapes for 29 spots, already varied by layout).

What actually reads as samey, in order:
1. The nature kit is one kit everywhere: the farm, the bluffs, the chapel knoll and the alpine bench wear the same grass
   mix, the same 3 bushes, 2 junipers, 2 pines, 4 rocks. Zones differ only in macro tone (moisture) and which layer is on.
2. Cliffs: 4 faces carry every wall; a wide view shows each 34-63 times; the same column/notch silhouette repeats.
3. Every area has the same palette: ochre ground, gold grass, green-grey sage, grey granite. No area owns a colour.
4. Hand-made places (fences, graves, orchard) repeat one model in a straight line: the eye catches the clone at once.
5. Ground surface: one golden mosaic everywhere; no trampled dirt, mud, ash, coal dust, leaf litter or flower drifts
   that say "someone lives / works / died here".

## 2. Per area: what makes it samey, what gives it identity, cost

Cost legend: runtime on Medium (RX 580). "Swap" = replaces existing instances (0 draws added). "+N draws" = new ISM
components in a typical view. Terrain macro changes cost nothing at runtime (same textures, same tiles) but need the
~35 min generator run and a re-import.

**Farm, orchard, family plot (spawn; home/orchard).** Samey: one apple tree x18 in rows, FenceRail x60 in straight
runs, the grass mix of the whole valley. Identity: lived-in homestead, the one green watered place.
- Orchard: Apple_A/B/C mixed per tree (old leaning, young replanted), one gap with a stump (Stump_B) and a prop ladder;
  windfall apples under trees (decal or tiny instanced mesh). Swap; +2 draws. Phase 1: Apple_B, Apple_C done.
- Fences: FenceRail / FenceRail_B (three-rail at the barn yard) / FenceRail_C (tired, back field) alternated by seed so
  no run repeats two sections in a row. Swap. Done.
- Ground: kitchen-garden rows and trampled dirt round the yard (macro `yards` already paints; widen it to the barn and
  well), wheel ruts on the farm road. Terrain macro only.
- Clutter: wash day, tools, chicken scratch already there; add milk churns at the gate, a broken plough, firewood.
  town.json-like vignettes for the farm (`make_town.py` pattern). +3-5 draws (instanced props).

**Ransom's Point (lookout).** Samey: bluff walls of CliffFace_A-D, Juniper_A/B on the top. Identity: wind-scoured
lookout, bone-pale rock, spider nest.
- Cliffs: twins (CliffFace_A2-D2) alternate along `bluffPath`/`ransomsPoint` runs. Swap. Done.
- Top: Juniper_B (sheared) and Juniper_D (creeping) only, no bushes; Boulder_E erratics on the skyline; webs between rocks
  at the nest ring. Swap.
- Ground: bleached, sparse grass (macro zone `lookout` drier and paler: already -0.1; push to -0.2 and lift its tone).

**Main Street (town).** Samey: LampPost x10 identical, Barrel_A x9, the same bushes in the back lots. Buildings are fine.
Identity: dusty, half-fled town; red-ochre street dust, litter of the flight.
- Street ground: a trampled red-ochre dust band with wheel ruts and hoof marks (macro `roads` tone for town roads),
  boardwalk-edge litter (paper, a dropped hat) as decals. Terrain macro; decals +2-4 draws.
- Lamps: two lamp variants (one bent, one with a broken glass, in the props script that holds LampPost). Swap.
- Back lots: Bush_E (dusty), Log_B, weeds; no oak/pine there. Swap.
- Colour: shop awnings and bunting are the town's colour accents; keep stores painted (already varied).

**Boot Hill (graveyard).** Samey: OldA-D boards x72, one wooden cross, mounds. Identity: the poor folk's hill, dry and
leaning; wooden markers only, rocks piled on graves.
- Grave_Headboard_OldE/OldF added to the `bootHill` mix; rock-pile graves (Rock_E flags + Rock_D chunks heaped: a new
  `cairnGrave` style in the dressing). Swap; +1 draw.
- DeadTree_C squat at the hanging tree's foot; the hanging tree itself DeadTree_B (taller snag). Swap. Done.
- Ground: sparse, pale, gravelly (macro `graveyard` -0.05 -> -0.18, stony layer).

**Chapel of Saint Ada (chapel).** Samey: the iron fence is one section x55, same boards as Boot Hill. Identity: the
cared-for white chapel, iron, cut stone, green knoll.
- Fence: Fence_IronSection_Leaning mixed in where the glebe wall fell and on the downhill side (1 in 4). Swap. Done.
- Graves: more crosses (Cross_B iron) and the fresh boards here; OldE/OldF fewer. Mix change only.
- Ground: greener grass and white-yarrow drifts on the knoll (macro `chapel` moisture +, `blooms.chapel` white).
- DeadTree_B/C for the three churchyard dead trees (each a different tree). Swap. Done.

**Undertaker's Yard, depot, Stage Gap (travel).** Samey: cliff faces in the gap, rocks. Identity: cinders, coal and
iron: the only industrial ground in the valley.
- Ground: black cinder ballast along the track, coal dust round the coal stage and water tower (macro `travel` tone
  dark grey; currently only drier). Terrain macro.
- Gap walls: twins; Rock_F chunks and FallenSlab at the feet; no grass on the ballast. Swap.
- Clutter: spare rails, sleepers stacked, a hand cart, coal heaps (props script). +3 draws.

**The Sink (pit).** Samey: CliffPanel_A-C (fine), the rim's Juniper/Bush mix like everywhere. Identity: dead, grey,
webbed ground; the place things die.
- Rim: Juniper_D creeping, Stump_B snapped, DeadTree_B/C webbed (Webwood). No green bushes within 15 m. Swap.
- Ground: grey ash and web-white patches on the rim, bones (macro `pit` tone grey-violet, blooms none). Terrain macro.
- Boulder_D tabular blocks on the floor among the collapsed blocks. Swap.

**Whitlock Fields and Mill Creek (fields).** Samey: hay bales one model, the field wall one section, Bush_A-C by the
creek. Identity: the working farm of the valley: mown stripes, hay, water, willows.
- Field wall: StoneWall / StoneWall_B (coping fallen, stones at its foot) / StoneWall_C alternated. Swap. Done.
- Creek: Bush_D (russet sumac) and reeds along the creek bottom, Boulder_F rounded erratics in the water's edge, Log_B
  bleached driftwood. Swap; +2 draws.
- Hay: a weathered grey HayBale variant for last year's stack (FarmProps; not made yet). Swap.
- Ground: the mown stripes exist (hayfieldWest); give hayfieldEast rows too (rows 0 -> 90). Terrain macro.

**Gravewind Point, the west road, Widow's Pines, Dry Wash (boss_arena, forest).** Samey: Pine_A/B only, the
Hogback/escarpment cliffs. Identity: the dark, cold wood and the wind-blasted point over the canyon.
- Widow's Pines: Pine_E tall boles in the middle, Pine_C young at the edges, Pine_D old and dead-topped in the gaps;
  fern and Stump_B/Log_A/Log_B on its floor; no oaks. Swap (weights); +2-3 draws.
- Point: Juniper_B/D only, Boulder_E, no bushes; fog cards at dusk exist. Swap.
- Dry Wash: Boulder_F and Rock_E cobbles along the wash bed, Log_B driftwood, pale sand tone (macro). Swap; terrain macro.
- Burnt homestead / sheep fold: StoneWall_B/C; ash patch at the chimney (macro). Swap.

**Old Quarry and Spoil Bank (quarry).** Samey: rocks A-D, same cliffs. Identity: cut, pale, angular stone.
- Boulder_D tabular blocks and Rock_E flags in the spoil, drill-marked cut faces (a new `QuarryFace` cliff piece with
  flat sawn planes: not made yet). Swap; +1 draw.
- Ground: pale limestone dust (macro `quarry` -0.25 -> paler, no grass mix: tufts only).

**The mountain: Larkspur Bench (alpine), Calder's Post (outpost), the Diggings (mine), the crest.** Samey: the same
Pine_A/B stands (benchPines*), Juniper_A, the valley's rocks. Identity: cooler, greener, blue larkspur, granite.
- Bench stands: Pine_C/Pine_E, Pine_D on the crest and skyline (bleached NeedlesPineOld). Swap (weights).
- Crest: Juniper_C columnar skyline junipers, Boulder_E erratics; granite (MI_RockGranite_Ransom) instead of sandstone
  where the outcrops are granite already. Swap.
- Diggings: coal-black spoil (macro `mine` -0.2 -> dark grey), Stump_A felled stumps round the claims (timbering), rusted
  ore carts. Swap; terrain macro.
- Calder's Post: a cared-for ranger's yard: split-rail FenceRail_B, woodpile, a flag. Swap.

**The boundary ridges and the bluffs (boundaryFoot, eastRidge, hogback, escarpment...).** Samey: long runs of 4 faces.
- Twins alternated: a run of 12 m faces becomes A, A2, A, A2... with the existing mirror/sink/turn variation; together
  that is 8 shapes x 2 mirrors per wall. Swap. Done.
- Cliff life: LipDrape_C dominates (53); give drapes per area (green drapes in the valley, Juniper_D on dry lips). Mix.

## 3. Cross-cutting levers (biggest win first)

1. **Area colour through the ground (terrain macro, zero runtime cost).** Extend `macro.zones` from moisture to a tone
   per zone kind (hue/value shift in `area_macro.py`/`area_mosaic.py`), and `macro.blooms` per kind: town red-ochre dust,
   travel cinder grey, pit ash grey, quarry pale, mine coal, chapel green + white yarrow, alpine blue larkspur (exists),
   fields stubble stripes. One generator run (`RansomsRest.py -- --macro`), re-import, `terrain_compare` (heights
   unchanged). Cost: ~35 min Blender, 6.8 GB; no fps cost.
2. **Variants by swap, never by addition.** Every variant replaces instances of its original (weights split), so the
   instance count and the cost per instance stay; only a few draws per view are added (one ISM per mesh per cell).
3. **Per-zone vegetation sets.** The scatter's spawns take a zone filter (it has one for pines): bushes, junipers and
   rocks get a set per zone kind (see each area). A view sees 1-2 variant meshes, not all of them.
4. **Material instances per area** (`level.materials`/`level.swaps`, exists): LeavesRusset and LeavesDusty pushed past
   a hex tint (Tint 2.4, 1.2, 0.8 like the amber orchard) so the sumac reads red; NeedlesPineOld bleached; the Sink's
   granite grey (exists). No new shaders.
5. **Clutter vignettes outside town** (the town.json pattern for the farm, the yard, the quarry, Calder's Post): small,
   instanced, culled at 40-60 m.
6. **Decals** for trampled paths, wheel ruts, ash, coal and web patches (MI_CliffStain pattern, fade by screen size).
   Budget: under 40 per view.

Perf guard: `Tools\perf.ps1 -Label "world variety" -Exec "Looter.Quality Medium"` before and after; `tour.ps1` for every
view; keep the worst view under 8.3 ms. Expected: +0.05-0.2 ms from the extra ISM components and decals; 0 from cliffs
(actors swapped 1:1), materials (same masters) and the macro.

## 4. Phase 1 done (tonight; no import, no layout or build-script edits)

Each in its original's script, appended after the originals (originals' FBX geometry byte-identical: only time stamps
and FBX object ids differ, checked with an export before and after). Previews: `Saved/ArtPreviews/<Category>/<Name>.png`.

| File | New models | Notes |
|---|---|---|
| `Art/Models/Rocks/Cliffs.py` | CliffFace_A2, B2, C2, D2 | twins: same sizes and shared beds, own seeds (columns, joints, notches, talus); 10k Nanite, 25% fallback |
| `Art/Models/Rocks/Rocks.py` | Boulder_D (tabular), Boulder_E (upright erratic), Boulder_F (rounded erratic), Rock_E (flat flag), Rock_F (layered chunk) | 26-40% fallback |
| `Art/Models/Vegetation/Pines.py` | Pine_C (young 6.5 m), Pine_D (old, dead-topped, NeedlesPineOld), Pine_E (tall 15 m bole) | new `bare_top`, `needles` options (defaults keep A/B) |
| `Art/Models/Vegetation/Scrub.py` | Juniper_C (columnar 3.2 m), Juniper_D (creeping 0.8 m) | |
| `Art/Models/Vegetation/Undergrowth.py` | Bush_D (russet sumac, LeavesRusset), Bush_E (dusty serviceberry, LeavesDusty), Stump_B (snapped), Log_B (bleached, BarkWeathered) | |
| `Art/Models/Vegetation/Trees.py` | Apple_B (old, gnarled), Apple_C (young) | LeavesApple, so the amber swap reaches them |
| `Art/Models/Vegetation/DeadTree.py` | DeadTree_B (8.5 m snag), DeadTree_C (squat 4 m) | |
| `Art/Models/Props/Fences.py` | FenceRail_B (three rails), FenceRail_C (sagging), StoneWall_B (coping fallen), StoneWall_C | chain like the originals; new `shift`, `missing`, `fallen` options |
| `Art/Models/Props/Graves.py` | Grave_Headboard_OldE, Grave_Headboard_OldF, Fence_IronSection_Leaning | |

Export tests: `Intermediate/ArtExport_WV_New1`, `ArtExport_WV_New2` (+ `ArtExport_WV_Final*` after the last fixes);
baselines `ArtExport_WV_Base1/2`.

## 5. Phase 2 needs (placement, terrain)

Import (Main): stage only the new names (`Tools\stage_import.ps1`), `Looter.ImportModels`, check bounds/hulls; new MIs
LeavesRusset, LeavesDusty, NeedlesPineOld, BarkWeathered come with the import. Add bestiary? No (not creatures).

Build scripts (after the fight agent is done with them):
- `build_area.py`: `CLIFF_PIECES`/`CLIFF_FRONT` learn the twins. Safest: after `varied_piece` (and the plain picker)
  choose a mesh, swap it to its twin on a hash of (group, i, k) (no new rnd draws, so every placement, sink, stretch,
  turn stays); CLIFF_FRONT for a twin = its original's (same front plane), checked with `Saved/cliff_cover.py`. Rerun
  `cliffs` mode, `cliff_in_buildings.py`, `path_probe.py`, walkways. Orchard: pick Apple_A/B/C per tree by a hash
  (`build_area.py` ~line 622).
- `build_area_dressing.py`: FOOTPRINT entries for every new mesh (sizes from the export log); fence kits take a list of
  sections with weights (`'section': ['FenceRail', 'FenceRail_B'...]`, never the same twice in a row); stone kit
  alternates StoneWall/_B/_C; grave styles' `mix` gets OldE/OldF; iron kit 1 in 4 Leaning; `churchyardDeadTrees` ->
  DeadTree_A/B/C; `TREES`, `TRUNK_LINES`, crown boxes for the new trees.
- `build_island_scatter.py`: pine entries Pine_A-E with weights per zone (bench/forest/crest), JUNIPER_MESHES +
  Juniper_C (crest), Juniper_D (rims, pits), bushes + Bush_D/E by zone, rocks + Rock_E/F, boulders + Boulder_D/E/F,
  forest floor + Stump_B/Log_B. Then regenerate, wait, save.
- `build_area_cliff_life.py` / `level.cliffLife`: drapes and crack plants per area (Juniper_D on dry lips).
- `layout.json` (`level.materials`): MI_LeavesRusset_Ransom (Tint ~[2.4, 1.15, 0.8]), MI_LeavesDusty_Ransom,
  MI_NeedlesPineOld tuned by eye.
- Terrain (`area_macro.py`, `area_mosaic.py`, `layout.json` `macro`): tone per zone kind + blooms per kind + hayfieldEast
  rows; generator run, re-import tiles' macro textures, `terrain_compare` (heights unchanged), `ground_preview.py` first.
- Not made yet (next art steps): weathered HayBale, LampPost variants, QuarryFace (sawn cliff piece), farm/yard/quarry
  clutter vignettes, ground decals (ruts, ash, coal, web), windfall apples.
- Checks: `path_probe.py`, `Looter.Bestiary.Entries` unaffected, perf before/after, a single `grab.ps1` per area to judge.

## 6. Phase 2 done (2026-10-22 night, not committed)

- Imported: the 27 variants (Final1/Final2 `_Import`) and Trees (apple fix: apples show at 8 and 15 m at LOD0 and LOD1,
  no LOD change needed). New MIs LeavesRusset/Dusty/NeedlesPineOld/BarkWeathered; area MIs MI_LeavesRusset_Ransom,
  MI_LeavesDusty_Ransom (layout `level.materials`).
- Level (census after): cliffs A 86 / A2 88, B 57/57, C 22/27, D 78/72; orchard Apple A 10, B 7, C 7; fences FenceRail 80,
  B 48 (pens, Whitlock yard), C 19 (Sink); iron 49 + 6 leaning; walls 14/3/5; churchyard dead trees A/B/C; boards OldE 4,
  OldF 3; scatter pines A 39, B 36, C 35, D 17, E 22; junipers A 52, B 81, C 75, D 45; bushes A 80, B 76, C 79, D 49,
  E 26; stumps/logs A 23/16, B 12/11; rocks E 6, F 9; boulders D 5.
- Ground: `macro.tones` + blooms (chapel, fields) + zones (farm, chapel greener, Boot Hill drier); macro re-painted
  (`--macro`, 46 min) and imported by a materials-only manifest; scatter mask re-imported; grass 62,732 instances (was
  64,557: thinned on the trampled and dusty places).
- Coffin Rock: a fallen slab in 3 pieces + hidden walk plank; walked in PIE from the ground to the top in about 6 s.
- Checks: path_probe 936 spots on 19 roads and ramps, nothing flagged. Perf RR spawn Medium: before 7.7 ms (p95 8.2,
  GPU 6.4, 1441 draws), after 7.8/7.7 on two runs (p95 8.4/8.3, GPU 6.5/6.4, 1705 draws). Tour: worst view 8.11 ->
  8.11 ms, views within -0.23..+0.20 ms, about +200 draws each (the new meshes' scatter components and their shadows).
- Shots: `Saved/World/before/`, `Saved/World/after/` (tour, every view), `compare_valley.png`, `compare_places.png`.
- Seen: the ground tones read faintly in the tour's wide shots (the town's street was dust already); the clearest wins
  are the stands (pines, junipers), the orchard, the cliffs' twins up close. Next if wanted: stronger tones (mix 0.7+),
  per-area cliff drapes, the clutter vignettes, decals; trim draws by dropping the rarest scatter variants if needed.
