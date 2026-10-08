// Style 2: Skyward Anime. A sun-drenched anime feature on the frontier: crisp two-tone cel light with lavender
// shadows, a deep saturated sky stacked with towering cel-shaded cumulus, clean coloured line work and sparkling
// highlights. Art direction: the orchestrator's style_briefs.md ("2. Skyward Anime"); research section F.
import { makeToonSky } from './lib/anime_sky.js';
import { foliageHook, groundHook, AIRY } from './lib/anime_shaders.js';
import { groundRecolor } from './lib/painted_shaders.js';

const ENV = {
  // a high bright sun; shadows mid-value and airy, never murky
  sun: { azimuth: 258, elevation: 56, color: '#fff3dc', intensity: 4.4, shadowSoftness: 0.6, shadowStrength: 1.0 },
  ambient: { sky: '#b4c6fa', ground: '#e0c49a', intensity: 1.85 },
  exposure: 1.0,
  toneMapping: 'neutral',
  // strong aerial perspective: far ridges go pale blue
  fog: { color: '#a9cdef', density: 0.0022, heightFalloff: 0.012, height: 0, start: 25, sunScatter: 0.25, sunColor: '#e8f4ff', maxOpacity: 0.92 },
  sky: { zenith: '#1d5fd8', horizon: '#bfe9f6', ground: '#9a8e7a', sunDisc: 1, sunGlow: 1, horizonFog: 0.5, clouds: { amount: 0 }, stars: 0, bodies: [] },
  lights: { scale: 1.0 },
  particles: { dust: 0.12, embers: 0.0, emberColor: '#ffffff', motes: '#fffbe8' },
  wind: { direction: 60, strength: 0.14, speed: 1.5 },
  smoke: { opacity: 0.35 },
};

// two tones: the shade keeps a fifth of the sun (anime shade is a colour, not a darkness)
const RAMP = [[-1, 0.2], [0.02, 1.0]];

