# Pipeline: Making levels

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## Making a level (grounded areas)

- The next new level (the Gilded Lily) starts as a World Partition map (the user, 2026-10-14), so the engine's HLOD
  (simplified stand-ins for far groups: the town, cliff belts) comes with it. Ransom's Rest stays a plain map: converting it
  means one file per actor and rechecking every build script, the minimap, tours and tests (1-2 days, risky), and HLOD would
  win only ~0.3-0.6 ms in its wide views (hiding every building saved 0.1-0.2 ms, every cliff 0.4-0.5). Check the build
  scripts (`build_area*.py`), `open_clean.py`, the minimap bake and `Looter.Perf.HideFolder` against World Partition first.
- Data: `Art/Levels/<Area>/layout.json` (hand-written: features, roads, zones, obstacles, placements, the `level` block, gameplay).
  `views.json` holds the tour viewpoints.
- Generator (Blender, headless): the `Art/Levels/area_*.py` modules build the terrain tiles, ring, canyon wall and backdrop, and
  write the macro, scatter and scrub masks and `layout_computed.json` (cliff points, far trees, scrub points, boundary).
  - `Tools\terrain_identity.ps1 -Area <Area>` checks the generator still rebuilds an area byte for byte. Run it after any generator
    change, once the change is committed (it compares with HEAD).
  - `Tools\terrain_compare.ps1 -Area <Area> [-Except <json>]` proves a grown or edited area kept its old ground: heights, AO and
    curvature inside HEAD's boundary, outside the `-Except` areas meant to change (Ransom's Rest's mountain: max 0.82 cm).
  - Erosion (2026-11-04, Ransom's Rest only; `Docs/Work/2026-11-04-Terrain/EROSION.md`): layout `erosion` runs
    `area_erosion.py` last in `Area.build()` (+~45 s): 2 m then 1 m passes of stream-power gullies, routed fans, creep and
    talus on the free ground, the broad change held to +-2 m; every mask (`area_erosion_masks.py`: roads, paths, pads,
    buildings, the town, obstacles, play zones, encounters, water, cliffs, the ridges' bands, the lip, the boundary line,
    `keep` lots) keeps its heights bit for bit (47% of the core; inside the boundary only Larkspur Bench's meadow moves).
    `erosion.enabled` false = the old shape. The macro paints gully beds and fans (`erosion.paint`); `--computed` writes
    the towns' flow and low-point maps into `Art/Levels/RansomsRest/erosion/`. Past the boundary the mesh's 4.8 m
    vertices smoothed it away: `erosion.mesh` (on, the user's call) keeps the eroded ground's vertices within 2.4 m, so
    `mesh.triangles` is 222000 (215,536 made, +32k, the rest at its old 1.2 m tolerance; off again if the tour says). Layout roads with
    `"paint": true` or `"kind": "track"` are paint-only tracks (no carving).
  - Run: `blender -b --factory-startup --python Art/Models/Terrain/<Area>.py -- --macro --computed` (~35 min at 512 m, 6.8 GB;
    `--preview` adds the look-sheet renders), then `Tools\models.ps1 -Only <Area>`.
  - Fitting terrain to art (the mountain, 2026-10-13): `pad` with `exact` (hard edge and flat top where a model's floor must meet
    the ground); `mesh.holes` (open the ground over a stair or an adit's mouth); `boundary.interiors` (splice an interior's tongue
    into the play area); closed-boundary stretches where no rock dressing may stand; `map.grownFrom` (grow the square without
    changing the old ground); `map.macroSplit` (four 4096 macro windows).
  - After re-gridding the core (a grown square): delete the old tile assets the new export doesn't overwrite (`SM_<Area>_Tile_*`)
    and their actors; `build_area.py` places every tile mesh it finds.
- In the editor: `Tools/Unreal/build_area.py <Area> [mode]`; no mode = full build. Modes:
  - `gameplay`: story pieces and creatures (`build_area_story.py` and its `build_area_<place>.py` scripts);
  - `environment`: light, sky, fog;
  - `beyond`: ring, backdrop, far trees;
  - `cliffs`: cliff faces and outcrops;
  - `dressing`: fences, walls, graves, props (`build_area_dressing.py`); the town's street lamps and porch lanterns
    (`town.json` `lights`, with their `gloom` candelas for the smog) are placed here.
  - `props`: `level.props` alone (the town gate, and the towns plan's `RR_Home_B` / `RR_Home_I` as `homeB` / `homeI`; a prop
    may carry `"lift"` cm over the ground at its pivot) with their lights, chimney smoke and households, as the full build
    places them; the grass scatter, `build_area_loot.py` and `build_area_fauna.py` read the buildings: run them after.
    `level.fires` (`{"house": <prop id or placement key>, "socket": "Fire", "byDay": true}`: the smith's forge, the bathhouse
    boiler) puts an `ATownFire` (`Fire_<house>`, Effects) at the house's `SOCKET_Fire` (`build_area.town_fires()`: the props mode and
    the full build's effects).
  - `lights`: the lived-in houses' lights alone (`level.lights`: lamp, reach, the windows' `gloomGlow` for the smog;
    `house_lights()`, otherwise part of the full build). Takes out the `Lights_<key>` actors, places them again and sets their
    households again (`build_area_townlife.py`'s `set_households`); `-Until "lights placed and saved"`.
- Cliffs (`build_area.py` cliffs()). The `level.cliffs` settings:
  - `varied` groups are mirrored, sunk and turned. `lean` groups lean with the wall; `leanTop` and `leanProud` can be set per group.
    The lip ceiling keeps tops under a rounded rim.
  - `abut` stretches a run's end into a rock, at most 2x its width.
  - `under` caps cliff tops under a platform (`build_area_platforms.py`; the burial deck).
  - `panels` is a run of narrow panels along a pit's curve (`build_area_panels.py`), with `talus` at their feet
    (`build_area_talus.py`).
  - `walled` (groups, or true): a point gets no faces unless the terrain's wall stands within 2.5 m behind it (2026-10-14:
    an `onFloor` ridge's band ran on past its floor, and 31 East Shoulder slabs stood on the valley floor, through the
    depot and across the rail line). `keepOut` (placement keys): no cliff piece, panel or outcrop stands inside those
    buildings (`build_area_keepout.py`: moved back into its wall, cut under a floor, slid along its run, else left out;
    each logged). Check with `Tools/Unreal/cliff_in_buildings.py <Area>` (read-only, `CLIFFIN` lines). Since 2026-10-14
    the generator writes an onFloor ridge's band only where its floors stand under its top (`Region.band_height` x
    `floor_weight`: eastShoulder 32 -> 1 point); `walled` stays as a guard.
  - Every ramp's walkway is cleared of cliff pieces (`build_area_walkways.py`): a player capsule is tested down its middle. A ramp's
    own wall moves back into its cut; a ledge's outer face (under the path) sinks until it clears, at most 1.5 m (2026-10-08: nine
    faces on the bluff path and the Sink ramp sank 25-100 cm, no gaps under the lip).
  - The review's rules (2026-10-14, the user: "fix any discrepancies or odd shapes"): no gap isolates a piece and no piece stands
    alone (`LONE_REACH` 15 m: its neighbours all left out, it goes too); no piece beside its own mirror (`VARY_SWAP`: the second of
    two identical takes another fitting kit piece); `level.cliffs.overlap` (cm, all or per group) widens a run's pieces;
    `level.cliffs.outcrops` {knob: {piece, height, yaw}} gives a knob another outcrop (benchGraniteH wears the narrow Slab: the
    reworked TorD stood on the Bench trail). Probe: `Saved/cliff_cover.py` (bare and buried shares per run). Photograph runs with
    `Tools/cliff_views.py` + `tour.ps1`.
  - The steep terrain reads as the kit's sandstone (M_Terrain's `SteepRock`, `SteepBeds`, `SteepAOLift`, `SteepSharp/Breakup/Pull`,
    `StrataSmooth/Flat`, all off by default; Ransom's Rest's four core `*_Steep` MIs turn them on; keep `SteepRockTint` equal to
    MI_RockCliff_Ransom's Tint). Preview without Unreal: `ground_preview.py walls`.
  - Cliff life (`build_area_cliff_life.py`, in the full build after gameplay and in the `cliffs` mode; `level.cliffLife`): drapes on
    lips, ledge plants, crack junipers, scree fans and fallen slabs at feet, the swallows' nests (SM_SwallowNests) under overhangs,
    stain decals (`MI_CliffStain`, the cell picked by the decal colour's R) on faces seen from the play area; caps per kind (RR: 285
    instances, 70 stains). `footprints()` keeps the scatter's grass off the fans and slabs.
