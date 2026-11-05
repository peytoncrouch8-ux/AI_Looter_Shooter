# PERF lane (2026-11-04, round 4): the heaviest Medium views

Goal (the user's pick, "Alongside the next work"): the heaviest Medium views at 1080p on the reference PC (RX 580, i7-8700) under
8.3 ms. Before: the 42-view RR tour (10:40, `Saved/ArtPreviews/Step1/tour_final2.csv`) Aerial 9.36, Aerial_Dusk 9.36,
PointOverValley_FarShadow 9.10, Aerial_NoBeyond 8.63, Farm 8.58, PointOverValley 8.57, Grave 8.55, WatchtowerValley 8.40,
ChapelOverTown_FarShadow 8.38; the barge's well deck (perf row 15:38) 9.2 ms.

## Findings from the existing captures (no window needed)

1. **The render thread is not doing the work; it waits.** Deck capture `Profile(20261104_153800).csv` (1800 frames): render thread
   9.19 ms, of which `Exclusive/RenderThread/EventWait/Visibility` 5.88 ms; its own work ~3.3 ms (RenderOther 0.79,
   RenderThreadOther 0.39, RenderLighting 0.27, RDG 0.25+0.22+0.14, PostProcessing 0.19, InitViews_Scene 0.14, the rest under 0.1
   each). GPU 7.98, game 4.30, RHI 1.73. Draws 1228 (shadow depths 388, base 281, prepass 275, occlusion tests 132, Slate 87),
   1.76 M triangles, 16 lights (15 batched), 60.5k GPU-scene instances.
2. **What the wait is (first reading, disproved by run 1, see below):** in 5.8 the visibility tasks read last frame's hardware occlusion queries with `bWait = true`
   (`SceneVisibility.cpp` ~2824, `RHIGetRenderQueryResult`); on D3D12 a query's result is ready only when the GPU has finished the
   submitted batch that holds it (`D3D12Query.cpp` 332, the sync point). So each frame the render thread waits until the GPU has
   (nearly) finished the last frame, then does its own ~1-3 ms before the GPU gets the new frame's first commands: the GPU idles
   that long. Hence frame = GPU + 1.2-1.4 ms in every view since 10-14 (Aerial 10-14: frame 8.06, GPU 6.64, EventWait 5.06;
   10:40 tour: Aerial 9.36 / GPU 8.14, PointOverValley 8.57 / 7.36, Farm 8.58 / 7.36, deck 9.19 / 7.98).
   `r.NumBufferedOcclusionQueries` is 1 (the engine's default for desktop; 2 on iOS Low): with 2 the render thread reads the
   results of two frames ago, already done, and should stop waiting; cost: an object that comes out from behind cover appears one
   frame (8 ms) later, and the render thread can queue one more frame (input latency: to be measured, CSV `InputLatencyTime`).
   `r.HZBOcclusion 1` was tried 10-14: it removed the wait but cost the GPU more, frame slower.
3. **The `_FarShadow` views are not the game:** `Looter.Perf.FarShadow 1 400` adds a far cascade the user turned down on 10-07
   (`Docs/Areas/RansomsRest.md` step 5a: "Kept: two 100 m cascades"). PointOverValley_FarShadow (9.10) and
   ChapelOverTown_FarShadow (8.38) measure a rejected option; the game's views are PointOverValley 8.57 and ChapelOverTown 8.06.
4. Medium's GPU at the heavy views was (10-14, last per-pass capture of RR): base pass 1.85-2.2, prepass 0.75, post 0.59,
   unaccounted 0.56, TAA 0.46-0.50, reflection environment 0.31-0.38, lights 0.34-0.37, shadow depths 0.09 (Aerial) - 0.35,
   translucency 0.18-0.23. Today's GPU is ~1.5 ms higher (Aerial 6.64 -> 8.14): texture step 1, Tripo buildings, the barge, world
   variety, the detail layer. No per-pass capture of today's scene yet.

## Runs (editor closed, ArtPause flag on, the window 16:33-17:11)

Views files (under `Saved/Perf/`, made by a script, not in git): `OcclusionAB.json` (24 stops: Aerial, Aerial_Dusk, BargeDeck,
PointOverValley, Farm, Grave, each base / two buffered frames, twice), `CostSplit.json` (57 stops: Aerial, BargeDeck,
PointOverValley, each with one switch at a time: occlusion modes, shadows off, one cascade, shadow radius 0.1, CSM caching, ISM
off, foliage off, translucency off, particles off, point/spot lights off, skeletal off, view distance 0.4, LOD distance 1.5,
barge hidden, Beyond hidden, base again). BargeDeck = the 15:38 deck row's camera (8521, -5543, z 5647, yaw 90, fov 90), with the
barge placed by `Looter.Landfall 4, Looter.Barge.Show 3 aboard, Looter.Pilot.State Grappled` before the tour. Also run:
`GpuBound.json` (screen percentage), `Split2.json` (outliner folders hidden one at a time, clustered lights, anisotropy, mip bias,
`Looter.Perf.DrawShares` per view), `WindowMode.json` (windowed vs windowed fullscreen), `Candidates.json` (normal state, the
heaviest seven views: anisotropy 4, LOD distance 1.25, both). Command: `perf.ps1 -NoRecord [-GpuStats] -Map
/Game/Maps/Lvl_RansomsRest -Frames <stops x 7 s x ~120> -Skip 0 -Exec "Looter.Quality Medium,[Landfall state,]Looter.Tour
Saved/Perf/<file>.json noshots"`; per stop: a node script over the CSV (Tour events), as `Toolsperfviews.ps1` does.

## Results

### Run 1 (16:34, `Profile(20261104_163359).csv`, no GPU stats): two buffered occlusion frames change nothing
Tour with the barge at the berth (Landfall 4 state: game thread 5-8 ms in the aerials, was 3.9 in the 10:40 tour). Per stop
(ms; lat = CSV InputLatencyTime):

| Stop | frame | game | render | RT wait | GPU | lat | draws |
|---|---|---|---|---|---|---|---|
| Aerial base / OQ2 / base / OQ2 | 9.73 / 9.98 / 9.95 / 9.74 | 8.1 / 7.7 / 6.6 / 5.8 | 9.5-9.9 | 3.6 / 5.0 / 5.4 / 6.2 | 8.04 / 8.58 / 8.54 / 8.50 | 40 / 49 / 43 / 48 | 1965 |
| Aerial_Dusk | 10.02 / 9.80 / 9.90 / 9.80 | 5.4-6.5 | 9.8-10.0 | 5.3-5.8 | 8.51-8.56 | 43 / 49 / 44 / 49 | 1958 |
| BargeDeck (tour camera, no HUD/arms) | 8.74 / 8.78 / 8.87 / 8.70 | 4.8-6.6 | 8.7-8.9 | 4.6-5.2 | 7.45-7.58 | 40 / 44 / 41 / 43 | 1090 |
| PointOverValley | 9.30 / 9.19 / 9.44 / 9.24 | 6.5-6.9 | 9.2-9.4 | 4.5-5.0 | 7.85-8.01 | 41 / 45 / 41 / 46 | 1838 |
| Farm | 9.29 / 9.16 / 9.30 / 9.24 | 6.3-6.8 | 9.2-9.3 | 4.7-5.2 | 7.87-7.92 | 41 / 45 / 41 / 46 | 1767 |
| Grave | 9.12 / 9.17 / 9.18 / 9.25 | 5.9-7.7 | 9.1-9.2 | 4.3-4.9 | 7.77-7.87 | 41 / 45 / 41 / 46 | 1804 |

`r.NumBufferedOcclusionQueries 2`: frame within noise (+-0.15), the wait stays, input latency +4-9 ms. Rejected. So finding 2's
reading was wrong: the wait is the render thread at `Tasks.DynamicMeshElementsPipe->Wait` (`SceneVisibility.cpp` 4893, the
visibility and dynamic-mesh-gather tasks on the workers), and frame = GPU + 1.2-1.7 holds in every stop. Either the GPU is the
real limit (the render thread runs ahead until the query read throttles it: with 2 frames it simply runs a frame further ahead,
which is what the latency shows) or the worker-side visibility work is. Next: the cost split (GPU stats) and a screen-percentage
test (a GPU-bound frame falls with the pixels).

### Run 2 (16:40, `Profile(20261104_163942).csv`, GPU stats on): one switch at a time
Same tour state (barge at the berth). ms; "wait" = the render thread's EventWait/Visibility.

| Stop | frame | render | wait | GPU | draws | Mtris | prepass | base | shadow | lights | transl. |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Aerial base / base2 | 9.83 / 9.66 | 9.7 | 4.5-6.1 | 8.19 / 8.40 | 1964 | 2.61 | 0.86 | 3.29-3.56 | 0.12 | 0.44 | 0.19 |
| Aerial HZB occlusion | 10.01 | 3.40 | 0 | 8.66 | 1767 | 2.72 | 0.89 | 3.66 | 0.11 | 0.43 | 0.18 |
| Aerial no occlusion | 9.51 | 3.05 | 0 | 8.44 | 1943 | 2.93 | | | | | |
| Aerial no shadows | 9.43 | | | 8.22 | 1674 | | | | 0 | | |
| Aerial no ISM | 9.01 | | | 7.78 | 1212 | 2.60 | 0.66 | 3.34 | 0.06 | | |
| Aerial Beyond hidden | 9.03 | | | 7.80 | 1938 | 2.55 | 0.69 | 3.13 | | | |
| Aerial LOD distance 1.5 | 9.54 | | | 8.22 | 1949 | 2.42 | 0.80 | 3.48 | | | |
| Aerial barge hidden | 9.46 | | | 8.21 | 1898 | 2.36 | 0.80 | | 0.10 | 0.36 | |
| Aerial no translucency | 9.43 | | | 8.14 | | | | | | | 0 |
| BargeDeck base / base2 | 8.62 / 8.69 | 8.6 | 5.1-5.3 | 7.40 | 1077 | 1.36 | 0.39 | 1.02 | 0.59 | **2.04** | 0.16 |
| BargeDeck point+spot lights off | **6.99** | | 3.83 | **5.77** | 1095 | | | | | 0.38 | |
| BargeDeck barge hidden | 6.93 | | | 5.56 | 1094 | 0.95 | | | 0.33 | 0.38 | |
| BargeDeck no shadows | 7.94 | | | 6.67 | 680 | 0.69 | | | 0 | | |
| BargeDeck one cascade | 8.46 | | | 7.11 | 903 | 1.01 | | | 0.31 | | |
| BargeDeck no ISM | 7.68 | | | 6.54 | 415 | 1.32 | 0.15 | 0.66 | 0.47 | | |
| BargeDeck Beyond hidden | 8.18 | | | 6.92 | 1057 | | 0.21 | 0.74 | | | |
| BargeDeck no skeletal | 9.21 (noise/hitch) | | | 7.87 | | | | | | | |
| PointOverValley base / base2 | 9.16 / 9.18 | 9.2 | 5.1 | 7.85 | 1828 | 2.42 | 0.82 | 2.90 | 0.41 | 0.41 | 0.25 |
| POV no ISM | 8.07 | | | 6.88 | 998 | 2.42 | 0.48 | 2.59 | 0.29 | | |
| POV Beyond hidden | 8.75 | | | 7.43 | 1807 | 2.35 | 0.64 | 2.52 | | | |
| POV no shadows | 8.75 | | | 7.37 | 1393 | 2.02 | | | 0 | | |
| POV LOD distance 1.5 | 8.89 | | | 7.62 | 1829 | 2.14 | 0.72 | 2.74 | | | |
| POV CSM caching | 8.96 | | | 7.69 | 1703 | 2.07 | | | 0.29 | | |
| POV one cascade / radius 0.1 / shadow LOD x2 | 9.09 / 9.24 / 9.09 | | | 7.82 / 7.94 / 7.81 | | | | | | | |

Within +-0.15 (noise) in all three views: OQ2, foliage off, particles off, skeletal off (Aerial, POV), view distance 0.4, shadow
radius 0.1, shadow LOD x2. Fixed costs in every view: post 0.6, TAA 0.48, unaccounted 0.6, reflection environment 0.35.

### Run 3 (16:49, `Profile(20261104_164901).csv`): the frame is the GPU's pixels

| Stop | frame 100% / 70% / 50% screen | GPU | game |
|---|---|---|---|
| Aerial | 9.73 / 8.10 / 7.47 | 8.43 / 6.72 / 5.87 | 5.0 / 5.9 / 6.6 |
| BargeDeck | 8.81 / 6.91 / 6.46 | 7.55 / 5.67 / 4.69 | 5.4 / 5.6 / 6.4 |
| Farm | 9.28 / 7.84 / 7.76 | 7.93 / 6.46 / 5.77 | 6.1 / 6.0 / 7.7 |

**Conclusion: Medium on the RX 580 is GPU-bound, mostly per pixel, in every heavy view.** The render thread's 4-6 ms wait is
the throttle (it reads the GPU's occlusion results), not work: HZB occlusion takes the render thread to 3.4 ms and the frame
stays (GPU +0.3-0.5). Frame = GPU busy + 1.25-1.45 ms in every stop, every switch (the GPU timer leaves out idle gaps; the gap
is likely desktop composition of the windowed run and other GPU clients; not tested fullscreen). Every GPU ms saved is a frame
ms. The "render-thread bound" reading in `Docs/Performance.md` (10-14 to today) was this. Second line: the game thread in the
Landfall state is 5-8 ms (Farm at 50% screen is game-bound at 7.7): a CPU floor near the line for later.

### Run 4 (16:51, `Profile(20261104_165135).csv`, GPU stats on): what each group costs (hidden by outliner folder)
Frame change against the view's base (ms; GPU in brackets where it explains it). Landfall state.

| Hidden / switched | Aerial | BargeDeck | PointOverValley | Farm |
|---|---|---|---|---|
| (base frame / GPU) | 10.0 / 8.5 | 8.9 / 7.6 | 9.45 / 8.0 | 9.6 / 8.2 |
| Terrain (diag) | **-1.6** (base pass 3.70 -> 2.17) | -0.1 | -0.6 | -0.4 |
| Gameplay (Main 6 barge, ships, NPCs, chests) | -0.5 | **-2.2** (lights 2.07 -> 0.38) | -0.6 | **-0.9** (0.8 M triangles, translucency 0.35 -> 0.19) |
| Scatter | -0.3 | -0.5 | -0.3 | **-0.7** (prepass -0.25, base -0.26, shadow -0.12) |
| Cliffs | -0.4 (0.77 M triangles) | -0.2 | -0.3 (0.96 M triangles) | -0.2 |
| Buildings | -0.15 | -0.15 | -0.2 | -0.15 |
| Dressing / CliffLife / Orchard | -0.1 / -0.1 / 0 | 0 / -0.2 / 0 | 0 each | -0.1 / 0 / 0 |
| `r.MaxAnisotropy 4` (Medium is 8 since step 1) | -0.4 (base pass -0.46) | 0 | -0.2 | -0.15 |
| `r.Streaming.MipBias 1` (half-size maps) | 0 | -0.2 | 0 | 0 |
| Clustered deferred lights (`r.UseClusteredDeferredShading_ToBeRemoved 1`) | 0 (+0.3 clustered pass) | -0.2 (lights 2.07 -> 0.43, clustered 1.39) | +0.2 | +0.1 |

Clustered lights: small at the deck, worse elsewhere, and deprecated in 5.7 ("to be removed"): rejected.
`Looter.Perf.DrawShares` (in the run's log): Aerial 970 sections from 596 parts on screen (59,450 instances), POV 761 / 548,
Farm 649 / 432, BargeDeck 222 / 162. Scatter: 70 parts, 55,000 instances in every view. Cliffs 116-205 parts (SM_CliffFace_*).

### Run 5 (17:01, `Profile(20261104_170101).csv`): windowed fullscreen = windowed
`r.SetRes 1920x1080wf` vs `w`, alternating, two rounds, four views: every pair within +-0.3, no direction (Aerial 9.90/9.57,
9.68/9.58; deck 8.63/8.55, 8.87/8.53; POV 8.90/8.83, 9.21/8.95; Farm 8.92/9.07, 9.11/8.93). The frame-GPU gap (1.2-1.6) is not
desktop composition; perf.ps1's windowed numbers stand for the game in the user's mode.

### Run 6 (17:07, `Profile(20261104_170647).csv`): Medium-setting candidates, the normal state (no Landfall)
Frame ms: base (two runs) / anisotropy 4 / LOD distance 1.25 (`r.StaticMeshLODDistanceScale`) / both.

| View | base | aniso 4 | LOD 1.25 | both |
|---|---|---|---|---|
| Aerial | 9.35, 9.21 | 8.92 | 9.15 | **8.70** |
| Aerial_Dusk | 9.18, 9.15 | 8.82 | 9.14 | 8.74 |
| PointOverValley | 8.60, 8.63 | 8.40 | 8.40 | **8.26** |
| Farm | 8.63, 8.51 | 8.40 | 8.55 | **8.37** |
| Grave | 8.33, 8.26 | 8.13 | 8.08 | 7.96 |
| WatchtowerValley | 8.33, 8.34 | 8.22 | 8.19 | 7.97 |
| MainStreet | 8.00, 8.16 | 7.83 | 8.01 | 7.94 |

Settings alone take every valley view to 8.26-8.37 at best and the aerials to 8.7: not enough, and both are visible choices
(the user picked 8x on 11-04 morning; earlier LOD switches on the Tripo 50/25/12 LODs). The rest has to come off the content.

## What costs what (summary)

- The frame is the GPU (per pixel): frame = GPU busy + 1.2-1.6 ms, every view, every switch; 70% screen cuts 1.4-1.9 ms.
- BargeDeck (8.6-9.0): the barge's point lights 1.65 ms of frame (lights pass 2.04 GPU; 16 lights in view: the deck lamp
  26 m, nine work lamps 12 m, the hall and office lanterns, the cage light), shadows 0.7, ISMs 0.9 (town scatter below).
- Aerial (9.2-9.35 normal, 9.7-10.0 Landfall): the terrain material 1.5-1.6 (`M_Terrain`: ~20 texture samples per pixel,
  every one taken at every distance: macro, grass, rock and its two side-laid copies with their normals, ground noise, thatch,
  stony, bands, places, roads), 8x anisotropy 0.4 (most of it on those samples at a slant), Beyond 0.8, ISMs 0.8, cliffs 0.4.
- Valley views (POV, Farm, Grave, Watchtower; 8.3-8.6 normal, 9.1-9.6 Landfall): scatter 0.3-0.7, terrain 0.4-0.6, cliffs
  0.2-0.3, Beyond 0.4, anisotropy 0.1-0.2; in the Landfall state the barge group +0.6-0.9 (her 135k-triangle hull at LOD0-1 in
  four passes, the plumes' translucency, the Lamplighter).
- Fixed in every view: post 0.6, TAA 0.48, unaccounted 0.6, reflection environment 0.35, sun and sky lights 0.4.
- Not costs: occlusion settings, shadow radius/LOD/caching, foliage, particles, skeletal meshes, view distance, full mips.

## Plan to the line (owners; measured gains)

1. **KIT** (`World/BargeRules.h`, `BargeBoardingKit.cpp`, `BargeBoardingProps.cpp`): the barge's lamps. A deferred light costs
   the screen area inside its sphere; on the deck the camera stands inside the 26 m deck lamp and several 12 m work lamps.
   Reach to what each lights (deck lamp 2600 -> ~1200, work lamps 1200 -> ~600-700), the hall and office lanterns and the cage
   light only drawn while the player is below deck (`SetVisibility` by the hall trigger) or reach inside the hall. Gain up to
   1.65 ms at the deck; the deck then ~7.2-7.4.
2. **MATERIAL** (`build_world_materials.py` `build_terrain`): a Medium path in `M_Terrain` through a Quality Switch (Medium sets
   `r.MaterialQualityLevel` 2 = Medium, High/Epic keep today's look): no side-laid rock normals, the thatch/stony layers and the
   places' detail taken only near (inside the custom HLSL behind a `[branch]` on PixelDepth, sampled with explicit gradients),
   so far ground pays macro + grass + rock. Gain up to 1.5 ms in the aerials, 0.4-0.6 in the valley; it also cuts most of
   anisotropy's cost. Check the instance's shader stats (Medium vs High) and re-measure aerials.
3. **User decision (Main asks):** Medium's anisotropy 8 -> 4 (-0.1..-0.4, measured above) once (2) is in and re-measured; and
   `r.StaticMeshLODDistanceScale` 1.25 on Medium (-0..-0.25). PERF writes either line in `QualityVariables` on a yes.
4. **Content via Main / LEVEL:** the cliff faces (`SM_CliffFace_*`, 116-205 on screen, 0.8-1 M triangles) a LOD at ~0.3 screen
   size; Beyond's far set (0.4-0.8) cheaper LODs; the barge in the Landfall state (REWORK/KIT): `SM_DredgeBarge` LOD screen
   sizes 0.5/0.25/0.12 -> e.g. 0.6/0.35/0.18 once seen from the town, plumes' translucency.
5. **The tour (Main):** drop `PointOverValley_FarShadow` and `ChapelOverTown_FarShadow` from `Art/Levels/RansomsRest/views.json`:
   they add `Looter.Perf.FarShadow 1 400`, the far cascade the user turned down on 2026-10-07 (`Docs/Areas/RansomsRest.md`
   step 5a: "Kept: two 100 m cascades"; `WorldDevCommands.cpp` FarShadowCommand is a test command only). And add a deck view in
   the Landfall state (the views here: `Saved/Perf/*.json`, BargeDeck uses `"top": -99000` for an absolute height).
6. **Docs (Main):** `Docs/Performance.md` "render-thread bound" (rules line 43, the 10-14 and 10-22 notes, the round 3 row's
   note) is the GPU: the render thread's EventWait/Visibility is it waiting for the GPU's occlusion results.
7. **The 1.2-1.6 ms gap (frame minus GPU busy) is unexplained** and is in every view: not the occlusion read (OQ2, HZB: the
   gap stays with the render thread at 3.4 ms), not desktop composition (run 5). D3D12's GPU time leaves idle gaps out, so the GPU
   idles ~14% of each frame. Finding why needs a GPU+CPU timeline: Unreal Insights (ships with the engine, `-trace=cpu,gpu,frame`)
   would show it; it is a program the pipeline has not used, so the user is asked first (CLAUDE.md rule 3). If it is a sync
   bubble, closing it is worth more than items 3-4 together.

Nothing was changed in Source, Config or content by this lane; no Performance.md rows (all runs -NoRecord). The views now are
today's: normal state Aerial 9.2-9.35, Aerial_Dusk 9.15-9.2, POV 8.6, Farm 8.5-8.6, Grave 8.3, WatchtowerValley 8.3, MainStreet
8.0-8.2 (run 6); Landfall state +0.5-0.7 in those; the deck 8.6-9.0 (tour camera), 9.2 (the 15:38 first-person row).

## Round 2 (the user's yes, 2026-11-04 evening: "4x", LODs "Yes, 25% sooner", "Yes, use Insights")

Done (not built, not committed; Main builds):
- `Settings/GraphicsSettingsSubsystem.cpp` `QualityVariables`: `r.MaxAnisotropy` {1, 4, 8, 8} (Medium was 8),
  `r.StaticMeshLODDistanceScale` {1, 1.25, 1, 1} (new); the table's values are floats now (`QualitySettings` returns
  `TMap<FString, float>`; integer variables get whole numbers, which `IConsoleVariable::Set(float)` writes as "%g").
  `Settings/GraphicsSettingsSubsystem.h` signature; `Tests/SettingsTests.cpp` (`Looter.Settings.QualityPresets`: Medium 4x,
  LOD distance 1.25 on Medium only). `UI/Menus/SettingsMenuHelp.cpp` names no filtering: unchanged.
- `Art/Levels/RansomsRest/views.json`: `PointOverValley_FarShadow` and `ChapelOverTown_FarShadow` gone (they measured the far
  cascade the user turned down on 2026-10-07); `BargeDeck_Landfall` and `BargeDeck_Landfall_Stay` added last (after
  DeckFight_Dusk: they leave the world in Main 6's state). 42 views. `Docs/Pipeline/Levels.md` view list follows.
- `Docs/Performance.md`: the GPU-bound rule (the render thread's EventWait is a throttle) and the round 3 note corrected; the
  10-14 and 10-22 notes reworded.

Handed on (Main relays): KIT the lamps (deck lamp 2600 -> ~1200, work lamps 1200 -> ~600-700, the below-deck lights only while
the player is below); MATERIAL the `M_Terrain` Medium path; Main's list the cliff and Beyond LODs and the barge's LOD screen sizes.

### The Insights capture (prepared)
`Saved/Perf/Insights.json`: Aerial (settle), Aerial_Trace, BargeDeck_Landfall (settle, Main 6's state), BargeDeck_Trace; each
_Trace stop runs `Trace.File C:/Dev/AI_Looter_Shooter/Saved/Perf/Insights_<view>.utrace cpu,gpu,frame,bookmark,log,task` as it
arrives and `Trace.Stop` as it leaves (~7 s, ~750 frames, no camera move inside the trace). Run (editor closed, PowerShell):
`Tools\tour.ps1 -Map /Game/Maps/Lvl_RansomsRest -Views Saved/Perf/Insights.json -NoShots -Exec "stat namedevents"`
(named events: the render thread's `WaitForVisibilityTasks` and the RHI's scopes show by name). Read headless (no window
needed): `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealInsights.exe" -OpenTraceFile="<utrace>"
-ABSLOG="<log>" -AutoQuit -NoUI -ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimingEvents <csv>
-columns=ThreadId,ThreadName,TimerId,TimerName,StartTime,EndTime,Duration,Depth -threads=* -timers=*" -log` (and
`TimingInsights.ExportTimers` for the totals); the GUI on the same file if the export leaves it open.
What to look for:
1. The GPU queue per frame: where its ~1.3 ms idle sits (before the frame's first pass, around Present, or a wait on a fence
   inside the frame: async compute, a copy, the occlusion results' readback) and what the CPU side is doing then.
2. The render thread's wait: which task `WaitForVisibilityTasks` waits on and when that task's GPU query read returns against
   the GPU's frame end; the RHI thread and the submission thread: when frame N's first command list reaches the GPU.
3. Present: whether the RHI thread blocks in Present / the frame-latency wait, and for how long.
4. The worker side's visibility tasks (frustum, occlusion, relevance, dynamic mesh gather) to confirm the CPU's headroom, and
   the game thread's 5-8 ms in Main 6's state (the second line under 8.3).

## Round 2 measured (19:47-19:57, built at 312cb821: Medium 4x + LODs 25% sooner, KIT's lamps round 15, M_Terrain's Medium path)

- Deck row (recorded): **9.2 -> 8.1 ms** (GPU 8.0 -> 6.8, render 8.1, game 4.6, draws 1234, 1.53 M triangles);
  `Profile(20261104_194801).csv`. Tour camera on the deck (BargeDeck_Landfall / _Stay): 7.74 / 7.75.
- Horde row (recorded): **8.5 -> 8.3 ms** (GPU 7.2 -> 7.0, game 6.5); `Profile(20261104_194943).csv`.
- The 42-view tour (`Saved/Tour/Lvl_RansomsRest.csv`, 19:5x): mean 7.20 (the same 40 views as 10:40: 7.32 -> 7.17); over 8.3:
  **Aerial_Dusk 8.72, Aerial 8.71, Farm 8.61, PointOverValley 8.40** (were 9.36, 9.36, 8.58, 8.57); then Grave 8.20 (8.55),
  FarmBarge 8.16, Grave_Dusk 8.12, MainStreet 8.06, MainStreet_Dusk 8.06, Aerial_NoBeyond 8.05, WatchtowerValley 8.04 (8.40),
  SinkRim 8.00, ChapelOverTown 8.00; the rest 4.98-7.96.
- M_Terrain's Medium path shows ~0.1 in the aerial: run 6's settings alone gave Aerial 8.70 / GPU 7.60, now 8.71 / 7.45
  (`r.MaterialQualityLevel` is 2 in the run's log, so the Medium permutation is the one drawn): far ground was not where the
  terrain's cost sat. The valley views are 0.1-0.25 over run 6's settings-only figures (Farm 8.37 -> 8.61, POV 8.26 -> 8.40,
  Grave 7.96 -> 8.20, WatchtowerValley 7.97 -> 8.04, MainStreet 7.94 -> 8.06): a different process and the build carries round
  4's content too (312cb821: the deck fight, the barge's repacked maps, Tripo repacks); not split. An A/B of the terrain's
  Medium path alone needs `r.MaterialQualityLevel 1` vs 2 in one tour (a material recache per switch).

### Insights (Saved/Perf/Insights_Aerial.utrace, Insights_BargeDeck.utrace; GPU events exported to Aerial_gpu.csv, BargeDeck_gpu.csv)
- **The frame-GPU gap is solved: there is none.** The graphics queue is busy 8.49 of the Aerial's 8.68 ms period (98%: idle
  0.10 inside frames, 0.08 between), deck 7.45 of 7.63. The CSV's `GPUTime` reports ~1.2 ms less than the queue's real
  occupancy (it leaves out the frame's work outside the scene, Present's 0.29 and its wait accounting). So the frame is the
  GPU, full stop; no sync bubble to close. Read perf rows' GPU column as "GPU - ~1.2".
- Aerial, GPU per frame (ms): base pass 2.84, post 1.44 (TAA 0.49, local exposure 0.13 plus ~0.27 unscoped, bloom 0.13,
  downsample chain 0.11, histogram 0.03), prepass 0.81, deferred lighting 0.80 (DiffuseIndirectAndAO 0.38: sky and reflection
  captures; lights 0.36), shadow depths 0.19, fog 0.14, sky 0.13 + LUTs 0.12, HZB 0.13, velocities 0.125, distance fields 0.22
  (DistanceFields 0.11, atlas 0.09, object buffers 0.02: kept up every frame although Medium has no consumer: no Lumen, DFAO
  or DF shadows), Slate 0.09.
- Deck, GPU per frame: deferred lighting 1.58 (lights 1.14: the lamps still the biggest item), post 1.39, base 1.03, shadow
  depths 0.64 (atlas 0.58), prepass 0.52, velocities 0.30, distance fields 0.21.

## What is left over 8.3 ms (normal state) and where the rest can come from
| View | now | needs | candidates (measured share) |
|---|---|---|---|
| Aerial, Aerial_Dusk | 8.71, 8.72 | -0.42 | Beyond 0.66 (Aerial_NoBeyond 8.05), cliffs 0.3-0.4, scatter 0.3, the distance-field upkeep 0.22 |
| Farm | 8.61 | -0.31 | scatter 0.7, Gameplay group (barge, NPCs, chests) up to 0.9 in Landfall, Orchard/Dressing 0.1, DF 0.2 |
| PointOverValley | 8.40 | -0.10 | scatter 0.3, cliffs 0.3, Beyond 0.4, DF 0.2 |
Margins under 0.2 ms: Grave 8.20, FarmBarge 8.16, Grave_Dusk 8.12. The deck (8.1 first person) has 0.2.
1. **The distance-field upkeep on Medium (0.22 ms in every view, no visible change):** find what asks for the scene's
   distance fields when Lumen, DFAO and DF shadows are off (a material with DistanceToNearestSurface, Niagara collision on the
   global DF, or the always-on DF scene update) and stop it on Medium. PERF's next step (an Insights CPU export of
   PrepareDistanceFieldScene's caller and `r.DistanceFields.*` A/B rows).
2. **Beyond (0.66 in the aerial, 0.4 in POV):** its far set's LODs/material on Medium (LEVEL, through Main).
3. **Cliffs (0.3-0.4; 0.8-1 M triangles):** `SM_CliffFace_*` a cheaper LOD (content, Main).
4. **Scatter (Farm 0.7):** Medium density or the grass's masked prepass (content/PCG: a visible choice; the user's).
5. Lamps at the deck still 1.14 GPU (KIT: further reach cuts if the deck must gain margin).

### Next: the distance-field upkeep (0.2 ms GPU + ~0.1 render thread in every view, no visible change expected)
Why Medium keeps the scene's distance fields: `ShouldPrepareDistanceFieldScene` (`DistanceFieldAmbientOcclusion.cpp` 813)
returns true for `ShouldPrepareForDFInsetIndirectShadow` = capsule indirect shadows supported (`r.CapsuleShadows` 1 from the
engine's ShadowQuality@1, `r.CapsuleIndirectShadows` 1) and the CapsuleShadows show flag on. Nothing in the project casts
capsule shadows (no `bCastCapsuleIndirectShadow`/`DirectShadow` in Source or Tools), so `r.CapsuleIndirectShadows 0` on Low and
Medium should drop the DF scene update (DistanceFields 0.11, atlas 0.09, object buffers 0.02 GPU; PrepareDistanceFieldScene
0.07-0.12 on the render thread) and change nothing seen. High/Epic keep it (Lumen and DFAO need the DFs anyway).
A/B ready: `Saved/Perf/CapsuleAB.json` (Aerial, Farm, PointOverValley, Grave, the deck in Main 6's state; base / off, twice):
`Tools\perf.ps1 -Label "PERF capsule A/B" -NoRecord -GpuStats -Map /Game/Maps/Lvl_RansomsRest -Frames 19000 -Skip 0 -Exec
"Looter.Quality Medium,Looter.Tour Saved/Perf/CapsuleAB.json noshots"` (~2.5 min). On a gain: `r.CapsuleIndirectShadows`
{0, 0, 1, 1} in `QualityVariables` and the test.

## The content list for a LODS lane (2026-11-04 night; numbers from runs 2, 4 and the 19:5x tour)

Rule (CLAUDE.md): walkable terrain meshes keep every triangle in their Nanite fallback (Medium draws it, collision is cooked
from it): the core's `Tile_*` stay untouched. Medium has no LOD chain on a Nanite mesh: 5.8's `FMeshNaniteSettings` has a
fallback only (percent or relative error), drawn at every distance; only a mesh with Nanite off gets LODs on Medium.
Triangle counts below are `RHI/PrimitivesDrawn` drops when the family is hidden (all passes: prepass, base, shadow cascades);
instanced scatter's triangles don't show there (GPU-driven draws) and are given as instances.

| Family | Reachable? collision from | On screen | Cost now (frame ms) | Change | Expected |
|---|---|---|---|---|---|
| Cliff faces `SM_CliffFace_A..D`, `_A2..D2` (`Art/Models/Rocks/Cliffs.py`, ~10k source, `FALLBACK` 25% = ~2.5k) | yes, but collision is their `UCX_` hulls (3-4 + one per fallen block), not the fallback: free of the rule | Aerial 159 pieces / 0.77 M tris, POV 205 / 0.96 M, Farm 116 / 0.49 M, deck 19 / 0.12 M | Aerial 0.42 (GPU 0.33), POV 0.24-0.3, Farm 0.2, deck 0.26 | fallback by relative error (`FallbackTarget` RelativeError, tuned to ~12% average) or `FALLBACK` 25 -> 12; check the near look at the plateau's faces | -0.15..-0.2 Aerial/POV, -0.1 Farm |
| Beyond ring `<Area>_Ring_<n>` (M_Terrain master, `<Area>RingMacro`; RING_TRIANGLES 38k) | no: past the boundary walls; the upland sectors collide (fallback-cooked, only bullets and grenades land there), the canyon's don't: free of the rule | in every open view; Beyond's triangles 0.06-0.07 M in all: its cost is pixels | Beyond all: Aerial 0.66 (tour: 8.71 vs Aerial_NoBeyond 8.05), RidgeFoot 0.72, POV 0.41, deck 0.44, PointWest 0.43, DeckWest 0.45 | not LODs: a Beyond instance of M_Terrain with the places, roads and side-laid steep reads switched off (MATERIAL), or bake its look into the macro map | split pending (FamilySplit) |
| Canyon wall `<Area>_CanyonWall_<n>` (RockCliff) | no, no collision | the canyon views | in Beyond's total | its material's detail reads off far (MATERIAL) | split pending |
| Backdrop `<Area>_Backdrop_<n>` (M_Backdrop, unlit, Nanite off, 6k) | no | every horizon | in Beyond's total; cheap by design (10-06: 0.22 max) | none | - |
| Far trees `FarTrees_SM_FarPine_A/B`, `SM_FarBroadleaf` (AInstancedScenery, 2,624 instances, never distance-culled, no shadows) | no | every open view | in Beyond's total | cull past ~1.5 km on Medium (they are specks there) or an impostor ring | split pending |
| Grass `SM_GrassClump_A/B/C` (34k instances), `SM_TallGrass_A` (6.7k), `SM_Clover_A` (3.4k), flowers (1.7k), `SM_DryTuft_A/B` (2k) (`GroundCover.py`: opaque, 150-300 tris a patch, LOD1 50% at screen 0.1, wind by WPO, no shadows) | yes, no collision: free of the rule | Farm, POV, Grave, valley views; culled at Medium's 0.6 x 40-50 m | all scatter: Farm 0.7 (prepass 0.25, base 0.26, shadows 0.12), POV 0.3, Aerial 0.3, deck 0.5 | the user's options below | - |
| Shrubs `SM_Sagebrush_A/B/C` (4.9k), `SM_Bush_*`, `SM_Juniper_*` (casting shadows) | yes, hulls | valley views | most of scatter's 0.12 shadow share at Farm | shadows off on Medium | ~-0.1 Farm |
| Buildings `SM_RR_*`, Tripo (LODs 50/25/12 at 0.5/0.25/0.12, x1.25 sooner on Medium since tonight) | yes, hulls | Aerial 0.40 M tris, Farm 0.37 M, POV 0.25 M | 0.15-0.2 | none beyond tonight's 1.25 | - |
| The barge `SM_DredgeBarge` (135k, Tripo LODs 50/25/12 at 0.5/0.25/0.12) | yes (deck), UCX | Main 6's state | hull hidden: Aerial 0.37, POV 0.13 | LOD screen sizes 0.6/0.35/0.18 (REWORK's mesh) | -0.05..-0.1 POV/Farm in Main 6 |

### Farm's scatter: options to put to the user (Farm needs -0.31; all scatter costs it 0.7)
Estimates from the measured total and its passes; the exact ms come from `Saved/Perf/FamilySplit.json` (needs
`Dev/PerfSplitCommands.cpp` built: `Looter.Perf.Scatter` thin/cull/shadow/lod/wpo, `Looter.Perf.HideLabel`).
1. **Thinner grass on Medium (70% of the patches):** about -0.2 at Farm, -0.1 in the other valley views; up close the meadow
   is a little sparser; High/Epic keep it all. (50%: about -0.3, visibly thin.)
2. **Grass ends nearer on Medium (70% of today's distance, ~17-21 m instead of 24-30 m):** about -0.1..-0.15; the grass line
   moves toward the player (the ground's patterns carry on past it to 30-40 m).
3. **Shrubs without shadows on Medium:** about -0.1; sagebrush and bushes stand without a ground shadow.
Invisible extras measured in the same run: the grass's wind stopped past 15 m and its LOD1 everywhere (an upper bound for an
earlier LOD switch).

### Next window (after the erosion rebuild and a build with `Dev/PerfSplitCommands.cpp`)
1. `Tools\perf.ps1 -Label "PERF capsule A/B" -NoRecord -GpuStats -Map /Game/Maps/Lvl_RansomsRest -Frames 19000 -Skip 0 -Exec
   "Looter.Quality Medium,Looter.Tour Saved/Perf/CapsuleAB.json noshots"` (~2.5 min)
2. `Tools\perf.ps1 -Label "PERF family split" -NoRecord -GpuStats -Map /Game/Maps/Lvl_RansomsRest -Frames 50000 -Skip 0 -Exec
   "Looter.Quality Medium,Looter.Tour Saved/Perf/FamilySplit.json noshots"` (56 stops, ~8 min: Beyond's four pieces, the
   grass options, shrub shadows, far-tree cull, the capsule switch, on Aerial, Farm, PointOverValley, Grave)

## Round 3 (the user's yes: "Shrubs without shadows" on Medium only; no grass thinning, no nearer grass end)
Done (not built, not committed; "ready to build"):
- `Settings/GraphicsSettingsSubsystem.cpp` `QualityVariables`: `Looter.Scatter.HighShadows` {0, 0, 1, 1}.
- `World/LumenScatterSubsystem.cpp/.h`: the console variable (default 1; its change handler walks every game world),
  `ShadowHighOnlyTag()`, `HighShadowsAllowed()`, `ApplyHighShadows(Actor, bShadows)` (any size, not behind `SmallRadius`),
  called beside `LeaveOutOf` at begin play and on level streaming. The LODS lane's tag (`build_island_scatter.py`
  `ShadowHighOnly` on sagebrush, rabbitbrush, bushes; not junipers) arrives with the post-erosion scatter generation.
- Tests: `Looter.World.LumenScatter.HighShadows` (tagged part loses and regains its shadow, untagged untouched, the variable
  exists); `Looter.Settings.QualityPresets` checks the preset's value. CODEMAP line.
- `Saved/Perf/FamilySplit.json`: `_ShrubNoShadow` became `_ShrubShadowsOn` (shadows back on for the stop: the saving by
  difference). `_NoFarTrees` answers the LODS lane's far-tree LODs (50/25% at screen 0.04/0.025 in `FarTrees.py`): take them
  if hiding the far trees saves 0.1 ms or more in the open views (Aerial, POV); their 0.23-0.37 M triangles per view at ~300
  each are invisible to `PrimitivesDrawn`, so only the frame and the base/prepass passes tell.
