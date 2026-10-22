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
  71%). A fresh install starts on Medium.
- Small instanced scatter (meshes under 1.5 m radius) stays out of Lumen and the distance fields (`ULumenScatterSubsystem`):
  Ransom's Rest's ~66k grass/flower/pebble instances made Lumen's surface cache recapture ~6,900 tiny cards
  (`LumenSceneUpdate` 400+ ms, frames of 1.3 s alternating with normal ones, even standing still).
- Presets on Ransom's Rest, RX 580, 10 heavy views (2026-10-14, after the fix): Low 4.8-7.9 ms, Medium 6.1-8.1, High 14.2-19.7
  (51-70 fps), Epic 35.6-44.7 (22-28 fps, Lumen's steady cost: screen probe gather ~13 ms, scene lighting ~6). Before: High
  21-657 ms, Epic 37-714 ms. Medium is GPU-bound (frame = GPU + ~1.4 ms; the render thread's 5 ms "EventWait/Visibility" is
  waiting on occlusion results: HZB occlusion removes it but costs the GPU more, frame slower).
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

- Ransom's Rest after the polish (2026-10-14, fc7363a: cliff life, town dressing with its clutter culled at ~30 m, animals,
  combat FX): 46 views, the heaviest 8.1 ms (PointOverValley_FarShadow 1,574 draws), Aerial 8.05, Aerial_Dusk 8.05, Farm
  8.0, PointOverValley 7.7; render thread = frame, GPU 6.8 at most. Margin ~0.2 ms: the next dressing must save draws first.
- Ransom's Rest after the mountain (2026-10-14, 75708bb): all 37 views at 128-206 fps, the heaviest 7.8 ms (Farm,
  PointOverValley_FarShadow, Aerial_Dusk), spawn 7.7 ms (GPU 6.4, render thread 7.7: render-thread bound, ~1,250-1,450 draws
  in the valley's open views, up from ~1,000). New views: DepotWaiting 6.9, WatchtowerValley 7.2 (taken from the yard below the deck: the view stood off it), LiftHeadBench 7.0,
  MineGallery 6.1 (13 lights, 26 lanterns: fine), MineDrift 5.4, MineStope 5.2. Hitches (worst frame ~30 ms) at MineDrift and
  Aerial_Dusk only. Margin to 8.3 ms is ~0.5 ms: new dressing must come instanced (draw calls are the cost).
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
