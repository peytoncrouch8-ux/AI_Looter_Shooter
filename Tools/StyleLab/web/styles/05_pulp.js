// Style 5: Teropa Pulp. The cover of a 1970s science-fiction paperback come alive: a ringed gas giant fills the eastern
// sky over the frontier town, two small moons, an airbrushed teal-to-tangerine dome, sienna and ochre ground, turquoise
// alien flora, chrome-bright metal and a tangerine sun against teal shade everywhere.
import { pulpSky } from './lib/pulp_sky.js';

const SKY = {
  zenith: '#0a4352', mid: '#2f9c9c', low: '#f1c7a0', horizon: '#ff9e66', belt: '#f0a3a0', ground: '#6e4132',
  streakLit: '#ffdcbc', streakShade: '#c88a88', haze: 0.85, stars: 0.6, streaks: 1.0,
  planet: {
    azimuth: 110, elevation: 10, size: 34, roll: -12, ringTilt: -19, ring: [1.28, 2.45], light: [-0.5, 0.62, 0.6],
    brightness: 0.95, veil: 0.4, ringOpacity: 0.95, ringBrightness: 0.95,
    bands: ['#f2cf98', '#ea9660', '#d27a2c', '#9c3219', '#40130c', '#25877f'],
    ringColors: ['#f4ddb4', '#d98c58', '#6f9f99'],
  },
  moons: [
    { azimuth: 58, elevation: 17, size: 4.6, color: '#efe0cc', color2: '#a08c80' },
    { azimuth: 314, elevation: 20, size: 7.5, color: '#d9ebe4', color2: '#86a3a2' },
  ],
};

// Albedo pushes per role: toward the cover's sienna, ochre and teal.
const SIENNA = '#c06a3c', OCHRE = '#d39a48', TEAL = '#2f948f';

// The cover's light: whatever turns from the sun or sits in shadow drifts toward teal at its own lightness, so the
// tangerine key always meets a teal shade (an artist's shadow colour, not a physical one).
const SHADE = /* glsl */`
{
  float litK = shadow * smoothstep(-0.05, 0.4, NdL);
  vec3 teal = vec3(0.028, 0.2, 0.29);
  col = mix(col, teal * (slLuma(col) / slLuma(teal)), (1.0 - litK) * 0.42);
}`;
const P = (kit, src, o) => kit.pbr(src, Object.assign({ hook: SHADE }, o));

