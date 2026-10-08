// Style 7: Neon Frontier. Teropa after sundown as a synthwave western: a violet-to-magenta dusk over a huge striped
// sun sinking into the east, every silhouette traced in thin glowing cyan, hot neon in the windows and lamps, a wet
// street that mirrors the sun, glowing violet haze. The HUD's cyan and orange become the world's light.
import { neonSky } from './lib/neon_sky.js';
import { contourPass, VEG_EXTRA, VEG_MARK } from './lib/neon_post.js';
import { inverseToneMap } from '../engine/tonecurve.js';

const TONE = 'aces';
const EXPOSURE = 1.0;

// The look's palette (as seen on screen; the sun's colours are scene-referred so they bloom).
const PAL = {
  zenith: '#0b0620', high: '#1e0c40', mid: '#4a1670', low: '#b8247a', horizon: '#ff7a3a', backHorizon: '#6a1f7e',
  ground: '#140a20',
  sunTop: '#ffd23a', sunMid: '#ff7a2a', sunBottom: '#ff2a8a', sunBright: 1.15,
  halo: '#c8206a', haloBright: 0.6,
  cloudDark: '#2a0f3c', cloudLit: '#ff4f9a', cloudBright: 1.0,
};
// The light comes from low in the east (long shadows down Main Street); the retro sun hangs a little higher, so its
// barred lower half clears the rooftops at the end of the street.
const SUN = { azimuth: 90, elevation: 6 };
const RSUN = { azimuth: 92, elevation: 12.5, size: 25 };

const glslVec = (c) => `vec3(${c.r.toFixed(4)}, ${c.g.toFixed(4)}, ${c.b.toFixed(4)})`;
const dirOf = (az, el) => {
  const a = az * Math.PI / 180, p = el * Math.PI / 180;
  return `normalize(vec3(${(-Math.sin(a) * Math.cos(p)).toFixed(5)}, ${Math.sin(p).toFixed(5)}, ${(Math.cos(a) * Math.cos(p)).toFixed(5)}))`;
};

// The wet street: the sky and the sun mirrored on the road and in puddles (a planar cheat in the material, one block of
// ALU, no extra pass). The sun's reflection stretches toward the viewer as on wet asphalt, breaks up where the ground is
// only damp, and is cut by the sun's shadow. Wet dirt is a dark mirror: the sky comes back at about a third.
function wetHook(kit) {
  const inv = (hex, k = 1) => glslVec(inverseToneMap(kit.color(hex), TONE, EXPOSURE).multiplyScalar(k));
  const tanR = Math.tan(RSUN.size * 0.5 * Math.PI / 180).toFixed(4);
  return /* glsl */`
  {
    float puddle = smoothstep(0.52, 0.6, slFbm(worldPos.xz * 0.075 + vec2(4.0, 1.3)));
    float flatK = (1.0 - tSteep) * (1.0 - tRock);
    float damp = (0.15 + 0.55 * tDirt) * (0.6 + 0.4 * slFbm(worldPos.xz * 0.6));
    float wet = clamp(max(damp, puddle) * flatK, 0.0, 1.0);
    vec3 Nw = normalize(mix(N, vec3(0.0, 1.0, 0.0), 0.35 + 0.6 * puddle));
    vec3 R = reflect(-V, Nw);
    float ndv = max(dot(Nw, V), 0.0);
    float fres = 0.04 + 0.96 * pow(1.0 - ndv, 5.0);
    float ry = max(R.y, 0.0);
    vec3 S = ${dirOf(RSUN.azimuth, RSUN.elevation)};
    vec2 rh = normalize(R.xz + 1e-5), lh = normalize(S.xz + 1e-5);
    float tw = pow(dot(rh, lh) * 0.5 + 0.5, 2.2);
    vec3 sky = mix(mix(${inv(PAL.backHorizon, 0.4)}, ${inv(PAL.horizon, 0.4)}, tw), ${inv(PAL.low, 0.4)}, smoothstep(0.0, 0.14, ry));
    sky = mix(sky, ${inv(PAL.mid, 0.4)}, smoothstep(0.1, 0.35, ry));
    sky = mix(sky, ${inv(PAL.high, 0.4)}, smoothstep(0.3, 0.7, ry));
    vec3 e = normalize(cross(vec3(0.0, 1.0, 0.0), S));
    vec3 n = cross(S, e);
    float cb = max(dot(R, S), 1e-3);
    vec2 q = vec2(dot(R, e), dot(R, n)) / cb / ${tanR};
    float sy = clamp(q.y * 0.5 + 0.5, 0.0, 1.0);
    q.x *= 1.5; q.y *= 0.3;
    float front = step(0.0, dot(R, S));
    float sun = (1.0 - smoothstep(0.7, 1.05, length(q))) * front * (0.35 + 0.65 * puddle);
    vec3 sunc = mix(${inv(PAL.sunBottom, PAL.sunBright * 0.8)}, ${inv(PAL.sunTop, PAL.sunBright * 0.8)}, sy);
    sky += ${inv(PAL.halo, 0.35)} * exp(-max(length(q) - 1.0, 0.0) * 1.2) * front;
    sky = mix(sky, sunc, sun * shadow);
    col = mix(col, sky, clamp(fres * wet, 0.0, 0.9));
  }`;
}

