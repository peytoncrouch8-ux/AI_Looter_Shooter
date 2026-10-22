# The art style catalog (the Style Lab's ten styles)

Written 2026-10-08 so a later switch of art style is quick. The user may change styles later on. Each style here is a
complete art direction (light, sky, fog, materials, post, effects), previewed in Ransom's Rest under the in-game HUD.
The numbers come from the style modules in `Tools/StyleLab/web/styles/` (`NN_<id>.js`, helpers in `lib/`), read at
HEAD `3f3b5dd`. The art direction is in `Briefs.md`, the research in `Research.md` (same folder), and the lab itself in
`Tools/StyleLab/README.md`. The live lab (private to the user): <https://claude.ai/artifact/WhmxkTia9qvvhqGLWDSDbr>,
keys 1-9 and 0 are styles 1-10, the backtick is today's look, `I` shows a style's card.

## The user's verdict (2026-10-08)

"For me the best ones are 3, 4, 5, and 6." The favourites are 3 Painted Frontier, 4 Inkslinger, 5 Teropa Pulp and
6 Sunbleached. Style 3 is being built on **the tutorial island only** (`Lvl_TutorialIsland`), reversible: the user will
judge it in game and may switch to 4, 5 or 6. Every other level stays in today's look (style 0). The other six
(1, 2, 7, 8, 9, 10) were not picked; they stay on file in case the look changes.

## Index

