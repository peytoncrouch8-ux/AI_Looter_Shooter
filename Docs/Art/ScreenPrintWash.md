# Screen Print Wash: the chosen art style

The user chose this look on 2026-10-02 from the art-style exploration (`StyleExploration/README.md`, round 2, style 10).
It replaces textured stylized realism (`StyleTarget_Outpost.png`, approved 2026-09-30) for the whole game. **The
transition has not started**: nothing in `Content/`, the materials or the texture generator has changed yet. The plan
below is for the next session; until it runs, `Docs/TutorialIsland.md` and `CLAUDE.md` still describe the game as it is.

- Target image: `StyleTarget_ScreenPrintWash.png` (the village cottage, 1600x900).
- Its siblings and the parents it was mixed from: `StyleExploration/hybrids/contact_sheet_all_six.png`,
  `StyleExploration/01_ink_and_flat_color.png` (Sable) and `04_gouache_storybook.png`.
- The recipe that made it, exactly: `Tools/Blender/style_plugins/screen_print_wash.py` (a plugin of
  `Tools/Blender/style_compose.py`; the passes come from `Tools/Blender/style_render.py`).

## The look

A flat-color print on cream watercolor paper. In the words of the recipe (pixel sizes are at 1600 px wide; the game
scales them with the resolution):

1. **Flat fills from a small palette.** Every surface is one flat color; a whole scene uses about ten. They come from
   the textures' average colors, flattened (mean shift), snapped to the palette, saturated a little (×1.22) and lifted
   into the paper: `fill = 0.83 × color + 0.17 × cream`, cream being `#f6eedc`. Textures carry no detail of their own.
2. **Two tones of light, hard edge.** Lit, or shade. The shade is the fill times `(0.70, 0.64, 0.86)` (violet, like
   Sable), saturation ×1.15. The terminator sits at a little under half of full sunlight and is softened by less than a
   pixel, so the edge reads wet rather than vector-sharp. Cast shadows are the shade tone; on grass the shadow darkens
   a further 10 %. Ambient occlusion only dims corners by up to 12 %.
3. **Pigment granulation in the shade,** no stipple and no halftone: a soft mottle (±20 % at about 5 px) and sparse dark
   specks (−17 %), nothing in the lit tone.
4. **Gouache edges.** Fills darken 10 % along their own edges (pigment pooling), and the whole color plate bleeds
   softly (30 % of a 3 px blur).
5. **Two line plates.** A sketchy pencil line, warm dark grey `#4a3f44` at 68 %, offset 1.5 px right and 1 px up, lies
   under a thin clean ink line, `#2a2024` at 88 %, 1 px wide (the clean 2 px line eroded from one side). Lines follow
   silhouettes, creases and material boundaries.
6. **Misregistration.** The color plate lands 2.5 px right and 1.5 px down from the line plate, its silhouette blurred
   0.6 px, so color bleeds past the lines like a screen print.
7. **Cream paper over everything,** the sky too: fibers (3.5 %) and mottle (6 %), showing more in the light than in the
   dark; a touch of contrast (×1.05 around 0.55) and a fine grain (0.8 %).
8. **Sky and distance.** A flat pastel gradient: horizon `#f6dcb8`, `#e4c9d9` at 30 %, `#bcd4ee` at 65 %, `#9ec1ea` at
   the top, with flat paper-white (`#fefbf4`) clouds outlined in pencil (`#5a4c52` at 60 %). The ground fades toward
   `#f1dcc0` with distance (80 % by the far distance).
9. **The one saturated color.** Teal (the door, the trim) is kept saturated (×1.7, brightness ×1.12) and shades to deep
   teal (fill × `(0.60, 0.74, 0.80)`) instead of violet. Rule for the game: what the player should notice (doors,
   loot, rarity, interactables, the lantern) gets a saturated accent; everything else is the pastel palette.

### The cottage's palette

What the recipe produced for the target image (lit fill, shade fill), largest area first; roles are approximate. The
rule above, not this table, is what the texture generator should implement, so every set gets its own flat plate.

| Lit | Shade | Share | Where |
|---|---|---|---|
| `#68753e` | `#484b32` | 46 % | grass |
| `#867b69` | `#5d4d5a` | 16 % | shakes (roof) |
| `#756754` | `#524047` | 8 % | timber |
| `#675a49` | `#48373d` | 6 % | timber, darker faces |
| `#594e40` | `#3e3036` | 6 % | beam ends, under the eaves |
| `#948b79` | `#675768` | 6 % | stone, plaster in half light |
| `#43413d` | `#2e2834` | 4 % | window glass |
| `#a99e89` | `#766376` | 3 % | plaster |
| `#c0b49b` | `#867086` | 3 % | plaster, lit |
| `#507f71` | `#305e5a` | 2 % | teal door and trim (the accent) |

## Rendering the target again, or another model in this style

    python Tools/Blender/style_render.py Art/Models/Buildings/Cottage.py Saved/StyleExplore/Cottage
    python Tools/Blender/style_compose.py Saved/StyleExplore/Cottage Saved/StyleExplore/Cottage/print --no-builtin --plugins Tools/Blender/style_plugins/screen_print_wash.py

Both run in Blender's Python (`blender -b --factory-startup --python <script> -- <args>`) or with the `bpy` module from
PyPI; compose also needs `numpy`, `opencv-contrib-python-headless` and `pillow`. Rendering a gun, the spider or a tree
this way is the cheapest check of a material decision before touching the engine.