export default {
  number: 5,
  id: 'pulp',
  name: 'Teropa Pulp',
  family: 'Retro sci-fi',
  tagline: 'A 1970s sci-fi paperback cover you can walk into: a ringed giant over the frontier.',
  pitch: 'An enormous banded gas giant hangs over Main Street with its rings slashing across the sky and two small moons '
    + 'beside it, in an airbrushed dome that runs from deep teal to tangerine. The ground is sienna and ochre, the trees are '
    + 'alien turquoise, metal shines like chrome, and a low tangerine sun throws long shadows into teal shade. One glance says '
    + '"another planet": the hook no other western has.',
  hudFit: 'The HUD\'s own complementary pair is the whole palette here (tangerine light, teal shade), and its retro-futurist '
    + 'gunmetal and Chakra Petch sit naturally under a paperback sky; the bright giant is kept off the corners\' text.',
  refs: [
    { title: 'Chris Foss (SF Encyclopedia)', url: 'https://sf-encyclopedia.com/entry/foss_chris',
      note: 'airbrushed gradients, vivid backgrounds, huge forms over landscapes with tiny people' },
    { title: 'The art of No Man\'s Sky', url: 'https://www.nomanssky.com/2016/04/art-of-no-mans-sky/',
      note: 'a science-fiction book cover come to life; Foss, McQuarrie and Moebius as anchors' },
    { title: 'How to paint explosive environments (Foss, Berkey, Elson colour)', url: 'https://www.creativebloq.com/features/how-to-paint-explosive-environments',
      note: 'the saturated complementary palette of 70s cover art' },
    { title: 'Dissecting the art style of Outer Wilds', url: 'https://80.lv/articles/dissecting-the-art-style-of-outer-wilds/',
      note: 'hard sunlight with clear forms instead of painterly noise; 60s-70s space imagery' },
    { title: '1970s NASA space-settlement paintings', url: 'https://anothermag.com/art-photography/7711/1970s-nasa-paintings',
      note: 'planets and moons as huge, calm presences in a painted sky' },
  ],
  unreal: {
    recipe: [
      'Sky: one unlit sky-sphere material (M_Sky_Pulp). A four-stop gradient on the camera vector\'s Z, a pink band opposite the sun, and the gas giant computed from dot(CameraVector, PlanetDir): a disc in its own frame, bands from a small 1D band LUT warped by a tiling noise texture, the ring from a ray-plane intersection in the same frame with a radial ring LUT, the planet\'s shadow on the ring and the ring\'s on the planet (one more ray-plane test each), two moon discs. About 180 instructions, on sky pixels only.',
      'Light: a directional sun at 18 degrees, tangerine (about 3000 K plus an orange filter), 4.5 lux; a Sky Light from a captured cubemap of that sky gives the teal fill. No SkyAtmosphere.',
      'Fog: Exponential Height Fog, peach (#e3a48a), density about 0.02, falloff 0.12, start 25 m, directional inscattering tangerine with exponent 6: far ridges layer into flat peach and teal silhouettes.',
      'Materials: the existing masters with a palette push per instance (Tint toward sienna for wood, rock and ground; the foliage TintVariation turned toward turquoise), a +1.5 mip bias on base colour for the smoothed airbrush look, metal roughness x0.5 so chrome catches the sky.',
      'Spiders: a darker violet-black chitin instance with a low roughness so they read against the ochre ground.',
      'Post: Bloom (standard, threshold 1.1), a LUT with lifted teal blacks and warm highlights, Film Grain 0.03, Vignette 0.32.',
    ],
    costMs: '~0.25 ms (the sky material and the LUT; no extra passes)',
    risks: [
      'The giant can steal attention in combat: keep its brightness under the sun\'s and never put it behind the crosshair at spawn points.',
      'Palette pushes can go muddy in the mid-tones; needs a pass per texture set rather than one global tint.',
      'The sky maths in a material must be checked for precision near the ring\'s edge at 4K; a baked 4K planet texture on a billboard is the fallback.',
      'Turquoise flora reads strongly alien: decide per biome how far it goes.',
    ],
  },
  env: {
    sun: { azimuth: 258, elevation: 18, color: '#ffa461', intensity: 4.6, shadowSoftness: 1.4, shadowStrength: 1.0 },
    ambient: { sky: '#3a8eaa', ground: '#9a5638', intensity: 1.5 },
    exposure: 1.05,
    toneMapping: 'aces',
    fog: { color: '#e2a58e', density: 0.0017, heightFalloff: 0.011, height: 0, start: 22, sunScatter: 0.9, sunColor: '#ffb070', sunExponent: 6 },
    sky: {
      zenith: SKY.zenith, horizon: SKY.horizon, ground: SKY.ground, sunDisc: 1, sunGlow: 1, horizonFog: 0.85,
      clouds: { amount: 0 }, stars: 0.6,
      bodies: [{ kind: 'planet', azimuth: SKY.planet.azimuth, elevation: SKY.planet.elevation, size: SKY.planet.size,
        color: SKY.planet.bands[0], color2: SKY.planet.bands[3], ring: { color: SKY.planet.ringColors[0], tilt: SKY.planet.ringTilt } }],
    },
    lights: { scale: 1.2 },
    particles: { dust: 0.35, embers: 0.0, emberColor: '#ffb070', motes: '#ffd2a8' },
    smoke: { opacity: 0.2 },
  },
  material(src, kit) {
    const role = src.role || 'other', name = src.name || '', master = src.master;
    const base = { blur: 1.5, saturation: 1.15, contrast: 1.04 };
    if (role === 'glow') return kit.pbr(src, { glowBoost: 1.5 });
    if (/Backdrop/i.test(name)) return P(kit, src, { tint: { color: '#c98a7a', amount: 0.55 }, saturation: 0.9 });
    if (master === 'Glass' || master === 'Smoke' || master === 'Waterfall') return kit.pbr(src, {});
    if (master === 'Water') return P(kit, src, { tint: { color: TEAL, amount: 0.6 }, envIntensity: 1.6 });
    if (role === 'creature') {
      // violet-black chitin with a hard sheen and a teal rim from the sky: dark shapes on ochre ground
      return P(kit, src, { blur: 1, value: 0.42, saturation: 0.7, tint: { color: '#4a2a52', amount: 0.55 }, roughness: 0.55,
        envIntensity: 1.4, rim: { color: '#5fc7c0', power: 3.5, strength: 0.55 } });
    }
    if (role === 'gun' && master === 'Gun') {
      return P(kit, src, { blur: 0.8, saturation: 1.1, roughness: 0.6, envIntensity: 1.7 });
    }
    if (role === 'metal') {
      const rusty = /Rust|Oxide|Olive/i.test(name);
      return P(kit, src, Object.assign({}, base, rusty ? { tint: { color: SIENNA, amount: 0.25 } } : { roughness: 0.5, envIntensity: 1.9 }));
    }
    if (role === 'wood') {
      if (/Teal|Oxide|Cream|Black/i.test(name)) return P(kit, src, Object.assign({}, base, { saturation: 1.3 }));
      return P(kit, src, Object.assign({}, base, { tint: { color: SIENNA, amount: 0.28 }, saturation: 1.25 }));
    }
    if (role === 'rock') return P(kit, src, Object.assign({}, base, { blur: 1.2, tint: { color: '#c56a3e', amount: 0.4 }, saturation: 1.2 }));
    if (role === 'stone') return P(kit, src, Object.assign({}, base, { tint: { color: '#c28e64', amount: 0.3 } }));
    if (role === 'ground') return P(kit, src, Object.assign({}, base, { tint: { color: OCHRE, amount: 0.3 } }));
    if (role === 'bark') return P(kit, src, Object.assign({}, base, { tint: { color: '#8a4630', amount: 0.35 } }));
    if (role === 'foliage' || role === 'grass') {
      if (name === 'FoliagePalette' || name === 'Hay') {
        // grass, tufts and flowers: dry gold with a teal cast in the shade
        return P(kit, src, { blur: 1, saturation: 1.15, hueShift: -6, tint: { color: '#c79a4a', amount: 0.35 } });
      }
      if (name === 'FoliageSage') return P(kit, src, { blur: 1, hueShift: 55, saturation: 0.9, value: 1.1 });
      // the trees: alien turquoise
      return P(kit, src, { blur: 1.2, tint: { color: '#1f9c94', amount: 0.85 }, saturation: 1.15, value: 1.1, translucency: 0.8 });
    }
    if (role === 'fabric' || role === 'paper') return P(kit, src, Object.assign({}, base, { saturation: 1.2 }));
    return P(kit, src, base);
  },
  terrain(src, kit) {
    return kit.terrainMaterial({ hook: SHADE, blur: 1, saturation: 1.15, value: 0.95, tint: { color: '#c46e3e', amount: 0.52 }, contrast: 1.05, detailStrength: 0.5 });
  },
  sky(kit) { return pulpSky(kit, SKY); },
  post(kit) {
    return [
      kit.post.ao({ radius: 1.2, intensity: 0.55 }),
      kit.post.bloom({ threshold: 1.1, strength: 0.32, radius: 0.85 }),
      kit.post.grade({ contrast: 1.08, saturation: 1.1, vibrance: 0.15, lift: '#0d2629', shadows: '#2c8a8a', highlights: '#ffb27a',
        splitAmount: 0.3, curve: 0.12 }),
      kit.post.vignette({ amount: 0.3, softness: 0.55, color: '#1c1020' }),
      kit.post.grain({ amount: 0.028, size: 1.3, colored: 0.15 }),
    ];
  },
  fx: { muzzle: '#ffc070', tracer: '#fff0c8', impact: '#ffb36a', blood: '#6fe3c4', hitFlash: '#ffe8c8' },
};
