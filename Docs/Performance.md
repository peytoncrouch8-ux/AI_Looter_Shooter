# Performance history

**Target:** 120 fps (8.3 ms) at 1080p on the Medium preset, at the heaviest view, on the reference PC (Radeon RX 580, Core i7-8700,
16 GB). It was 60 fps until 2026-09-30. Nanite is for High and Epic only and Lumen for Epic only, so everything must look right on the Nanite
fallback. Per-pass budgets: `Docs/TutorialIsland.md`. How to measure: `Docs/Pipeline/Workflow.md`, "Performance".

Measured with `Tools\perf.ps1`: standalone game window, CSV profiler, first frames (loading) skipped; times are averages in ms.
Run it before and after anything that could change cost; it appends a row to the table below. Keep the table to the latest runs;
move older rows to `Docs/Archive/Performance_history.md` (the full log of every run since 2026-09-29).

## Rules

- Presets Low, Medium, High, Epic = the engine's scalability level + Lumen + anti-aliasing method + Nanite. Lumen, TSR and
  distance field AO only on Epic (TAA otherwise); High = Nanite + virtual shadow maps + SSAO + screen-space reflections; Nanite
  off below High (it costs the RX 580 about 2.5 ms). All presets render at full resolution (the engine's own Medium renders at
  71%). A fresh install starts on Medium. Since 2026-11-04 (the art audit's step 1) Medium keeps its full-size textures
  (r.Streaming.MipBias 0, was 1), 8x anisotropy (was 2; 4x since that evening, the PERF rule below), creature LOD0 (r.SkeletalMeshLODBias 0, was 1) and a 1000 MB streaming
  pool (was 600), as High; Low stays the engine's own. Measured 2026-11-04 (PC not quiet: agents running): the 42-view Ransom's
  Rest tour mean 7.19 -> 7.35 ms (PointOverValley left out: a 57 ms streaming hitch on the first view after the change), every
  view +0.3..0.8 ms, the heaviest Aerial_Dusk 8.69 -> 9.40, Aerial 8.58 -> 9.38, PointOverValley_FarShadow 8.73 -> 9.08 (seven
  views over 8.3 ms, three were before); the 10-spider horde 7.4 -> 8.6 ms (GPU 6.2 -> 7.2, triangles 1.94 -> 2.21 M: the
  spiders' LOD0); LMG fire into it 8.4 -> 9.0. Which setting costs what ("isolate" rows, each one put back alone): anisotropy
  2 -> no change (8.6), mip bias 1 -> 8.4, skeletal LOD bias 1 -> 8.3, all three back (the control) -> 8.1, not the night's 7.4:
  the PC was ~0.7 ms noisier (render thread 7-8 ms, was 5), so step 1's own cost is the isolates' 0.5 ms in the horde (creature
  LOD0 0.3, full mips 0.2, anisotropy 0) and the tour's +0.3..0.8 per view is mostly that noise. Close-up grabs
  (`Saved/ArtPreviews/Step1/closeup`): the filtering shows on slanted boards, full mips show nothing on Tripo walls (soft at the
  source, audit finding 4), LOD0 barely at 3 m. The user's pick (2026-11-04): keep the filtering and the full mips, creature LOD
  bias back to the engine's Medium (1). Final (10:34, step 1 as picked, the reworked Tripo barge with the detail layer in the
  level): the tour mean 7.39 ms over 42 views (7.19 before step 1 at a quieter PC), the heaviest Aerial 9.36, Aerial_Dusk 9.36,
  PointOverValley_FarShadow 9.10 (nine views over 8.3, mostly aerial and noise); the barge views SpyglassBarge 7.38 -> 7.82,
  FarmBarge 7.84 -> 8.17, DeckFight_Dusk 7.50 -> 7.40 (that was round 2's boxed hull, 56k triangles, which the user then
  rejected too); the horde 8.4 ms, LMG fire 8.6 ("Art round 1 final" rows; the earlier "scripted barge hull" rows measured the
  hull the user dropped first). Round 3 (15:38, the Tripo hull adjusted piece by piece and lengthened: 135k triangles, 152
  hulls, three detail slots; the kit's 16 models; 9 work lamps without shadows): ON the barge's well deck (`Looter.Landfall 4,
  Looter.Barge.Show 3 aboard, Looter.Pilot.State Grappled`; the first perf row there) 9.2 ms / 108 fps, GPU-bound (render 9.2
  is the throttle below, GPU 8.0, game 4.2): her deck is 47 m over the town; the town horde 8.5. Split by the PERF lane
  (`Docs/Work/2026-11-04-Art/PERF.md`): the barge's point lights 1.65 ms of the deck's frame (lights pass 2.04 GPU).
- **Medium is GPU-bound per pixel in every heavy view (PERF lane, 2026-11-04 evening, six A/B tours):** at 70% screen the
  frame drops 1.4-1.9 ms; every GPU saving shows 1:1 in the frame. The rows' GPU column reads 1.2-1.6 ms under the frame
  because the CSV's `GPUTime` under-counts: Unreal Insights shows the graphics queue busy 98% of the frame (Aerial 8.49 of
  8.68 ms, the deck 7.45 of 7.63), no sync bubble. The render thread's 4-6 ms `EventWait/Visibility` is a throttle, not
  work: it waits for the GPU's last-frame occlusion results; HZB occlusion takes the render thread to 3.4 ms and the frame
  stays (GPU +0.3-0.5); `r.NumBufferedOcclusionQueries 2` changes nothing but adds 4-9 ms of input latency. So draw calls cost
  through the GPU, and what costs is pixels: the barge's lamps at her deck 1.65, `M_Terrain` in the aerials 1.5 (~20 texture
  reads a pixel), Beyond 0.4-0.8, scatter 0.3-0.7, cliffs 0.3-0.4, 8x filtering 0.1-0.4. The user's yes (2026-11-04 evening):
  Medium filters 4x (was 8x) and takes static mesh LODs a quarter sooner (`r.StaticMeshLODDistanceScale` 1.25), measured
  together Aerial 9.3 -> 8.7, PointOverValley 8.6 -> 8.26, Farm 8.6 -> 8.37, Grave 8.3 -> 7.96. The tour's two `_FarShadow`
  views went (they measured the far cascade turned down on 2026-10-07); `BargeDeck_Landfall` (+`_Stay`) measures her deck.
  Built with the KIT's lamps (round 15) and `M_Terrain`'s Medium path (312cb821, 19:47): the deck 9.2 -> 8.1 ms, the horde
  8.5 -> 8.3; the 42-view tour mean 7.20 (the same 40 views 7.32 -> 7.17), over 8.3 only Aerial_Dusk 8.72, Aerial 8.71, Farm
  8.61, PointOverValley 8.40 (were 9.36, 9.36, 8.58, 8.57). What is left and where: `Docs/Work/2026-11-04-Art/PERF.md`.
