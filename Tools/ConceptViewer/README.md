# Island concept viewer

A web page that shows layout concepts for the tutorial island (Skyreach) in the Screen Print Wash style
(`Docs/Art/ScreenPrintWash.md`). Concepts can be explored in 3D before anything is built in Unreal: fly over the island,
walk its roads, and compare all four concepts from the same viewpoint. It draws the game's own scripted models and the
island's real terrain. Trees, flowers, fences, livestock and other small things that have no model yet come from a
procedural kit in the page.

The published page is <https://claude.ai/artifact/NoY9cGCEDzz4yB1ZwPb49e>. It is private to its owner until shared. The
concepts themselves are described in `Docs/TutorialIslandConcepts.md`.

## Files

- `web_export.py`: exports the scripted models (`Art/Models`) and the island terrain (`Art/Levels/area_model.py`) as
  GLBs, with the print palette baked into an RGBA vertex color. The alpha is a shading flag: 1 for a normal fill, 0.5
  for an accent (teal shade), 0 for unshaded (glows, flower heads, webs). It also writes `heights.bin`, a 256 x 256
  float32 grid of the terrain the page draws, and `manifest.json`, which lists every model with its bounds and sockets.
- `export_all.sh`: runs the exporter for every model the viewer uses, then the terrain. Each model script runs in its
  own process, because a few kits leave state behind.
- `assemble.py`: builds the page from `web/` and the export, and packs every binary file as base64 `.json`. The
  artifact host serves JSON but not `.glb` or `.bin`.
- `web/ui.html`: the markup and styles, in the game's LooterUI look (dark glass, orange accent, cyan hairlines).
- `web/engine.js`: coordinates, the palette, the print material, the sky, the ink-line post-process, model loading and
  draping over the ground.
- `web/kit.js`: the procedural kit, and the helpers concepts use to place houses, fields, roads and creatures.
- `web/concepts.js`: the four layouts, with their text, labels and viewpoints.
- `web/app.js`: expands a layout into instances, scatters vegetation, and animates creatures, smoke, the windmill and
  birds.
- `web/ui.js`: the camera from above and on foot, views and flights, the tour, compare, labels, the minimap and boot.
- `test/shots.js`: screenshots of views. `test/interact.js` drives the interface like a person would and reports
  console errors.

## Rebuild

```
pip install "bpy==4.5.*" numpy pillow opencv-contrib-python-headless   # once
Tools/ConceptViewer/export_all.sh                                       # writes Saved/ConceptViewer/web
python3 Tools/ConceptViewer/assemble.py
cd Tools/ConceptViewer/test && npm install
node shots.js ../../../Saved/ConceptViewer/web /tmp/shots 0,1,2,3 all 1280x720 clean labels
node interact.js ../../../Saved/ConceptViewer/web /tmp/interact
```

The exporter runs under plain Python with the `bpy` module, not inside Blender. Publish
`Saved/ConceptViewer/web/index.html` together with the files listed in `Saved/ConceptViewer/publish_files.json`.

The tests serve the page and three.js from disk, so they need no network. Set `CHROMIUM_PATH` to use a browser other
than Playwright's own.

## Conventions

- Layouts are written in Unreal centimeters (X north, Y east, yaw 0 = +X). The page converts to three.js meters with
  x = -Y/100, y = Z/100, z = X/100. Models face +z, which is Unreal forward.
- Long pieces (fences, hedges, barricades) run along their local +x, like the game's `FenceRail`, so a piece placed
  along a line takes the line's yaw + 90.
- Ink lines come from the second difference of inverse depth, which is zero on any plane, so flat ground never draws
  false lines. Small, many things (grass, flowers, cobbles) stay out of the normal pass, so they get no crease lines
  from afar.
