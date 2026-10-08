// Style 6: Sunbleached. A high-key, luminous, mythic frontier: a blinding white-gold sun, sandstone that glows from
// within, soft lavender shadows, sand that glitters as you move, and rim light blazing on every silhouette. The spiders
// are the only dark things in a field of light.
import { rimHook, bounceHook, glitterHook, sheenHook, innerGlowHook, shadeHook } from './lib/bleached_glsl.js';
import { bleachedSky } from './lib/bleached_sky.js';

const TONE = 'aces';
const EXPOSURE = 1.05;
// The sun stands ahead and left of Main Street, high enough to light the street and low enough to sit in the frame's
// top-left corner (clear of the spiders), its ice halo arcing over the town.
const SUN = { azimuth: 64, elevation: 19 };
const SKY = {
  zenith: '#2f90c2', mid: '#7fbfd4', horizon: '#fbeed6', ground: '#e8d6bc', anti: '#c8bce8',
  cirrus: '#efebe4', cirrusShade: '#d2cde4', sun: '#fff6e6', sunBright: 16, glow: '#ffe2a8', glowAmount: 1.0,
  halo: '#fff0d8', haloAmount: 0.6, cirrusAmount: 0.75, horizonFog: 0.6,
};
const RIM = 'vec3(1.0, 0.82, 0.58)';
const BOUNCE = 'vec3(0.42, 0.32, 0.22)';
const GLOW = 'vec3(1.0, 0.7, 0.42)';
const SHADE = 'vec3(0.9, 0.84, 1.22)';

