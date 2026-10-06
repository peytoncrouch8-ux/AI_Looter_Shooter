# Performance history

Measured with `Tools\perf.ps1`: standalone game window, CSV profiler, first frames (loading) skipped. Times are averages in ms.
Target: 120 fps (8.3 ms) at 1080p on the Medium preset on the reference PC (Radeon RX 580, Core i7-8700, 16 GB); it was 60 fps until 2026-09-30.

| Date | Change | Commit | Resolution | Frame | fps | p95 | Game | Render | RHI | GPU | Draws | Triangles | Load (s) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2026-09-29 | Baseline before cleanup | 78a9c94 | 1920x1080 | 23.8 | 42 | 24.9 | 7.2 | 23.8 | 2.5 | 21.5 | 2125 | 1194674 | 5.08 |
| 2026-09-29 | Props baked to Nanite static meshes | e6a165d | 1920x1080 | 28.6 | 35 | 29.9 | 7.3 | 28.6 | 2.6 | 25.9 | 1813 | 177977 | 0.88 |
| 2026-09-29 | PCG meadow replaces placed grass (604 instances) + soft collision fix + shared shaders | dd073a5 | 1920x1080 | 29.7 | 34 | 31.0 | 7.3 | 29.7 | 2.7 | 27.0 | 2011 | 190089 | 0.89 |
| 2026-09-29 | Quality preset Epic (the presets commit, measured on top of e99da87) | e99da87 | 1920x1080 | 38.0 | 26 | 39.5 | 7.4 | 38.0 | 2.7 | 35.2 | 1957 | 186765 | 0.83 |
| 2026-09-29 | Quality preset High (the presets commit, measured on top of e99da87) | e99da87 | 1920x1080 | 22.0 | 45 | 23.3 | 7.2 | 22.0 | 2.5 | 19.7 | 1649 | 186871 | 0.80 |
| 2026-09-29 | Quality preset Low (the presets commit, measured on top of e99da87) | e99da87 | 1920x1080 | 7.4 | 134 | 9.0 | 7.4 | 4.6 | 1.8 | 5.1 | 1602 | 219279 | 0.80 |
| 2026-09-29 | Quality preset Medium (the presets commit, measured on top of e99da87) | e99da87 | 1920x1080 | 12.3 | 81 | 13.4 | 6.3 | 12.3 | 2.0 | 10.9 | 1703 | 223596 | 0.78 |
| 2026-09-29 | Quality preset Medium, no Substrate or ray tracing | 7e88635 | 1920x1080 | 12.5 | 80 | 13.5 | 6.3 | 12.4 | 2.0 | 10.8 | 1729 | 225499 | 0.77 |
| 2026-09-29 | Quality preset Epic, no Substrate or ray tracing | 7e88635 | 1920x1080 | 38.1 | 26 | 39.4 | 7.3 | 38.1 | 2.7 | 35.3 | 1950 | 186900 | 0.78 |
| 2026-09-29 | Skeletal spider, Medium | c540328 | 1920x1080 | 11.9 | 84 | 13.2 | 5.3 | 11.9 | 2.0 | 10.4 | 487 | 209196 | 0.79 |
| 2026-09-30 | Guns from parts, baked ammo boxes, Medium | 886339d | 1920x1080 | 11.8 | 85 | 12.4 | 4.8 | 11.8 | 2.0 | 10.3 | 498 | 208386 | 0.66 |
| 2026-09-30 | Ground fixes: full terrain fallback, 3.5 m meadow patches fitted to the ground, props settled, Medium | 3402b1b | 1920x1080 | 12.1 | 83 | 12.9 | 4.9 | 12.1 | 2.0 | 10.6 | 489 | 238332 | 0.50 |
| 2026-09-30 | Medium for 120 fps: cascaded shadows, no SSAO/DFAO, motion blur off by default | 48dccdc | 1920x1080 | 6.7 | 148 | 7.3 | 5.2 | 6.7 | 1.9 | 5.6 | 490 | 315514 | 0.59 |
| 2026-09-30 | Tutorial island (new style), spawn view, Medium; tour.ps1: 139-182 fps at all 7 viewpoints | c536389 | 1920x1080 | 6.2 | 162 | 7.3 | 3.3 | 6.0 | 1.6 | 4.6 | 363 | 606664 | 0.85 |
| 2026-10-01 | Finished tutorial island (waterfall, smoke, pond plants, meadow trees, tutorial, XP bar), spawn, Medium; tour 151-201 fps | 32673d6 | 1920x1080 | 5.6 | 179 | 6.0 | 3.9 | 5.6 | 1.6 | 4.6 | 452 | 614310 | 0.77 |
| 2026-10-01 | FPS counter on the HUD, hold-to-equip, kill-weapon ammo | 4284e2c | 1920x1080 | 5.6 | 177 | 6.0 | 3.9 | 5.6 | 1.6 | 4.6 | 455 | 615448 | 0.77 |
| 2026-10-01 | Bestiary page, slim creature tags, ammo pickup radius | e8a6e4d | 1920x1080 | 5.6 | 179 | 6.0 | 3.9 | 5.6 | 1.6 | 4.6 | 456 | 616177 | 0.76 |
| 2026-10-01 | Ammo pickup feed, divided creature bars, bestiary unknown pages | ba7cff7 | 1920x1080 | 5.7 | 176 | 6.4 | 4.3 | 5.6 | 1.7 | 4.6 | 457 | 617539 | 0.76 |
| 2026-10-01 | Guns from Bullpup/Ranchhand parts (rifle in hand), Meadow Wolf spider with LODs, Medium | a5d9770 | 1920x1080 | 5.6 | 179 | 6.0 | 4.2 | 5.6 | 1.5 | 4.6 | 461 | 643017 | 0.82 |
| 2026-10-01 | New HUD (hex weapon slots), drag-and-drop inventory, per-gun wear (rifle in hand), Medium | 30f0d97 | 1920x1080 | 5.6 | 179 | 6.0 | 4.7 | 5.6 | 1.6 | 4.5 | 449 | 622828 | 0.82 |
| 2026-10-01 | HUD ammo ticks repaint only on change (rifle in hand), Medium | 30f0d97 | 1920x1080 | 5.6 | 178 | 6.2 | 4.8 | 5.5 | 1.6 | 4.6 | 443 | 621581 | 0.80 |
| 2026-10-01 | Main menu + save sessions (dev play, no session), Medium | 79ac9f2 | 1920x1080 | 5.6 | 180 | 6.0 | 4.2 | 5.6 | 1.5 | 4.6 | 395 | 601805 | 0.81 |
| 2026-10-01 | Round slots, magazine gauge, minimap compass, no ammo light, Medium | 45260c4 | 1920x1080 | 5.7 | 176 | 6.9 | 5.0 | 4.9 | 1.8 | 4.5 | 428 | 627705 | 0.95 |
| 2026-10-01 | Round 2: distant creatures update less often, health ring, XP bar, FOV setting (90), ammo beams | 0175bcd | 1920x1080 | 5.6 | 179 | 6.0 | 3.0 | 5.6 | 1.4 | 4.6 | 445 | 629243 | 1.00 |
| 2026-10-01 | Scattered trees, rocks, stumps and logs collide; XP bar with the level in the middle; 1.5 m ammo beams | 52c06af | 1920x1080 | 5.7 | 176 | 6.1 | 3.0 | 5.7 | 1.5 | 4.5 | 449 | 630955 | 1.10 |
| 2026-10-06 | Ransom's Rest greybox (step 5a), grave spawn, Medium; tour 174-228 fps at 20 viewpoints (Beyond 0.2-0.3 ms, dusk no dearer) | db5f8ec | 1920x1080 | 5.5 | 180 | 6.0 | 2.7 | 5.5 | 1.4 | 4.6 | 489 | 970407 | 1.05 |
| 2026-10-06 | Tutorial island with the jetty and sky islands (steps 10, 13), spawn, Medium; tour 153-231 fps at 10 viewpoints | db5f8ec | 1920x1080 | 5.6 | 179 | 6.0 | 3.0 | 5.6 | 1.5 | 4.6 | 448 | 628868 | 1.01 |
| 2026-10-06 | RR town gate, 1 Unpaid chasing (baseline for the 12) | b075ad2 | 1920x1080 | 5.5 | 180 | 6.0 | 2.8 | 5.5 | 1.4 | 4.5 | 465 | 1047482 | 1.22 |
| 2026-10-06 | RR town gate, 12 Unpaid chasing, with GPU stats (step 16) | b075ad2 | 1920x1080 | 5.9 | 169 | 6.6 | 4.6 | 5.8 | 1.6 | 4.8 | 661 | 1114988 | 1.11 |
| 2026-10-06 | RR town gate, 12 Unpaid chasing, no GPU stats (step 16) | b075ad2 | 1920x1080 | 5.9 | 169 | 6.3 | 4.3 | 5.9 | 1.6 | 4.9 | 672 | 1115644 | 1.04 |
