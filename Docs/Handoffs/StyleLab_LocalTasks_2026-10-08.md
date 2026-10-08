# For the local session: the tutorial island in style 3 (the art catalog is done)

Written by the cloud session on 2026-10-08 for a session on the user's PC (the cloud has no Unreal and can't upload Git
LFS files). The user's words: *"Have the local session catalog these in the art files, I may change art styles later
on. For me the best ones are 3,4,5, and 6. Have the local session start building tutorial island in style number 3,
but only tutorial island, I may change it if it doesn't look good in game."*

Start with `git pull origin main`. The ten styles are in `Tools/StyleLab/web/styles/` (one module each, `NN_<id>.js`,
helpers in `lib/`), with the art direction in `Docs/Art/StyleLab/Briefs.md`, the web research in
`Docs/Art/StyleLab/Research.md`, and the lab itself in `Tools/StyleLab/README.md`. The live lab is
<https://claude.ai/artifact/WhmxkTia9qvvhqGLWDSDbr> (shared by link): keys 1-9 and 0 are styles 1-10, the backtick is
today's look, `I` shows a style's card.

When a task is done, delete it from this file (and the file once both are), and keep `Docs/Handoff.md` current.

## Task B: the tutorial island in style 3, Painted Frontier, and only the tutorial island

Only `/Game/Maps/Lvl_TutorialIsland` changes. Ransom's Rest, the menu and every other level must look exactly as now,
and the change must be easy to undo: the user will judge it in game and may switch to 4, 5 or 6.

- **The look:** `Docs/Art/StyleLab/Catalog.md` section 3 has its numbers in Unreal terms. `03_painted.js` with `lib/painted_shaders.js` and `lib/painted_edges.js`, and the shared sky
  `lib/anime_sky.js`. The brief is "3. Painted Frontier" in `Briefs.md`, and `shots/sheet_03.jpg` shows the target.
  - Its GLSL hook paints the light in: warm, lifted tops; cool, darker undersides; a dark-to-light gradient up each
    object's height; caught convex edges; world-space hue blotches the size of a brush; brush strokes on the ground.
  - Textures read as softened, painted shapes (the lab blurs them by mip bias, with saturation about 1.35 and the
    contrast up), with high roughness and a warm rim.
  - A late-morning warm sun at about 40°, and blue-violet shadows that never go black.
  - Soft AO, gentle bloom and vibrance; no outlines; big painted clouds.
- **Suggested route** (adapt it per `Docs/Pipeline.md` and `CLAUDE.md`):
  - a "Painted" static switch in the masters (`Tools/Unreal/build_world_materials.py`), off by default;
  - island-only material instances that turn it on (the area `level.materials` / `level.swaps` mechanism, or the
    island build's equivalent);
  - the island's own light, sky and fog settings, and a post-process volume in that level only.
  
  Repainting the texture sets in `looter_textures.py` is the brief's full recipe, but start with materials, light, sky
  and post so the user sees it in game soon, then propose the repaint.
- **Budget:** 8.3 ms at 1080p on Medium (RX 580 class).
  - Measure the island with `Tools\perf.ps1` before and after, and tour all its views (`Tools\tour.ps1`).
  - Keep every `Looter.*` test green.
  - Commit each working step, and keep `Docs/Pipeline.md`, `Docs/Handoff.md` and `Docs/Plan.md` Phase 6 current.
- Show the user tour screenshots of the island and ask them to play it. `CLAUDE.md`'s rules apply: ask before any new
  tool, and after two failures ask the user.
