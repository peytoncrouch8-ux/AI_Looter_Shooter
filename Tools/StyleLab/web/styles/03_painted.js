// Style 3: Painted Frontier. Chunky, sunny, hand-painted adventure: every surface brushed by hand with the light
// already painted in (warm tops, cool undersides, dark feet, caught edges), bold sun-baked reds, ochres and teal, and
// blue-violet shade. Art direction: the orchestrator's style_briefs.md ("3. Painted Frontier"); research section D.
import { makeToonSky } from './lib/anime_sky.js';
import { paintHook, paintGroundHook, groundHeights, groundRecolor, biasedNormalChunk } from './lib/painted_shaders.js';
import { edgePaint } from './lib/painted_edges.js';
import { swapFlash, restoreFlash, PAINTED_FLASH } from './lib/clay_flash.js';

const ENV = {
  // late-morning warm sun; shade tinted blue-violet, never black
  sun: { azimuth: 165, elevation: 42, color: '#ffe0ae', intensity: 4.3, shadowSoftness: 1.6, shadowStrength: 0.95 },
  ambient: { sky: '#8fa8ee', ground: '#c89060', intensity: 2.05 },
  exposure: 1.06,
  toneMapping: 'neutral',
  fog: { color: '#bcd0e6', density: 0.0018, heightFalloff: 0.014, height: 0, start: 35, sunScatter: 0.45, sunColor: '#ffe2b8', maxOpacity: 0.85 },
  sky: { zenith: '#2a6fcf', horizon: '#b7dcf0', ground: '#a08460', sunDisc: 1, sunGlow: 1, horizonFog: 0.5, clouds: { amount: 0 }, stars: 0, bodies: [] },
  lights: { scale: 1.1 },
  particles: { dust: 0.3, embers: 0.0, emberColor: '#ffd08a', motes: '#ffe8c0' },
  wind: { direction: 60, strength: 0.13, speed: 1.3 },
  smoke: { opacity: 0.4 },
};

