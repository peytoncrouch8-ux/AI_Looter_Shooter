# Performance history

Measured with `Tools\perf.ps1`: standalone game window, CSV profiler, first frames (loading) skipped. Times are averages in ms.
Target: 60 fps at 1080p on the Medium preset on the reference PC (Radeon RX 580, Core i7-8700, 16 GB).

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