export default {
  number: 6,
  id: 'bleached',
  name: 'Sunbleached',
  family: 'Luminous',
  tagline: 'A pilgrimage across a dead saint\'s land: blinding white-gold light, lavender shade, glittering sand.',
  pitch: 'The frontier at its most mythic: a white-gold sun blazes over Main Street inside a faint ice halo, the air '
    + 'glows with luminous haze, sandstone and timber shine gold as if lit from within, every shadow is cool lavender, '
    + 'the sand glitters as you move and silhouettes burn with rim light. It is the reverence of a pilgrimage across a '
    + 'dead saint\'s land, and the spiders read as the only dark shapes in a field of light.',
  hudFit: 'The HUD\'s outlined white text and gunmetal metalwork read as the darkest values on screen; its cyan is the '
    + 'one cool accent the world leaves free, and its orange sits inside the world\'s gold.',
  refs: [
    { title: 'Sand Rendering in Journey (GDC 2013)', url: 'https://gdcvault.com/play/1017742/Sand-Rendering-in',
      note: 'the glitter specular, the "ocean" sheen toward the sun and the diffuse contrast' },
    { title: 'Journey sand shader reconstruction (Alan Zucconi)', url: 'https://www.alanzucconi.com/2019/10/08/journey-sand-shader-3/',
      note: 'how the glitter and rim terms are built' },
    { title: 'Art of Sky: Children of the Light (80.lv)', url: 'https://80.lv/articles/interview-a-deep-dive-into-the-art-of-sky-children-of-the-light-with-thatgamecompany',
      note: 'light as both the image and the theme; warmth as a reward' },
    { title: 'Designing emotional environments for Sky (80.lv)', url: 'https://80.lv/articles/how-to-design-emotional-game-environments-for-sky-children-of-the-light/',
      note: 'high-key values, silhouettes against glowing haze' },
    { title: 'Journey\'s desert, chosen to connect people (Gameranx)', url: 'https://gameranx.com/updates/id/13724/article/journey-s-desert-setting-was-chosen-to-help-players-connect-on-a-human-level/',
      note: 'why a near-monochrome sunlit world feels mythic' },
  ],
  unreal: {
    recipe: [
      'Directional light: yaw 58, 16 degrees up, white-gold #ffe6b4; soft shadows (source angle ~2); a lavender-blue sky light (#7f8cf0) so the shade reads cool against the gold, its lower hemisphere warm.',
      'Sky: an unlit sky-sphere material: white-gold horizon to pale turquoise, a lavender band opposite the sun, sheared cirrus from two panning noise samples, a wide sun glow, and the 22-degree ice halo as a thin ring (dot(view, sun) against cos 22 degrees, warm inside and cool outside).',
      'Exponential height fog: warm cream, directional inscattering toward the sun; light shafts on the directional light.',
      'Masters: one material function adds a backlit fresnel rim (white-gold), a warm wrap past the terminator (the glow from within) and a lift toward sandstone cream (albedo x1.1, saturation 0.95).',
      'M_Terrain and rock: glitter from a hashed world-space cell normal against the half vector (cell size doubling with distance), plus a broad sheen lobe toward the sun: pure ALU, no textures.',
      'Spiders stay dark (albedo x0.3) with a thin rim, so they are the darkest shapes in the frame.',
      'Post: a low-threshold, wide, gentle bloom (a diffusion-filter glow round everything bright: the luminous air), a LUT for the slight lavender lift, a light sharpen; fixed exposure.',
    ],
    costMs: '~0.4 ms (glitter/rim ALU ~0.1, light shafts ~0.2, bloom already paid)',
    risks: [
      'High-key art can wash out: the spiders and loot must stay dark and saturated (they are darkened on purpose here).',
      'Glitter can shimmer in motion; it fades out past ~70 m and its cell size grows with distance.',
      'A contemplative look may clash with gunplay; the white-gold flashes and dark enemies keep combat punchy.',
    ],
  },
  env: {
    sun: { azimuth: SUN.azimuth, elevation: SUN.elevation, color: '#ffdfa6', intensity: 4.6, shadowSoftness: 2.6, shadowStrength: 0.92 },
    ambient: { sky: '#7f8cf0', ground: '#c89a72', intensity: 1.25 },
    exposure: EXPOSURE,
    toneMapping: TONE,
    fog: { color: '#f6e8d4', density: 0.0035, heightFalloff: 0.025, start: 25, sunScatter: 0.9, sunColor: '#fff7e4', sunExponent: 6 },
    sky: {
      zenith: SKY.zenith, horizon: SKY.horizon, ground: SKY.ground, sunDisc: 1.5, sunGlow: 1.4, horizonFog: SKY.horizonFog,
      clouds: { amount: 0 }, stars: 0, bodies: [],
    },
    lights: { scale: 1.0 },
    particles: { dust: 0.2, embers: 0.2, emberColor: '#ffe7a8', motes: '#ffe0a0' },
    wind: { direction: 60, strength: 0.12, speed: 1.2 },
    smoke: { opacity: 0.16 },
  },
  material(src, kit) {
    const role = src.role || 'other';
    if (role === 'creature') {
      // the only dark things in the light: dark, desaturated, edged with a thin blazing rim
      return kit.pbr(src, { value: 0.24, saturation: 0.5, roughness: 0.8, hook: rimHook('vec3(1.0, 0.8, 0.5)', 1.1, 4.0) });
    }
    if (role === 'glow') return kit.pbr(src, { glowBoost: 1.5 });
    if (role === 'gun') return kit.pbr(src, { value: 1.0, hook: rimHook(RIM, 0.2) });
    if (role === 'foliage' || role === 'grass') {
      return kit.pbr(src, { saturation: 0.85, value: 1.12, hueShift: 6, hook: shadeHook(SHADE, 0.8) + rimHook(RIM, 0.45) });
    }
    if (src.master === 'Glass' || src.master === 'Smoke' || src.master === 'Waterfall' || role === 'water') return kit.pbr(src, { value: 1.05 });
    if (src.unlit) return kit.pbr(src, { value: 1.2, saturation: 0.7, tint: { color: '#efcfa6', amount: 0.35 } });
    const stony = role === 'rock' || role === 'stone';
    let hook = shadeHook(SHADE) + rimHook(RIM, 0.5) + bounceHook(BOUNCE, 0.15) + innerGlowHook(GLOW, 0.3);
    if (stony) hook += glitterHook(0.06, 0.012, 10.0, 50);
    return kit.pbr(src, { saturation: 0.95, value: 1.1, tint: { color: '#eec79a', amount: 0.35 }, hook });
  },
  terrain(src, kit) {
    return kit.terrainMaterial({ saturation: 1.0, value: 1.1, tint: { color: '#ecbf88', amount: 0.45 },
      hook: shadeHook(SHADE) + sheenHook(0.4) + glitterHook(0.05, 0.01, 16.0, 60, 0.45) + rimHook(RIM, 0.15)
        + bounceHook(BOUNCE, 0.1) + innerGlowHook(GLOW, 0.2) });
  },
  sky(kit) {
    return bleachedSky(kit, Object.assign({ exposure: EXPOSURE, toneMapping: TONE }, SKY));
  },
  post(kit) {
    return [
      kit.post.ao({ radius: 1.2, intensity: 0.5, color: '#3a3480' }),
      // shafts from the sun's core only (a threshold above the bright sky), so they stream past the rooflines
      // instead of washing the whole corner white
      kit.post.godrays({ strength: 0.3, decay: 0.97, density: 0.9, color: '#fff0cc', threshold: 1.6 }),
      // a low threshold and a wide, gentle kernel: a diffusion-filter glow round everything bright, the luminous air
      kit.post.bloom({ threshold: 0.95, strength: 0.35, radius: 1.0 }),
      kit.post.halation({ threshold: 1.6, radius: 1.0, color: '#ffcf8a', strength: 0.15 }),
      kit.post.grade({ contrast: 1.06, saturation: 1.12, lift: '#0e0b20' }),
      kit.post.sharpen({ amount: 0.25 }),
      kit.post.vignette({ amount: 0.12, softness: 0.6, color: '#4a3e6a' }),
    ];
  },
  fx: { muzzle: '#fff2c8', tracer: '#fffbea', impact: '#ffe9a8', blood: '#8be05a', hitFlash: '#fff6e0' },
};