- Small instanced scatter (meshes under 1.5 m radius) stays out of Lumen and the distance fields (`ULumenScatterSubsystem`):
  Ransom's Rest's ~66k grass/flower/pebble instances made Lumen's surface cache recapture ~6,900 tiny cards
  (`LumenSceneUpdate` 400+ ms, frames of 1.3 s alternating with normal ones, even standing still).
- Presets on Ransom's Rest, RX 580, 10 heavy views (2026-10-14, after the fix): Low 4.8-7.9 ms, Medium 6.1-8.1, High 14.2-19.7
  (51-70 fps), Epic 35.6-44.7 (22-28 fps, Lumen's steady cost: screen probe gather ~13 ms, scene lighting ~6). Before: High
  21-657 ms, Epic 37-714 ms. Medium is GPU-bound (frame = GPU + ~1.4 ms; the render thread's 5 ms "EventWait/Visibility" is
  its throttle, waiting on the GPU's occlusion results: HZB occlusion removes it but costs the GPU more, frame slower).
- Medium's GPU at the heaviest views (Farm, PointOverValley, Aerial; 6.3-6.8 ms): base pass 1.7-1.9, prepass 0.75-0.9, post 0.6,
  unaccounted 0.55, TAA 0.47, shadow depths 0.35-0.52, lights 0.34, reflection environment 0.3-0.4, translucency 0.2. What
  groups cost (hidden, frame): scatter 0.3-0.9 ms, cliffs 0.4-0.5 (204 pieces, already merged into ~40 draws by the engine's
  dynamic instancing: their cost is triangles; 25% fallback already), buildings 0.1-0.2 (240-370 draws), dressing and cliff
  life 0-0.3 each. `foliage.DensityScale` changes nothing at runtime (PCG scatter applies it when generated).
- Hardware ray tracing off. Substrate off (same frame time, fewer shaders to compile).
- The outline (0.18 ms) and the clouds (0.4 ms) are cheap enough for every preset; the outline needs no optimizing.
- Medium for 120 fps (2026-09-30): cascaded shadows, no SSAO/DFAO, motion blur off by default.
- Every PCG scatter instance scales with `foliage.DensityScale`, for the presets.
- Volumetric clouds were the big cost on the island (9.5 ms with the engine's layer, 2 ms thinned); the sky dome costs almost
  nothing.
- Preset costs on the old sky-island scene (2026-09-29, Lumen on High then): Low 7.4 ms, Medium 12.3, High 22.0, Epic 38.1.

## Current reference numbers (Medium, 1080p)

- Fights after 12.2 (2026-10-21, re-measured with the PC quiet, `Saved\ArtPause.flag` set): 10 spiders fighting in town
  (`Looter.Perf.Horde Spider 10 Basic 0 -1400 90`) 7.5-7.7 ms over 4 runs (game thread 6.4-6.9 including ~1 ms waiting on the
  render thread); director off 7.6-8.2. The two 9.1/9.7 ms rows before were a busy PC: every game-thread bucket rose
  together, UI and DebugHUD ~1.9x too. The director's own cost (CSV `Exclusive/GameThread/Creature*`, `FightRefresh`):
  under 0.05 ms; facing the player while circling the ring (`CreatureFace`) 0.3 ms for 10. The spawn horde
  (`Looter.Perf.Horde Spider 8 Basic`) stands in the farm's safe zone and never fights: measure fights in town.

- Noon re-measure (2026-10-22, eea24a97, quiet PC, ArtPause on): 37-view RR tour all under 8.3 ms; heaviest Aerial 8.1, Aerial_Dusk 8.1, PointOverValley_FarShadow 8.1 (1,849 draws), Farm 7.8, Grave 7.7; GPU-bound (read then as render-thread bound; see the PERF rule above). Worst-frame hitches only at Aerial_Dusk 32 ms and MineDrift 34 ms (as before). Spawn 7.8 (p95 8.5); 8 spiders in town 7.6 (p95 8.7); 8 Unpaid in town 7.7 (p95 9.4); Slag Bomb every 4 s 7.8 (p95 9.4). Draws up ~250-400 since 10-14 (world variety, char art): margin ~0.2 ms.
- Ransom's Rest after the polish (2026-10-14, fc7363a: cliff life, town dressing with its clutter culled at ~30 m, animals,
  combat FX): 46 views, the heaviest 8.1 ms (PointOverValley_FarShadow 1,574 draws), Aerial 8.05, Aerial_Dusk 8.05, Farm
  8.0, PointOverValley 7.7; render thread = frame (its throttle on the GPU), GPU 6.8 at most. Margin ~0.2 ms.
- Ransom's Rest after the mountain (2026-10-14, 75708bb): all 37 views at 128-206 fps, the heaviest 7.8 ms (Farm,
  PointOverValley_FarShadow, Aerial_Dusk), spawn 7.7 ms (GPU 6.4, render thread 7.7: GPU-bound, ~1,250-1,450 draws
  in the valley's open views, up from ~1,000). New views: DepotWaiting 6.9, WatchtowerValley 7.2 (taken from the yard below the deck: the view stood off it), LiftHeadBench 7.0,
  MineGallery 6.1 (13 lights, 26 lanterns: fine), MineDrift 5.4, MineStope 5.2. Hitches (worst frame ~30 ms) at MineDrift and
  Aerial_Dusk only. Margin to 8.3 ms is ~0.5 ms.
- Ransom's Rest (2026-10-08): all 28 views at 149-216 fps (6.7 ms at the heaviest); spawn 6.3 ms; base pass 1.1-1.2 ms, prepass
  0.5-0.7, shadows at most 0.5; no single art asset is a real cost.
- Skyreach with Crossroads Town round 1 (2026-10-08): 160-226 fps at every view (heaviest, above the town, 6.3 ms).
- Hostile load (2026-10-06, RR town gate): 1 Unpaid chasing 5.5 ms; 12 chasing 5.9 ms.

## Latest runs

| Date | Change | Commit | Resolution | Frame | fps | p95 | Game | Render | RHI | GPU | Draws | Triangles | Load (s) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2026-10-06 | RR town gate, 12 Unpaid chasing, no GPU stats (step 16) | b075ad2 | 1920x1080 | 5.9 | 169 | 6.3 | 4.3 | 5.9 | 1.6 | 4.9 | 672 | 1115644 | 1.04 |
| 2026-10-07 | RR deck at dusk, Abel and 10 adds hunting (plan view 13, Looter.Abel.Perf 10), Medium | a64961d | 1920x1080 | 5.5 | 182 | 6.1 | 5.0 | 5.4 | 1.7 | 4.4 | 395 | 285509 | 1.73 |
| 2026-10-09 | Polish waves 1-2 (fauna, ambience, feedback), Skyreach | 38331af | 1920x1080 | 6.1 | 164 | 6.9 | 4.1 | 6.0 | 1.7 | 4.9 | 663 | 665050 | 1.27 |
| 2026-10-09 | Polish waves 1-2 (late-summer land, grass 40-50 m, fauna, camps), Ransom's Rest | 38331af | 1920x1080 | 6.8 | 147 | 7.4 | 3.7 | 6.8 | 1.6 | 5.5 | 1001 | 1075257 | 2.44 |
| 2026-10-14 | RR after the mountain expansion and the play-through fixes, spawn | 75708bb | 1920x1080 | 7.7 | 130 | 8.2 | 3.4 | 7.7 | 1.6 | 6.4 | 1375 | 1714813 | 2.55 |
| 2026-10-15 | Ellis as the player's body (retarget live), RR spawn | ffb56c2 | 1920x1080 | 7.5 | 134 | 7.9 | 3.6 | 7.4 | 1.6 | 6.2 | 1408 | 1672126 | 2.70 |
| 2026-10-15 | Ellis round 2: swing springs, 96 bones, hat tip, RR spawn | 53d5f22 | 1920x1080 | 7.6 | 131 | 8.1 | 3.7 | 7.6 | 1.6 | 6.4 | 1407 | 1670356 | 2.85 |
| 2026-10-15 | Ellis round 6: hat seated, hair cards, cape chains, 142 bones, RR spawn | 0ad1801 | 1920x1080 | 7.6 | 131 | 8.2 | 3.9 | 7.6 | 1.6 | 6.4 | 1406 | 1671510 | 2.95 |
| 2026-10-16 | Ember powers round 1, RR spawn | 92cb87e | 1920x1080 | 7.7 | 130 | 8.2 | 3.8 | 7.7 | 1.6 | 6.4 | 1421 | 1673227 | 2.76 |
| 2026-10-20 | Heroes: Ellis's body, RR spawn | d09573b8 | 1920x1080 | 7.7 | 130 | 8.2 | 3.9 | 7.7 | 1.6 | 6.4 | 1427 | 1665261 | 3.84 |
| 2026-10-20 | Heroes: Crane's body, RR spawn | d09573b8 | 1920x1080 | 7.7 | 129 | 8.3 | 3.9 | 7.7 | 1.7 | 6.4 | 1427 | 1665893 | 2.82 |
| 2026-10-20 | Heroes: Gauge's body, RR spawn | d09573b8 | 1920x1080 | 7.7 | 131 | 8.2 | 3.7 | 7.6 | 1.6 | 6.4 | 1426 | 1663690 | 2.77 |
| 2026-10-20 | Heroes: Pike's body, RR spawn | d09573b8 | 1920x1080 | 7.7 | 130 | 8.2 | 3.8 | 7.7 | 1.6 | 6.4 | 1429 | 1665078 | 2.80 |
| 2026-10-20 | Heroes: Odessa's body, RR spawn | d09573b8 | 1920x1080 | 7.7 | 129 | 8.3 | 4.1 | 7.7 | 1.8 | 6.4 | 1428 | 1663670 | 2.81 |
| 2026-10-20 | Fight baseline: Ellis, 8 spiders, no power, RR spawn | d09573b8 | 1920x1080 | 7.7 | 129 | 8.3 | 4.9 | 7.7 | 1.7 | 6.5 | 1459 | 1760641 | 2.80 |
| 2026-10-20 | Powers: Gauge Slag Bomb every 8 s, 8 spiders | d09573b8 | 1920x1080 | 7.8 | 128 | 8.5 | 5.3 | 7.8 | 1.8 | 6.5 | 1464 | 1787378 | 2.83 |
| 2026-10-20 | Powers: Gauge Iron Hide every 6.5 s, 8 spiders | d09573b8 | 1920x1080 | 7.7 | 129 | 8.2 | 4.9 | 7.7 | 1.7 | 6.5 | 1439 | 1739149 | 2.78 |
| 2026-10-20 | Powers: Odessa Fifty-Two Pickup every 2 s, 8 spiders | d09573b8 | 1920x1080 | 7.8 | 128 | 8.3 | 5.2 | 7.7 | 1.7 | 6.5 | 1516 | 1777962 | 2.99 |
| 2026-10-20 | Powers: Crane Grave Ward every 8 s, 8 spiders | d09573b8 | 1920x1080 | 8.0 | 126 | 8.5 | 4.9 | 8.0 | 1.7 | 6.7 | 1473 | 1767249 | 2.80 |
| 2026-10-20 | Powers: Pike Spyglass every 8 s, 8 spiders | d09573b8 | 1920x1080 | 7.8 | 128 | 8.4 | 5.1 | 7.8 | 1.7 | 6.5 | 1481 | 1783767 | 2.78 |
| 2026-10-20 | Powers: Ellis Sundown every 6.5 s, 8 spiders | d09573b8 | 1920x1080 | 7.7 | 129 | 8.2 | 4.8 | 7.7 | 1.7 | 6.5 | 1471 | 1783713 | 2.83 |
| 2026-10-20 | Gravemother Barrow fight at the RR spawn (her moves as they come) | d09573b8 | 1920x1080 | 7.7 | 130 | 8.2 | 4.1 | 7.7 | 1.7 | 6.4 | 1446 | 1636166 | 2.82 |
| 2026-10-20 | Tiers: 8 Regular spiders (baseline) | b6fbda06 | 1920x1080 | 7.7 | 129 | 8.3 | 4.9 | 7.7 | 1.7 | 6.5 | 1463 | 1770817 | 2.96 |
| 2026-10-20 | Tiers: 8 Hallowed spiders | b6fbda06 | 1920x1080 | 7.8 | 129 | 8.3 | 4.8 | 7.7 | 1.7 | 6.5 | 1473 | 1788075 | 2.83 |
| 2026-10-20 | Tiers: 8 Ethereal spiders | b6fbda06 | 1920x1080 | 7.8 | 128 | 8.4 | 5.0 | 7.8 | 1.7 | 6.5 | 1472 | 1813687 | 2.80 |
| 2026-10-21 | Gunfeel baseline: 8 spiders | 41539fc7 | 1920x1080 | 7.9 | 127 | 9.0 | 5.9 | 6.7 | 1.8 | 6.5 | 1465 | 1762357 | 9.03 |
| 2026-10-21 | Gunfeel 12.1 done: 8 spiders | 8dafc347 | 1920x1080 | 7.6 | 132 | 8.0 | 5.3 | 7.6 | 1.6 | 6.2 | 1450 | 1779561 | 3.22 |
| 2026-10-21 | Fight 12.2: 8 spiders | 852087ea | 1920x1080 | 9.1 | 109 | 11.2 | 9.1 | 4.7 | 2.5 | 6.6 | 1469 | 1784185 | 3.74 |
| 2026-10-21 | Fight 12.2: 10 spiders in town | 852087ea | 1920x1080 | 9.7 | 103 | 11.9 | 9.7 | 4.7 | 2.5 | 6.3 | 1592 | 1404293 | 3.54 |
| 2026-10-21 | Fight 12.2 re-measured, quiet PC: 10 spiders fighting in town (director on) | 987ea592 | 1920x1080 | 7.7 | 129 | 9.6 | 6.9 | 6.7 | 1.8 | 6.3 | 1593 | 1386292 | n/a |
| 2026-10-21 | ...same, Looter.Fight.Director 0 | 987ea592 | 1920x1080 | 7.6 | 132 | 8.1 | 6.4 | 7.6 | 1.7 | 6.3 | 1614 | 1445202 | n/a |
| 2026-10-21 | ...8 spiders at the spawn (in the farm's safe zone: they don't fight) | 987ea592 | 1920x1080 | 7.7 | 130 | 8.2 | 5.0 | 7.7 | 1.6 | 6.4 | 1468 | 1761967 | n/a |
| 2026-10-22 | Loot 12.3: 8 spiders in town, Ember on all every 2 s | 62bf264c | 1920x1080 | 7.6 | 132 | 8.2 | 6.4 | 7.5 | 1.8 | 6.3 | 1596 | 1375535 | 3.38 |
| 2026-10-22 | VFX pass: horde 8 spiders in town | 87262e96 | 1920x1080 | 7.5 | 133 | 8.0 | 5.8 | 7.5 | 1.7 | 6.2 | 1590 | 1378697 | 3.42 |
| 2026-10-22 | VFX pass: horde 8 spiders + Slag Bomb every 4 s (Gauge) | 87262e96 | 1920x1080 | 7.9 | 126 | 8.6 | 6.4 | 7.9 | 1.8 | 6.7 | 1630 | 1406206 | 3.41 |
| 2026-10-22 | World variety BEFORE, RR spawn | 37636b49 | 1920x1080 | 7.7 | 130 | 8.2 | 3.9 | 7.7 | 1.7 | 6.4 | 1441 | 1661795 | 3.38 |
| 2026-10-22 | World variety AFTER, RR spawn | 37636b49 | 1920x1080 | 7.8 | 128 | 8.4 | 4.6 | 7.8 | 1.9 | 6.5 | 1705 | 1739014 | 3.92 |
| 2026-10-22 | World variety AFTER (repeat), RR spawn | 37636b49 | 1920x1080 | 7.7 | 129 | 8.3 | 3.9 | 7.7 | 1.7 | 6.4 | 1704 | 1736720 | 3.50 |
| 2026-10-22 | World variety bolder tones, RR spawn | c64525df | 1920x1080 | 7.8 | 129 | 8.4 | 4.7 | 7.7 | 1.9 | 6.5 | 1703 | 1736594 | 9.57 |
| 2026-10-22 | Char art import: element parts, CHAR-MAT, ghosts/slime, Crane/Odessa faces; RR spawn, 8 spiders | 1ab00ccf | 1920x1080 | 7.9 | 126 | 9.0 | 6.2 | 6.5 | 2.0 | 6.5 | 1744 | 1845440 | 3.95 |
| 2026-10-22 | Char art import: 8 Unpaid fighting in town (painted ghost) | 1ab00ccf | 1920x1080 | 7.5 | 133 | 8.1 | 6.8 | 7.5 | 1.9 | 6.3 | 1792 | 1323127 | 3.52 |
| 2026-10-22 | PERF noon: RR spawn | eea24a97 | 1920x1080 | 7.8 | 128 | 8.5 | 5.1 | 7.8 | 2.0 | 6.5 | 1702 | 1735219 | 3.96 |
| 2026-10-22 | PERF noon: 8 spiders in town | eea24a97 | 1920x1080 | 7.6 | 132 | 8.7 | 6.3 | 7.2 | 1.9 | 6.3 | 1786 | 1408106 | 3.56 |
| 2026-10-22 | PERF noon: 8 Unpaid in town | eea24a97 | 1920x1080 | 7.7 | 129 | 9.4 | 7.1 | 6.5 | 2.0 | 6.3 | 1795 | 1313323 | 3.65 |
| 2026-10-22 | PERF noon: 8 spiders + Slag Bomb every 4 s (Gauge) | eea24a97 | 1920x1080 | 7.8 | 128 | 9.4 | 6.9 | 7.1 | 1.9 | 6.4 | 1842 | 1450826 | 3.79 |
| 2026-10-22 | GROUND-DETAIL places (M_Terrain places, clutter, yarrow), RR spawn | bc9a98cf | 1920x1080 | 8.0 | 125 | 9.1 | 6.4 | 7.8 | 2.3 | 6.6 | 1709 | 1736668 | 4.36 |
| 2026-10-22 | GROUND-DETAIL places, place map mip by distance, RR spawn | bc9a98cf | 1920x1080 | 7.8 | 129 | 8.3 | 3.9 | 7.8 | 1.7 | 6.5 | 1708 | 1735934 | 3.64 |
| 2026-10-22 | GROUND-DETAIL places final (pattern within 60 m, map within 120 m), RR spawn | bc9a98cf | 1920x1080 | 7.7 | 130 | 8.2 | 4.0 | 7.7 | 1.7 | 6.5 | 1709 | 1735518 | 3.74 |
| 2026-10-23 | Gun feel pass + grips + arachnids, RR spawn | 7ea84f74 | 1920x1080 | 7.8 | 128 | 8.3 | 4.0 | 7.8 | 1.7 | 6.6 | 1709 | 1735289 | 3.66 |
| 2026-10-23 | Gun feel pass: 8 spiders, RR spawn | 7ea84f74 | 1920x1080 | 7.9 | 127 | 8.4 | 4.0 | 7.9 | 1.7 | 6.6 | 1709 | 1735634 | 3.62 |
| 2026-10-27 | playtest rounds 1-2 (Unpaid kit, grenades, legendary burst), RR spawn | 4b24064a | 1920x1080 | 7.8 | 127 | 8.4 | 4.1 | 7.8 | 1.7 | 6.6 | 1716 | 1742954 | 3.44 |
| 2026-10-28 | overnight 10-28: gun patterns M_Gun 800 instr, Tripo AR/melee/chests, cloth, cup fit | 2a54db8d | 1920x1080 | 7.6 | 132 | 7.9 | 4.0 | 7.6 | 1.6 | 6.5 | 1726 | 1744268 | 3.50 |
| 2026-10-28 | Core before (tag card in) | e31979df | 1920x1080 | 6.2 | 162 | 7.6 | 6.0 | 4.6 | 2.0 | 4.9 | 686 | 663844 | 3.85 |
| 2026-10-28 | Core before, 8 Unpaid horde | e31979df | 1920x1080 | 7.8 | 129 | 11.4 | 7.7 | 3.2 | 1.9 | 4.9 | 782 | 685104 | 1.85 |
| 2026-10-29 | Core after (rounds 1-2) | 597d0d6c | 1920x1080 | 6.1 | 163 | 6.5 | 4.3 | 6.1 | 1.7 | 5.1 | 690 | 671597 | 1.92 |
| 2026-10-29 | Core after, 8 Unpaid horde | 597d0d6c | 1920x1080 | 7.0 | 142 | 8.1 | 7.0 | 3.3 | 1.8 | 5.0 | 766 | 673220 | 1.80 |
| 2026-10-30 | Overnight 10-30: base | e0a73e5e | 1920x1080 | 6.2 | 162 | 6.6 | 4.2 | 6.0 | 1.6 | 5.1 | 685 | 698595 | 2.81 |
| 2026-10-30 | Overnight 10-30: 8 Unpaid horde | e0a73e5e | 1920x1080 | 6.4 | 156 | 7.4 | 6.4 | 3.0 | 1.8 | 5.0 | 747 | 747170 | 1.84 |
| 2026-10-30 | Overnight 10-30: horde + 3 shrieks | e0a73e5e | 1920x1080 | 7.9 | 127 | 10.4 | 6.1 | 7.7 | 1.7 | 6.8 | 777 | 798943 | 1.83 |
| 2026-10-30 | Overnight 10-30: horde + 1 shriek every 2.5 s | e0a73e5e | 1920x1080 | 6.5 | 153 | 7.8 | 6.2 | 4.3 | 1.8 | 5.3 | 762 | 766871 | 1.79 |
| 2026-10-30 | Overnight 10-30: Tripo Gravemother fight in town | e0a73e5e | 1920x1080 | 6.4 | 157 | 6.9 | 4.8 | 6.1 | 1.7 | 5.2 | 803 | 718376 | 1.80 |
| 2026-11-01 | Wardens r1 + Mains 2-3 + Tripo people/farm/Drover/Gravemother | 05f5512d | 1920x1080 | 5.8 | 171 | 6.6 | 4.9 | 5.6 | 1.8 | 4.9 | 693 | 697998 | n/a |
| 2026-11-04 | Phase 3 after (changes 1-12, 70059aa3): 10 Basic spiders in town | 70059aa3 | 1920x1080 | 7.4 | 135 | 8.6 | 7.3 | 5.0 | 1.9 | 6.2 | 1917 | 1940484 | 6.03 |
| 2026-11-04 | Phase 3 fire: LMG 30 s into 10 Basic spiders | 70059aa3 | 1920x1080 | 8.4 | 119 | 9.9 | 8.4 | 4.0 | 2.0 | 6.6 | 1497 | 1267313 | 4.46 |
| 2026-11-04 | Phase 3 fire: Rifle 30 s into 10 Basic spiders | 70059aa3 | 1920x1080 | 8.5 | 118 | 9.8 | 8.4 | 4.7 | 2.0 | 6.8 | 1555 | 1483828 | 4.34 |
| 2026-11-04 | Phase 3 fire: Shotgun 30 s into 10 Basic spiders | 70059aa3 | 1920x1080 | 8.4 | 119 | 9.9 | 8.3 | 4.4 | 2.0 | 6.7 | 1572 | 1345256 | 4.25 |
| 2026-11-04 | Phase 3 fire: SMG 30 s into 10 Basic spiders | 70059aa3 | 1920x1080 | 8.3 | 120 | 9.8 | 8.2 | 4.9 | 1.9 | 6.7 | 1596 | 1417355 | 4.42 |
| 2026-11-04 | Phase 3 fire: LMG, muzzle and impact lights off | 70059aa3 | 1920x1080 | 8.5 | 118 | 10.0 | 8.5 | 3.9 | 2.0 | 6.6 | 1576 | 1378181 | 4.35 |
| 2026-11-04 | Phase 3 before (e3390778 code): 10 Basic spiders in town | 70059aa3 | 1920x1080 | 9.4 | 107 | 9.8 | 7.4 | 7.8 | 2.3 | 6.7 | 1898 | 1880873 | 14.70 |
| 2026-11-04 | Art step 1 after (Medium keeps texture and creature detail): 10 Basic spiders in town | 3c85407f | 1920x1080 | 8.6 | 116 | 9.6 | 7.2 | 7.9 | 1.8 | 7.2 | 1914 | 2212572 | 5.68 |
| 2026-11-04 | Art step 1 after: LMG 30 s into 10 Basic spiders | 3c85407f | 1920x1080 | 9.0 | 111 | 11.5 | 9.0 | 4.5 | 2.1 | 6.9 | 1592 | 1817050 | 4.69 |
| 2026-11-04 | Art step 1 isolate: anisotropy back to 2 | 3c85407f | 1920x1080 | 8.6 | 117 | 10.3 | 7.6 | 7.1 | 1.9 | 6.9 | 1899 | 2289524 | 7.40 |
| 2026-11-04 | Art step 1 isolate: mip bias back to 1 | 3c85407f | 1920x1080 | 8.4 | 118 | 9.2 | 6.7 | 8.3 | 1.8 | 7.2 | 1900 | 2247444 | 4.51 |
| 2026-11-04 | Art step 1 isolate: skeletal LOD bias back to 1 | 3c85407f | 1920x1080 | 8.3 | 120 | 9.1 | 6.8 | 8.1 | 1.8 | 7.0 | 1914 | 1958588 | 4.57 |
| 2026-11-04 | Art step 1 isolate: all three back (control) | 3c85407f | 1920x1080 | 8.1 | 123 | 9.0 | 7.2 | 7.9 | 1.8 | 6.8 | 1913 | 1935069 | 4.71 |
| 2026-11-04 | Art round 1 final (step 1 as picked, scripted barge hull): 10 Basic spiders in town | 3c85407f | 1920x1080 | 8.6 | 116 | 10.8 | 8.5 | 4.6 | 2.1 | 6.3 | 1923 | 1898579 | 8.54 |
| 2026-11-04 | Art round 1 final: LMG 30 s into 10 Basic spiders | 3c85407f | 1920x1080 | 8.9 | 113 | 11.2 | 8.8 | 4.8 | 2.0 | 7.0 | 1525 | 1311132 | 5.23 |
| 2026-11-04 | Art round 1 final (step 1 as picked, reworked Tripo hull + detail layer): 10 Basic spiders in town | 3c85407f | 1920x1080 | 8.4 | 119 | 9.2 | 6.8 | 8.2 | 1.8 | 7.1 | 1914 | 1889788 | 5.82 |
| 2026-11-04 | Art round 1 final (reworked Tripo hull): LMG 30 s into 10 Basic spiders | 3c85407f | 1920x1080 | 8.6 | 116 | 10.1 | 8.5 | 5.2 | 2.0 | 7.1 | 1631 | 1443430 | 4.67 |
| 2026-11-04 | Art round 3: on the reworked barge's well deck (9 work lamps, kit props) | 527a6d5d | 1920x1080 | 9.2 | 108 | 9.9 | 4.2 | 9.2 | 1.7 | 8.0 | 1228 | 1760609 | 6.19 |
| 2026-11-04 | Art round 3: 10 Basic spiders in town | 527a6d5d | 1920x1080 | 8.5 | 118 | 9.1 | 6.2 | 8.5 | 1.7 | 7.2 | 1911 | 2077555 | 4.14 |
| 2026-11-04 | PERF round 2 (Medium 4x filtering, LODs 25% sooner) + KIT lamps r15 + M_Terrain Medium path: on the barge's well deck | 312cb821 | 1920x1080 | 8.1 | 123 | 8.7 | 4.6 | 8.1 | 1.8 | 6.8 | 1234 | 1532296 | 6.22 |
| 2026-11-04 | PERF round 2 + KIT lamps r15 + M_Terrain Medium path: 10 Basic spiders in town | 312cb821 | 1920x1080 | 8.3 | 120 | 9.1 | 6.5 | 8.1 | 1.7 | 7.0 | 1918 | 2053139 | 4.17 |
| 2026-11-04 | Art round 5: on the barge's well deck | 312cb821 | 1920x1080 | 8.2 | 122 | 9.0 | 5.4 | 8.2 | 2.1 | 6.9 | 1145 | 1454652 | 8.38 |
| 2026-11-04 | Art round 5: 10 Basic spiders in town | 312cb821 | 1920x1080 | 8.8 | 113 | 10.5 | 7.6 | 7.8 | 2.0 | 7.3 | 1841 | 1934115 | 4.52 |
| 2026-11-05 | Art round 6: on the barge's well deck | f3bd38c7 | 1920x1080 | 7.9 | 126 | 8.5 | 4.3 | 7.9 | 1.6 | 6.7 | 1173 | 1572859 | 7.06 |
| 2026-11-05 | Art round 6: 10 Basic spiders in town | f3bd38c7 | 1920x1080 | 7.9 | 127 | 8.5 | 6.4 | 7.8 | 1.7 | 6.7 | 1749 | 1666296 | 4.12 |
| 2026-11-05 | Art round 7: on the barge's well deck | ec41a743 | 1920x1080 | 7.9 | 126 | 8.6 | 4.5 | 7.9 | 1.7 | 6.7 | 1192 | 1600212 | 4.48 |
| 2026-11-05 | Art round 7: 10 Basic spiders in town | ec41a743 | 1920x1080 | 7.9 | 126 | 8.5 | 6.0 | 7.9 | 1.7 | 6.7 | 1830 | 1693789 | 4.18 |
| 2026-11-05 | Art round 8: on the barge's well deck | 294b8134 | 1920x1080 | 8.0 | 125 | 8.6 | 4.7 | 7.9 | 1.7 | 6.8 | 1208 | 1600508 | 4.67 |
| 2026-11-05 | Art round 8: 10 Basic spiders in town | 294b8134 | 1920x1080 | 8.0 | 124 | 8.7 | 6.3 | 7.9 | 1.7 | 6.8 | 1858 | 1733498 | 4.19 |