## Transition plan (next session)

The order shows the look early, keeps every step measurable, and leaves the game playable after each one. Measure on
Medium (`Tools/perf.ps1 -Label ... -Exec "Looter.Quality Medium"`, `Tools/tour.ps1 -Quality Medium`) before the first
step and after each one that can change cost; the budget stays 8.3 ms at the heaviest viewpoint.

- [ ] **0. Before pictures.** A tour on Medium for the "before" screenshots and timings (`Docs/Performance.md`).
- [ ] **1. Docs.** Rewrite the art bible in `Docs/TutorialIsland.md` for this look (palette rule, two-tone light, line
      rules, sky, the accent rule); the art-style bullet in `CLAUDE.md`; a Phase 6 in `Docs/Plan.md` with these boxes.
- [ ] **2. The post-process, applied from C++.** A post-process material `/Game/Art/PostProcess/M_PP_ScreenPrint`, built
      by a script like `Tools/Unreal/build_world_materials.py` (`build_screen_print_post.py`) so it can be rebuilt, with
      a parameter collection `MPC_ScreenPrint` for everything tunable. The game adds it as a blendable from C++ (the
      player's view, `Player/PlayerViewComponent`, or the controller), and `UGraphicsSettingsSubsystem` can scale or
      drop parts of it per preset. One material, before tonemapping:
      - the light term `SceneColor ÷ BaseColor` quantized to lit and shade, recomposed as `BaseColor × tone`, the shade
        multiplied by `(0.70, 0.64, 0.86)` (teal-tagged pixels shade to deep teal: see step 4);
      - ink (1 px) and pencil (jittered, offset) lines from depth and normal edges, the pencil's jitter from a tiling
        noise texture so it does not crawl every frame;
      - the color plate read 2.5 px right and 1.5 px down (scaled by width / 1600), lines not offset;
      - granulation in the shade from a world-space triplanar noise (world position from depth) so it sticks to
        surfaces instead of swimming;
      - the paper, screen-space and subtle;
      - fade to `#f1dcc0` by distance (or the height fog's color, step 6, so the sky dome matches).
      Budget about 0.8 ms at 1080p on the RX 580; verify with `perf.ps1 -GpuStats` and `perfdiff.ps1`.
- [ ] **3. Flat plates for every texture set.** `Tools/Blender/looter_textures.py --style print`: each set's base color
      becomes its flat fill (the house trim sheet: one flat color per strip, boards and stones separated only by a thin
      line in the pencil color; leaf atlases: flat leaf color, alpha kept), the normal map flat, ORM constant roughness
      with the baked occlusion kept (it becomes the pooling). Import them (the model importer's texture path,
      `Looter.ImportModels` on one model first: the cottage), and compare the tour view of the cottage with the target.
- [ ] **4. Masters.** `build_world_materials.py` gives `M_World`, `M_Gun`, `M_WorldFoliage` and `M_Terrain` a `Print`
      switch: no normal map, roughness 0.9, color variation off, moss as a second flat tone; an `Accent` flag (or the
      instance's tint) marks the teal surfaces the post-process shades to deep teal. `M_SkyClouds` gets the flat
      paper-white clouds (coverage, hard soft edge, pencil outline color); `M_Backdrop` and `M_Water` take the pastel
      horizon. Every instance keeps its parent, so placed actors do not change.
- [ ] **5. Foliage and terrain.** Grass and flowers as flat blades (grass `#68753e` lit, `#484b32` shade as the
      starting point; flowers keep saturated accents); foliage writes a custom stencil so the edge pass skips normal
      edges on leaf cards and keeps only depth edges (otherwise the lines turn to noise); the terrain's macro map
      quantized to a few flat tones, detail textures off.
- [ ] **6. Light.** The sun with hard shadows (shadow filtering sharpened), a neutral sky light (the two-tone ramp eats
      most of its gradient), exponential height fog colored `#f1dcc0`, the sky dome's gradient as above, placed by the
      island build (`build_tutorial_island.py`'s lighting step).
- [ ] **7. Guns, creatures, loot.** `M_Gun`'s wear becomes granulation and pooling; rarity glow and loot beams stay
      saturated accents; the spider and slime get flat plates (`SpiderBody`, the gel's tint); the first-person arms and
      gun are checked at the HUD's scale (lines at 1 px must stay unbroken on the gun).
- [ ] **8. UI.** No change (the `LooterUI` kit is its own thing); check the HUD's orange and cyan over the pastel world.
- [ ] **9. Retire the old look.** Delete the realism mode of the texture generator once every set has a print plate,
      move `StyleTarget_Outpost.png` into `StyleExploration/` as history, drop `Variation` and the moss gradient where
      nothing uses them; `Lvl_Skyreach` keeps its flat stylized materials as the reference level.
- [ ] **10. Measure and record.** The tour on Medium at every viewpoint, screenshots beside the target,
      `Docs/Performance.md` updated, all `Looter.*` tests passing.

Known risks to watch: lines swimming or aliasing at distance (fade line opacity with depth and keep a minimum
thickness), the pencil jitter crawling (anchor the noise), screen-space paper under motion (keep it subtle; every
game with this look accepts it), and foliage edges turning to noise without the stencil exclusion.
