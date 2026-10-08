// Style 8, Ember Gothic: the weird west as a ghost story. Night under a huge full moon, a fog sea rolling between the
// graves and down Main Street, a cold blue-green world, and the only warm things are the lanterns, the lit windows and
// the muzzle flash, while the saints' cyan embers drift in the air.
import { makeGothicSky } from './lib/gothic_sky.js';
import { makeGothicFog } from './lib/gothic_fog.js';
import { buildLanterns, hipLantern } from './lib/gothic_lanterns.js';
import { inverseToneMap } from '../engine/tonecurve.js';
import { preloadFogGround } from './lib/gothic_fog.js';

preloadFogGround();

const EXPOSURE = 1.9;
const MOON = { azimuth: 306, elevation: 22 };

let hip = null;
const HIP = 22;    // Pa's lantern, candela

export default {
  number: 8,
  id: 'gothic',
  name: 'Ember Gothic',
  family: 'Dark weird west',
  tagline: 'A ghost story by moonlight: fog between the graves, lanterns, and the saints\' embers drifting cyan.',
  pitch: 'Ransom\'s Rest at night under a huge full moon: a low fog sea rolls down Main Street and between the graves, lit '
    + 'silver by the moon and gold by the lanterns and windows, while the saints\' cyan embers drift through it. It is the '
    + 'story\'s own mood (a revenant, a churchyard, a gravekeeper), the look where the chapel and boot hill are strongest, and '
    + 'the HUD\'s orange and cyan become the only warm and cold lights in the world.',
  hudFit: 'The best fit of all: the dark, quiet corners make the white outlined text and the orange and cyan lines glow, and the world\'s embers share the HUD\'s cyan.',
  refs: [
    { title: 'Hunt: Showdown, first impressions (PC Gamer)', url: 'https://pcgamer.com/hunt-showdown-is-just-incredible', note: 'moonlight diffused through fog, silhouettes threading gnarled trees' },
    { title: 'The development of Hunt: Showdown (80.lv)', url: 'https://80.lv/articles/the-development-of-hunt-showdown', note: 'volumetric fog that every light affects' },
    { title: 'Hunt: Showdown, Point blank through the fog', url: 'https://www.huntshowdown.com/news/point-blank-through-the-fog', note: 'fog as tension, not just distance' },
    { title: 'Bloodborne in UE4 (Simon Barle, 80.lv)', url: 'https://80.lv/articles/simon-barle-bloodborne-in-ue4', note: 'a cool base against warm point lights; never let the scene sink to black; gloss keeps it from going flat' },
    { title: 'The gothic sensibilities of Darkest Dungeon (GameSpot)', url: 'https://www.gamespot.com:443/articles/the-gothic-sensibilities-of-darkest-dungeon/1100-6424880/', note: 'no dead black or grey; one spot of unexpected colour' },
    { title: 'Lighting, Atmosphere, and Tonemapping in Ghost of Tsushima (SIGGRAPH 2021)', url: 'https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html', note: 'the Purkinje shift: night goes cooler and less saturated, not just darker' },
  ],
  unreal: {
    recipe: [
      'Directional light as the moon: 0.3 lux, 7500 K tinted #8fb0d8, 22 degrees up in the north-west (beside the chapel steeple from boot hill), cascaded shadows; a SkyLight captured from a night sky material (low, teal) at intensity 0.15.',
      'Sky sphere material: blue-black gradient, a full-moon disc with a maria texture and an aureole, thin panning cloud banks lit by moon direction, star field; a second crescent moon as a card.',
      'Two exponential height fogs: a thin one for distance (density 0.005) and a dense ground layer (falloff 0.5/m) at the valley floor with directional inscattering in the moon\'s colour; the Sink gets its own local fog volume at its floor so the pit is no soup. No volumetric fog on Medium.',
      'Fog cards: camera-facing soft particles (depth fade) with a slow panning noise, placed by the area build in streets and graves, 6-12 per view; an additive glow sprite on every lamp in place of volumetric light scattering.',
      'Lamp posts along Main Street, the road to boot hill, the graves and the chapel gate (static mesh + point light, 9 m, no shadows, the six nearest lit), window emissive at 4.5, house lamps boosted and reaching out of the doors; a flickering point light on the character for the hip lantern.',
      'Materials: one material parameter collection drives albedo desaturation (0.5) and darkening, dewy ground roughness (0.6) and a cool fresnel rim on creatures for readability.',
      'Post: SSAO, bloom (threshold 0.9), light shafts on the moon, a LUT with crushed teal shadows and a lifted fog floor, vignette, film grain. Niagara: cyan ember sprites around the camera.',
    ],
    costMs: '~1.2 ms (fog cards and the second height fog ~0.5, lamps 0.3, post 0.4)',
    risks: [
      'Dark screenshots vanish on Steam\'s dark store pages: the store shots must be composed around the moon, the fog and the lamps.',
      'Spiders in fog: the cool rim and the fog behind them must keep them as silhouettes; the gamma slider must never crush them.',
      'Light count: more than six lamps near the camera need a budget (the nearest ones only), or forward lighting cost climbs.',
      'Brown/dark shooter fatigue: the cyan embers and warm lamps carry the colour; without them it reads as murk.',
    ],
  },
  env: {
    sun: { azimuth: MOON.azimuth, elevation: MOON.elevation, color: '#8fb0d8', intensity: 1.9, shadowSoftness: 2.2, shadowStrength: 1.0 },
    ambient: { sky: '#26405a', ground: '#10161a', intensity: 0.8 },
    exposure: EXPOSURE,
    toneMapping: 'aces',
    fog: { color: '#34444f', density: 0.0045, heightFalloff: 0.06, height: 0, start: 4, sunScatter: 0.4, sunColor: '#6f87a3', sunExponent: 10 },
    sky: { zenith: '#05080f', horizon: '#26323e', ground: '#0c1014', sunDisc: 0, sunGlow: 0, horizonFog: 0.8,
      clouds: { amount: 0 }, stars: 0.8, bodies: [] },
    lights: { scale: 5 },
    particles: { dust: 0.2, embers: 0.55, emberColor: '#2fa6e6', motes: '#9fb4c8' },
    wind: { direction: 70, strength: 0.08, speed: 0.9 },
    smoke: { opacity: 0.12 },
  },
  material(src, kit) {
    const role = src.role;
    if (src.name === 'WindowGlow' || src.name === 'WindowGlow_Dark') {
      // lamplight behind the panes: most houses lit, the dark cottages only faintly
      const lit = src.name === 'WindowGlow';
      return kit.pbr(Object.assign({}, src, { glow: lit ? 4.5 : 1.3, glowColor: '#ff9f50', color: '#3a2a1c' }), {});
    }
    if (role === 'glow') return kit.pbr(src, { glowBoost: 2.2 });
    if (role === 'creature') {
      return kit.pbr(src, { saturation: 0.5, value: 0.7, rim: { color: '#a9ccff', power: 2.2, strength: 1.4 } });
    }
    if (role === 'grass') return kit.pbr(src, { saturation: 0.42, value: 0.45, hueShift: 8, translucency: 0.12 });
    if (role === 'foliage') return kit.pbr(src, { saturation: 0.42, value: 0.6, hueShift: 8, translucency: 0.18 });
    if (role === 'gun' || src.master === 'Gun') return kit.pbr(src, { saturation: 0.75 });
    if (role === 'water') return kit.pbr(src, { value: 0.6 });
    if (role === 'smoke') return kit.pbr(src, {});
    return kit.pbr(src, { saturation: 0.5, value: 0.74, normalScale: 1.2, roughness: role === 'stone' || role === 'rock' ? 0.8 : 1 });
  },
  terrain(src, kit) {
    const m = kit.terrainMaterial({ saturation: 0.45, value: 0.72, normalStrength: 1.0 });
    m.roughness = 0.62;        // dew: the ground takes a sheen toward the moon
    return m;
  },
  sky(kit) {
    const inv = (hex) => inverseToneMap(kit.color(hex), 'aces', EXPOSURE);
    const lin = (hex, k = 1) => kit.color(hex).multiplyScalar(k);
    return makeGothicSky(kit, {
      zenith: inv('#04070d'), horizon: inv('#1e2a36'), ground: inv('#0a0d10'),
      moonColor: lin('#eef2f5', 1), glowColor: lin('#8fa9c8', 0.22), moonBright: 0.9,
      cloudLit: inv('#7d8ea2'), cloudDark: inv('#141c26'),
      moonSize: 9.5, stars: 1.0, clouds: 0.34, horizonFog: 0.85,
      moon2: { azimuth: 141, elevation: 15, size: 4.2, color: lin('#cfe0d8', 1.2) },
    });
  },
  post(kit) {
    const fog = makeGothicFog(kit, {
      density: 0.056, falloff: 0.5, base: -0.3, maxDist: 150, noiseScale: 0.05, noiseContrast: 1.8, wind: [0.45, 0.15],
      moonColor: kit.color('#8fa6c0').multiplyScalar(0.45), ambColor: kit.color('#26405a').multiplyScalar(0.5),
      lampScatter: 0.28,
    });
    return [
      kit.post.ao({ radius: 1.3, intensity: 0.85 }),
      fog,
      kit.post.bloom({ threshold: 1.2, strength: 0.32, radius: 0.8 }),
      kit.post.godrays({ strength: 0.14, decay: 0.972, density: 0.9, color: '#a6bfe0', threshold: 0.9 }),
      kit.post.grade({ contrast: 1.08, saturation: 0.92, curve: 0.18, lift: '#060c10', shadows: '#2a5866', highlights: '#c2cdd6',
        splitAmount: 0.3, temperature: -0.18 }),
      kit.post.vignette({ amount: 0.5, softness: 0.6, color: '#020406' }),
      kit.post.grain({ amount: 0.035, size: 1.4, colored: 0.12 }),
    ];
  },
  fx: { muzzle: '#ffd49a', tracer: '#ffe1ae', impact: '#ffc07a', blood: '#9be35a', hitFlash: '#d8ecff' },
  activate(kit) {
    // house lamps reach out of the windows and doors as pools on the street
    for (const s of kit.lights) { s.range = Math.max(s.range * 1.8, 6); s.colorOverride = kit.color('#ffa458'); }
    for (const l of buildLanterns(kit, { intensity: 2.6, range: 9 })) kit.lights.push(l);
    // the engine picks the lit lamps nearest the camera; ask it to pick again from the new list
    if (kit.sun) kit.sun._chosen = null;
    hip = hipLantern(kit, { intensity: HIP, distance: 20 });
    kit.camera.add(hip);
  },
  deactivate(kit) {
    if (hip && hip.parent) hip.parent.remove(hip);
    hip = null;
  },
  update(dt, t) {
    // the hip lantern's flame breathes and gutters a little as Pa walks
    if (hip) hip.intensity = HIP * (0.9 + 0.06 * Math.sin(t * 7.3) + 0.04 * Math.sin(t * 17.9 + 1.3));
  },
};
