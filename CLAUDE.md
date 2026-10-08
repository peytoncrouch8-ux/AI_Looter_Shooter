# AI_Looter_Shooter

A stylized first/third-person looter shooter in Unreal Engine 5.8, C++-first. The game is the runtime module
`Source/AI_Looter_Shooter`; `Source/LooterEditor` holds editor-only level tools and never ships. Levels: the tutorial
island `/Game/Maps/Lvl_TutorialIsland` (`Docs/TutorialIsland.md`) and the campaign's first area, Ransom's Rest,
`/Game/Maps/Lvl_RansomsRest` (`Docs/Areas/RansomsRest.md`). `/Game/Maps/Lvl_Skyreach` (the old sky-island style) is
kept for reference.

Read first: `Docs/Handoff.md` (where the work stands), `Docs/Pipeline.md` (how things get made: tools, level and asset
workflows, lessons, bug fixes), `CODEMAP.md` (every source file in one line). Also `Docs/Plan.md`, `Docs/Performance.md`,
`Docs/Story.md` and `Docs/Areas/` (one design per area). This file rides along with every request, every agent's too:
keep it short and put details in the pipeline.

## How we work (the user's rules; add new ones here)

1. **The goal:** the game will be sold on Steam. No copyrighted content anywhere (art, sound, names, text, code), and
   the player's enjoyment comes first. Aim for the quality of the best big-studio games.
2. **One orchestrator, at most 6 agents.** The main session, running on Opus 5.5, plans, orchestrates and implements.
   It reads the docs that bear on the work first, verifies every agent's work, keeps the quality up and keeps the
   agents in step. Roles, as needed: Planning, Story/Logistics, Special FX/Sounds, Art/UI/Assets, Demo (plays the game
   and fixes bugs) and Review (checks code, world and assets for consistency).
   - **The orchestrator picks each agent's model** to get the most from the usage: the strongest for design and tricky
     code, a lighter one (Sonnet, Haiku) for reviews, mechanical edits and simple searches.
   - **Parallel agents work on disjoint files against one shared contract** the orchestrator writes first: the names,
     signatures, palette and placements they all code to (`Docs/Pipeline.md`, "Who does what").
3. **Ask the user first** before bringing in a new tool or program.
4. **Ask when a request is unclear,** as tap-to-answer choices (AskUserQuestion), not open questions.
5. **Report briefly:** a few lines, not paragraphs.
6. **Two failures, then ask.** If a task fails twice, ask the user how to go on.
7. **Feedback matters.** Every action (keys, shooting, hits, kills, pickups, menus) should feel rewarding, with
   high-quality visual and sound effects.
8. **Keep the pipeline written down** in `Docs/Pipeline.md`: bug fixes, how assets and terrain are made, what worked
   and what didn't, the theme and look, how work is divided. Update it as work finishes.
9. **Keep the handoff current.** `Docs/Handoff.md` says what's finished, where we left off and where we're headed.
   Update it as each request finishes.
10. **No working through a degraded context.** Before auto-compaction, tell the user it's time for a new session and
    make sure the handoff has what the next one needs.

## The game's flow

`ALooterGameMode` (the default) gives each player an `ALooterPlayerController`, the `ALooterHUD` and
`/Game/Player/BP_LooterCharacter`, a data-only child of `ALooterCharacter`. The game starts at the main menu: the
default map opened with `?game=Menu` (`LocalMapOptions` in DefaultEngine.ini; `ALooterMenuGameMode`). Single Player picks one of three sessions
(`USessionSubsystem`) and opens the level with `?Session=N`; the session saves the player and the world, and the pause
menu's Save & Quit goes back to the menu. Play-In-Editor, or a map named on the command line (`perf.ps1`, `tour.ps1`),
plays a new game and saves nothing. `Looter.Session.Play <1-3>` plays a session from the console. The standalone game
and PIE share `Saved\SaveGames`; command-line runs keep theirs under `%LOCALAPPDATA%\UnrealEngine\5.8\Saved`.

## Build, run, test

- **Build** with the editor closed: `Tools\launch.ps1 -Build` closes it, builds, reopens it and waits for the MCP
  server. A full rebuild takes about 2.5 minutes. The module builds without unity, so every `.cpp` includes what it
  uses and anonymous-namespace names can't clash. For people: `Launch AI_Looter_Shooter.bat` plays the game, and
  `Launch AI_Looter_Shooter Editor (backup).bat` builds and opens the editor.
- **Tests:** `Tools\runtests.ps1` runs every `Looter.*` test (editor open). Keep them all passing.
- **Performance:** `Tools\perf.ps1 -Label "what changed" -Exec "Looter.Quality Medium"` with the editor closed,
  before and after anything that could change cost; `Tools\tour.ps1` for every viewpoint of a level. Options and
  per-pass captures are in the pipeline.
