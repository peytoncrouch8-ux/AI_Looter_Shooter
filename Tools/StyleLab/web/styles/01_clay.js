// Style 1: Clay Frontier. The frontier as a stop-motion film set: plasticine buildings worked by thumbs, felt ground,
// clay spiders that move at twelve frames a second, big soft studio lights and a macro lens. Art direction:
// the orchestrator's style_briefs.md ("1. Clay Frontier"); research section J.
import { makeToonSky } from './lib/anime_sky.js';
import { clayHook, feltHook, macroLens, clayColor } from './lib/clay_shaders.js';
import { edgePaint } from './lib/painted_edges.js';
import { swapFlash, restoreFlash, COTTON_FLASH } from './lib/clay_flash.js';
import { groundRecolor, biasedNormalChunk } from './lib/painted_shaders.js';

const ENV = {
  // a warm key from the south, high on its stand; a big cool soft-box fill; shadows wide and never black
  sun: { azimuth: 215, elevation: 38, color: '#ffe6c8', intensity: 4.3, shadowSoftness: 5.0, shadowStrength: 0.92 },
  ambient: { sky: '#a6c0f2', ground: '#d6a26c', intensity: 1.25 },
  exposure: 1.0,
  toneMapping: 'neutral',
  fog: { color: '#d6e0ee', density: 0.0009, heightFalloff: 0.016, height: 0, start: 40, sunScatter: 0.3, sunColor: '#ffe6c4', maxOpacity: 0.7 },
  sky: { zenith: '#94bde3', horizon: '#f2e4c8', ground: '#c9b28c', sunDisc: 0, sunGlow: 0.4, horizonFog: 0.55, clouds: { amount: 0 }, stars: 0, bodies: [] },
  lights: { scale: 1.2 },
  particles: { dust: 0.22, embers: 0.0, emberColor: '#ffcf8a', motes: '#fff0d8' },
  wind: { direction: 60, strength: 0.05, speed: 1.0 },
  smoke: { opacity: 0.5 },
  animStep: 1 / 12,
};

// The clay plates: one colour of plasticine per material, picked like a set dresser would (the rest follow clayColor).
const PLATES = {
  WoodPlanks: '#d6966a', WoodCream: '#f2dcb4', WoodTeal: '#4f9e98', WoodOxide: '#c45a46', WoodBlack: '#4d3c42',
  WoodEndGrain: '#e2ac84', HouseTrim: '#f4e0bc', StoneWall: '#c8ab92', RockCliff_Ransom: '#c9a27e', RockCliffDen_Ransom: '#987056',
  RockGranite_Ransom: '#aea6ad', RockGranite_Sink: '#a8968a', ChestGranite: '#aea6ad', PaintCream: '#eedfbc', PaintOxide: '#bd523c',
  PaintBlack: '#433a3e', MetalRust: '#a86a4a', MetalWorn_Windmill: '#8e939e', IronBlack: '#45424a', BrassWorn: '#d0a050',
  BarkOak: '#7c5638', BarkPine: '#8c5838', BarkBirch: '#ece2d0', Charcoal: '#3a302c', Hay: '#e6bd62', GroundDirt: '#c9a072',
  DenFloor: '#9a7656', Ballast: '#8c7a6a',
  LeavesOak_Ransom: '#7ea64c', LeavesBirch_Ransom: '#d0b84e', LeavesApple_Ransom: '#9eac4a', NeedlesPine: '#4f8250',
  FoliageSage: '#93a462', FoliageJuniper: '#4e7444', FoliagePalette: '#9caa58', FarTrees: '#7f9a56',
  GunPolymerSand: '#ead9b2', GunPolymerBlack: '#3d3c44', GunPolymerGrey: '#7c8088', GunBlackMetal: '#3c3e48', GunSteelBlued: '#4a5470',
  GunSteelBare: '#8c9098', GunWalnut: '#a05c38', GunBrass: '#dcae4c', GunRubber: '#35343a', GunShellRed: '#c04a36',
};

// the toon ramp shaped as a soft wrap: no hard terminator, light creeping round the form like a big soft box
const RAMP = [[-1, 0.0], [-0.3, 0.1], [0.12, 0.52], [0.55, 0.9], [0.95, 1.0]];

