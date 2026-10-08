// kit.terrainMaterial: the ground the way M_Terrain blends it (Tools/Unreal/build_world_materials.py): the area's macro
// colour map times the detail textures' light and dark (grass, dirt by mask R, rock by mask G), and faces steeper than
// about 40 degrees (full by 55) take the rock laid on from the side, in the macro's colour darkened by a steep tint.
import * as THREE from 'three';
import { linColor } from './util.js';

const PARS = /* glsl */`
uniform sampler2D uTMacro, uTMasks, uTGrassBC, uTGrassN, uTDirtBC, uTDirtN, uTRockBC, uTRockN;
uniform vec4 uTRect;
uniform vec3 uTTile;
uniform vec3 uTMean;
uniform vec4 uTSteep;        // cos(full), cos(start), steep detail, detail strength
uniform vec3 uTSteepTint;
uniform vec4 uTNormal;       // detail normal strength, steep normal strength, macro strength, (unused)
uniform vec2 uTFade;         // detail fades from .x to .y metres
uniform vec3 uTMacroMean;
uniform sampler2D uTRing;
uniform vec4 uTRingRect;
uniform float uTHasRing;
vec3 tNW0;
float tSteep, tRock, tDirt, tFade;
vec2 tFacing;
vec3 slTerrainDetail(sampler2D t, vec2 uv) {
  // two scales, so the tiling does not read
  vec3 a = texture(t, uv, uSlBlur).rgb;
  vec3 b = texture(t, uv * 0.273 + 0.31, uSlBlur).rgb;
  return a * mix(1.0, slLuma(b) / max(slLuma(a), 0.03), 0.3);
}
`;

const MAP = /* glsl */`
{
  tNW0 = normalize(inverseTransformDirection(normalize(vNormal), viewMatrix));
  vec2 uvM = (vSlWorldPos.xz - uTRect.xy) / (uTRect.zw - uTRect.xy);
  vec2 edge = min(uvM, 1.0 - uvM);
  float inside = smoothstep(0.0, 0.01, min(edge.x, edge.y));
  // past the core: the ring's own macro map, and past that its mean
  vec2 uvR = (vSlWorldPos.xz - uTRingRect.xy) / (uTRingRect.zw - uTRingRect.xy);
  vec2 edgeR = min(uvR, 1.0 - uvR);
  float insideR = smoothstep(0.0, 0.01, min(edgeR.x, edgeR.y)) * uTHasRing;
  vec3 farMacro = mix(uTMacroMean, texture2D(uTRing, clamp(uvR, 0.0, 1.0)).rgb, insideR);
  vec3 macro = mix(farMacro, texture2D(uTMacro, clamp(uvM, 0.0, 1.0)).rgb, inside);
  vec4 mask = mix(vec4(0.0, 0.0, 0.5, 0.0), texture2D(uTMasks, clamp(uvM, 0.0, 1.0)), inside);
  tSteep = 1.0 - smoothstep(uTSteep.x, uTSteep.y, tNW0.y);
  tDirt = mask.r; tRock = mask.g;
  float dist = length(vSlWorldPos - cameraPosition);
  tFade = 1.0 - smoothstep(uTFade.x, uTFade.y, dist);
  vec2 wp = vSlWorldPos.xz;
  vec3 g = slTerrainDetail(uTGrassBC, wp / uTTile.x);
  vec3 d = slTerrainDetail(uTDirtBC, wp / uTTile.y);
  vec3 r = slTerrainDetail(uTRockBC, wp / uTTile.z);
  vec3 det = mix(mix(g, d, tDirt), r, tRock);
  float mean = mix(mix(uTMean.x, uTMean.y, tDirt), uTMean.z, tRock);
  vec3 lumaMod = macro * mix(1.0, slLuma(det) / max(mean, 0.04), uTSteep.w * tFade);
  // macro strength 1: the macro's colour with the detail's light and dark (the game); 0: the detail textures' own colour
  vec3 col = mix(det * mix(1.0, slLuma(macro) / max(mean, 0.04), 0.5), lumaMod, uTNormal.z);
  tFacing = pow(abs(tNW0.xz), vec2(4.0));
  tFacing /= max(tFacing.x + tFacing.y, 1e-4);
  if (tSteep > 0.001) {
    vec3 rx = texture(uTRockBC, vSlWorldPos.zy / uTTile.z, uSlBlur).rgb;
    vec3 rz = texture(uTRockBC, vSlWorldPos.xy / uTTile.z, uSlBlur).rgb;
    vec3 side = rx * tFacing.x + rz * tFacing.y;
    vec3 steepCol = macro * uTSteepTint * mix(1.0, slLuma(side) / max(uTMean.z, 0.04), uTSteep.z);
    steepCol = mix(side * uTSteepTint * mix(1.0, slLuma(macro) / max(uTMean.z, 0.04), 0.5), steepCol, uTNormal.z);
    col = mix(col, steepCol, tSteep);
  }
  diffuseColor.rgb *= col;
}
diffuseColor.rgb = slAdjust(diffuseColor.rgb);
slVAO = mix(1.0, vSlVC.a, uSlAOStrength);
`;

