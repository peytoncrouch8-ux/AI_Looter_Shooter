# The ten styles: art direction briefs (the orchestrator's, 2026-10-08)

Ordered from cartoon to realistic. Each is a complete art direction, not a filter: light, sky, fog, materials,
post-process and effects all change. The research behind them is in `Research.md` beside this file (section
letters given); take each style's references (with URLs) from there. Style 0 (`00_today.js`) is today's look; every
style must clearly beat it on "would this screenshot stop someone scrolling Steam?".

Rules for all ten:
- **The HUD stays as it is** (gunmetal, white outlined text, orange #ff9f1c, cyan #5ac8ff). Keep the screen corners
  calm enough for it; never put the HUD's exact orange or cyan on large world surfaces (loot and the saints' embers
  own those), except where a brief says the world echoes them in light.
- **Combat must read.** Spiders (dark bodies) and loot must stand out at 25 m in every style. Check the `street` and
  `fight` shots for it.
- **Ship-able in Unreal on Medium** (RX 580, 8.3 ms frame, no Lumen/Nanite): every style's card gives a recipe and a
  cost estimate. Nothing copyrighted: references inspire, nothing is copied (no logos, names or specific designs).
- **Distinct.** If two of your styles start looking alike, push them apart. A style must be recognisable from a
  thumbnail.

---

## 1. Clay Frontier (`01_clay`, family "Handmade") — research J
**The idea:** the whole frontier as a stop-motion film set. Plasticine buildings with thumbprints and smoothed seams,
felt-and-flock ground, wire-armature spiders that move in charming 12 fps steps, lit by big soft studio lights. A
hand-made feature film you can play: nobody else's shooter looks like this.
- **Materials:** albedo from the manifest's representative `color` (not the photo textures), saturated a little and
  warmed, with a slow world-space hue drift so no two walls are the same clay batch. Only the big structure of the
  texture sets survives as *sculpted* relief (normal maps blurred, strength raised: planks read as tool-pressed
  grooves). A procedural thumb-smear/fingerprint normal (elongated fbm strokes, ~2-4 cm) over everything. Waxy
  sheen: roughness ~0.5 with a soft broad specular; wrap lighting (~0.45) and a warm red-orange glow in the light's
  terminator, like light through clay. Strong contact occlusion in crevices.
- **Light:** a warm key at ~35° elevation with soft wide shadows; a cool, bright fill (sky ambient); the shadows never
  go black.
- **Sky:** a painted studio backdrop: smooth pale-blue to cream gradient with fat cotton-wool clouds (soft, felt-like,
  no outlines).
- **Post:** macro depth of field, the miniature tell: sharp to ~25 m, then a soft growing blur (the near gun slightly
  soft too); gentle bloom; light vignette; a whisper of grain; `env.animStep = 1/12`.
- **Effects:** muzzle flash as a puffy orange-yellow cotton burst, chunky clay sparks, green clay gobs for spider
  hits; embers as little glowing beads.
- **Unreal:** a clay master (palette colour + world-space smear normal + wrap/subsurface-profile-lite), palette
  plates per set, Gaussian DOF, stop-motion stepping on animation blueprints (`UpdateRate`). Cost ≈0.6 ms.

## 2. Skyward Anime (`02_anime`, family "Anime cel") — research F
**The idea:** a sun-drenched anime feature on the frontier: crisp two-tone cel light, saturated sky gradients with
towering sculpted cumulus, clean coloured line work, sparkling highlights, lush gradient grass. The look with the
biggest audience on Steam, done with a western's heat.
- **Materials:** `toon` with a hard terminator (2 bands, tiny softness), shadow side tinted toward a cool lavender
  blue (~40%), albedo from the textures blurred to their big shapes (mip ~3) with saturation ~1.3 and value up.
  Anime specular on metal and glass (a crisp highlight shape), a warm rim light on creatures and edges. Grass and
  foliage: a vertical gradient from deep green at the root to bright yellow-green tips.
