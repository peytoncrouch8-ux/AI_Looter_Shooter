# Pipeline

How things get made and fixed in this project, so nothing is worked out twice. Update it whenever work finishes: a new
way of doing something, a bug and its fix, something that didn't work. `CLAUDE.md` has the rules; `Docs/Handoff.md` has
where the work stands.

## The game

- **Story:** *Revenant* (`Docs/Story.md`). A weird-west campaign on the planet Teropa. Ellis Ransom is shot dead at
  Ransom's Point by the gang that stole the saints' embers, wakes in the family plot, and makes a deal with Mister Sexton:
  seven embers at the new moon, and Pa crosses. One area per Hollow; each area has its own design in `Docs/Areas/`.
- **Areas:** the tutorial island (called Skyreach in the game) teaches the basics; Ransom's Rest is the first campaign
  area (done 2026-10-08). The next, the *Gilded Lily* (Lucky Ned's casino), is on hold.
- **Look:** stylized realism: real lighting and textured materials, still a little stylized. The user rejected faceted
  low-poly. Golden-hour palettes on Ransom's Rest (ochre, olive, sandstone), late summer. Loot colours (rarity orange,
  teal, gold) stay off the world.
- **UI:** "Concept C" (`UI/Style/LooterUIStyle.h`): dark glass panels, orange accents, cyan lines, Chakra Petch. The HUD
  has no panels. Icons are Inked icons drawn in code.
- **Feel:** every action should reward the player: tracers, impacts, muzzle flash, spring recoil, hit flash, sound.
- **Selling:** nothing copyrighted. All models are scripted in Blender here, textures are made here, names are original.
- **Player numbers (2026-10-08):** 0.85 of the mannequin's size; walk 510, sprint 790, crouch 255 cm/s; eye at about
  140 cm; jump 103.5 cm. A slide comes out of sprint plus crouch. Creature speeds scale with `LooterPlayerSize::SpeedScale`.

## Who does what

- **The main session** (the orchestrator, on Opus 5.5) owns Unreal, builds, the asset scripts, the tests, play checks,
  git and the docs. It splits work among at most 8 agents and checks everything they hand back before it goes in.