export default {
  number: 2,
  id: 'anime',
  name: 'Skyward Anime',
  family: 'Anime cel',
  tagline: 'A sun-drenched anime feature on the frontier: crisp cel light, towering cumulus, clean coloured lines.',
  pitch: 'Two-tone cel light with lavender shadows, hand-laid colour gradients on grass and ground, warm rim light on every '
    + 'silhouette and thin coloured line work, under a deep blue sky stacked with sculpted cumulus. It is the look with the '
    + 'biggest audience on Steam, played straight with a western\'s heat: dust-gold streets, sun-bleached planks, spiders '
    + 'cut out as dark shapes against the light.',
  hudFit: 'Anime UI lives over bright pastel worlds; the gunmetal HUD adds the weight that keeps it a western, and its cyan and orange read like the highlights in the sky and on the dust.',
  refs: [
    { title: 'HoyoToon: using the Genshin shader', url: 'https://github.com/Melioli/HoyoToon/wiki/Using-the-Genshin-Shader', note: 'a soft N.L window, a thin second band, painted shadow masks' },
    { title: 'URP simple Genshin shaders', url: 'https://github.com/NoiRC256/URPSimpleGenshinShaders', note: 'ramp light, rim and outlines in a real-time engine' },
    { title: 'Breath of the Wild\'s look (Zelda Universe, GDC 2017)', url: 'https://zeldauniverse.net/2017/03/14/takizawa-explains-how-the-wind-waker-hd-inspired-the-visual-style-for-breath-of-the-wild/', note: 'painterly world, environments softer than characters' },
    { title: 'Wuthering Waves on Unreal (Epic interview)', url: 'https://www.unrealengine.com/developer-interviews/exploring-the-post-apocalyptic-charm-of-asg-open-worlds-in-wuthering-waves', note: 'anime cel on UE: gradients in base colour, post-material grading, fake volume fog' },
    { title: 'Global toon shading in UE5 via post process', url: 'https://medium.com/@little_michael101/building-a-toon-shader-in-unreal-engine-5-globally-not-per-mesh-part-1-6ef9aade3380', note: 'the post-process route this recipe takes' },
  ],
  unreal: {
    recipe: [
      'Global cel light in a post-process material before tonemapping: diffuse light = SceneColor / (BaseColor + epsilon), stepped by a 2-stop curve (edge at 0.02, width 0.02), shade side multiplied toward lavender (#6f6fd0, 40%); BaseColor from the GBuffer.',
      'Textures: the sets at mip 3 (texture LOD bias in the material instances) with saturation 1.3; grass and leaf materials add a vertex-colour root-to-tip gradient; terrain gets a low-frequency painted wash.',
      'Specular: a stepped GGX highlight on metal and glass via a material function (smoothstep of N.H^4), and a warm Fresnel rim on creatures.',
      'Outlines: one post-process material, Sobel on depth and normals (1.2 px), line colour = SceneColor x 0.25, fading 40-250 m.',
      'Sky: an unlit sky sphere with a vertex-gradient and cumulus built from sphere sprites (or a painted panorama), plus ExponentialHeightFog density 0.03, falloff 0.12, pale blue for aerial perspective.',
      'Sun at 52 degrees, 4.4 lux, sky light 1.75 from the sky sphere capture; bloom 0.35; light shafts for the glare.',
    ],
    costMs: '~0.6 ms (post cel + outline 0.5, materials ~0)',
    risks: [
      'Spiders and props need anime shape language (bigger shapes, painted shadow masks) to match the environment; the realistic models are the weak link.',
      'Outlines shimmer on foliage cards at range; fade them early and skip alpha-tested pixels.',
      'The look is associated with gacha games; the western palette and the gunmetal HUD have to carry the difference.',
    ],
  },
  env: ENV,

  material(src, kit) {
    const role = src.role || 'other';
    const m = src.master;
    if (m === 'Smoke' || m === 'Waterfall') return kit.pbr(src, { saturation: 0.4, value: 1.3 });
    if (m === 'Glass') return kit.toon(src, { ramp: RAMP, softness: 0.02, specular: { size: 0.1, strength: 3, color: '#ffffff' }, envIntensity: 1.2, rim: { color: '#ffffff', power: 2.5, strength: 0.6 } });
    if (m === 'Water') return kit.toon(src, { albedo: 'flat', color: '#3fa6c8', ramp: RAMP, softness: 0.02, specular: { size: 0.08, strength: 3 }, envIntensity: 1.0 });
    if (role === 'glow' || m === 'Glow') return kit.pbr(src, { albedo: 'flat', glowBoost: 1.5 });
    // the far backdrop's silhouettes: pale blue ranges, as an anime background layers its distance
    if (m === 'Flat' && src.unlit) return kit.pbr(src, { albedo: 'flat', color: '#9cc2e8', value: 0.62 });
    const base = {
      ramp: RAMP, softness: 0.018, shadowTint: '#8a84dc', shadowTintAmount: 0.22,
      blur: 3, saturation: 1.32, value: 1.08, contrast: 0.92, normalScale: 0.35, aoStrength: 0.8,
      envIntensity: 0.25, hook: AIRY,
    };
    if (role === 'creature') {
      const pale = src.name.includes('Pale');
      return kit.toon(src, Object.assign({}, base, {
        albedo: 'flat', color: pale ? '#c9bfd8' : '#2c2140', normalScale: 0.5,
        rim: { color: '#ffd9a0', power: 2.6, strength: 0.9 },
        specular: { size: 0.05, strength: 1.4, color: '#ffe8c8' }, hook: AIRY,
      }));
    }
    if (role === 'foliage' || role === 'grass' || src.master === 'WorldFoliage') {
      const leafy = src.alphaMask;
      return kit.toon(src, Object.assign({}, base, {
        blur: leafy ? 1 : 3, saturation: 1.3, value: 1.05, hueShift: role === 'fabric' ? 0 : 3,
        shadowTint: '#3f6aa8', shadowTintAmount: 0.38, translucency: 0.9,
        hook: role === 'fabric' ? AIRY : foliageHook() + AIRY,
      }));
    }
    const metal = role === 'metal' || role === 'gun' || src.master === 'Gun';
    const wood = role === 'wood';
    return kit.toon(src, Object.assign({}, base, {
      value: wood ? 1.35 : base.value, tint: wood ? { color: '#c98f5a', amount: 0.25 } : null,
      specular: metal ? { size: 0.07, strength: 2.2, color: '#ffffff' } : null,
      rim: { color: '#fff0d8', power: 4, strength: metal ? 0.35 : 0.18 },
    }));
  },

  terrain(src, kit) {
    return kit.terrainMaterial({
      toon: true, ramp: RAMP, softness: 0.02, shadowTint: '#8a84dc', shadowTintAmount: 0.28,
      macroStrength: 1, detailStrength: 0.3, normalStrength: 0.12, steepNormal: 0.25, blur: 3,
      // dust-gold street, bright spring-green verges, pale sandstone cliffs
      hook: groundRecolor(kit, { dirt: '#dcbb80', grass: '#78a454', rock: '#ddb68c', keep: 0.25 }) + groundHook() + AIRY,
    });
  },

  sky(kit) {
    return makeToonSky(kit, ENV, {
      zenith: '#1b58d4', mid: '#3f8ff0', midAt: 0.28, power: 0.62, horizon: '#c4ecf7', ground: '#9a8e7a', haze: 0.42,
      sun: { size: 1.4, disc: 26, glow: 1.1, glare: 2.5 },
      towers: { count: 9, height: 1.5, width: 1.0, seed: 5 },
      floaters: { amount: 0.36, size: 0.085, low: 14, high: 55 },
      cel: { edge: 0.1, softness: 0.016, wrap: 0.3, rim: 0.5 }, topLight: 0.3, mass: 0.6,
      cloud: { lit: '#ffffff', shade: '#b3b4ea', dark: '#9393d6', rim: '#ffffff' },
      felt: 0, brush: 0, warp: 0.004, backdropShadow: 0, drift: 0.002, bottomDark: 0.75, sunBleed: 0.25, edgeSoft: 0.012,
    });
  },

  post(kit) {
    return [
      kit.post.ao({ radius: 0.9, intensity: 0.45, power: 1.4, color: '#2a2a6a' }),
      kit.post.bloom({ threshold: 1.25, strength: 0.42, radius: 0.85 }),
      kit.post.godrays({ strength: 0.22, decay: 0.955, density: 0.8, color: '#fff6dc', samples: 40 }),
      kit.post.outline({ width: 1.15, color: '#2a2440', colorFromScene: 0.85, depthEdge: 1, normalEdge: 0.75, innerLines: 1, fadeNear: 25, fadeFar: 170, opacity: 0.85 }),
      kit.post.grade({ contrast: 1.05, saturation: 1.08, vibrance: 0.22, temperature: 0.02, gain: '#fffdf6', curve: 0.08 }),
      kit.post.fxaa({ subpix: 0.4 }),
    ];
  },

  fx: { muzzle: '#fff2a8', tracer: '#ffffff', impact: '#fffbe0', blood: '#c6ff6a', hitFlash: '#ffffff' },
};
