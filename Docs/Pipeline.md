# Pipeline

How things get made and fixed here. Update when work finishes (a new way, a bug and its fix, what didn't work). Rules:
`CLAUDE.md`. State of the work: `Docs/Handoff.md`. Old detail, not loaded by default: `Docs/Archive/Pipeline_history.md`.

## The game

- Story: *Revenant* (`Docs/Story.md`), weird west on the planet Teropa. One area per Hollow, each designed in `Docs/Areas/`.
- Areas, map order since 2026-10-13:
  - 1 Ransom's Rest: a new game starts on its cold open; its first missions teach the controls by hints. Mountain expansion:
    `Docs/Areas/RansomsRest_Expansion.md`.
  - 2 the *Gilded Lily*: behind the north mountain, by train; not built.
  - 3 Skyreach (the old tutorial island): reached later from the Lily by skylift; off the station boards for now, test with
    `Looter.Travel Skyreach`.
- Missions: all but Main 1 come from a giver (`EMissionStart::Offered`, `FMissionOffer`): a speaker point, a notice board or an
  artifact (`AMissionArtifact`).
  - ? = pick up, ! = turn in (the user's call, 2026-10-13: the reverse of Borderlands). Gold = story, white = sides. Shown on the
    map, the minimap and over the giver (`UGiverMarkSubsystem`).
  - Offer card `UMissionOfferWidget`: Accept / Not now. The Missions tab lists only missions taken up.
    `Looter.Mission.Offer <id>` opens a card.
  - Features: optional objectives (`bOptional`: never hold a step, pay `OptionalSilver` at the turn-in); mission items
    (`AMissionItem`, carried, dropped by a creature); a turn-in with a choice of two (held to choose; story flags in
    `FCampaignRecord::StoryFlags`, read by `FStoryCondition::WithFlags`); readable props; the porch plate; an UNDERGROUND minimap
    plate (actors tagged `Underground`); any speaker point or a vendor's hatch as a turn-in.
- Hob's roosts (the user's pick, 2026-10-13): the ONE save / respawn / fast-travel point on every map: the Crow's Mast
  (`SM_HobRoost`, bell `SM_HobRoostBell`), beacon in Hob's Ember (#D8321C, core #FF8A52; `URoostSettings`, in Project Settings
  under Game > Hob's Roosts; `LooterUI::Color::Roost`).
  - Walk within 9 m (clear line) to find one: column shoots up, bell rings, crows lift, three circle the beacon, ROOST FOUND,
    early autosave. Continue wakes the player at the area's last found roost (`FCampaignRecord::LastGraves`).
  - Fast travel and deaths: Hob's crows carry the player across the screen (`URoostCarrySubsystem`, `UHudCrowVeilWidget`); Hob
    speaks at every death.
  - The actor is still `ARespawnMarker` (save ids unchanged). Placed from `layout.json` `gameplay.roosts` by
    `Tools/Unreal/build_area_roosts.py`. `Looter.Grave.List`, `Looter.Grave.Mark <id> | forget <id|all>`.
- Impacts and bullet marks: a hit's surface (13: dirt, grass, gravel, sand, stone, wood, metal, sheet metal, glass, water, cloth,
  hay, bone) comes from tags, then the trim sheet's strip under the hit (collision UVs), the terrain's baked surface map,
  material-name words; no physical surface types. Each has its sound, puff and mark; glancing hits ricochet.
  - Marks are pooled DBuffer decals (`Looter.BulletMarks.Max/.Hold/.Fade`, 64, 20 s + 15 s fade); atlas script-made
    (`Art/Textures/BulletMarks/make_bullet_marks.py`).
  - Re-run `Tools/Unreal/build_impact_surfaces.py` after a macro map, scatter map or new trim-sheet model changes.
- Objective highlight: within 13 m, what the tracked mission's current objective is about (optional ones too, the giver once it
  waits; never a fight's creatures) glows orange as an overlay on its meshes (a spot of a building round a door).
  `M_ObjectiveHighlight` from `build_highlight_material.py`; `Looter.Highlight.List`.
- Holds: every hold-to-use shows the kit's round ring (`SLooterProgressRing`, `LooterUI::MakeHoldKeyCap`).
- Lost Loot (the user's call, 2026-10-13, so ground guns don't pile up and cost frame time): a gun on the ground lasts 5 minutes
  (not while the player stands within 6 m), then goes to the depot's LOST PROPERTY window. Leaving a map or quitting sends every
  ground gun there; a load never puts one back. 30 spaces, +10 per upgrade (five, bought with silver); the oldest unnamed gun goes
  out when full. `Looter.LostLoot.*`.
- The ore lift: `AOreLift` rides as a skippable look-only scene; the body is parked on the far landing at the start (a save,
  death or load mid-ride finds it there). `Looter.Lift.*`.