- Scatter (PCG): `Tools/Unreal/build_island_scatter.py <Area>`.
  - Re-imports changed masks by MD5, then generates. Wait for the instance count to settle, then save.
  - Zones of kinds alpine, outpost, mine and crest, and `benchPines*` stands, grow pines only (PCG Intersection: one `In` pin,
    density MINIMUM or MULTIPLY; there is no BINARY), stands and lone meadow trees alike; the boxes reach 5 m past the zones
    (`PINE_ZONE_MARGIN`, over the mask's 4 m forest fade: a lone oak stood on the Bench until 2026-10-14).
  - Keeps off the dressing's footprints and wears the area's material swaps. Run `dressing` before the scatter when fences or graves
    move.
  - Shrubs (sagebrush, big and pit sage, rabbitbrush, bushes; not the junipers) are tagged `ShadowHighOnly` (`entry()`'s
    `medium_shadow=False`, 2026-11-04, the user's pick): their shadows are High and Epic's only once the game switches tagged
    components off on Low/Medium (the code: `Docs/Work/2026-11-04-Art/LODS.md`; not built yet).
- Materials per area: `level.materials` makes area instances of shared MIs; `level.swaps` puts them on everything the build placed,
  and on the scatter; `level.swapsOn` swaps on named actors only (the windmill's steel).
- World variety (2026-10-22; the user: RR "looks samey", anything allowed; measured and planned in `Saved/World/PLAN.md`): one nature
  kit everywhere was the sameness (4 cliff faces for every wall, 204 of them in one view; 2 pines, 3 bushes, 2 junipers, one apple,
  one fence rail, one iron section). Rule: a variant replaces its original's instances (weights split, places kept), never adds.
  - Cliffs: `CLIFF_TWINS` (Cliffs.py's A2-D2: same sizes and beds, own rock); a piece is picked among the originals exactly as
    before, then stands as its twin when `cliff_twin` (crc32 of group, point, course) says so: half the faces, no placement moved.
  - Orchard: `ORCHARD_APPLES` (A x3, B x2, C) by crc32 of row and tree. Without a full build: `Saved/World/scripts/orchard_swap.py`.
  - Dressing: fence and wall `sections` by weight (kit or line; `section_mesh`, never the same variant twice running): the stock pens
    and the Whitlock yard three-rail (B), the Sink's fences tired (C), the churchyard 1 in 5 leaning, walls B/C mixed; grave mixes
    take OldE/OldF; `SPOTS` `meshes` (one per spot: the churchyard's dead trees A/B/C). Amos's fence stays FenceRail (his pose).
  - Scatter: `VALLEY_PINES`, `MOUNTAIN_PINES` (up the mountain), `BUSHES`, `FLOOR_WOOD`, `ROCKS_MIX`, `BOULDERS_MIX`; junipers
    A-D, crest ones B and C. A variant not imported is left out. Weights must be whole numbers (PCG's int32: 0.6 spawned
    nothing; the broadleaf stands' `DeadTree_A` 0.4 never grew either).
  - Ground per place: layout `macro.tones` {zone kind: color, mix, patch, alpha (stony ground, under 0.5), value, grass (share left)}
    (`area_mosaic._tones`; grass in `grass_density`): town red-ochre dust, travel cinders, pit ash, quarry pale dust, mine coal,
    Boot Hill, the point and the deck paler, the chapel knoll green; `macro.blooms` chapel white yarrow, fields yellow; `macro.zones`
    farm and chapel greener. Bolder since the user's call (2026-10-22 morning): the places' mix 0.7-0.75 with patch 0.3 (the
    first pass's 0.5 / 0.5 read only faintly from the tour's views), the edges' 0.45-0.5, grass left 0.35-0.45 where trodden.
    Run `Art/Models/Terrain/RansomsRest.py -- --macro` (artrun; ~40 min, heights untouched), then import the macro maps without the
    terrain export: a materials-only manifest (`Intermediate/ArtExport_WV_Macro/macro.json`, `"models": []`; the importer updates
    MI_<set> and re-imports a changed texture), then `build_island_scatter.py` (re-imports the scatter mask by MD5).
  - Places at eye level (2026-10-22, the tones read only from above): layout `macro.places` layers (R town dust and ruts:
    Main Street, yardRoad; G Sink ash and cinders; B quarry grit and chips; A coal: Diggings pads, tramway, rail bed,
    depot) over zones, features (pads, hills, pits) and roads (`[weight, reach m]` trails a place out along a road).
    `Art/Levels/area_places.py <Area>` (Blender's Python, 2 s, no terrain run) writes `T_<Area>Places_M` (M_Terrain's
    PlaceMap) and `T_<Area>PlacesScatter_M` (R grass and flowers kept, G/B/A clutter). Then `build_area.py <Area> dressing`
    (makes `MI_PlaceCinder/Chalk/Coal_Ransom`), `build_world_materials.py M_Terrain` (puts map, PlaceRect, ramps on the
    `instances` named), `build_island_scatter.py <Area>` (thins grass, flowers, dry tufts; place clutter = PebbleCluster_A
    and Rock_D in the place MIs; the chapel's white yarrow from `places.yarrow`), settle, save. Previews:
    `ground_preview.py places`; eye-level tour views `Saved/GroundDetail/place_views.json`; sheet
    `Saved/ArtPreviews/Terrain/Places_compare.png`. Perf: unchanged (RR spawn 7.7, tour worst Aerial_Dusk 8.2) only
    because the pattern is read within 60 m and the map within 120 m; reading them everywhere cost wide views +0.3 ms.
  - Roads and chest spots (round 3, 2026-10-27, notes 16 and 26: "the map doesn't really prompt the player to use roads, it feels
    like a free for all"; "chests should be in more distinguished locations"):
    - Ground: layout `macro.roads` -> `Art/Levels/area_places.py <Area>` also writes `Art/Textures/<Area>Places/T_<Area>Roads_M.png`
      (R road, G across m/16 + 0.5, B half width/8, A ruts; 14 roads, 0.3 s). `M_Terrain`'s road term
      (`build_world_materials.py`: `ROAD_WEIGHTS`, group Roads, `apply_roads`; `RoadAmount` 0 on the master, set on the area's
      `MI_<Area>Macro*_Steep`) makes the bed stony and lighter, worn dark edges, wheel ruts (a path: one trodden line) with relief,
      read within `look.reach` m (120). Cost: one more 4 MB map read within 120 m and three more on road pixels within 60 m; if
      over budget lower `look.reach` to 80 in `macro.roads`, or `RoadRelief` 0. `place_texture` finds `<Area>Roads_M` in
      `<Area>Places`. `ground_preview.py` doesn't preview roads. Perf after round 3: RR spawn 7.8 ms (`Game.md`).
    - Roadside (layout `roadside`, `build_area_dressing.Plan.roadside`): named signposts (`ARoadSign`, `SM_Signpost`) at branches (13:
      FarmRoad RANSOM FARM / MAIN STREET; NorthRoad CHAPEL / MAIN STREET; SinkRoad THE SINK / MAIN STREET; QuarryTrack OLD QUARRY /
      MAIN STREET; WestRoadChapel and WestRoadGrave KEEPER'S GATE / CHAPEL; KeepersPath RANSOM FARM / KEEPER'S GRAVE; FieldsPath
      WHITLOCK FIELDS / MAIN STREET; YardRoad DEPOT / MAIN STREET; BluffApproach RANSOM'S POINT / RANSOM FARM; BenchTrail CALDER'S
      POST / LIFT HEAD; CrestPath GOAT STEPS / LIFT HEAD; OreTramway THE DIGGINGS / LIFT HEAD), 4 lantern posts (2 on the farm
      road into the gate, 1 yard road, 1 fields path), marker posts (`FencePost`) every 22 m (west road 6, keeper's path 2, north
      road 2, Sink road 2, quarry track 2, fields path 1) and cairns every 18 m (bench trail 4, crest path 1). `mapRoads` places an
      `AMapRoad` per road (`MapRoad_<id>`, folder Dressing/Roadside), which `MinimapSubsystemRoads.cpp` paints into the bake: roads
      sand with a dark edge, paths pale, rails steel; so the HUD minimap and the map page show them. Signposts: names read left to
      right on the boards' faces; if one shows mirrored, swap the front/back `FacingYaw` in `RoadSign.cpp` `Face()`.
    - Chests (`build_area_caches.py` dict spots with `perch`, `Site.perch_floor`: a perch spot stands on whatever blocks the player
      there; `build_area_loot.py` `bridge:<id>` anchors). Old -> new: RangerCache_BluffPath (path top, by the east lip (-7450,-7380))
      -> the lookout deck by the right rail (Lookout frame (210,-215) yaw +90, about (-8815,-10210), 6.5 m up, facing north; second
      spot in front of the hut's bench (-8700,-10100)); the loose Strongbox (ring 2.5-5 m round Wagon_BurntA (-1950,-1950)) -> the Dry
      Wash bed beside Keeper's Bridge (rings 2.8-4.2 m, prefers a pier or bank at its back; id `Strongbox_<x>_<y>` changes);
      CampCache_OrchardEdge (open ground (-4700,-1750)) -> the crate spots at Coffin Rock (first tucked at its NE foot (-5195,-2335);
      those failed `Looter.Encounters.Camps.Placement`, so `layout.json` now has (-5062,-2379), (-5122,-2559), (-4902,-1899), 4.4 m
      off the rock, old open spots last); CampCache_DinnerPail (open camp (17600,-1050)) -> behind granite H (16452,-1518) facing 62,
      2 more, old last. Each falls back to its old spots and the build log names the spot used. Kept: RangerCache_Windmill and
      _SinkRim (Ruth's note names them), the gang's Strongbox (office), the saloon backlot strongbox, SheepFold's and MillCreek's
      crates, every SECRETS chest.
    - Rebuild order (after a layout patch with `layout_patch.py`): `area_places.py <Area>`; build; `build_world_materials.py
      M_Terrain` (recompiles the road term, `apply_roads`); `build_area.py <Area> dressing` (signs, lanterns, markers, MapRoads; it
      reruns `build_area_loot.py`); `build_area.py <Area> gameplay`, then `build_area_loot.py <Area>` again (keeps off the moved
      chests), `build_area_fauna.py`; `build_island_scatter.py <Area>` (keeps off the new pieces); `path_probe.py`. Tests
      `Looter.UI.Minimap`, `Looter.UI.Map`, `Looter.World.RoadSign`, `Looter.Encounters.Camps`.
    - Not verified in play: the road term's look and shader compile cost on a low end card, the signs' TextRender facing, the deck
      crate (it shows in the cold open's deck shots, beside the deck's own crate and rope).
  - Area MIs: `MI_LeavesRusset_Ransom` (Tint 3.2/1.0/0.7: the sumac red), `MI_LeavesDusty_Ransom`; a hex tint can't push past 1.
- Path checks: `Tools/Unreal/path_probe.py <Area>` walks a player capsule down every road and ramp; `Tools/Unreal/width_probe.py`
  probes across one. Run them after cliff, dressing or scatter changes.
  - Both areas use `"scatter": {"roadside": "pebbles"}`, so no rock or boulder lands on a road (the tutorial island had a boulder in
    its forest road until 2026-10-08).
  - The bridge's one flagged spot on the keeper's path is the step onto its deck.
- Gameplay pieces from the layout's `gameplay` block need no generator run (read straight from `layout.json`): jetty, landings,
  posters, the gunsmith's benches (`gameplay.benches`, `build_area_benches.py`; Ransom's Rest has none since 2026-10-28, its old one
  removed by `Tools/Unreal/remove_rr_bench.py`). `build_area.py <Area> gameplay` places them again.
  Also from it (2026-10-09):
  - encounters between the story's fights (`gameplay.encounters`, `build_area_camps.py`): camps with a Supply Crate
    (`CampCache_<id>`), patrols on a road, ambushes that rise or drop when the player walks in. Each waits for a story step and keeps
    off the safe zones and every scripted fight's ground; set pieces (a `setPiece` block, and the valley's `fights` with
    `"pass": "valley"`): "Fight: Layout" below;
  - Skyreach's notice board (`build_area_board.py`): the practice postings' board on the town's NoticeBoard piece, and the lookout's
    marker.

  From 2026-10-12, also in the gameplay pass:
  - Vendors (`build_area_vendors.py`): an `AVendor` at a shop's door, derived from the built level, so the store must stand first (it
    comes from the full build's models; the script runs after the town life). It stands 0.95 m out from the store's lamp socket,
    facing out, in `Gameplay/Vendors`. Ransom's Rest gets Pruitt's General Store (`Vendor_Pruitt`, open after Main 3), Skyreach the
    practice store on the square (`Vendor_Crossroads`, always open); any other layout gets none. A new shop is a new entry in the
    script, with its `OpenWhen`. On its own: `Tools\console.ps1 "py
    C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_vendors.py RansomsRest"`. It places nothing, and says so, when the game
    module's `AVendor` isn't built yet.
  - Vigil Stones (`build_area_shrines.py`): a stone beside each boss arena (the Gravemother's at the Sink's ramp head, Abel's outside
    the Keeper's Gate), after the story's pieces and before the camps: on level walkable ground clear of the layout's roads, ramps,
    obstacles and rocks, off the boss's ground, facing where the player comes out. Each choice is logged; a boss whose arena or actor
    isn't in the level gets a warning and no stone.
  - Creature levels (`gameplay.levels` in the layout: `band`, `zones`, `kinds`). A creature's level is where it stands, never the
    player's: the first zone holding its spot gives it a level rolled between its two (`zones`: a layout `zone`, an `obstacle`, its
    own `polygon`, or a `radius` round a point `at` or `between` two obstacles); a `kinds` entry sets a class's level (the bosses).
    - After editing, run `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_area_assets.py"` in the open editor: it
      writes band, zones and kinds into `DA_Area_<Area>` (`UAreaDefinition`). `Looter.Areas.Bands` checks the assets against the
      layouts. An area with no layout yet (the Lily) keeps a plain `band` in the script.
    - Tests: `Looter.Areas.LevelBand`, `Looter.Areas.Bands`, `Looter.Creatures.AreaLevels` (every fight's and encounter's levels, the
      bosses'). Never read the player's level for a creature. Table: `Docs/Progression.md`, "Creature levels".
  - Creature pursuit (`FCreaturePursuit`, `CreatureBasePursuit.cpp`): a hunt goes on until there is a real reason to stop; a creature
    that can't get to the player waits at its ground's edge (or under them), watching, then walks home when its patience runs out.
    - Tunables: Project Settings > Game > Encounters > Pursuit (`UEncounterSettings`): sense radius, grace out of sight, sniff time
      and radius, lost-after, unseen limit, patience (`ProwlSeconds`), stuck time, spurn.
    - A group that hangs back (the Spitter, the Bell-Ringer) tracks no nearer than its hold distance. Bosses keep their fights' own
      rules. Tests: `Looter.Creatures.Pursuit.*`.
- Ambient life (`build_area_fauna.py`, at the end of a full build or on its own): bird perches traced on fence rails, roofs, graves
  and dead limbs (crows 15 m clear of Hob), insect zones from flowers, orchards and water, tumbleweed lanes, dust devil spots,
  washing on laundry lines. Derived from the built level: re-run after dressing changes. `UFaunaSubsystem` updates it (nothing ticks
  on its own); `Looter.Fauna 0/1`, `Looter.Fauna.Stats`.
- The small animals (`build_area_animals.py`, after the fauna in the full build, or on its own; look sheet 44): chicken yards and the
  porch cat from town.json's `spots` (pens from the picket yard and the coop's run, the cat's exit and hide point traced under its
  porch, or `hide`/`exit` on the spot), cliff swallows at the placed SM_SwallowNests (one flock per cluster group). Code-posed from
  pose tables (`Tools/animal_poses.py` writes `World/*PoseData.inl` from `Intermediate/AnimalsModel/*_poses.json`, which the
  models' scripts write: rerun both after a pose change). They're fauna: `Looter.Fauna 0/1`, `.Scare`.
- Air, light and life with nothing placed (concept C, 2026-10-14): lights tagged `Flicker` breathe near the player
  (`ULightFlickerSubsystem`; AHouseLights' lamps by default, the mine's and keeper's lanterns, the town's lamps); lit windows
  (homes' `SOCKET_Window_<n>`) play a passing shadow or a curtain now and then (`UWindowLifeSubsystem`, never a face); motes, pollen,
  leaves and moths round the camera (`UAmbientAirSubsystem`: trees, lamps and underground boxes scanned at start and at each
  lighting switch; pollen follows the terrain's surface map); a gust every 40-120 s (`UGustSubsystem`, MPC_Wind read by
  M_WorldFoliage and M_Smoke) bows plants, leans smoke, flaps washing, spins the windmill, rolls dust. Hung things swing on
  `USwingComponent` (put it at the pivot, attach the mesh). `Looter.Flicker`, `Looter.WindowLife`, `Looter.Air`, `Looter.Gust`.
- A town's dressing on Ransom's Rest: `Art/Levels/RansomsRest/make_town.py` writes town.json (vignettes per building, in its
  frame; `APPROVED` names the new clutter that goes in; `Saved/rr_town_probe.py` first, for real ground and spots), placed by
  the dressing like Crossroads Town's; the town's lamps light at dusk (`place_town_lights`). New town meshes need PIECES and
  FOOTPRINT entries in `build_area_dressing.py` (make_town.py prints them).
- Ambient sound emitters (`build_area_sound.py`, in the full build): creeks, ponds, falls, open edges, Main Street.
- The lootable world (`build_area_loot.py`, 2026-10-09): runs in the full build and the `dressing` mode, after the dressing and
  gameplay pieces stand and before the fauna; on its own, `Tools\console.ps1 "py
  C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_loot.py RansomsRest"`.
  - Turns the dressing's wooden crates and barrels (`Crate_A`, `Crate_B`, `Barrel_A`; never the steel drum) into `ABreakableProp`s
    where they stood, and stands breakables, graves to dig, coffins to pry, mailboxes and footlockers about the area at spots it
    finds (level, clear, off roads, off the story's and encounters' spots, never near a respawn grave or the family plot). Everything
    sits in the area's `Loot` folder, so a rebuild replaces only that.
  - Reads the built level: run it after any dressing or story change. Idempotent (a run gives the dressing's instances back first).
    Ids come from where each piece stands, so the session keeps the same one broken or opened; moving a piece forgets its state.
  - Breakables are pooled and nothing ticks idle: pieces are thrown from one pool of components (`UBreakableDebrisSubsystem`, at most
    40 out); a prop ticks only while a hit rocks it.
  - Models: `Art/Models/Props/Lootables.py` (pieces modeled where they sat, origin the prop's pivot); sounds: the `props` group.
    Check with `Looter.Breakable.Break/Mend/List` and `Looter.Chest.Open/Reset/List`.
- Town life (`build_area_townlife.py`, 2026-10-09): runs in `build_area.py`'s gameplay pass, after the story's pieces (the shutters
  must stand; the house lights come from the effects pass), or on its own like the loot script. Ransom's Rest only (a layout with
  Ellis's grave). Places `ATownLifeDoor`s outside the unnamed townsfolk's doors (folder `Gameplay/TownLife`) and gives the lit
  houses and the store's shutters a household, which is all `UTownLifeSubsystem` needs; nobody is seen, only heard. Households, their
  sounds and the mutters' lines are `TownLifeRules`. Places nothing, and says so, when the game module's classes aren't built yet.
- The map page and fast travel (the inventory's fourth page; M, down on the D-pad): the map is the minimap's runtime bake, so a
  level needs its `Ground` tags and nothing else. Kept by hand:
  - the places' names (`UI/Inventory/MapPlaces.cpp`, copied from the layout's `preview.labels`: change both together);
  - each grave's name (`ARespawnMarker::DisplayName` on the marker, else the map shows its id in words).
  - Pins come from the level (found roosts that are travel stations, stations, benches, open stores, risen vigil stones, givers,
    turn-ins, the objective; never chests, the user 2026-10-27), so a new
    kind of pin is a `MapPins` change.
  - Check with `Looter.Map` (opens the page), `Looter.Map.Graves` (each grave and whether travel could go there),
    `Looter.Map.Travel <grave id>`; tests `Looter.UI.Map.*`.
- **Landfall's world** (WORLD, 2026-10-31; built by `Tools/Unreal/build_area_landfall.py`, hooked into `build_area_story.place()` after the
  deck's StoryLighting and before Hob; everything in `Gameplay/Landfall`; runs alone too: `py .../build_area_landfall.py RansomsRest`, clears
  only its folder; Hob and Delia need the full gameplay pass). One table of other lanes' names (`NAMES`, `SHIP`) and one of stand-in paths
  (`STAND_INS`, `MATERIALS`, the farm's `BARRICADE`). Frames: layout.json X north, Y east; UE cm. Run order: `Saved/landfall_probe.py` (read-only;
  prints `LANDFALL` lines + `Saved/landfall_probe.txt`: classes, stand-in bounds, merge API, an ASCII map of the ledge, the whole plan, course
  clearance per segment, Beyond meshes' collision), `build_landfall_materials.py` (5 MIs in `/Game/Art/Landfall/Materials`:
  `MI_Landfall_AbelTrail` M_GravewindWisp gold-white, `_SoulSmog` M_Smoke black-violet, `_SoulSmogGlint` faint cyan, `_BargeIron` MI_IronBlack,
  `_HoseRubber` MI_PaintBlack; no new master), `build_area.py RansomsRest environment` (adds the Smog state), `build_area.py RansomsRest
  gameplay`, then `build_area_loot.py` / `build_area_fauna.py` (they read the built level), optionally `build_island_scatter.py` (grass back
  where Ellis's grave stood; kept off the barricade); delete unused `/Game/Art/Landfall/Baked/SM_Landfall_*` from earlier runs (a changed level
  makes a new hash). Read the log's `landfall plan:` lines: tier (`on`/`relaxed`/`alongside`/`design` = nothing fit, check), share on the
  ledge, winch not None, 4 climbers, course raised count (0-3 fine; 14 = still touching), `open air under the stop: True`, barge keel ~ rim +
  4500, spyglass yaw ~51, trail ~40-50 wisps, hat not None, porch dredgers 5 / dead 3; warnings "placed one by one" (merge failed), "no ... class",
  "left out".
  - **The Mooring Ledge finding:** the real ledge (boundary corners 71-74, mooring post ~(-10100,-10200, z -25)) is only ~9 m deep, so the
    hull (30 x 8 m) cannot sit on it: the moored tier is `alongside` (hull wholly over the drop beside the ledge edge, deck flush with the ledge,
    level; one side <= 1.5 m from ground along >= 35% of its length), over the Mooring Notch. Tiers tried in order: `on` (bow third 90% on
    ground, >= 40% of the hull, stern over the drop, roll 2.5 / pitch 1.5 degrees), `relaxed` (60%, 30%), `alongside`. A traced 50 cm height grid
    round the post (X -1100..+900, Y -2000..+2300) classifies cells ground / rough / drop / rock; keep-outs: post 4.2 m (covers the seeds' page,
    letter and `Secret_MooringLocker`, all <= 2.7 m from it), stairs' foot 3 m. HardLanding should scrape her along the edge and settle; the
    winch's line holds her.
  - Course: start ~350 m west over the plains (40 m over what is under it), down into the canyon (25 m under the ledge level, 190 m out), the
    grapple stop 120 m back along her approach 8 m over the ledge (approach = moored heading if open air there, else due east), then two landing
    points (40 m out 12 m up; 3 m over Moored). Every segment before the stop is swept (keel -6 m, bag top +17 m, both sides) and raised 5 m at a
    time (<= 14 tries). The ship is placed AT Moored in the editor (tours see her on the ledge) and FLIGHT puts her at `Course[0]` for a new game
    (a command-line tour plays a new game, so LedgeMoored / LedgeFromStairs won't show her moored). SHIP dict = the stand-in's real numbers:
    length 3240 (bag; hull 2430), beam 1130 (bag; hull 700), keel_below_pivot 0, top_over_pivot 980, deck_over_pivot 56, forward_yaw 0.
  - Winch: beside the bow on the ledge side, 2.6 m off the hull. `LedgeClimbers` (`AmbushSpawner`, entrance Rise, 35 m, 4 Basic Unpaid at ledge
    cells next to the drop, >= 2.5 m off the hull, `during Main1 step 4` only, give up 30 m, tag `Unpaid_LedgeClimbers`; its spots sit at the drop's
    edge and may fall outside the playable boundary: `Looter.World.RansomsRest.InsideBoundary`). `FarmPorch` (`EncounterSpawner`, at the porch
    front; spots 15-23 m from the porch; on approach 45 m; `during Main1 from the trail step before porch step + 1`; give up 60 m; tag
    `Landfall_FarmPorch`; max 5 alive). `encounter_when` reads `story_texts.ENCOUNTERS[id]` (from 4 before 5; from 7 before 9).
  - Spyglass: the lookout deck point (commonest height 5.6-7.6 m over the pivot) with a clear eye-height line to the barge, >= 1.3 m from
    Sexton's seat and the ranger cache's two spots; faces the barge.
  - **Barge** (one `AStoryProp`, tag `Landfall_Barge`, pivot = keel middle; the reveal looks at its actor location): the user's Tripo hull `/Game/Art/Tripo/SM_DredgeBarge`,
    reworked 2026-11-04 (75 m, two stacks, plumes at its `Stack_n` sockets; "The barge in the level" below). The first build, still the branch for a missing hull: `SM_Skiff_A_Gang` x7 (63 m),
    all slots `MI_Landfall_BargeIron`, keel 45 m over the Sink's highest rim vertex, broadside to the lookout; 3 stacks (cylinders), 12 glowing
    vents (MI_CinderGlow), a hose drum under the keel. **Hose** (`Landfall_BargeHose`): >= 6 sections 1.5 m across with collars, a 6 m sideways
    sag, to an iron intake 4.5 m off the floor's middle. Baking each into one mesh (`/Game/Art/Landfall/Baked/SM_Landfall_<Piece>_<hash>`,
    reused while the parts don't change; the merge is `StaticMeshEditorSubsystem.merge_static_mesh_actors`) is off (`BAKE = False` since 2026-11-01: the 5.8 merge destroyed the placed parts), so the barge is one live prop
    `Landfall_Barge_01` (the hull as is) and no `SM_Landfall_Barge_*` is made; the fallback runs: 5 props / 8 sections / trail 30 wisps culled at 60 m. Smog: 3 plumes x4 (32 m) on the stacks (`SM_SmokePlume`, `MI_Landfall_SoulSmog`), a cyan glint plume x2.8 in the
    middle one, x2.2 at the intake. Drone: `AAmbientEmitter` loop `Ambience.Smog`. All shown `before Main6` (`BARGE_UNTIL`).
  - **Smog rule:** see `Game.md` "Landfall and the Lamplighter" (state values in layout.json; `AStoryLighting` rule `Smog while before [Main6]`).
  - **Abel's trail** (`Landfall_AbelTrail`, one baked AStoryProp, `during Main1 from the trail step`): ~45 wisps (`SM_GravewindWisp_B/C/A` x0.55-0.75,
    lifted 1.1 m, pitched 22 degrees so the ribbons lie over the ground, flowing toward the farm) every 3.4 m: the lookout's foot -> bluff path
    (ramp reversed) -> bluff approach -> past Abel's grave -> family plot path -> round the farmhouse's north corners -> the porch steps; the
    merge must keep vertex colours and UV1.
  - **Abel's hat:** the bluff path's middle bend (-7000,-6600), 1.1 m toward the bluff side (see `Game.md` for its readable and flag).
  - **Loudspeakers:** posts (`SM_FencePost` stretched to 4.2 m, solid), two brass cone horns, a black plate; 3 `InstancedProps` (culled 150 m);
    one on Ransom's Point 6.5-11 m from the lookout toward the bluff path's top, horns toward town; up to three on Main Street at X +-700 (Y 900,
    2600, 4300), horns across the street; each off roads, on Ground, nothing solid there; a `TargetPoint` tagged `Landfall_Loudspeaker` at each
    post's horns (`UVaneBroadcast` could play its hiss there).
  - **Edits in other files (WORLD/HOOKS):** `build_area_story.py`: Ellis's grave actor destroyed in `place_family_plot` (the layout placement
    `graveEllis` KEPT: ~12 scripts gate "is Ransom's Rest" on it), Ellis's readable headboard + `hero_headboards` removed, Abel's stays; the farm
    safe zone is now `after Main1` (the porch fight is inside it; `ColdWelcomeTests` line changed to match); Hob placed on Abel's headboard and
    perched by Main 1 step from `story_texts.LANDFALL_PERCHES` (winch for the anchor step, lookout rail for the point step, Abel's headboard for
    the trail step, porch beam end for the porch step; away for steps 0-3; after Main 1 on the porch beam), warning if a perch has no spot.
    `build_area_farm.py`: `ADeliaRansom` on the porch floor + 68 cm (`DELIA_HALF_HEIGHT`) facing out, tag `Speaker_Delia`, name "Delia Ransom",
    `story_texts.TOPICS['Speaker_Delia']` first then the later topics (mine, Main 6 with its `Delia.Main6` event, Main 7); the screen-door
    `ASpeakerPoint` is placed only if her class is missing; the porch barricade (6 solid stand-ins tagged `Obstacle`,`Barricade`: SM_Bench,
    SM_LumberStack, SM_FlourSacks x2, SM_Crate_B, SM_Cart; each ground/road/overlap checked; keep her eye line at ~1.3 m clear).
    `build_area_missionprops_valley.py`: the porch plate is a free pickup `after Main1`. `layout.json`: environment `Smog` state; level-band zones
    `landfallLedge` (polygon, levels 1-2) and `landfallPorch` (at [-4000,-5280], r 2600, levels 1-2) before `west` (then
    `create_area_assets.py`; tests `Looter.Areas.Bands`, `Looter.Creatures.AreaLevels`). `views.json`: LedgeMoored, LedgeFromStairs, SpyglassBarge,
    FarmBarge, SinkUnderBarge before `DeckFight_Dusk` (last but the deck: `BargeDeck_Landfall`, `_Stay` after it since 2026-11-04); TrailFromPoint left out until `Looter.Landfall 7` is known not to move the
    tour's camera. Tour views to check: those five plus PointOverValley, Farm, Aerial, Aerial_Dusk, PointWest (sees
    the ship), MainStreet (speakers, plumes), SinkRim.
  - **Perf risks (margin 0.2-0.5 ms):** valley views gain the barge (1 prop; casts a shadow inside the shadow distance only), the hose,
    5 translucent plumes 18-32 m tall (overdraw looked up at from town / the Sink), the drone; the Farm view (7.8 ms, a heaviest one) gains 6
    barricade draws + Delia + the trail (steps 7-8 only). If over: `cast_shadow` off on the barge, drop the glint plume, `STACK_PLUME` 3; bake the
    barricade (`merge_physics_data` True) or cut yard pieces. `perf.ps1`'s "RR spawn" moves (a new game starts on the ship over the canyon), so
    its numbers aren't comparable with the old spawn's: compare tour views.
  - **Unchecked:** the merge API in 5.8 Python and that the merged pivot is the first part's place; whether the canyon/ring meshes (tagged Beyond)
    collide (else the course sweep can't see them); `AReadableProp`'s `shown_when` re-read when the hat flag is set; the skiff bag shape under
    the stacks and vents; the barricade stand-ins' long axes; M_Smoke's CloudTint dimming the glint.
- **The barge in the level** (2026-11-04; `Docs/Work/2026-11-04-Art/BARGE_LEVEL.md` lists what the level saves, its heights are round 2's; model and kit: `Game.md` "The barge: the reworked Tripo hull and the kit on it"). Landfall's Sink barge and
  Main 6's `Barge_Pilot` both use `/Game/Art/Tripo/SM_DredgeBarge`, the user's Tripo hull adjusted piece by piece (round 3: 93.4 m long, x -3750..5590 about a pivot that is no longer its middle; the same path as the bought 75 m hull; the scripted
  `/Game/Art/Vehicles/` hull and its `SCRIPTED_BARGE` are gone). Landfall: `build_area_landfall.py` `STAND_INS['barge_hull']`, whose `barge_parts` takes the Tripo branch (`is_tripo`, `tripo_barge`): the hull as is, one plume per `Stack_n` socket, the
  hose at `HoseDrum`, a warning if the mesh lacks `Stack_1` or `HoseDrum` (the `Gangway` and `Hatch` sockets are ignored); the size comes from the mesh's bounds (`plan.barge_size`); Landfall's barge is one live prop (no bake, so no
  `SM_Landfall_Barge_*` to delete). Main 6's own barge takes the hull from C++ (`ABargeBoarding::HullPath`); `build_area_main6.py` `WRECK_MODELS` has it second (`SM_BargeWreck` first; the wreck's fallback hull, worn in `MI_GunScorched`). Both meshes
  are read live, so a re-import needs no clearing of stale copies. Who reads the hull: Landfall's `place_barge` (the Sink barge, the plumes, the drum from `HoseDrum`, the drone path from the hull's length), Main 5's `Hose` (re-plans with
  `lf.barge_parts`), Main 6's wreck; Main 4's tender and Main 3's skiff use only the keel.
  - Main 6 saves everything on the deck as the world position the C++ getters gave when it ran (`GetFightSpots GetOfficeSpot GetHornSpots GetHatchSpot GetDeckMiddle GetHallMiddle GetHoseTop GetBerth`; properties `hull`, `hose_target`, `fall_target`,
    `boards_point`: renaming any breaks the scripts). So after any `BargeRules` change, and after the C++ build (the getters answer from the compiled class), rebuild. The C++-spawned pieces (hatch marker, furnace, valves, hose drum, deck kit, hall,
    stairs, lamps) follow at BeginPlay. `KitLift` stays 0: a raise stored on the placed actor would be lost at the next rebuild (it would need a C++ default).
  - `build_area_main6.py` runs alone (`run()`: `py .../build_area_main6.py RansomsRest`; refuses with unsaved edits elsewhere): it destroys the build-tagged actors in `Gameplay/Main6` (13: `Barge_Pilot`, `Encounter_BargeDeck`,
    `Encounter_FurnaceHall`, `Barge_Horn_1..4`, `Item_BargeManifest`, `Main6_BargeWreck`, `Main6_WreckSmoke_1..3`, `PilotSortie`), places them again, sets again the Lamplighter's `gone_when`, the winch's `cast_off_when`, `hidden_when` on
    `Landfall_Barge*`/`Landfall_Smog*` and `gone_when` on `Intake_Sink*`, and saves. The barge's x,y comes from `get_hose_top()` (the drum lands `DRUM_BESIDE` 7 m off the chapel ridge; `KEEL_OVER` 25 m stays); the sortie's course end uses
    `get_berth()` and warns if it fails (it used to fall back silently to the keel middle).
  - **Rebuild order** (after the import, `build_world_materials.py DetailInstances` and `Tools\launch.ps1 -Build`): the whole `build_area.py RansomsRest gameplay` (Landfall, Mains 2-5, then Main 6; no loot or fauna rerun). Barge only: Landfall alone,
    THEN Main 6 alone (the other way round Landfall wipes Main 6's patches and the sortie's refs); that loses Main 5's patches (the hose and three plumes stay shown after the choke) until the next gameplay pass. Main 6 alone leaves Landfall's pieces
    as built (the hose's start does not move while `HoseDrum` stays at (-889, 323, 0)). The level was last saved in `0c0750bf` (15:58); whether its Main 6 pieces were rebuilt after BARGE.md round 12 moved zone heights and spots is not recorded: rebuild from the final code and compare the log below.
  - Check the log: the kit's deck 2256 over her keel (the hatch spot, the cross walk; `kit_lift` 0); BargeDeck 9 spots 2270..2391 (well deck 2270, walk mouths 2277, forecastle 2391), spawner 2320; FurnaceHall 8 spots 1320 (`HallFloor` 1300 + 20, the
    hall is below deck), spawner 1370; the sortie's berth 1928 (`WellDeckZ` - `BerthUnderDeck` - 422); the manifest on the stoker's desk (the catwalk, 1880 over the keel, plus the bench); Landfall "2 stacks smoking" (`No socket` = the model lacks
    `Stack_1`/`HoseDrum`); no "can't be called from Python", "aimed at the barge's keel middle", "no ... class", "left out". The deck fight is 9 creatures on 9 spots and `count=9` (`create_mission_assets.py`), the hall 8 and `count=8`: change both
    sides together.
  - To look at once: Landfall places the Sink barge by the mesh's length about the pivot, and the pivot is no longer the middle (the bow reaches 18 m past the old end, the stern the same as before): the tour's SpyglassBarge and FarmBarge views.

## The tutorial island and the older levels

- Tutorial island (Skyreach): built by scripts from `Art/Levels/TutorialIsland/layout_computed.json` (the terrain model writes it).
  - `Tools/Unreal/build_tutorial_island.py` places the terrain, cliffs, buildings, lighting and gameplay actors; rebuilding replaces
    only what it placed; its `gameplay` mode places just the spawn, dummies, spiders and slimes again.
  - `Tools/Unreal/build_island_scatter.py` scatters grass, flowers, trees and rocks with PCG from the scatter mask.
  - `Tools/Unreal/review_stage.py` photographs new models under the island's lighting.
- Crossroads Town (round 1, 2026-10-08): buildings, roads, the Wallow's pools (small `pond` features), zones and creature groups are
  in `layout.json`; the rest comes from `Art/Levels/TutorialIsland/make_town.py`, which turns the concept's drawing
  (`crossroads_town.json`) into `town.json`: pieces, fence lines, Web Hollow's webs, the no-tree areas, and the concept pieces still
  waiting for a model (`deferred`).
  - It applies the concept's fixes, maps its kit to the game's models, and keeps every piece off buildings and porches, roads,
    gameplay spots, each other and the rim (3 m), printing every nudge and drop.
  - `layout.json` `level.town` names `town.json`; `build_area_dressing.py` places it instanced (one `AInstancedProps` per mesh and
    solidity: reeds, lily pads, lone bushes, webs and cocoons are passable), its fences through the kits, its strands tied into the
    dead trees' trunks. `no_tree_boxes()` keeps the scatter's trees off the gardens, creature grounds and the town trees' crowns. The
    scatter grows no trees on the island any more: the concept's own woods are town pieces.
  - Order: edit `layout.json` -> terrain (`TutorialIsland.py -- --macro --computed`, then `models.ps1 -Only TutorialIsland`) ->
    `make_town.py` -> full `build_area.py TutorialIsland` (the `dressing` mode doesn't redo the no-tree boxes) ->
    `build_island_scatter.py TutorialIsland`, wait for the count to settle, save -> `path_probe.py`.
- Older levels are built in the editor. Procedural props are `StylizedProp` actors (shape, seed, two colors). Before committing such
  a level, run `Looter.BakeLevelProps`: it swaps them for static mesh actors and saves their meshes and materials under
  `/Game/Environment/Props`.
- Seating props: after placing props or changing terrain, run `Tools/Unreal/conform_hills.py` (terrain changes only: it fits the
  hills' rims under the ground), then `Looter.SettleProps` (`selected` for the selection), and save. It seats every prop so no edge
  hovers, leaning low, wide ones with the slope.
  - Not after a full `build_area.py` run (2026-10-13): the build already stands everything on the new ground, and SettleProps
    re-seats every mesh within 6 m of the ground: it tipped the mine portal 15°, dropped Keeper's Bridge 2.8 m into the wash and
    lifted the lift head 1.5 m. `TRANSACTION UNDO` in the console took it back. Use it only on a selection, for props placed by hand.
- Lvl_Skyreach's meadow: grass and flowers come from the `Meadow` PCG volume (`/Game/Environment/PCG/PCG_Meadow`), which raycasts
  onto `Ground` actors and avoids `Obstacle` ones. After changing terrain, select it, press Generate and save.
  - Ground cover never collides (a placed static mesh actor takes its mesh's collision unless `bUseDefaultCollision` is off).
  - Patches are about 3.5 m and lie on the slope; `World/PCGGroundFitFilter` drops the ones that would hang off an edge;
    `Looter.BakeGroundCover` bakes their meshes again.

## Island styles (Style Lab looks on the tutorial island, 2026-10-08)

- The user's call (2026-10-08): "I don't want to change the style." The look stays as it is; no style work unless the user asks.
  Style 3 (Painted Frontier) was rejected and removed, style 6 (Sunbleached) undone, style 4 (Inkslinger) stopped before any code.
  The island wears today's look. Trials skip tests and screenshots (the user's call); full checks only when a style goes into the
  game.
- Tools that exist, if asked again: `Tools/Unreal/build_island_style.py`, `style_common.py` (shared helpers), `style_sunbleached.py`
  (its numbers), `Art/Levels/TutorialIsland/style_sunbleached.json` (the island's settings),
  `/Game/Art/Materials/IslandStyle/Sunbleached/` (its island-only MIs). A style = a static switch on the masters, off by default (so
  Sunbleached costs nothing now), plus any post material (`build_world_materials.py`; `styled()` chains one style's switch inside
  another's).
- Apply: `build_world_materials.py M_World M_WorldFoliage M_Terrain M_SkyClouds M_SunbleachedPost`, then
  `build_island_style.py sunbleached apply`: island-only instances for every slot on those masters, light filters, and a
  `<Style>Post` volume (tagged IslandStyle) above `IslandPost`; recorded in `style_applied.json` (commit it with the level). `undo`
  restores everything; `status` reports what's styled. One style at a time (the script refuses otherwise). After an island rebuild
  or a new scatter, run `apply` again. To drop a style, delete its `IslandStyle/<Style>` folder once the island no longer wears it.
- Lighting: the island's Day must stay the default (`Looter.World.Lighting.Levels`). A style's own sun waits in its json's `state`
  block until `LightingTests.cpp` skips the default-Day check on a level with an `IslandStyle` actor (one line).
- Lessons from style 3: rim terms on alpha-tested leaves speckle unless they read the crown's vertex normal; a screen-space edge pass
  cost the most (+0.8-1.0 ms in all).

## The Style Lab (art styles previewed in the game's scene, without Unreal)

- `Tools/StyleLab/` (README there): a three.js rebuild of a piece of Ransom's Rest from the game's own sources with the in-game HUD
  over it, so art styles can be judged "in game" from a cloud session with no Unreal. Built 2026-10-08: ten styles, the user's picks
  3, 4, 5, 6 (3 and 6 were tried on the island). Style 0 is today's look.
- `Docs/Archive/StyleLab/Catalog.md`: all ten styles with Unreal-terms numbers, recipe, cost, refs; contact sheets in
  `Docs/Art/StyleLab/Sheets/`; briefs and research beside it.
- Export: `Tools/StyleLab/export/export_all.sh` (Blender's `bpy` 4.5 module under Python 3.11); `assemble.py` builds
  `Saved/StyleLab/site/`. The artifact host serves no `.glb` or `.bin`, so models and heights go as base64 JSON (`--b64-glb`; packs
  under about 9.5 MB so each JSON stays under 16 MB); a publish over 64 MB goes in two calls.
- Cloud sessions commit text only (see the bug list: LFS uploads are refused); binaries live under `Saved/StyleLab/`.

## Finds (2026-10-29; Docs/Work/2026-10-28-Core/FINDS.md, CONTRACT.md)
- 15 finds in Ransom's Rest (the 9 secrets of 10-22 with tells and better loot; new: Tarn lockbox (dive, Still Water),
  Tumble bolt-hole (130 cm crawl), Rimrock Ranger Post (steps), Pike's cache (field-book scrap at Coffin Rock -> dig;
  Bench Mark), the miser's grave (ambush, 60 silver), the Tollman (wandering Gravebound elite, respawns 20 min after a kill;
  Ferryman's Due)) + 6 Ledger pages counted apart. Tells: two low circling crows and a glint for unfound finds (2026-10-29: crows 3.4-4.6 m up on a 3 m wheel, one perched on a
  chest under open sky that flies off within 7 m; an under-water find glows pale green within 20 m instead of flashing). Map: total
  and per-region counts. The FOUND banner pays XP (~4,442 in all) and a completion bonus.
- Built by `Tools/Unreal/build_area_finds.py` (+`_sites.py`, `_graves.py`) from layout.json `gameplay.finds`, run inside
  `build_area.py RansomsRest gameplay`. Missing art (stand-ins used): a laid rope, a card and knife, a bigger cache tag.
  Pike's cache digs up the grave's usual coffin (a variant body needs a C++ flag). E6's 2 grenade tins aren't paid yet.