export default {
  number: 3,
  id: 'painted',
  name: 'Painted Frontier',
  family: 'Hand-painted',
  tagline: 'Chunky, sunny, hand-painted adventure with the light brushed into every plank.',
  pitch: 'Every surface looks brushed by hand with the light already painted in: sun-warmed tops, cool undersides, dark '
    + 'feet, caught edges, colour grouped into a few confident values. Bold sun-baked reds, ochres and teal under a rich '
    + 'blue sky of big soft painted clouds; shade goes blue-violet, never grey. The friendliest and most readable option, '
    + 'a frontier storybook that still looks expensive.',
  hudFit: 'Clean colour blocks keep every corner calm for the floating text, and the gunmetal HUD reads as the premium metal layer over a painted world.',
  refs: [
    { title: 'GDC18: Visual adventures on Sea of Thieves (80.lv)', url: 'https://80.lv/articles/gdc18-visual-adventures-on-sea-of-thieves/', note: 'simplified structures, hand-painted textures, wear and repairs' },
    { title: 'Stylized graphics: Fortnite and Sea of Thieves (Habrador)', url: 'https://blog.habrador.com/2018/08/stylized-graphics-fortnite-sea-of-thieves.html', note: 'how painted materials carry the light' },
    { title: 'Designing Overwatch (Cook and Becker)', url: 'https://cookandbecker.com/en/article/378/designing-overwatch.html', note: 'handcrafted textures; painted bevels make a comic rim light' },
    { title: 'Illustrative rendering in Team Fortress 2 (Valve)', url: 'https://cdn.steamstatic.com/apps/valve/2008/GDC2008_StylizationWithAPurpose_TF2.pdf', note: 'warm/cool hue shifts, rim light and value grouping for readability' },
    { title: 'Valorant\'s art style (Inverse)', url: 'https://inverse.com/gaming/valorant-art-style-interview-moby-francke', note: 'illustrative, legible on any hardware' },
  ],
  unreal: {
    recipe: [
      'Repaint the texture sets in the generator: bigger shapes, 3-4 value groups per material, baked top-light and bevel highlights in the albedo (no photographic noise).',
      'M_World adds, per pixel: a world-normal warm/cool split (tops x warm, undersides x cool), painted bevels from the normal map\'s up-facing detail, a gradient by height above the landscape (a Runtime Virtual Texture of the terrain height, or the object\'s bounds), and a tiling world-aligned brush-stroke mask (3 tones) plus a low-frequency hue/value blotch texture.',
      'Shading: Default Lit with roughness 0.85, specular 0.2; a warm Fresnel rim; shadow colour pushed blue-violet with the sky light tint (#8fa8ee) and a post-process shadow tint from the lighting/base-colour ratio.',
      'Edges: bake curvature into vertex colour or a mask and brighten convex edges in the material (cheaper and steadier than the lab\'s screen-space curvature pass).',
      'Light: sun at 40 degrees, 4.3 lux warm; sky light 1.6; GTAO with a violet tint (post-process AO colour).',
      'Sky: painted panorama sky sphere with soft brushed cumulus; height fog light blue for depth.',
    ],
    costMs: '~0.3 ms (material maths only; curvature baked)',
    risks: [
      '"I have seen this": the Fortnite/Overwatch lineage. The frontier palette, the spiders and the weird-west props must carry the identity.',
      'Too-flat textures read as a mobile game; the value bands and blotches need the repainted sets, not just a blur.',
      'A cartoon tone can fight the haunted story unless the shapes lean spooky in the dark areas.',
    ],
  },
  env: ENV,

  material(src, kit) {
    const role = src.role || 'other';
    const m = src.master;
    if (m === 'Smoke' || m === 'Waterfall') return kit.pbr(src, { saturation: 0.6, value: 1.2 });
    if (m === 'Glass') return kit.pbr(src, { roughnessValue: 0.1, envIntensity: 1.3, rim: { color: '#ffffff', power: 2.5, strength: 0.4 } });
    if (m === 'Water') return kit.pbr(src, { albedo: 'flat', color: '#2f9fae', roughnessValue: 0.1, envIntensity: 1.1 });
    if (role === 'glow' || m === 'Glow') return kit.pbr(src, { albedo: 'flat', glowBoost: 1.5 });
    if (m === 'Flat' && src.unlit) return kit.pbr(src, { saturation: 1.15, tint: { color: '#8aa8d8', amount: 0.45 }, value: 1.1 });
    const H = groundHeights(kit);
    const extra = { pars: H.pars, uniforms: H.uniforms, normal: biasedNormalChunk(kit.THREE, 1.5), key: 'paintedH15' };
    // per role: how hard the values group, the saturation the palette wants
    const sat = { wood: 1.45, rock: 1.35, stone: 1.2, metal: 1.25, foliage: 1.4, grass: 1.35, bark: 1.3, creature: 1.0 }[role] ?? 1.3;
    const opts = {
      blur: 1.6, saturation: src.set === 'HouseTrim' ? 1.6 : role === 'wood' ? 1.25 : sat, contrast: 1.1, value: role === 'wood' ? 1.75 : 1.2, normalScale: 1.0,
      tint: role === 'wood' && src.set !== 'HouseTrim' ? { color: '#b8783e', amount: 0.3 } : undefined, roughness: 1.25, envIntensity: 0.35,
      aoStrength: 1.1, rim: { color: '#ffd9a8', power: 3.0, strength: 0.4 }, _extra: extra,
      hook: paintHook({ heights: true, bands: 4, bandAmount: 0.7, blotch: 0.26, feet: 0.74 }),
    };
    if (src.alphaMask) {
      // leaves, webs, posters: keep the cut-outs crisp, paint the colour
      opts.blur = 0.6; opts.hook = paintHook({ heights: true, bands: 0, blotch: 0.3, feet: 0.75, feetHeight: 6 });
    }
    if (role === 'foliage' || role === 'grass') {
      opts.hueShift = 4; opts.hook = paintHook({ heights: true, bands: 3, blotch: 0.32, feet: 0.7, feetHeight: 5,
        warmTop: [1.22, 1.14, 0.78], coolUnder: [0.55, 0.65, 0.95] });
    }
    if (role === 'creature') {
      // a dark wine-black with a bold warm rim and a cool kicker: reads at any range
      const pale = src.name.includes('Pale');
      Object.assign(opts, { albedo: 'flat', color: pale ? '#b6a8a0' : '#3a1f24', normalScale: 0.8,
        rim: { color: '#ffc58a', power: 2.4, strength: 0.75 }, roughnessValue: 0.55,
        hook: paintHook({ heights: true, bands: 0, blotch: 0.15, feet: 0.8, feetHeight: 1.2 }) });
    }
    if (role === 'gun' || m === 'Gun') {
      Object.assign(opts, { blur: 1.0, roughness: 0.9, envIntensity: 0.6, hook: paintHook({ heights: false, bands: 3, blotch: 0.12 }) });
    }
    if (role === 'metal') Object.assign(opts, { roughness: 0.85, envIntensity: 0.7 });
    // sandstone painted in warm reds and ochres, the shade going violet
    if (role === 'rock') Object.assign(opts, { tint: { color: '#cf7a4c', amount: 0.35 }, value: 1.3, saturation: 1.3 });
    return kit.pbr(src, opts);
  },

  terrain(src, kit) {
    return kit.terrainMaterial({
      macroStrength: 1, detailStrength: 0.35, normalStrength: 0.35, steepNormal: 0.5, blur: 2,
      steepTint: '#d49a6c', steepDetail: 0.9,
      // sun-baked sienna dirt, olive-teal grass, red sandstone
      hook: groundRecolor(kit, { dirt: '#d9b98c', grass: '#7fa848', rock: '#cf7d50', keep: 0.6 }) + paintGroundHook(),
    });
  },

  sky(kit) {
    return makeToonSky(kit, ENV, {
      zenith: '#2f74cc', mid: '#62a6dc', midAt: 0.3, power: 0.7, horizon: '#d2e8e2', ground: '#a08460', haze: 0.45,
      sun: { size: 1.6, disc: 18, glow: 0.9, glare: 0 },
      towers: { count: 8, height: 1.1, width: 1.1, seed: 9 },
      floaters: { amount: 0.48, size: 0.1, low: 13, high: 52 },
      cel: { edge: 0.0, softness: 0.22, wrap: 0.4, rim: 0.3 }, topLight: 0.35, mass: 0.65,
      cloud: { lit: '#fff4e2', shade: '#b8b0d6', dark: '#a08cb8', rim: '#ffe6c0' },
      felt: 0, brush: 0.16, warp: 0.012, backdropShadow: 0, drift: 0.0018, bottomDark: 0.6, sunBleed: 0.35, edgeSoft: 0.08,
    });
  },

  post(kit) {
    return [
      kit.post.ao({ radius: 1.6, intensity: 0.85, power: 1.5, color: '#4a3070' }),
      edgePaint(kit, { worldWidth: 6, minWidth: 1, maxWidth: 3, convex: 1.0, concave: 0.45, litBias: 0.25, tint: '#ffe9c4', creaseTint: '#3c2a5c', fadeFar: 110 }),
      kit.post.bloom({ threshold: 1.3, strength: 0.3, radius: 0.8 }),
      kit.post.grade({ contrast: 1.07, saturation: 1.14, vibrance: 0.3, temperature: 0.03, lift: '#0c0816', gain: '#fffaf0', curve: 0.1 }),
      kit.post.vignette({ amount: 0.18, softness: 0.65, color: '#1c1428', roundness: 0.7 }),
    ];
  },

  fx: { muzzle: '#ff9a2e', tracer: '#fff0b0', impact: '#ffb648', blood: '#a6f04a', hitFlash: '#fff2d6' },

  // a fat painted burst in place of the engine's star
  activate(kit) { swapFlash(kit, PAINTED_FLASH); },
  deactivate(kit) { restoreFlash(kit); },
};
