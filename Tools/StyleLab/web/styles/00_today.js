// Style 0: the game as it looks today, for comparison. Stylized realism: the texture sets through plain PBR, under
// Ransom's Rest's golden afternoon (Art/Levels/RansomsRest/layout.json level.environment: the sun at 247.5 degrees,
// 15 up, 4100 K; thin warm-grey height fog from 60 m glowing amber toward the sun; a clear blue sky overhead).
export default {
  number: 0,
  id: 'today',
  name: 'Today',
  family: 'Current build',
  tagline: 'The game as it ships now: textured stylized realism in a golden afternoon.',
  pitch: 'The baseline every other style is measured against. The game\'s own texture sets on the World, Gun and Terrain masters, '
    + 'the area\'s macro colour map under the detail textures, and the low west-southwest sun of Ransom\'s Rest with its long shadows '
    + 'and warm haze toward the canyon.',
  hudFit: 'The gunmetal, orange and cyan HUD was designed over this look, so it reads cleanly, though the mid-tone ground gives the orange little to push against.',
  refs: [],
  unreal: {
    recipe: [
      'What the build does now: M_World, M_WorldFoliage, M_Gun and M_Terrain masters with the Art/Textures sets through material instances.',
      'Lighting from build_area_environment.py: directional sun 6.5 lux at 4100 K, SkyAtmosphere, sky light, exponential height fog with directional inscattering.',
      'Medium: no Lumen, no Nanite; baked vertex occlusion stands in for SSAO.',
    ],
    costMs: 'baseline',
    risks: ['Reads as generic: flat mid-tones, little shape language, nothing a screenshot would be remembered by.'],
  },
  env: {
    sun: { azimuth: 247.5, elevation: 15, color: '#ffc58f', intensity: 4.2, shadowSoftness: 1.2, shadowStrength: 1.0 },
    ambient: { sky: '#8aa6cf', ground: '#86704e', intensity: 1.9 },
    exposure: 1.2,
    toneMapping: 'aces',
    fog: { color: '#a9b2bb', density: 0.0005, heightFalloff: 0.015, height: 0, start: 60, sunScatter: 0.85, sunColor: '#f2c28a' },
    sky: {
      zenith: '#4d7ec2', horizon: '#c9d2da', ground: '#8a7a5e', sunDisc: 1, sunGlow: 1, horizonFog: 0.7,
      clouds: { amount: 0.28, color: '#fff3e2', shade: '#9ea7b8', scale: 1.0, sharpness: 0.35, speed: 0.004 },
      stars: 0, bodies: [],
    },
    lights: { scale: 1.0 },
    particles: { dust: 0.45, embers: 0.0, emberColor: '#ffb070', motes: '#ffe2b0' },
  },
  material(src, kit) { return kit.pbr(src, {}); },
  terrain(src, kit) { return kit.terrainMaterial({}); },
  post(kit) {
    return [
      kit.post.ao({ radius: 1.2, intensity: 0.5 }),   // the game has no SSAO on Medium; a light touch, as the baked occlusion
      kit.post.bloom({ threshold: 1.6, strength: 0.25, radius: 0.7 }),
      kit.post.grade({ contrast: 1.04, saturation: 1.04, temperature: 0.04 }),
    ];
  },
  fx: { muzzle: '#ffd27a', tracer: '#ffe6a8', impact: '#ffcf7a', blood: '#9be35a', hitFlash: '#ffffff' },
};