// The detail normal in the world (DirectX maps on glTF-style UVs: red along +u, green along +v), then into view space.
const NORMAL = /* glsl */`
{
  vec2 wp = vSlWorldPos.xz;
  vec3 ng = texture(uTGrassN, wp / uTTile.x).xyz * 2.0 - 1.0;
  vec3 nd = texture(uTDirtN, wp / uTTile.y).xyz * 2.0 - 1.0;
  vec3 nr = texture(uTRockN, wp / uTTile.z).xyz * 2.0 - 1.0;
  vec3 nt = mix(mix(ng, nd, tDirt), nr, tRock);
  float k = uTNormal.x * (1.0 - tSteep) * tFade;
  vec3 N0 = tNW0;
  vec3 T = normalize(vec3(1.0, 0.0, 0.0) - N0 * N0.x);
  vec3 B = normalize(vec3(0.0, 0.0, 1.0) - N0 * N0.z);
  vec3 nW = normalize((T * nt.x + B * nt.y) * k + N0 * max(nt.z, 0.2));
  if (tSteep > 0.001 && uTNormal.y > 0.0) {
    vec3 sx = texture(uTRockN, vSlWorldPos.zy / uTTile.z).xyz * 2.0 - 1.0;
    vec3 sz = texture(uTRockN, vSlWorldPos.xy / uTTile.z).xyz * 2.0 - 1.0;
    // X-facing faces: u along +z, v along +y; Z-facing: u along +x, v along +y (green is DirectX: down the image = +v)
    vec3 ox = vec3(0.0, sx.y, sx.x);
    vec3 oz = vec3(sz.x, sz.y, 0.0);
    vec3 side = ox * tFacing.x + oz * tFacing.y;
    nW = normalize(mix(nW, normalize(N0 + side * uTNormal.y), tSteep));
  }
  normal = normalize((viewMatrix * vec4(nW, 0.0)).xyz);
}
`;

const ROUGH = /* glsl */`
float roughnessFactor = roughness * mix(0.95, 0.84, max(tRock, tSteep));
`;

export function buildTerrainMaterial(kit, opts = {}, ctx) {
  const { assets, manifest } = ctx;
  const t = manifest.terrain || {};
  const layers = {};
  for (const l of t.layers || []) layers[l.name] = l;
  const layer = (name, fallback) => layers[name] || layers[fallback] || Object.values(layers)[0] || { set: null, tile: 4 };
  const gL = layer('grass'), dL = layer('dirt', 'grass'), rL = layer('rock', 'grass');
  const sets = manifest.textureSets || {};
  const meanLuma = (set) => {
    const s = sets[set];
    if (!s || !s.mean) return 0.3;
    const c = linColor(s.mean);
    return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
  };
  const blur = opts.blur || 0;
  const tex = (set, kind) => assets.tex(set, kind, { blur: kind === 'bc' ? 0 : 0 });
  const deg = THREE.MathUtils.degToRad;
  const uniforms = {
    uTMacro: { value: ctx.macroTex || assets.placeholder.bc },
    uTMasks: { value: ctx.masksTex || assets.placeholder.bc },
    uTGrassBC: { value: tex(gL.set, 'bc') }, uTGrassN: { value: tex(gL.set, 'n') },
    uTDirtBC: { value: tex(dL.set, 'bc') }, uTDirtN: { value: tex(dL.set, 'n') },
    uTRockBC: { value: tex(rL.set, 'bc') }, uTRockN: { value: tex(rL.set, 'n') },
    uTRect: { value: new THREE.Vector4(...(t.macroRect || [-100, -100, 100, 100])) },
    uTTile: { value: new THREE.Vector3(gL.tile || 4, dL.tile || 4, rL.tile || 8).multiplyScalar(opts.tileScale || 1) },
    uTMean: { value: new THREE.Vector3(meanLuma(gL.set), meanLuma(dL.set), meanLuma(rL.set)) },
    uTSteep: { value: new THREE.Vector4(Math.cos(deg(opts.steepFull ?? 55)), Math.cos(deg(opts.steepStart ?? 40)),
      opts.steepDetail ?? 1.15, opts.detailStrength ?? 0.6) },
    uTSteepTint: { value: linColor(opts.steepTint || [0.8, 0.74, 0.68]) },
    uTNormal: { value: new THREE.Vector4(opts.normalStrength ?? 0.8, opts.steepNormal ?? 0.6, opts.macroStrength ?? 1, 0) },
    uTFade: { value: new THREE.Vector2(...(opts.detailFade || [60, 260])) },
    uTMacroMean: { value: ctx.macroMean ? ctx.macroMean.clone() : new THREE.Color(0.3, 0.25, 0.15) },
    uTRing: { value: ctx.ringTex || assets.placeholder.bc },
    uTRingRect: { value: new THREE.Vector4(...(t.ringMacroRect || [-1, -1, 1, 1])) },
    uTHasRing: { value: ctx.ringTex ? 1 : 0 },
  };
  const src = { name: 'Terrain', master: 'Terrain', set: null, color: '#ffffff', roughness: 0.92, metallic: 0, role: 'ground' };
  const o = Object.assign({}, opts, {
    albedo: 'flat', color: '#ffffff', normalScale: 0, blur,
    _extra: { pars: PARS, map: MAP, normal: NORMAL, roughness: ROUGH, uniforms, key: 'terrain' },
  });
  delete o.toon;
  const m = opts.toon ? kit.toon(src, o) : kit.pbr(src, o);
  m.roughness = 1;
  m.userData.terrainUniforms = uniforms;
  return m;
}