export default {
  number: 1,
  id: 'clay',
  name: 'Clay Frontier',
  family: 'Handmade',
  tagline: 'A stop-motion western you can play: plasticine, felt and a macro lens.',
  pitch: 'The whole frontier is a hand-built film set: every wall is plasticine with thumb-worked strokes and a waxy sheen, '
    + 'the hills are felt, the sky is a painted backdrop with cotton clouds hung in front of it, and the spiders jerk toward '
    + 'you at twelve frames a second. A macro lens makes every view a tabletop miniature. Nobody else\'s shooter looks like '
    + 'this, and a clay revenant in a clay ghost town is exactly the kind of weird the West of Teropa wants.',
  hudFit: 'The glossy gunmetal HUD sits over a matte, pastel, softly blurred world like a real camera\'s overlay; its orange and cyan read as bright plastic accents against the clay.',
  refs: [
    { title: 'Harold Halibut: the making of (Creative Bloq)', url: 'https://www.creativebloq.com/features/making-harold-halibut', note: 'a whole game handmade and scanned: the material honesty to aim for' },
    { title: 'Harold Halibut interview (Inverse)', url: 'https://inverse.com/gaming/harold-halibut-interview', note: 'stop-motion feel from edited motion; sets re-lit in 3D' },
    { title: 'Slow Bros on tactile gaming (Skwigly)', url: 'https://skwigly.co.uk/harold-halibut-slow-bros-ole-tillmann-tactile-gaming', note: 'why handmade surfaces read as warmth' },
    { title: 'Miniature faking (Wikipedia)', url: 'https://en.wikipedia.org/wiki/Miniature_faking', note: 'shallow focus plus bright saturated paint reads as a model set' },
  ],
  unreal: {
    recipe: [
      'M_Clay master: base colour from a per-material "clay plate" parameter (one plasticine colour, darks lifted), trim sheets gradient-mapped through a 4-stop clay ramp by their mip-4 lightness; times a world-space macro hue/chroma/value noise so no two walls are one batch. No photo albedo.',
      'Normal: the set\'s normal map at mip 2 (sculpted grooves only) blended with a tiling world-aligned "thumb smear" normal texture (baked once from the lab\'s procedural strokes and dents), faded out past 25 m.',
      'Shading: Default Lit, roughness 0.5, specular 0.35, plus a Subsurface Profile-lite: a custom node adds a red-orange term in the terminator band (wrap ~0.45) and in shadow penumbra (from the shadow factor in the material\'s light vector pin, or a Subsurface shading model with a warm profile on High+).',
      'Terrain: M_Terrain\'s grass layer swapped for a felt texture set (fibre normal + velvet Fresnel), dirt and rock layers smoothed to clay with strata grooves.',
      'Light: directional key at 36 degrees from the south, 3.6 lux warm; sky light cubemap of the painted backdrop at 2.1 for the cool fill; contact shadows on; shadow cascades with a wide light source angle (~4 degrees).',
      'Sky: an unlit sphere with a painted backdrop texture (gradient + felt cotton clouds that shadow the backdrop), no SkyAtmosphere.',
      'Post: Gaussian DOF (Medium) with focal distance from a line trace under the crosshair, ~1 octave of sharp depth; bloom 0.3; vignette; grain 0.02. Stop motion: Animation Blueprints\' UpdateRateOptimizations or a custom 12 fps pose step on creatures and effects only.',
    ],
    costMs: '~0.7 ms (DOF 0.4, smear normal in the material 0.1, GTAO already on)',
    risks: [
      'Every asset needs a clay pass (palette plate per material, sculpted normals): cheap with the generator, but the photo textures must not leak through.',
      'DOF softens enemies past ~30 m; keep the focus on what the crosshair rests on and never blur the near 25 m.',
      '12 fps creatures must keep their hitboxes smooth (step only the pose).',
    ],
  },
  env: ENV,

  material(src, kit) {
    const role = src.role || 'other';
    const m = src.master;
    if (m === 'Smoke' || m === 'Waterfall') return kit.pbr(src, { saturation: 0.6, value: 1.15 });
    if (m === 'Glass') return kit.pbr(src, { roughnessValue: 0.12, envIntensity: 1.2 });
    if (m === 'Water') return kit.pbr(src, { albedo: 'flat', color: '#3f8f98', roughnessValue: 0.08, envIntensity: 1.0 });
    if (role === 'glow' || m === 'Glow') return kit.pbr(src, { albedo: 'flat', glowBoost: 1.4, roughnessValue: 0.4 });
    if (m === 'Flat' && src.unlit) return kit.pbr(src, { albedo: 'flat', color: clayColor(kit.THREE, src.color, 'stone'), saturation: 1.1 });
    const common = {
      ramp: RAMP, softness: 0.22, wrap: 0.45, shadowTint: '#7f8fc0', shadowTintAmount: 0.14, envIntensity: 0.15,
      aoStrength: 1.35,
    };
    const plateOf = (r) => PLATES[src.name] || clayColor(kit.THREE, src.color, r);
    if (src.alphaMask && src.set) {
      // leaves and webs keep their cut-outs; the shapes are melted into sponge-like clumps by a soft mip
      const isWeb = role === 'fabric';
      return kit.toon(src, Object.assign({}, common, {
        blur: isWeb ? 0.5 : 1.6, saturation: isWeb ? 0.4 : 1.25, value: isWeb ? 1.3 : 1.05, contrast: 0.7,
        tint: { color: plateOf(role), amount: isWeb ? 0.2 : 0.8 }, normalScale: 0,
        hook: clayHook({ strength: 0.6, scale: 3.0, dents: false, sheen: 0.5, glow: 0.5, drift: 1.4 }),
      }));
    }
    if (role === 'creature') {
      // chocolate-and-plum clay with a glossy finish, a rim so it reads against the felt
      const mat = kit.toon(src, Object.assign({}, common, {
        albedo: 'flat', color: src.name.includes('Pale') ? '#8c7f8a' : '#3b2a2a', normalScale: 1.4,
        rim: { color: '#ffd6a8', power: 3.0, strength: 0.35 },
        hook: clayHook({ strength: 1.1, scale: 2.0, dents: true, sheen: 2.2, glow: 1.2, drift: 0.3 }),
        _extra: { normal: biasedNormalChunk(kit.THREE, 1), key: 'clayN1' },
      }));
      return mat;
    }
    const plate = plateOf(role);
    const isGun = src.master === 'Gun' || role === 'gun';
    // trim sheets and palettes keep their regions (roof, siding, stone, tin, stripes), each melted to one clay colour
    const atlas = src.set === 'HouseTrim' || src.set === 'FoliagePalette';
    const albedo = atlas ? { albedo: 'texture', blur: 3.5, saturation: 1.3 } : { albedo: 'flat', color: plate };
    const lin = (hex) => { const c = kit.color(hex); return [c.r, c.g, c.b]; };
    const ramp = src.set === 'HouseTrim' ? ['#4a3a46', '#b47466', '#e4b88c', '#f7ecd6'].map(lin) : null;
    const mat = kit.toon(src, Object.assign({}, common, albedo, {
      normalScale: isGun ? 0.6 : 1.7,
      specular: role === 'metal' || isGun ? { size: 0.18, strength: 0.6, color: '#fff2dc' } : null,
      glowBoost: 1.3,
      // only a set's big shapes survive, as sculpted relief: its normal map read at a soft mip
      _extra: { normal: biasedNormalChunk(kit.THREE, isGun ? 2 : 4.5), key: isGun ? 'clayN2' : 'clayN45' },
      hook: clayHook({ strength: isGun ? 0.8 : 1.35, scale: isGun ? 0.4 : 5.0, dents: true,
        sheen: role === 'metal' ? 1.6 : 1.0, glow: role === 'metal' ? 0.4 : 1.0, drift: isGun ? 0.3 : 1.0, ramp }),
    }));
    return mat;
  },

  terrain(src, kit) {
    return kit.terrainMaterial({
      toon: true, ramp: RAMP, softness: 0.22, wrap: 0.45, shadowTint: '#7f8fc0', shadowTintAmount: 0.14,
      macroStrength: 1, detailStrength: 0.18, normalStrength: 0.25, steepNormal: 0.35, blur: 3,
      steepTint: '#c99a78', steepDetail: 0.5,
      // light putty on the roads, sap-green felt where grass grows, sandstone clay on the slopes
      hook: groundRecolor(kit, { dirt: '#d9c4a4', grass: '#7f9f58', rock: '#d0a486', keep: 0.5 }) + feltHook(),
    });
  },

  sky(kit) {
    return makeToonSky(kit, ENV, {
      zenith: '#6fa8e2', mid: '#a9cdef', midAt: 0.3, power: 0.7, horizon: '#f2e2c4', ground: '#c9b28c', haze: 0.32,
      sun: { size: 3.2, disc: 2.2, glow: 0.35, glare: 0 },
      towers: { count: 8, height: 1.1, width: 1.2, seed: 2 },
      floaters: { amount: 0.55, size: 0.13, low: 9, high: 50 },
      cel: { edge: 0.15, softness: 0.38, wrap: 0.6, rim: 0.12 },
      cloud: { lit: '#fffbf2', shade: '#d8d6e8', dark: '#c3bfd6', rim: '#fff4e0' },
      felt: 0.10, brush: 0.0, warp: 0.006, backdropShadow: 0.10, drift: 0.0015, bottomDark: 0.45, sunBleed: 0.15, edgeSoft: 0.06,
    });
  },

  post(kit) {
    return [
      kit.post.ao({ radius: 1.1, intensity: 1.1, power: 1.5, color: '#3a2a3a' }),
      edgePaint(kit, { worldWidth: 10, minWidth: 1.2, maxWidth: 4.5, convex: 0.6, concave: 0.4, litBias: 0.3, tint: '#fff3e0', creaseTint: '#5a3a50', fadeFar: 90 }),
      macroLens(kit, { blur: 5.5, plateau: 1.0, ramp: 2.4, near: 1.0, gun: 1.3 }),
      kit.post.bloom({ threshold: 1.1, strength: 0.32, radius: 0.85 }),
      kit.post.grade({ contrast: 1.1, saturation: 1.12, vibrance: 0.25, temperature: 0.0, lift: '#0d0a12', gain: '#fffaf2', curve: 0.12 }),
      kit.post.vignette({ amount: 0.22, softness: 0.6, color: '#2a1a14', roundness: 0.7 }),
      kit.post.grain({ amount: 0.022, size: 1.6, colored: 0.15 }),
    ];
  },

  fx: { muzzle: '#ffb347', tracer: '#fff1c8', impact: '#ffc85e', blood: '#7fd34e', hitFlash: '#fff4e2' },

  // the muzzle flash as a puffy cotton-wool burst
  activate(kit) { swapFlash(kit, COTTON_FLASH); },
  deactivate(kit) { restoreFlash(kit); },
};