- **Light:** high bright sun (~50°), shadows mid-value (never murky).
- **Sky:** a deep saturated blue zenith to pale cyan horizon, big cumulus with two-tone cel shading (white lit side,
  lavender shade) and crisp edges; strong aerial perspective: far ridges go pale blue.
- **Post:** thin coloured outlines (base colour darkened, ~1.2 px, inner lines on creases, fading with distance),
  soft bloom, a little sun glare, clean grade.
- **Effects:** star-shaped muzzle flash, sparkle bursts on hits, clean white tracers.
- **Unreal:** cel ramp in a post-process from the lighting/base-colour ratio, plus outline pass; textures simplified.
  Cost ≈0.6 ms.

## 3. Painted Frontier (`03_painted`, family "Hand-painted") — research D
**The idea:** chunky, sunny, hand-painted adventure: every surface looks brushed by hand with the light already
painted in: warm tops, cool undersides, highlighted edges, bold saturated colour. The friendliest, most readable
option (the Sea of Thieves / Overwatch family), but with a frontier palette of sun-baked reds, ochres and teal.
- **Materials:** `pbr` with the textures softened to painted shapes (mip ~2.5), saturation ~1.35, contrast up; a
  painted-light `hook`: up-facing surfaces lifted and warmed, down-facing darkened and cooled, a dark-to-light
  gradient up each object's height, convex edges highlighted (from normal/occlusion), world-space brush-sized hue
  blotches. High roughness, little specular, a warm rim.
- **Light:** late-morning warm sun (~40°), shadows tinted blue-violet, never black.
- **Sky:** a rich blue with big soft-edged painted clouds.
- **Post:** soft AO, gentle bloom, vibrance; no outlines.
- **Effects:** big chunky orange flashes, thick bright tracers, cartoon debris.
- **Unreal:** repaint the texture sets in the generator (bigger shapes, baked light), master adds world gradient,
  height gradient and rim. Cost ≈0.3 ms.

## 4. Inkslinger (`04_ink`, family "Graphic novel") — research E
**The idea:** a living graphic novel: heavy hand-inked contours that swell and thin, ink hatching that lives in the
shadows and crawls into the cracks, textured saturated colour. The looter-shooter's classic comic look pushed harder
and grittier, in rust, ochre and turquoise, clearly not a copy of anyone's.
- **Materials:** `toon` with three bands (light, mid, shadow) and hard edges, shadows tinted warm brown-red; textures
  kept (they become the "colourist's" texture), posterized lightly (~6 levels), saturation ~1.2. `hook`: world-space
  triplanar hatching in the shadow and mid bands (one direction in the mid tone, cross-hatching in deep shadow,
  ~5-8 cm spacing, slightly wobbly lines), ink grime in cavities from occlusion/normal detail.