- Look: stylized realism; the user rejected faceted low-poly. Golden-hour palettes on Ransom's Rest (ochre, olive, sandstone),
  late summer. Loot colours (rarity orange, teal, gold) stay off the world.
- Player numbers (2026-10-13): 0.85 of the mannequin's size and 0.9 pace on top (the user: "pace of play is super fast"): walk
  459, sprint 711, crouch 230, slide 783 cm/s; eye about 140 cm; jump 103.5 cm. A slide comes out of sprint plus crouch. Chasing
  creatures scale with `LooterPlayerSize::MoveScale` (size and pace); slimes don't.

## Who does what

- Main (orchestrator, Opus 5.5): owns Unreal, builds, asset scripts, tests, play checks, git, docs; splits work among at most 8
  agents and checks everything they hand back.
- Models (the orchestrator's call): Opus for design and tricky code (new systems, rasterizers, widgets with timing and layout
  logic); Sonnet for well-specified code against a contract, reviews, doc edits, mechanical changes; Haiku for simple searches and
  listings.
- Shared contract for parallel agents (first used for the HUD upgrade, 2026-10-07). Before launching agents that touch each
  other's code, Main writes one contract file (in its scratchpad) every brief points to: each new class's public API with exact
  signatures, new palette entries with values, where each widget is placed and by whom, which agent owns which files. Agents write
  disjoint files at the same time; Main builds them together once.
- Agents never build, open Unreal or touch git. They report files changed, what Main must run, CODEMAP lines, tests affected,
  risks they couldn't check. For editor numbers they write a read-only probe (`Saved\<short>.py`) that Main runs.
- Layout patches (2026-10-13): while one agent owns an area's `layout.json` (the terrain agent), the others write
  `Art/Levels/<Area>/patches/<name>.json` (path, op set/merge/append, where {id}, unlessKind, comment; entries marked `file` are
  area_shape.py tables, edited by hand). Main applies them with `Tools/layout_patch.py <Area>` (keeps the file's formatting;
  `--check` first), deletes the patch files, and regenerates the terrain if they touch the ground.
- The art session (a separate session) models in Blender (`Art/Models/**`), never commits, never opens Unreal. It exports into
  `Intermediate\ArtExport_<Family>`, then messages Main with files, triangle counts, pivots, sockets. Main imports, commits, and
  sends back in-game shots to check.
- Main's helpers (in `Tools`):
  - `Tools\commitlib.py`: `commit(files, codemap_edits, title, body)` stages files plus CODEMAP edits applied to HEAD's copy, so
    other uncommitted CODEMAP lines stay out. Edits an agent already wrote into the working copy are skipped there.
  - `Tools\stage_import.ps1 <export folder> <model,model>`: copies chosen models and their materials into `<folder>_Import`, for
    `Looter.ImportModels`.
  - `Tools/Unreal/open_clean.py [map]`: collects garbage, then loads a map from disk.
  - `Tools\winshot.ps1 <handle>`: captures a window with PrintWindow; use it on a Save Content dialog.
  - `Tools\dialogclick.ps1 <handle> <x> <y>`: clicks a point in that dialog, in winshot's pixels, only if the dialog really came to
    the front (Save Content's Don't Save is 493 502; a one-line Message's OK is 535 127).
  - `Tools\menushots.ps1 [-Label]`: photographs the inventory through its states with a realistic loadout.
  - `Tools\castshots.ps1 [-Tag]`: every creature and character in its states from three angles, with `numbers.csv` (feet in the
    ground, limbs through bodies, sliding feet, pops as a change of world velocity per 60 fps frame) and a `past_mark` column. Dead
    bodies and falling slimes aren't sampled; a limb's 30% nearest its joint is excused against the part it hangs from. Still read
    the pictures before trusting a number.
  - `Tools/Unreal/pie_check.py`, `path_probe.py`, `width_probe.py`: below.
- Briefs that worked: give the agent the user's exact words, the files it owns, the files it must not touch, the run order Main
  will use, and what to report. Say "edit with the Edit/Write tools only" (a PowerShell `-replace` once corrupted a script). A usage
  limit can stop agents mid-step: resume each with SendMessage, saying what landed on disk.

## The everyday loop

1. Before any editor work, check the user isn't playing: `Tools/Unreal/pie_check.py` (a play session in the editor);
   `Get-CimInstance Win32_Process` for `UnrealEditor.exe ... -game` (the launcher's game). While they play, run no console commands
   and no builds. Saves fail quietly on files the game holds.
2. Build: `Tools\launch.ps1 -Build`. If it says "EDITOR DID NOT CLOSE", a "Save Content" dialog is open: find it with
   `Tools\editorwindows.ps1`, capture it with PrintWindow, read its list, choose Don't Save unless those assets were meant to change.
3. Scripts: run the asset scripts the change needs through `Tools\console.ps1 "py <absolute path>"`. Keep the command under about
   200 characters, or it's dropped. Each `build_area.py` mode saves the level itself.
4. Tests: `Tools\runtests.ps1`, all green; `-Filter Looter.<Group>` runs a subset. Tests leave the level dirty: reopen it clean
   afterwards (`Tools/Unreal/open_clean.py`), or closing the editor can save the tests' leftovers into the level (happened once:
   restored from git).
5. Look: `Tools\tour.ps1 -Map <map> -Views <json>`. Scratch view lists go in `Saved/TourViews/`; screenshots land in
   `Saved\Screenshots\Tour`. Read every screenshot before calling a visual change done.
6. Commit and push each working step on its own. The message says why and ends with the co-author line. To commit part of a shared
   file (CODEMAP), stage edits on HEAD's copy (`Tools\commitlib.py`).

## Making a level (grounded areas)

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
  - Every ramp's walkway is cleared of cliff pieces (`build_area_walkways.py`): a player capsule is tested down its middle. A ramp's
    own wall moves back into its cut; a ledge's outer face (under the path) sinks until it clears, at most 1.5 m (2026-10-08: nine
    faces on the bluff path and the Sink ramp sank 25-100 cm, no gaps under the lip).
