# Style Lab

A real-time three.js page that rebuilds a piece of Ransom's Rest (Main Street, boot hill, the chapel on its knoll, the
Sink) from the game's own models and texture sets, puts the new in-game HUD over it, lets you walk and shoot spiders,
and switches between ten complete art directions with one key. Style 0 is the game's look today. The contract every
agent codes to is in the orchestrator's notes; this file is the reference for writing a style.

## Files

| Path | What |
|---|---|
| `web/index.html` | The page: a fragment (the artifact host adds the document skeleton), the lab's UI in the LooterUI look, the import map (three 0.170.0 from jsDelivr) |
| `web/engine/main.js` | Boot, the style switch, shots and stills, input, the frame loop, `window.lab` |
| `web/engine/materials.js` | The kit's material builders (`pbr`, `toon`, `flat`) and the shared shader patches; `terrainmat.js` the terrain |
| `web/engine/post.js` | The HDR target and every post pass; `glsl.js` shared GLSL; `tonecurve.js` tone curves and their inverse |
| `web/engine/lighting.js` | The sun's three shadow cascades, the hemisphere ambient, the lantern light pool |
| `web/engine/sky.js` | The sky dome (gradient, sun, clouds, stars, planets, moons, rings, a striped retro sun) and its environment map |
| `web/engine/world.js` | Terrain, placed instances (one BatchedMesh per material), PCG scatter, colliders, shot raycasts |
| `web/engine/player.js`, `creatures.js`, `fx.js` | The walk and the gun, the spiders, effects and particles |
| `web/engine/assets.js`, `shots.js`, `ui.js`, `hudstub.js`, `util.js` | Loading, the six viewpoints, the lab's UI, a stand-in HUD, helpers |
| `web/styles/index.js` | Lists the style files (`NN_<id>.js`; rewritten from disk by `assemble.py` and the test server) |
| `web/styles/00_today.js` | Style 0: the game today |
| `web/styles/NN_<id>.js` | One file per style; helpers in `web/styles/lib/<style-id>_*.js` |
| `web/hud/` | The in-game HUD (HUD agent) |
| `test/` | `serve.mjs` (local server), `shots.mjs` (stills), `interact.mjs` (plays it), `site.mjs` (shared routing) |
| `export/` | The exporter (see Export below) |
| `assemble.py` | Builds `Saved/StyleLab/site/` to publish |

Binaries never go in the repository: the export, the site and screenshots live under `Saved/StyleLab/`.

## Run it

```
cd Tools/StyleLab/test && npm install                       # once (PLAYWRIGHT_BROWSERS_PATH=/opt/pw-browsers here)
node serve.mjs                                              # http://localhost:8090/?style=0&shot=street (sources + export)
node shots.mjs ../web ../../../Saved/StyleLab/shots/x --styles 0,3 --shots street,chapel --size 1920x1080
node interact.mjs ../web ../../../Saved/StyleLab/shots/interact
python3 ../assemble.py                                      # Saved/StyleLab/site + publish_files.json
node shots.mjs ../../../Saved/StyleLab/site ../../../Saved/StyleLab/shots/site   # the assembled site
```

`shots.mjs` writes `<NN>_<style-id>__<shot>.png`. It runs Chromium headless with SwiftShader (software WebGL): a 1080p
still takes 30-90 s. `--extra <dir>` loads the `.js` style modules in that folder too (try a style without adding it to
the list; they load through `?extra=__extra/<file>`). `--hud 0` hides the HUD.

URL parameters: `style=N`, `shot=<id>`, `still=1` (fixed time, posed creatures and effects, HUD frozen,
`lab.ready` true when final), `hud=0`, `ui=0` (no lab UI), `stats=1` (fps, draws), `data=<dir>` (default `data/`),
`scale=<n>` (render scale), `extra=<urls>`, `sync=1` (finish every frame: software GL in tests).

Headless Chromium leaks memory without bound while the pointer is locked, so `interact.mjs` never locks it (it pulls
the trigger through the player) and steps the world with `lab.tick(seconds)` (30 Hz, no drawing) between key events:
software GL draws a frame every few seconds.

`window.lab`: `setStyle(n)` (promise), `setShot(id)`, `setHud(bool)`, `still(bool)`, `view(n, id)` (a style at a shot,
one still), `tick(seconds)` (tests: step the world without drawing), `ready` (a promise while a still is pending, then `true`; `isReady` mirrors it), `styles`, `state()`.

