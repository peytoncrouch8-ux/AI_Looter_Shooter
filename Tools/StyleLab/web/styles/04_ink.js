// Style 4: Inkslinger. A living graphic novel: heavy hand-inked contours that swell and taper, hatching that lives only
// in the shade and crawls into the cracks (drawn in world space, so it sits on the walls, not on the screen), the
// texture sets kept as the colourist's paint, softened into flats and pushed toward red ochre, rust and turquoise, under
// a hard afternoon sun and a comic sky of hatched cumulus.
import { INK_HOOK, INK_HOOK_GUN, INK_HOOK_SOFT, INK_HOOK_TERRAIN, INK_OUTLINE, INK_FLASH_FS } from './lib/ink_glsl.js';
import { inkSky } from './lib/ink_sky.js';

const INK = '#1a1210';
const SKY = {
  zenith: '#13586a', mid: '#3d9c9a', low: '#dcd8b0', horizon: '#e8a85a', ground: '#5a3424',
  cloudLit: '#fbf1dc', cloudShade: '#c9b39a', cloudDeep: '#8a7f86', ink: INK, haze: 0.6, hatchDeg: 0.28,
};

// The light model every surface shares: three hard bands (shade, mid, light) kept fairly light, because the ink does
// the darkening; the hook colours the mid band toward rust and the deep shade toward a cool plum.
const BANDS = { ramp: [[-1, 0.42], [0.04, 0.7], [0.42, 1.0]], softness: 0.012 };

const T = (kit, src, o) => kit.toon(src, Object.assign({}, BANDS, { hook: INK_HOOK, blur: 1.8, saturation: 1.12, contrast: 1.08 }, o));