| # | Style | Family | Verdict | Sun (from, up) | Cost |
|---|---|---|---|---|---|
| 0 | [Today](#0-today-the-baseline) | Current build | baseline | WSW 247.5°, 15° | n/a |
| 1 | [Clay Frontier](#1-clay-frontier) | Handmade | | SW 215°, 38° | ~0.7 ms |
| 2 | [Skyward Anime](#2-skyward-anime) | Anime cel | | WSW 258°, 56° | ~0.6 ms |
| 3 | [Painted Frontier](#3-painted-frontier) | Hand-painted | **favourite**, on the tutorial island | SSE 165°, 42° | ~0.3 ms |
| 4 | [Inkslinger](#4-inkslinger) | Graphic novel | **favourite** | SW 222°, 34° | ~0.8 ms |
| 5 | [Teropa Pulp](#5-teropa-pulp) | Retro sci-fi | **favourite** | WSW 258°, 18° | ~0.25 ms |
| 6 | [Sunbleached](#6-sunbleached) | Luminous | **favourite** | ENE 64°, 19° | ~0.4 ms |
| 7 | [Neon Frontier](#7-neon-frontier) | Neon dusk | | E 90°, 6° | ~0.8 ms |
| 8 | [Ember Gothic](#8-ember-gothic) | Dark weird west | | NW 306°, 22° (moon) | ~1.2 ms |
| 9 | [Celluloid West](#9-celluloid-west) | Film stock | | SW 214°, 36° | ~0.4 ms |
| 10 | [Golden Hour](#10-golden-hour) | Cinematic realism | | E 80°, 12.5° | ~1.0 ms |

Costs are the style authors' own estimates for the RX 580 on Medium (not measured). Measure with `Tools\perf.ps1`
before choosing. Cheapest to dearest: 5 (0.25), 3 (0.3), 6 and 9 (0.4), 2 (0.6), 1 (0.7), 4 and 7 (0.8), 10 (1.0),
8 (1.2).

All ten, side by side on Main Street (1-2, 3-4, 5-6, 7-8, 9-10 in rows):

![Ten styles on Main Street](Sheets/ten_styles_street.jpg)

## How to read the numbers

- **Sun azimuth** is the compass direction the light comes **from**: 0 north (UE +X), 90 east (UE +Y), 180 south,
  270 west. Looking east down Main Street (the `street` shot), a sun at 90 is dead ahead, 180 is on your right, 270
  behind you. **Elevation** is degrees above the horizon. In Unreal the DirectionalLight points the other way:
  actor **yaw = azimuth + 180 (mod 360), pitch = -elevation** (the same rule `build_area_environment.py` uses for
  today's sun). Each style lists both.
- **Sun colour and intensity** are in the lab's units. Where the style's `unreal.recipe` names an Unreal value (lux,
  Kelvin), it is quoted as "recipe". Where the recipe and the code disagree (they do, by a few degrees), the code is
  what the contact sheets show, so the code is quoted first and the difference is noted.
- **Sky and fog colours** are sRGB as they look on screen at the style's exposure and tone mapping: the lab inverts the
  tone curve to feed the renderer (`tonecurve.js`, `inverseToneMap`). Use them as targets for what the screen shows.
- **Tone mapping:** `aces` is a fitted ACES filmic curve (Unreal's default look). `neutral` is a Khronos-PBR-Neutral-style
  curve (a soft shoulder from 0.76 that keeps hue; no stock Unreal equivalent: approximate with the filmic sliders or
  a LUT). `none` means no curve at all (style 9 uses its own film curve in post). Exposure multiplies before the curve.
- **Fog** is height fog: opacity along level ground = `1 - exp(-density * (distance - start))`, capped at `maxOpacity`,
  with the density falling off by `exp(-heightFalloff * height)` per metre. The "fog at 50 / 100 / 300 m" numbers
  below are computed from that for a camera near the ground. The lab's units are per metre; Unreal's Exponential Height
  Fog uses different numbers, which a recipe gives where its author chose them. The fog's colour also tints the sky
  horizon (`horizonFog`).
- **Post order:** scene-referred passes first (ao, bloom, halation, godrays, dof, and custom passes marked `hdr`), then
  tone mapping, then the display-referred ones (grade, outline, grain, vignette, ...). Passes below are listed in the
  module's order. `ambient` is a hemisphere light (sky colour above, ground colour below) and is Unreal's sky light.
  `lights.scale` multiplies the scene's house lamps.
- **Computed values** are explained where they appear (the sun's glow colour in the toon, ink and pulp skies is the
  sun's colour x intensity x 0.3; a disc "N° across" is twice the radius the shader uses).
- **Costs** (`costMs`) are the authors' estimates, never measured on the reference PC.

## How to read a contact sheet

Each sheet is 1920x1620, six shots in two columns: row 1 `street` (looking east on Main Street) and `fight` (a spider
at close range, the gun firing, the HUD hit state); row 2 `chapel` (the chapel yard gate, looking south-east over
town) and `boothill` (among the graves, the chapel on its knoll); row 3 `sink` (the pit floor, toward the den) and
`aerial` (40 m up over the town). The street sheet above has all ten styles at the `street` shot. If an image does not
load, the same files are the lab artifact's `shots/sheet_NN.jpg` and `shots/ten_styles_street.jpg`. Some aerial shots
show faint horizontal bands on far ground. The lab's GTAO read the depth buffer's steps (`Docs/Pipeline.md`, lessons,
lists the fix). If bands still show, that is the lab, not the style.

---

## 0. Today (the baseline)

![Style 0 contact sheet](Sheets/sheet_00.jpg)

Family "Current build". The game as it ships: the texture sets through plain PBR on the World, Gun and Terrain masters,
under Ransom's Rest's golden afternoon. Every other style is judged against this ("would it stop someone scrolling
Steam?"). Its weakness: generic mid-tones, little shape language, nothing a screenshot is remembered by.
Module: `00_today.js`. Today's values in the game come from `Art/Levels/RansomsRest/layout.json` (`level.environment`)
and `Tools/Unreal/build_area_environment.py`.

- **Sun:** from 247.5° (WSW), 15° up; colour #ffc58f; intensity 4.2 (the build: 6.5 lux, 4100 K); softness 1.2;
  shadow strength 1.0. Unreal actor yaw 67.5, pitch -15.
- **Ambient:** sky #8aa6cf, ground #86704e, 1.9.
- **Exposure / tone:** 1.2, `aces`.
- **Sky:** zenith #4d7ec2, horizon #c9d2da, below-horizon #8a7a5e; sun disc 1, glow 1, horizonFog 0.7; clouds amount
  0.28, colour #fff3e2, shade #9ea7b8, scale 1.0, sharpness 0.35, speed 0.004; no stars or bodies.
- **Fog:** #a9b2bb, density 0.0005, falloff 0.015, start 60 m, sun scatter 0.85 in #f2c28a (exponent 8 default). Fog at
  50 / 100 / 300 m: 0 / 2 / 11 %.
- **Materials:** `pbr` with the sets as they are; terrain is the game's M_Terrain blend, defaults.
- **Post:** ao radius 1.2 intensity 0.5; bloom threshold 1.6 strength 0.25 radius 0.7; grade contrast 1.04, saturation
  1.04, temperature 0.04. (The game has no SSAO on Medium: baked vertex occlusion stands in.)
- **Effects:** muzzle #ffd27a, tracer #ffe6a8, impact #ffcf7a, blood #9be35a, hit flash #ffffff. Dust 0.45, motes
  #ffe2b0, no embers; wind, smoke at the lab's defaults.
- **HUD:** designed over this look; the mid-tone ground gives the orange little to push against.

---

## 1. Clay Frontier

![Style 1 contact sheet](Sheets/sheet_01.jpg)

Family **Handmade** (research J). Module `01_clay.js`; helpers `lib/clay_shaders.js`, `clay_flash.js`, `anime_sky.js`,
`painted_edges.js`, `painted_shaders.js` (shared with 2 and 3). Verdict: not picked.

- **The idea:** the whole frontier as a stop-motion film set. Plasticine buildings with thumb-worked strokes and a waxy
  sheen, felt hills, a painted backdrop with cotton clouds, spiders that move in 12 fps steps under a macro lens.
- **Why it wows:** nobody else's shooter looks like this. The macro depth of field turns every view into a tabletop
  miniature, and a clay revenant in a clay ghost town is the kind of weird Teropa wants.
- **Under the HUD:** the glossy gunmetal HUD sits over a matte, pastel, softly blurred world like a camera overlay;
  its orange and cyan read as bright plastic accents.
- **Risks:** every asset needs a clay pass (palette plate per material, sculpted normals) and the photo textures must
  not leak through. The depth of field softens enemies past ~30 m (keep the near 25 m sharp). 12 fps creatures must
  keep smooth hitboxes (step only the pose).

**The look in Unreal terms**

- **Sun:** from 215° (SW), 38° up; #ffe6c8; intensity 4.3; softness 5.0 (very soft, wide shadows); strength 0.92.
  Actor yaw 35, pitch -38. Recipe: 36° from the south, 3.6 lux, light source angle ~4°.
- **Ambient:** sky #a6c0f2, ground #d6a26c, 1.25 (a cool soft-box fill). Lamps x1.2. Recipe sky light 2.1 from the
  backdrop cubemap.
- **Exposure / tone:** 1.0, `neutral`.
- **Sky** (own dome `makeToonSky`, felt look): gradient zenith #6fa8e2, mid #a9cdef at 30 % height (power 0.7),
  horizon #f2e2c4, below #c9b28c, haze 0.32. Sun disc 3.2° across, soft, strength 2.2, glow 0.35, no glare. Eight cloud
  towers (height 1.1, width 1.2) plus floating puffs (amount 0.55, size 0.13, from 9° to 50° up); cloud colours lit
  #fffbf2, shade #d8d6e8, deep #c3bfd6, rim #fff4e0; felt fibres 0.10, backdrop shadow 0.10, edge soft 0.06. Env sky
  (reflections): zenith #94bde3, horizon #f2e4c8, no clouds.
- **Fog:** #d6e0ee, density 0.0009, falloff 0.016, start 40 m, sun scatter 0.3 in #ffe6c4, max opacity 0.7. Fog at
  50 / 100 / 300 m: 1 / 5 / 21 %.
- **Materials:** `toon` with a soft wrap ramp (`[-1,0] [-0.3,0.1] [0.12,0.52] [0.55,0.9] [0.95,1.0]`, softness 0.22,
  wrap 0.45), shade tinted #7f8fc0 at 14 %, sky reflection 0.15, occlusion x1.35. Albedo is **flat clay colour** from
  a plate table (about 50 named plates, e.g. WoodPlanks #d6966a, WoodTeal #4f9e98, StoneWall #c8ab92, Hay #e6bd62);
  anything not listed goes through `clayColor` (chroma up, darks lifted so nothing is black, hue nudged warm, a
  lightness band per role). Trim sheets and palettes keep their regions at mip 3.5, saturation 1.3, gradient-mapped
  through four clay stops (#4a3a46 #b47466 #e4b88c #f7ecd6). The set's normal map is read at mip bias 4.5 (sculpted
  grooves only; guns 2) at strength 1.7 (guns 0.6), plus a procedural thumb relief: tool strokes plus oval dents. Its
  size scales per group: walls and props x5 (dents in 40 cm cells, fading out between 25 and 125 m), foliage x3,
  creatures x2, the gun x0.4. Waxy two-lobe sheen, a red-orange glow in the terminator and penumbra
  (light through clay), a slow hue and chroma drift across the world so no two walls are one batch. Creatures:
  chocolate #3b2a2a (Pale #8c7f8a), rim #ffd6a8, glossy. Leaves and webs keep cut-outs at mip 1.6, contrast 0.7,
  tinted to their plate.
- **Terrain:** toon, macro strength 1, detail 0.18, normals 0.25, blur 3. Recolour: roads #d9c4a4, felt grass #7f9f58,
  slopes #d0a486 (keep 0.5 of the macro's patches). Felt hook: fibres (fade 10-70 m), velvet sheen at grazing angles,
  pressed strata on slopes.
- **Post:** ao radius 1.1, intensity 1.1, power 1.5, colour #3a2a3a; edge paint (screen-space curvature: convex edges
  caught light, concave creases dark; world width 10, 1.2-4.5 px, convex 0.6, concave 0.4, fades by 90 m); **macro lens**
  (autofocus depth of field on the median depth under the crosshair, clamped 3-260 m; blur 5.5 px, plateau 1.0
  octave, ramp 2.4, near 1.0, gun 1.3); bloom 1.1 / 0.32 / 0.85; grade contrast 1.1, saturation 1.12, vibrance 0.25,
  lift #0d0a12, gain #fffaf2, curve 0.12; vignette 0.22 / 0.6; grain 0.022, size 1.6, coloured 0.15.
- **Effects:** muzzle #ffb347, tracer #fff1c8, impact #ffc85e, blood #7fd34e, hit #fff4e2. The muzzle flash is a
  cotton-wool burst of eight puffs round a hot core. Dust 0.22. Wind 0.05. Smoke 0.5. **`animStep` 1/12**
  (creatures, effects, wind and smoke move in 12 fps steps).
- **Unreal recipe (condensed):**
  - `M_Clay`: base colour from a per-material clay plate, trim sheets gradient-mapped through a 4-stop clay ramp by
    mip-4 lightness, times a world-space hue/chroma/value noise. No photo albedo.
  - Normal: the set's normal at mip 2 plus a tiling world-aligned thumb-smear normal, faded past 25 m.
  - Shading: Default Lit, roughness 0.5, specular 0.35, a Subsurface-Profile-lite red-orange term in the terminator
    (wrap ~0.45).
  - Terrain: grass layer swapped for felt; dirt and rock smoothed to clay with strata.
  - Light: key at 36° from the south, 3.6 lux; sky light cubemap of the backdrop at 2.1; wide light source angle.
  - Sky: an unlit sphere with a painted backdrop texture, no SkyAtmosphere.
  - Post: Gaussian DOF on a line-trace focus (~1 octave sharp), bloom 0.3, vignette, grain 0.02; 12 fps pose step on
    creatures and effects only (UpdateRateOptimizations or a custom step).
- **Cost:** ~0.7 ms (DOF 0.4, smear normal 0.1, GTAO already on). The brief said 0.6.
- **Refs:**
  - Harold Halibut: the making of (Creative Bloq) <https://www.creativebloq.com/features/making-harold-halibut>:
    a game made by hand and scanned.
  - Harold Halibut interview (Inverse) <https://inverse.com/gaming/harold-halibut-interview>: stop-motion feel from
    edited motion.
  - Slow Bros on tactile gaming (Skwigly) <https://skwigly.co.uk/harold-halibut-slow-bros-ole-tillmann-tactile-gaming>.
  - Miniature faking (Wikipedia) <https://en.wikipedia.org/wiki/Miniature_faking>: shallow focus plus bright paint
    reads as a model set.

---

## 2. Skyward Anime

![Style 2 contact sheet](Sheets/sheet_02.jpg)

Family **Anime cel** (research F). Module `02_anime.js`; helpers `lib/anime_sky.js`, `anime_shaders.js`,
`painted_shaders.js`. Verdict: not picked.

- **The idea:** a sun-drenched anime feature on the frontier. Crisp two-tone cel light with lavender shade, a deep blue
  sky with towering sculpted cumulus, clean coloured line work, sparkling highlights, lush gradient grass.
- **Why it wows:** the look with the biggest audience on Steam, played straight with a western's heat. Dust-gold
  streets, sun-bleached planks, spiders cut out as dark shapes against the light.
- **Under the HUD:** anime UI lives over bright pastel worlds; the gunmetal HUD adds the weight that keeps it a
  western, and its cyan and orange read like the highlights in the sky and on the dust.
- **Risks:** spiders and props need anime shape language (bigger shapes, painted shadow masks) to match; the realistic
  models are the weak link. Outlines shimmer on foliage cards at range. The look is tied to gacha games in people's
  minds.

**The look in Unreal terms**

- **Sun:** from 258° (WSW), 56° up; #fff3dc; intensity 4.4; softness 0.6 (crisp); strength 1.0. Actor yaw 78, pitch
  -56. Recipe: 52°, 4.4 lux; the brief said ~50°.
- **Ambient:** sky #b4c6fa, ground #e0c49a, 1.85. Recipe sky light 1.75.
- **Exposure / tone:** 1.0, `neutral`.
- **Sky** (`makeToonSky`, crisp cel clouds): zenith #1b58d4, mid #3f8ff0 at 28 % height (power 0.62), horizon #c4ecf7,
  below #9a8e7a, haze 0.42. Sun disc 1.4° across with strength 26, glow 1.1 and a four-spoke glare (2.5). Nine cloud
  towers (height 1.5) plus floating puffs (amount 0.36, from 14° to 55°); cel edge 0.1, softness 0.016, rim 0.5;
  clouds lit #ffffff, shade #b3b4ea, deep #9393d6.
- **Fog:** #a9cdef, density 0.0022, falloff 0.012, start 25 m, sun scatter 0.25 in #e8f4ff, max opacity 0.92 (strong
  aerial perspective: far ridges go pale blue). Fog at 50 / 100 / 300 m: 5 / 15 / 45 %. Recipe: Unreal height fog density
  0.03, falloff 0.12.
- **Materials:** `toon`, **two tones** (`[-1,0.2] [0.02,1.0]`: the shade keeps a fifth of the sun), softness 0.018,
  shade tinted #8a84dc at 22 %. Textures at mip 3, saturation 1.32, value 1.08, contrast 0.92, normals 0.35,
  occlusion 0.8, sky reflection 0.25. An `AIRY` hook adds a sky-coloured bounce in every shade (0.34, 0.36, 0.60 x 0.5).
  Grass and leaves: root-to-tip gradient (root 0.48/0.62/0.58 to tip 1.16/1.12/0.72), shade tinted #3f6aa8 at 38 %,
  translucency 0.9. Wood value 1.35 with a #c98f5a tint. Metal and glass: hard specular (size 0.07, strength 2.2;
  glass 0.1 / 3), warm rim #fff0d8. Creatures: flat #2c2140 (Pale #c9bfd8), rim #ffd9a0 at 0.9. Far backdrop: pale
  blue #9cc2e8 at value 0.62.
- **Terrain:** toon, shade #8a84dc at 28 %, detail 0.3, normals 0.12, blur 3. Recolour: street #dcbb80, grass #78a454,
  cliffs #ddb68c (keep 0.25). Lit grass goes yellow-green; the ground's light and dark are laid in as flat cel patches.
- **Post:** ao radius 0.9, intensity 0.45, colour #2a2a6a; bloom 1.25 / 0.42 / 0.85; godrays strength 0.22, decay
  0.955, 40 samples, #fff6dc; **outline** width 1.15 px, colour #2a2440 taken 85 % from the scene's darkened colour,
  depth 1, normals 0.75, inner lines on, fades 25 to 170 m, opacity 0.85; grade contrast 1.05, saturation 1.08,
  vibrance 0.22, gain #fffdf6, curve 0.08; fxaa 0.4.
- **Effects:** muzzle #fff2a8 (star flash), tracer #ffffff, impact #fffbe0, blood #c6ff6a, hit #ffffff. Dust 0.12.
  Wind 0.14, speed 1.5. Smoke 0.35.
- **Unreal recipe (condensed):**
  - Global cel light in a post-process material before tonemapping: diffuse light = SceneColor / BaseColor, stepped by
    a 2-stop curve (edge 0.02, width 0.02), shade multiplied toward lavender #6f6fd0 at 40 %.
  - Textures at mip 3 (LOD bias in the instances), saturation 1.3; grass and leaves with a vertex-colour gradient;
    terrain with a low-frequency painted wash.
  - Specular: stepped GGX (smoothstep of N.H^4) on metal and glass; warm Fresnel rim on creatures.
  - Outlines: one post material, Sobel on depth and normals (1.2 px), colour = SceneColor x 0.25, fading 40-250 m.
  - Sky: unlit sphere with vertex gradient and sphere-sprite cumulus (or a painted panorama); height fog density 0.03,
    falloff 0.12, pale blue.
  - Sun 52°, 4.4 lux, sky light 1.75; bloom 0.35; light shafts for the glare.
- **Cost:** ~0.6 ms (post cel + outline 0.5, materials ~0).
- **Refs:**
  - HoyoToon, Genshin shader notes <https://github.com/Melioli/HoyoToon/wiki/Using-the-Genshin-Shader>
  - URP simple Genshin shaders <https://github.com/NoiRC256/URPSimpleGenshinShaders>
  - Breath of the Wild's look (Zelda Universe) <https://zeldauniverse.net/2017/03/14/takizawa-explains-how-the-wind-waker-hd-inspired-the-visual-style-for-breath-of-the-wild/>
  - Wuthering Waves on Unreal (Epic) <https://www.unrealengine.com/developer-interviews/exploring-the-post-apocalyptic-charm-of-asg-open-worlds-in-wuthering-waves>
  - Global toon shading in UE5 via post process <https://medium.com/@little_michael101/building-a-toon-shader-in-unreal-engine-5-globally-not-per-mesh-part-1-6ef9aade3380>

---

## 3. Painted Frontier

![Style 3 contact sheet](Sheets/sheet_03.jpg)

Family **Hand-painted** (research D). Module `03_painted.js`; helpers `lib/painted_shaders.js`, `painted_edges.js`,
`anime_sky.js`, `clay_flash.js`. **Verdict: the user's favourite (one of 3, 4, 5, 6). Being built on the tutorial
island only (`Lvl_TutorialIsland`), reversible.**

- **The idea:** chunky, sunny, hand-painted adventure. Every surface looks brushed by hand with the light already
  painted in: warm tops, cool undersides, dark feet, caught edges. Bold sun-baked reds, ochres and teal under a rich
  blue sky of big soft painted clouds. Shade goes blue-violet, never grey.
- **Why it wows:** the friendliest and most readable option (the Sea of Thieves / Overwatch family) that still looks
  expensive. Clear colour blocks, a postcard shot at the chapel and boot hill, and almost free to run (~0.3 ms).
- **Under the HUD:** clean colour blocks keep every corner calm for the floating text, and the gunmetal HUD reads as
  the premium metal layer over a painted world.
- **Risks:** "I have seen this" (the Fortnite / Overwatch lineage): the frontier palette, the spiders and the
  weird-west props must carry the identity. Textures that are only blurred read as a mobile game: the value bands
  and blotches want the repainted sets. A cartoon tone can fight the haunted story unless the shapes lean spooky in
  the dark areas.

**The look in Unreal terms**

- **Sun:** from 165° (SSE), 42° up; #ffe0ae; intensity 4.3; softness 1.6; strength 0.95. Actor yaw 345, pitch -42.
  Recipe: 40° (the brief said ~40°), 4.3 lux, sky light 1.6. Looking east down Main Street the sun is on your right.
- **Ambient:** sky #8fa8ee (blue-violet), ground #c89060, 2.05. Lamps x1.1.
- **Exposure / tone:** 1.06, `neutral`.
- **Sky** (`makeToonSky`, soft brushed clouds): zenith #2f74cc, mid #62a6dc at 30 % height (power 0.7), horizon
  #d2e8e2, below #a08460, haze 0.45. Sun disc 1.6° across, strength 18, glow 0.9, no glare. Eight cloud towers (height
  1.1, width 1.1) plus floating puffs (amount 0.48, size 0.1, from 13° to 52°); cel edge 0 with softness 0.22, wrap
  0.4, rim 0.3, top light 0.35; clouds lit #fff4e2, shade #b8b0d6, deep #a08cb8, warm rim #ffe6c0; brush strokes
  0.16, edge warp 0.012, sun bleed 0.35. Env sky (reflections): zenith #2a6fcf, horizon #b7dcf0.
- **Fog:** #bcd0e6, density 0.0018, falloff 0.014, start 35 m, sun scatter 0.45 in #ffe2b8, max opacity 0.85. Fog at
  50 / 100 / 300 m: 3 / 11 / 38 %.
- **Materials:** `pbr` with the sets softened to painted shapes. Texture mip bias **1.6** (the brief said ~2.5; cut-outs
  0.6, guns 1.0), saturation by role (wood 1.25, rock 1.3, stone 1.2, metal 1.25, foliage 1.4, grass 1.35, bark 1.3,
  anything else 1.3, HouseTrim 1.6), contrast 1.1, value 1.2 (wood 1.75, rock 1.3), normal map read at mip bias 1.5,
  roughness x1.25 (metal 0.85, guns 0.9), sky reflection 0.35 (metal 0.7), occlusion 1.1, warm rim #ffd9a8 (power 3,
  strength 0.4). Wood tinted #b8783e at 30 %, rock #cf7a4c at 35 %. Creatures: flat wine-black #3a1f24 (Pale #b6a8a0),
  rim #ffc58a at 0.75, roughness 0.55. Water #2f9fae. Far backdrop tinted #8aa8d8.
- **The painted light** (`paintHook`; it divides the lighting out of the colour, repaints the albedo, puts the light
  back):
  - **Value bands:** the albedo's brightness is regrouped into painted steps, one per stop (a factor of two), edges
    soft and wandering with a brush noise; 70 % applied.
  - **Brush blotches:** world-space noise at about 0.7 m scale shifts value (+-13 %), hue (+-0.0175 of the colour wheel)
    and saturation (+-15 %).
  - **Brush strokes:** dabs in three tones, 0.45 m long and 7 cm wide, +-10 % value, warm or cool shifts, fading out
    from 18 to 45 m.
  - **Painted light:** up-facing surfaces x(1.16, 1.06, 0.88) up to 55 %; down-facing x(0.62, 0.66, 0.92) up to 80 %.
  - **Painted bevels:** the normal map's detail catches light on upper edges and goes dark on lower lips (clamped
    -45 % to +60 %).
  - **Dark feet, light heads:** a gradient from 0.74 at the ground to 1.06 at 3.2 m above the terrain's height, read
    from a 512x512 half-float texture of the terrain heights (foliage 0.7 over 5 m, creatures 0.8 over 1.2 m).
  - **Violet shade:** where the sun does not reach, colour x(0.80, 0.74, 1.28) at 55 %, plus an albedo x(0.30, 0.30,
    0.52) fill x0.55 x occlusion.
  - Foliage: 3 bands, blotch 0.32, warm top (1.22, 1.14, 0.78), cool underside (0.55, 0.65, 0.95), hue +4. Cut-outs
    (leaves, webs, posters): no bands, blotch 0.3. Guns: 3 bands, blotch 0.12, no height gradient.
- **Terrain:** macro strength 1, detail 0.35, normals 0.35, steep 0.5, blur 2, steep tint #d49a6c. Recolour: sun-baked
  dirt #d9b98c, olive-teal grass #7fa848, red sandstone #cf7d50 (keep 0.6 of the macro's patches). Ground hook: big
  brushed patches (~6 m, +-15 %, a finer layer +-8 % fading from 30 to 120 m), hue wander +-0.025, strokes 1.1 m x
  16 cm (+-10 %, fade by 90 m), lit ground warm (1.07, 1.0, 0.9), shade violet (0.82, 0.82, 1.2).
- **Post:** ao radius 1.6, intensity 0.85, power 1.5, **violet** colour #4a3070; **edge paint** (scene-referred,
  world width 6, 1-3 px, convex 1.0, concave 0.45, lit bias 0.25, edge tint #ffe9c4, crease tint #3c2a5c, fades by
  110 m); bloom 1.3 / 0.3 / 0.8; grade contrast 1.07, saturation 1.14, vibrance 0.3, temperature 0.03, lift #0c0816,
  gain #fffaf0, curve 0.1; vignette 0.18 / 0.65, colour #1c1428. No outlines, no grain.
- **Effects:** muzzle #ff9a2e, tracer #fff0b0, impact #ffb648, blood #a6f04a, hit #fff2d6. The muzzle flash is a fat
  four-lobed painted burst with a cream core. Dust 0.3, motes #ffe8c0. Wind 0.13. Smoke 0.4.
- **Unreal recipe (condensed):**
  - Repaint the texture sets in the generator (`looter_textures.py`): bigger shapes, 3-4 value groups per material,
    baked top light and bevel highlights, no photographic noise. (The lab only blurs them; the repaint is the full
    recipe.)
  - `M_World` adds per pixel: warm/cool split by world normal, painted bevels from the normal map's up-facing
    detail, a gradient by height above the landscape (a Runtime Virtual Texture of the terrain height, or the object's
    bounds), a tiling world-aligned brush mask (3 tones) and a low-frequency hue/value blotch texture.
  - Shading: Default Lit, roughness 0.85, specular 0.2, warm Fresnel rim; shadow colour pushed blue-violet with the
    sky light tint #8fa8ee and a post shadow tint from the lighting / base-colour ratio.
  - Edges: bake curvature into vertex colour or a mask and brighten convex edges in the material (cheaper and
    steadier than the lab's screen-space pass).
  - Light: sun 40°, 4.3 lux warm; sky light 1.6; GTAO with a violet tint (post-process AO colour).
  - Sky: a painted panorama sky sphere with soft brushed cumulus; light-blue height fog.
- **Cost:** ~0.3 ms (material maths only; curvature baked).
- **Refs:**
  - GDC18: Visual adventures on Sea of Thieves (80.lv) <https://80.lv/articles/gdc18-visual-adventures-on-sea-of-thieves/>
  - Stylized graphics: Fortnite and Sea of Thieves (Habrador) <https://blog.habrador.com/2018/08/stylized-graphics-fortnite-sea-of-thieves.html>
  - Designing Overwatch (Cook and Becker) <https://cookandbecker.com/en/article/378/designing-overwatch.html>
  - Illustrative rendering in Team Fortress 2 (Valve, GDC 2008) <https://cdn.steamstatic.com/apps/valve/2008/GDC2008_StylizationWithAPurpose_TF2.pdf>
  - Valorant's art style (Inverse) <https://inverse.com/gaming/valorant-art-style-interview-moby-francke>
- **In the sheet:** pale violet-and-cream street, deep blue sky, big flat-lit clouds, a saturated green and red
  landscape at the chapel and the aerial. The spiders are wine red-black with a warm rim and read clearly.

---

## 4. Inkslinger

![Style 4 contact sheet](Sheets/sheet_04.jpg)

Family **Graphic novel** (research E). Module `04_ink.js`; helpers `lib/ink_glsl.js`, `lib/ink_sky.js`.
**Verdict: the user's favourite (one of 3, 4, 5, 6).**

- **The idea:** a living graphic novel. Heavy hand-inked contours that swell and thin, hatching that appears only in the
  shade and crawls into the cracks (drawn in world space, so it sits on the walls, not on the screen), the texture
  sets kept as the colourist's paint in rust, ochre and turquoise.
- **Why it wows:** an unmistakable thumbnail. A hard afternoon sun, a turquoise sky of hatched cumulus, a rust core
  where the light turns and cool plum in the deep shade. Outlines make spiders and loot pop. It is a weird-west comic
  you play, and the lines are ours (the hatching follows the light, not a texture).
- **Under the HUD:** the HUD's outlined white text and gunmetal frames already speak the language of ink. World lines
  stay at most ~3 px near and thin with distance, so the HUD's strokes remain the heaviest, and the calm turquoise sky
  keeps the top corners quiet.
- **Risks:** closeness to Borderlands: keep the world-space hatching by light, the rust / ochre / turquoise palette
  and the swelling line weight; never ink the albedo flat. Line crawl on grass and far geometry (crease lines fade by
  100 m, hatching by 170 m). Band exceptions needed for glows, glass and foliage. Hatch density must be checked at 4K
  and at 720p (line width needs a floor). In the sheets the gun's pale polymer shows as bold black-and-white patches:
  check that it reads as intended.

**The look in Unreal terms**

- **Sun:** from 222° (SW), 34° up; #fff0da; intensity 3.0; softness 0.5 (hard); strength 1.0. Actor yaw 42, pitch -34.
  The recipe gives no sun numbers (the brief says a strong afternoon sun, high contrast).
- **Ambient:** sky #6a9aa8, ground #8a5a3a, **0.55** (low on purpose: the ink does the darkening). Lamps x1.1.
- **Exposure / tone:** 1.0, `aces`.
- **Sky** (own dome `inkSky`): four-stop gradient: horizon ochre #e8a85a, low #dcd8b0 (up to 3.5 % height), mid
  turquoise #3d9c9a (3-17 %), zenith teal #13586a (20-75 %), below #5a3424, haze 0.6. Cloud banks with three bands:
  lit #fbf1dc, mid #c9b39a, deep #8a7f86; the mid band hatched one way and the deep band cross-hatched in strokes fixed
  to the sky (spacing 0.28° of sky), broken pen strokes along the shaded undersides only, no outline round the lit
  tops. Hero towers sit over the end of Main Street (east) and beside the chapel from boot hill (north-west). The sun
  is a white disc about 4° across ringed in ink (#1a1210). Env sky: same colours, sun glow 0.6, horizonFog 0.75, clouds
  amount 0.5 with hard edges.
- **Fog:** #d7b48a (ochre-tan), density 0.0012, falloff 0.012, start 60 m, sun scatter 0.5 in #f2c88a. Fog at 50 / 100 /
  300 m: 0 / 5 / 25 %.
- **Materials:** `toon` with three **hard** bands: `[-1,0.42] [0.04,0.7] [0.42,1.0]` (shade 42 %, mid 70 %, light
  100 % of the sun), softness 0.012. Textures at mip 1.8, saturation 1.12, contrast 1.08. Tints toward the palette:
  wood #b8703a at 20 % (teal planks #1f8f8c at 35 %, saturation 1.25), rock #b85a38 at 35 %, stone #b8703c at 20 %,
  grass #8a9a4a at 35 % (value 0.9), foliage #4f8a52 at 40 %, bark #6a3420 at 30 %, water #1f8a8a at 60 %. Creatures:
  value 0.5, saturation 0.9, rust tint #5a2418 at 30 %, hard specular. Glows via `flat` x1.4.
- **The ink** (`INK_HOOK`, runs in every material):
  - Hatching is **world-space**, three planar projections blended by the normal, strokes at 0.06 m spacing; the spacing
    doubles wherever lines would come closer than 5.5 px on screen, so lines never swim.
  - Direction A at about 50° (0.87 rad) lives in the mid and deep bands; direction B across it (-0.6 rad) only in the
    deep shade; direction C steep (1.35 rad, 0.042 m) is grime in cavities where occlusion drops under 0.78.
  - Each stroke wobbles and breaks where a pen would lift. Hatching fades from 70 m to 170 m (water and foliage
    40-110 m at 60 % density).
  - The colourist's shade: a rust core (0.42, 0.10, 0.045) at 30 % in the mid band, cool plum (0.13, 0.10, 0.20) at
    40 % in the deep shade, at their own lightness.
  - "Inked textures": where the texture's detail falls well below its own mip-5 mean (plank seams, cracks), ink
    (up to 85 %). Terrain inks its detail textures and the macro map's wheel ruts, and draws dark ground tone as dry
    pen strokes within ~40 m. Deepest cavities spot to black.
  - Ink colour is #1a1210 (linear 0.0085, 0.0052, 0.0042). The gun's hook has no crevice ink (grime 0.5).
- **Terrain:** toon, same bands, blur 1.2, saturation 1.05, value 0.88, contrast 1.1, tint #c27848 at 38 %, detail 0.8.
- **Post (in order):** bloom 1.6 / 0.25 / 0.5; the **ink contour pass** (display-referred): silhouettes from the
  second difference of inverse depth in four directions, creases from normal differences at half the width; width
  about 3.2 px at 2.5 m and under, 1.25 px at 55 m and over, swelling by a world-space noise (x0.55 to x1.45) so
  strokes taper; silhouettes fade out from ~180 m to 520 m, creases from 40 m to 100 m; lines **boil at 8 drawings a
  second** (frozen in stills); far lines melt into the colour behind (x0.42) from 30 m to 260 m. Then grade contrast
  1.06, saturation 1.05, vibrance 0.1, lift #0e0806, shadows #6a3a30, highlights #ffe2b0, split 0.18, curve 0.12;
  vignette 0.22 / 0.6, colour #1a0c08; grain 0.018, size 1.2, mono. There is **no ao pass**: the hatch and spotted
  blacks do that job.
- **Effects:** muzzle #ffe46a, tracer #fff4d0, impact #ffcf4a, blood #8fe04a, hit #ffffff. The muzzle flash is an
  opaque jagged comic star (nine uneven spikes, white-hot core, yellow and orange rings, ink rim). Dust 0.2. Smoke 0.12.
- **Unreal recipe (condensed):**
  - Cel bands in the deferred post, as Hi-Fi Rush: SceneColor / (BaseColor x light) snapped to three bands (0.42, 0.7,
    1.0), the middle tinted toward rust and the deepest toward plum at the same lightness. Base colour at +1.8 mip bias.
  - Hatching in the master materials, not the post: a world-aligned (triplanar) hatch texture pair at 6 cm,
    blended by pixel footprint (DDX/DDY of world position), masked by the shadow and N.L terms.
  - Inked textures: ink the base colour where it falls well below its own mip-5 mean (one extra sample).
  - Contours: one post pass on SceneDepth (inverse-depth second difference in 4 directions) and the GBuffer normal,
    3 px near to 1 px far, swelling by world noise, colour #1a1210, far lines tinted from the scene.
  - Sky: an unlit sky-sphere material (four-stop gradient, 2D cloud layer with three-band shading, hatch texture in
    sky space).
  - Muzzle flash: a jagged star mesh in an opaque translucent material.
- **Cost:** ~0.8 ms (outline + bands 0.5, hatching 0.2, sky 0.1).
- **Refs:**
  - How Borderlands' devs created its style (80.lv) <https://80.lv/articles/how-borderlands-devs-created-franchise-s-signature-art-style>:
    the counter-example (ink in textures plus one outline); ours hatches by light in world space.
  - Borderlands 4's "dynamic inking" (Adam May) <https://cogconnected.com/feature/borderlands-4s-adam-may/>
  - 3D toon rendering in Hi-Fi Rush (GDC 2024) <https://gdcvault.com/play/1034251/3D-Toon-Rendering-in-Hi>
  - Wild Bastards' line work (Aftermath) <https://aftermath.site/wild-bastards-blue-manchu-art.md>
  - Outline post process in UE (Tom Looman) <https://tomlooman.com/unreal-engine-outline-multi-color-post-process/>

---

## 5. Teropa Pulp

![Style 5 contact sheet](Sheets/sheet_05.jpg)

Family **Retro sci-fi** (research H). Module `05_pulp.js`; helper `lib/pulp_sky.js`.
**Verdict: the user's favourite (one of 3, 4, 5, 6).**

- **The idea:** the cover of a 1970s science-fiction paperback come alive. An enormous banded gas giant with rings fills
  the sky over the frontier town, with two small moons, in an airbrushed dome from deep teal to tangerine. Sienna and
  ochre ground, alien turquoise trees, chrome-bright metal, a tangerine sun against teal shade.
- **Why it wows:** one glance says "another planet": the strongest hook for a weird west on Teropa, and no other
  western has it. The HUD's own pair (orange and cyan-teal) is the whole palette. It is the cheapest wow (~0.25 ms):
  the sky is one material and the rest is palette pushes and a LUT.
- **Under the HUD:** the HUD's retro-futurist gunmetal and Chakra Petch sit naturally under a paperback sky; the
  bright giant stays off the corners' text. Shaded walls go deep teal, which suits the white outlined text.
- **Risks:** the giant can steal attention in combat (keep it dimmer than the sun and never behind the crosshair at
  spawn points). Palette pushes can go muddy in the mid-tones (needs a pass per texture set). The sky maths need a check
  near the ring's edge at 4K (a baked 4K planet on a billboard is the fallback). Turquoise trees read strongly alien:
  decide per biome how far to go.

**The look in Unreal terms**

- **Sun:** from 258° (WSW), 18° up; tangerine #ffa461; intensity 4.6; softness 1.4; strength 1.0. Actor yaw 78, pitch
  -18. Recipe: about 3000 K plus an orange filter, 4.5 lux.
- **Ambient:** sky teal #3a8eaa, ground #9a5638, 1.5. Lamps x1.2. Recipe: a Sky Light from a cubemap of the sky gives
  the teal fill; no SkyAtmosphere.
- **Exposure / tone:** 1.05, `aces`.
- **Sky** (own dome `pulpSky`, all analytic, no textures): gradient horizon tangerine #ff9e66, low peach #f1c7a0 (to 7 %
  height), mid turquoise #2f9c9c (5-32 %), zenith deep teal #0a4352 (28-95 %), below #6e4132, haze 0.85; a pink belt
  #f0a3a0 low on the horizon opposite the sun; the sun's side glows; sun disc about 1.4° across; faint daylight stars
  (0.6) only above ~20° and away from the sun; low painted stratus streaks #ffdcbc / #c88a88 from the horizon up to about 17°.
- **The gas giant:** at azimuth **110° (ESE), 10° up** (the brief asked for 20-30°; the code is 10°), **34° across**,
  rolled -12°, rings tilted -19° (we see their underside, the near arc crossing the upper disc), ring radii 1.28 to
  2.45 planet radii, ring opacity 0.95. Planet brightness 0.95, veil 0.4 (our own sky lies in front of it). Bands cream
  #f2cf98, peach #ea9660, ochre #d27a2c, rust #9c3219, deep sienna #40130c, teal #25877f (a cold teal belt south of the
  equator and teal poles, plus a great storm: a cream oval in a rust collar). Ring colours #f4ddb4, #d98c58, #6f9f99,
  with a faint inner ring, a bright middle ring, a great gap and an outer ring. It is lit from a fixed direction in its
  own frame (-0.5, 0.62, 0.6), so it is always a fat gibbous with the rings' shadow across it, and the planet's shadow
  falls on the rings. A moon's transit shadow is on the disc.
- **Moons:** one at 58° (ENE), 17° up, 4.6° across (#efe0cc / #a08c80); one at 314° (NW), 20° up, 7.5° across
  (#d9ebe4 / #86a3a2). They are lit by the real sun (phases follow it). The `street` shot sees the giant and the
  small moon, `chapel` the giant, `boothill` the big moon.
- **Fog:** peach #e2a58e, density 0.0017, falloff 0.011, start 22 m, sun scatter 0.9 in #ffb070, exponent 6. Fog at 50 /
  100 / 300 m: 5 / 12 / 38 %. Recipe: density 0.02, falloff 0.12, start 25 m, directional inscattering exponent 6, so
  far ridges layer into flat peach and teal silhouettes.
- **Materials:** `pbr`. The **teal shade hook:** whatever turns from the sun or sits in shadow drifts toward teal
  (0.028, 0.20, 0.29) at its own lightness, up to 42 %: tangerine key always meets teal shade. Base: textures mip 1.5,
  saturation 1.15, contrast 1.04. Wood tinted sienna #c06a3c at 28 % (saturation 1.25; painted, teal and oxide planks
  at 1.3 untinted), rock #c56a3e at 40 %, stone #c28e64 at 30 %, ground ochre #d39a48 at 30 %, bark #8a4630 at 35 %.
  Dry grass and tufts: tint #c79a4a at 35 %, hue -6; sage hue +55. **Trees: alien turquoise** (#1f9c94 at 85 %),
  translucency 0.8. **Metal: chrome** (roughness 0.5, sky reflection 1.9); rusty metal tinted sienna 25 %. The gun:
  roughness 0.6, reflection 1.7. **Creatures:** violet-black (#4a2a52 at 55 %, value 0.42, saturation 0.7), roughness
  0.55, reflection 1.4, teal rim #5fc7c0 (power 3.5, strength 0.55). Water teal #2f948f at 60 %, reflection 1.6.
- **Terrain:** the teal shade hook, blur 1, saturation 1.15, value 0.95, tint #c46e3e at 52 %, contrast 1.05, detail 0.5.
- **Post:** ao radius 1.2, intensity 0.55; bloom 1.1 / 0.32 / 0.85; grade contrast 1.08, saturation 1.1, vibrance
  0.15, lift #0d2629 (lifted teal blacks), shadows #2c8a8a, highlights #ffb27a, split 0.3, curve 0.12; vignette 0.3 /
  0.55, colour #1c1020; grain 0.028, size 1.3, coloured 0.15.
- **Effects:** muzzle #ffc070, tracer #fff0c8, impact #ffb36a, blood #6fe3c4 (teal), hit #ffe8c8. Dust 0.35, motes
  #ffd2a8. Smoke 0.2.
- **Unreal recipe (condensed):**
  - Sky: one unlit sky-sphere material (`M_Sky_Pulp`): a four-stop gradient on the camera vector's Z, a pink band
    opposite the sun, and the giant from `dot(CameraVector, PlanetDir)`: a disc in its own frame, bands from a small
    1D LUT warped by a tiling noise texture, rings from a ray-plane intersection with a radial ring LUT, ring and
    planet shadows (one more ray-plane test each), two moon discs. ~180 instructions, on sky pixels only.
  - Light: directional sun 18°, ~3000 K plus an orange filter, 4.5 lux; Sky Light from a captured cubemap of that sky.
  - Fog: Exponential Height Fog, peach #e3a48a, density ~0.02, falloff 0.12, start 25 m, tangerine inscattering
    exponent 6.
  - Materials: the existing masters with a palette push per instance (Tint toward sienna for wood, rock and ground;
    foliage TintVariation toward turquoise), +1.5 mip bias on base colour, metal roughness x0.5.
  - Spiders: a darker violet-black chitin instance with low roughness.
  - Post: Bloom (threshold 1.1), a LUT with lifted teal blacks and warm highlights, Film Grain 0.03, Vignette 0.32.
- **Cost:** ~0.25 ms (the sky material and the LUT; no extra passes). The brief said 0.2.
- **Refs:**
  - Chris Foss (SF Encyclopedia) <https://sf-encyclopedia.com/entry/foss_chris>
  - The art of No Man's Sky <https://www.nomanssky.com/2016/04/art-of-no-mans-sky/>
  - How to paint explosive environments (Foss, Berkey, Elson colour) <https://www.creativebloq.com/features/how-to-paint-explosive-environments>
  - Dissecting the art style of Outer Wilds (80.lv) <https://80.lv/articles/dissecting-the-art-style-of-outer-wilds/>
  - 1970s NASA space-settlement paintings <https://anothermag.com/art-photography/7711/1970s-nasa-paintings>

---

## 6. Sunbleached

![Style 6 contact sheet](Sheets/sheet_06.jpg)

Family **Luminous** (research K). Module `06_bleached.js`; helpers `lib/bleached_glsl.js`, `lib/bleached_sky.js`.
**Verdict: the user's favourite (one of 3, 4, 5, 6).**

- **The idea:** a high-key, luminous, mythic frontier. A blinding white-gold sun inside a faint ice halo, sandstone and
  timber that glow as if lit from within, cool lavender shadows, sand that glitters as you move, rim light on every
  silhouette. The reverence of a pilgrimage across a dead saint's land.
- **Why it wows:** the light itself: a huge soft sun glow and halo, long lavender shadows, glints on the street,
  luminous haze. Silhouettes against the light are strong, and the spiders are the only dark things in a field of
  light.
- **Under the HUD:** the HUD's outlined white text and gunmetal read as the darkest values on screen. Its cyan is the
  one cool accent the world leaves free; its orange sits inside the world's gold, so lean on the cyan.
- **Risks:** high-key art can wash out (the spiders and loot must stay dark and saturated: in the sheets the spiders
  read as mid-dark shapes with a light rim, not black). The aerial shot is mostly pale haze. Glitter can shimmer in
  motion (in the code it fades out between ~22 and 60 m, the recipe says ~70 m; cell size grows with distance). A contemplative look may clash with gunplay; the
  white-gold flashes and dark enemies keep combat punchy. Near-monochrome art risks monotony over a long game.

**The look in Unreal terms**

- **Sun:** from 64° (ENE), 19° up (ahead and left of Main Street, in the frame's top-left corner, clear of the
  spiders); white-gold #ffdfa6; intensity 4.6; softness 2.6; strength 0.92. Actor yaw 244, pitch -19. Recipe: yaw 58,
  16° up, #ffe6b4, light source angle ~2 (the code is 6° more to the east and 3° higher).
- **Ambient:** sky lavender-blue #7f8cf0, ground warm #c89a72, 1.25. Lamps x1.0.
- **Exposure / tone:** 1.05, `aces`.
- **Sky** (own dome `bleachedSky`): horizon #fbeed6, mid #7fbfd4 (to 22 % height), zenith pale turquoise #2f90c2
  (18-80 %), below #e8d6bc; a lavender band #c8bce8 low on the horizon opposite the sun; fog colour mixed in at the
  horizon (0.6). **Cirrus:** long sheared wisps on a high layer, #efebe4 / #d2cde4, amount 0.75, lit gold toward the
  sun. **The sun:** a white-hot disc (#fff6e6 x 16) about 1.5° across, a huge soft glow #ffe2a8 (three falloffs: wide,
  middle, tight core), and the **22° ice halo** (a thin ring at exactly 0.3840 rad = 22.0° from the sun, reddish inside,
  bluish outside, amount 0.6). Env sky: sun disc 1.5, glow 1.4, no clouds.
- **Fog:** luminous cream #f6e8d4, density 0.0035, falloff 0.025, start 25 m, sun scatter 0.9 in #fff7e4, exponent 6.
  Fog at 50 / 100 / 300 m: 8 / 23 / 62 %.
- **Materials:** `pbr`, saturation 0.95, value 1.1, tint toward sandstone cream #eec79a at 35 %. A hook chain on every
  surface:
  - **Lavender shade:** where the sun does not reach, colour x(0.90, 0.84, 1.22) (blue up, green down), never grey.
  - **Warm rim** (1.0, 0.82, 0.58): `(1 - N.V)^3` x (0.22 + 1.1 x backlit^2) x occlusion, k 0.5. It blazes on
    silhouettes standing between the eye and the sun.
  - **Warm bounce** (0.42, 0.32, 0.22) at k 0.15, brightest on sides and undersides in shade.
  - **Glow from within** (1.0, 0.70, 0.42) at k 0.3: sunlight wrapped a little past the terminator, so sandstone and
    sun-dried timber glow.
  - **Glitter** on rock and stone (density 6 %, strength 10, fades by 50 m) and terrain (5 %, strength 16, fades by 60 m):
    sparse facets in world-space cells (1.2 cm at 2 m, **doubling with each doubling of distance** so a glint stays
    about 2 px wide) that flash when they mirror the sun into the eye, twinkling slowly.
  - Terrain adds a broad **sheen** lobe toward the sun at grazing angles (Journey's "ocean specular", k 0.4), saturation
    1.0, value 1.1, tint #ecbf88 at 45 %.
  - Foliage: saturation 0.85, value 1.12, hue +6, rim 0.45. **Creatures:** value 0.24, saturation 0.5, roughness 0.8,
    rim (1.0, 0.8, 0.5) at 1.1, power 4. Gun: rim 0.2. Far backdrop: value 1.2, saturation 0.7, cream tint.
- **Post:** ao radius 1.2, intensity 0.5, colour #3a3480; **godrays** strength 0.3, decay 0.97, density 0.9, #fff0cc,
  threshold 1.6 (shafts from the sun's core only, so they stream past rooflines instead of whitening the corner);
  **bloom** threshold 0.95, strength 0.35, radius 1.0 (a low threshold and wide kernel: the luminous air);
  **halation** threshold 1.6, radius 1.0, #ffcf8a, 0.15; grade contrast 1.06, saturation 1.12, lift #0e0b20; sharpen
  0.25; vignette 0.12 / 0.6, colour #4a3e6a. No grain.
- **Effects:** muzzle #fff2c8, tracer #fffbea, impact #ffe9a8 (white-gold sparks), blood #8be05a, hit #fff6e0. Dust
  0.2, glowing embers 0.2 in #ffe7a8, motes #ffe0a0. Wind 0.12. Smoke 0.16.
- **Unreal recipe (condensed):**
  - Light: directional yaw 58, 16° up, #ffe6b4, soft shadows (source angle ~2); a lavender-blue sky light (#7f8cf0)
    with a warm lower hemisphere.
  - Sky: an unlit sky-sphere material: white-gold horizon to pale turquoise, lavender band opposite the sun, sheared
    cirrus from two panning noise samples, a wide sun glow, the 22° halo as a thin ring (`dot(view, sun)` against
    cos 22°, warm inside, cool outside).
  - Exponential height fog: warm cream with directional inscattering; light shafts on the directional light.
  - Masters: one material function adds the backlit fresnel rim, a warm wrap past the terminator and a lift toward
    sandstone cream (albedo x1.1, saturation 0.95).
  - M_Terrain and rock: glitter from a hashed world-space cell normal against the half vector (cell size doubling with
    distance) plus a broad sheen lobe: pure ALU, no textures.
  - Spiders: albedo x0.3 with a thin rim.
  - Post: a low-threshold, wide, gentle bloom, a LUT for the slight lavender lift, a light sharpen; fixed exposure.
- **Cost:** ~0.4 ms (glitter and rim ALU 0.1, light shafts 0.2, bloom already paid).
- **Refs:**
  - Sand rendering in Journey (GDC 2013) <https://gdcvault.com/play/1017742/Sand-Rendering-in>
  - Journey sand shader reconstruction (Alan Zucconi) <https://www.alanzucconi.com/2019/10/08/journey-sand-shader-3/>
  - Art of Sky: Children of the Light (80.lv) <https://80.lv/articles/interview-a-deep-dive-into-the-art-of-sky-children-of-the-light-with-thatgamecompany>
  - Designing emotional environments for Sky (80.lv) <https://80.lv/articles/how-to-design-emotional-game-environments-for-sky-children-of-the-light/>
  - Journey's desert, chosen to connect people (Gameranx) <https://gameranx.com/updates/id/13724/article/journey-s-desert-setting-was-chosen-to-help-players-connect-on-a-human-level/>

---

## 7. Neon Frontier

![Style 7 contact sheet](Sheets/sheet_07.jpg)

Family **Neon dusk** (research I). Module `07_neon.js`; helpers `lib/neon_sky.js`, `lib/neon_post.js`,
`engine/tonecurve.js`. Verdict: not picked.

- **The idea:** Teropa after sundown as a synthwave western. An indigo-to-magenta sky over a huge striped sun sinking
  at the end of Main Street, a rain-slick street that mirrors it, every roofline and spider traced in thin glowing
  cyan, hot neon in the windows and lamps.
- **Why it wows:** maximum thumbnail pop; nobody in the western lane looks like this. The world is dark and the light is
  the content, so the saints' cyan embers, neon windows and muzzle flashes carry the image.
- **Under the HUD:** the HUD's cyan and orange become the world's own neon; its corners sit on dark violet ground and
  indigo sky, the best contrast the HUD gets in any style.
- **Risks:** saturation fatigue over long sessions (keep the ground dark and the neon to edges and lights). Contour lines
  can crawl on foliage and fences (fade by distance, keep off thin cards). A synthwave sun can read as off-brand for a
  gothic chapel and churchyard; the cyan embers tie it back.

**The look in Unreal terms**

- **Sun (the light):** from 90° (E), only 6° up; coral #ff6a5c; intensity 3.2; softness 1.4; strength 1.0. Actor yaw
  270, pitch -6. Long shadows run down Main Street toward the player.
- **Ambient:** sky blue-violet #4a56d6, ground #2a1238, 1.8. **Lamps x5.0** (house lamps are recoloured alternately hot
  pink #ff3a8a and orange #ff7a2a in `activate`).
- **Exposure / tone:** 1.0, `aces`.
- **Sky** (own dome `neonSky`): zenith #0b0620, high #1e0c40, mid #4a1670, low magenta #b8247a, horizon hot orange
  #ff7a3a toward the sun and #6a1f7e on the far side, below #140a20; plum cloud streaks #2a0f3c lit #ff4f9a; stars
  0.9. **The retro sun** is a sky object (it is not the light): azimuth 92°, **12.5° up**, **25° across**, 7 bars cut
  from its lower half by `frac()` of its height, yellow #ffd23a to orange #ff7a2a to pink #ff2a8a, brightness 1.15 so
  it blooms, halo #c8206a at 0.6. The disc hangs a little higher than the light so its barred lower half clears the
  rooftops. Env sky: sun disc 0, stars 0.8.
- **Fog:** violet #4e2a82, density 0.0045, falloff 0.035, start 8 m, sun scatter 0.95 in hot pink #ff4f86, exponent 5.
  Fog at 50 / 100 / 300 m: 17 / 34 / 73 %.
- **Materials:** `pbr`. Albedo dark and cool: value 0.55, saturation 0.7, tint #6a5aa8 at 22 %; roughness 0.55 on
  metal, stone and rock (0.8 elsewhere); a backlit rim in hot red-pink (1.0, 0.32, 0.4) at 0.55. Foliage: value 0.5,
  hue +40, tint #1f6a86; marks itself in the alpha so the contour pass traces it only against the sky. **Windows:** a
  black base with emissive neon x2.4 whose hue drifts across the town (magenta to orange, now and then cyan): a
  building's windows share a colour, the street alternates. Lanterns #ff7a2a glow x3; other glows x2.5; the gun's
  accent strip a cyan tube (#29d4ff x2.2). Creatures: value 0.32, a hot-pink rim (1.0, 0.1, 0.35) at 0.9. The Sink's
  egg sacs pulse acid green; webs get a faint cyan sheen. Water: value 0.4, roughness 0.03.
- **Terrain:** value 0.4, saturation 0.55, tint #5a4a9a at 30 %, roughness 0.7, plus the **wet street**: a planar
  reflection cheat in the material (no extra pass). Puddles from noise (~13 m patches), damp dirt, the view vector
  reflected off a flattened normal; the sky gradient and the retro sun (stretched toward the viewer as on wet asphalt,
  cut by the sun's shadow) are looked up analytically and Fresnel-weighted (up to 90 %).
- **Post:** ao radius 1.2, intensity 0.6, colour #0a0414; **cyan contours** (scene-referred, so they bloom): colour
  #12c4ff, width 1.8 px, strength 1.3, creases 0.6, fade 30 to 160 m, edges against the sky traced out to 900 m,
  vegetation traced only against the sky, needs a depth jump of 0.7 m to count; **bloom** threshold 0.85, strength
  0.75, radius 0.85; grade contrast 1.06, saturation 1.12, vibrance 0.15, lift #0e0620; chromatic aberration 0.00025
  (a whisper; the default is 0.0025); grain 0.025, size 1.4, coloured 0.25; vignette 0.35 / 0.55, colour #06020f.
- **Effects:** muzzle #ff6a4a, tracer #8ff0ff (cyan-white), impact #ff8a3a, blood #6dff5a (neon green), hit #ff4fd0.
  **Embers 0.9 in cyan #5ac8ff**, dust 0.25, motes #ff8ad8. Wind 0.1. Smoke 0.18.
- **Unreal recipe (condensed):**
  - Sky: unlit sky-sphere material with the elevation gradient and the retro sun (yaw 92, 12.5° up, 25° across) as a
    disc in its tangent plane, bars by `frac()`, emissive; plum cloud streaks; hashed-grid stars.
  - Light: directional yaw 90, 6° up, coral #ff6a5c; blue-violet sky light #4a56d6; Lumen off.
  - Fog: Exponential Height Fog, violet, hot-pink directional inscattering.
  - Materials: one function on the masters (albedo x0.55 and cooled, backlit rim in the sun's colour, window
    emissives x2.4 drifting hue, orange and pink lamps, acid-green egg sacs).
  - Wet street in M_Terrain: reflect the view vector off a flattened normal, look up the sky's gradient and sun
    analytically (same maths as the sky material), Fresnel-weighted by the road mask and puddle noise. No SSR or
    planar reflection.
  - Contours: a post material before tonemapping (BL_BeforeTonemapping) tracing depth discontinuities and creases in
    cyan emissive so bloom makes them glow. Foliage writes a CustomStencil value and is traced only against the sky.
  - Post: bloom ~1.2, a little chromatic aberration, grain, vignette, a LUT with a violet lift.
- **Cost:** ~0.8 ms (contour pass ~0.3, bloom ~0.3, the rest material ALU).
- **Refs:**
  - Blade Runner 2049: Las Vegas in orange haze (StudioBinder) <https://www.studiobinder.com/blog/blade-runner-2049-cinematography-analysis/>
  - Tron: Legacy colour analysis <https://www.pushing-pixels.org/2010/12/28/the-colors-of-tron-legacy.html>
  - Cyberpunk 2077 visual style <https://adrianlungu.substack.com/p/cyberpunk-2077s-visual-style-crafting>
  - Far Cry 3: Blood Dragon review (bit-tech) <https://bit-tech.net/reviews/gaming/far-cry-3-blood-dragon/1/>: the
    warning that permanent neon night hurts readability.
  - Marathon art direction (80.lv) <https://80.lv/articles/marathon-art-director-shares-the-team-s-creative-influences>:
    a bold graphic identity must be one's own.

---

## 8. Ember Gothic

![Style 8 contact sheet](Sheets/sheet_08.jpg)

Family **Dark weird west** (research C). Module `08_gothic.js`; helpers `lib/gothic_sky.js`, `gothic_fog.js`,
`gothic_lanterns.js`, `engine/tonecurve.js`. Verdict: not picked.

- **The idea:** the weird west as a ghost story. Night under a huge pale moon, a fog sea rolling between the graves and
  down Main Street, a cold blue-green world. The only warm things are the lanterns, lit windows and muzzle flashes,
  while the saints' cyan embers drift in the air.
- **Why it wows:** the story's own mood (a revenant, a churchyard, a gravekeeper), and the look where the chapel and boot
  hill are strongest. The cyan embers become a light source and the HUD's orange and cyan become the world's only
  warm and cold lights.
- **Under the HUD:** the best fit of all: dark, quiet corners make the white outlined text and the orange and cyan
  lines glow, and the embers share the HUD's cyan.
- **Risks:** dark screenshots vanish on Steam's dark store pages (compose store shots around the moon, fog and lamps).
  Spiders in fog must stay silhouettes (a cool rim; the gamma slider must never crush them). More than six lamps near
  the camera need a budget. Brown and dark shooter fatigue: the embers and warm lamps carry the colour. The Sink is
  still dark in this style (known, 2026-10-08).

**The look in Unreal terms**

- **Moonlight (the "sun"):** from 306° (NW), 22° up (beside the chapel steeple as seen from boot hill); #8fb0d8;
  intensity 1.9; softness 2.2; strength 1.0. Actor yaw 126, pitch -22. Recipe: 0.3 lux, 7500 K tinted #8fb0d8, cascaded
  shadows; a SkyLight captured from the night sky at 0.15.
- **Ambient:** sky #26405a, ground #10161a, 0.8; very low dark teal. **Lamps x5.**
- **Exposure / tone:** **1.9** (a dark scene brought up), `aces`.
- **Sky** (own dome `gothicSky`): zenith #04070d, horizon #1e2a36, below #0a0d10, horizonFog 0.85; a **huge full moon**
  9.5° across at the light's direction (#eef2f5, aureole and faint corona #8fa9c8); a second pale crescent moon at
  141° (SE), 15° up, 4.2° across; stars 1.0, fading near the moon; thin silver-edged moonlit cloud banks (amount 0.34,
  lit #7d8ea2, dark #141c26). Env sky: zenith #05080f, horizon #26323e, stars 0.8.
- **Fog (two layers):** the height fog: #34444f, density 0.0045, falloff 0.06, start 4 m, scatter 0.4 in moon-blue
  #6f87a3, exponent 10; fog at 50 / 100 / 300 m: 19 / 35 / 74 %. Plus a **ground-fog pass** where the player stands:
  density 0.056 per metre, falloff 0.5 per metre, base -0.3 m, to 150 m, broken into slow banks by world noise (scale
  0.05, contrast 1.8, drifting), lit by the moon and by up to eight lamps (an analytic in-scatter per lamp, so the
  halos are smooth). The fog floor follows the terrain's height texture down into pits (the Sink) and stays at level
  where the ground rises (the chapel knoll).
- **Lamps:** every house lamp reaches out of the windows and doors (range x1.8, at least 6 m) in #ffa458; **13 iron lamp
  posts** (six along Main Street, one at the chapel yard's gate, the gravekeeper's lantern among the graves, three on
  the road up past boot hill, two at the back lots; intensity 2.6, range 9 m; the six nearest the camera are lit);
  **Pa's lantern** at the player's hip (22 candela, 20 m, breathing and guttering by about +-10 %). Windows: lit houses
  glow 4.5 in #ff9f50, dark cottages 1.3.
- **Materials:** `pbr`, saturation 0.5, value 0.74, normals 1.2, roughness x0.8 on stone and rock. Grass: saturation
  0.42, value 0.45, hue +8. Foliage: saturation 0.42, value 0.6. **Creatures:** saturation 0.5, value 0.7, a **cool
  rim #a9ccff** (power 2.2, strength 1.4) so they read against the fog. Gun saturation 0.75. Terrain: saturation 0.45,
  value 0.72, normals 1.0, roughness 0.62 (a dew sheen toward the moon).
- **Post:** ao radius 1.3, intensity 0.85; the ground-fog pass above; bloom 1.2 / 0.32 / 0.8; **godrays from the moon**
  strength 0.14, decay 0.972, density 0.9, #a6bfe0, threshold 0.9; grade contrast 1.08, saturation 0.92, curve 0.18,
  lift #060c10, shadows #2a5866 (crushed teal), highlights #c2cdd6, split 0.3, temperature -0.18; vignette 0.5 / 0.6,
  colour #020406; grain 0.035, size 1.4, coloured 0.12.
- **Effects:** muzzle #ffd49a, tracer #ffe1ae, impact #ffc07a, blood #9be35a, hit #d8ecff. **Embers 0.55 in #2fa6e6**,
  dust 0.2, motes #9fb4c8. Wind 70°, 0.08, speed 0.9. Smoke 0.12.
- **Unreal recipe (condensed):**
  - Directional light as the moon (0.3 lux, 7500 K tinted #8fb0d8, 22° up in the north-west), cascaded shadows; a
    SkyLight captured from a low teal night sky at 0.15.
  - Sky sphere material: blue-black gradient, a full-moon disc with a maria texture and an aureole, panning moonlit
    cloud banks, stars, a second crescent moon as a card.
  - **Two exponential height fogs** (a thin one for distance, density 0.005, and a dense ground layer, falloff 0.5/m,
    with inscattering in the moon's colour); the Sink gets its own local fog volume. No volumetric fog on Medium.
  - **Fog cards:** camera-facing soft particles (depth fade) with a slow panning noise, placed by the area build in
    streets and graves, 6-12 per view; an additive glow sprite on every lamp in place of volumetric scattering.
  - Lamp posts along Main Street, the road to boot hill, the graves and the chapel gate (static mesh + point light, 9 m,
    no shadows, the six nearest lit); window emissive 4.5; a flickering point light on the character for the hip lantern.
  - Materials: one material parameter collection drives albedo desaturation (0.5) and darkening, dewy ground roughness
    (0.6) and a cool fresnel rim on creatures.
  - Post: SSAO, bloom (threshold 0.9), light shafts on the moon, a LUT with crushed teal shadows and a lifted fog floor,
    vignette, grain. Niagara: cyan ember sprites around the camera.
- **Cost:** ~1.2 ms (fog cards and the second height fog ~0.5, lamps 0.3, post 0.4). The costliest of the ten.
- **Refs:**
  - Hunt: Showdown, first impressions (PC Gamer) <https://pcgamer.com/hunt-showdown-is-just-incredible>
  - The development of Hunt: Showdown (80.lv) <https://80.lv/articles/the-development-of-hunt-showdown>
  - Hunt: Showdown, point blank through the fog <https://www.huntshowdown.com/news/point-blank-through-the-fog>
  - Bloodborne in UE4 (Simon Barle, 80.lv) <https://80.lv/articles/simon-barle-bloodborne-in-ue4>: a cool base against
    warm point lights; never let a scene sink to black.
  - The gothic sensibilities of Darkest Dungeon (GameSpot) <https://www.gamespot.com:443/articles/the-gothic-sensibilities-of-darkest-dungeon/1100-6424880/>
  - Lighting, Atmosphere, and Tonemapping in Ghost of Tsushima (SIGGRAPH 2021) <https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html>: the
    Purkinje shift (night goes cooler and less saturated).

---

## 9. Celluloid West

![Style 9 contact sheet](Sheets/sheet_09.jpg)

Family **Film stock** (research B). Module `09_celluloid.js`; helper `lib/celluloid_film.js`. Verdict: not picked.

- **The idea:** the frontier shot on 1960s widescreen film. Saturated dye reds and deep polarised blues, crushed inky
  shadows, a red-orange halation round every highlight, grain that jumps at 24 frames a second, a whisper of gate
  weave: a spaghetti western you play.
- **Why it wows:** instantly "a western", built from the grade and the lens rather than new art (the models and
  textures are untouched), and almost no game commits to it.
- **Under the HUD:** the HUD stays crisp and digital over a soft, grainy image, which separates it cleanly. The film's
  warm palette rhymes with the orange; the sky's blue keeps the cyan honest. UI is drawn after post, so the weave and
  grain never touch it.
- **Risks:** grain and halation soften enemy edges at range (grain is kept fine and weighted to the mid tones, away
  from the darks where spiders sit). Grain fights TSR and video compression on store trailers (keep a "clean" toggle).
  Gate weave must stay sub-pixel or it reads as a bug. A Leone homage risk: emulate the stock and the lensing only,
  never music, framing or titles.

**The look in Unreal terms**

- **Sun:** from 214° (SW), 36° up; #fff0d8; intensity **6.2** (bright and hard: only style 10's 7.0 is higher); softness
  0.6; strength 1.0. Actor yaw 34, pitch -36. Recipe: 9 lux, 5200 K, 35°, contact shadows on.
- **Ambient:** sky #6f8fc4, ground #7a5a3c, 1.0. Recipe: sky light low (0.6) so shade stays deep.
- **Exposure / tone:** 1.0, **`none`**: no stock tone curve at all, the film "print" pass is the curve.
- **Sky** (the kit's own dome): zenith deep polarised blue #082a72, horizon dusty #5b86ba, below #7a6a55, sun disc 1,
  glow 0.6, horizonFog 0.3; clouds amount 0.36, white #ffffff with shade #8b97ad, scale 0.7, **sharpness 0.82** (crisp
  edges), speed 0.003. No stars.
- **Fog:** thin and dusty, #c8c2b2, density 0.0007, falloff 0.02, start **120 m**, sun scatter 0.3 in #efd7b0. Fog at
  50 / 100 / 300 m: 0 / 0 / 12 %.
- **Materials:** `pbr`, saturation 1.12, contrast 1.04 (reds and ochres up). Wood, fabric and paper: saturation 1.25,
  hue -6, contrast 1.06. Foliage and grass: saturation 0.95, hue -6, value 0.95 (toward olive). Creatures: saturation
  1.05, value 0.8, a soft cool rim #a8c4ec (power 3, strength 0.45). Terrain: saturation 0.98, hue -2, contrast 1.06.
- **Post (in order):** ao radius 1.1, intensity 0.7; **halation** threshold 0.32, radius 0.55, colour red-orange #ff4a1c,
  strength **1.8**; an **anamorphic streak** (scene-referred) threshold 5, strength 0.45, spread 10 px, #ffe0bc: a
  horizontal smear on the brightest lights; bloom 0.9 / 0.22 / 0.85; the **print** (scene-referred, outputs a
  display-referred image): the exposure goes into log stops, **dye crosstalk 0.3** (each dye layer sharpened against
  the other two), a logistic characteristic curve per channel (toe slope 0.86 below mid grey, shoulder 0.66 above,
  mid grey prints at 0.4), per-channel offsets (+0.06, 0, -0.03 stops: warm highlights, inky blue-black shadows),
  greens pulled toward olive (hue 0.30 to 0.20 by up to 30 %, quieter), yellows burnt toward sienna, saturation 1.1,
  base-density black #120b07; the **projector** (display-referred): gate weave (sub-pixel, new every 24 fps frame, a
  0.4 % scale to hide the edges), a four-tap soften 0.85, lamp flicker 2 %, **dye-cloud grain 0.095** at size 2.0
  jumping at 24 fps and weighted to the mid and low tones, the odd dust speck; vignette 0.55 / 0.8, colour #120a04,
  roundness 0.8.
- **Effects:** muzzle #fff1c4 (hot white-yellow), tracer #fff4d6, impact #ffd88a, blood #9be35a, hit #fffaf0 (a single
  bright frame). Dust 0.35, motes #ffe2b8. Wind 40°, 0.14, speed 1.6. Smoke 0.2.
- **Unreal recipe (condensed):**
  - Directional light 9 lux at 5200 K, 35° from the south-west, contact shadows; SkyLight 0.6.
  - SkyAtmosphere tuned for a deep blue zenith (Rayleigh up, Mie low), a cloud card layer with crisp cumulus; thin
    dusty height fog at the horizon only.
  - Tonemapper: the film slope / toe / shoulder set project-wide (toe 0.7, shoulder 0.3), then a LUT baked from this
    style's print pass (dye crosstalk, per-channel curves, greens to olive).
  - Halation: the bloom pass with a red-orange tint (two kernels, threshold 0.9), or a post material reusing the
    bloom mips.
  - One post material after tonemapping: gate weave per 24 fps frame, dye-cloud grain at 24 fps weighted to the mid
    tones, rare dust specks, a warm vignette, a horizontal streak sampled from the bloom output.
  - Materials untouched apart from a parameter collection: saturation 1.12, reds and ochres up a little.
- **Cost:** ~0.4 ms (LUT free, halation 0.15, projector 0.2).
- **Refs:**
  - A Fistful of Dollars, cinematography analysis (Color Culture) <https://colorculture.org/a-fistful-of-dollars-cinematography-analysis/>
  - For a Few Dollars More, cinematography analysis <https://colorculture.org/for-a-few-dollars-more-cinematography-analysis/>
  - Spaghetti western genre look (Melies) <https://melies.co/cinematic-techniques/genre-looks/spaghetti-western>
  - Once Upon a Time in the West 4K review (The Digital Bits) <https://digitalbits.com/item/once-upon-a-time-in-the-west-2024-4k-uhd>
  - TXC stock film emulator for UE5 (Unreal forums) <https://forums.unrealengine.com/t/txc-stock-film-emulator-physical-based-film-emulation/2711083>
  - Time-varying film grain (gamedev.net) <https://gamedev.net/shaderlab/57-time-varying-film-grain/>
  - Red Dead Revolver review (GamesRadar) <https://www.gamesradar.com/uk/red-dead-revolver-review>

---

## 10. Golden Hour

![Style 10 contact sheet](Sheets/sheet_10.jpg)

Family **Cinematic realism** (research A and P). Module `10_golden.js`; helpers `lib/golden_rays.js`, `golden_flare.js`.
Verdict: not picked.

- **The idea:** grounded cinematic realism, the "big studio" option. A low golden sun rakes down Main Street, long cool
  shadows run toward the camera, dust hangs in the air lit like gold, sun shafts break between the false fronts and
  the valley layers away into warm haze, in a rich, natural grade with the saints' cyan as the only cool.
- **Why it wows:** the light is the art. Same textures as today, but more light, more air and more drama: a trailer can
  be cut from it. The realistic end of the range.
- **Under the HUD:** the HUD's orange is the sun's own colour and its cyan the only cool in the frame; the shadowed
  foreground keeps the bottom corners dark and calm under the text.
- **Risks:** it is compared directly with the big open-world games, so the sky and haze must be excellent. Too much haze
  goes muddy; backlit shots need the sky fill and AO tuned so the near side never goes flat brown. A sun ahead of
  the player hides enemies in glare (keep the haze bright behind them and the flare off the crosshair). The look
  depends on a fixed time of day. The Sink is still dark in this style (known, 2026-10-08).

**The look in Unreal terms**

- **Sun:** from 80° (E), **12.5°** up (just left of Main Street's axis, peeking past the false fronts); #ffb36b;
  intensity **7.0**; softness 1.4; strength 1.0. Actor yaw 260, pitch -12.5. Recipe: 10 lux at 3600 K, 12° up, cascades to
  200 m (the brief said ~9°).
- **Ambient:** sky #7399d8, ground #7a6048, **2.7** (strong sky fill).
- **Exposure / tone:** 1.12, `aces`.
- **Sky** (the kit's own dome): zenith #285aa8, horizon #c6cfd4, below #8a7a5e, sun disc 1, glow 0.45, horizonFog 0.75;
  clouds amount 0.3, gold #ffe0b4 with shade #9a92a6, scale 0.55, sharpness 0.2 (soft), speed 0.004. Recipe: SkyAtmosphere
  (Mie anisotropy 0.85) with a baked sky capture.
- **Fog:** #a3afbd, density 0.0024, falloff 0.02, start 15 m, sun scatter 0.55 in warm #ffcf8f, **exponent 10** (a tight
  glow toward the sun). Fog at 50 / 100 / 300 m: 8 / 18 / 50 %. Recipe: directional inscattering exponent 6, start 20 m.
- **Materials:** `pbr` at full texture detail with normals a little stronger: 1.2 (wood 1.3, stone and rock 1.35),
  metal roughness x0.8, foliage saturation 0.92 and value 0.92 with translucency 0.75, creatures with a warm rim
  #ffb46a (power 3, strength 0.6). Terrain: normals 1.05, detail 0.7, saturation 1.04.
- **Post:** ao radius 1.5, intensity 0.75, power 1.5; **sun shafts** (custom, half-res, 56 samples) strength 0.32,
  decay 0.97, density 0.92, #ffc884, composited by how much air lies in front of each pixel (`air` 22 m): a spider at
  4 m stays a dark silhouette with a gold rim instead of being washed by shafts that are mostly behind it; a **lens
  flare** (restrained: a six-spoke glare, a faint warm veil and five coloured ghosts, strength 1, ghosts 0.08, glare 0.8,
  veil 0.07, strength follows how much of the sun's disc is clear sky); bloom threshold **2.2** (only the brightest),
  strength 0.2, radius 0.8; grade contrast 1.05, saturation 1.06, vibrance 0.2, **curve 0.22**, shadows #3f6878 (teal),
  highlights #ffcf96 (warm), split 0.4; sharpen 0.25; vignette 0.28 / 0.6, colour #0e0906; grain 0.014, size 1.2.
- **Effects:** muzzle #ffd48a, tracer #ffe7b0, impact #ffcf80, blood #9be35a, hit #fff4e0. **Dust 0.9** (heavy golden
  motes #ffd9a0), rare cyan embers 0.06 (#5ac8ff). Wind 0.12. Smoke 0.24.
- **Unreal recipe (condensed):**
  - Directional light 10 lux at 3600 K, 12° up in the east-north-east, cascaded shadows to 200 m; light shafts (bloom
    + occlusion) with their bloom scaled down for near pixels by a post material so close enemies stay silhouettes.
  - SkyAtmosphere for the sky and aerial perspective (Mie anisotropy 0.85), a cirrus card lit by the sun colour; a
    baked sky capture for this time of day (no real-time capture).
  - Exponential height fog with warm directional inscattering (exponent 6), start 20 m, so the ridges layer into haze.
  - Materials unchanged; normal intensity 1.25 and per-role roughness via a parameter collection; a warm fresnel on
    creatures.
  - Post: SSAO 0.8 radius 150, bloom 0.2 (high threshold), Image Based Lens Flare at a low intensity, a LUT with a
    gentle S-curve, warm highlights and teal shadows, film grain 0.15, a light sharpen, vignette 0.3.
  - Niagara: golden dust motes around the camera (GPU sprites lit by the sun's direction), drifting.
- **Cost:** ~1.0 ms (light shafts 0.3, SSAO 0.5, the rest in the LUT).
- **Refs:**
  - Lighting, Atmosphere, and Tonemapping in Ghost of Tsushima (SIGGRAPH 2021) <https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html>
  - Ghost of Tsushima art director interview (Red Bull) <https://redbull.com/us-en/ghost-of-tsushima-art-director-interview-jason-connell>
  - Creating the Atmospheric World of Red Dead Redemption 2 (SIGGRAPH 2019) <https://advances.realtimerendering.com/s2019/index.htm>
  - Graphics study: Red Dead Redemption 2 (imgeself) <https://imgeself.github.io/posts/2020-06-19-graphics-study-rdr2/>
  - Creating a new time of day for Hunt: Showdown <https://www.huntshowdown.com/news/creating-a-new-time-of-day-for-hunt-showdown>
  - Mad Max: Fury Road, the colourist (FilmLight) <https://filmlight.ltd.uk/customers/meet-the-colourist/eric_whipp.php>
  - Greig Fraser on Dune: Part Two (British Cinematographer) <https://britishcinematographer.co.uk/greig-fraser-asc-acs-dune-part-two/>

---

## Switching later: a checklist

A style change in Unreal is the same few steps whichever style it is. Do them in this order, and measure after each.

1. **Light:** the directional light's yaw, pitch, colour and lux from the style's "Sun" line (the recipe's lux and
   Kelvin where it has them); the sky light's colour and intensity from "Ambient".
2. **Sky:** 3 needs a painted-panorama sky sphere; 4, 5, 6 an unlit sky-sphere material (5 is the most work: planet,
   rings, moons); 9 and 10 use SkyAtmosphere.
3. **Fog:** Exponential Height Fog from the "Fog" line and the recipe's Unreal numbers; 8 adds a second fog and fog cards.
4. **Materials:** a function or parameter set on the masters in `Tools/Unreal/build_world_materials.py`, behind a static
   switch, off by default, so a level opts in through its own material instances (the area `level.materials` / `level.swaps`
   mechanism). 3 and 1 want repainted or plated texture sets; 4 wants hatch textures; 6 and 5 are material maths only;
   9 and 10 leave materials alone.
5. **Post:** a post-process volume in that level only, with the passes listed, then a LUT (5, 6, 9, 10) or a custom
   post material (3 edge paint, 4 contours, 7 contours).
6. **Effects and creatures:** muzzle, tracer, impact and blood colours from "Effects"; each style also darkens or rims
   the spiders so they read at 25 m: keep that.
7. **Check:** `Tools\perf.ps1` before and after (8.3 ms at 1080p on Medium), `Tools\tour.ps1` for every view, the HUD
   at 1080p (`Tools\hudshots.ps1`), and the combat read at 25 m in the street and fight views.

## Source files

All under `Tools/StyleLab/web/styles/`: `00_today.js`, `01_clay.js`, `02_anime.js`, `03_painted.js`, `04_ink.js`,
`05_pulp.js`, `06_bleached.js`, `07_neon.js`, `08_gothic.js`, `09_celluloid.js`, `10_golden.js`. Helpers in `lib/`:
`anime_sky.js` (the cartoon dome of 1, 2 and 3), `anime_shaders.js`, `clay_shaders.js`, `clay_flash.js` (muzzle flashes
of 1 and 3), `painted_shaders.js`, `painted_edges.js` (1 and 3), `ink_glsl.js`, `ink_sky.js`, `pulp_sky.js`,
`bleached_glsl.js`, `bleached_sky.js`, `neon_sky.js`, `neon_post.js`, `gothic_sky.js`, `gothic_fog.js`,
`gothic_lanterns.js`, `celluloid_film.js`, `golden_flare.js`, `golden_rays.js`. Engine pieces they use (post passes,
material builders, tone curves): `Tools/StyleLab/web/engine/` and the README's kit reference.
