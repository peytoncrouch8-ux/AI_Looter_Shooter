# PERF2 lane (2026-11-04 night): the game thread after round 5, new hitches, the GPU views over the line

Inputs: the 42-view tours `Saved/ArtPreviews/Step1/tour_round4_before_erosion.csv` (19:51-19:56, build 312cb821) and
`tour_round5_after_erosion.csv` (22:00-22:05, d54521d4 + the erosion rebuild); logs `Saved/Logs/Tour-backup-2026.11.05-00.56.01.log`
(before) and `Saved/Logs/Tour.log` (after); CSVs in `%LOCALAPPDATA%\UnrealEngine\5.8\Saved\Profiling\CSV` (19:48 deck row
`Profile(20261104_194801)`, 19:49 horde `194943`, 22:05 capsule A/B `220540`, 22:10 family split `221018`, 22:18 deck `221828`,
22:20 horde `222004`); `Saved/Perf/Insights_Aerial.utrace` (19:57). Scripts (scratchpad `perf2/`, not in git): `gtagg.js`
(game-thread timers per frame from an Insights ExportTimingEvents CSV), `csvdiff.js`, `csvstops.js` (per tour stop),
`csvhitch.js` (slow frames and the Exclusive/* timers in them), `cpuwatch.ps1` (CPU load, clock and per-process CPU during a run).

## 1. The 22:00 game-thread rise is the machine, not the code (inferred, strong; Run A confirms)
- Every CPU timer rose by the same share. Deck row 19:48 -> 22:18: GameThreadTime 4.49 -> 5.39; Exclusive/GameThread UI +29%,
  TickActors +19%, Animation +29%, Tickables +34%, EndOfFrameUpdates +26%, Slate TickPlatform +32%, RHIThreadTime 1.76 -> 2.08
  (+18%). Horde 19:49 -> 22:20: game 6.60 -> 7.64, CharacterMovement/UI/Animation/TickActors/Tickables each +16-26%. Slate's
  TickPlatform (the OS message pump) and the RHI thread don't depend on the level.
- The tour's views by the same factor: MineStope 3.47 -> 5.54, DeckWest_NoBeyond 3.75 -> 5.89, MainStreet 3.86 -> 6.17, Aerial
  3.88 -> 5.13 (x1.3-1.6); DeckFight_Dusk 7.20 -> 11.41 is x1.58 (and its mean carries one 240 ms frame of a 3 s window, ~0.9
  ms). History of the same views (all tour CSVs): game 3.6-4.0 from 10-22 to 19:52 (MainStreet, Aerial, MineStope), DeckFight
  7.1-8.35.
- Other work ran in the 22:00 window (none 19:51-19:56): a Tripo repack (`Intermediate/TripoRepack/deba5b…npz` written 22:00:53,
  `Saved/ArtPreviews/Repack/BargeWalls_dry_BC.png` 22:04:52), and since 20:39 a `find.exe / -name LightComponent.h -path
  *Engine/Classes/Components*` (PID 17756, parent 20960; not this lane's, stopping it was refused to this lane) crawling the
  whole disk, still running at 23:03. At 23:03 the machine was 60-74% busy: two blender processes ~2 cores each, an
  UnrealEditor ~2 cores, the find ~1 core.
- The CSV-profiled runs agree: per-stop game at 22:05 (CapsuleAB: Aerial 4.7-6.1, Farm 4.5-5.7, POV 4.7-5.1, Grave 4.9-5.1) is
  no higher than 17:06's on the older build (Candidates: Aerial 4.7-6.7, Farm 4.8-5.8, POV 5.2-6.0, Grave 4.2-5.5): identical
  stops swing +-1 ms within one CSV run, every timer together. (CSV profiling itself adds ~1 ms of game thread: compare perf.ps1
  rows with perf.ps1 rows, tour rows with tour rows.)
- Code read, 312cb821..d54521d4 (Source): no new per-frame work. `ATownFire` ticks only while burning (`Main3_BarrelFire` is
  hidden in a new game: shown after Main1); `AHouseLights`/`ALightingStates` don't tick; `LightingStates::Kind` is a 5-entry
  lookup on a state change; `ULumenScatterSubsystem::ApplyHighShadows` runs at begin play and on streaming only;
  `UMissionObjective::NoteStarted` searches spawners only for hidden objectives until they start; the Ambience/AmbientAir/
  WindowLife dusk checks are name lookups. Nothing traces the terrain per frame but character movement (horde
  CharacterMovement +20%, the same as every other timer).

## 2. New hitches since round 5 (measured; real for the player)
None over 41 ms after start-up in any CSV 16:33-19:49 or the 19:51 tour (worst 32 ms). Since round 5:
- `Profile(20261104_220540)` row 4758 (Farm_NoCapsule1): 251 ms frame, `Exclusive/GameThread/TimerManager` 245.6 ms: one timer
  callback (no FlushAsyncLoading or ActorSpawning inside it). Which of the 52 `SetTimer` users: unknown.
- `Profile(20261104_221018)` row 11860: 133 ms, `AllWorkers/Audio` 137 + `GameThread/EngineTickMisc` 128 (the game thread
  waiting on audio work); row 12376 (Farm_Base): 27 ms, the same. `Profile(20261104_221828)` (the deck row, Main 6's state)
  row 447: 162 ms, Audio 163 + EngineTickMisc 153.
- The 22:00 tour: worst frames 218-270 ms at Grave, PointWest, PointWest_Dusk, MineStope, DeckFight_Dusk; Farm 2.2 s with
  `LogD3D12RHI: Waited for PSO creation for 200 ms` (03:00:29 UTC).
- Candidates (inferred; Run B's hunt trace names them): the audio stall is a sound's first play waiting for its decode or
  stream chunk (`build_sound_bank.py` sets no loading behaviour); d54521d4 made DuskSmog play the dusk beds
  (`AmbienceRules::IsDusk`), and the deck row's Main 6 state is DuskSmog. The timer stall: whatever the trace shows.
- Window life is not it: `UWindowLifeSubsystem::AddHouse` already loads the card's material at begin play.

## 3. What the game thread costs (19:57 baseline, `Insights_Aerial.utrace`, 1,391 frames, stat namedevents on)
Frame 8.6 ms, of which 4.44 waiting on the render thread; work ~4.15: world tick 2.25 incl, Slate 0.85 tick + 0.68 draw,
TickableGameObjects 0.28, BeginRenderingViewFamily 0.31, end-of-frame updates 0.24. Single items per frame: the player's
`UInteractionComponent` 0.18 ms (the biggest one component: every registry interactable gets its virtual location/box asked
before the 4 m distance test, `InteractionComponentFocus.cpp` 70-145), SlatePrepass 0.16, WaitForTasks 0.39, ISM instance
rebuilds 24 a frame (BatchUpdateInstancesTransforms + BuildRenderData + CalcBounds ~0.14), UFaunaSubsystem 0.07,
UAmbientAirSubsystem 0.04, PlayerHUD/minimap/flask widget ticks 0.04/0.04/0.03. CSV `Exclusive/GameThread/UI` is 0.9-1.3 ms
of every frame. Only the deck fight is game-bound (7.2 ms quiet); everything else waits on the GPU.

## 4. The GPU views over the line (FamilySplit 22:10 and CapsuleAB 22:05; GPU-bound stops, so CPU load doesn't move them)
Frame ms, base = mean of the run's two base stops.
| Stop | Aerial | Farm | PointOverValley | Grave |
|---|---|---|---|---|
| base (FamilySplit) | 8.57 | 8.50 | 8.34 | 7.96 |
| r.CapsuleIndirectShadows 0 (FamilySplit / CapsuleAB mean) | -0.24 / -0.28 | -0.11 / -0.09 | -0.19 / -0.14 | -0.11 / -0.12 |
| far trees hidden (bound) | -0.35 (prepass 0.80->0.68, base 2.66->2.47) | -0.38 (0.98->0.78, 2.29->2.08) | -0.53 (0.63->0.50, 2.42->2.13) | -0.31 |
| backdrop hidden | -0.23 (prepass -0.15) | -0.05 | -0.23 | 0 |
| ring / canyon wall hidden | -0.12 / 0 | 0 / 0 | -0.19 / -0.15 | 0 / 0 |
| grass thin 50/70, cull 70, LOD1, no wind past 15 m | 0 (all within +-0.1) | 0 | 0 | 0 |
| shrub shadows back on | 0 | +0.05 (shadow depths 0.58->0.61) | +0.03 | +0.1 |
| far trees culled past 1.5 km | 0 (culls nothing) | 0 | 0 | 0 |
Noise: identical stops differ by up to 0.2 (POV's two bases 8.23 / 8.45); Aerial's, Farm's and Grave's by 0.03-0.19. By the
GPU column (its timer misses part of the frame, PERF.md): far trees -0.31 / -0.39 / -0.46 / -0.30, backdrop -0.20 / -0.09 /
-0.10 / -0.01, capsule -0.08 / -0.05 / -0.02 / -0.04 (the distance-field upkeep sits mostly outside the GPU timer; frame is
the measure). Shadow depths (Smog, 100 m, 2 cascades): Farm 0.58, POV 0.40, Grave 0.37, Aerial 0.11.

## 5. Fix made (not built; ready to build)
- `Settings/GraphicsSettingsSubsystem.cpp` `QualityVariables`: `r.CapsuleIndirectShadows` {0, 0, 1, 1}. Nothing casts capsule
  shadows (no `bCastCapsule*` in Source/Tools/Art, no capsule pass in `Saved/Perf/Aerial_gpu.csv`), but while they're allowed
  `ShouldPrepareForDFInsetIndirectShadow` (5.8 `CapsuleShadowRendering.cpp` 1198, `DistanceFieldAmbientOcclusion.cpp` 841) keeps
  the scene's distance fields up every frame: Medium's only consumer. Measured -0.1..-0.28 frame (table above); look identical.
  Low already had none (BaseScalability `r.CapsuleShadows=0`). High could take it too (no DF consumer there either): not
  done, High isn't the line. Test: `Looter.Settings.QualityPresets` checks the value per preset and that the variable exists
  (`Tests/SettingsTests.cpp`, `HAL/IConsoleManager.h` included).

## 6. Proposals for Main to route (content; estimates)
1. Far-tree LODs (LODS.md section 2, `Art/Models/Vegetation/FarTrees.py` `finish()` line ~446: `obj['LODs'] = '50,25'`,
   `obj['LODScreens'] = '0.04,0.025'`; export `Tools\artrun.ps1 -Export FarTrees -Family FarTreesLods`, `Looter.ImportModels`;
   no level rebuild): the bound is -0.31..-0.53; their cost is sub-pixel triangles (0.23-0.37 M a view at ~300 each, 70-92%
   of them 500-900 m out), so -73% of the triangles should give ~-0.2..-0.35. The biggest single invisible GPU item left.
2. Smog/SmogThin sun `shadowDistance` 6000 (already in the working tree's `layout.json`; needs the `environment` build mode):
   cascades 100 -> 60 m, est. -0.15..-0.25 at Farm/POV/Grave (shadow depths 0.58/0.40/0.37 above), ~-0.05 Aerial. Visible only
   past 60 m, in the smog's haze.
3. Drop the grass options from any further list: no measurable GPU in any view. The shrub-shadow saving is ~0.05, not 0.1.
4. Backdrop (-0.23 Aerial/POV hidden): opaque, unlit, 6k triangles (`build_world_materials.py` `build_backdrop`, 3282): its
   cost is its depth-prepass fill of the sky band; no cheap invisible change found. Not proposed.
Estimate with 5 + 1 + 2 (not additive with certainty; GPU-bound so frame tracks GPU): Aerial 8.57 -> ~8.0-8.1, Farm 8.50 ->
~7.9-8.1, POV 8.34 -> ~7.7-7.9, Grave 7.96 -> ~7.5-7.6. MainStreet (8.06 quiet, 19:52) is not in FamilySplit: capsule should
give ~-0.1..-0.2 there (the DF upkeep doesn't depend on the view); far trees there unmeasured.

## 7. Window request (editor closed, `Saved\ArtPause.flag` up, the stray find stopped, no repack/bake/shader compile running)
Build first with this lane's change (and the working tree's LIGHTS round 2 / warm pass if Main wants them in). Each run with
`cpuwatch.ps1` beside it (CPU %, the clock's `% Processor Performance`, CPU seconds per process; logs to `Saved/Perf/`).
- **Run A (~6 min): the contention check.** `Tools\tour.ps1 -Map /Game/Maps/Lvl_RansomsRest -Views
  Art/Levels/RansomsRest/views.json -NoShots` (no -Warm, as at 19:51 and 22:00), then copy `Saved/Tour/Lvl_RansomsRest.csv`'s
  rows to `Saved/ArtPreviews/Step1/tour_perf2_quiet.csv`. Expect game 3.5-4.2 in every view and DeckFight ~7.2-7.6 if section
  1 holds; GPU/frame lower by the capsule switch.
- **Run B (~3 min): three Insights traces.** `Tools\tour.ps1 -Map /Game/Maps/Lvl_RansomsRest -Views Saved/Perf/Perf2Trace.json
  -NoShots -Exec "stat namedevents"`: `Perf2_Aerial.utrace` (the 19:57 stop repeated: a timer-by-timer diff), `Perf2_Hunt.utrace`
  (Grave -> MineStope, nine stops with MainStreet_Dusk's light change: names the timer and audio stalls if they recur),
  `Perf2_Deck.utrace` (Abel and ten adds: what the only game-bound view spends), then DeckFight_SlateInval between two plain
  stops (`Slate.EnableGlobalInvalidation 1`: what the HUD could give the fight; a number, not a fix).
After the window (no window needed): headless Insights exports (`UnrealInsights.exe -OpenTraceFile=… -AutoQuit -NoUI
-ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimingEvents <csv> -columns=ThreadName,TimerName,StartTime,EndTime,Duration,Depth
-threads=GameThread -timers=*"`, then `gtagg.js`), the fixes for what they show, and this file.
