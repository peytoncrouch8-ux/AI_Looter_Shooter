# Performance history

Measured with `Tools\perf.ps1`: standalone game window, CSV profiler, first frames (loading) skipped. Times are averages in ms.
Target: 60 fps at 1080p on the Medium preset on the reference PC (Radeon RX 580, Core i7-8700, 16 GB).

| Date | Change | Commit | Resolution | Frame | fps | p95 | Game | Render | RHI | GPU | Draws | Triangles | Load (s) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2026-09-29 | Baseline before cleanup | 78a9c94 | 1920x1080 | 23.8 | 42 | 24.9 | 7.2 | 23.8 | 2.5 | 21.5 | 2125 | 1194674 | 5.08 |
