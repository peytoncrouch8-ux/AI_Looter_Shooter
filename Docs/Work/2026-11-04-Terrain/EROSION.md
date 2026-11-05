# Erosion for Ransom's Rest (TERRAIN lane, 2026-11-04)

The user's calls: the terrain "reads as a generator" (2026-11-04 grill round), "Erosion pass first" on "Ransom's Rest only";
2026-10-30: hydraulic + thermal erosion in `area_shape.py`, our own numpy, sparing pads/roads/exact features, flow and
sediment maps feeding the macro colours. Audit `Docs/Work/2026-11-03-ArtAudit/AUDIT.md` finding 8, plan row 10. Built
offline: no editor, no level rebuild, nothing committed. Main runs the rebuild (below).

## What it is
- `Art/Levels/area_erosion.py`: the last step of `Area.build()` (after the features, `boundary.foot`, the seam band). An
  area without a layout `erosion` block, or `"enabled": false`, builds exactly as before (TutorialIsland, TerrainTest have
  none). Deterministic (every draw from `erosion.seed`). Adds 26-45 s to the shape step (masks 13-23 s, 2 m pass 5-8 s,
  1 m pass 6-13 s; the shape step itself 4.5 min on a quiet machine, ~10 min beside other runs).
- `Art/Levels/area_erosion_masks.py`: the masks and the protection weight.
- `Art/Levels/area_erosion_maps.py`: the towns' flow and low-point maps (`--computed`).
- `area_shape.py`: calls it; keeps `foot_lift` (what `boundary.foot` raised, for the masks; only with an erosion block);
  roads with `"paint": true` or `"kind": "track"` (`paint_only()`) are left out of the shape (Towns plan ask 5: paint-only
  wheel tracks, no carving; not in `area.roads`, so not in the macro's road paint or `layout_computed.json` `roads`;
  Unreal scripts reading layout `roads` directly (dressing roadside, MapRoads, path_probe) still see them: tell the
  towns lanes).
