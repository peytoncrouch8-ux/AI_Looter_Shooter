# AI_Looter_Shooter

Stylized first/third-person looter shooter, Unreal Engine 5.8, C++-first. Game: runtime module
`Source/AI_Looter_Shooter`. `Source/LooterEditor`: editor-only level tools, never ships. Levels: Ransom's Rest
`/Game/Maps/Lvl_RansomsRest` (campaign area 1; `Docs/Areas/RansomsRest.md`, mountain in `RansomsRest_Expansion.md`);
the tutorial island `/Game/Maps/Lvl_TutorialIsland` (a session's fallback; `Docs/TutorialIsland.md`); the menu: `/Game/Maps/Lvl_MenuTeropa` (Teropa from orbit); `/Game/Maps/Lvl_Skyreach`
(old sky-island style, kept).

Read first: `Docs/Handoff.md` (state, next steps), `Docs/Pipeline.md` (index of the topic files in `Docs/Pipeline/`: tools, workflows, lessons, bug fixes; read those that bear on the work), `Docs/INDEX.md` (every doc;
`node Tools/docindex.js` rebuilds it and the user's Project Library page https://claude.ai/artifact/1yHV6GGJvvcwxWwZqi4T4s, republish at each session end),
`CODEMAP.md` (every source file, one line each). As needed: `Docs/Plan.md`, `Docs/Performance.md`, `Docs/Story.md`,
`Docs/Progression.md`, `Docs/Areas/`; `Docs/Release.md` only when preparing a release. History and background (`Docs/Archive`, `Docs/Art`, `Docs/Handoffs`,
`Docs/Polish`, `*Concepts.md`, `*_Review.md`): only when pointed to. This file rides with every request and every agent:
keep it short; details go in the pipeline.

## How we work (the user's rules; add new ones here)

1. **Goal:** sold on Steam. No copyrighted content anywhere (art, sound, names, text, code). Player enjoyment first.
   Quality of the best big-studio games.
2. **One orchestrator, at most 8 agents.** The main session (Opus 5.5) plans, orchestrates, implements; reads the docs
   that bear on the work first; verifies every agent's work; keeps quality up and agents in step. Roles as needed:
   Planning, Story/Logistics, Special FX/Sounds, Art/UI/Assets, Demo (plays the game, fixes bugs), Review (code, world,
   assets consistency).
   - The orchestrator picks each agent's model: strongest for design and tricky code; Sonnet/Haiku for reviews,
     mechanical edits, simple searches.
   - Parallel agents work on disjoint files against one shared contract the orchestrator writes first (names,
     signatures, palette, placements; `Docs/Pipeline/Workflow.md`, "Who does what").
3. **Ask the user first** before bringing in a new tool or program.
4. **Unclear request: ask** with tap-to-answer choices (AskUserQuestion), not open questions.
5. **Report briefly:** 3 short sentences or less (the user, 2026-10-21: saves tokens).
6. **Two failures, then ask** the user how to go on.
7. **Feedback matters:** every action (keys, shooting, hits, kills, pickups, menus) feels rewarding, with high-quality
   visual and sound effects.
8. **Keep the pipeline notes (`Docs/Pipeline/<Topic>.md`, indexed in `Docs/Pipeline.md`) current:** bug fixes, how assets and terrain are made, what worked and didn't, theme and
   look, how work is divided. Update as work finishes.
9. **Keep `Docs/Handoff.md` current:** what's finished, where we left off, where we're headed. Update as each request
   finishes.
10. **No working through a degraded context.** Before auto-compaction, tell the user it's time for a new session and
    make sure the handoff has what the next one needs.
11. **Docs are for the AI** (the user never reads them; 2026-10-13): terse plain-English notes, each fact in one place,
    every name, number, rule and user decision kept; history goes to `Docs/Archive`.
12. **Job notes in git:** a new job keeps its contract/plan in `Docs/Work/<date>-<job>/`; moved to `Docs/Archive/Work/` when committed.

## The game's flow

- `ALooterGameMode` (default) gives each player `ALooterPlayerController`, `ALooterHUD` and
  `/Game/Player/BP_LooterCharacter` (data-only child of `ALooterCharacter`).
- Starts with the startup logos, then the main menu: default map (Lvl_MenuTeropa) opened with `?game=Menu` (`LocalMapOptions` in DefaultEngine.ini;
  `ALooterMenuGameMode`). Single Player picks one of three sessions (`USessionSubsystem`), opens the level with
  `?Session=N`; the session saves player and world; pause menu's Save & Quit returns to the menu.
- PIE opens the menu too. `Looter.PlayLevel [map]` (`pie.ps1` runs it) or a map on the command line (`perf.ps1`,
  `tour.ps1`) plays a new game and saves nothing. `Looter.Session.Play <1-3>` plays a session from the console.
- Standalone game and PIE share `Saved\SaveGames`; command-line runs use `%LOCALAPPDATA%\UnrealEngine\5.8\Saved`.

## Build, run, test

- **Build** with the editor closed: `Tools\launch.ps1 -Build` closes it, builds, reopens, waits for the MCP server
  (full rebuild ~2.5 min). No unity build: every `.cpp` includes what it uses; anonymous-namespace names can't clash.
  For people: `Launch AI_Looter_Shooter.bat` plays; `Launch AI_Looter_Shooter Editor (backup).bat` builds and opens.
- **Tests** (the user, 2026-10-21): run only the new or changed work's tests, once, when it's committed:
  `Tools\runtests.ps1 -Filter Looter.<Group>` (editor open). No full `Looter` run unless the user asks.
- **Performance:** `Tools\perf.ps1 -Label "what changed" -Exec "Looter.Quality Medium"` (editor closed) before and
  after anything that could change cost; `Tools\tour.ps1` for every viewpoint. Options in the pipeline.
- **Editor automation** over MCP, port 8000: `Tools\mcp.ps1 <toolset> <tool> '<json>'`, `Tools\runscript.ps1`
  (sandboxed Python), `Tools\describe.ps1 <toolset>`, `Tools\console.ps1 "<command>" [-Until <regex>]` (no focus
  needed), `Tools\pie.ps1`, `Tools\input.ps1` (keys, mouse), `Tools\grab.ps1` (screenshots). Unreal Python:
  `Tools\console.ps1 "py <absolute path>"`; use it for editor-only properties MCP refuses (`set_editor_property`,
  bools without the `b`).

## Safety

- Never send Ctrl+key or Delete through `Tools\input.ps1` unless a text box verifiably has focus (once deleted every
  actor in the loaded level).
- `Tools\input.ps1` stops when the Unreal Editor isn't the foreground window. Keep that guard (a crashed editor once let
  a console command be typed into the Claude chat and sent).
- A modal editor dialog blocks every MCP call until answered. `Tools\editorwindows.ps1` lists the editor's windows.
- Stop PIE before closing the editor. Check what an asset tool will overwrite before running it.
- Commit after every working step; messages end with the co-author trailer the session asks for.

## Code rules

- C++ first. Blueprints and data assets hold data and configuration only, never gameplay logic.
- One class per file, under ~500 lines per file; a bigger class splits its `.cpp` by topic (`WeaponBaseFiring.cpp`).
  Private helpers in the `.cpp`.
- Code organized by game area (`Docs/Plan.md`). Update `CODEMAP.md` when adding, moving or deleting a file.
- Comments explain why, in plain words. Style: tabs, UE naming, `F`/`U`/`A`/`E` prefixes. Log to `LogLooter`.

## UI rules (the user requires these)

- Every UI is C++ with the `LooterUI` kit (`UI/Style/LooterUIStyle.h`): dark glass panels, Chakra Petch, colours "Slate &
  Salt" since 2026-10-29 (slate glass, salt lines, gold = selected; was Concept C blue/cyan/orange). Colors only from
  `LooterUI::Color` (or `LooterUI::Hex` inside it).
- Gameplay HUD has no backing panels: floating outlined text, slim slanted bars, gunmetal metalwork. Pieces: player
  frame (portrait in a medallion, health bar, level gem, experience bar) bottom-left; weapon column with upright
  cartridge bottom-right; bezelled minimap top-right, the mission tracker under it. `Tools\hudshots.ps1` photographs it at 1080p
  through its states. Mockup CSS px are used as Slate font sizes as they are (the user's call: in game text is a third
  bigger than in mockups).
- The gun card is the tag card (the user's pick 2026-10-28 evening; `UI/Cards`): Borderlands' item-card layout in our own
  look, the tag shape with the level's gem, bold 4 px lines, keys floating under it, motion and a sound on open. Its colours
  "Slate & Salt" (`LooterUI::Color::Slate*`, `SaltLine`, `SoulGold`) are the user's pick for the whole UI: other screens move
  to them in later rounds. Pipeline Game.md "The gun card".
- Every background a widget paints calls `LooterUI::MarkBackground` (the UI transparency setting fades it). Text,
  outlines and bars stay solid.
- Vector art from mockups goes through the kit's brushes (`IconBrush`, `PaintedIconBrush`), never new texture assets.
- Weapon and ammo icons are Inked icons: `Art/Icons/InkedIcons.py` -> `UI/Style/InkedIconData.inl`, drawn with
  `LooterUI::InkedIconBrush`, tinted white (grey dims them).

## Assets

- Git LFS stores every binary. Prefixes: `SM_`, `SK_`, `M_`/`MI_`, `T_`, `DA_`, `BP_`, `UCX_` (collision), `SOCKET_`
  (attach points), `S_` (sound waves), `SC_`/`SMX_`/`ATT_` (sound classes, mix, attenuation).
- Sounds are script-made (the user's call), except guns and impacts may use royalty-free libraries licensed for games
  (2026-10-21; keep each pack's licence with it): recipes in `Art/Sounds`, rendered by `Tools\sounds.ps1`, imported by
  `Tools/Unreal/build_sound_bank.py`; code plays them by cue (`Audio/LooterSound.h`, `LooterSoundCues.h`).
- Never generate meshes while the game runs; bake them into assets.
- Blender models in `Art/Models/<Category>/`; `Tools\models.ps1` imports them (rules: `Art/README.md`). Change the
  model in Blender and reimport, never edit the imported mesh. Blender beside a running editor goes through
  `Tools\artrun.ps1` (waits while `Saved\ArtPause.flag` exists: create it before measuring, delete after).
- Guns are assembled from parts (`Art/Models/Weapons/<Gun>.py`, `<Gun>.parts.csv`; pipeline). A dropped gun saves its
  parts by key: never rename or reuse a key.
- Look: textured "stylized realism": masters in `/Game/Art/Materials/Masters`, texture sets from `Art/Textures/<Set>`,
  always through material instances. Nanite can't draw additive glow; use an emissive surface.
- Level building (area builds, tutorial island scripts, props, scatter, ground fit): the pipeline. Terrain meshes keep
  every triangle in their Nanite fallback (Medium draws it; collision is cooked from it).
- Minimap reads tags: `Ground` (walkable terrain, its extent), `Obstacle` (solid things on it). Tag meshes you place;
  props tag themselves.
- Every new creature, enemy, NPC or friend gets a bestiary page (`/Game/Data/Bestiary/DA_Bestiary_<Name>`, `ActorClass`
  set); `Looter.Bestiary.Entries` checks them.
- Volumes answer world-static queries: a trace for real geometry uses `LooterWorld::StaticGeometryParams`.
- **Performance target:** 120 fps (8.3 ms), 1080p Medium, reference PC (RX 580, i7-8700, 16 GB), heaviest view.
  Nanite is High/Epic only, Lumen Epic only: everything must look right on the Nanite fallback without Lumen.
