# Pipeline: Workflow (agents, the loop, play checks, packaging, performance, tests)

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

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
- Sound recipes (round 4 lessons): render EVERY new recipe group's cues (`Tools\sounds.ps1 <patterns>`, then `build_sound_bank.py`) and
  check each is in the bank: round 4's `Player.Underwater.*` cues were missed once (not rendered or in the bank until the play-check
  fixes commit). A cue whose name lives in a header other than `LooterSoundCues.h` (`SwimCue`, `GrenadeKindCue`, `MeleeFX`'s names built
  by key) is not on that list, so compare `cues.json` and `Content/Audio` with the recipe by hand.
  An agent that renders ALL sounds to check its own recipe rewrites `cues.json` and leaves untracked WAVs for recipes still unfinished
  (GRENADES did, round 4): tell agents to render only their own cues (`Tools\sounds.ps1 <their patterns>`), and rerender after a merge.
- Holding trees (rounds 1-4, 2026-10-27, `Docs/Work/2026-10-27-Playtest/CONTRACT*.md`): agents that change C++ work in
  `Saved/<Job><Round>Code/Source/AI_Looter_Shooter/...` (copies of the changed files, the originals' hashes in `HASHES.txt`, new
  files beside them) and write a README there (files, what it does, CODEMAP lines, the exact hooks Main adds in files they don't
  own, run order, tests, risks); Python and art files an agent owns are edited in place. Main checks each `git hash-object`
  still matches HASHES.txt, copies the tree over `Source/`, builds once, runs the scripts, and writes CODEMAP and the pipeline
  notes (a DOCS agent does that at the end of a round). Two agents never own one file: the second writes its exact edit in its
  README for the owner (round 3: INTRO's story lines went to STORY's `create_story_lines.py`; WORLD's SECRETS anchors to STORY).
- A script that sets editor state (World Settings flags, level properties) can raise a modal dialog that blocks every MCP call:
  `Tools\editorwindows.ps1` lists the editor's windows; see `Bugs.md` ("A modal 'build lighting' message").
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
4. Tests: `Tools\runtests.ps1`, all green; `-Filter Looter.<Group>` runs a subset. Tests leave the level dirty: the runner reopens it
   clean afterwards (`Tools/Unreal/open_clean.py`; closing the editor once saved the tests' leftovers into the level) and closes
   the Message Log. `-Restart` also restarts the editor: after two full runs PIE crawled at 1-3 fps (2026-10-14).
5. Look: `Tools\tour.ps1 -Map <map> -Views <json>` (an interior's view gives `top`, a world height under its ceiling where the
   ground search starts; `Tools/cliff_views.py <Area>` writes a view per cliff point). Scratch view lists go in `Saved/TourViews/`; screenshots land in
   `Saved\Screenshots\Tour`. Read every screenshot before calling a visual change done.
6. Commit and push each working step on its own. The message says why and ends with the co-author line. To commit part of a shared
   file (CODEMAP), stage edits on HEAD's copy (`Tools\commitlib.py`).

## Play checks (playing a level through in PIE, 2026-10-14)

- The editor's Play button opens the level that is open: to see the main menu (startup cards off unless `Looter.Startup.InEditor 1`,
  the planet, the hero select, the new game's intro) open `Lvl_MenuTeropa` first; Play from any other map plays that map. Since
  2026-10-27 the menu is `/Game/Maps/Lvl_MenuTeropa` (`Game.md` "Main menu"), not the island.
- Start: `Tools\pie.ps1 -Map RansomsRest` (Looter.PlayLevel: a new game, no session, nothing saved; Main 1 starts at step 3
  with no gun: `Looter.GiveWeapon Rifle Common 1`, then `play.py ammo`; `God` keeps the player up).
- `Tools\playstep.ps1` runs one step: `-Py "<play.py command>"`, `-Cmd "<console>"`, `-Keys` (input.ps1 steps),
  `-Button ACCEPT|CLOSE|...` (a game menu's button: found in the Slate tree, clicked with the real mouse), `-Focus`
  (what Interact would use), `-List` (open missions and the guns), `-Shot <name>` (half-size grab).
- `Tools/Unreal/play.py`: `goto <tag|label> [cm] [yaw]` (stands in front of it on the floor, looking at its meshes),
  `at x y z [yaw]`, `find <word>`, `where`, `look`, `lookat <tag>`, `what [yaw] [pitch]` (what the view's middle meets),
  `press <Interact|Fire|Jump|Reload|Move> [seconds] [x] [y]` (feeds the input action itself: no focus needed; Move's y is
  forward), `fight [s] [m]` (aims at creatures in sight, pulses Fire, keeps ammo full), `kill [m]`, `ammo`, `guns`,
  `equip <slot>`, `view [n]` (F5's cycle), `inventory [close]`.
- Lessons: the console box takes keyboard focus, and only a left click gives the game viewport focus back (it fires a held
  gun), so use `press` for game actions and real keys only for menus (Tab, number keys). Offer and turn-in cards and the
  notice boards take mouse clicks, not injected actions. Teleports skip a roost's find (walk the last 15 m) and give hits
  time to knock the view off a target: check `-Focus` before pressing.
- Never `set_editor_property` on a play session's objects, and never edit the PIE pawn's components in the Details
  panel: a changed value's notice reruns the actor's construction script (`ConsolidatedPostEditChange` ->
  `RerunConstructionScripts`; no PIE guard in 5.8). On a moving player that rebuilt BP_LooterCharacter's Blueprint
  components and the weapon manager came back empty (2026-10-14, twice: the play-through's vanished guns). play.py stops
  the player with `stop_movement_immediately`/`clear_accumulated_forces`; the game warns `Guns: ... lost ...`.
- Probes for walkability: `Tools/Unreal/reach_probe.py` misreads floors inside thick hulls (stairs, the watchtower);
  walking it with `press Move` legs and `where` is the reliable check.

## Packaging a test build

- Run: stop PIE, then `Tools\package.ps1 -CloseEditor` (closes the editor via `Tools\close.ps1`, like `launch.ps1 -Build`) in the
  background. Without `-CloseEditor` it refuses while the editor is open (packaging fights it for files and CPU); it never closes a
  running game (`-game`) or an `UnrealEditor-Cmd` cook. Stopping PIE first is the caller's job.
  - Options: `-SmokeTest` (see below), `-Clean` (full rebuild and recook), `-Zip` (also zip it, to send to someone; opt-in),
    `-DebugInfo` (keep .pdb files), `-OutDir <folder>`.
  - The first run is long (every shader compiles for D3D12 SM6 and D3D11 SM5); later ones reuse the derived data cache.
  - Progress: `Saved\Logs\package_<yyyy-MM-dd_HHmm>.log` (UAT's whole log). A failure prints the first errors. Exit codes: 0 ok, 1 refused,
    2 packaging failed, 3 smoke test failed. It prints the elapsed time and the archive's size.
- Playtest builds land in `Builds\Playtest\<yyyy-MM-dd_HHmm>\Windows\` (git-ignored; run `AI_Looter_Shooter.exe`). `Play Latest Build.bat`
  at the repo root starts the newest one fullscreen at the menu (it needs no editor, and says so if no build exists).
- `-SmokeTest` starts the real game exe (`Windows\AI_Looter_Shooter\Binaries\Win64\`; the root exe is a stub) windowed 1280x720 twice:
  once with no map (the menu: log must show `LoadMap(/Game/Maps/Lvl_MenuTeropa`) and once with `/Game/Maps/Lvl_RansomsRest` named
  (log must show that LoadMap), each 6 s of play after the load with no `Fatal error`/`Assertion failed`, then closes it. Logs:
  `Saved\Logs\package_<stamp>_smoke_menu.log` / `_smoke_ransomsrest.log`. Log checks only: look at a window with `Tools\grab.ps1` yourself.
  A packaged Windows Development build: logs and the console (`~`, the `Looter.*` commands) stay in. `-prereqs` adds the Visual C++
  runtime installer.
- Cooked (`Config/DefaultGame.ini`, `[/Script/UnrealEd.ProjectPackagingSettings]`): `Lvl_MenuTeropa` (the menu's map, the
  default map since 2026-10-27), `Lvl_TutorialIsland` (a session's fallback map) and `Lvl_RansomsRest` with everything they refer to,
  plus the `DirectoriesToAlwaysCook` (`/Game/Art/Melee` for the heroes' tools, loaded by path; `/Game/UI` covers the Unreal logo). Left out: `Lvl_Skyreach`, `Maps/Dev`,
  `Developers`, the Skyreach props, editor content. `LooterEditor` is an Editor module; the MCP plugins (`ModelContextProtocol`,
  `AllToolsets`) are Editor-only in the .uproject.
- Rule: a path in C++ is not a reference. The cooker follows references only. A `LoadObject`, `FindIfMade`,
  `TSoftObjectPtr(FSoftObjectPath(...))` or `TryLoadClass` on an asset nothing else refers to finds nothing in a package, and the
  `FindIfMade` ones quietly draw a cube instead. Keep such assets under a directory in `DirectoriesToAlwaysCook` (or add its
  directory). After a new package, look for stand-in shapes: the jetty, depot, train, egg sacs, lanterns, chests, fonts.
- Share: (`-Zip`) a Drive, OneDrive or Dropbox link to the zip (no new upload tool without the user's OK). The friend unzips somewhere
  writable (not Program Files) and runs `AI_Looter_Shooter.exe`; the first launch compiles pipeline states while the level loads. A
  GPU without D3D12 SM6 starts it with `-dx11`.
- Saves and logs sit beside the game, not in AppData: a packaged Development build isn't an "installed" one, so they go to
  `<unzipped>\AI_Looter_Shooter\Saved\` (SaveGames, Logs, Crashes). Ask a tester for the log after a bug.

## Performance

- Target, rules and logged numbers: `Docs/Performance.md`. Per-pass budgets: `Docs/TutorialIsland.md`. Lumen and Nanite are switched
  by `UGraphicsSettingsSubsystem::QualitySettings`.
- `perf.ps1 -GpuStats` records each pass; `Tools\perfdiff.ps1` compares two captures; `-Map` measures another level.
- Measure with the editor closed and Blender paused (create `Saved\ArtPause.flag`, delete it after):
  - `Tools\tour.ps1` for every view;
  - `Tools\perf.ps1 -Label ... -Exec "Looter.Quality Medium" -Map <map>`, which appends a row to `Docs/Performance.md`.
    Run it from PowerShell, never through Git Bash: bash rewrites `/Game/Maps/...` into `C:/Program Files/Git/Game/...`
    and the game waits 15 min on a map it can't load (every `/Game/...` argument to a .ps1 has the same trap).
- Per pass: `perf.ps1 -GpuStats -Exec "Looter.Quality Medium,Looter.Tour <views.json> noshots" -Frames 16000 -Skip 0 -NoRecord`, then
  `Tools\perfviews.ps1 <csv>`. Pass the views file: without it, `Looter.Tour` walks the tutorial island's list. `-Pattern
  'Exclusive/RenderThread'` lists the render thread's work.
- Heroes and powers (2026-10-20): `-Exec "Looter.Quality Medium,Looter.Hero <Id>"` wears a hero for the run;
  `Looter.Perf.Horde Spider 8 Basic` puts a fight in view (the player unhurt); `Looter.Embers.Repeat <id> <seconds>` fires
  a power again and again past its cooldown (an -Exec runs once, before the capture). An `-Exec` with `ShowFlag.PointLights
  0` in both runs tells a light's cost from the rest. Found that way: one extra point light while a power lasts costs ~0.3 ms
  on Medium (GPU lights pass + the render thread's visibility wait), so `AEmberPropActor::LightsAllowed` keeps power lights
  (Grave Ward's lantern, Slag Bomb's pool) to High and Epic.
- What a group costs: a view file of its own under `Saved/` (any path under the project) with each view twice, once with
  `"exec": ["Looter.Perf.HideFolder RansomsRest/<folder> 1"]`, `"after": [... 0]`; `Looter.Perf.DrawShares <view>` in a view's
  `after` lists what it drew by outliner folder and mesh (`Saved/Tour/DrawShares.csv`); `ProfileGPU` in `after` dumps one
  frame's passes to the log (the frame as the tour leaves: can miss spikes; use the CSV capture for those);
  `r.LumenScene.DumpStats 2` dumps Lumen's scene (cards, objects, surface cache).
- A view measured right after a jump includes the renderer's catch-up: list a view twice (`<name>_Stay`, same place) to time
  it standing still.
- Lesson (2026-10-14): `Looter.Quality` in a tour or perf run saves the preset to the project's `Saved/SaveGames`, the same file
  the user's game reads: end every batch of runs on Medium (or the user's own preset).

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
- `Looter.Hero.Unlock` typed at the console (0 or 1) outranked the tests' own set (`ECVF_SetByCode` is lower than the
  console's): `Looter.Heroes.Playable`, `.Save` and `.Screen` failed on "Unlocked: Odessa..." until the editor restarted
  (2026-10-20, twice). Fixed: the tests' `FHeroUnlock` sets and restores it at `ECVF_SetByConsole` (checked: Unlock 1
  typed, then Looter.Heroes 7/7). A test that sets a console variable a developer may also type: use the console's priority.
