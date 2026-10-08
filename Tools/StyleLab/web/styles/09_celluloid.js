// Style 9, Celluloid West: the frontier shot on 1960s widescreen film. Saturated dye reds and deep polarised blues,
// inky crushed shadows, a red-orange halation round every highlight, grain that jumps at 24 frames a second and a
// whisper of gate weave: a spaghetti western you play, on the game's own models and textures.
import { streak, print, projector } from './lib/celluloid_film.js';

export default {
  number: 9,
  id: 'celluloid',
  name: 'Celluloid West',
  family: 'Film stock',
  tagline: 'A 1960s widescreen western you play: dye-rich reds and blues, inky shadows, halation and dancing grain.',
  pitch: 'Every frame looks printed on 1960s film stock: a hard afternoon sun, a deep polarised sky, dye-transfer reds and '
    + 'ochres against crushed inky shadows, a warm halation glow round the highlights, and real grain and gate weave. It is '
    + 'instantly "a western", it is built from the grade and the lens rather than new art, and almost no game commits to it.',
  hudFit: 'The HUD stays crisp and digital over a soft, grainy image, which separates it cleanly; the film\'s warm palette rhymes with the orange, and the sky\'s blue keeps the cyan honest.',
  refs: [
    { title: 'A Fistful of Dollars, cinematography analysis (Color Culture)', url: 'https://colorculture.org/a-fistful-of-dollars-cinematography-analysis/', note: 'harsh sun, burnt umber, dust as diffusion' },
    { title: 'For a Few Dollars More, cinematography analysis (Color Culture)', url: 'https://colorculture.org/for-a-few-dollars-more-cinematography-analysis/', note: 'hard light and the earthy palette' },
    { title: 'Spaghetti western genre look (Melies)', url: 'https://melies.co/cinematic-techniques/genre-looks/spaghetti-western', note: 'bleached sky, dry highlights, grit' },
    { title: 'Once Upon a Time in the West 4K review (The Digital Bits)', url: 'https://digitalbits.com/item/once-upon-a-time-in-the-west-2024-4k-uhd', note: 'Techniscope grain and the dye-transfer softness' },
    { title: 'TXC stock film emulator for UE5 (Unreal forums)', url: 'https://forums.unrealengine.com/t/txc-stock-film-emulator-physical-based-film-emulation/2711083', note: 'the film stage in UE\'s post chain: colour before tonemap, multi-radius halation, grain by format' },
    { title: 'Time-varying film grain (gamedev.net)', url: 'https://gamedev.net/shaderlab/57-time-varying-film-grain/', note: 'luminance-weighted grain that jumps per frame' },
    { title: 'Red Dead Revolver review (GamesRadar)', url: 'https://www.gamesradar.com/uk/red-dead-revolver-review', note: 'a game that wore its Leone debt on its sleeve: grain, bleached film' },
  ],
  unreal: {
    recipe: [
      'Directional light 9 lux at 5200 K, 35 degrees up from the south-west, contact shadows on; SkyLight low (0.6) so shade stays deep.',
      'SkyAtmosphere tuned for a deep blue zenith (Rayleigh scattering up, Mie low), a cloud card layer with crisp cumulus; height fog thin and dusty at the horizon only.',
      'Tonemapper: the film slope/toe/shoulder set project-wide (toe 0.7, shoulder 0.3), then a LUT baked from this style\'s print pass (dye crosstalk, per-channel curves, greens to olive).',
      'Halation: the bloom pass with a red-orange tint (two kernels, threshold 0.9) or a post material reusing the bloom mips.',
      'One post-process material after tonemapping: gate weave (sub-pixel UV offset per 24 fps frame), dye-cloud grain at 24 fps weighted to the mid tones, rare dust specks, a warm vignette, a horizontal streak sampled from the bloom output.',
      'Materials untouched apart from a material parameter collection: saturation 1.12, reds and ochres up a little.',
    ],
    costMs: '~0.4 ms (LUT free, halation 0.15, the projector material 0.2)',
    risks: [
      'Grain and halation soften enemy edges at range: grain is kept fine and weighted away from the darks where spiders sit.',
      'Grain fights TSR and video compression on store trailers; keep a "clean" toggle for capture.',
      'Gate weave must stay sub-pixel or it reads as a bug; never apply it to the HUD.',
      'Leone homage risk: emulate the stock and the lensing only; no music, framing or titles borrowed.',
    ],
  },
  env: {
    sun: { azimuth: 214, elevation: 36, color: '#fff0d8', intensity: 6.2, shadowSoftness: 0.6, shadowStrength: 1.0 },
    ambient: { sky: '#6f8fc4', ground: '#7a5a3c', intensity: 1.0 },
    exposure: 1.0,
    toneMapping: 'none',
    fog: { color: '#c8c2b2', density: 0.0007, heightFalloff: 0.02, height: 0, start: 120, sunScatter: 0.3, sunColor: '#efd7b0', sunExponent: 8 },
    sky: { zenith: '#082a72', horizon: '#5b86ba', ground: '#7a6a55', sunDisc: 1, sunGlow: 0.6, horizonFog: 0.3, brightness: 1,
      clouds: { amount: 0.36, color: '#ffffff', shade: '#8b97ad', scale: 0.7, sharpness: 0.82, speed: 0.003 },
      stars: 0, bodies: [] },
    lights: { scale: 1.0 },
    particles: { dust: 0.35, embers: 0.0, emberColor: '#ffb070', motes: '#ffe2b8' },
    wind: { direction: 40, strength: 0.14, speed: 1.6 },
    smoke: { opacity: 0.2 },
  },
  material(src, kit) {
    const role = src.role;
    if (role === 'glow') return kit.pbr(src, {});
    if (role === 'creature') return kit.pbr(src, { saturation: 1.05, value: 0.8, rim: { color: '#a8c4ec', power: 3, strength: 0.45 } });
    if (role === 'foliage' || role === 'grass') return kit.pbr(src, { saturation: 0.95, hueShift: -6, value: 0.95 });
    if (role === 'wood' || role === 'fabric' || role === 'paper') return kit.pbr(src, { saturation: 1.25, hueShift: -6, contrast: 1.06 });
    if (role === 'gun' || src.master === 'Gun') return kit.pbr(src, {});
    return kit.pbr(src, { saturation: 1.12, contrast: 1.04 });
  },
  terrain(src, kit) {
    return kit.terrainMaterial({ saturation: 0.98, hueShift: -2, contrast: 1.06 });
  },
  post(kit) {
    return [
      kit.post.ao({ radius: 1.1, intensity: 0.7 }),
      kit.post.halation({ threshold: 0.32, radius: 0.55, color: '#ff4a1c', strength: 1.8 }),
      streak(kit, { threshold: 5, strength: 0.45, spread: 10, color: '#ffe0bc' }),
      kit.post.bloom({ threshold: 0.9, strength: 0.22, radius: 0.85 }),
      print(kit, { exposure: 1.0, crosstalk: 0.3, toe: 0.86, shoulder: 0.66, mid: 0.4, olive: 1, saturation: 1.1,
        offset: [0.06, 0.0, -0.03], black: '#120b07' }),
      projector(kit, { weave: 1, flicker: 0.02, soften: 0.85, grain: 0.095, grainSize: 2.0, specks: 1 }),
      kit.post.vignette({ amount: 0.55, softness: 0.8, color: '#120a04', roundness: 0.8 }),
    ];
  },
  fx: { muzzle: '#fff1c4', tracer: '#fff4d6', impact: '#ffd88a', blood: '#9be35a', hitFlash: '#fffaf0' },
};