- Scatter (PCG): `Tools/Unreal/build_island_scatter.py <Area>`.
  - Re-imports changed masks by MD5, then generates. Wait for the instance count to settle, then save.
  - Zones of kinds alpine, outpost, mine and crest, and `benchPines*` stands, grow pines only (PCG Intersection: one `In` pin,
    density MINIMUM or MULTIPLY; there is no BINARY).
  - Keeps off the dressing's footprints and wears the area's material swaps. Run `dressing` before the scatter when fences or graves
    move.
- Materials per area: `level.materials` makes area instances of shared MIs; `level.swaps` puts them on everything the build placed,
  and on the scatter; `level.swapsOn` swaps on named actors only (the windmill's steel).
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
    off the safe zones and every scripted fight's ground;
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

## Making an asset

1. The art session scripts the model in Blender (`Art/Models/<Category>/<File>.py`), with shared material names and texture sets from
   `Art/Textures/<Set>`.
2. It export-tests: `Tools\artrun.ps1` or `Tools\models.ps1 -NoImport -Out Intermediate\ArtExport_<Family>`.
3. Main stages only the new models (`Tools\stage_import.ps1 <folder> <names>`), runs `Looter.ImportModels <folder>_Import`, checks the
   import log (bounds, sockets, collision), and commits the `.py` with the `.uasset`s.
4. A colour-only change in the manifest can be set on the MI directly instead of reimporting a skeletal mesh (the lantern glow).
5. Tools: `Tools\artrun.ps1` runs a model script (`-Preview` renders it), any Blender script, or an export test, at below-normal
   priority, waiting while `Saved\ArtPause.flag` exists. `Tools/Blender/tangentcheck.py` checks an exported FBX's tangents as
   Unreal's import will; with `--log` it sorts the editor log's tangent warnings into the model's own and Unreal's reduced builds'.
6. Materials: the textured masters (`M_World`, `M_Gun` with per-gun wear, `M_WorldFoliage`, `M_Terrain`, `M_Water`) are built by
   `Tools/Unreal/build_world_materials.py`. Older surfaces use the flat stylized materials (`M_StylizedSurface`,
   `M_StylizedFoliage`, `M_StylizedGlow`).
   - The ground's layers (2026-10-12, `M_Terrain`): under the macro map's colour sit three detail layers and damp soil. The macro map
     picks them: grass on green ground; matted thatch (`GroundThatch`) on gold and dry ground; stony ground (`GroundStony`: crust,
     gravel, stones, and leaf litter under trees) on bare soil, roads and scree; darker, glossier damp soil in the hollows (the
     terrain's baked occlusion). Each layer is normalized by its own mean (`T_<Set>_stats.json`, written with the textures), so a
     texture change moves the pattern, never the ground's colour from afar.
   - Textures come from `Tools/Blender/looter_textures.py` (sets `GroundThatch`, `GroundStony`, `GroundNoise`; the thatch's height and
     the stony ground's leaf litter ride in the colour map's alpha). The master is rebuilt by `build_world_materials.py M_Terrain`;
     its numbers are `GROUND_DEFAULTS` (repeats, how far the borders wander, the damp, where the contrast fades out), and an area's
     instance may change any of them.
   - Judge a change without Unreal first: `Tools\artrun.ps1 -Script Tools\Blender\ground_preview.py` renders the layers from tour
     viewpoints at eye height (before above, after below) into `Saved/ArtPreviews/Terrain/`. It is a numpy port of the master's code:
     keep it in step with `GROUND_WEIGHTS` and `GROUND_DEFAULTS`. It has no shadows, grass or hollows' damp, so look in the editor too.
7. Guns from parts: `Art/Models/Weapons/<Gun>.py` models the parts (sockets chain them; sights carry `SOCKET_Aim` for aiming down
   sights). `<Gun>.parts.csv` lists each part's key, name, name word, rarity and stat ranges in percent (capped per stat,
   `Weapons/WeaponParts.h`). After importing, run `Tools/Unreal/setup_gun_parts.py` in the editor to fill the gun's definition from
   the spreadsheet.
   - The Drover (2026-10-09), the third family (Bullpup rifle, Ranchhand shotgun, Drover revolver on pistol ammo):
     `Art/Models/Weapons/Drover.py` and `Drover.parts.csv`; parts by key as the others (frame, barrel, cylinder, sight, grip; never
     rename or reuse a key).
   - A new kind needs the C++ first (`EWeaponKind::Revolver`, `EWeaponReloadPart::Cylinder`), then in the editor, in this order:
     `create_revolver.py` (makes `DA_Revolver`, puts it in the default loot table), `setup_gun_parts.py` (the parts from the CSV),
     `create_rank_assets.py` (so the ranks' loot tables take the new gun).
   - The Drover's cylinder swings out on SOCKET_Crane; its chambers are the CSV's Magazine for that cylinder (rarity never changes
     the count). A short gun is held out by its definition's `HoldReach`.
8. Throwables: `Art/Models/Throwables/<Name>.py` (the grave-salt grenade, `SaltGrenade.py`) becomes `/Game/Art/Throwables/SM_<Name>`
   through `Tools\models.ps1 -Only <Name>`. Code loads it by path (`AGraveSaltGrenade`, `AGrenadePickup`), and a path is not a
   reference: `/Game/Art/Throwables` is listed under `DirectoriesToAlwaysCook` in `Config/DefaultGame.ini`, so a packaged game has
   it. A new category folder needs the same line.

Rules that saved time:
- Exports must be repeatable. Never iterate BMesh sets (their order changes between runs). Export twice and compare; only FBX IDs
  and time stamps may differ.
- Faces without UV area give tangent warnings; `Tools/Blender/tangentcheck.py` predicts them.
- Terrain meshes keep 100% Nanite fallback. Medium draws the fallback and collision is cooked from it.
- A variant of a model (Farmhouse_Ransom) goes in the same script, with the original exported byte-identical.

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

## Sounds (script-made, 2026-10-08)

- The user's rule: sounds are synthesized from scratch, like the scripted models: no recordings, libraries, AI generators or new
  programs. Blender 4.4's bundled Python has numpy (no system Python on this PC).
- Make: `Art/Sounds/lib` (synthesis library), `Art/Sounds/recipes/<group>.py` (one recipe per cue, seeded takes). `Tools\sounds.ps1
  [cue patterns]` renders `Art/Sounds/Out/<Cue>_NN.wav` and `cues.json` (each cue's mix) and runs `check.py` (peaks, DC, clicks, loop
  seams, harsh bands; spectrograms and a listening page in `Saved/SoundCheck/`). Agents can't hear: they judge by numbers and
  pictures; the user listens.
- Into the game: `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_sound_bank.py [check]"` imports the WAVs to
  `/Game/Audio`, makes the classes (under `SC_Master`), the mix and attenuations, and fills `DA_SoundBank`.
- In code: `LooterSound::PlayAt/PlayAttached/Play2D/Start/Stop` by cue name (`Audio/LooterSoundCues.h`). A cue without sounds plays
  nothing, so code can call new cues first. Add a cue to the header and to `cues.json` together.
- Groups (each a recipe file listed in `recipes/__init__.py`'s `GROUPS`; an unlisted file is never rendered). Cue name namespaces:
  `LooterSoundCue::Lootables`, `::Throwable`, `::Voice`, `::TownLife`, `::Revolver`, `::Economy`, `::BellRinger`, `::Shrine`,
  `::Spitter`; `WeaponSounds.h` picks a gun's cues by its kind.
  - 2026-10-09: `props` (the lootable world), `throwables` (the grave-salt grenade), `voices` (the Unpaid's murmured barks, one
    syllable a cue; the spiders' and slimes' idle calls; the living heard through walls on Ransom's Rest). The Drover's shots and
    reload steps are in `guns`, the fast-travel whoosh in `ui`.
  - 2026-10-12: `economy` (silver found, 2D and the player's own: `Loot.Silver.Pickup`; vendor: `Vendor.Buy`, `Vendor.Sell`,
    `Vendor.NotEnough`), `bells` (the Bell-Ringer's bronze hand bell), `shrines` (the Vigil Stones; the iron bell is `bells.py`'s hand
    bell made duller, so `shrines.py` imports from it). The Venom Spitter's cues are in `creatures`. The pickup chime keeps the
    2-5 kHz band cut, since a fight's worth of pickups sounds it over and over.
  - 2026-10-13 (69 cues): `givers`, `graves` (a grave marked as the save point), `holds` (the hold ring's completion, kept in the
    frequent-cue 2-5 kHz check), `lostloot`, `missions`, `orelift`, `watchfire`, `miners` (the dead miners; Hiram's and Thaddeus's
    sounds), `foreman`. Stamps on the cards land 0.11 s in, on the cards' slam.
  - Lessons (`Art/Sounds/README.md`): start soft cues on a small physical touch (the check flags a late start); put a loop's seam in a
    lull between events; mix loop layers by loudness, not peak.
- For the user: the listening page is an Artifact with the WAVs as its files (<https://claude.ai/artifact/VDmyrPmAsxiNa6boNuqKxa>);
  republish it from the session's scratchpad copy after a render.
- Lesson: setting a USoundClass's `child_classes` all at once parents only the first new child; add them one at a time
  (`build_sound_bank.py` does).

## Packaging a test build

- Run: close the editor (`Tools\close.ps1`; no game running either), then start `Tools\package.ps1` in the background.
  - Options: `-Clean` (full rebuild and recook), `-NoZip`, `-DebugInfo` (keep .pdb files), `-OutDir <folder>`.
  - The first run is long (every shader compiles for D3D12 SM6 and D3D11 SM5); later ones reuse the derived data cache.
  - Progress: `Saved\Logs\Package.log` (UAT's whole log). A failure prints the first errors.
- It makes `Saved\Packaging\<date-time>\Windows\` (run `AI_Looter_Shooter.exe`) and beside it
  `AI_Looter_Shooter_Test_<date-time>.zip`; the script prints the zip's path, size and time taken. A Windows Development build: logs
  and the console (`~`, the `Looter.*` commands) stay in. `-prereqs` adds the Visual C++ runtime installer.
- Cooked (`Config/DefaultGame.ini`, `[/Script/UnrealEd.ProjectPackagingSettings]`): `Lvl_TutorialIsland` (also the menu's map) and
  `Lvl_RansomsRest` with everything they refer to, plus the `DirectoriesToAlwaysCook`. Left out: `Lvl_Skyreach`, `Maps/Dev`,
  `Developers`, the Skyreach props, editor content. `LooterEditor` is an Editor module; the MCP plugins (`ModelContextProtocol`,
  `AllToolsets`) are Editor-only in the .uproject.
- Rule: a path in C++ is not a reference. The cooker follows references only. A `LoadObject`, `FindIfMade`,
  `TSoftObjectPtr(FSoftObjectPath(...))` or `TryLoadClass` on an asset nothing else refers to finds nothing in a package, and the
  `FindIfMade` ones quietly draw a cube instead. Keep such assets under a directory in `DirectoriesToAlwaysCook` (or add its
  directory). After a new package, look for stand-in shapes: the jetty, depot, train, egg sacs, lanterns, chests, fonts.
- Share: a Drive, OneDrive or Dropbox link to the zip (no new upload tool without the user's OK). The friend unzips somewhere
  writable (not Program Files) and runs `AI_Looter_Shooter.exe`; the first launch compiles pipeline states while the level loads. A
  GPU without D3D12 SM6 starts it with `-dx11`.
- Saves and logs sit beside the game, not in AppData: a packaged Development build isn't an "installed" one, so they go to
  `<unzipped>\AI_Looter_Shooter\Saved\` (SaveGames, Logs, Crashes). Ask a tester for the log after a bug.

## The Style Lab (art styles previewed in the game's scene, without Unreal)

- `Tools/StyleLab/` (README there): a three.js rebuild of a piece of Ransom's Rest from the game's own sources with the in-game HUD
  over it, so art styles can be judged "in game" from a cloud session with no Unreal. Built 2026-10-08: ten styles, the user's picks
  3, 4, 5, 6 (3 and 6 were tried on the island). Style 0 is today's look.
- `Docs/Art/StyleLab/Catalog.md`: all ten styles with Unreal-terms numbers, recipe, cost, refs; contact sheets in
  `Docs/Art/StyleLab/Sheets/`; briefs and research beside it.
- Export: `Tools/StyleLab/export/export_all.sh` (Blender's `bpy` 4.5 module under Python 3.11); `assemble.py` builds
  `Saved/StyleLab/site/`. The artifact host serves no `.glb` or `.bin`, so models and heights go as base64 JSON (`--b64-glb`; packs
  under about 9.5 MB so each JSON stays under 16 MB); a publish over 64 MB goes in two calls.
- Cloud sessions commit text only (see the bug list: LFS uploads are refused); binaries live under `Saved/StyleLab/`.

## Performance

- Target, rules and logged numbers: `Docs/Performance.md`. Per-pass budgets: `Docs/TutorialIsland.md`. Lumen and Nanite are switched
  by `UGraphicsSettingsSubsystem::QualitySettings`.
- `perf.ps1 -GpuStats` records each pass; `Tools\perfdiff.ps1` compares two captures; `-Map` measures another level.
- Measure with the editor closed and Blender paused (create `Saved\ArtPause.flag`, delete it after):
  - `Tools\tour.ps1` for every view;
  - `Tools\perf.ps1 -Label ... -Exec "Looter.Quality Medium" -Map <map>`, which appends a row to `Docs/Performance.md`.
- Per pass: `perf.ps1 -GpuStats -Exec "Looter.Quality Medium,Looter.Tour <views.json> noshots" -Frames 16000 -Skip 0 -NoRecord`, then
  `Tools\perfviews.ps1 <csv>`. Pass the views file: without it, `Looter.Tour` walks the tutorial island's list.

## Tests

- `Tools\runtests.ps1` runs one batch per test group, with `obj gc` between batches (all in one batch let test worlds' GPU buffers
  pile up until the driver removed the device).
- Test worlds are editor worlds. Traps:
  - A movement component gets no capsule there: call `SetUpdatedComponent` and set the movement mode yourself.
  - Blueprint native events are dropped: open an `FEditorScriptExecutionGuard`. See `Tests/LocomotionTestWorld.h` and
    `BossTestWorld.h`.
  - Damage through `UGameplayStatics::Apply*Damage` is dropped before play begins: broadcast the actor's damage events the way
    `BossTestWorld::Hurt` does.
- A test's name must not be the start of another test's name (`Looter.Economy.Offer` beside `Looter.Economy.Offer.Odds`;
  `Looter.Locomotion.Pace` beside `.Pace.Chases`, fixed by naming it `.Pace.Speeds`): the runner takes it for a group and reports it
  "Not Run" (happened twice on 2026-10-13). Name every test as a leaf.

## Bugs and how they were fixed

Each line: bug (date). Cause. Fix.

- Game crashed at start in Enhanced Input ("Unhandled tracking mode", 2026-10-09) though the build and every test passed. Cause: an
  agent added members to `KeyBindingSubsystem.h` mid-build, so files compiled against the old header were never rebuilt and read
  `CharacterContext` at the old offset (an input action). Fix: touch every changed header before the integration build; no builds
  while agents edit headers; after each integration build start the game once (`menushots.ps1`/`hudshots.ps1`): tests don't add
  input contexts.
- The tutorial island's dressing build put 38 of Ransom's Rest's pieces on it (graves, dead trees, bales, fallen pines). Cause:
  `build_area_dressing`'s `EXTRA_GRAVE_ROWS` and `SPOTS` applied to every layout. Fix: only on Ransom's Rest (`EXTRAS_AREA`).
- Skyreach's scattered trees all vanished with the town. Cause: Web Hollow's no-tree clearing sits on the forest rise's middle, where
  the scatter's woods grew densest. Fix: the concept's woods round the hollow placed as town pieces (`make_town.py`).
- A bench blocked the forest road at the town square. Cause: the concept's two memorial benches were exempt from the road check (the
  plaza core, 6.5 m). Fix: only the memorial itself is exempt (3 m); the path probe found it.
- The view jerked at a slide's start and end (the user, 2026-10-08). Cause: the engine's crouch sets the body mesh's relative Z
  without moving it, so the camera (on the body) sat ~24 cm off for a frame after a stand outside the movement tick; in the air a
  crouch resizes the capsule about its middle; two added smoothsteps of different lengths; alphas that reversed speed in one frame.
  Fix: `RefreshBodyTransform` each tick and after our own stand; the eye tracks its height above the feet on one minimum-jerk curve
  (`FViewEase`, re-planned from speed and acceleration), as do the roll and stance blends; `Looter.Locomotion.Slide.View` checks
  every frame of seven endings.
- The slide stopped the player abruptly. Cause: a straight 0.2 s ease to crouch speed. Fix: a smooth ease to the next speed; forward
  held goes back to the sprint (73aa50f).
- The cloud session's commit with PNGs and a font would not push. Cause: the cloud refuses Git LFS uploads (`lfs.github.com` verify:
  Forbidden). Fix: from the cloud, commit text only: inline small assets as data URIs (`Tools/StyleLab/web/hud/assets.js`), keep
  generated binaries in `Saved/`.
- The Style Lab's trees had black fringes, the valley's grass turned yellow-red. Cause: Pillow's `resize()` premultiplies RGBA, so
  colour under alpha 0 was lost; lossy WebP drops it too. Fix: resize RGB and alpha separately, save cut-outs lossless with
  `exact=True`, write macro maps as RGB with the alpha in its own file.
- The Style Lab's aerial views had horizontal stripes on far terrain. Cause: GTAO read the depth buffer's coarse far steps as
  terraces, and a fixed shadow bias drew acne in the wide far cascade. Fix: AO allows for the depth step and samples wider far away
  (near plane 0.12 m); shadow lookups offset along the surface normal, more on slopes turned from the sun.
- The editor crashed in the boss bar test (HUD upgrade). Cause: `Outline.Add(Outline[0])`; TArray asserts when it adds a reference
  to its own element (the add may reallocate). Fix: add a copy, `Outline.Add(FVector2D(Outline[0]))`. Grep new code for `X.Add(X[`
  before building, recursively: Windows PowerShell's `Select-String -Path Source\**\*.cpp` is not recursive and missed the cracked
  coin's `Points.Add(Points[0])` (2026-10-08). Use `Get-ChildItem Source -Recurse -Include *.cpp,*.h |
  Select-String '(\w+)\.(Add|Emplace)\(\s*\1\['`.
- HUD text came out a third bigger than the mockup. Cause: Slate sets a font's Size in points at 96 DPI (Size 18 draws 24 px); the
  mockups use CSS px. Kept: the user chose the larger text (2026-10-07), so mockup px stay the Slate Size; px × 0.75 would match.
- Keycap corners drew twice their size; the boss hatch leaned 26°. Cause: Slate sizes a Box brush's ends and repeats a tiled brush in
  the texture's own texels, not by `ImageSize`. Fix: make such textures at 1 texel per slate unit (`PaintedIconBrush` at
  PixelsPerUnit 1).
- The first test build failed to compile (11 errors). Cause: `ADirectionalLight::GetComponent()` and
  `UStaticMesh::IsNaniteEnabled()` exist only in editor builds; the lighting code read the sun through the first, so a packaged game
  would have broken. Fix: `Cast<UDirectionalLightComponent>(Sun->GetLightComponent())`; the test's Nanite check under `WITH_EDITOR`.
  Editor builds never catch these: package to find them.
- Editor automation plugins would have shipped in the game. Cause: `ModelContextProtocol` and `AllToolsets` had no `TargetAllowList`
  (one is marked NoRedist). Fix: `"TargetAllowList": ["Editor"]` in the .uproject.
- The first cook stopped: "Content is missing from cook". Cause: the engine's Landmass plugin loads startup materials that refer to
  `/Engine/EditorMaterials`, which `-SkipCookingEditorContent` marks never-cook. Fix: the flag dropped from `package.ps1` and the ini
  (`bSkipEditorContent=False`).
- `Tools\close.ps1` said "EDITOR DID NOT CLOSE" with no dialog. Cause: a floating Content Browser held the process's main window, so
  closing it only closed the panel. Fix: `close.ps1` asks the editor to `QUIT_EDITOR` when the main window isn't the editor.
- A hit tinted the whole screen red. Cause: the old hurt camera fade (`PlayerVitalsSubsystem`, up to 55%) on top of the new red
  edges. Fix: the fade on hits removed; the HUD shows hits (edges, portrait, chip); death and respawn fades kept.
- The gun's name stopped short of the right edge, its gem far from it. Cause: a scale box fitted the name to the 24 px line's height
  (shrinking every name) and centred the rest. Fix: no scale box; `FitWeaponName` measures a long name and sets it smaller.
- The Gravemother spawned stuck in her den's wall. Cause: the den's floor is part of Den Rock, tagged Obstacle, so its spots were
  refused; the fallback spot was checked with a man-sized capsule. Fix: room for the largest body; the floor inside a lair's own rock
  counts as ground (`FEncounterGroundProbe`), f3e3579.
- A giant rock blocked the tutorial island's lookout ramp. Cause: a cliff run's end piece, stretched to meet its one neighbour,
  reached across the ramp. Fix: walkways cleared with a capsule test (8e0e50c, bd96147).
- A lump came through the burial deck. Cause: the Rim's top cliff course stood under the deck's open end. Fix: `level.cliffs.under`
  cuts pieces 40 cm under the boards (732b43d).
- The Sink's walls became floating rock fragments. Cause: faces brought 50 cm proud through a noisy, curving wall. Fix: old placement
  for theSink; then the art's narrow panels (de5a8fe).
- The egg sacs couldn't be shot. Cause: a new sampling constant reused the name `SAC_STEP` (the mission step). Fix: renamed
  `SAC_PROBE_STEP`; the Keeper's Lantern test caught it.
- Hillside scrub read as even dots. Cause: thick patches asked for more plants than the candidate spacing held, so it drew a grid;
  the 10 m patches averaged out at range. Fix: 1.5 m spacing, 15-40 m patches, varied sizes (7c305cc).
- Spiders caught a walking player. Cause: the player got 15% slower and spider chase wasn't scaled with it. Fix: creature speeds
  times `SpeedScale` (6b03dff).
- Fence panels leaned on slopes. Cause: every section rolled to the slope. Fix: plumb level sections stepped down slopes (c49039f).
- The scatter script failed in a fresh editor. Cause: it imported a neighbour module, relying on `build_area.py` having added its
  folder to the path. Fix: it adds its own folder now.
- Gravewind blades drew black. Cause: the material lacked the instanced-mesh usage flag. Fix: flag set (3a04a48).
- Saves failed quietly. Cause: the user's standalone game held the content files. Fix: check for a running game before editor work.
- After Save & Quit then Continue, no damage numbers, creature tags or loot labels (the user, 2026-10-13). Cause: they draw on the
  engine's `WidgetComponentScreenLayer`, which the game viewport keeps per player for the whole run; the pause menu collapsed it,
  Save & Quit left from there, and the next level's HUD remembered "shown" for itself (`bWorldLabelsVisible`) so never showed it
  again. A trip from the map page or a station board did the same. Fix: `ALooterHUD::ShowWorldLabels` sets it from what the layer
  shows now, every frame, and at BeginPlay/EndPlay (`Looter.LevelReload.WorldLabels`). Rule: anything kept by the viewport, local
  player or game instance (layers, input mode, cursor, mixes) outlives the level; set it from its real state, never a per-level copy.
- After Save & Quit, hostiles stopped showing on the minimap (2026-10-13). Cause: its 32 markers were handed out in drawing order
  (ammo, loot, hostiles), and every Continue gave every ammo and silver pickup a fresh 5 minutes, so a player saving every few
  minutes kept every drop round the fights (a log: 59 to 260 ammo pickups over six loads). Fix: hostiles get markers first
  (`SplitMarkers`, `Looter.UI.Minimap.MarkerPool`); the session keeps each pickup's time left (`LifeLeft`,
  `Looter.LevelReload.LootLife`).

## What worked, and what didn't

- Worked:
  - Probes before fixes. A read-only script that logs real numbers (gap depths, overlaps, coverage) settled every look problem
    faster than guessing.
  - Dry runs in plain Python (a mocked `unreal` and the generator's heights) catch logic errors before the editor. They miss
    everything that needs real geometry, so always tour after.
  - Data-driven rules in `layout.json`, so another area can reuse them.
  - One commit per step, and partial CODEMAP staging.
  - Asking the art session for its call on looks, with in-game shots.
  - Settings that preview live under the pause menu (2026-10-12): a paused world doesn't tick the player controller's camera
    updates, so a camera setting set from the menu shows only once the game runs again.
    - The field of view slider calls `UPlayerViewComponent::PreviewFieldOfView`, which sets the camera and locks
      `APlayerCameraManager::SetFOV` to it until the first frame that runs.
    - The menu's dimming is its own layer so the slider can fade it and ghost the panel (`SettingsMenuPreview.cpp`).
    - The HUD's minimap is collapsed under the menu, so its zoom preview is a second `UHudMinimapWidget` (`SetPreview`) in the same
      corner.
    - The previews count a controller's capture and a held key's repeated moves as holding, so a slider stepped from the keyboard or
      a gamepad shows its preview while it is held.
  - Menus for a controller and the keyboard (2026-10-12), the main menu's and the settings menu's alike:
    - `FMenuNavigator` (`UI/Menus/MenuNavigator.h`) keeps the focus over a menu's stops (a button, a slider, a row of choices) in
      rows, which the menu hands it with `SetRows` each time it rebuilds.
    - `MenuNav` has the keys as plain rules: arrows, W A S D, the D-pad and the left stick move; Enter, Space, E and A pick; Esc and
      B back out; Page Up and Down or the bumpers turn the sections; a slider held steps faster.
    - `SettingsMenuNav.cpp` and `MainMenuNav.cpp` wire each menu to it. A new menu gets the same by building its stops and calling
      the navigator, not by handling keys itself.
    - The focus look (`LooterUIFocus.h`: an orange outline doubled inside, a lit edge on the left, a faint wash on a row) is drawn
      only while the last input in a menu was a key or the gamepad (`LooterUI::IsMenuFocusShown`); under the mouse the buttons keep
      their hover look. The prompts along the bottom name the gamepad's buttons or the keyboard's keys by `IsMenuGamepad`. Give a new
      row its look with `LooterUI::AddFocusFrame` while building.
    - Tests: `MenuNavTests.cpp`.
- Didn't work:
  - Pushing generic cliff faces proud of a noisy pit wall: they poked through as fragments.
  - Level-wide material swaps for one actor (use `swapsOn`).
  - Judging scrub in stand-in renders at Blender culls; preview at the game's Medium cull distances instead.
  - Line-by-line `perl -pi` or regex edits with `\n` in the pattern on CRLF source files: they match nothing and fail silently. Use
    the Edit tool, or allow `\r?` before the newline, and check the result.
  - A file-scope name in a test (`const FName Extra`) that an engine header also uses: C4459 fails the build.
  - A foreground build or test run longer than the tool's 10 minutes is killed with no log: run them in the background.
  - Chaining `git add -u && ... ; git commit`: when `git add` failed on the transient "Permission denied", the commit still ran with
    whatever was staged. Stage, check, then commit.
  - Starting a build without checking whether the user was playing (step 1 of the loop).