- **Editor automation** goes over MCP on port 8000: `Tools\mcp.ps1 <toolset> <tool> '<json>'`, `Tools\runscript.ps1`
  (sandboxed Python), `Tools\describe.ps1 <toolset>`, `Tools\console.ps1 "<command>" [-Until <regex>]` (no window
  focus needed), `Tools\pie.ps1`, `Tools\input.ps1` (keys, mouse) and `Tools\grab.ps1` (screenshots). Unreal Python
  runs as `Tools\console.ps1 "py <absolute path>"`; use it for editor-only properties the MCP tools refuse
  (`set_editor_property`, bools without the `b`).

## Safety

- Never send Ctrl+key or Delete through `Tools\input.ps1` unless a text box verifiably has focus (this once deleted
  every actor in the loaded level).
- `Tools\input.ps1` stops when the Unreal Editor isn't the foreground window. Keep that guard: a crashed editor once
  let a console command get typed into the Claude chat and sent.
- A modal editor dialog blocks every MCP call until answered. `Tools\editorwindows.ps1` lists the editor's windows.
- Stop play-in-editor before closing the editor. Check what an asset tool will overwrite before running it.
- Commit after every working step. Messages end with the co-author trailer the session asks for.

## Code rules

- C++ first. Blueprints and data assets hold data and configuration only, never gameplay logic.
- One class per file, under about 500 lines per file; a bigger class spreads its `.cpp` over files named by topic
  (`WeaponBaseFiring.cpp`). Private helpers go in the `.cpp`.
- Organize code by game area (`Docs/Plan.md`). Update `CODEMAP.md` when you add, move or delete a file.
- Comments explain why, in plain words. Match the style: tabs, UE naming, `F`/`U`/`A`/`E` prefixes. Log to
  `LogLooter`.

## UI rules (the user requires these)

- Every UI is built in C++ with the `LooterUI` kit (`UI/Style/LooterUIStyle.h`), "Concept C": dark glass panels,
  orange accents, cyan lines, Chakra Petch. Colors come only from `LooterUI::Color` (or `LooterUI::Hex` inside it).
- The gameplay HUD has no backing panels: floating outlined text and slim slanted bars, in gunmetal metalwork.
- Every background a widget paints calls `LooterUI::MarkBackground`, so the UI transparency setting fades it. Text,
  outlines and bars stay solid.
- Vector art from mockups goes through the kit's brushes (`IconBrush`, `PaintedIconBrush`), never new texture assets.
- Weapon and ammo icons are Inked icons, generated from `Art/Icons/InkedIcons.py` into `UI/Style/InkedIconData.inl`
  and drawn with `LooterUI::InkedIconBrush`, tinted white (grey dims them).

## Assets

- Git LFS stores every binary. Prefixes: `SM_`, `SK_`, `M_`/`MI_`, `T_`, `DA_`, `BP_`, `UCX_` (collision),
  `SOCKET_` (attach points).
- Never generate meshes while the game runs; bake them into assets.
- Blender models live in `Art/Models/<Category>/`; `Tools\models.ps1` imports them (`Art/README.md` has the rules).
  Change a model in Blender and import again, never edit the imported mesh. Blender beside a running editor goes
  through `Tools\artrun.ps1`, which waits while `Saved\ArtPause.flag` exists (create it before measuring, delete after).
- Guns are assembled from parts (`Art/Models/Weapons/<Gun>.py` and `<Gun>.parts.csv`; see the pipeline). A dropped gun
  saves its parts by key: never rename or reuse a key.
- The look is textured "stylized realism": the masters in `/Game/Art/Materials/Masters` with texture sets from
  `Art/Textures/<Set>`, always through material instances. Nanite can't draw the additive glow; use an emissive surface.
- Level building (area builds, the tutorial island's scripts, props, scatter, ground fit) is in the pipeline. Terrain
  meshes keep every triangle in their Nanite fallback: Medium draws it and collision is cooked from it.
- The minimap reads tags: `Ground` (walkable terrain, its extent) and `Obstacle` (solid things on it). Tag meshes you
  place; props tag themselves.
- Every new creature, enemy, NPC or friend gets a bestiary page (`/Game/Data/Bestiary/DA_Bestiary_<Name>`, with
  `ActorClass` set). `Looter.Bestiary.Entries` checks them.
- Volumes answer world-static queries: a trace looking for real geometry uses `LooterWorld::StaticGeometryParams`.
- **Performance target:** 120 fps (8.3 ms) at 1080p on Medium on the reference PC (RX 580, i7-8700, 16 GB), at the
  heaviest view. Lumen and Nanite are for High and Epic only, so everything must look right on the Nanite fallback.
