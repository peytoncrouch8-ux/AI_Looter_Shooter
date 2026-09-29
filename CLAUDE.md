# AI_Looter_Shooter

A stylized first/third-person looter shooter in Unreal Engine 5.8, written C++-first. One runtime module,
`Source/AI_Looter_Shooter`. The playable level is `/Game/Maps/Lvl_Skyreach` (floating sky islands).

The C++ `ALooterGameMode` (project default) gives each player an `ALooterPlayerController`, the `ALooterHUD` and
`/Game/Player/BP_LooterCharacter`: a data-only child of `ALooterCharacter` that holds the meshes, animation, camera
placement and gameplay component settings.

- `Docs/Plan.md`: the pipeline cleanup plan and where it stands.
- `Docs/Performance.md`: measured performance history.

## Build, run, test

- The editor must be closed to build: `Tools\launch.ps1 -Build` closes it cleanly, builds, reopens it and waits for the
  MCP server. The double-click launcher `Launch AI_Looter_Shooter.bat` does the same for people.
- Tests: `Tools\runtests.ps1` runs every `Looter.*` automation test (the editor must be open). Keep them all passing.
- Performance: `Tools\perf.ps1 -Label "what changed"`, with the editor closed. It measures a standalone 1080p window and
  appends the result to `Docs/Performance.md`. Measure before and after anything that could change cost.
- Editor automation goes over MCP on port 8000:
  - `Tools\mcp.ps1 <toolset> <tool> '<json>'` calls one tool.
  - `Tools\runscript.ps1 <file.py>` runs sandboxed Python: define `run()` and call tools with `execute_tool`.
  - `Tools\describe.ps1 <toolset>` lists a toolset's tools.
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
- One class per file, and aim for under ~500 lines per file. Private helpers go in the `.cpp`.
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

## Assets

- Git LFS stores every binary: `.uasset`, `.umap`, `.blend`, `.fbx`, images, audio, fonts.
- Name prefixes: `SM_` static mesh, `SK_` skeletal mesh, `M_`/`MI_` materials, `T_` textures, `DA_` data assets,
  `BP_` Blueprints, `UCX_` collision hulls, `SOCKET_` attach points.
- Do not generate meshes while the game runs. Bake generated models into assets (see `Docs/Plan.md`).
- Performance target: 60 fps at 1080p on the Medium preset on the reference PC (Radeon RX 580, i7-8700, 16 GB). Lumen
  lighting is for the High and Epic presets only.