- **Outlines:** thick ink (~3 px near, ~1 px far), near-black brown (#1a1210), depth + normal edges with inner crease
  lines, a little width jitter so it feels drawn.
- **Light:** strong afternoon sun, high contrast.
- **Sky:** a deep teal-to-ochre comic sky with inked, hatched cloud masses (hatching, not outlined flat shapes).
- **Effects:** jagged inked muzzle star with a white core, ink-lined spark streaks, a black-and-white hit flash frame.
- **Unreal:** post outline (Sobel) + world-space hatch textures in the materials driven by the shadow term. Cost
  ≈0.8 ms. Risk: closeness to an existing franchise's look: the hatching in world space, the palette and the line
  weight variation must make it ours.

## 5. Teropa Pulp (`05_pulp`, family "Retro sci-fi") — research H
**The idea:** the cover of a 1970s science-fiction paperback come alive: an enormous ringed gas giant fills the sky
over the frontier town, with two small moons; an airbrushed turquoise-to-tangerine sky; sienna and ochre ground;
chrome-bright highlights. One glance says "alien planet": the strongest hook for a weird west on Teropa, and nobody in
the western lane has it.
- **Sky (the star):** zenith deep teal, horizon tangerine-peach, airbrush-smooth. The gas giant ~30-40° across with
  its rings: soft banded stripes (cream, rust, teal), ring gaps, the rings' shadow across the planet, a soft
  atmospheric rim; placed so the `street` (looking east), `chapel` (south-east) and `boothill` (north-north-west)
  shots each see a big sky body: put the planet in the east-south-east at ~20-30° elevation, and a moon or the
  second moon to the north-west. Faint daylight stars near the zenith.
- **Materials:** `pbr`, textures smoothed a little (mip ~1.5, the airbrush feel), colours pushed toward the palette
  (sienna, ochre, teal), metal shinier (chrome highlights).
- **Light:** a tangerine sun at ~18°, with strong teal sky/planet fill: orange light against teal shadow everywhere
  (the HUD's own complementary pair).
- **Fog:** warm peach haze with strong aerial perspective: far ridges become flat layered teal and peach silhouettes.
- **Post:** bloom on the sun and highlights, fine grain, vignette, a grade with lifted teal blacks.
- **Unreal:** a sky sphere material with the planet layer, height fog with directional inscattering, a LUT. Cost
  ≈0.2 ms: the cheapest wow.

## 6. Sunbleached (`06_bleached`, family "Luminous") — research K
**The idea:** a high-key, luminous, mythic frontier: a blinding white-gold sun, sandstone that glows from within,
soft lavender shadows, sand that glitters as you move, rim light shimmering on every silhouette: the reverence of a
pilgrimage across a dead saint's land. Spiders read as dark shapes against the light.
- **Materials:** `pbr` with albedo lifted and warmed toward sandstone cream (desaturated a little), a view-dependent
  glitter `hook` on ground and rock (sparse sharp glints from high-frequency world noise), strong warm fresnel rim on
  everything, a warm bounce on the shadow side.
- **Light:** sun ~25°, intense white-gold, high exposure; shadows lavender (#9a8fc8-ish), soft.
- **Sky:** bleached: white-gold horizon to pale turquoise zenith, a huge soft sun glow, thin cirrus.
- **Fog:** luminous warm-white haze, heavy with distance.
- **Post:** big soft bloom, halation-like glow, god rays from the sun, low-contrast highlights, a touch of sharpen.
- **Effects:** white-gold sparks, glowing sand puffs, embers bright.
- **Unreal:** grade + bloom + light shafts, glints in the terrain material. Cost ≈0.4 ms.

## 7. Neon Frontier (`07_neon`, family "Neon dusk") — research I
**The idea:** Teropa after sundown as a synthwave western: a violet-to-magenta sky over a huge striped sun sinking
into the horizon, every silhouette traced in thin glowing cyan, hot neon in the windows and lanterns, slick
reflective ground, glowing haze. The HUD's cyan and orange become the world's light; maximum thumbnail pop.
- **Sky:** indigo zenith → magenta → hot orange horizon band, a big retro sun with horizontal stripes cut out of its
  lower half, low on the horizon in the `street` shot's view (east), stars and a faint grid-free glow.
- **Materials:** `pbr`, albedo darkened (~0.55 value) and cooled, ground and metal wetter (roughness x0.5) to catch
  reflections; glows and lanterns pushed to neon (boost x3, warmer orange and magenta).
- **Light:** a low magenta-orange key from the sun's side, a violet ambient, strong rim; the lanterns lit and boosted.
- **Fog:** violet haze with magenta sun scattering.
- **Post:** cyan glowing outlines (thin, ~1.5 px, fading with distance, bloomed), strong bloom, a little chromatic
  aberration, light grain, vignette.
- **Effects:** cyan-white tracers, magenta-orange muzzle flashes, neon-green spider hits.
- **Unreal:** outline post with emissive output into bloom, height fog, sky material. Cost ≈0.8 ms.

## 8. Ember Gothic (`08_gothic`, family "Dark weird west") — research C
**The idea:** the weird west as a ghost story: night under a huge pale moon, ground fog rolling between the graves
and down Main Street, a cold blue-green desaturated world, and the only warm things are lanterns, muzzle flashes and
the saints' cyan embers drifting in the air. The story's own mood (a revenant, a churchyard, a gravekeeper).
- **Sky:** deep blue-black, a huge pale moon (visible in the `boothill` and `chapel` shots), thin moonlit clouds,
  stars.
- **Light:** moonlight cool (#8fb0d8), ~30° elevation, low intensity; a very low dark-teal ambient; every lantern lit
  and boosted (warm pools of light); in `activate`, a warm lantern light at the player's hip (Pa's lantern) under
  `kit.styleRoot`.
- **Fog:** thick low ground fog (high height falloff), blue-grey, catching the moon and the lanterns.
- **Materials:** `pbr`, albedo desaturated (~0.5) and darker, wet sheen on the ground; embers and glows bright.
- **Post:** AO, bloom on lights, god rays from the moon, a grade with crushed teal shadows and lifted fog, strong
  vignette, grain. Readability: spiders get a faint cool rim so they read against the fog.
- **Particles:** many cyan embers, fog wisps.
- **Unreal:** two height fogs + fog cards (volumetric fog is too costly on Medium), moonlight + lantern lights, LUT.
  Cost ≈1.2 ms.

## 9. Celluloid West (`09_celluloid`, family "Film stock") — research B
**The idea:** the frontier shot on 1960s widescreen film: saturated dye reds and deep polarised sky blues, crushed
inky shadows, a red-orange halation glowing around every highlight, real grain dancing at 24 fps, a whisper of gate
weave: a spaghetti western you play. Grounded models, unmistakable image.
- **Light:** a hard high-afternoon sun (~35°), harsh contrast, short black shadows.
- **Sky:** a deep saturated blue with crisp white clouds, a dusty pale horizon.
- **Materials:** `pbr`, saturation ~1.15 with reds and ochres pushed, greens toward olive.
- **Post:** a film curve (crushed toe, soft shoulder) and dye-layer colour crosstalk (rich reds and blues, muted
  greens), halation (red-orange), animated 24 fps luminance-weighted grain, subtle gate weave, vignette, a hint of
  horizontal anamorphic streak on the brightest lights.
- **Effects:** hot white-yellow flashes, the hit flash as a single bright frame.
- **Unreal:** LUT + halation + grain post. Cost ≈0.4 ms.

## 10. Golden Hour (`10_golden`, family "Cinematic realism") — research A + P
**The idea:** grounded cinematic realism at its best, the "big studio" option: a low golden sun raking across the
valley, long blue shadows, dust hanging in the air lit like gold, deep layered atmosphere, sun shafts through the
dust, and a rich, saturated-but-natural grade (desert warmth with the saints' cyan as the only cool). It must look
clearly better than today's look (style 0): more light, more air, more drama.
- **Light:** sun at ~9° elevation, warm (#ffb36b), strong; shadows long and sky-blue tinted; GTAO strong.
- **Sky:** a physical-looking sky: warm glowing horizon around the sun, deepening blue zenith, high cirrus lit gold.
- **Fog:** warm height fog with strong sun inscattering (a glow toward the sun), aerial perspective making the ridges
  layer into the distance.
- **Materials:** `pbr` with full texture detail, normals a little stronger, roughness tuned per role.
- **Post:** AO, god rays through the dust, subtle bloom, a warm/teal split-tone grade with a gentle S-curve, fine
  grain, a little sharpen, soft vignette.
- **Particles:** golden dust motes, drifting.
- **Unreal:** SkyAtmosphere + height fog with inscattering + light shafts + LUT, which Medium affords. Cost ≈1.0 ms.
