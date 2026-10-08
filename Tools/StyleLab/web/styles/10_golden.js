// Style 10, Golden Hour: grounded cinematic realism, the "big studio" option. A low golden sun rakes down Main Street,
// long blue shadows run toward the camera, dust hangs in the air lit like gold, sun shafts break between the false
// fronts and the valley layers away into warm haze; a rich, natural grade with the saints' cyan as the only cool.
import { makeFlare } from './lib/golden_flare.js';
import { makeRays } from './lib/golden_rays.js';

export default {
  number: 10,
  id: 'golden',
  name: 'Golden Hour',
  family: 'Cinematic realism',
  tagline: 'A low gold sun down Main Street: long blue shadows, dust lit like gold, layered haze to the horizon.',
  pitch: 'The realistic end done the way the best open-world games do it: the light is the art. A sun twelve degrees above the '
    + 'canyon rim backlights the town, throws long cool shadows toward the player, catches every mote of dust and rims '
    + 'every silhouette in gold, while the ridges fade in warm layers behind. Same textures as today, but with light, air '
    + 'and drama a trailer can be cut from.',
  hudFit: 'The HUD\'s orange is the sun\'s own colour and its cyan the only cool in the frame; the shadowed foreground keeps the bottom corners dark and calm under the text.',
  refs: [
    { title: 'Lighting, Atmosphere, and Tonemapping in Ghost of Tsushima (SIGGRAPH 2021)', url: 'https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html', note: 'one sky drives clouds, haze and fog; artist-controlled deviation from physical; local tonemapping' },
    { title: 'Ghost of Tsushima art director interview (Red Bull)', url: 'https://redbull.com/us-en/ghost-of-tsushima-art-director-interview-jason-connell', note: '"90% of the colour and mood comes from your upper hemisphere"' },
    { title: 'Creating the Atmospheric World of Red Dead Redemption 2 (SIGGRAPH 2019)', url: 'https://advances.realtimerendering.com/s2019/index.htm', note: 'sky, fog and light shafts as the frame\'s backbone' },
    { title: 'Graphics study: Red Dead Redemption 2 (imgeself)', url: 'https://imgeself.github.io/posts/2020-06-19-graphics-study-rdr2/', note: 'sky-lit ambient, a separate volumetric fog and light-shaft stage' },
    { title: 'Creating a new time of day for Hunt: Showdown', url: 'https://www.huntshowdown.com/news/creating-a-new-time-of-day-for-hunt-showdown', note: 'a realistic base image graded to a bold golden look, not "too tropical"' },
    { title: 'Mad Max: Fury Road, the colourist (FilmLight)', url: 'https://filmlight.ltd.uk/customers/meet-the-colourist/eric_whipp.php', note: 'grade-built richness on neutral art: warm sand against a strong blue sky' },
    { title: 'Greig Fraser on Dune: Part Two (British Cinematographer)', url: 'https://britishcinematographer.co.uk/greig-fraser-asc-acs-dune-part-two/', note: 'hard desert light, open shade, haze for depth' },
  ],
  unreal: {
    recipe: [
      'Directional light 10 lux at 3600 K, 12 degrees up in the east-north-east (just left of Main Street\'s axis, peeking past the false fronts), cascaded shadows to 200 m; light shafts (bloom + occlusion) on, their bloom scaled down for near pixels by a post material so close enemies stay silhouettes.',
      'SkyAtmosphere for the sky and aerial perspective (Mie anisotropy 0.85 for the glow round the sun), a cirrus cloud card lit by the sun colour; SkyLight real-time capture off, a baked capture for this time of day.',
      'Exponential height fog with directional inscattering (exponent 6, warm), start 20 m, so the ridges layer into the haze.',
      'Materials unchanged; normal intensity 1.25 and per-role roughness via a material parameter collection; a warm fresnel on creatures.',
      'Post: SSAO 0.8 radius 150, bloom 0.2 (threshold high), the Image Based Lens Flare at a low intensity, a LUT with a gentle S-curve, warm highlights and teal shadows, film grain 0.15, a light sharpen, vignette 0.3.',
      'Niagara: golden dust motes around the camera (GPU sprites lit by the sun\'s direction), drifting.',
    ],
    costMs: '~1.0 ms (light shafts 0.3, SSAO 0.5, the rest in the LUT)',
    risks: [
      'Compared directly with the big open-world games: the sky and the haze have to be excellent, or it reads as "realistic but lesser".',
      'Haze too strong goes muddy; backlit shots need the sky fill and AO tuned so the near side never goes flat brown.',
      'A sun ahead of the player hides enemies in glare: spiders read as dark silhouettes against the bright haze, so keep the haze bright behind them and the flare off the crosshair.',
      'A fixed time of day: the look depends on the sun\'s angle; other times need their own grade.',
    ],
  },
  env: {
    sun: { azimuth: 80, elevation: 12.5, color: '#ffb36b', intensity: 7.0, shadowSoftness: 1.4, shadowStrength: 1.0 },
    ambient: { sky: '#7399d8', ground: '#7a6048', intensity: 2.7 },
    exposure: 1.12,
    toneMapping: 'aces',
    fog: { color: '#a3afbd', density: 0.0024, heightFalloff: 0.02, height: 0, start: 15, sunScatter: 0.55, sunColor: '#ffcf8f', sunExponent: 10 },
    sky: { zenith: '#285aa8', horizon: '#c6cfd4', ground: '#8a7a5e', sunDisc: 1, sunGlow: 0.45, horizonFog: 0.75, brightness: 1,
      clouds: { amount: 0.3, color: '#ffe0b4', shade: '#9a92a6', scale: 0.55, sharpness: 0.2, speed: 0.004 },
      stars: 0, bodies: [] },
    lights: { scale: 1.0 },
    particles: { dust: 0.9, embers: 0.06, emberColor: '#5ac8ff', motes: '#ffd9a0' },
    wind: { direction: 60, strength: 0.12, speed: 1.3 },
    smoke: { opacity: 0.24 },
  },
  material(src, kit) {
    const role = src.role;
    if (role === 'glow') return kit.pbr(src, {});
    if (role === 'creature') return kit.pbr(src, { rim: { color: '#ffb46a', power: 3, strength: 0.6 }, normalScale: 1.2 });
    if (role === 'foliage' || role === 'grass') return kit.pbr(src, { saturation: 0.92, value: 0.92, translucency: 0.75 });
    if (role === 'metal') return kit.pbr(src, { roughness: 0.8, normalScale: 1.2 });
    if (role === 'gun' || src.master === 'Gun') return kit.pbr(src, {});
    if (role === 'wood') return kit.pbr(src, { normalScale: 1.3, saturation: 1.05 });
    if (role === 'stone' || role === 'rock') return kit.pbr(src, { normalScale: 1.35 });
    return kit.pbr(src, { normalScale: 1.2 });
  },
  terrain(src, kit) {
    return kit.terrainMaterial({ normalStrength: 1.05, detailStrength: 0.7, saturation: 1.04 });
  },
  post(kit) {
    return [
      kit.post.ao({ radius: 1.5, intensity: 0.75, power: 1.5 }),
      makeRays(kit, { strength: 0.32, decay: 0.97, density: 0.92, color: '#ffc884', air: 22 }),
      makeFlare(kit, { strength: 1, ghosts: 0.08, glare: 0.8, veil: 0.07 }),
      kit.post.bloom({ threshold: 2.2, strength: 0.2, radius: 0.8 }),
      kit.post.grade({ contrast: 1.05, saturation: 1.06, vibrance: 0.2, curve: 0.22, shadows: '#3f6878', highlights: '#ffcf96',
        splitAmount: 0.4, splitBalance: 0.05, temperature: 0.0 }),
      kit.post.sharpen({ amount: 0.25 }),
      kit.post.vignette({ amount: 0.28, softness: 0.6, color: '#0e0906' }),
      kit.post.grain({ amount: 0.014, size: 1.2, colored: 0.1 }),
    ];
  },
  fx: { muzzle: '#ffd48a', tracer: '#ffe7b0', impact: '#ffcf80', blood: '#9be35a', hitFlash: '#fff4e0' },
};
