# Art-style research for the Teropa looter shooter (2026-10-08)

Purpose: give the owner ten radically different art directions to choose from, with real references, the technical
recipe in real-time engines, why each wows (or fails) and how each sits under the fixed HUD (gunmetal metalwork, white
outlined floating text, orange #ff9f1c accents, cyan #5ac8ff lines, Chakra Petch).

## Read this first: how reliable this file is

- `WebFetch` failed on every host in this sandbox (`getaddrinfo ENOTFOUND`, including wikipedia.org, gdcvault.com,
  advances.realtimerendering.com, 80.lv). So I read no page or slide deck directly. Everything below comes from
  `WebSearch` result summaries, which quote the pages, and every URL is one that appeared in those result lists.
- Where a claim is from a page, the URL follows it. Where it is my own engineering judgement it is marked
  **(inference)**. GPU costs for the RX 580 are **estimates** (nobody publishes RX 580 numbers for these effects); they
  must be measured with `Tools\perf.ps1` before any style is chosen. The frame budget is 8.3 ms (120 fps), Medium,
  1080p, Lumen off, Nanite fallback.
- The two best primary sources to read in full when a browser is available:
  Patry, "Lighting, Atmosphere, and Tonemapping in Real-Time Samurai Cinema" (SIGGRAPH 2021,
  <https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html>, video
  <https://www.youtube.com/watch?v=GOee6lcEbWg>) and Bauer, "Creating the Atmospheric World of Red Dead
  Redemption 2" (SIGGRAPH 2019, <https://advances.realtimerendering.com/s2019/index.htm>).

## The brief in one paragraph (what every style must survive)

Teropa: a weird west on an alien frontier. Revenant gunslinger; saints' embers that glow cyan; giant spiders; a haunted
farm; chapel and churchyard; windmill; dry canyons; golden-hour ochre, olive and sandstone ground. Shipped HUD is
orange + cyan on gunmetal with no panels, so the world behind it must keep the corners calm enough for floating text,
and enemies/loot must still pop.

Two hue facts matter for every style below. (1) The HUD's orange is the golden-hour sun's colour and its cyan is the
saints' ember colour, so the HUD is already a complementary pair that the world can echo: warm key light, cyan
emissives. (2) Orange against cyan is extremely common in film (Mad Max: Fury Road, Blade Runner 2049, Tron: Legacy and Coco,
cited in sections P, I and M; AWN notes many recent films share a cyan/orange look:
<https://www.awn.com/print/blog/colorful-world-mad-max-fury-road>), so it is safe, but it is also what every
"cinematic" game does. The wow has to come from the *treatment* (line, texture, light behaviour, post), not from the
hue pair.

## Quick map: all 16 candidates (your 12 plus 4 I added), cartoon to real, and how each relates to the rejected list

| # | Style | Closest rejected look | Why it is not a repeat |
|---|---|---|---|
| M | Marigold and spirit-glow folk art (Coco, Guacamelee-type) | gouache storybook | Saturated emissive petals/embers as light sources, folk-art ornament, not painted texture |
| J | Claymation / miniature diorama | none | Photographed/handmade material look, macro depth of field |
| N | Chunky pixel-lit 3D (HD-2D light on low-res render) | flat-shaded graphic | Resolution-quantised image, palette-limited, bloom + tilt-shift |
| O | PS1/PS2 lo-fi "Y2K" (vertex snap, affine, dither) | none | Hardware-artefact look, very cheap |
| F | Anime cel (Genshin/BotW/Zenless) | flat-shaded graphic | Ramp-lit characters, painted-shadow, soft sky, not flat graphic |
| D | Chunky hand-painted stylised (Sea of Thieves/Overwatch/Valorant) | textured stylised realism | Painted-in lighting, bevel-rim, exaggerated shape language |
| H | 1970s pulp paperback / alien frontier (Foss) | none | Airbrush gradients, huge planet in the sky, striped hardware |
| I | Neon dusk (Tron/Blade Runner 2049/Blood Dragon) | none | Night, emissive-driven light, haze |
| C | Dark weird-west gothic (Hunt/Bloodborne/Darkest Dungeon palette) | none | Low-key, fog, ember glow as the key light |
| B | Leone Technicolor (Techniscope grain, halation) | oil-painted realism filter | Film-stock emulation, not paint |
| A | Grounded golden-hour cinematic realism (RDR2/Tsushima) | textured stylised realism | Physically-based sky, local tonemapping, atmospheric depth as the star |
| K | Luminous sun-bleached mythic (Journey/Sky) | gouache storybook | Glitter sand, rim shimmer, near-monochrome palette |
| P (added) | Hyper-saturated desert action (Mad Max: Fury Road grade) | none | Grade-built richness on neutral art |
| E / G / L | Ink-comic, painterly, woodcut | comic halftone / oil-paint / pen-and-ink | **Overlap risk**, see their sections for the "clearly different" execution |

Note E, G and L sit closest to what was already rejected. They are written up because you asked, with the
differentiated execution that would make them not-a-repeat, and with a warning. The shortlist and ratings are at the
end.

---

## A. Grounded golden-hour cinematic realism

**References**
- Ghost of Tsushima (and Ghost of Yōtei): the lighting/tonemapping talk and the art-direction interviews.
  Patry's SIGGRAPH 2021 slides list: one physically based sky model driving clouds, haze and fog particles; HDR
  lighting; indirect light computed at runtime from the dynamic sky; artist-adjustable deviation from physical
  correctness; a *local* tonemapping operator; a custom tonemapping colour space and white balance; and a Purkinje
  shift for low light (<https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html>; summary by a
  third party at <https://www.glowybits.com/blog/2022/12/18/ghost_talks/>; a secondhand note says the local tonemap
  mixes a bilateral grid with Gaussian blurs to avoid halos, see also
  <https://bartwronski.com/2022/02/28/exposure-fusion-local-tonemapping-for-real-time-rendering/>).
- Art director Jason Connell: "90% of the colour and mood and emotion comes from your upper hemisphere" (the sky);
  large single-species fields and forests rather than realism for its own sake; wind as a navigation device
  (<https://redbull.com/us-en/ghost-of-tsushima-art-director-interview-jason-connell>,
  <https://blog.playstation.com/?p=333111>).
- Ghost of Yōtei (2025): art director Joanna Wang calls light and wind "the interface", keeps a "living painting"
  look with a yellow motif (<https://mobilesyrup.com/2025/09/25/ghost-of-yotei-sucker-punch-joanna-wang-art-director-interview/>,
  <https://gamingbolt.com/ghost-of-yotei-art-director-explains-why-the-yellow-cover-is-important-to-story-and-themes>,
  <https://www.domusweb.it/en/news/2025/09/30/ghost-of-yotei-videogame.html>). Nominee for Best Art Direction at
  The Game Awards 2025 (<https://www.gematsu.com/2025/11/the-game-awards-2025-nominees-announced>).
- Red Dead Redemption 2: Bauer's SIGGRAPH 2019 abstract covers sky, cloud/fog rendering, volumetric effects and the
  ambient lighting model for sky-sourced indirect light. A RenderDoc study (unofficial) finds a sky cubemap built from
  a paraboloid sky map plus cloud textures, prefiltered with the split-sum approximation for image-based lighting,
  and a separate volumetric fog + light-shaft stage; sky/cloud compute work dominates the frame
  (<https://imgeself.github.io/posts/2020-06-19-graphics-study-rdr2/>). Players note the haze can wash colour out
  (<https://steamcommunity.com/app/1174180/discussions/0/1733258530996487565>).
- Hunt: Showdown's daylight look: "a very realistic base image with lots of nice gradients, value information and
  dynamic range", then grading to get "the bold golden look for the daylight setting"; volumetric fog where every light
  affects the fog; an early daylight pass was too saturated and "too tropical", and was reworked
  (<https://80.lv/articles/the-development-of-hunt-showdown>,
  <https://www.huntshowdown.com/news/creating-a-new-time-of-day-for-hunt-showdown>).

**Defining traits**
- Palette: warm key (amber-gold), cool sky-blue fill; earth tones graded to one family with one accent colour per
  region (Yōtei: each region gets its own seasonal palette).
- Value structure: three clear planes (dark foreground silhouette, mid-value subject, bright hazy backdrop); the sky
  carries most of the mood.
- Light: low sun (about 10 to 20 degrees, **inference** for "golden hour"), long soft-edged shadows, strong sky
  ambient, warm rim on characters; light shafts through dust.
- Atmosphere: height fog and aerial perspective do the depth work; dust in the air diffuses the sun.
- Post: filmic tonemapper with a gentle toe, slight bloom, subtle vignette, little or no grain.

**How it is built in real time**
- Tonemapper: UE's filmic defaults are slope 0.88, toe 0.55, shoulder 0.26, black clip 0.0, white clip 0.04; Epic's
  4.27 notes give the legacy look as 0.98 / 0.3 / 0.22 / 0 / 0.025 and say to set these project-wide, not per shot
  (<https://dev.epicgames.com/documentation/en-us/unreal-engine/color-grading-and-the-filmic-tonemapper-in-unreal-engine>,
  <https://docs.unrealengine.com/4.27/RenderingAndGraphics/PostProcessEffects/ColorGrading>).
- Sky-driven ambient (SkyLight real-time capture or a baked cubemap per time-of-day), exponential height fog with
  volumetric fog only on the hero views, a cloud layer (cheap 2D cloud texture on Medium, **inference**).
- Sea of Thieves shows the cheap route for clouds on a mid GPU: they skipped ray-marching for per-vertex lighting that
  approximates subsurface scattering, for artistic control and speed (summary at
  <https://blog.habrador.com/2018/08/stylized-graphics-fortnite-sea-of-thieves.html>, paper
  <https://history.siggraph.org/wp-content/uploads/2022/09/2018-Talks-Ang_The-Technical-Art-of-Sea-of-Thieves.pdf>).

**Why it wows / risks**
- Wow: the "photo mode" shot (sun low over the canyon, dust in the beam, chapel silhouette). Tsushima and RDR2 are the
  reference for "screenshots people share". Highest ceiling for a Steam trailer if the sky is good.
- Risk: this is the most expensive look to make *distinctive*; players compare it directly with RDR2. Haze that is too
  strong looks muddy (RDR2 complaint). Enemy readability depends on rim light and colour (spiders in brown sand need
  a warm rim or an emissive eye cluster). Performance: volumetric fog and clouds are the cost on an RX 580
  **(estimate: volumetric fog 1 to 2 ms at 1080p half-res; cloud layer under 0.5 ms if a texture)**.

**HUD fit**: very good. Calm gradients behind text; sunset orange rhymes with the HUD orange; cyan embers pop as the
only cool accent. Keep the sun low-left so the bottom-left player frame sits on shadowed ground.

**Teropa translation**: the "ochre/olive/sandstone" brief is exactly this style. To avoid "just realistic", commit to
one signature: a huge low sun disc with a halo, and cyan ember trails as the only saturated cool in the frame.

---

## B. 1960s spaghetti-western Technicolor film look

**References**
- Leone / Tonino Delli Colli: Techniscope (2-perf 35mm, 2.35:1) with Technicolor Italia; the format optically
  enlarges grain, and dye-transfer prints softened fine detail
  (<https://digitalbits.com/item/once-upon-a-time-in-the-west-2024-4k-uhd>,
  <https://www.moma.org/calendar/film/598> for the "rich, earthy palette").
- Shot language: wide landscape then extreme close-up of eyes, often in one move; harsh natural sun with long shadows
  outdoors, low-key light indoors; the grade leans burnt umber, bleached sky, dry highlights; airborne dust acts as
  diffusion; Techniscope's smaller negative gives a grittier texture
  (<https://colorculture.org/a-fistful-of-dollars-cinematography-analysis/>,
  <https://colorculture.org/for-a-few-dollars-more-cinematography-analysis/>,
  <https://melies.co/cinematic-techniques/genre-looks/spaghetti-western>,
  <https://www.rogerebert.com/scanners/opening-shots-the-good-the-bad-and-the-ugly>; sources disagree about lens
  choice, so do not rely on a single "Leone lens").
- Games that tried it: Red Dead Revolver (reviewers: "severely indebted to Leone", grainy film, "bleached out")
  (<https://www.gamesradar.com/uk/red-dead-revolver-review>, <https://www.techcentral.ie/red-dead-revolver/>,
  <https://www.gamingnexus.com/Article/509/Red-Dead-Revolver/>).
- Film-emulation tech for UE5: a commercial plugin runs the film stage in the post chain, colour before tonemap, with
  multi-radius halation (threshold, remjet, tint) and grain/damage scaled by film format
  (<https://forums.unrealengine.com/t/txc-stock-film-emulator-physical-based-film-emulation/2711083>).

**Defining traits**
- Palette: dusty ochre, burnt sienna, bleached-pale sky, deep brown shadows; one hot red or turquoise accent.
- Values: bold, contrasty; crushed but not dead shadows; highlights allowed to bloom.
- Light: noon-hard sun is the classic, but golden hour works; no fill, dust as diffusion.
- Post: 2.35:1 letterbox optional (see risks), grain, halation on bright edges, gate weave and dust specks (use very
  sparingly), mild vignette, slightly lifted blacks, a lens that feels long.

**How it is built**
- Order (practical synthesis): film-colour transform in linear HDR before tonemapping, halation (threshold bright
  areas, blur at two or three radii reusing the bloom mip chain, warm tint, add back), tonemap, then grain last
  (<https://forums.unrealengine.com/t/txc-stock-film-emulator-physical-based-film-emulation/2711083>; the
  imgix docs list a compose order of chromatic aberration, grain, bloom, halation, vignette, so order is a design
  choice: <https://docs.imgix.com/apis/video/vfx/video-vfx-halation>).
- Grain: animated, luminance-weighted (strongest in mid-tones), time-seeded hash, updated at a fixed cadence so it
  jumps between "exposures" instead of sliding; colour grain from three decorrelated hashes
  (<https://gamedev.net/shaderlab/57-time-varying-film-grain/>,
  <https://reshade.me/forum/shader-suggestions/915-martins-improved-film-grain-shader>). Cost is one fullscreen pass,
  no texture reads, about 0.2 to 0.4 ms at 1080p on an RX 580 **(estimate)**.
- Halation: bloom-mip-chain based, so nearly free if bloom is already on **(inference)**.

**Why it wows / risks**
- Wow: instantly "that's a western"; extreme close-up hero shots of the revenant's eyes with halation on the cyan
  ember reflection; a cohesive, borrowed-but-not-copied cinematic language that few games use. Strong trailer
  material.
- Risk: grain + halation soften enemy edges and can hurt readability at range; heavy grain also fights TAA/TSR and
  video compression on store trailers; a letterboxed frame wastes screen space for a shooter and fights the HUD
  corners (offer it as a photo-mode only). Copyright note: emulate the *look* of the stock and the lensing, never
  Morricone-like music or Leone shot recreations.

**HUD fit**: good. Grain sits under the HUD, and the HUD's crisp vector edges contrast pleasantly with the soft film
image. Keep HUD outside the grain pass (UI is drawn after post anyway).

**Teropa translation**: golden-hour ochre + Techniscope warm cast; cyan embers become the halation showpiece. The
halation tint is an adjustable parameter in film-emulation tools (the plugin above exposes a tint), so a warm
orange-red halo around the cyan glow is a design choice, not physics (**inference**).

---

## C. Dark weird-west gothic: dusk, moonlight, fog, glowing embers

**References**
- Hunt: Showdown: PC Gamer's first impression is moonlight diffused through fog and a hunter's silhouette threading
  gnarled trees; "stunning lighting and festering bayous are stifling"
  (<https://pcgamer.com/hunt-showdown-is-just-incredible>); fog as tension: audio hints at what is in the fog
  (<https://www.huntshowdown.com/news/point-blank-through-the-fog>); volumetric fog where every light affects it
  (<https://80.lv/articles/the-development-of-hunt-showdown>); the 1896 update advertises better "colour and tonal
  separation" in key areas (<https://press.crytek.com/crytek-brings-the-beloved-stillwater-bayou-map-back-to-hunt-showdown-1896-in-a-stunning-visual-and-gameplay-upgrade>).
- Bloodborne (fan breakdown by EA DICE's Simon Barle): gloss on materials keeps it from looking flat; cool base
  counterweighted by warm point lights; never let scenes sink to near-black because monitors vary; hand-placed point
  lights to lift silhouettes (<https://80.lv/articles/simon-barle-bloodborne-in-ue4>). Treat as fan analysis.
- Darkest Dungeon (Chris Bourassa): no pure black or dead grey in the palette (reserved for inking), warm palette
  with a faint yellow cast even in cool hues, heavy line work leaves room for unexpected colour spots
  (<https://www.gamespot.com:443/articles/the-gothic-sensibilities-of-darkest-dungeon/1100-6424880/>,
  <https://80.lv/articles/red-hook-studios-talks-about-the-creation-of-darkest-dungeon>).
- Weird West (WolfEye): hard outlines, brush-like colour fills, earthy palette, dark without being monochrome; a
  reviewer notes some scenes turn murky and characters are hard to make out
  (<https://www.digitaltrends.com/gaming/weird-west-preview/>, <https://thesixthaxis.com/2022/03/31/weird-west-review>,
  <https://waytoomany.games/2022/03/31/review-weird-west/>, <https://cogconnected.com/preview/weird-west-preview/>).
- Ghost of Tsushima's Purkinje shift (low-light vision simulation) is the principled way to make night cooler and
  less saturated (<https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html>).

**Defining traits**
- Palette: desaturated blue-green or violet-grey base, with warm lantern/campfire amber and the cyan embers as the
  only saturated lights. One accent per frame.
- Values: low-key, but a lifted floor (Barle's rule); readable silhouettes against pale fog.
- Light: moon as a cold directional key (steep angle for long, clean shadows, **inference**), embers as point lights
  that colour fog volumes.
- Fog: dense height fog, lit; ground mist in the churchyard; fog hides draw distance, which also saves GPU.
- Post: strong vignette, mild bloom on emissives, restrained grade, little grain.

**How it is built**
- Few dynamic lights with large soft radius plus emissive meshes (cheap on Polaris); fog cards and a cheap
  froxel-less exponential height fog with a tinted inscattering; light-shaft bloom; wet-gloss specular on stone.
- Embers: emissive material with additive sprites (Nanite cannot draw the additive glow, per CLAUDE.md: use emissive
  surfaces).

**Why it wows / risks**
- Wow: the cyan embers become a light source and the hero of every screenshot; cheapest route to "atmosphere"; fog
  hides low-poly content. This is the style where the *current* game assets (chapel, churchyard, hanging tree) look
  best.
- Risk: dark screenshots are the weakest on a Steam page (thumbnails are small; Steam-page guides warn that
  low-contrast and dark capsules blend into Steam's dark UI, see the Steam section at the end); brown/dark shooters
  are an overused trope (Gearbox set out to buck it with Borderlands:
  <https://cookandbecker.com/en/article/212/bucking-the-brown-shooter-trend.html>); spider readability in fog;
  accessibility (gamma).

**HUD fit**: excellent. Dark ground makes white outlined text and orange/cyan lines pop; the HUD's cyan will visually
match the world's embers.

**Teropa translation**: "night in Ransom's Rest": moonlit churchyard, cyan saint-embers drifting, a single warm window
in the farm. To keep the thumbnail strong, compose store shots at blue hour (dusk), not midnight.

---

## D. Chunky hand-painted stylised

**References**
- Sea of Thieves (Rare, UE4): GDC 2018 talk by art director Ryan Stevenson; rules reported: simplify structures, paint
  textures by hand rather than using photos, props look used with wear and repairs; water stays physically based
  under the stylisation (<https://80.lv/articles/gdc18-visual-adventures-on-sea-of-thieves/>,
  <https://blog.habrador.com/2018/08/stylized-graphics-fortnite-sea-of-thieves.html>,
  <https://gamedeveloper.com/art/video-how-rare-crafted-the-look-and-feel-of-i-sea-of-thieves-i->,
  <https://www.mcvuk.com/development-news/rares-sea-of-thieves-started-with-photorealistic-water/>); Shacknews Best
  Art Style 2018 (<https://shacknews.com/article/109011/shacknews-best-art-style-of-2018-sea-of-thieves>).
- Overwatch: handcrafted textures "to ensure the world felt made by people rather than simulated by computers"; painted
  bevels in textures create a comic-like rim light
  (<https://cookandbecker.com/en/article/378/designing-overwatch.html>,
  <https://80.lv/articles/comparing-team-fortress-2-and-overwatch-art-direction>,
  <https://news.blizzard.com/en-us/article/23189038/2>).
- Valorant: art lead Moby Francke calls it "illustrative visual design", a balance between realism and cel-shaded
  animation; "we're not trying to make the most beautiful game ever, we're trying to make an accessible game";
  legibility on any hardware (<https://inverse.com/gaming/valorant-art-style-interview-moby-francke>).
- Team Fortress 2 (the root of the lineage): illustrative rendering with rim highlights and luminance/hue shifts so
  characters read in any light; half-Lambert diffuse; Fresnel rim; subdued Phong for metals
  (<https://www.cs.princeton.edu/courses/archive/fall07/cos597B/papers/mitchell-team-fortress.pdf>,
  <https://cdn.steamstatic.com/apps/valve/2008/GDC2008_StylizationWithAPurpose_TF2.pdf>,
  <https://wiki.teamfortress.com/wiki/Illustrative_Rendering_in_Team_Fortress_2>); an Unreal implementation of the
  TF2/Riot wrap-falloff + rim approach exists at <https://github.com/Vaei/MobFort>.
- Fortnite's move to UE5.1 shows that stylised art can take Lumen/Nanite on top: roughly 300,000 polygons for a tree,
  real-time GI tinting characters (<https://www.techradar.com/news/fortnite-chapter-4-powered-by-unreal-engine-51-looks-truly-next-generation>).

**Defining traits**
- Palette: saturated but harmonised; warm lights/cool shadows with hue shifts (shadows go violet/teal, never grey).
- Shapes: exaggerated, simple, chunky; bevels painted into the texture; hand-painted wear, no photographic noise.
- Light: bright, high-key, soft shadows, strong sky ambient; rim light on everything.
- Post: light bloom, saturated grade, optional soft outline only on characters.

**How it is built**
- Albedo painted with baked-in AO/bevel highlights; low-frequency normal maps; half-Lambert/wrap lighting with
  hue-shifted shadow colour; Fresnel rim; vertex-colour gradients on large surfaces (ground, rock). All cheap: the
  whole look is in the materials, not in post. **RX 580 cost: lowest of any shipped-quality style (estimate: no
  extra post passes).**

**Why it wows / risks**
- Wow: "looks expensive but friendly"; great readability; ages well; proven on Steam (Valorant, Overwatch, Sea of
  Thieves). Screenshot-friendly because every prop has clear colour blocks.
- Risk: "I've seen this" (Fortnite/Overwatch lineage); cartoon tone fights a haunted/revenant story unless the
  palette and shape language lean spooky (curved, elongated, "Tim Burton" shapes), and risks being called "mobile
  game" if the textures are too flat. Valorant itself was called "cheap-looking, cartoony and bland" by some critics.

**HUD fit**: good. Clean colour blocks keep the screen readable; the metal-and-outline HUD reads as the "premium"
layer.

**Teropa translation**: sunbaked storybook wild west with cyan-glowing saints; spiders get bold silhouette colours.

---

## E. Thick-ink comic cel with hand-inked textures (closest to rejected "comic halftone" and "ink & flat colour")

**References**
- Borderlands (2009): not cel shading. Hand-inked textures scanned and coloured in Photoshop, plus software that adds
  graphic-novel outlines and sharper shadows; artists were trained as a group with style guides; the post process only
  adds the black line (<https://80.lv/articles/how-borderlands-devs-created-franchise-s-signature-art-style>,
  <https://gamespot.com/articles/behind-borderlands-11th-hour-style-change/1100-6253257/>,
  <https://www.cgw.com/Publications/CGW/2009/Volume-32-Issue-11-Nov-2009-/A-Fine-Line.aspx>,
  <https://destructoid.com/?p=44099>). The style was a late (three-quarters through) pivot from a realistic look.
- Borderlands 4 (2025, UE5): "dynamic inking" lets far denser foliage; art director Adam May: it is "inked", more
  like a graphic novel than cel shading (<https://cogconnected.com/feature/borderlands-4s-adam-may/>,
  <https://www.cgmagonline.com/interviews/borderlands-4-adam-may/>). Reviews praise visuals, criticise performance
  (<https://app.cohorted.co.uk/news/borderlands-4-what-the-reviews>).
- Hi-Fi Rush (UE4, 60 fps): GDC 2024 "3D Toon Rendering in Hi-Fi Rush": comic shaders, toon lights, dynamic and static
  shadow-map strategies, light-probe GI, face shadow solution; stylisation done in a deferred post stage; N·L compared
  against a threshold with a step function for crisp shadows; faces use a threshold-map texture because normal
  tweaking broke under rotation (<https://gdcvault.com/play/1034251/3D-Toon-Rendering-in-Hi>,
  <https://80.lv/articles/the-making-of-hi-fi-rush-s-3d-toon-rendering-style>,
  <https://www.gcores.com/articles/181912>, Futurama as the style anchor:
  <https://www.unrealengine.com/developer-interviews/hi-fi-rush-was-inspired-by-shaun-of-the-dead-and-futurama>).
- Wild Bastards (2024, space western, Blue Manchu): the linework is modelled as geometry, not texture; a dynamic sun
  drives it and the inverted line work turns to colour where it falls in shade; "we wish we had a term"
  (<https://aftermath.site/wild-bastards-blue-manchu-art.md>, <https://www.pcgamesn.com/wild-bastards/review>,
  <https://steamdeckhq.com/?p=19434>).
- Call of Juarez: Gunslinger frames its cel-shaded look as a dime-novel illustration with a screen border
  (<https://imfdb.org/wiki/Call_of_Juarez:_Gunslinger>, <https://www.thesixthaxis.com/2013/04/22/hands-on-call-of-juarez-gunslinger/>,
  <https://www.gamereactor.eu/call-of-juarez-gunslinger-review/>).
- Weird West (see C): brush-like fills and hard outlines.

**How it is built**
- Outlines three ways in UE: (1) post-process edge detect on depth/normal (Sobel), (2) Custom Depth + Stencil index
  post process (up to 255 indices, one pass; the Tom Looman multi-colour material is about 144 instructions vs 97 for
  single colour in its UE4-era article: <https://tomlooman.com/unreal-engine-outline-multi-color-post-process/>,
  <https://300mind.studio/blog/outline-effects-in-unreal-engine-5/>), (3) inverted hull or geometry lines
  (per-mesh, no screen pass, but doubles draw cost for lined meshes). Wild Bastards' "lines turn to colour in shade"
  can be done by tinting hull colour with N·L **(inference)**.
- Textures: hand-inked hatching baked into albedo (Borderlands) is the trick that separates it from a toon-shader
  look: line density encodes shading and survives any lighting, so it is Medium-friendly.

**To avoid being a repeat of the rejected "comic halftone / ink & flat colour"**: drop halftone dots and flat fills.
Use full-colour painted textures with ink *only on contours and creases*, dynamic sun, shade-turns-to-colour lines
(Wild Bastards), and chunky shapes. Do not build the whole look from hatching alone.

**Why it wows / risks**
- Wow: unmistakable thumbnail identity; ages well (Borderlands is 17 years old and still reads); outlines make
  enemies and loot pop. Hi-Fi Rush and Dispatch show how well "premium TV animation" sells
  (<https://www.destructoid.com/dispatch-developer-interview/>, <https://www.videogameschronicle.com/reviews/dispatch-review/>).
- Risk: "Borderlands clone" perception, and it is *also* the nearest to what the owner already rejected. Outline
  weight must be tuned per resolution; line "crawl" in motion. Cost is the outline pass (estimate: 0.3 to 0.8 ms for
  depth+normal Sobel at 1080p on RX 580).

**HUD fit**: very good: the HUD's metal frame and outlined text share the "ink" language. Risk of two competing line
weights; keep world outlines at most 2 px at 1080p so HUD strokes remain the heaviest.

---

## F. Anime cel-shaded

**References**
- Genshin Impact: community reverse-engineering of the character shader: soft N·L with a smoothing window of about
  0.1, a second offset N·L pass with a harder edge for a thin secondary shadow band, shadows under arms/necks/folds
  painted into textures, packed light-map channels (R specular/metallic, G hand-painted shadow, B specular threshold,
  A material ID), normal-map blue channel used as a distance-thickened detail line, inverted-hull outlines with
  averaged normals baked into vertex colours to avoid scaling artefacts
  (<https://github.com/Melioli/HoyoToon/wiki/Using-the-Genshin-Shader>,
  <https://github.com/NoiRC256/URPSimpleGenshinShaders>, <https://godotshaders.com/shader/toon-shader-inspired-of-genshin-impact/>,
  <https://bjayers.artstation.com/blog/category/5>). miHoYo's own slides (Unity Japan, PBR-based stylised rendering)
  are at <https://docswell.com/s/UnityJapan/KWRPQ5-210617-unity-dojo20211mihoyozhenzhongyi>. These are reverse-
  engineered, not official.
- Breath of the Wild (GDC 2017, Takizawa): HD mockups from earlier Zelda models; Wind Waker's look was too tied to its
  cartoon style for a physics-and-chemistry world; the result is painterly, inspired by gouache and plein-air art
  (<https://zeldauniverse.net/2017/03/14/takizawa-explains-how-the-wind-waker-hd-inspired-the-visual-style-for-breath-of-the-wild/>,
  <https://www.thumbsticks.com/gdc17-designing-zelda-breath-of-the-wild/>,
  <https://nintendoeverything.com/art-director-on-why-zeldas-art-style-constantly-changes-and-how-breath-of-the-wilds-look-was-made/>).
  A video analysis says cel shading runs in a deferred step with per-pixel material IDs and that terrain is not cel
  shaded while characters are (secondhand: <https://setsideb.com/?p=3303>).
- Wuthering Waves (Kuro, UE4.26 with ported UE5 features): deferred rendering for SSR/AO on mobile; an independent
  character lighting pipeline; gradients in base colour and mask textures; UE4 lighting, fake volume fog and
  post-materials for the gradient effect; custom LUT colour grading per scene with a stable lighting baseline
  (<https://www.unrealengine.com/developer-interviews/exploring-the-post-apocalyptic-charm-of-asg-open-worlds-in-wuthering-waves>,
  <https://automaton-media.com/en/column/even-from-a-developers-perspective-wuthering-waves-use-of-unreal-engine-is-borderline-perverse-a-ue4-game-decked-out-in-custom-technology/>,
  <https://forums.unrealengine.com/t/deconstructing-replicating-wuthering-waves-post-process-anime-shader-model-in-ue5/2739985>).
- UE toon recipes: global (not per-mesh) toon in UE5 via post-process materials
  (<https://medium.com/@little_michael101/building-a-toon-shader-in-unreal-engine-5-globally-not-per-mesh-part-1-6ef9aade3380>).
  Whether UE 5.8's Substrate has a ready-made toon BSDF is **unverified**; check the engine docs before relying on it.

**Defining traits**
- Palette: pastel-to-vivid with *colour-shifted shadows* (warm light, cool coloured shadow), clean gradients, soft
  skies with painted clouds.
- Values: 2-3 tone ramps on characters; environments are softer (BotW: terrain not toon-shaded).
- Light: soft rim, bloom on highlights, bright haze; shadow edges either hard (toon) or smoothed 0.1 window.
- Outline: thin, coloured (darkened albedo, not black), thicker on characters than environment.

**Why it wows / risks**
- Wow: a very large audience for the look (Genshin Impact, Wuthering Waves, Zenless Zone Zero; the size of that
  audience is my inference, I did not find a source with numbers); instantly beautiful in screenshots; characters
  sell the game; great sky and foliage motion (BotW wind).
- Risk: it needs *character art* to match (enemies like giant spiders must be redesigned with anime-style shape
  language); the cost of per-character painted shadow maps; the "gacha" association; low-poly environment surfaces
  look flat unless gradient-painted. Performance: ramp lighting is cheap per pixel, outline pass about 0.3 to 0.8 ms
  **(estimate)**; Hi-Fi Rush runs its toon pipeline at 60 fps on a customised UE4 (GDC 2024 summary), so 120 fps on
  Medium is plausible only if dynamic shadows are limited (**inference**).

**HUD fit**: good. Anime UI often sits on pastel scenes; here the gunmetal HUD adds contrast and keeps the western
tone. Cyan embers become anime-style magical particles.

---

## G. Painterly brushwork in motion (overlaps rejected "gouache storybook" and "oil-painted realism filter")

**Warning**: the previous oil-painted realism filter (Kuwahara-type) is already rejected. The only versions of "G" that
are not a repeat are those that put the paint in the *textures and light*, not in a full-screen filter. Details:

**References**
- Arcane (Fortiche): hand-painted textures with painted light and shadow rather than shader-computed; no toon shader,
  no sharp shadow edges; painted backgrounds and 2D effects; fan rebuilds use baked light/curvature/AO as a base with
  slope blur and then hand-painted gradients and strokes (square alpha brush at reduced opacity), and pass-based
  compositing for character renders
  (<https://redsharknews.com/why-netflixs-arcane-looks-so-good-how-fortiche-ramped-up-the-animation-pipeline>,
  <https://80.lv/articles/a-closer-look-at-texturing-in-arcane-part-2>,
  <https://80.lv/articles/3d-artist-recreates-arcane-s-ekko-with-substance-3d-blender>). Fortiche's own tool chain is
  not documented in the sources I could reach.
- Dishonored 2 (Arkane): textures hand-painted in Photoshop, no complex procedural pipeline; art direction wanted an
  18th-century-painting feel and reduced visual noise; Deathloop uses a master-material procedural base instead
  (<https://80.lv/articles/asset-material-production-in-dishonored-2/>,
  <https://www.adobe.com/products/substance3d/magazine/deathloops-award-winning-art-pipeline-with-substance.html>,
  <https://www.cookandbecker.com/en/article/281/the-art-of-deathloop.html>). I found no source that says either uses
  a runtime painterly filter.
- Disco Elysium: painters first, fixed isometric camera so each screen is composed like a canvas; hand-painted,
  dynamically lit backgrounds; real-time only for animated/interactive items (Unity); the fixed camera is the enabler
  (<https://gdconf.com/news/come-gdc-and-see-how-disco-elysiums-unique-style-was-achieved-0>,
  <https://rpgcodex.net/article.php?id=12760>). Not portable to a free-camera shooter.
- Clair Obscur: Expedition 33 (2025, UE5, Game of the Year and Best Art Direction at The Game Awards 2025): Belle
  Époque dark fantasy; "pictorial art is central" (title, creatures, architecture); small team (about 30); Lumen and
  Nanite; the Lumen switch undid hand-tuned lighting and locations were retuned; a small library of custom smart
  materials (obsidian, black marble, gold) forms the backbone
  (<https://www.shacknews.com/article/147150/clair-obscur-expedition-33-game-awards-win>,
  <https://www.musee-orsay.fr/en/program/whats-on/exhibitions/clair-obscur-expedition-33>,
  <https://creativebloq.com/entertainment/gaming/clair-obscur-expedition-33-review-a-brilliantly-weird-utterly-unique-game>,
  <https://80.lv/articles/clair-obscur-expedition-33-dev-shares-how-ue5-shaped-the-game>,
  <https://www.unrealengine.com/developer-interviews/inside-the-development-journey-of-clair-obscur-expedition-33>,
  <https://blog.adobe.com/en/publish/2025/10/16/how-substance-3d-powered-art-clair-obscur-expedition-33>).
- Kuwahara references (for the *rejected* filter route, and for cost): Kyprianidis et al. 2009, GPU real-time video
  (<https://umsl.edu/~kangh/Papers/kang_cgf09.pdf>); polynomial weights computed inline instead of via a lookup
  texture (<https://www.umsl.edu/~kangh/Papers/kang-tpcg2010.pdf>); UE tutorial and assets
  (<https://www.kodeco.com/100-unreal-engine-4-paint-filter-tutorial>,
  <https://yuna-ink.itch.io/kurahawa-post-process-effect-ue5>, <https://github.com/noxtgm/kuwahara-filter>); pass
  structure for the anisotropic version is Sobel structure tensor, Gaussian blur, then the oriented kernel. A radius
  of 4 to 6 on an RX 580 at 1080p is likely 2 to 5 ms **(estimate; it can eat a third or more of the 8.3 ms
  budget)**, so a full-screen Kuwahara is a poor fit for the 120 fps Medium target.

**Not-a-repeat execution (recommended if G is chosen)**: "painted light": hand-painted/gradient albedo with painted
shadow and highlight colour baked in, soft shadows, brush-stroke noise in the *albedo and normal* only, 2D painted
effects (smoke, dust) as sprite sheets, painted sky dome. No Kuwahara. This is Arcane/Dishonored 2's route and it is
nearly free at runtime.

**Why it wows / risks**
- Wow: unique per-frame beauty and critical praise (Clair Obscur's sweep). "Screenshot looks like a painting".
- Risk: highest art labour (every asset hand-painted); motion shimmer if brush noise is screen-space; overlap with the
  rejected looks; painterly low-contrast scenes hurt enemy readability.

**HUD fit**: good if values are calm; painterly noise behind small HUD text can reduce legibility, so keep HUD
corners low-frequency (sky and ground gradients).

---

## H. 1970s pulp sci-fi paperback / alien frontier

**References**
- Chris Foss: brush + airbrush; bulky, brightly coloured, ornately detailed, "almost Gothic" ships in contrast to grey
  streamlined ones; stripes and numbers; huge machines over landscapes with tiny or absent humans; backgrounds with
  vivid greens, yellows and reds, not just black/blue (<https://sf-encyclopedia.com/entry/foss_chris>,
  <https://www.sffworld.com/2011/07/bookreview736/>); inspired Homeworld's artists
  (<https://kotaku.com/the-art-of-homeworld-enjoy-5887904>); a tutorial to recreate the colour of Foss, John Berkey
  and Peter Elson (<https://www.creativebloq.com/features/how-to-paint-explosive-environments>).
- No Man's Sky: "look like a science fiction book cover come to life"; art director Grant Duncan cites Foss, John
  Harris, Moebius and Ralph McQuarrie; Foss painted when most art used black starfields and dull grey ships
  (<https://www.nomanssky.com/2016/04/art-of-no-mans-sky/>,
  <https://en.wikipedia.org/wiki/Development_of_No_Man%27s_Sky>,
  <https://www.pushsquare.com/news/2016/07/these_alternate_no_mans_sky_ps4_covers_look_just_like_classic_sci-fi_books>).
- Outer Wilds: art test began as plein-air explorer painting (Bierstadt), later switched to a hard-edged, sun-lit
  style because the painterly look did not read for jetpack movement; crisp edges separate floors from walls and the
  rocks respond to light; blends "camping in space" naturalism with 1960s-70s NASA imagery
  (<https://80.lv/articles/dissecting-the-art-style-of-outer-wilds/>,
  <https://www.mobiusdigitalgames.com/news/changing-the-outer-wilds-art-style>,
  <https://www.mobiusdigitalgames.com/blog/our-new-style>).
- Starfield's "NASA-punk": retro-analogue, worn, lived-in ("romance of the golden age of early spaceflight"), but
  criticised for a barren, bland result (<https://www.pcgamesn.com/starfield/nasa-punk>,
  <https://www.pcgamer.com/starfields-vision-of-the-final-frontier-makes-space-seem-like-a-boring-barren-expanse>).
- Period art: NASA Ames space-settlement paintings (Rick Guidice), 1970s sci-fi paperback art overview
  (<https://anothermag.com/art-photography/7711/1970s-nasa-paintings>,
  <https://www.sciencefriday.com/articles/this-70s-artist-painted-our-future-in-space/>,
  <https://bookpage.com/reviews/worlds-beyond-time-adam-rowe-book-review/>).
- ARC Raiders (Embark, UE5, 2025): retro-future, post-apocalypse; late-1970s/80s fashion and Cold War utilitarian
  tech; nature breaking through brutalist structures; praised art direction and a stable UE5 implementation
  (<https://www.purexbox.com/features/opinion-arc-raiders-is-one-of-the-most-visually-impressive-multiplayer-games-ive-ever-played>,
  <https://opencritic.com/game/19023/-/reviews?page=2>).

**Defining traits**
- Palette: airbrushed gradients; saturated complementary pairs (orange sand against teal-green sky, magenta/violet
  shadows); a planet with rings taking a third of the sky.
- Shapes: huge hardware with stripes, panel lines, hazard marks; props shown at scale against tiny humans.
- Light: single hard sun plus gradient sky; rim from planet-shine.
- Texture: smooth gradients with crisp panel lines, not photographic noise.

**How it is built**
- Gradient sky dome with a ring-planet billboard and a second sun/moon; large-scale colour gradients applied by
  world-height and distance (cheap fog colour ramp); hard-edged geometry with decal stripes; minimal post (soft
  bloom, slight chromatic aberration).
- Planet/ring: a prerendered texture quad with a ring-shadow decal on the ground (cheap).

**Why it wows / risks**
- Wow: the biggest *concept* hook for "alien frontier": a sky with a gas giant is an instant store-page screenshot;
  retro-futurism with orange sands and cyan ember tech is a genuinely less-used pairing than gothic or neon; the
  palette allows saturated cheerful colour without being cartoonish.
- Risk: gradients can look flat without strong lighting; "NASA-punk" shows how easily it becomes bland; asset work
  to add stripes/panel lines; the spider enemies need fitting into the pulp bestiary (Foss's biological work is
  less documented in my sources).

**HUD fit**: strong. Retro-futurist type and instrument-panel language match the HUD's gunmetal; Chakra Petch already
reads as sci-fi.

**Teropa translation**: Teropa's ringed planet in the sky is the hero; windmill and farm become "pioneer technology"
under it. Spiders could become Foss-like insectoid hardware-organics.

---

## I. Neon dusk / synthwave western

**References**
- Cyberpunk 2077: neon yellow as brand colour; the lead environment artist compared the approach to a famous soda
  bottle; the intent was to escape the genre's red/blue pairing
  (<https://adrianlungu.substack.com/p/cyberpunk-2077s-visual-style-crafting>); grade has a large effect on
  perception (many ReShade grades).
- Far Cry 3: Blood Dragon: neon + permanent night, neon sources doing most illumination; saturated hues can disorient
  in fast combat; outdoor scenes hard to read (<https://bit-tech.net/reviews/gaming/far-cry-3-blood-dragon/1/>,
  <https://www.denofgeek.com/?p=123568>).
- Tron: Legacy: a two-hue world, muted metallic blue and thick orange; faces nearly monochrome blue
  (<https://www.pushing-pixels.org/2010/12/28/the-colors-of-tron-legacy.html>).
- Blade Runner 2049: Vegas wrapped in orange haze (reference: a 2009 Australian dust storm), haze as a diffuser that
  keeps the image soft but real, smoke to direct the eye (<https://www.studiobinder.com/blog/blade-runner-2049-cinematography-analysis/>,
  <https://www.premiumbeat.com/blog/symmetry-color-cinematography-blade-runner>,
  <https://colorculture.org/blade-runner-2049-cinematography-analysis/>).
- Marathon (Bungie, 2026): bold graphic design, "make weird cool", Designers Republic-like type and shapes, praised and
  polarising ("Roblox-style cubes with neon highlights" on first look); also an art-theft controversy in 2025 that
  Bungie admitted and resolved by crediting the artist (<https://80.lv/articles/marathon-art-director-shares-the-team-s-creative-influences>,
  <https://www.gfinityesports.com/article/why-marathons-art-direction-has-me-ready-to-dive-into-bungies-upcoming-shooter>,
  <https://buttondown.com/reframe/archive/bury-my-shell-on-tau-ceti-iv/>,
  <https://www.videogameschronicle.com/news/the-artist-whose-work-was-used-in-marathon-without-permission-has-been-credited-as-visual-design-consultant>).
  Lesson for this project: originality matters (CLAUDE.md rule 1), do not copy another studio's graphic identity.

**Defining traits**
- Palette: deep indigo/purple dusk with magenta-orange horizon band; emissive cyan, magenta and orange accents; the
  world is dark and *light is the content*.
- Light: lit by emissives and a low sun slice; wet-looking reflective ground (SSR or planar cheat).
- Post: strong bloom, chromatic aberration at edges, vignette, scanline/film grain optional.

**How it is built**
- Emissive-driven look: bloom threshold tuned so only emissives bloom; coloured exponential fog; sky gradient with a
  big sun disc; cheap reflections (cubemap + roughness) on ground. Cost mostly bloom: about 0.5 to 1 ms
  **(estimate)**.

**Why it wows / risks**
- Wow: emissive cyan embers *are* the look; thumbnails with dark purple and glowing cyan are eye-catching; the
  Steam market has proven demand for neon-dusk.
- Risk: saturation fatigue and combat readability (Blood Dragon's headaches); "synthwave western" may feel off-
  brand against the haunted farm and chapel unless the neon is organic (ember-based) rather than electric signage.

**HUD fit**: excellent. Orange + cyan HUD is basically the world's palette. Risk: the HUD can disappear into
equally saturated backgrounds; keep a darker ground plane in the HUD corners.

---

## J. Claymation / miniature diorama

**References**
- Harold Halibut (Slow Bros, Unity, not Unreal): everything handmade then scanned; 200 to 500 photos per asset for
  photogrammetry; characters scanned on a turntable, retopologised and rigged; sets scanned in parts and re-lit in 3D;
  motion capture edited to imitate stop-motion; compared with early Aardman and Wes Anderson/LucasArts
  (<https://www.creativebloq.com/features/making-harold-halibut>, <https://inverse.com/gaming/harold-halibut-interview>,
  <https://skwigly.co.uk/harold-halibut-slow-bros-ole-tillmann-tactile-gaming>,
  <https://en.wikipedia.org/wiki/Harold_Halibut>).
- Tilt-shift "miniature faking": shallow depth of field plus high camera angle, boosted saturation and contrast to
  mimic bright model paint; a student project blurred more the further from a focus line and raised saturation by 50%
  (<https://en.wikipedia.org/wiki/Miniature_faking>). I found no source claiming Tiny Glade or Lumberjack's Dynasty
  use tilt-shift, so do not cite them as examples.
- HD-2D uses tilt-shift/DoF the same way (see N).

**Defining traits**
- Palette: warm, slightly desaturated clay pigments with visible fingerprints and plasticine sheen; felt, wire,
  painted cardboard backdrops.
- Light: small-set studio light: a hard key, soft fill, visible shadow on a backdrop; stop-motion 12 fps animation
  feel on enemies and props only (not the camera).
- Post: macro DoF with a bokeh blur, slight film grain, saturation boost, no outline.

**How it is built**
- Photogrammetry-like surface detail baked from handmade clay sculpts (the game would need Blender sculpt + bake
  pipelines, which the Art folder already supports), subsurface-wrap shading for clay, thumbprint normal noise,
  "boiling" low-frequency vertex jitter on animated props at 12 fps, DoF.
- In first person, tilt-shift cannot be a high-angle cheat; use near/far DoF with a focus plane at the weapon
  distance **(inference)**. Cost: depth of field about 0.5 to 1.5 ms **(estimate)**.

**Why it wows / risks**
- Wow: nobody expects a clay revenant western; strong "handmade" store-page identity (Harold Halibut earned press for
  exactly this). Giant clay spiders and a plasticine sun are screenshot gold.
- Risk: very high art cost (every model needs clay-feel textures); animation that imitates stop-motion hurts
  shooter responsiveness if applied to the player; first-person tilt-shift does not work; grain and DoF blur
  reduce readability at range.

**HUD fit**: surprising but good: the glossy-metal HUD contrasts with the matte clay world, and orange/cyan read as
plastic-bright accents.

---

## K. Luminous sun-bleached mythic (Journey/Sky/Abzu)

**References**
- Journey: John Edwards' GDC 2013 "Sand Rendering in Journey": non-physical "ocean" specular and a
  "phenomenological" diffuse model; reconstructions list sharpened mips, anisotropic masking, glitter specular, ocean
  specular, diffuse contrast and a detail heightmap; a subtle shimmer on dune edges separates dunes in a limited
  palette; the desert was chosen to let players connect on a human level
  (<https://gdcvault.com/play/1017742/Sand-Rendering-in>, <https://www.alanzucconi.com/2019/10/08/journey-sand-shader-3/>,
  <https://polycount.com/discussion/comment/2621156>,
  <https://gameranx.com/updates/id/13724/article/journey-s-desert-setting-was-chosen-to-help-players-connect-on-a-human-level/>).
- Sky: Children of the Light: light is used both visually and thematically; warmth as a reward in cool, desaturated
  areas (a rain forest where warmth comes only from other players and sanctuaries); won the Webby People's Voice for
  Best Art Direction 2021 (<https://80.lv/articles/interview-a-deep-dive-into-the-art-of-sky-children-of-the-light-with-thatgamecompany>,
  <https://80.lv/articles/how-to-design-emotional-game-environments-for-sky-children-of-the-light/>,
  <https://gdcvault.com/play/1026812/Art-of-Sky-Children-of>,
  <https://winners.webbyawards.com/2021/games/features/best-art-direction/173889/sky-children-of-the-light>).
- Mad Max / Dune cross-reference for sun-bleached desert grading: see P.

**Defining traits**
- Palette: near-monochrome sand and sky gradient (one hue family), one saturated accent (a red scarf; here, cyan
  embers).
- Values: very high-key, soft contrast, strong silhouettes; glitter sparkles on the ground.
- Light: low sun with strong back- and rim light; fog as glow; shadow colour is a deep warm teal, not grey.
- Post: bloom, soft glow, very clean (no grain).

**How it is built**
- Glitter specular from a noise normal blended onto the base normal; rim shimmer on edges; soft analytic shadows;
  gradient skybox; a single highly saturated emissive accent. Almost free on the GPU (no heavy post).

**Why it wows / risks**
- Wow: unique silhouette-driven imagery; a shot of the revenant on a dune ridge with cyan embers in the wind is a
  poster. Emotionally distinctive among shooters.
- Risk: it is a contemplative look; enemies must be dark silhouettes against a bright field (good for readability,
  actually) but near-monochrome art risks monotony over 20 hours; mythic tone may clash with gritty gunplay.

**HUD fit**: good: a bright field needs the HUD's outlined text (already outlined) and darker HUD metalwork contrast.
Orange is already near the world palette so the HUD may blend; lean on the cyan.

---

## L. Dime-novel engraving / woodcut with colour (overlaps rejected "pen & ink crosshatch")

**References**
- Return of the Obra Dinn: 1-bit; the dithering is a custom blue-noise/Bayer mix; flicker under camera motion was
  fixed by mapping the dither onto a sphere around the camera (image stable under rotation), render raised from
  640x360 to 800x450; crisp outlines from edge detection; legibility comes from outlines and careful dithering
  (<https://blog.playstation.com/archive/2019/10/17/lucas-pope-on-return-of-the-obra-dinns-art-style>,
  <https://www.alanzucconi.com/?p=10365>, <https://killscreen.com/return-obra-dinn-update-details-challenges-1-bit-rendering>,
  <https://dukope.itch.io/mars-after-midnight/devlog/285964/working-in-one-bit>, devlog
  <https://forums.tigsource.com/index.php?topic=40832.msg1363742#msg1363742>).
- Pentiment: medieval manuscript illumination fused with German woodcut (Nuremberg Chronicle as a main reference),
  coloured; ink behaving like real ink on parchment, typography signals class; no shader breakdown found
  (<https://www.gamedeveloper.com/art/deep-dive-the-art-of-pentiment>,
  <https://www.inverse.com/gaming/pentiment-game-art-interview>,
  <https://www.gamereactor.eu/the-history-behind-pentiment-and-the-creation-of-its-artistic-style-1191273/>).
- Dime novels as a western source: Call of Juarez: Gunslinger frames its game as a dime-novel illustration
  (<https://imfdb.org/wiki/Call_of_Juarez:_Gunslinger>), a theme that makes an engraved-print look historically
  apt.
- Generic woodcut-shader lead: luminance-varied line widths with a paper/ink separator (Maya/Arnold tutorial) and a
  Lambert + ramp + edge-lines Blender ink shader (<https://blendernpr.org/?p=2104>); unrelated to Pentiment.

**Not-a-repeat execution**: the rejected look was a pen-and-ink crosshatch. A "chromolithograph / dime-novel cover"
look differs: flat mis-registered colour plates (3 to 4 spot inks), halftone-free, coarse paper grain, a few engraved
hatching lines only for form shadows, bold title-block style lettering for pickups. Dithered Obra-Dinn-style stipple
over colour is another differentiator. Still, it is the style most likely to be called "we already saw this".

**Why it wows / risks**
- Wow: Obra Dinn won over many players with *one* gimmick; the printed-paper idea ties to the dime-novel western
  trope and to lore (the revenant as a dime-novel hero).
- Risk: dither/hatch shimmer in motion (needs Pope's sphere-mapping fix), eye strain over long sessions, readability
  of small enemies, high effort on gradients; 120 fps friendly (cheap post).

**HUD fit**: the HUD's no-panels outlined text would blend with engraved lines; put a slightly darker, flat paper
tone near HUD corners.

---

## M (added). Marigold and spirit-glow folk art: the Coco/Día-de-Muertos palette

**Why I added it**: it is the one reference whose key colours are literally the HUD's: marigold orange (#ff9f1c is
marigold) against spirit glow (cyan). The saints' embers are already "spirits". It is a culturally rich, instantly
recognisable palette that no mainstream shooter owns. Handle with care: build on the *folk-art language* (papel
picado, retablos, marigold petals, calaveras) with original designs and respect, not copied characters.

**References**
- Coco (Pixar): marigold bridge; petals as light sources (a point-cloud light type for many points; footsteps stir
  glowing petals); Guanajuato-inspired vertical layered streets; the living world is quieter than the Land of the
  Dead; about two-thirds of the film is after dark (<https://animatedviews.com/2017/pixar-production-designer-harley-jessup-remembers-coco/>,
  <https://www.cgw.com/Press-Center/In-Focus/2018/A-Night-to-Remember-Coco.aspx>,
  <https://www.slashfilm.com/world-of-coco/>,
  <https://thewaltdisneycompany.com/news/pixars-coco-uses-innovative-visual-effects-celebrate-family-tradition/>,
  <https://6abc.com/2658725>).
- Sky: Children of the Light's "warmth as reward" logic applies directly (see K).
- Combine with: Darkest Dungeon's "colour spots in heavy linework" (see C) if outlines are used.

**Defining traits**
- Palette: marigold orange, hot pink, turquoise, deep indigo night; cyan spirit glow; papel-picado cut-paper patterns
  on banners and windows; sugar-skull motifs (as props, not gags).
- Light: night or deep blue hour; the *only* lights are emissive petals/embers/lanterns, each casting a coloured
  pool.
- Post: soft bloom, glow halo, gentle vignette.

**How it is built**
- Emissive materials with additive-looking sprites; point-cloud-style cheap lighting via many small low-radius
  non-shadowed lights or lit-particle approaches (budget: tens, not hundreds, on Medium, **inference**); papel-picado
  patterns as alpha-masked cloth meshes swaying in wind; lit fog tinted by the nearest emissive cluster.

**Why it wows / risks**
- Wow: marigold fields lit by cyan spirits are inherently screenshot-worthy; unique niche; coherent with weird
  west + saints + churchyard + revenant story.
- Risk: cultural sensitivity (do it with care and original work); many lights = cost on Polaris; saturation and
  small bright petals reduce enemy readability if not value-controlled; too "cute" if the spiders are cartoonish.

**HUD fit**: excellent: orange + cyan lines are the palette.

---

## N (added). Chunky pixel-lit 3D / HD-2D lighting

**References**
- HD-2D (Octopath Traveler, UE4, a team with only six programmers at peak): pixel sprites in 3D scenery with
  lighting, depth of field and tilt-shift for a diorama feel; a student paper in the search results lists AO, lens
  flares, motion blur, vignette, DoF and bloom as the post effects (weak source); early prototypes used pixel sprites with lighting experiments; combat effects were lifted by adding a
  point light to each effect so characters cast shadows on the environment
  (<https://en.wikipedia.org/wiki/HD-2D>, <https://www.unrealengine.com/spotlights/octopath-traveler-s-hd-2d-art-style-and-story-make-for-a-jrpg-dream-come-true>,
  <https://www.unrealengine.com/developer-interviews/octopath-traveler-ii-builds-a-bigger-bolder-world-in-its-stunning-hd-2d-style>).
- Pixel-art rendering of 3D scenes: Pixelat3D technique paper (<https://sol.sbc.org.br/index.php/sbgames/article/view/45414>).

**For a first/third-person shooter**: sprites-in-3D does not fit; the transferable parts are (1) render the 3D scene
at low resolution (about 480 to 540p) and point-upscale to 1080p, (2) quantise to a limited palette with ordered
dither, (3) apply HD-2D's lighting (strong bloom, DoF, vignette, coloured point lights on every effect). Enemies and
the gunslinger can be 3D models with chunky texels or camera-facing sprites.

**Defining traits**
- Palette: limited, rich, highly saturated hues; a strong colour-per-region rule; bloom everywhere.
- Light: dramatic coloured lights; every muzzle flash, ember and spark casts light and shadow (the Octopath trick).
- Post: bloom, DoF, vignette, pixel-grid snapping; no outlines.

**How it is built**
- Low-res render target + nearest upscale (cuts pixel cost by about 4x at 540p, **a performance gain, not a cost**),
  palette LUT, bloom. RX 580 cost: very low.

**Why it wows / risks**
- Wow: the "retro RPG, modern lighting" hook is hot on Steam and is cheap to run; pixel-lit muzzle flash and ember
  glow look fantastic in GIFs. Trailer-friendly.
- Risk: pixelation fights aiming precision at range (small targets become blocks) and fights HUD crispness unless the
  HUD is drawn at native resolution (it is, UI is composed after the 3D, **inference**); mixing with a
  gunmetal vector HUD gives a deliberate hi-lo contrast but divides opinion; first-person users may find low-res
  nauseating.

**HUD fit**: good: a crisp native-res HUD over a pixelated world is a deliberate hi-lo contrast (inference; the
HUD is composited after the 3D scene so it stays sharp).

---

## O (added). PS1/PS2 lo-fi "Y2K" survival-horror look

**References**
- Technique: vertex snapping (integer-rounded positions give the wobble), affine texture mapping (no perspective
  correction), 4x4 Bayer dithering to hide colour banding, distance fog
  (<https://www.godotengine.org/asset-library/asset/4687>, <https://github.com/MenacingMecha/godot-psx-style-demo>,
  <https://menacingmecha.itch.io/godot-psx-style-demo>, <https://itch.io/devlog/849811/vfx-breakdown-lets-go-retro.amp>).
- Crow Country (SFB Games, $20): PS1/PS2 survival-horror tribute; top 30 sellers on Steam in May 2024; 100,000 copies
  by October 2024; a July 2026 tracker lists 98% positive across 7,866 reviews and about 214k estimated copies
  (estimates) (<https://newsletter.gamediscover.co/p/how-crow-country-spooked-its-way>,
  <https://steamdeckhq.com/news/crow-country-has-sold-over-100000-copies/>,
  <https://www.pcgamesn.com/crow-country/reviews-steam>, <https://raijin.gg/app/1996010/Crow_Country>).
- Signalis (pixel-art anime, not 3D PSX) is the adjacent hit (<https://raijin.gg/app/1262350/SIGNALIS>). The PS1
  trend continues in 2025 releases (<https://www.truetrophies.com/news/ps5-indie-games-march-2025>).

**Defining traits**
- Palette: limited, muddy-saturated, fog-heavy; low-poly, 64 to 128 px textures, vertex colours.
- Light: baked/vertex lighting, flat directional sun, harsh fog colour; strong ambient dithering.
- Post: dither, 320x240 to 480p internal, posterise, mild CRT.

**How it is built**
- Vertex shader snapping to a coarse grid, affine UVs (noperspective interpolation), low-res texture sampler, fog by
  distance; point-upscale. In UE this is a material function with vertex position quantisation and "noperspective"
  UVs, which is hard to do cleanly under Nanite (Nanite does not run custom vertex snapping
  **(inference, verify)**), so the Medium-fallback mesh would carry it. Cost: negligible.

**Why it wows / risks**
- Wow: surprisingly strong sales proof (Crow Country); fog hides distance; "vintage horror" matches a haunted farm and
  chapel; very cheap to run at 120 fps.
- Risk: it is a niche, polarising aesthetic; it deliberately lowers fidelity, which conflicts with "quality of the
  best big-studio games" (CLAUDE.md rule 1); the owner may see it as a downgrade.

**HUD fit**: contrast-driven. The crisp, modern HUD over a PS1 world is a conscious mix; can look good if the HUD
stays minimal.

---

## P (added). Hyper-saturated desert action: the Mad Max: Fury Road grade

**Why I added it**: it is the best precedent for "orange sand, teal sky, alien-feeling desert" and shows that a grade
can create richness from neutral art, and it is neither gothic nor pastel.

**References**
- Colourist Eric Whipp: "saturated and graphic, and the night scenes should be blue"; director George Miller wanted to
  avoid the bleached, desaturated post-apocalypse look; wardrobe and art direction were neutral, so the grade built
  richness from sandy beige against a strong blue sky; eight-month DI, about two months for day-for-night; actors'
  eyes rotoscoped for contrast (<https://filmlight.ltd.uk/customers/meet-the-colourist/eric_whipp.php>,
  <https://definitionmagazine.com/features/mad-max-fury-road-the-ultimate-digital-intermediate/>,
  <https://www.awn.com/print/blog/colorful-world-mad-max-fury-road>,
  <https://colorculture.org/mad-max-fury-road-cinematography-analysis/>). Sources disagree about how saturated; the
  colourist's own brief says saturated.
- Contrasting reference, Dune: Part Two: warm, restrained desert; hard light and open shade; upward fill to offset
  sand bounce; haze; "no existing desert film we liked" so they developed a palette
  (<https://britishcinematographer.co.uk/greig-fraser-asc-acs-dune-part-two/>, <https://www.camnoir.com/ep257/>,
  <https://colorculture.org/cinematography-analysis-of-dune-part-two-in-depth/>).

**Defining traits**: deep orange sand, vivid teal-blue sky, strong warm/cool split between lit and shadow areas,
sharp crisp contrast, character eyes lifted; high-saturation skies and clouds; dust.

**How it is built**: LUT-based colour grade on a filmic tonemap (UE's colour grading LUT or the ACES-based grade),
sky gradient, a separate cool shadow tint (tint shadows via the colour-grading shadow wheel), local contrast boost
(clarity), eye-light on characters. Cost: nearly free (one LUT).

**Why it wows / risks**: high wow per cost; works on stock assets; "orange-teal" is the most overused grade, so avoid
the muddy default blockbuster version and commit to the Fury Road saturation and clean separation.

**HUD fit**: excellent; the HUD's orange and cyan are exactly the pair.

---

## Cross-cutting: 2025-2026 games praised for art direction

- Clair Obscur: Expedition 33 won Game of the Year and Best Art Direction at The Game Awards 2025 (nine wins);
  the Best Art Direction nominees were Clair Obscur, Death Stranding 2, Ghost of Yōtei, Hades II, Hollow Knight:
  Silksong (<https://www.shacknews.com/article/147150/clair-obscur-expedition-33-game-awards-win>,
  <https://www.gematsu.com/2025/11/the-game-awards-2025-nominees-announced>). Lesson: a *committed, art-historical
  idea* (Belle Époque painting) executed on the engine's stock features beat photorealism. A small team (about 30)
  did it.
- Ghost of Yōtei (2025): the same studio's follow-up, reviewed as surpassing Tsushima; Kurosawa-mode filter
  (black and white + grain + more wind) was a marketing hook (<https://esquiresg.com/ghost-of-yotei-video-game-interview/>).
  Lesson: a "filter mode" is itself a marketing asset; the Style Lab could ship a photo-mode selector.
- ARC Raiders (UE5, 2025): praised art direction and polish for a multiplayer shooter
  (<https://www.purexbox.com/features/opinion-arc-raiders-is-one-of-the-most-visually-impressive-multiplayer-games-ive-ever-played>).
- Marathon (2026): bold graphic identity, polarising, plagiarism backlash (see I).
- Borderlands 4 (2025, UE5): keeps the inked style; praised visuals, performance criticised (see E).
- MOUSE: P.I. For Hire (2026): a first-person shooter with hand-drawn 1930s rubber-hose animation in monochrome;
  GameSpot calls the art its best asset; some reviews cite performance issues
  (<https://www.gamespot.com/reviews/mouse-p-i-for-hire-review-rodent-noir/1900-6418481/>,
  <https://www.pushsquare.com/reviews/ps5/mouse-p-i-for-hire>, <https://www.nookgaming.com/mouse-p-i-for-hire-review/>).
  Lesson: a radically different art style on a plain FPS loop is itself a draw.
- Dispatch (2025, AdHoc): "premium TV animation" look, praised widely; the team said Telltale-style titles looked
  "like a video game" and they wanted a look that sits alongside an animated TV show
  (<https://www.destructoid.com/dispatch-developer-interview/>, <https://www.videogameschronicle.com/reviews/dispatch-review/>).
- Wild Bastards (2024), a space-western FPS: the closest genre neighbour; it proves the "stylised space western FPS"
  space is open (<https://www.pcgamesn.com/wild-bastards/review>).
- Westerns coming: Westlanders (first-person wagon survival, 2026 trailer), Far Far West (robot-cowboy co-op), Erosion
  (post-apocalyptic western roguelike), Western Rye (Aug 2026 launch), Hard West 2, Evil West, Blood West. WolfEye
  (Weird West) is making an unnamed retro-futuristic first-person RPG with a Fallout-meets-Weird West look
  (<https://www.pcgamesn.com/western-games-best>, <https://gamerant.com/biggest-upcoming-2026-western-rpg-games/>,
  <https://need4games.ro/en/new-game-showcase-2026-every-game-reveal-and-update>,
  <https://gamesbeat.com/wolfeye-studios-partners-with-neowiz-on-new-sci-fi-action-rpg/>). Lesson: the weird-west-
  sci-fi lane is getting crowded, so a *distinctive treatment* matters more than the setting.
- Overview lists: <https://www.metacritic.com/pictures/best-video-games-of-2026-at-midyear/20/>,
  <https://gamingbolt.com/10-best-looking-games-of-2026-so-far>.

## Cross-cutting: what makes a stylised shooter's Steam screenshots stand out

Sourced:
- The first five seconds on the page matter; clarity of genre in the capsule; shrink the capsule to test whether the
  genre registers in a second; capsules need contrast and a single focal point; dark capsules vanish on Steam's dark
  UI; screenshots must be in-game (Valve's rule excludes concept art and pre-rendered stills; verify current
  guidance); put core gameplay in the first three screenshots; avoid overlays and build-dating text
  (<https://unity.com/blog/tips-strategies-for-marketing-indie-games> (Zukowski talk),
  <https://presskit.gg/field-guides/steam-page-optimization-guide>,
  <https://www.steampageanalyzer.com/blog/steam-screenshot-guide>,
  <https://www.steampageanalyzer.com/blog/steam-capsule-design-guide>,
  <https://game-wisdom.com/critical/beginners-guide-steam-store-pages>,
  <https://www.gamedeveloper.com/business/a-beginner-s-guide-to-designing-indie-game-store-pages>,
  <https://bugnet.io/blog/steam-capsule-art-a-guide-for-indie-devs>,
  <https://megacatstudios.com/blogs/game-development/the-power-of-first-impressions-how-to-design-a-killer-steam-capsule>,
  <https://mein-mmo.de/en/steam-screenshots-ingame-material,122682>).
- Readability is part of the art direction in the best examples: Valorant ("legible on any hardware"), TF2
  (rim light and hue shifts so players tell classes apart), Outer Wilds (switched style to read surfaces).

My judgement (not from a single source):
1. One *recognisable idea* per frame, visible at thumbnail size: a silhouette plus one saturated accent plus a big
   graphic shape in the sky.
2. Design the screenshot compositions up front (a hero spot per area at a chosen time of day, camera at the best
   focal length), the way Tsushima/Yōtei were authored as "paintings".
3. Show action in at least two of five screenshots (muzzle flash, hit sparks, ember burst, dust), because those are
   where a style differs from the competition.
4. Colour-script the game: assign each region a palette (Yōtei does this) so five screenshots look like five
   different places, not one.
5. Keep the HUD visible in at least one shot so the HUD's identity becomes part of the game's brand, but keep it out of
   the capsule.
6. Make the cheapest pass count: grading and sky are the highest wow-per-millisecond tools on Medium.

## Technical cheat-sheet for the RX 580 target (all costs are estimates; measure)

| Technique | Where it lives | Estimated cost at 1080p, RX 580 | Source for method |
|---|---|---|---|
| Filmic tonemap + LUT grade | Post (free in UE) | <0.1 ms | UE docs |
| Film grain + vignette + halation | 1 to 2 fullscreen passes | 0.2 to 0.5 ms | links in B |
| Depth+normal outline (Sobel) | Fullscreen pass | 0.3 to 0.8 ms | Tom Looman, 300mind (E) |
| Stencil/custom-depth outline | Fullscreen pass | 0.3 to 0.6 ms | same |
| Inverted hull outline | Per-mesh, +draw | 0.1 to 0.5 ms scene dependent | Genshin reverse-eng. (F) |
| Toon ramp lighting | Material | ~0 extra | Hi-Fi Rush (E) |
| Volumetric fog | Froxel / half-res | 1 to 2 ms | Hunt, RDR2 (A, C) |
| Depth of field (bokeh) | Fullscreen | 0.5 to 1.5 ms | (J) |
| Anisotropic Kuwahara (r=4 to 6) | 3 passes | 2 to 5 ms | Kyprianidis (G) |
| Low-res render + upscale | Resolution | gains 40 to 70% of pixel cost | (N, O) |
| Lumen | Not on Medium (project rule) | n/a | CLAUDE.md |

Because Lumen and Nanite are High/Epic only here, anything dependent on Lumen GI (Clair Obscur-style bounce light)
must be replaced on Medium by baked lighting + sky capture + hand-placed fill lights. Sandfall's own experience: the
switch to Lumen initially broke hand-tuned lighting and locations were retuned, so "lighting must be authored to the
look, not left to GI" is the standing lesson.

## Summary table (ratings 1 to 5)

Wow factor: 5 = best. Combat readability: 5 = best. RX 580 cost: **1 = very cheap, 5 = very heavy**. HUD fit: 5 = best.
"Novelty" = how unlike the rejected list and how unusual in shooters.

| Style | Wow | Combat readability | RX 580 cost | HUD fit | Novelty | Main risk |
|---|---|---|---|---|---|---|
| A Golden-hour cinematic realism | 4 | 3 | 4 | 4 | 2 | Compared with RDR2; haze muddy; costly |
| B Leone Technicolor | 4 | 3 | 2 | 4 | 4 | Grain softens enemies; letterbox |
| C Dark weird-west gothic | 4 | 3 | 2 | 5 | 3 | Dark thumbnails; brown/dark trope |
| D Chunky hand-painted | 3 | 5 | 1 | 4 | 2 | "Fortnite/Overwatch" lineage |
| E Ink-comic cel (Borderlands/Hi-Fi) | 4 | 5 | 2 | 4 | 2 | Nearest to rejected; clone perception |
| F Anime cel | 5 | 4 | 2 | 4 | 3 | Needs matching character art |
| G Painterly (painted light, no filter) | 4 | 3 | 1 | 3 | 3 | Labour; overlap; motion shimmer |
| H 1970s pulp paperback | 5 | 4 | 2 | 5 | 5 | Flat gradients; bland if timid |
| I Neon dusk | 4 | 3 | 3 | 5 | 3 | Saturation fatigue; off-brand for gothic |
| J Claymation / diorama | 5 | 3 | 3 | 4 | 5 | Art labour; first-person tilt-shift fails |
| K Luminous sun-bleached | 4 | 4 | 1 | 3 | 4 | Monotony; tone vs gunplay |
| L Dime-novel engraving/woodcut | 3 | 2 | 2 | 3 | 3 | Overlap; shimmer; eye strain |
| M Marigold + spirit-glow | 5 | 3 | 3 | 5 | 5 | Cultural care; many lights |
| N Chunky pixel-lit 3D | 4 | 2 | 1 | 4 | 4 | Aiming at range; polarising |
| O PS1/PS2 lo-fi | 3 | 3 | 1 | 3 | 3 | Reads as downgrade; niche |
| P Fury Road saturated desert | 4 | 4 | 1 | 5 | 3 | Orange-teal fatigue if done muddy |

## Shortlist recommendation: ten styles, cartoon to real

If the owner wants ten genuinely different looks, spanning cartoon to realistic, with the best wow-per-cost and best
HUD fit, I would pick these (most cartoonish first):

1. **J Claymation / miniature diorama** (most unusual, handmade; wow 5). Use macro DoF and clay-texture, not tilt-shift.
2. **O PS1/PS2 lo-fi** (cheapest; proven by Crow Country).
3. **N Chunky pixel-lit 3D** (resolution-quantised with HD-2D light).
4. **M Marigold + spirit-glow folk art** (perfect orange/cyan match).
5. **F Anime cel** (the biggest Steam audience).
6. **D Chunky hand-painted** (safest readable "friendly" option).
7. **H 1970s pulp paperback / ringed planet** (strongest concept hook for "alien frontier").
8. **I Neon dusk** or **C Dark gothic** (pick one; they share the ember-as-light idea. C fits the story better; I is
   flashier for thumbnails).
9. **B Leone Technicolor** (distinct film-stock identity).
10. **A Golden-hour cinematic realism** with Fury Road (P) grade (the realistic end).

Dropped to the "backup bench": E (nearest to the rejected comic/ink looks), G (painterly, high overlap/labour), K
(contemplative tone), L (overlap with pen-and-ink).

Three that I think have the highest chance to "WOW" on a Steam page for this game, in order: **H** (ringed-planet
pulp-paperback frontier, a big graphic idea nobody else in the western lane has), **M** (marigold and spirit glow,
the HUD's own colours as the world's colours), and **B+P combined as a final "realistic" option** (Leone-grain and
halation on a saturated Fury Road grade, ember cyan the only cool).

## Preview-recipe notes for the three.js Style Lab (inference)

- Everything above that is "post" is easy in three.js: tonemap and LUT, bloom, vignette, grain, DoF (BokehPass),
  outline (Sobel on depth+normal or an inverted-hull), low-res render + palette quantise + ordered dither.
- Toon ramps: `MeshToonMaterial` with a gradient map; for painted-light styles bake gradient AO into vertex colours.
- Glitter (K): add a high-frequency noise normal and a rim term in `onBeforeCompile`.
- PS1 (O): vertex snap in the vertex shader (`gl_Position.xy = floor(gl_Position.xy / gl_Position.w * res) / res * gl_Position.w`),
  `noperspective`-style UVs via a varying multiplied by w, nearest-neighbour textures.
- Ringed planet (H) and big sun (A, B, P): a sprite/quad in the sky dome; the ring shadow as a decal.
- Embers (all): additive point sprites plus a small point light; keep light count low (the browser is not a Polaris
  GPU, but the previews should respect a 120 fps UE budget in spirit).