// A backlit rim: edges facing away from the camera toward the sun catch its hot colour.
const RIM_HOOK = (rimHex, k) => `
  {
    float ndv = max(dot(N, V), 0.0);
    float back = 0.3 + 0.7 * pow(max(dot(-V, L), 0.0), 2.0);
    col += RIMC * pow(1.0 - ndv, 3.0) * back * ${k.toFixed(3)};
  }`.replace('RIMC', rimHex);

export default {
  number: 7,
  id: 'neon',
  name: 'Neon Frontier',
  family: 'Neon dusk',
  tagline: 'Synthwave dusk on the frontier: a striped sun, a violet sky and every edge traced in glowing cyan.',
  pitch: 'Teropa after sundown, lit like a synthwave album cover: an indigo-to-magenta sky, a huge striped sun sinking '
    + 'behind the hills at the end of Main Street, a rain-slick street mirroring it, and every roofline and spider '
    + 'traced in thin glowing cyan. The world is dark and the light is the content, so the saints\' cyan embers, the '
    + 'neon windows and the muzzle flashes carry the image. Nobody in the western lane looks like this.',
  hudFit: 'The HUD\'s cyan and orange are the world\'s own neon here, and its corners sit on dark violet ground and '
    + 'indigo sky, the best contrast the HUD gets in any style.',
  refs: [
    { title: 'Blade Runner 2049: Las Vegas in orange haze', url: 'https://www.studiobinder.com/blog/blade-runner-2049-cinematography-analysis/',
      note: 'haze as a diffuser that keeps a saturated image soft but real' },
    { title: 'Tron: Legacy colour analysis', url: 'https://www.pushing-pixels.org/2010/12/28/the-colors-of-tron-legacy.html',
      note: 'a two-hue world: emissive lines on dark metal' },
    { title: 'Cyberpunk 2077 visual style', url: 'https://adrianlungu.substack.com/p/cyberpunk-2077s-visual-style-crafting',
      note: 'a brand colour of neon, and escaping the genre\'s stock pairing' },
    { title: 'Far Cry 3: Blood Dragon review (bit-tech)', url: 'https://bit-tech.net/reviews/gaming/far-cry-3-blood-dragon/1/',
      note: 'the warning: permanent neon night hurts readability; keep the ground dark and the enemies rimmed' },
    { title: 'Marathon art direction (80.lv)', url: 'https://80.lv/articles/marathon-art-director-shares-the-team-s-creative-influences',
      note: 'bold graphic identity, and why it must be our own' },
  ],
  unreal: {
    recipe: [
      'Sky: an unlit sky-sphere material: elevation gradient (indigo, violet, magenta, an orange band toward the sun), the retro sun (yaw 92, 12.5 degrees up, 25 degrees across) as a disc in its tangent plane, yellow to hot pink, bars cut from its lower half by frac() of its height, emissive so it blooms; plum cloud streaks; stars from a hashed grid.',
      'Directional light at yaw 90, 6 degrees up, coral #ff6a5c (long shadows down Main Street); a blue-violet sky light (#4a56d6) so shade reads cool against the hot key; Lumen off.',
      'Exponential height fog, violet, with directional inscattering in hot pink toward the sun.',
      'Materials: one material function on the masters: albedo x0.55 and cooled; a backlit fresnel rim in the sun\'s colour; window emissives x2.4 whose hue drifts across the town (magenta, orange, a few cyan); lamps orange and pink; the Sink\'s egg sacs acid green.',
      'Wet street: in M_Terrain, a planar-reflection cheat: reflect the view vector off a flattened normal, look the sky\'s gradient and sun up analytically (the same math as the sky material), Fresnel-weighted by the road mask and a puddle noise; no SSR or planar reflection needed.',
      'Contours: a post-process material before tonemapping (BL_BeforeTonemapping) that traces scene-depth discontinuities and creases in cyan emissive, so the engine\'s bloom makes them glow. Foliage writes a CustomStencil value and is traced only against the sky (otherwise canopies and grass fizz).',
      'Post: bloom ~1.2, a little chromatic aberration, grain, vignette, a LUT with a violet lift.',
    ],
    costMs: '~0.8 ms (contour pass ~0.3, bloom ~0.3, the rest is material ALU)',
    risks: [
      'Saturation fatigue over long sessions (Blood Dragon\'s lesson): keep the ground dark and the neon to edges and lights.',
      'Contour lines can crawl on foliage and fences in motion: fade them by distance and keep them off thin cards.',
      'A synthwave sun can read as off-brand for a gothic chapel and churchyard; the story\'s cyan embers tie it back.',
    ],
  },
  env: {
    sun: { azimuth: SUN.azimuth, elevation: SUN.elevation, color: '#ff6a5c', intensity: 3.2, shadowSoftness: 1.4, shadowStrength: 1.0 },
    ambient: { sky: '#4a56d6', ground: '#2a1238', intensity: 1.8 },
    exposure: EXPOSURE,
    toneMapping: TONE,
    fog: { color: '#4e2a82', density: 0.0045, heightFalloff: 0.035, start: 8, sunScatter: 0.95, sunColor: '#ff4f86', sunExponent: 5 },
    sky: { zenith: PAL.zenith, horizon: PAL.horizon, ground: PAL.ground, sunDisc: 0, sunGlow: 0.6, horizonFog: 0.6,
      clouds: { amount: 0 }, stars: 0.8, bodies: [] },
    lights: { scale: 5.0 },
    particles: { dust: 0.25, embers: 0.9, emberColor: '#5ac8ff', motes: '#ff8ad8' },
    wind: { direction: 60, strength: 0.1, speed: 1.2 },
    smoke: { opacity: 0.18 },
  },
  material(src, kit) {
    const name = src.name || '';
    const role = src.role || 'other';
    if (name.startsWith('WindowGlow')) {
      // windows: hot neon whose hue drifts slowly across the town (magenta, orange, now and then cyan), so a building's
      // windows share a colour and the street alternates
      return kit.pbr(src, { albedo: 'flat', color: '#050308', roughnessValue: 0.3, glowBoost: 0, hook: `
        {
          float wt = sin(dot(worldPos.xz, vec2(0.23, 0.17))) * 0.5 + 0.5;
          float wc = smoothstep(0.82, 0.9, sin(dot(worldPos.xz, vec2(-0.11, 0.29)) + 1.3) * 0.5 + 0.5);
          vec3 nc = mix(vec3(1.0, 0.1, 0.42), vec3(1.0, 0.38, 0.06), smoothstep(0.35, 0.65, wt));
          nc = mix(nc, vec3(0.15, 0.7, 1.0), wc);
          col = nc * 2.4;
        }` });
    }
    if (name === 'LanternGlow') return kit.pbr(Object.assign({}, src, { glowColor: '#ff7a2a' }), { glowBoost: 3 });
    if (role === 'glow') return kit.pbr(src, { glowBoost: 2.5 });
    if (role === 'creature') {
      return kit.pbr(src, { value: 0.32, saturation: 0.5, roughness: 0.7, hook: `
        {
          float ndv = max(dot(N, V), 0.0);
          col += vec3(1.0, 0.1, 0.35) * pow(1.0 - ndv, 2.5) * 0.9;
        }` });
    }
    // the gun: dark, glossy, its accent strip a cyan tube
    if (name === 'GunAccentGlow') return kit.pbr(Object.assign({}, src, { glowColor: '#29d4ff' }), { glowBoost: 2.2 });
    if (role === 'gun') return kit.pbr(src, { value: 0.6, saturation: 0.8, roughness: 0.6, hook: RIM_HOOK('vec3(1.0, 0.3, 0.55)', 0.4) });
    if (role === 'foliage' || role === 'grass') {
      // vegetation marks itself (scene alpha) so the contours trace it only against the sky
      return kit.pbr(src, { value: 0.5, saturation: 0.75, hueShift: 40, tint: { color: '#1f6a86', amount: 0.45 },
        hook: RIM_HOOK('vec3(1.0, 0.3, 0.5)', 0.8) + VEG_MARK, _extra: VEG_EXTRA });
    }
    if (role === 'water') return kit.pbr(src, { value: 0.4, roughnessValue: 0.03 });
    // the Sink's egg sacs glow acid green from inside; webs carry a faint cyan sheen like strung light
    if (name === 'EggSilk') {
      return kit.pbr(src, { value: 0.5, hook: `
        {
          float ndv = max(dot(N, V), 0.0);
          float pulse = 0.85 + 0.15 * sin(uSlTime * 2.0 + dot(worldPos, vec3(1.3, 0.7, 2.1)));
          col = col * 0.4 + vec3(0.35, 1.0, 0.22) * (0.6 + 1.4 * pow(ndv, 2.0)) * pulse;
        }` });
    }
    if (name.startsWith('Web')) return kit.pbr(src, { value: 0.6, hook: 'col += vec3(0.35, 0.8, 1.0) * 0.5;' });
    if (src.master === 'Glass' || src.master === 'Smoke' || src.master === 'Waterfall' || src.unlit) {
      return kit.pbr(src, { value: 0.6, tint: { color: '#5a3a8a', amount: 0.4 } });
    }
    const wet = role === 'metal' || role === 'stone' || role === 'rock';
    return kit.pbr(src, { value: 0.55, saturation: 0.7, tint: { color: '#6a5aa8', amount: 0.22 }, roughness: wet ? 0.55 : 0.8,
      hook: RIM_HOOK('vec3(1.0, 0.32, 0.4)', 0.55) });
  },
  terrain(src, kit) {
    const m = kit.terrainMaterial({ value: 0.4, saturation: 0.55, tint: { color: '#5a4a9a', amount: 0.3 }, hook: wetHook(kit) });
    m.roughness = 0.7;
    return m;
  },
  sky(kit) {
    return neonSky(kit, Object.assign({}, PAL, {
      exposure: EXPOSURE, toneMapping: TONE, sunAzimuth: RSUN.azimuth, sunElevation: RSUN.elevation, sunSize: RSUN.size,
      stripes: 7, stars: 0.9, horizonFog: 0.55,
    }));
  },
  post(kit) {
    return [
      kit.post.ao({ radius: 1.2, intensity: 0.6, color: '#0a0414' }),
      contourPass(kit, { width: 1.8, strength: 1.3, color: '#12c4ff', crease: 0.6, fadeNear: 30, fadeFar: 160 }),
      kit.post.bloom({ threshold: 0.85, strength: 0.75, radius: 0.85 }),
      kit.post.grade({ contrast: 1.06, saturation: 1.12, vibrance: 0.15, lift: '#0e0620' }),
      kit.post.chromatic({ amount: 0.00025 }),
      kit.post.grain({ amount: 0.025, size: 1.4, colored: 0.25 }),
      kit.post.vignette({ amount: 0.35, softness: 0.55, color: '#06020f' }),
    ];
  },
  activate(kit) {
    // the house lamps become neon: hot orange and pink pools
    (kit.lights || []).forEach((l, i) => {
      l.scale = 1;
      l.colorOverride = kit.color(i % 2 ? '#ff3a8a' : '#ff7a2a');
    });
    // vegetation carries the contour mark in its alpha, so its cut-outs use a plain alpha test instead of
    // alpha-to-coverage (these are this style's own cached materials, so this holds until the page reloads)
    kit.scene.traverse((o) => {
      const mats = Array.isArray(o.material) ? o.material : (o.material ? [o.material] : []);
      for (const m of mats) {
        const sl = m.userData && m.userData.sl;
        const rawExtra = sl && sl.rawOpts && sl.rawOpts._extra;
        if (rawExtra && rawExtra.key === VEG_EXTRA.key && m.alphaToCoverage) { m.alphaToCoverage = false; m.needsUpdate = true; }
      }
    });
  },
  fx: { muzzle: '#ff6a4a', tracer: '#8ff0ff', impact: '#ff8a3a', blood: '#6dff5a', hitFlash: '#ff4fd0' },
};