Keys: `1`-`9`, `0` styles 1-10, `` ` `` style 0, `V`/`B` next/previous shot, `H` HUD, `I` the style card, `U` hide the lab
UI. Click to walk: `WASD`, mouse, `Shift` sprint, `Space` jump, left mouse fire, right mouse aim, `R` reload, `Esc`.

Shots (UE cm, eye height above the ground, UE yaw/pitch, horizontal FOV): `street` (60, -1000; 6 m east of
views.json's MainStreet so the gate sign is overhead), `fight` (200, 1700), `chapel` (5650, -2000, yaw 132: at the chapel
yard's gate, 3.5 m north of views.json's ChapelOverTown), `boothill` (3400, 300, yaw 326: 6 m east of views.json's,
among the graves, clear of the oaks), `sink` (4100, 5300, yaw 45, pitch -4: on the pit floor between views.json's SinkRim and SinkFloor, toward the lit
wall and the den's webbed mouth), `aerial` (-6000, -6000,
40 m up).

## Writing a style

```js
export default {
  number: 3, id: 'ember-gothic', name: 'Ember Gothic', family: 'Dark realism',
  tagline: '...', pitch: '...', hudFit: '...',
  refs: [{ title: '...', url: 'https://...', note: '...' }],
  unreal: { recipe: ['...'], costMs: '~0.9 ms', risks: ['...'] },
  env: { ... },                       // below
  material(src, kit) { return kit.pbr(src, {}); },        // once per manifest material, cached per style
  terrain(src, kit) { return kit.terrainMaterial({}); },  // optional
  sky(kit) { return null; },          // optional: a THREE.Mesh (replaces the dome) or a Material (on the dome)
  post(kit) { return [kit.post.ao({}), kit.post.bloom({}), kit.post.grade({})]; },
  fx: { muzzle: '#ffd27a', tracer: '#ffe6a8', impact: '#ffcf7a', blood: '#9be35a', hitFlash: '#ffffff' },
  activate(kit) {},                   // optional: on becoming active (add objects under kit.styleRoot)
  deactivate(kit) {},                 // optional: on being switched away (kit.styleRoot is emptied for you)
  update(dt, t, kit) {},              // optional, every frame (dt real seconds, t world time)
};
```

`src` is the manifest's material record plus `name`: `master` (World, WorldFoliage, Terrain, Water, Gun, Glass, Smoke,
Waterfall, Glow, Flat), `set`, `tint`, `tintLinear`, `uvScale`, `color` (representative sRGB albedo), `roughness`,
`metallic`, `glow`, `glowColor`, `twoSided`, `alphaMask`, `wind`, `role` (wood, metal, stone, ground, rock, foliage,
bark, grass, fabric, paper, glass, bone, creature, skin, glow, gun, water, smoke, other), `params`, `unlit` (the far
backdrop's silhouettes). Branch on `role`, `master` or `name` to give groups their own treatment.

Colours: every `'#hex'` is sRGB as you would pick it. Lighting runs in linear light; tone mapping comes last. The sky's
and the fog's colours are what you see on screen at the style's exposure: the engine inverts the tone curve for them.

### env

| Key | Fields (defaults) |
|---|---|
| `sun` | `azimuth` (UE yaw the light comes from, 0 north, 90 east; 250), `elevation` (deg; 20), `color`, `intensity` (3), `shadowSoftness` (1: about 3.5 cm of penumbra), `shadowStrength` (0-1; 1) |
| `ambient` | `sky`, `ground`, `intensity` (0.6): a hemisphere light |
| `exposure` | 1 (multiplies before tone mapping) |
| `toneMapping` | `'aces'` \| `'agx'` \| `'neutral'` \| `'none'` |
| `fog` | `color`, `density` (per metre at the base height; 0.002), `heightFalloff` (per metre; 0.05), `height` (base, m; 0), `start` (m; 0), `sunScatter` (0-1; 0.5), `sunColor`, `sunExponent` (8), `maxOpacity` (1). Height fog in every kit material, the sky's horizon, the smoke and particles |
| `sky` | `zenith`, `horizon`, `ground`, `sunDisc` (0 hides the disc), `sunGlow`, `horizonFog` (0-1, how far the horizon takes the fog's colour; 0.65), `brightness` (1), `clouds: { amount (0-1), color, shade, scale, sharpness (0 soft - 1 hard edges), speed }`, `stars` (0-1), `bodies` (up to 4, below) |
| `lights` | `scale` (1): the scene's lamps (Unreal candela, roughly 0.65 cd per unit here) |
| `particles` | `dust` (0-1 motes, lit toward the sun), `embers` (0-1), `emberColor`, `motes` (dust colour) |
| `wind` | `direction` (UE yaw it blows toward; 60), `strength` (sway in metres; 0.12), `speed` (1.4) |
| `smoke` | `opacity` (0.22): chimney smoke |
| `animStep` | 0 = smooth; e.g. `1/12`: creatures, effects, wind and smoke move in steps of that length (stop motion); the camera and HUD stay smooth |

`sky.bodies[]`: `{ kind: 'planet' | 'moon' | 'sun' | 'retrosun', azimuth, elevation, size (angular diameter, deg),
color, color2 (bands / maria / the retro sun's foot colour), stripes (planet bands or retro sun bars), glow (0-2),
roll (deg), ring: { color, tilt (deg from edge-on; 18), inner (1.4), outer (2.3) planet radii, roll } }`. Planets and
moons are lit by the sun (phases follow it), rings pass in front of and behind the disc and take its shadow; `sun` and
`retrosun` are emissive (bright enough to bloom).

### The kit

`kit.THREE`, `kit.renderer`, `kit.scene`, `kit.camera`, `kit.time` (world seconds), `kit.manifest`, `kit.style`,
`kit.env`, `kit.color('#hex')` (a linear THREE.Color), `kit.tex(set, 'bc'|'n'|'orm', { blur, uvScale })` (cached; bc is
sRGB, n and orm linear; `blur` 0-6 halves the resolution per step), `kit.styleRoot` (a Group emptied on every switch),
`kit.viewmodel` (the gun's root, a child of the camera), `kit.player` (`{ position, yaw }`, feet, UE yaw),
`kit.lights` (the scene's lamps: `{ p, color, intensity, range, note, scale, colorOverride }`; set `scale` or
`colorOverride` (a THREE.Color) to boost or recolour; reset on every switch), `kit.sun` (the lighting: `sunDir`,
`sunColor`, `hemi`, `cascades`), `kit.sky` (its `uniforms`), `kit.shared` (uniforms every kit material shares:
`uSlTime`, `uSlFogColor`, `uSlFog`, `uSlWind`, ...).

#### Material builders

Every builder takes `(src, opts)` and gives, with nothing to do: shadows (three soft cascades), the style's height
fog, vertex occlusion (COLOR_0.a), foliage alpha test (alpha to coverage) + two sides + wind sway (COLOR_0.r weight,
.g phase), DirectX normal maps, skinning, instancing and batching, glow (emissive from `src.glow`), `uvScale` and
`tintLinear`. Common options:

| Option | Default | What |
|---|---|---|
| `albedo` | `'texture'` if the material has a set | `'texture'` or `'flat'` (src.color, or `color`) |
| `color` | src.color / tint | the flat albedo, or a multiplier on the texture |
| `blur` | 0 | texture mip bias 0-6 (painted looks) |
| `saturation`, `value`, `contrast` | 1 | on the albedo (contrast around mid grey) |
| `hueShift` | 0 | degrees |
| `tint` | none | `{ color, amount }`: recolours the albedo toward a hue, keeping its lightness |
| `posterize` | 0 | levels of the lit colour (0 off) |
| `normalScale` | 1 (flat: 0) | normal map strength |
| `roughness` / `roughnessValue` | 1 / - | multiplier on the map / a fixed value |
| `metalness` | src.metallic | |
| `aoStrength` | 1 | vertex and texture occlusion |
| `rim` | none | `{ color, power: 3, strength: 0.5 }`: fresnel rim, brighter on the lit side |
| `glowBoost` | 1 | multiplies src.glow |
| `envIntensity` | pbr 1, toon 0.3, flat 0 | sky reflections |
| `translucency` | foliage 0.6 | light through leaves facing the sun |
| `upNormal` | grass 0.65, foliage 0.3 | bends normals up (soft, even foliage shading) |
| `groundTint` | grass 0.45 | takes the terrain's macro colour under it |
| `wind` | src.wind | sway (env.wind sets direction, strength, speed) |
| `opacity` | - | below 1 makes it transparent |
| `unlit` | src.unlit | albedo times about what sunlit ground shows, no shading |
| `hook` | '' | GLSL run last on `vec3 col` (linear, before fog), with `vec3 N, V, L` (world), `float NdL, shadow, ao`, `vec2 uvTex`, `vec3 worldPos`; also `uSlTime`, `cameraPosition`, `slIGN(vec2)`, `slVNoise(vec2)`, `slFbm(vec2)`, `slLuma(vec3)` |
| `hookAfterFog` | false | run the hook after the fog instead |

- `kit.pbr(src, opts)`: three's physically based lighting.
- `kit.toon(src, opts)`: a ramp. Extra: `steps` (2-5; 3) or `ramp: [[NdL edge, light level], ...]` (NdL -1..1; a point in
  shadow reads -1, e.g. `[[-1, 0.15], [0.05, 0.6], [0.5, 1]]`), `softness` (0.05, edge width in NdL), `wrap` (0-1),
  `shadowTint` ('#hex': the shade shifts toward this hue, keeping its lightness) and `shadowTintAmount` (0.5 if
  set), `specular: { size: 0.1, strength: 1, color }` (a hard highlight).
- `kit.flat(src, opts)`: flat colour (albedo `'flat'` unless asked) through the toon ramp, unlit-ish: lit parts take
  `light` (number, or '#hex'; default the sun's hue at brightness 1), shade parts `shade` (default the sky's hue x 0.45).
  Takes the toon options.
- `kit.terrainMaterial(opts)`: the game's M_Terrain blend (the macro map's colour x the detail textures' light and dark;
  grass, dirt by mask R, rock by mask G; faces from 40 to 55 degrees take the rock laid on from the side). Extra:
  `toon` (true: a toon material underneath, with the toon options), `macroStrength` (1: the macro's colour as the game;
  0: the detail textures' own colours), `detailStrength` (0.6), `normalStrength` (0.8), `steepStart` (40),
  `steepFull` (55), `steepTint` ([0.8, 0.74, 0.68] or '#hex'), `steepDetail` (1.15), `steepNormal` (0.6), `tileScale` (1),
  `detailFade` ([60, 260] m), plus every common option.

A style may return any THREE.Material; lit built-in ones are adopted (one sun, shadows, fog). The kit's are strongly
preferred. Materials are cached per style, so switching back is instant.

#### Post passes

Each returns a pass with live `.params` (change them in `update`) and `.uniforms`. Scene-referred passes (`ao`,
`bloom`, `halation`, `godrays`, `dof`, and `custom` with `stage: 'hdr'`) run first in the listed order, then tone
mapping, then the display-referred passes in the listed order (colours there are sRGB as you see them). Depth is always
there; a normal buffer is rendered only when a pass needs it (`outline` with inner lines, `custom` with `needsNormal`).

| Pass | Options (defaults) |
|---|---|
| `ao` | `radius` (1.4 m), `intensity` (1), `power` (1.6), `halfRes` (true), `color` ('#000000'), `maxPixels` (90). GTAO, depth-aware blur |
| `bloom` | `threshold` (1.0, scene-referred), `strength` (0.35), `radius` (0.75), `tint` |
| `halation` | `threshold` (0.8), `radius` (0.9), `color` ('#ff5a2a'), `strength` (0.35): film's red glow round highlights |
| `godrays` | `strength` (0.5), `decay` (0.965), `density` (0.85), `color` ('#ffd9a0'), `threshold` (0), `samples` (48): the sky round the sun, blurred toward it |
| `dof` | `focus` (12 m), `range` (8 m in focus), `blur` (6 px), `nearBlur` (0.4: how much the gun blurs), `farOnly` (false: true leaves everything nearer than the focus sharp but the gun) |
| `outline` | `width` (1 px at 1080p), `color` ('#1a1410'), `colorFromScene` (0-1: lines take the scene's darkened colour), `depthEdge` (1), `normalEdge` (1), `innerLines` (1: creases from normals; 0 silhouettes only), `fadeNear` (60 m), `fadeFar` (400 m), `opacity` (1) |
| `kuwahara` | `radius` (4 px at 1080p, max 6), `sharpness` (8), `halfRes` (false): generalized Kuwahara, painterly |
| `grade` | `exposure` (stops, folded into tone mapping), `contrast` (1), `saturation` (1), `vibrance` (0), `lift` / `gamma` / `gain` (numbers, [r,g,b], or '#hex' = what black / mid grey / white becomes), `temperature` (-1 blue .. 1 warm), `tint` (-1 green .. 1 magenta), `shadows` / `highlights` ('#hex' split toning), `splitBalance` (-1..1), `splitAmount` (0.35), `curve` (0-1 S-curve) |
| `grain` | `amount` (0.04), `size` (1.5 px), `colored` (0.2) |
| `vignette` | `amount` (0.3), `softness` (0.5), `color`, `roundness` (0.6) |
| `chromatic` | `amount` (0.0025) |
| `tiltshift` | `focus` (0.5, screen height), `band` (0.15), `blur` (6 px) |
| `sharpen` | `amount` (0.3) |
| `posterize` | `levels` (6), `dither` (0) |
| `hatch` | `scale` (6 px), `angle` (45), `density` (1), `color`, `lightResponse` (1), `width` (0.3): up to four layers of hatching by darkness |
| `fxaa` | `subpix` (0.6); put it last |
| `custom` | `{ fragment, uniforms, params, bind(params, uniforms), needsDepth, needsNormal, stage: 'ldr'|'hdr' }`; the fragment defines `vec4 effect(vec2 uv)` and has `tColor`, `tDepth`, `tNormal`, `linearDepth(uv)`, `viewPos(uv)`, `viewNormal(uv)`, `uResolution`, `uTime`, `uSunScreen` (uv, z = 1 when in front), `uNear`, `uFar`, `slIGN`, `slVNoise`, `slFbm`, `slLuma`, `slToSRGB`, `slToLinear` |

With no `post`, a style gets `[ao, bloom, grade]`. Every frame ends dithered.

### Performance

The frame is one HDR pass (4x MSAA, half float) with depth, the post passes, and the sun's three 2048 cascades (the far
two re-rendered every 2nd and 4th frame). Static things are one BatchedMesh per material (about 100 draws a pass);
grass and small scatter are instanced around the camera. Full-screen passes cost about 0.1-0.4 ms each at 1080p on a
mid-range GPU; `kuwahara` (about 1-2 ms), `ao` (0.5-0.8 ms half-res) and `dof` cost the most. A normal buffer (outline
inner lines) re-draws the scene once more.

## Export

The Style Lab draws Ransom's Rest from the game's own sources: the scripted Blender models, the texture sets, the
area generator's terrain and maps, and the level build's placements. `Tools/StyleLab/export/` turns them into
`Saved/StyleLab/export/` (generated, never committed: binaries stay out of the repository):

    Tools/StyleLab/export/export_all.sh [--force]

| Step | Script | What it does | Time |
|---|---|---|---|
| 1 | `terrain_build.py` | Builds the terrain with `Art/Levels/area_model.py` (as `Art/Models/Terrain/RansomsRest.py` does) into `Saved/StyleLab/work/terrain.blend` | ~3.5 min, skipped when the blend exists (`--force` rebuilds) |
| 2 | `run_models.py` | Runs every `Art/Models/*/*.py` through `model_export.py`, one process each, 3 at a time; cached by the script's and the kits' hashes | ~7 min cold |
| 3 | `terrain_export.py` | `terrain_near.glb` (the game's core mesh inside the region), `terrain_far.glb` (the rest of the core, water, ring, canyon wall, backdrop), `heights.bin`, the macro map and masks | ~1 min |
| 4 | `scene_build.py` | Runs `Tools/Unreal/build_area.py RansomsRest` (the full build, every `build_area_*.py`) under `mock_unreal.py` and records what it spawns | ~10 s |
| 5 | `lab_export.py` | `scene.json`, `manifest.json`, `models/pack_N.glb` (< 13 MB each), the texture sets as webp | ~35 s |
| 6 | `check.py [--blender]` | The contract's checks (packs parse, nodes = manifest, every material has a record, every texture exists, Main Street and the chapel where they belong) | seconds |

Python is the bpy 4.5 module: `/home/user/bpyenv/bin/python`. Outputs are deterministic (rerun, same bytes).

**Models** (`model_export.py`). Each model script runs in an empty scene as `Tools/models.ps1` would; every top-level
mesh or armature is a model, its child meshes merged with modifiers applied and its pivot at the origin, `UCX_`/`USP_`/
`UCP_` and `_` objects left out. UVs, custom normals, the `Col` vertex colours (glTF `COLOR_0`, linear: A is baked
occlusion; vegetation R wind, G phase) and the real material names are kept; textures are not embedded (the manifest
points at the sets). Rigs keep their skin and bones (`bones`, `boneRest`; GLB bone names carry the model's name,
`Spider_femur_0_l`, with `gameBones` the game's names); sockets come in three's axes and Unreal's. `Gun_Bullpup` and
`Gun_Ranchhand` are each script's `DEFAULT` parts on the body's sockets, as the game assembles a common gun. Model
scripts write into the repository as they run (the spider's texture set, `DenDressing.placement.json`, the screen
weave): a write guard (`labcommon.install_write_guard`) sends every such write to `Saved/StyleLab/work/sandbox`, so the
export never changes a tracked file. The kits' Windows font paths are pointed at `Art/Fonts/Rye-Regular.ttf`.

**Placements** (`mock_unreal.py`, `scene_build.py`). The level build's own Python runs unchanged against a stand-in
`unreal` module: actors, components, attachments, sockets, instanced meshes and material slots behave as in the
editor, and every trace (`line_trace_component`, `SystemLibrary.line_trace_single`, `capsule_overlap_components`)
meets the real triangles of the exported models and terrain pieces through mathutils BVH trees. So cliffs, panels,
talus, abutments, walkway clearing, platforms, dressing (fences plumb on slopes, grave rows, yard props), effects,
house lights and every story script (`build_area_story/chapel/sink/den/deck/depot/caches/farm/whitlock/posters`) place
what they place in Unreal; only the light, sky and fog step is skipped. What C++ constructors add is filled in from
the C++'s asset paths (the Keeper's Lantern and its snare, the depot's building, the train's cars at their layout
spots, wanted posters as cards over `T_Posters_BC`'s cells). PCG's scatter is exported as rules (`scene.scatter`, from
`Tools/Unreal/build_island_scatter.py`: cell, keep threshold, slope band, scale, models, masks), with the generator's
listed scrub (pit tufts and sage, rim and crest junipers, sage groups) as instances. `scene.noTrees` holds the boxes
the tree layers keep out of (build_area.py's no_tree_zones from `level.noTreeZones`), and the rules built with
`trees=True` (trees, meadow trees, crease pines) carry `noTrees: true`. Some cliff pieces are mirrored (negative x
scale), as in the game: the engine batches them apart with the winding flipped.

**Terrain.** `Terrain_Near`/`Terrain_Far` are the game's own adaptive core mesh (143k triangles over 400 m, the same
triangles Unreal draws at every quality), cut along the region with shared seam vertices; `heights.bin` is ray-cast
from them (1024² cell centres over [-200, 200] m). Every lab map is row 0 at z0, column 0 at x0 (load with
flipY = false), and RGB or grayscale: a map's alpha (the macro map's detail selector, the masks' last channel) is a
grayscale file of its own (`<map>_A.webp`), since a browser loses the colour under alpha 0. `manifest.terrain.rules`
gives M_Terrain's blend with Ransom's Rest's numbers.

**Textures.** RGB and alpha are always resized apart (Pillow resizes RGBA premultiplied, which wipes the colour under
alpha 0); cut-out colour maps (leaves, needles, webs, posters) are lossless RGBA webp keeping the source's dilated
colour under alpha 0, which `check.py` compares with the source.

**Materials.** One record per material name, with the area's swaps applied in the GLBs (`MI_RockCliff_Ransom`,
`LeavesOak_Ransom`, the dark cottages' `WindowGlow_Dark`, the Sink's own webs and granite, the windmill's steel):
`master` (the game's `gameMaster` mapped onto the contract's list), `set`, `tint`/`tintLinear`, representative `color`,
`roughness`/`metallic` (the set's ORM means), `glow`, `twoSided`, `alphaMask`, `wind`, `role`, and `params` (moss,
tint variation, opacity). Normal maps are DirectX. The engine gives each tree its share of `params.TintVariation`, as
the game does.

**Not exported / approximated.** The light, sky and fog (each style brings its own); the Gravewind's dusk-only wisps and
canyon fog; the cold open's set. Creatures stand at their spawners (spread over the spawn radius, seeded); the
Gravemother is `Spider__Pale` at 1.8x. House lamps use the game's candelas with a warm colour (the C++ light's colour is
not in the data). Posters are cards, not decals. Smoke is listed as points (`SmokePlume` is in the packs). The lab
places the non-spider characters at rest and leaves out the Unpaid (no AI for them here).