- `area_mesh.py`: the erosion mesh pass (`erosion.mesh`, ON since the user's call, see Mesh). `area_macro.py`: the erosion paint
  (`erosion.paint`) and the ridge faces kept painted on eroded ground. `area_model.py`: writes the maps with `--computed`.

## Method (one pass; passes run coarse to fine, each from the ground the one before left)
1. Roughness: seeded 3-octave noise (pass `roughness`: 60 cm at 25 m on the 2 m pass) added where erosion may work: the
   generated faces are smooth extruded profiles, and on them every method tried combed even parallel rills or wore the
   face down evenly; the noise gives the water hollows of several sizes to gather in.
2. Every iteration: rain (seeded noise, `rain`) drains by stochastic D8 (one lower neighbour, picked with weight
   slope^spread; over the iterations it spreads like multiple-flow routing). A (m2 of rain gathered) by doubling along the
   receivers, the tree depth by binary lifting (no loop over cells).
3. Incision: stream power solved implicitly up each receiver tree (Braun and Willett 2013), stable at any strength:
   F = incision x erodibility x A'^areaExponent / L, A' = (A - threshold) eased past `saturate` m2 (1500: a bench's runoff
   pouring onto a face cut 5 m trenches along the masks' edges without it); rock (steeper than `thermal.rock`) at
   `rockWear`.
4. Deposition: eroded soil routed down the tree; a cell keeps `settle` x what it gets past capacity transport x A x S
   (fans, aprons, pits filling).
5. Creep (diffusion, m2/iteration) and thermal (ground over `repose` 42 deg slumps rate 0.05 x half its excess; over
   `rock` 58 deg holds).
6. Per iteration at most `maxStep` cm, per pass at most its maxCut/maxFill.
Then `hold`: the change's broad part (its blur over 20 m) is clipped to +-2 m, the rest kept: the faces keep the
profiles and skylines the level was approved with (unclipped, the 2 m pass lowers whole faces 5 m: it hit maxCut
everywhere). The total is clipped to maxCut 6 m / maxFill 2.5 m, resampled onto the 12.5 cm raster (bilinear, after a
one-cell blur) and applied as h' = h + w dh: w exactly 0 inside every mask (bit for bit), easing to 1 over `feather` 6 m
(chamfer distance on 0.5 m blocks), times the seam band's fade (the core still meets the ring exactly).

"At the raster's resolution": the change lands on every 12.5 cm cell, but is solved on 2 m and 1 m cells. Finer gullies
cannot reach the mesh: past the boundary its vertices are 2.4-4.8 m apart at a 1.2 m height tolerance (Mesh). Tried and
dropped: one 50 cm pass (1-cell rills, D8 staircases), transport-limited explicit (unstable), droplets (Beyer 2015; wore
faces evenly: every fresh drop erodes where it falls), flow carving from droplet counts (same).

## Parameters (layout.json `erosion`; defaults in `area_erosion.DEFAULTS`)
- `enabled` true, `seed` 1, `strength` 1.0 (scales incision).
- `passes`: [2 m: 150 iterations, maxCut 500, roughness 60 cm / 25 m, rain wavelength 30 m variation 0.5, incision
  0.0003, areaExponent 1.2, spread 1.5, creep 0.02, maxStep 10] then [1 m: 60 iterations, maxCut 150, incision 0.0001,
  creep 0.03]. A pass's keys over the block's: cell, iterations, maxCut, maxFill, roughness, rain, hydraulic, thermal,
  strength. Cells must nest in the raster (4096 cells: 2 m = 256, 1 m = 512) and in the last pass's.
- `hydraulic`: incision 0.0002, areaExponent 1.0, threshold 0, saturate 1500, transport 0.03, transportExponent 1.0,
  settle 0.3, spread 2.0, creep 0.01, maxStep 4, rockWear 0.35. `thermal`: repose 42, rock 58, rate 0.05.
  `rain`: amount 1, variation 0.4, wavelength 70 m.
- `hold` {reach 2000, keep 200} cm; `maxCut` 600, `maxFill` 250, `feather` 600 cm.
- `masks` margins (cm, area_erosion.DEFAULTS): roads 150, ramps 1200, steps 1400, trails 150, footprints 300,
  placements 500, town 250, obstacles 250, knobs 150, zones 300, encounters 300, spots 600, water 800, designed 200,
  gullies 300, bands 400, lip 800, boundaryOut 800, boundaryIn 600, holes 500, towns 400, townSkirt 3000.
- `townZones` [mainStreet, depot]; `keep`: the six candidate lots and five back-lot boxes (Towns plan 4.4; ids lotFeedStore,
  lotSmithy, lotBackLaneNorth, lotSouthBack, lotPenLaneEnd, lotWestEnd, backPenLane, backWorkshopYard, backDugout,
  backAmosHouse, backCoalStage); the towns lanes send more as layout patches (TERRAIN owns layout.json this round).
- `paint` true; `mesh` {enabled true, cut 30, spacing 240, detail 1.0}, with layout `mesh.triangles` 190000 -> 222000;
  `maps` {folder erosion, windows town X -3200..2400
  Y -1600..14800, farm X -5600..-1800 Y -7400..-4400, whitlock X -7800..-5600 Y 2800..4800}.
- Note: `layoutSha1` changes (erosion is a read block): the rebuild's `--computed` rewrites layout_computed.json.

## Masks (each from the area's own data; `area_erosion_masks.py` docstring has the rules)
roads + shoulders (`area.road_gap` < widest shoulder 2.2 m + 1.5 m), paint-only tracks, ramps (+12 m: their cut, fill and
cliff walls), the Goat Steps (+14 m), trails, pads with blends and banks (`pad_weight`), building footprints, every
placement (half its BUILDINGS diagonal + 5 m) and level.props, town.json (pieces, deferred pieces, picket lines, no-tree
boxes, spots), obstacles (fences, walls, graves, cairns, biers, ruins, props, outcrops, boulder groups; tree stands only
inside the boundary: past it they are the ridges' woods, which the scatter re-traces), knobs' seats, every zone but forest
and alpine (`FREE_ZONES`), the encounters (story fights, camps, patrols, ambushes, fights, lurks, keepOut; frame "mine"
ones are inside the mine), roosts, boards, finds, the foreman's stone, the mine's mouth, openGround, mesh.holes and
boundary.interiors, water (wet cells, pond shores, creek banks and valleys, gorges, +8 m), designed steep ground
(`area.designed`, plateau and pit rims, +2 m), the Dry Wash, the regional ridges' cliff bands (from 4 m out past the foot
to 4 m past the band top: the cliff kit's points stay where they are), the escarpment lip (8 m), the boundary line
(-8 m .. +6 m: the 44-degree rise rule `terrain_check.py` holds), and wherever `boundary.foot` lifted (+2 m).
Coverage: 47.0% of the core kept exactly (roads 4.6%, zones 20.3%, encounters 19.3%, obstacles 10.2%, towns 11.4%, bands
13.2%, boundary 9.6% ...; the log prints them). The town's bench: read as the town's flat ground (mainStreet and depot
zones + 30 m skirt, the town pads hotelPad/workshopPad/whitlockYardPad, the lots); no layout feature is named that.

## What changes (measured on the final heights, out of the shape snapshot)
- Inside the playable boundary (62,000 m2): 2,600 m2 moved over 5 cm, all on Larkspur Bench (the alpine meadow 1,676 m2,
  the bench pine stands 977 m2), cut at most 1.12 m, fill 0.08 m; every zone of play, lot, pad, road, yard: 0.0000 cm
  (checked per polygon: mainStreet, depot, farm, chapel, bootHill, sink, whitlock, ransomsPoint, gravewindPoint,
  calderPost, diggings, oldQuarry, the 11 keep polygons, the 3 town pads, town.json's 4 yards). Ground steeper than 44 deg
  inside the boundary: 3.09% before and after.
- Past the boundary inside the core (125,000 m2): 66% moved over 5 cm, 53% over 50 cm; median -1.23 m, p1 -3.61 m, p99
  +1.81 m, extremes -5.02 / +2.50 m. Faces: Larkspur Ridge cut to 4.4 m, East Ridge 3.6 m, the Hogback 3.0 m, Larkspur
  High's foot 1.1 m. Slope on moved ground 29.5 -> 29.8 deg mean, >45 deg 18.9% -> 17.6%, >58 deg 3.0% -> 3.4%.
- Fans mostly can't form at the ridges' feet: the bands and the boundary strip are kept, so soil reaching them leaves the
  solve; the existing cliff-life scree fans (meshes) stand there. They do form on free gentle ground (the Bench).
- The read: renders show gully furrows on the Hogback's long face and East Ridge's, notched band tops and softened
  ledges; modest at the game's mesh density (Mesh). The macro map adds darker stony beds where the change sank below its
  6 m mean or the water gathered, paler grit on fans; 0 where the masks kept the ground (no sediment tint on the town or
  the farmyard: Towns ask 3).
- Bit-identity: the pre-erosion shape built with the working tree's code equals HEAD's code's to the bit (snapshots of
  `Area.build()` before and after the edits); after erosion all 10,456,750 kept cells equal it to the bit (float64 and the
  float32 `area.h`). Confirmed end to end: the generator's own run (`Tools/Blender/terrain_dump.py`, the working tree,
  `--computed`, into the scratchpad: 489 s, peak 4.3 GB, erosion 26 s, every check PASS: seam gap 0.000 mm, core 183,478
  of 190,000, ring 37,786, canyon wall 10,850, backdrop 5,996) gives the same heights, kept cells equal to HEAD's. Again
  with the mesh pass on (836 s beside the editor; core 215,536 of 222,000 PASS, seam PASS): heights the same, the 24
  towns areas 0.000000 cm, the erosion maps byte for byte the ones in the repository; layout_computed.json differs from
  the no-pass run only in farTrees (seated on the new mesh) and layoutSha1.
- Its layout_computed.json against the repository's: placements, footprints, roads, ramps, ponds, creeks, gullies,
  waterfalls, bridge, boundary, region, heightRange, orchardRows identical; cliffs identical but 2 of the 129
  boundaryFoot points (one wall's top 2.1 cm lower, one tall wall's upper course feet 13-38 cm along it: the rock past
  closed edges measures its wall out past the 8 m kept strip); farTrees re-seated (65 stand inside the core square, 7 of
  them on ground erosion moved over 20 cm, the worst 2.79 m: rebuild `beyond`); openGround overTarget 1.35% -> 1.25%;
  scrub moved; layoutSha1 new.

## Mesh (the audit's "the 12.5 cm raster never reaches the mesh")
- HEAD: 183,480 top triangles at a 121.1 cm height tolerance (bisection to mesh.triangles 190,000): inside the boundary
  ~57.7k points at 1.2 m (the playable cap), past it ~33.3k at mostly 4.8 m. Erosion alone: 183,478 triangles, tolerance
  120.4 cm: the mesh barely changes and sub-meter erosion relief past the boundary is smoothed away.
- The pass (`erosion.mesh`): where erosion moved the ground over `cut` cm (grown a cell), vertices at most `spacing`
  apart and the tolerance x `detail`. Costs at the HEAD tolerance: cut 15 cm +32.5k triangles (51% of the core), 30 cm
  +31.8k (48%), 50 cm +30.6k (44%), 80 cm +28.0k (39%). Inside 190k it can't fit: the bisection runs to its 4 m cap and
  still makes 203,314 triangles (and every crease elsewhere coarsens).
- ON (the user, 2026-11-04: "Mesh pass on", measured on the tour before it is kept): cut 30 cm, spacing 2.4 m, detail 1,
  layout `mesh.triangles` 222000. The bisection by budget: 216k -> 209,636 triangles at 178.4 cm, 218k -> 211,548 at
  152.0 cm, 220k -> 213,574 at 133.6 cm, 222k -> 215,536 at 119.3 cm (HEAD's 121.1 cm: the rest keeps its density).
  Final: 215,536 top triangles, +32,056 (+17.5%) over HEAD's 183,480, all of them on the eroded ground past the
  boundary. Perf: +32k triangles on the Nanite fallback (Medium), expected well under 0.1 ms GPU; unmeasured, and the
  audit put the heaviest view's margin at 0.07 ms: `perf.ps1` and `tour.ps1` decide whether it stays (off again:
  `erosion.mesh.enabled` false and `mesh.triangles` 190000).

## The towns' maps (Towns plan section 6, asks 1-7)
1. Heights frozen over the town, yards, pads and the six lots: masks above, measured 0.0000 cm (target was <= 2 cm).
2. `Art/Levels/RansomsRest/erosion/`: flow_<id>.png and lows_<id>.png (16-bit grey, north up, row 0 the window's north
   edge, one 12.5 cm cell a pixel) and maps.json (covers, size [columns, rows], encodings: flow = 65535 x clip((log10 A +
   2) / 8, 0, 1), lows = mm of standing water before the hollow spills). Priority flood (Barnes 2014) over the window and
   30 m round it; written with `--computed` (and already written from these heights: town 1313 x 449 px, 95,236 cells
   over 1 cm, deepest 46.9 cm; farm 241 x 305, 5,247, 19.7 cm; whitlock 161 x 177, 980, 2.2 cm). Caveats: on exactly flat
   pads and footprints the flood's order routes the water in rings and stripes (flow there means little); carved road
   beds (12 cm under their shoulders) hold water along their length until they spill. Preview:
   `Saved/ArtPreviews/Erosion/towns_maps/`. 16-bit PNG, not .npy: every binary goes through LFS, and .png is tracked.
3. Sediment colour 0 on the town and lots (and the farmyard: the farm zone is kept).
4. Pads for the new lots: send `patches/towns_lots.json`; they go in the same rebuild (the lots are already kept).
5. Paint-only tracks: `"paint": true` or `"kind": "track"` on a layout road (`area_shape.paint_only`: the shape skips
   it; erosion keeps its ground; area_places.py paints it).
6. One owner per file: TERRAIN owns layout.json this round; area_shape, area_macro, the scatter mask TERRAIN's.
7. No gullies across Pen Lane, the fields path, the yard road or the north road within 30 m of the town: townSkirt 30 m
   round mainStreet and depot, roads kept; Main Street's edges inside its zone: unchanged.

## Rebuild (Main, in a window; the editor open for steps 2-4). READY.
0. Before: a perf baseline with the editor closed, `Tools\perf.ps1 -Label "before erosion" -Exec "Looter.Quality Medium"`, and
   `Tools\tour.ps1` (the rule: perf before and after anything that could change cost; the mesh pass adds 32k triangles).
1. `Tools\artrun.ps1 -Script Art\Models\Terrain\RansomsRest.py -ScriptArgs '--macro','--computed'` (two quoted strings:
   one '--macro,--computed' string reaches argparse as one unknown token and the run silently builds meshes only), or
   `blender -b --factory-startup --python Art/Models/Terrain/RansomsRest.py -- --macro --computed`. Expected 35-45 min,
   ~7 GB: the shape 4.5-8 min, erosion 26-45 s, the mesh, AO, ring and computed ~6 min, the macro's four windows and the
   scatter mask the rest (~25-30 min). Writes Art/Textures/RansomsRestMacro{SW,SE,NW,NE}, the scatter mask,
   layout_computed.json, Art/Levels/RansomsRest/erosion/ (identical to what is there now). Log lines to read: `erosion:
   masks ... all 47.0%`, `pass 200 cm ... cut 495 cm, fill 242 cm`, `pass 100 cm ... cut 126 cm, fill 216 cm`, `erosion:
   done ... cut up to 502 cm, fill up to 250 cm; 45.9% of the core moved over 5 cm`, `top surface, height tolerance 119.3
   cm`, `top surface 108774 vertices, 215536 triangles`, `check: core 215536 triangles of 222000 - PASS`, seam gap 0.000 mm
   PASS, `macro: wrote` x4, `erosion maps: wrote 3 windows`.
2. `Tools\models.ps1 -Only RansomsRest` (re-runs the wrapper without flags, the shape with erosion and the meshes: ~8-10 min;
   then exports and imports the tiles, ring, canyon wall, backdrop, water and the macro textures: ~10 min).
3. Editor: `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest beyond"` (~1-2 min: ring,
   backdrop, far trees from the new farTrees: 7 old ones would float or sink by up to 2.8 m).
4. Editor: `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_scatter.py RansomsRest"` (re-imports
   the scatter mask by MD5, regenerates), wait for the instance count to settle, save (~3-5 min).
5. No `cliffs` rebuild (2 boundaryFoot points within 2 cm); no dressing, gameplay or loot rerun (every kept cell is bit
   for bit HEAD's).
Total ~60-70 min of machine time (measured here: the generator's `--computed` run without the macro 8 min 9 s on a quiet
machine, 13 min 56 s beside the editor and another Blender run; the macro and the import are the Handoff's figures).

## What Main must check after
- `Tools\terrain_check.ps1 RansomsRest` (layoutSha1, the boundary's 44-degree rise, courses).
- `Tools\terrain_compare.ps1 -Area RansomsRest -Except Saved\ArtPreviews\Erosion\towns_except.json`: every listed area
  0.00 cm. Its overall verdict FAILs on the 2,600 m2 of Larkspur Bench inside the boundary (expected; the Bench meadow is
  free by design: add `alpine` to the kept kinds if the user wants it frozen). NOT RUN HERE: `Saved\ArtPause.flag`
  (dated 16:33, no editor or game running) held it in its wait. I ran Blender renders and numpy erosion runs while that
  flag existed: if a measurement was meant to be running then, it may be off.
- Diff the new layout_computed.json against HEAD's (expected as above: all identical but 2 boundaryFoot points, farTrees,
  openGround, scrub, layoutSha1).
- After the commit: `Tools\terrain_identity.ps1 -Area TutorialIsland` and `-Area TerrainTest` (byte for byte: no erosion
  block there).
- `Tools/Unreal/path_probe.py RansomsRest`; `Tools\tour.ps1` + `Tools\perf.ps1` (RidgeFoot, PointOverValley, ValleyNorth,
  NorthBoundary, LiftHeadBench, WatchtowerValley, Aerial, the heaviest views) against step 0: the terrain +32,056
  triangles, the macro and scatter changed; the mesh pass stays only if every view keeps 8.3 ms on Medium (else
  `erosion.mesh.enabled` false, `mesh.triangles` 190000, steps 1-2 again);
  `Looter.World.RansomsRest.InsideBoundary`, `Looter.UI.Minimap`, `Looter.Encounters.Camps`.
- In the editor: the ridge faces from the valley and the Bench (gullies, the band tops), the cliff kit's tops against
  lowered ground right behind the bands (the bands' strip keeps 4 m), far trees and PCG pines on the faces re-seated.

## Open, for the user
- Decided (the user, 2026-11-04): the mesh pass on (kept if the tour measures fine); Larkspur Bench's meadow erodes as
  built (minor); the strength as is. Later, after the user sees it: a stronger pass (hold keep 2 m -> more, the 2 m
  pass's incision).

## Previews (`Saved/ArtPreviews/Erosion/`)
- `maps/`: core_shade_before_after.png and core_slope_before_after.png (0.5 m px, north up, west the canyon),
  core_change.png (blue cut, orange fill, the kept ground green), core_masks.png, core_flow.png; crops larkspur, eastridge,
  hogback, high, bench as _sun (247.5 deg, 25 deg), _shade (NW), _south (light from 160 deg), _slope.
- `renders_before_after/` (Eevee through the generator's own mesh, ring and preview code, tour views and four over-the-
  valley views, textured and clay): before | after (as shipped: the mesh pass on, 222k). `renders_meshpass/`: after
  without the pass | after with it.
  `renders_raw/`. The macro map in the renders is HEAD's (no macro repaint offline).
- `macro_erosion_layer_hogback.png` (the paint alone on a 128 m window), `towns_maps/`, `towns_except.json`.
- Stray: my first two render batches went to `C:\render_before` and `C:\render_m` (a relative output path); nothing uses
  them: the user removes them.