export default {
  number: 4,
  id: 'ink',
  name: 'Inkslinger',
  family: 'Graphic novel',
  tagline: 'A frontier graphic novel that moves: inked by hand, hatched in the shade, coloured in rust and turquoise.',
  pitch: 'Every surface is inked: bold contours that swell near and thin far, crease lines on every board, and hatching that '
    + 'appears only where the sun does not reach, cross-hatched in the deep shade and scratched into every crack. The colour '
    + 'underneath is the texture sets themselves, softened into a colourist\'s flats: red ochre in the sun, a rust core where '
    + 'the light turns, cool plum in the deep shade, under a turquoise sky of towering hatched cumulus. A weird-west comic '
    + 'you play, and the lines are ours.',
  hudFit: 'The HUD\'s outlined white text and gunmetal frames already speak the language of ink; world lines stay at most 3 px '
    + 'near and thin with distance, so the HUD\'s strokes remain the heaviest on screen, and the calm turquoise sky keeps the '
    + 'top corners quiet.',
  refs: [
    { title: 'How Borderlands\' devs created its signature style (80.lv)', url: 'https://80.lv/articles/how-borderlands-devs-created-franchise-s-signature-art-style',
      note: 'the counter-example: ink lives in hand-drawn textures plus one outline pass; ours hatches by light in world space instead' },
    { title: 'Borderlands 4\'s "dynamic inking" (Adam May interview)', url: 'https://cogconnected.com/feature/borderlands-4s-adam-may/',
      note: 'inked like a graphic novel rather than cel shading' },
    { title: '3D Toon Rendering in Hi-Fi Rush (GDC 2024)', url: 'https://gdcvault.com/play/1034251/3D-Toon-Rendering-in-Hi',
      note: 'hard N.L thresholds for crisp bands, stylisation in a deferred post stage' },
    { title: 'Wild Bastards\' line work (Aftermath)', url: 'https://aftermath.site/wild-bastards-blue-manchu-art.md',
      note: 'a space-western where the sun drives the line work and shade changes the lines' },
    { title: 'Outline post process in UE (Tom Looman)', url: 'https://tomlooman.com/unreal-engine-outline-multi-color-post-process/',
      note: 'the Sobel/stencil outline the Unreal version builds on' },
  ],
  unreal: {
    recipe: [
      'Cel bands in the deferred post (as Hi-Fi Rush): a post material reads SceneColor / (BaseColor x light) to get the light level and snaps it to three hard bands (0.42, 0.7, 1.0 of the sun), tinting the middle band toward rust and the deepest toward a cool plum at the same lightness. Base colour takes a +1.8 mip bias so textures read as flats.',
      'Hatching in the master materials, not the post: a world-aligned (triplanar) hatch texture pair (one direction, cross) sampled at 6 cm and blended into the next mip-like level by pixel footprint (DDX/DDY of world position), masked by the shadow and N.L terms the post computes the bands from (pass the band through a custom stencil-free GBuffer channel, or recompute N.L in the material from the sun direction in a Material Parameter Collection and use the shadow from a light function).',
      'Inked textures: in each texture set\'s instance, ink the base colour where it falls well below its own mip-5 mean (seams, cracks): one extra texture sample.',
      'Ink contours: one post pass on SceneDepth (inverse-depth second difference in 4 directions) and the GBuffer normal (creases), width 3 px near to 1 px far, modulated by a world-space noise texture for swelling strokes, line colour #1a1210, far lines tinted from the scene.',
      'Sky: an unlit sky-sphere material: four-stop gradient, a 2D cloud layer with three-band shading, sky-space hatch texture on the shaded bands and a contour where density crosses the threshold on the underside.',
      'Muzzle flash: a jagged star mesh with an opaque translucent material (white core, yellow, orange, ink rim) instead of the additive flipbook.',
    ],
    costMs: '~0.8 ms (outline + band post ~0.5 ms, hatching ~0.2 ms in materials, sky ~0.1 ms)',
    risks: [
      'Closeness to Borderlands: keep the world-space hatching by light, the rust/ochre/turquoise palette and the swelling line weight; never ink the albedo flat.',
      'Line crawl in motion on dense grass and far geometry: crease lines fade by 100 m and hatching by 170 m; tune per level.',
      'Toon bands from a post process need per-material exceptions (glows, glass, foliage translucency).',
      'Hatch density must be checked at 4K and at 720p: spacing is set in pixels via the footprint, but line width needs a floor.',
    ],
  },
  env: {
    sun: { azimuth: 222, elevation: 34, color: '#fff0da', intensity: 3.0, shadowSoftness: 0.5, shadowStrength: 1.0 },
    ambient: { sky: '#6a9aa8', ground: '#8a5a3a', intensity: 0.55 },
    exposure: 1.0,
    toneMapping: 'aces',
    fog: { color: '#d7b48a', density: 0.0012, heightFalloff: 0.012, height: 0, start: 60, sunScatter: 0.5, sunColor: '#f2c88a', sunExponent: 8 },
    sky: {
      zenith: SKY.zenith, horizon: SKY.horizon, ground: SKY.ground, sunDisc: 1, sunGlow: 0.6, horizonFog: 0.75,
      clouds: { amount: 0.5, color: SKY.cloudLit, shade: SKY.cloudShade, scale: 1, sharpness: 1, speed: 0.004 }, stars: 0, bodies: [],
    },
    lights: { scale: 1.1 },
    particles: { dust: 0.2, embers: 0.0, emberColor: '#ffb070', motes: '#ffe2b0' },
    smoke: { opacity: 0.12 },
  },
  material(src, kit) {
    const role = src.role || 'other', name = src.name || '', master = src.master;
    if (role === 'glow') return kit.flat(src, { glowBoost: 1.4 });
    if (/Backdrop/i.test(name)) return kit.flat(src, { tint: { color: '#c08a5a', amount: 0.4 }, steps: 2 });
    if (master === 'Glass' || master === 'Smoke' || master === 'Waterfall') return kit.pbr(src, {});
    if (master === 'Water') return T(kit, src, { tint: { color: '#1f8a8a', amount: 0.6 }, hook: INK_HOOK_SOFT });
    if (role === 'creature') {
      // near-black chitin with rust in the light: a dark shape under heavy contours
      return T(kit, src, { value: 0.5, saturation: 0.9, tint: { color: '#5a2418', amount: 0.3 }, specular: { size: 0.08, strength: 1.4, color: '#ffd9a0' } });
    }
    if (master === 'Gun' || role === 'gun') return T(kit, src, { hook: INK_HOOK_GUN, saturation: 1.05, specular: { size: 0.06, strength: 1.2 } });
    if (role === 'metal') return T(kit, src, { specular: { size: 0.07, strength: 1.0 } });
    if (role === 'wood') {
      if (/Teal/i.test(name)) return T(kit, src, { tint: { color: '#1f8f8c', amount: 0.35 }, saturation: 1.25 });
      return T(kit, src, { tint: { color: '#b8703a', amount: 0.2 }, saturation: 1.15 });
    }
    if (role === 'rock') return T(kit, src, { tint: { color: '#b85a38', amount: 0.35 }, saturation: 1.15 });
    if (role === 'stone') return T(kit, src, { tint: { color: '#b8703c', amount: 0.2 }, saturation: 1.1 });
    if (role === 'grass' || name === 'FoliagePalette') {
      return kit.toon(src, Object.assign({}, BANDS, { saturation: 1.05, value: 0.9, tint: { color: '#8a9a4a', amount: 0.35 } }));
    }
    if (role === 'foliage') return T(kit, src, { hook: INK_HOOK_SOFT, saturation: 1.15, tint: { color: '#4f8a52', amount: 0.4 } });
    if (role === 'bark') return T(kit, src, { tint: { color: '#6a3420', amount: 0.3 } });
    return T(kit, src, {});
  },
  terrain(src, kit) {
    return kit.terrainMaterial(Object.assign({}, BANDS, { toon: true, hook: INK_HOOK_TERRAIN, blur: 1.2, saturation: 1.05, value: 0.88, contrast: 1.1,
      tint: { color: '#c27848', amount: 0.38 }, detailStrength: 0.8 }));
  },
  sky(kit) { return inkSky(kit, SKY); },
  post(kit) {
    const THREE = kit.THREE;
    const ink = kit.post.custom({
      fragment: INK_OUTLINE, needsDepth: true, needsNormal: true,
      uniforms: { uCamWorld: { value: new THREE.Matrix4() }, uWNear: { value: 3.2 }, uWFar: { value: 1.25 }, uBoil: { value: 1 },
        uCrease: { value: 1 }, uSilFar: { value: 520 }, uCreaseFar: { value: 100 }, uInk: { value: new THREE.Vector3(0.102, 0.071, 0.063) } },
      bind: (p, u) => { u.uCamWorld.value.copy(kit.camera.matrixWorld); },
    });
    return [
      kit.post.bloom({ threshold: 1.6, strength: 0.25, radius: 0.5 }),
      ink,
      kit.post.grade({ contrast: 1.06, saturation: 1.05, vibrance: 0.1, lift: '#0e0806', shadows: '#6a3a30', highlights: '#ffe2b0',
        splitAmount: 0.18, curve: 0.12 }),
      kit.post.vignette({ amount: 0.22, softness: 0.6, color: '#1a0c08' }),
      kit.post.grain({ amount: 0.018, size: 1.2, colored: 0.0 }),
    ];
  },
  fx: { muzzle: '#ffe46a', tracer: '#fff4d0', impact: '#ffcf4a', blood: '#8fe04a', hitFlash: '#ffffff' },
  activate(kit) {
    // the comic muzzle star on the engine's flash card (put back in deactivate)
    const flash = kit.viewmodel && kit.viewmodel.children.find((c) => c.material && c.material.uniforms && c.material.uniforms.uSeed);
    if (!flash) return;
    const m = flash.material;
    // the engine's own shader (another style's swap keeps it in slOrigFrag)
    if (!m.userData.inkSaved) m.userData.inkSaved = { fs: m.userData.slOrigFrag || m.fragmentShader, blending: m.blending, depthWrite: m.depthWrite };
    m.fragmentShader = INK_FLASH_FS;
    m.blending = kit.THREE.NormalBlending;
    m.needsUpdate = true;
  },
  deactivate(kit) {
    const flash = kit.viewmodel && kit.viewmodel.children.find((c) => c.material && c.material.uniforms && c.material.uniforms.uSeed);
    if (!flash || !flash.material.userData.inkSaved) return;
    const m = flash.material, s = m.userData.inkSaved;
    m.fragmentShader = s.fs; m.blending = s.blending; m.depthWrite = s.depthWrite;
    delete m.userData.inkSaved;
    m.needsUpdate = true;
  },
};
