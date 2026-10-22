# Release

Read only when preparing a Steam release (the user, 2026-10-14). Work to do before shipping, with what's known.

## Smoothness (frame pacing and stutter)

Averages meet the target (Medium 120+ fps on the RX 580 at every tour view, 2026-10-14), but:

- **Occasional slow frames:** tour "worst" frames on Medium of 9-14 ms in some views (PointOverValley 14.1, MainStreet
  13.7, ChapelOverTown, Farm ~9) and ~28-30 ms once at MineDrift and Aerial_Dusk. Find each cause with a CSV capture
  (`perf.ps1 -GpuStats`, look at the spike frames' stats as for Lumen in `Docs/Performance.md`): garbage collection
  (`gc.*` timing), level/texture streaming, shader compiles, game-thread spikes (spawns, AI, PCG), audio.
- **Shader / PSO stutter:** the first time a material or effect appears in a fresh install, the driver compiles its
  pipeline state and the frame hitches (Borderlands 3 was criticized for this). Before release: confirm UE5's PSO
  precaching is on, gather a bundled PSO cache from full playthroughs (`-logPSO`), and test a packaged Shipping build
  from a clean driver shader cache on a first run.
- **Measure the Shipping build, not the editor binary:** every timing so far is `UnrealEditor.exe -game`. Package
  (Pipeline/Workflow.md "Packaging a test build") and re-time Medium/High/Epic; then 1% lows, not only averages.
- **Frame pacing options:** a frame rate cap, VSync, fullscreen modes (borderless vs exclusive) checked for even pacing.

## Expectations to state on the store page

- Presets on the reference PC (RX 580, i7-8700, 1080p): Medium 120+ fps, High 51-70, Epic 22-28 (Lumen: for strong
  cards). Estimates for an RTX 4090: ~250-300 fps (1080p, CPU-limited with an i7-8700), Epic ~60 fps at 4K.