- **Models per agent** (the orchestrator's call, to get the most from the usage):
  - the strongest (Opus) for design and tricky code: new systems, rasterizers, widgets with timing and layout logic;
  - Sonnet for well-specified code against a contract, reviews, doc edits and mechanical changes across files;
  - Haiku for simple searches and listings.
- **Parallel agents and a shared contract** (first used for the HUD upgrade, 2026-10-07). Before launching agents
  that touch each other's code, Main writes one contract file (in its scratchpad) that every brief points to: each new
  class's public API with exact signatures, new palette entries with values, where each widget is placed and by whom,
  and which agent owns which files. The agents then write disjoint files at the same time, without waiting for each
  other, and Main builds them together once.
- **Agents** write code or scripts on disjoint files. They never build, open Unreal or touch git. They report the files
  they changed, what Main must run, the CODEMAP lines, the tests affected, and the risks they couldn't check. When an
  agent needs editor numbers, it writes a read-only probe (`Saved\<short>.py`) and Main runs it.
- **The art session** (a separate session) models in Blender (`Art/Models/**`), never commits and never opens Unreal.
  It exports into `Intermediate\ArtExport_<Family>`, then messages Main with the files, triangle counts, pivots and
  sockets. Main imports and commits, and sends back in-game shots to check.
- **Main's helpers** (in `Tools`, so a new session has them):
  - `Tools\commitlib.py`: `commit(files, codemap_edits, title, body)` stages files plus CODEMAP edits applied to HEAD's
    copy, so other uncommitted CODEMAP lines stay out. Edits an agent already wrote into the working copy are skipped
    there.
  - `Tools\stage_import.ps1 <export folder> <model,model>`: copies chosen models and their materials into
    `<folder>_Import`, for `Looter.ImportModels`.
  - `Tools/Unreal/open_clean.py [map]`: collects garbage, then loads a map from disk.
  - `Tools\winshot.ps1 <handle>`: captures a window with PrintWindow; use it on a Save Content dialog.
  - `Tools/Unreal/pie_check.py`, `path_probe.py` and `width_probe.py`: described below.
- **Briefs that worked:** give the agent the user's exact words, the files it owns, the files it must not touch, the
  run order Main will use, and what to report. Say "edit with the Edit/Write tools only" (a PowerShell `-replace` once
  corrupted a script). A usage limit can stop agents mid-step: resume each with SendMessage, saying what landed on disk.

## The everyday loop

1. **Before any editor work,** check that the user isn't playing:
   - `Tools/Unreal/pie_check.py` for a play session in the editor;
   - `Get-CimInstance Win32_Process` for `UnrealEditor.exe ... -game` (the launcher's game).
   While they play, run no console commands and no builds. Saves fail quietly on files the game holds.
2. **Build:** `Tools\launch.ps1 -Build`. If it says "EDITOR DID NOT CLOSE":
   - a "Save Content" dialog is open; find it with `Tools\editorwindows.ps1` and capture it with PrintWindow;
   - read its list, and choose Don't Save unless those assets were meant to change.
3. **Scripts:** run the asset scripts the change needs, through `Tools\console.ps1 "py <absolute path>"`.
   - Keep the command under about 200 characters, or it's dropped.
   - Each `build_area.py` mode saves the level itself.
4. **Tests:** `Tools\runtests.ps1`, all green. `-Filter Looter.<Group>` runs a subset.
   - Tests leave the level dirty, so reopen it clean afterwards (`Tools/Unreal/open_clean.py`). Otherwise closing the
     editor can save the tests' leftovers into the level (this happened once: restored from git).
5. **Look:** run a tour with `Tools\tour.ps1 -Map <map> -Views <json>`.
   - Scratch view lists go in `Saved/TourViews/`; the screenshots land in `Saved\Screenshots\Tour`.
   - Read every screenshot before calling a visual change done.
6. **Commit and push** each working step on its own.
   - The commit message says why, and ends with the co-author line.
   - To commit part of a shared file (CODEMAP), stage edits on HEAD's copy (`Tools\commitlib.py`), so other uncommitted
     work stays out.

## Making a level (grounded areas)

- **Data:** `Art/Levels/<Area>/layout.json` (hand-written: features, roads, zones, obstacles, placements, the `level`
  block, gameplay). `views.json` holds the tour viewpoints.
- **Generator (Blender, headless):** the `Art/Levels/area_*.py` modules build the terrain tiles, the ring, the canyon wall
  and the backdrop. They also write the macro, scatter and scrub masks and `layout_computed.json`: cliff points,
  far trees, scrub points, boundary.
  - `Tools\terrain_identity.ps1 -Area <Area>` checks the generator still rebuilds an area byte for byte. Run it after
    any generator change, once the change is committed (it compares with HEAD).
- **In the editor:** `Tools/Unreal/build_area.py <Area> [mode]`. With no mode it does the full build. The modes:
  - `gameplay`: the story pieces and creatures (`build_area_story.py` and its `build_area_<place>.py` scripts);
  - `environment`: the light, sky and fog;
  - `beyond`: the ring, backdrop and far trees;
  - `cliffs`: the cliff faces and outcrops;
  - `dressing`: fences, walls, graves and props (`build_area_dressing.py`).
- **Cliffs** (`build_area.py` cliffs()). The `level.cliffs` settings:
  - `varied` groups are mirrored, sunk and turned.
  - `lean` groups lean with the wall.
  - The lip ceiling keeps tops under a rounded rim.
  - `leanTop` and `leanProud` can be set per group.
  - `abut` stretches a run's end into a rock, at most 2x its width.
  - `under` caps cliff tops under a platform (`build_area_platforms.py`; the burial deck).
  - `panels` is a run of narrow panels along a pit's curve (`build_area_panels.py`), with `talus` at their feet
    (`build_area_talus.py`).
  - Every ramp's walkway is cleared of cliff pieces (`build_area_walkways.py`): a player capsule is tested down its middle.
- **Scatter** (PCG): `Tools/Unreal/build_island_scatter.py <Area>`.
  - It re-imports changed masks by their MD5, then generates. Wait for the instance count to settle, then save.
  - It keeps off the dressing's footprints and wears the area's material swaps.
  - Run `dressing` before the scatter when fences or graves move.
- **Materials per area:**
  - `level.materials` makes area instances of shared MIs.
  - `level.swaps` puts them on everything the build placed, and on the scatter.
  - `level.swapsOn` swaps on named actors only (the windmill's steel).
- **Path checks:** `Tools/Unreal/path_probe.py <Area>` walks a player capsule down every road and ramp;
  `Tools/Unreal/width_probe.py` probes across one. Run them after cliff or dressing changes.

## The tutorial island and the older levels

- **Tutorial island:** built by scripts from `Art/Levels/TutorialIsland/layout_computed.json` (the terrain model writes
  it). `Tools/Unreal/build_tutorial_island.py` places the terrain, cliffs, buildings, lighting and gameplay actors, and
  rebuilding replaces only what it placed; its `gameplay` mode places just the spawn, dummies, spiders and slimes again.
  `Tools/Unreal/build_island_scatter.py` scatters grass, flowers, trees and rocks with PCG from the scatter mask.
  `Tools/Unreal/review_stage.py` photographs new models under the island's lighting.
- **Older levels** are built in the editor. Procedural props are `StylizedProp` actors (shape, seed, two colors).
  Before committing such a level, run `Looter.BakeLevelProps`: it swaps them for static mesh actors and saves their
  meshes and materials under `/Game/Environment/Props`.
- **Seating props:** after placing props or changing terrain, run `Tools/Unreal/conform_hills.py` (terrain changes
  only: it fits the hills' rims under the ground), then `Looter.SettleProps` (`selected` for the selection), and save.
  It seats every prop so no edge hovers, leaning low, wide ones with the slope.
- **Lvl_Skyreach's meadow:** grass and flowers come from the `Meadow` PCG volume (`/Game/Environment/PCG/PCG_Meadow`),
  which raycasts onto `Ground` actors and avoids `Obstacle` ones. After changing terrain, select it, press Generate and
  save. Ground cover never collides (a placed static mesh actor takes its mesh's collision unless
  `bUseDefaultCollision` is off). Patches are about 3.5 m and lie on the slope; `World/PCGGroundFitFilter` drops the
  ones that would hang off an edge, and `Looter.BakeGroundCover` bakes their meshes again.

## Making an asset

1. The art session scripts the model in Blender (`Art/Models/<Category>/<File>.py`), with shared material names and
   texture sets from `Art/Textures/<Set>`.
2. It export-tests: `Tools\artrun.ps1` or `Tools\models.ps1 -NoImport -Out Intermediate\ArtExport_<Family>`.
3. Main stages only the new models (`Tools\stage_import.ps1 <folder> <names>`), runs
   `Looter.ImportModels <folder>_Import`, checks the import log (bounds, sockets, collision), and commits the `.py` with
   the `.uasset`s.
4. A colour-only change in the manifest can be set on the MI directly instead of reimporting a skeletal mesh (the
   lantern glow).
5. **Tools:** `Tools\artrun.ps1` runs a model script (`-Preview` renders it), any Blender script, or an export test,
   at below-normal priority, waiting while `Saved\ArtPause.flag` exists. `Tools/Blender/tangentcheck.py` checks an
   exported FBX's tangents as Unreal's import will; with `--log` it sorts the editor log's tangent warnings into the
   model's own and Unreal's reduced builds'.
6. **Materials:** the textured masters (`M_World`, `M_Gun` with per-gun wear, `M_WorldFoliage`, `M_Terrain`,
   `M_Water`) are built by `Tools/Unreal/build_world_materials.py`. Older surfaces use the flat stylized materials
   (`M_StylizedSurface`, `M_StylizedFoliage`, `M_StylizedGlow`).
7. **Guns from parts:** `Art/Models/Weapons/<Gun>.py` models the parts (sockets chain them; sights carry `SOCKET_Aim`
   for aiming down sights). `<Gun>.parts.csv` lists each part's key, name, name word, rarity and stat ranges in percent
   (capped per stat, `Weapons/WeaponParts.h`). After importing, run `Tools/Unreal/setup_gun_parts.py` in the editor to
   fill the gun's definition from the spreadsheet.

Rules that saved time:
- Exports must be repeatable. Never iterate BMesh sets (their order changes between runs). Export twice and compare; only
  FBX IDs and time stamps may differ.
- Faces without UV area give tangent warnings. `Tools/Blender/tangentcheck.py` predicts them.
- Terrain meshes keep 100% Nanite fallback. Medium draws the fallback and collision is cooked from it.
- A variant of a model (Farmhouse_Ransom) goes in the same script, with the original exported byte-identical.

## The Style Lab (art styles previewed in the game's scene, without Unreal)

`Tools/StyleLab/` (README there) rebuilds a piece of Ransom's Rest in three.js from the game's own sources and lays the
in-game HUD over it, so art styles can be judged "in game" from a cloud session that has no Unreal (2026-10-08: the ten
styles the user chooses from).

- **Export** (`Tools/StyleLab/export/export_all.sh`, Blender's `bpy` 4.5 module under Python 3.11): the scripted models
  (GLB packs), the texture sets (webp), the terrain from the area generator, and every placement the editor build makes:
  `Tools/Unreal/build_area.py RansomsRest` runs unchanged under `mock_unreal.py`, with traces hitting the exported
  terrain. PCG scatter goes over as rules plus masks. Output in `Saved/StyleLab/export/`.
- **Engine** (`web/engine/`): sun with soft cascades, sky dome (clouds, stars, planets, moons), height fog, HDR with
  MSAA, a post chain, material builders (`pbr`, `toon`, `flat`, terrain) with a GLSL hook, walk-and-shoot play with
  spiders, and six fixed shots rendered as repeatable stills (`test/shots.mjs`, software WebGL in headless Chromium).
- **Styles** are one module each (`web/styles/NN_<id>.js`): light, sky, fog, materials, post and effect colours, plus a
  card with references, an Unreal recipe and a cost estimate. Style 0 is today's look.
- **HUD** (`web/hud/`): the approved mockup with the in-game differences; its icons and font are inlined (`assets.js`).
- **Publishing:** `assemble.py` builds `Saved/StyleLab/site/`. The artifact host serves no `.glb` or `.bin`, so models and
  heights go as base64 JSON (`--b64-glb`; packs under about 9.5 MB so each JSON stays under 16 MB), and a publish over
  64 MB goes in two calls.

## Performance

- **Target:** 8.3 ms (120 fps) at 1080p on Medium, at the heaviest view, on an RX 580 class PC. Per-pass budgets are in
  `Docs/TutorialIsland.md`. Lumen and Nanite are switched by `UGraphicsSettingsSubsystem::QualitySettings`.
- `perf.ps1 -GpuStats` records each pass; `Tools\perfdiff.ps1` compares two captures; `-Map` measures another level.
- **Measure with the editor closed and Blender paused** (create `Saved\ArtPause.flag`, delete it after):
  - `Tools\tour.ps1` for every view;
  - `Tools\perf.ps1 -Label ... -Exec "Looter.Quality Medium" -Map <map>`, which appends `Docs/Performance.md`.
- **Per pass:**
  - `perf.ps1 -GpuStats -Exec "Looter.Quality Medium,Looter.Tour <views.json> noshots" -Frames 16000 -Skip 0 -NoRecord`,
    then `Tools\perfviews.ps1 <csv>`.
  - Pass the views file: without it, `Looter.Tour` walks the tutorial island's list.
- **Ransom's Rest, 2026-10-08:**
  - all 28 views at 149-216 fps (6.7 ms at the heaviest); spawn 6.3 ms;
  - base pass 1.1-1.2 ms, prepass 0.5-0.7, shadows at most 0.5;
  - no single art asset is a real cost.

## Tests

- `Tools\runtests.ps1` runs one batch per test group, with `obj gc` between batches.
  - Running them all in one batch let test worlds' GPU buffers pile up until the driver removed the device.
- Test worlds are editor worlds, which has two traps:
  - A movement component gets no capsule there: call `SetUpdatedComponent` and set the movement mode yourself.
  - Blueprint native events are dropped: open an `FEditorScriptExecutionGuard`. See `Tests/LocomotionTestWorld.h` and
    `BossTestWorld.h`.

## Bugs and how they were fixed

| Bug | Cause | Fix |
|---|---|---|
| The cloud session's commit with PNGs and a font would not push | The cloud's network policy refuses Git LFS uploads (`lfs.github.com` verify: Forbidden) | From the cloud, commit text only: inline small assets as data URIs (`Tools/StyleLab/web/hud/assets.js`), keep generated binaries in `Saved/` |
| The Style Lab's trees had black fringes and the valley's grass turned yellow-red | Pillow's `resize()` premultiplies RGBA, so colour under alpha 0 was lost; lossy WebP drops it too | Resize RGB and alpha separately, save cut-outs lossless with `exact=True`, write macro maps as RGB with the alpha in its own file |
| The Style Lab's aerial views had horizontal stripes across far terrain | GTAO read the depth buffer's coarse far steps as terraces, and a fixed shadow bias drew acne in the wide far cascade | AO allows for the depth step and samples wider far away (near plane 0.12 m); shadow lookups offset along the surface normal, more on slopes turned from the sun |
| The editor crashed in the boss bar test (HUD upgrade) | `Outline.Add(Outline[0])`: TArray asserts when it adds a reference to its own element, since the add may reallocate | Add a copy, `Outline.Add(FVector2D(Outline[0]))`; grep new code for `X.Add(X[` before building |
| HUD text came out a third bigger than the mockup | Slate sets a font's Size in points at 96 DPI, so Size 18 draws 24 px; the mockups use CSS px | Known and kept: the user chose the larger text (2026-10-07), so mockup px stay the Slate Size; a mockup's px × 0.75 would match it exactly |
| Keycap corners drew twice their size; the boss hatch leaned 26° | Slate sizes a Box brush's ends and repeats a tiled brush in the texture's own texels, not by `ImageSize` | Make such brushes' textures at 1 texel per slate unit (`PaintedIconBrush` at PixelsPerUnit 1) |
| A hit tinted the whole screen red | The old hurt camera fade (`PlayerVitalsSubsystem`, up to 55%) on top of the new red edges | The fade on hits removed; the HUD shows hits (edges, portrait, chip); death and respawn fades kept |
| The gun's name stopped short of the right edge, its gem far from it | A scale box fitted the name to the 24 px line's height (shrinking every name) and centred the rest | No scale box: `FitWeaponName` measures a long name and sets it smaller |
| The Gravemother spawned stuck in her den's wall | The den's floor is part of Den Rock, tagged Obstacle, so its spots were refused; the fallback spot was checked with a man-sized capsule | Room for the largest body; the floor inside a lair's own rock counts as ground (`FEncounterGroundProbe`), f3e3579 |
| A giant rock blocked the tutorial island's lookout ramp | A cliff run's end piece, stretched to meet its one neighbour, reached across the ramp | Walkways cleared with a capsule test (8e0e50c, bd96147) |
| A lump came through the burial deck | The Rim's top cliff course stood under the deck's open end | `level.cliffs.under` cuts pieces 40 cm under the boards (732b43d) |
| The Sink's walls became floating rock fragments | Faces brought 50 cm proud through a noisy, curving wall | Old placement for theSink; then the art's narrow panels (de5a8fe) |
| The egg sacs couldn't be shot | A new sampling constant reused the name `SAC_STEP` (the mission step) | Renamed `SAC_PROBE_STEP`; the Keeper's Lantern test caught it |
| Hillside scrub read as even dots | Thick patches asked for more plants than the candidate spacing held, so it drew a grid; the 10 m patches averaged out at range | 1.5 m spacing, 15-40 m patches, varied sizes (7c305cc) |
| The slide stopped the player abruptly | A straight 0.2 s ease to crouch speed | A smooth ease to the next speed; forward held goes back to the sprint (73aa50f) |
| Spiders caught a walking player | The player got 15% slower, and spider chase wasn't scaled with it | Creature speeds times `SpeedScale` (6b03dff) |
| Fence panels leaned on slopes | Every section rolled to the slope | Plumb level sections stepped down slopes (c49039f) |
| The scatter script failed in a fresh editor | It imported a neighbour module, relying on `build_area.py` having added its folder to the path | It adds its own folder now |
| Gravewind blades drew black | The material lacked the instanced-mesh usage flag | Flag set (3a04a48) |
| Saves failed quietly | The user's standalone game held the content files | Check for a running game before editor work |

## What worked, and what didn't

- **Worked:**
  - Probes before fixes. A read-only script that logs real numbers (gap depths, overlaps, coverage) settled every
    look problem faster than guessing.
  - Dry runs in plain Python (a mocked `unreal` and the generator's heights) catch logic errors before the editor.
    They miss everything that needs real geometry, so always tour after.
  - Data-driven rules in `layout.json`, so another area can reuse them.
  - One commit per step, and partial CODEMAP staging.
  - Asking the art session for its call on looks, with in-game shots.
- **Didn't work:**
  - Pushing generic cliff faces proud of a noisy pit wall: they poked through as fragments.
  - Level-wide material swaps for one actor (use `swapsOn`).
  - Judging scrub in stand-in renders at Blender culls; preview at the game's Medium cull distances instead.
  - Starting a build without checking whether the user was playing.
