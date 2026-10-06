# AI_Looter_Shooter

A stylized first/third-person looter shooter in Unreal Engine 5.8, written C++-first. The game is the runtime module
`Source/AI_Looter_Shooter`; `Source/LooterEditor` is an editor-only module of level-building tools that never ships. The
playable level is the tutorial island, `/Game/Maps/Lvl_TutorialIsland` (`Docs/TutorialIsland.md`); the older
`/Game/Maps/Lvl_Skyreach` (floating sky islands, the previous art style) is kept for reference.

The C++ `ALooterGameMode` (project default) gives each player an `ALooterPlayerController`, the `ALooterHUD` and
`/Game/Player/BP_LooterCharacter`: a data-only child of `ALooterCharacter` that holds the meshes, animation, camera
placement and gameplay component settings.

The game starts at the main menu: the default map opened with `?game=Menu` (DefaultEngine.ini's `LocalMapOptions`), so
`ALooterMenuGameMode` shows the menu over the island. Single Player picks one of three sessions (`USessionSubsystem`),
which opens the level with `?Session=N`. The session saves the player and the world, and the pause menu's Save & Quit goes back
to the menu. Play-In-Editor on a level, or a map named on the command line (`perf.ps1`, `tour.ps1`), plays it without
a session: a new game, and nothing is saved. `Looter.Session.Play <1-3>` plays a session from the console. The
standalone game (the launcher) and Play-In-Editor share the saves in `Saved\SaveGames`. Runs that name a map on the
command line keep theirs under `%LOCALAPPDATA%\UnrealEngine\5.8\Saved`.

- `Docs/Plan.md`: the pipeline cleanup plan and where it stands.
- `Docs/Performance.md`: measured performance history.
- `Docs/Story.md`: the campaign's story; `Docs/Areas/` holds one design per area.

## Build, run, test

- The editor must be closed to build: `Tools\launch.ps1 -Build` closes it cleanly, builds, reopens it and waits for the
  MCP server. For people, double-clicking `Launch AI_Looter_Shooter.bat` plays the game standalone (it compiles first
  when Unreal is closed), and `Launch AI_Looter_Shooter Editor (backup).bat` builds and opens the editor.
- Tests: `Tools\runtests.ps1` runs every `Looter.*` automation test (the editor must be open). Keep them all passing.
- Performance: `Tools\perf.ps1 -Label "what changed" -Exec "Looter.Quality Medium"`, with the editor closed. It
  measures a standalone 1080p window and appends the result to `Docs/Performance.md`. Measure before and after
  anything that could change cost, on Medium (the minimum spec). `-GpuStats` records each pass; compare two captures
  with `Tools\perfdiff.ps1`. `-Map` measures another level.
- Per area: `Tools\tour.ps1 [-Quality Medium]` runs the game through the level's viewpoints
  (`Art/Levels/TutorialIsland/views.json`, the in-game `Looter.Tour` command) and prints each one's frame, game, render
  and GPU time, with a screenshot of each in `Saved\Screenshots\Tour`. The budget holds at every viewpoint, not just
  the spawn. To see which passes cost what at each one, capture a tour with `perf.ps1 -GpuStats` and split it with
  `Tools\perfviews.ps1` (its header has the command).
- Editor automation goes over MCP on port 8000:
  - `Tools\mcp.ps1 <toolset> <tool> '<json>'` calls one tool.
  - `Tools\runscript.ps1 <file.py>` runs sandboxed Python: define `run()` and call tools with `execute_tool`.
  - `Tools\describe.ps1 <toolset>` lists a toolset's tools.
  - `Tools\console.ps1 "<command>" [-Until <log regex>]` runs an editor console command through the Slate inspector
    (no window focus or simulated keys) and prints the log lines it wrote.
  - Unreal Python runs through it too: `Tools\console.ps1 "py <absolute path to a .py file>"`. Use it for properties
    the MCP object tools refuse to write (editor-only ones such as a mesh descriptor, `bUseDefaultCollision` or
    material usage flags): `set_editor_property`, with bool names written without the `b` (`cast_shadow`).
  - `Tools\pie.ps1 -Commands ...` starts a play session.
  - `Tools\input.ps1` sends keys and the mouse. `Tools\grab.ps1` takes screenshots into `Saved\Screenshots\Tools`.
- The module builds without unity (`bUseUnity = false`), so every `.cpp` compiles on its own: include what you use, and
  file-private names in anonymous namespaces can't clash between files. A full rebuild takes about two and a half minutes.

## Safety

- Never send Ctrl+key or Delete through `Tools\input.ps1` unless a text box verifiably has focus. They reach the level
  editor otherwise (this once deleted every actor in the loaded level).
- `Tools\input.ps1` stops before any step when the Unreal Editor isn't the foreground window. Keep that guard: once, a
  crashed editor let a play-test's console command get typed into the Claude chat window and sent.
- A modal editor dialog (save prompt, "transfer interface functions?") blocks every MCP call until it's answered. Find
  it with `Tools\editorwindows.ps1`, which lists the editor's top-level windows.
- Stop play-in-editor before closing the editor. Check what an asset tool will overwrite before running it.
- Commit after every working step. Messages end with the co-author trailer the session asks for.

## Code rules

- C++ first. Blueprints and data assets only hold data and configuration, never gameplay logic.
- One class per file, and aim for under ~500 lines per file. Private helpers go in the `.cpp`. A class that outgrows
  that spreads its `.cpp` over files named by topic (`WeaponBaseFiring.cpp`, `WeaponManagerSlots.cpp`).
- `CODEMAP.md` lists every source file in one line. Read it first to find where something lives.
- Organize code by game area; the planned layout is in `Docs/Plan.md`. Update `CODEMAP.md` when you add, move or delete
  a file.
- Comments explain why, in plain words. Match the surrounding style (tabs, UE naming, `F`/`U`/`A`/`E` prefixes).
- Log categories go per area (`LogLooter` for now; the reorganization adds one per domain).

## UI rules (the user requires these)

- Every UI is built in C++ with the `LooterUI` style kit (`UI/Style/LooterUIStyle.h`). This is "Concept C": dark glass panels
  with orange accents, cyan lines and the Chakra Petch font. Never use ad-hoc colors or plain UMG styling. Use
  `LooterUI::Hex` and the `LooterUI::Color` palette.
- The gameplay HUD is the exception: no backing panels, only floating outlined text and slim slanted bars.
- Every background a widget paints must call `LooterUI::MarkBackground` so the UI transparency setting fades it. Text,
  outlines and bars stay solid.
- Vector art from mockups (icons, silhouettes) goes through `LooterUI::IconBrush`, never through new texture assets.
- Weapon and ammo icons are Inked icons (the user's pick). Their outlines live in `Art/Icons/InkedIcons.py`, which
  generates `UI/Style/InkedIconData.inl`. Draw them with `LooterUI::InkedIconBrush`, tinted white (grey dims them).

## Assets

- Git LFS stores every binary: `.uasset`, `.umap`, `.blend`, `.fbx`, images, audio, fonts.
- Name prefixes: `SM_` static mesh, `SK_` skeletal mesh, `M_`/`MI_` materials, `T_` textures, `DA_` data assets,
  `BP_` Blueprints, `UCX_` collision hulls, `SOCKET_` attach points.
- Do not generate meshes while the game runs. Bake generated models into assets (see `Docs/Plan.md`).
- Blender models live in `Art/Models/<Category>/` (hand-made `.blend` or scripted `.py`). `Tools\models.ps1` exports and
  imports them into `/Game/Art/<Category>` with fixed settings; `Art/README.md` has the authoring rules. Change a model
  in Blender and import it again, never edit the imported mesh.
- Blender work beside a running editor goes through `Tools\artrun.ps1`: a model script (with `-Preview`, its renders),
  any Blender script, or an export test into `Intermediate\ArtExport_<Family>`. It runs at below-normal priority and
  waits while `Saved\ArtPause.flag` exists: create that file before a `perf.ps1` or `tour.ps1` measurement and delete it
  after. `Tools/Blender/tangentcheck.py` checks an exported FBX's tangents the way Unreal's import will; with `--log` it
  sorts the editor log's tangent warnings into the model's own and Unreal's reduced builds'.
- Guns are assembled from parts when they drop. `Art/Models/Weapons/<Gun>.py` models the parts (sockets chain them;
  sights carry `SOCKET_Aim` for aiming down sights), and `<Gun>.parts.csv` beside it lists each part's key, name, name
  word, rarity and stat ranges in percent (capped per stat, `Weapons/WeaponParts.h`). After importing, run
  `Tools/Unreal/setup_gun_parts.py` in the editor: it fills the gun's definition from the spreadsheet. A dropped gun
  saves its parts by key, so never rename or reuse a key.
- The art style is moving to textured "stylized realism" (`Docs/TutorialIsland.md`, the tutorial island first). New
  models use the textured masters in `/Game/Art/Materials/Masters` (`M_World`, `M_Gun` for gun parts with per-gun
  wear, `M_WorldFoliage`, `M_Terrain`, `M_Water`, built by `Tools/Unreal/build_world_materials.py`) with texture sets from `Art/Textures/<Set>`. Older
  surfaces use the flat stylized materials (`M_StylizedSurface`, `M_StylizedFoliage`, `M_StylizedGlow`). Always go
  through material instances. Nanite can't draw the additive glow; use an emissive surface (Glow setting) on Nanite
  meshes.
- The tutorial island is built by scripts from `Art/Levels/TutorialIsland/layout_computed.json` (which the terrain
  model writes): `Tools/Unreal/build_tutorial_island.py` places the terrain, cliffs, buildings, lighting and gameplay
  actors, and `Tools/Unreal/build_island_scatter.py` scatters grass, flowers, trees and rocks with PCG from the
  terrain's scatter mask. Rebuilding replaces only what they placed (`build_tutorial_island.py gameplay` places just
  the gameplay actors again: spawn, dummies, spiders, slimes). `Tools/Unreal/review_stage.py` photographs new
  models under the island's lighting.
- Older levels are built in the editor. Procedural props are `StylizedProp` actors (shape, seed, two colors). Before
  committing a level, run `Looter.BakeLevelProps` in the editor console. It swaps them for static mesh actors and saves
  their meshes and materials under `/Game/Environment/Props`.
- After placing props or changing terrain, run `Looter.SettleProps` (`Looter.SettleProps selected` for just the
  selection): it seats every prop on the ground so no edge hovers, leaning low, wide ones with the slope. Save the level.
  After changing terrain, first run `Tools/Unreal/conform_hills.py`: it fits the hills' rims back under the ground.
- Terrain meshes (island, hills, cliffs) keep every triangle in their Nanite fallback: Medium draws the fallback and
  collision is cooked from it, so a reduced one makes everything placed by traces float over the ground High draws.
- In Lvl_Skyreach, grass and flowers come from the `Meadow` PCG volume (`/Game/Environment/PCG/PCG_Meadow`). It raycasts onto actors
  tagged `Ground` and avoids actors tagged `Obstacle`. After changing terrain, select the volume and press Generate,
  then save the level. Ground cover never collides; a placed static mesh actor takes its mesh's collision unless
  `bUseDefaultCollision` is off. The patches are small (about 3.5 m) and lie on the slope so they follow the ground,
  and a ground fit filter (`World/PCGGroundFitFilter`) drops the ones that would hang off an edge;
  `Looter.BakeGroundCover` bakes their meshes again from the generator.
- The minimap reads actor tags. Its extent comes from actors tagged `Ground` (walkable terrain). Actors tagged
  `Obstacle` (solid things standing on the ground) are drawn as obstacles, and anything untagged is drawn as ground.
  Props and baked props tag themselves; tag other meshes you place.
- Every new creature, enemy, NPC or friend gets a bestiary page: a `UBestiaryEntry` data asset in `/Game/Data/Bestiary`
  (`DA_Bestiary_<Name>`; duplicate one). Write its name, section, description and field notes, and set `ActorClass`:
  its level, health, attack, experience, defeat count and stand model come from that class. `Looter.Bestiary.Entries`
  checks every page.
- Volumes (the meadow's PCG volume, triggers) answer world-static object queries. A trace that looks for the
  ground or other real geometry that way must use `LooterWorld::StaticGeometryParams`, which skips them.
- Performance target: 120 fps (8.3 ms) at 1080p on the Medium preset on the reference PC (Radeon RX 580, i7-8700,
  16 GB), at the heaviest view of the level; the budgets are in `Docs/TutorialIsland.md`. Lumen lighting and Nanite are for the High and Epic presets only
  (`UGraphicsSettingsSubsystem::QualitySettings`), so everything must also look right without them: every mesh
  draws its Nanite fallback on Medium and Low.
