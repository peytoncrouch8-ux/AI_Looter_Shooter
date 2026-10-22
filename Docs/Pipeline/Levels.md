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
  - `dressing`: fences, walls, graves, props (`build_area_dressing.py`).
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
  - Area MIs: `MI_LeavesRusset_Ransom` (Tint 3.2/1.0/0.7: the sumac red), `MI_LeavesDusty_Ransom`; a hex tint can't push past 1.
- Path checks: `Tools/Unreal/path_probe.py <Area>` walks a player capsule down every road and ramp; `Tools/Unreal/width_probe.py`
  probes across one. Run them after cliff, dressing or scatter changes.
  - Both areas use `"scatter": {"roadside": "pebbles"}`, so no rock or boulder lands on a road (the tutorial island had a boulder in
    its forest road until 2026-10-08).
  - The bridge's one flagged spot on the keeper's path is the step onto its deck.
- Gameplay pieces from the layout's `gameplay` block need no generator run (read straight from `layout.json`): jetty, landings,
  posters, the gunsmith's benches (`gameplay.benches`, `build_area_benches.py`). `build_area.py <Area> gameplay` places them again.
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
  - Pins come from the level (open respawn graves, stations, benches, chests found or opened, turn-ins, the objective), so a new
    kind of pin is a `MapPins` change.
  - Check with `Looter.Map` (opens the page), `Looter.Map.Graves` (each grave and whether travel could go there),
    `Looter.Map.Travel <grave id>`; tests `Looter.UI.Map.*`.

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
