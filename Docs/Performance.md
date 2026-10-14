# Performance history

**Target:** 120 fps (8.3 ms) at 1080p on the Medium preset, at the heaviest view, on the reference PC (Radeon RX 580, Core i7-8700,
16 GB). It was 60 fps until 2026-09-30. Lumen and Nanite are for High and Epic only, so everything must look right on the Nanite
fallback. Per-pass budgets: `Docs/TutorialIsland.md`. How to measure: `Docs/Pipeline.md`, "Performance".

Measured with `Tools\perf.ps1`: standalone game window, CSV profiler, first frames (loading) skipped; times are averages in ms.
Run it before and after anything that could change cost; it appends a row to the table below. Keep the table to the latest runs;
move older rows to `Docs/Archive/Performance_history.md` (the full log of every run since 2026-09-29).

## Rules

- Presets Low, Medium, High, Epic = the engine's scalability level + Lumen + anti-aliasing method + Nanite. Lumen only on High and
  Epic; TSR only on Epic (TAA otherwise); Nanite off below High (it costs the RX 580 about 2.5 ms). All presets render at full
  resolution (the engine's own Medium renders at 71%). A fresh install starts on Medium.
- Hardware ray tracing off. Substrate off (same frame time, fewer shaders to compile).
- The outline (0.18 ms) and the clouds (0.4 ms) are cheap enough for every preset; the outline needs no optimizing.
- Medium for 120 fps (2026-09-30): cascaded shadows, no SSAO/DFAO, motion blur off by default.
- Every PCG scatter instance scales with `foliage.DensityScale`, for the presets.
- Volumetric clouds were the big cost on the island (9.5 ms with the engine's layer, 2 ms thinned); the sky dome costs almost
  nothing.
- Preset costs on the old sky-island scene (2026-09-29): Low 7.4 ms, Medium 12.3, High 22.0, Epic 38.1.

## Current reference numbers (Medium, 1080p)

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
