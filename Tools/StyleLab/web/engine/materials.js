// The kit's material builders: pbr, toon, flat and terrainMaterial. Each is three's MeshStandardMaterial with its shader
// patched: the sun's cascaded soft shadows, the style's height fog, vertex occlusion (COLOR_0.a), foliage wind
// (COLOR_0.r weight, .g phase), DirectX normal maps, albedo adjustments, toon ramps, rim, glow and a GLSL hook.
import * as THREE from 'three';
import { NOISE, FOG, COLOR } from './glsl.js';
import { linColor, strHash, logOnce } from './util.js';

// Uniforms every kit material shares (one object each, so updating the value updates every material).
export const SHARED = {
  uSlTime: { value: 0 },
  uSlAnimTime: { value: 0 },
  uSlFogColor: { value: new THREE.Color(0.6, 0.6, 0.6) },
  uSlFogSunColor: { value: new THREE.Color(1, 0.9, 0.7) },
  uSlFog: { value: new THREE.Vector4(0.002, 0.05, 0, 0) },
  uSlFogSun: { value: new THREE.Vector4(0.5, 8, 1, 0) },
  uSlSunDirW: { value: new THREE.Vector3(0, 1, 0) },
  uSlWind: { value: new THREE.Vector4(0.8, 0.6, 0.12, 1.4) },     // dir x, dir z, strength (m), speed
  uSlShadowStrength: { value: 1 },
  uSlCascade: { value: new THREE.Vector4(15, 45, 180, 0.15) },     // far of cascades 0..2, blend band
  uSlShadowRadius: { value: new THREE.Vector4(2, 2, 1.5, 0) },     // filter radius in texels per cascade
  uSlMacro: { value: null },                                       // the terrain macro map (ground tint)
  uSlMacroRect: { value: new THREE.Vector4(-100, -100, 100, 100) },
  uSlMacroMean: { value: new THREE.Color(0.3, 0.25, 0.15) },
  uSlUnlit: { value: 1 },                                          // what an unlit surface's albedo is multiplied by
};

const MODES = { pbr: 0, toon: 1, flat: 2 };

const POISSON = `const vec2 SL_POISSON[12] = vec2[12](vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696, 0.457),
  vec2(-0.203, 0.621), vec2(0.962, -0.195), vec2(0.473, -0.480), vec2(0.519, 0.767), vec2(0.185, -0.893),
  vec2(0.507, 0.064), vec2(0.896, 0.412), vec2(-0.322, -0.933), vec2(-0.792, -0.598));`;

// Vertex: world position (with wind) for fog and hooks, the vertex colour for occlusion and wind.
const VERT_PARS = /* glsl */`
varying vec3 vSlWorldPos;
varying vec4 vSlVC;
#if !defined( USE_COLOR ) && !defined( USE_COLOR_ALPHA )
attribute vec4 color;
#endif
uniform float uSlTime;
uniform vec4 uSlWind;
#ifdef SL_WIND
vec3 slWindOffset(vec3 wp, vec4 vc) {
  float w = vc.r;
  float ph = vc.g * 6.2831853 + dot(wp.xz, vec2(0.31, 0.23));
  float t = uSlTime * uSlWind.w;
  float gust = 0.55 + 0.45 * sin(t * 0.37 + dot(wp.xz, vec2(0.045, 0.031)));
  float sway = 0.45 + 0.55 * sin(t + ph) + 0.18 * sin(t * 2.7 + ph * 1.9);
  vec2 d = uSlWind.xy * (uSlWind.z * w * sway * gust);
  vec2 flutter = vec2(sin(t * 4.1 + ph * 3.0), cos(t * 3.7 + ph * 2.3)) * uSlWind.z * 0.18 * w;
  d += flutter;
  return vec3(d.x, -0.25 * dot(d, d), d.y);
}
#endif
`;
const VERT_PROJECT = /* glsl */`
vec4 slLocal = vec4( transformed, 1.0 );
#ifdef USE_BATCHING
  slLocal = batchingMatrix * slLocal;
#endif
#ifdef USE_INSTANCING
  slLocal = instanceMatrix * slLocal;
#endif
vec4 slWorld = modelMatrix * slLocal;
vSlVC = color;
#ifdef SL_WIND
  slWorld.xyz += slWindOffset( slWorld.xyz, color );
#endif
vSlWorldPos = slWorld.xyz;
vec4 mvPosition = viewMatrix * slWorld;
gl_Position = projectionMatrix * mvPosition;
`;

export function patchVertex(vs) {
  vs = vs.replace('void main() {', VERT_PARS + '\nvoid main() {');
  vs = vs.replace('#include <project_vertex>', VERT_PROJECT);
  if (vs.includes('#include <worldpos_vertex>')) vs = vs.replace('#include <worldpos_vertex>', 'vec4 worldPosition = slWorld;');
  return vs;
}

// Fragment declarations: fog, noise, the sun's cascaded shadow, the ramp and the albedo adjustments.
const FRAG_PARS = /* glsl */`
varying vec3 vSlWorldPos;
varying vec4 vSlVC;
uniform float uSlTime;
uniform float uSlFade;
uniform float uSlBlur;
uniform vec4 uSlAdj;          // saturation, value, contrast, hue shift (turns)
uniform vec4 uSlTintC;        // tint colour, amount
uniform float uSlAOStrength;
uniform float uSlShadowStrength;
uniform vec4 uSlCascade;
uniform vec4 uSlShadowRadius;
uniform vec4 uSlRim;          // colour, strength
uniform float uSlRimPower;
uniform float uSlPosterize;
uniform vec2 uSlRamp[6];
uniform float uSlRampN;
uniform float uSlSoft;
uniform float uSlWrap;
uniform vec4 uSlShadowTint;   // colour, amount
uniform vec4 uSlSpec;         // size, strength, (unused), (unused)
uniform vec3 uSlSpecColor;
uniform vec3 uSlFlatLit;
uniform vec3 uSlFlatShade;
uniform float uSlTranslucency;
uniform float uSlUpNormal;
uniform float uSlGroundTint;
uniform float uSlUnlit;
uniform sampler2D uSlMacro;
uniform vec4 uSlMacroRect;
uniform vec3 uSlMacroMean;
${NOISE}
${FOG}
${COLOR}
float slShadow = 1.0;
float slNdL = 0.0;
float slLevel = 1.0;
float slVAO = 1.0;
vec3 slSunCol = vec3(0.0);

vec3 slAdjust(vec3 c) {
  if (uSlAdj.w != 0.0) { vec3 h = slRgb2Hsv(max(c, 0.0)); h.x = fract(h.x + uSlAdj.w); c = slHsv2Rgb(h); }
  float l = slLuma(c);
  c = max(mix(vec3(l), c, uSlAdj.x), 0.0);
  if (uSlAdj.z != 1.0) c = pow(c / 0.18, vec3(uSlAdj.z)) * 0.18;
  c *= uSlAdj.y;
  if (uSlTintC.a > 0.0) c = mix(c, uSlTintC.rgb * (slLuma(c) / max(slLuma(uSlTintC.rgb), 1e-3)), uSlTintC.a);
  return clamp(c, 0.0, 1.0);
}

float slRampLevel(float x) {
  float lv = uSlRamp[0].y;
  for (int i = 1; i < 6; i++) {
    if (float(i) >= uSlRampN) break;
    float e = uSlRamp[i].x;
    lv = mix(lv, uSlRamp[i].y, smoothstep(e - uSlSoft, e + uSlSoft, x));
  }
  return lv;
}

#if defined( USE_SHADOWMAP ) && NUM_DIR_LIGHT_SHADOWS > 0
${POISSON}
float slShadowPCF(sampler2D map, vec2 size, float bias, vec4 coord, float radius, mat2 R) {
  vec3 c = coord.xyz / coord.w;
  c.z += bias;
  if (c.x < 0.0 || c.x > 1.0 || c.y < 0.0 || c.y > 1.0 || c.z > 1.0) return -1.0;
  vec2 ts = radius / size;
  float s = 0.0;
  for (int i = 0; i < 12; i++) s += step(c.z, unpackRGBAToDepth(texture2D(map, c.xy + R * SL_POISSON[i] * ts)));
  return s / 12.0;
}
#define SL_CASC(i, r) slShadowPCF(directionalShadowMap[i], directionalLightShadows[i].shadowMapSize, directionalLightShadows[i].shadowBias, vDirectionalShadowCoord[i], r, R)
float slFix(float s) { return s < 0.0 ? 1.0 : s; }
float slSunShadow() {
  float d = length(vSlWorldPos - cameraPosition);
  float a = slIGN(gl_FragCoord.xy) * 6.2831853;
  mat2 R = mat2(cos(a), sin(a), -sin(a), cos(a));
  float band = uSlCascade.w;
  float s0, s1, t;
  if (d < uSlCascade.x) {
    s0 = SL_CASC(0, uSlShadowRadius.x);
    t = smoothstep(uSlCascade.x * (1.0 - band), uSlCascade.x, d);
    #if NUM_DIR_LIGHT_SHADOWS > 1
    if (t > 0.0) { s1 = SL_CASC(1, uSlShadowRadius.y); if (s1 >= 0.0) s0 = mix(slFix(s0), s1, t); }
    #endif
    return slFix(s0);
  }
  #if NUM_DIR_LIGHT_SHADOWS > 1
  if (d < uSlCascade.y) {
    s0 = SL_CASC(1, uSlShadowRadius.y);
    t = smoothstep(uSlCascade.y * (1.0 - band), uSlCascade.y, d);
    #if NUM_DIR_LIGHT_SHADOWS > 2
    if (t > 0.0) { s1 = SL_CASC(2, uSlShadowRadius.z); if (s1 >= 0.0) s0 = mix(slFix(s0), s1, t); }
    #else
    s0 = mix(slFix(s0), 1.0, t);
    #endif
    return slFix(s0);
  }
  #endif
  #if NUM_DIR_LIGHT_SHADOWS > 2
  if (d < uSlCascade.z) {
    s0 = SL_CASC(2, uSlShadowRadius.z);
    t = smoothstep(uSlCascade.z * (1.0 - band), uSlCascade.z, d);
    return mix(slFix(s0), 1.0, t);
  }
  #endif
  return 1.0;
}
#endif
`;

// The sun and point lights, by mode. Only directionalLights[0] lights the scene: the other directional lights are the
// sun's shadow cascades.
function lightsBegin(mode) {
  const sunPBR = `directLight.color *= slShadow; RE_Direct( directLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );`;
  const sunToon = `
    slLevel = slRampLevel(mix(-1.0, (slNdL + uSlWrap) / (1.0 + uSlWrap), slShadow));
    reflectedLight.directDiffuse += directLight.color * slLevel * BRDF_Lambert( material.diffuseColor );
    if (uSlSpec.y > 0.0) {
      vec3 slH = normalize(directLight.direction + geometryViewDir);
      float slNH = max(dot(geometryNormal, slH), 0.0);
      float slE = 1.0 - uSlSpec.x;
      float slSp = smoothstep(slE - uSlSoft * 0.25, slE + uSlSoft * 0.25, pow(slNH, 4.0)) * uSlSpec.y * slShadow * step(0.0, slNdL);
      reflectedLight.directSpecular += directLight.color * uSlSpecColor * slSp * 0.15;
    }`;
  const sunFlat = `
    slLevel = slRampLevel(mix(-1.0, (slNdL + uSlWrap) / (1.0 + uSlWrap), slShadow));
    reflectedLight.directDiffuse += material.diffuseColor * mix(uSlFlatShade, uSlFlatLit, slLevel);`;
  const pointPBR = `RE_Direct( directLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );`;
  const pointToon = `{ float pl = slRampLevel(dot(geometryNormal, directLight.direction));
      reflectedLight.directDiffuse += directLight.color * pl * BRDF_Lambert( material.diffuseColor ); }`;
  const sun = mode === 0 ? sunPBR : mode === 1 ? sunToon : sunFlat;
  const point = mode === 0 ? pointPBR : pointToon;
  return /* glsl */`
vec3 geometryPosition = - vViewPosition;
vec3 geometryNormal = normal;
vec3 geometryViewDir = ( isOrthographic ) ? vec3( 0, 0, 1 ) : normalize( vViewPosition );
vec3 geometryClearcoatNormal = vec3( 0.0 );
IncidentLight directLight;
#if NUM_DIR_LIGHTS > 0
  directLight.color = directionalLights[ 0 ].color;
  directLight.direction = directionalLights[ 0 ].direction;
  directLight.visible = true;
  slSunCol = directLight.color;
  slNdL = dot( geometryNormal, directLight.direction );
  #if defined( USE_SHADOWMAP ) && NUM_DIR_LIGHT_SHADOWS > 0
    if ( receiveShadow ) slShadow = slSunShadow();
  #endif
  slShadow = mix( 1.0, slShadow, uSlShadowStrength );
  ${sun}
#endif
#if NUM_POINT_LIGHTS > 0
  PointLight pointLight;
  #pragma unroll_loop_start
  for ( int i = 0; i < NUM_POINT_LIGHTS; i ++ ) {
    pointLight = pointLights[ i ];
    getPointLightInfo( pointLight, geometryPosition, directLight );
    ${point}
  }
  #pragma unroll_loop_end
#endif
vec3 iblIrradiance = vec3( 0.0 );
vec3 irradiance = getAmbientLightIrradiance( ambientLightColor );
#if NUM_HEMI_LIGHTS > 0
  #pragma unroll_loop_start
  for ( int i = 0; i < NUM_HEMI_LIGHTS; i ++ ) {
    irradiance += getHemisphereLightIrradiance( hemisphereLights[ i ], geometryNormal );
  }
  #pragma unroll_loop_end
#endif
${mode === 2 ? 'irradiance = vec3( 0.0 );' : ''}
vec3 radiance = vec3( 0.0 );
vec3 clearcoatRadiance = vec3( 0.0 );
`;
}

const LIGHTS_MAPS = /* glsl */`
#if defined( USE_ENVMAP ) && defined( RE_IndirectSpecular ) && defined( ENVMAP_TYPE_CUBE_UV )
  radiance += getIBLRadiance( geometryViewDir, geometryNormal, material.roughness );
#endif
`;

const MAP_FRAG = /* glsl */`
#ifdef USE_MAP
  vec4 sampledDiffuseColor = texture( map, vMapUv, uSlBlur );
  diffuseColor *= sampledDiffuseColor;
#endif
#ifdef SL_GROUNDTINT
  {
    vec2 gtUV = (vSlWorldPos.xz - uSlMacroRect.xy) / (uSlMacroRect.zw - uSlMacroRect.xy);
    vec3 gt = (gtUV.x > 0.0 && gtUV.x < 1.0 && gtUV.y > 0.0 && gtUV.y < 1.0) ? texture2D(uSlMacro, gtUV).rgb : uSlMacroMean;
    float gl = slLuma(diffuseColor.rgb);
    diffuseColor.rgb = mix(diffuseColor.rgb, gt * (gl / max(slLuma(gt), 0.02)) , uSlGroundTint);
  }
#endif
diffuseColor.rgb = slAdjust(diffuseColor.rgb);
slVAO = mix(1.0, vSlVC.a, uSlAOStrength);
`;

const UPNORMAL = /* glsl */`
#ifdef SL_UPNORMAL
  normal = normalize(mix(normal, (viewMatrix * vec4(0.0, 1.0, 0.0, 0.0)).xyz, uSlUpNormal));
#endif
`;

const VAO_FRAG = /* glsl */`
reflectedLight.indirectDiffuse *= slVAO;
reflectedLight.indirectSpecular *= slVAO;
reflectedLight.directDiffuse *= mix(1.0, slVAO, 0.45);
`;

function postLight(hook, hookAfterFog) {
  return /* glsl */`
{
  vec3 col = outgoingLight;
  #ifdef SL_UNLIT
    col = diffuseColor.rgb * uSlUnlit + totalEmissiveRadiance;
  #endif
  vec3 slNW = normalize(inverseTransformDirection(normal, viewMatrix));
  vec3 slV = normalize(cameraPosition - vSlWorldPos);
  #ifdef SL_TRANSLUCENT
    float slBack = pow(max(dot(-slV, uSlSunDirW), 0.0), 4.0);
    col += diffuseColor.rgb * slSunCol * slBack * uSlTranslucency * slShadow * 0.3;
  #endif
  #ifdef SL_RIM
    col += uSlRim.rgb * pow(1.0 - max(dot(slNW, slV), 0.0), uSlRimPower) * uSlRim.a * (0.35 + 0.65 * slShadow * max(slNdL, 0.0));
  #endif
  #if defined( SL_TOON ) || defined( SL_FLAT )
    if (uSlShadowTint.a > 0.0) {
      vec3 target = uSlShadowTint.rgb * (slLuma(col) / max(slLuma(uSlShadowTint.rgb), 1e-3));
      col = mix(col, target, uSlShadowTint.a * (1.0 - slLevel));
    }
  #endif
  #ifdef SL_POSTERIZE
    { vec3 p = col / (1.0 + col); p = floor(p * uSlPosterize + 0.5) / uSlPosterize; col = p / max(1.0 - p, 1e-3); }
  #endif
  #if defined( SL_HOOK ) && !defined( SL_HOOK_AFTER_FOG )
  {
    vec3 N = slNW; vec3 V = slV; vec3 L = uSlSunDirW; float NdL = dot(N, L); float shadow = slShadow; float ao = slVAO;
    vec2 uvTex = vUv; vec3 worldPos = vSlWorldPos;
    ${hookAfterFog ? '' : hook}
  }
  #endif
  outgoingLight = col;
}
`;
}

function fogFrag(hook, hookAfterFog) {
  return /* glsl */`
gl_FragColor.rgb = slApplyFog( gl_FragColor.rgb, cameraPosition, vSlWorldPos );
#if defined( SL_HOOK ) && defined( SL_HOOK_AFTER_FOG )
{
  vec3 col = gl_FragColor.rgb;
  vec3 N = normalize(inverseTransformDirection(normal, viewMatrix)); vec3 V = normalize(cameraPosition - vSlWorldPos);
  vec3 L = uSlSunDirW; float NdL = dot(N, L); float shadow = slShadow; float ao = slVAO; vec2 uvTex = vUv; vec3 worldPos = vSlWorldPos;
  ${hookAfterFog ? hook : ''}
  gl_FragColor.rgb = col;
}
#endif
`;
}

const FADE_FRAG = `
if ( uSlFade < 0.999 && slIGN( gl_FragCoord.xy ) > uSlFade ) discard;
`;

export function patchFragment(fs, mode, hook = '', hookAfterFog = false, extra = null) {
  fs = fs.replace('void main() {', FRAG_PARS + (extra && extra.pars || '') + '\nvoid main() {');
  fs = fs.replace('#include <clipping_planes_fragment>', '#include <clipping_planes_fragment>\n' + FADE_FRAG);
  fs = fs.replace('#include <map_fragment>', extra && extra.map ? extra.map : MAP_FRAG);
  // foliage cards: back faces keep the front's normal (as M_WorldFoliage), so a canopy shades as one rounded mass
  fs = fs.replace('#include <normal_fragment_begin>', '#include <normal_fragment_begin>\n#if defined( SL_KEEPFRONT ) && defined( DOUBLE_SIDED )\n  normal *= faceDirection;\n#endif');
  if (extra && extra.normal) fs = fs.replace('#include <normal_fragment_maps>', extra.normal);
  else fs = fs.replace('#include <normal_fragment_maps>', '#include <normal_fragment_maps>\n' + UPNORMAL);
  if (extra && extra.roughness) fs = fs.replace('#include <roughnessmap_fragment>', extra.roughness);
  fs = fs.replace('#include <lights_fragment_begin>', lightsBegin(mode));
  fs = fs.replace('#include <lights_fragment_maps>', LIGHTS_MAPS);
  fs = fs.replace('#include <aomap_fragment>', '#include <aomap_fragment>\n' + VAO_FRAG);
  fs = fs.replace('#include <opaque_fragment>', postLight(hook, hookAfterFog) + '\n#include <opaque_fragment>');
  fs = fs.replace('#include <fog_fragment>', fogFrag(hook, hookAfterFog));
  return fs;
}

function rampFrom(o) {
  // A ramp is [[NdL edge, light level], ...] with NdL in -1..1 (a point in shadow reads -1). steps n makes n even levels
  // with edges spread over the lit half (the first at the terminator).
  let ramp = o.ramp;
  if (!ramp) {
    const n = Math.max(2, Math.min(5, Math.round(o.steps || 3)));
    ramp = [[-1, 0]];
    for (let i = 1; i < n; i++) ramp.push([i === 1 ? 0.0 : (i - 1) / (n - 1) * 0.9, i / (n - 1)]);
  }
  ramp = ramp.slice(0, 6).map((r) => [r[0], r[1]]);
  if (ramp[0][0] > -1) ramp.unshift([-1, ramp[0][1]]);
  ramp = ramp.slice(0, 6);
  const arr = [];
  for (let i = 0; i < 6; i++) arr.push(new THREE.Vector2(...(ramp[Math.min(i, ramp.length - 1)])));
  return { arr, n: ramp.length };
}

// ------------------------------------------------------------------------------------------------ the kit
export class MaterialKit {
  constructor(ctx) {
    this.ctx = ctx;   // { assets, manifest, envMap() }
    this.all = new Set();
  }

  _defaults(src, o, mode) {
    const role = src.role || 'other';
    const foliage = src.master === 'WorldFoliage' || role === 'foliage' || role === 'grass';
    return {
      albedo: o.albedo || (src.set ? 'texture' : 'flat'),
      blur: o.blur ?? 0,
      saturation: o.saturation ?? 1, value: o.value ?? 1, contrast: o.contrast ?? 1, hueShift: o.hueShift ?? 0,
      tint: o.tint || null, posterize: o.posterize ?? 0,
      normalScale: o.normalScale ?? (mode === 'flat' ? 0 : 1),
      roughness: o.roughness ?? 1, roughnessValue: o.roughnessValue, metalness: o.metalness,
      aoStrength: o.aoStrength ?? 1, rim: o.rim || null, glowBoost: o.glowBoost ?? 1,
      hook: o.hook || '', hookAfterFog: !!o.hookAfterFog,
      envIntensity: o.envIntensity ?? (mode === 'pbr' ? 1 : mode === 'toon' ? 0.3 : 0),
      color: o.color,
      translucency: o.translucency ?? (foliage ? 0.6 : 0),
      upNormal: o.upNormal ?? (role === 'grass' ? 0.65 : foliage ? 0.3 : 0),
      groundTint: o.groundTint ?? (role === 'grass' ? 0.45 : 0),
      wind: o.wind ?? !!src.wind,
      windScale: o.windScale ?? 1,
      opacity: o.opacity,
      // toon
      steps: o.steps, ramp: o.ramp, softness: o.softness ?? (mode === 'flat' ? 0.03 : 0.05),
      shadowTint: o.shadowTint, shadowTintAmount: o.shadowTintAmount ?? (o.shadowTint ? 0.5 : 0),
      specular: o.specular || null, wrap: o.wrap ?? 0,
      light: o.light ?? 1.0, shade: o.shade,
    };
  }

  // The material every builder starts from.
  _build(src, opts, modeName) {
    const { assets, manifest } = this.ctx;
    const mode = MODES[modeName];
    src = src || {};
    const o = this._defaults(src, opts || {}, modeName);
    const m = new THREE.MeshStandardMaterial();
    m.name = src.name || 'kit';
    const set = src.set && manifest.textureSets && manifest.textureSets[src.set] ? src.set : null;
    const textured = o.albedo === 'texture' && set;
    const tint = Array.isArray(src.tintLinear) ? new THREE.Color().setRGB(src.tintLinear[0], src.tintLinear[1], src.tintLinear[2], THREE.LinearSRGBColorSpace)
      : linColor(src.tint || '#ffffff');
    const uvs = { uvScale: src.uvScale && src.uvScale !== 1 ? src.uvScale : 0, blur: 0 };
    if (textured) {
      m.map = assets.tex(set, 'bc', uvs);
      m.color.copy(o.color ? linColor(o.color) : tint);
    } else {
      m.color.copy(linColor(o.color || src.color || '#8a8a8a'));
    }
    const hasN = set && manifest.textureSets[set].n;
    if (hasN && o.normalScale > 0) {
      m.normalMap = assets.tex(set, 'n', uvs);
      // DirectX green, glTF UVs (flipY off): with derivative tangents +y is right; with vertex tangents it flips.
      const dx = (manifest.textureSets[set].normal || 'DirectX') === 'DirectX';
      const tangents = !!assets.hasTangents;
      const sign = (dx ? 1 : -1) * (tangents ? -1 : 1);
      m.normalScale.set(o.normalScale, o.normalScale * sign);
    }
    const hasOrm = set && manifest.textureSets[set].orm;
    if (hasOrm && o.roughnessValue === undefined) {
      const orm = assets.tex(set, 'orm', uvs);
      m.roughnessMap = orm; m.metalnessMap = orm;
      m.aoMap = orm; m.aoMapIntensity = o.aoStrength;
      m.roughness = THREE.MathUtils.clamp(o.roughness, 0, 2);
    } else {
      m.roughness = o.roughnessValue ?? THREE.MathUtils.clamp((src.roughness ?? 0.8) * o.roughness, 0.03, 1);
    }
    m.metalness = o.metalness ?? (src.metallic ?? 0);
    if (mode === 2) { m.metalness = 0; m.roughness = 1; }
    if (src.twoSided) m.side = THREE.DoubleSide;
    const alpha = src.alphaMask && textured && manifest.textureSets[set].alpha !== false;
    if (alpha) { m.alphaTest = 0.5; m.alphaToCoverage = true; }
    // Glow: emissive from src.glow
    const glow = (src.glow || 0) * o.glowBoost;
    if (glow > 0) { m.emissive.copy(linColor(src.glowColor || '#ffb24d')); m.emissiveIntensity = glow; }
    // Masters with their own needs
    if (src.master === 'Glass') { m.transparent = true; m.opacity = o.opacity ?? 0.3; m.depthWrite = false; m.roughness = 0.05; }
    else if (src.master === 'Smoke' || src.master === 'Waterfall') { m.transparent = true; m.opacity = o.opacity ?? 0.5; m.depthWrite = false; }
    else if (src.master === 'Water') { m.roughness = o.roughnessValue ?? 0.06; m.metalness = 0; }
    else if (o.opacity !== undefined && o.opacity < 1) { m.transparent = true; m.opacity = o.opacity; }
    m.envMap = this.ctx.envMap ? this.ctx.envMap() : null;
    m.envMapIntensity = o.envIntensity;
    m.fog = false;

    const ramp = rampFrom(o);
    const shadeDefault = o.shade !== undefined ? linColor(o.shade) : new THREE.Color(0.42, 0.45, 0.55);
    const u = {
      uSlFade: { value: 1 },
      uSlBlur: { value: o.blur },
      uSlAdj: { value: new THREE.Vector4(o.saturation, o.value, o.contrast, (o.hueShift || 0) / 360) },
      uSlTintC: { value: o.tint ? new THREE.Vector4(...linColor(o.tint.color).toArray(), o.tint.amount ?? 0.5) : new THREE.Vector4(1, 1, 1, 0) },
      uSlAOStrength: { value: o.aoStrength },
      uSlRim: { value: o.rim ? new THREE.Vector4(...linColor(o.rim.color || '#ffffff').toArray(), o.rim.strength ?? 0.5) : new THREE.Vector4(0, 0, 0, 0) },
      uSlRimPower: { value: o.rim ? (o.rim.power ?? 3) : 3 },
      uSlPosterize: { value: o.posterize || 0 },
      uSlRamp: { value: ramp.arr }, uSlRampN: { value: ramp.n }, uSlSoft: { value: o.softness }, uSlWrap: { value: o.wrap },
      uSlShadowTint: { value: o.shadowTint ? new THREE.Vector4(...linColor(o.shadowTint).toArray(), o.shadowTintAmount) : new THREE.Vector4(0, 0, 0, 0) },
      uSlSpec: { value: o.specular ? new THREE.Vector4(o.specular.size ?? 0.1, o.specular.strength ?? 1, 0, 0) : new THREE.Vector4(0.1, 0, 0, 0) },
      uSlSpecColor: { value: linColor(o.specular && o.specular.color || '#ffffff') },
      uSlFlatLit: { value: typeof o.light === 'number' ? new THREE.Color(1, 1, 1).multiplyScalar(o.light) : linColor(o.light) },
      uSlFlatShade: { value: shadeDefault },
      uSlTranslucency: { value: o.translucency },
      uSlUpNormal: { value: o.upNormal },
      uSlGroundTint: { value: o.groundTint },
    };
    m.defines = m.defines || {};
    m.defines.USE_UV = '';
    if (modeName === 'toon') m.defines.SL_TOON = '';
    if (modeName === 'flat') m.defines.SL_FLAT = '';
    if (o.wind) m.defines.SL_WIND = '';
    if (o.posterize > 0) m.defines.SL_POSTERIZE = '';
    if (o.rim) m.defines.SL_RIM = '';
    if (o.translucency > 0) m.defines.SL_TRANSLUCENT = '';
    if (o.upNormal > 0) m.defines.SL_UPNORMAL = '';
    if (src.twoSided && (src.master === 'WorldFoliage' || src.role === 'foliage' || src.role === 'grass')) m.defines.SL_KEEPFRONT = '';
    if (o.groundTint > 0) m.defines.SL_GROUNDTINT = '';
    if (src.unlit || o.unlit) m.defines.SL_UNLIT = '';
    if (o.hook) { m.defines.SL_HOOK = ''; if (o.hookAfterFog) m.defines.SL_HOOK_AFTER_FOG = ''; }
    m.userData.sl = { mode: modeName, src, opts: o, rawOpts: opts, uniforms: u, kit: true, wind: o.wind, alpha };
    m.userData.slUniforms = u;
    const hook = o.hook, after = o.hookAfterFog, extra = opts && opts._extra;
    m.onBeforeCompile = (shader) => {
      Object.assign(shader.uniforms, SHARED, u);
      if (extra && extra.uniforms) Object.assign(shader.uniforms, extra.uniforms);
      shader.vertexShader = patchVertex(shader.vertexShader);
      shader.fragmentShader = patchFragment(shader.fragmentShader, mode, hook, after, extra);
      m.userData.shader = shader;
    };
    const key = 'sl:' + modeName + ':' + strHash((hook || '') + (after ? 'A' : '') + (extra ? extra.key : ''));
    m.customProgramCacheKey = () => key;
    this.all.add(m);
    return m;
  }

  pbr(src, opts = {}) { return this._build(src, opts, 'pbr'); }
  toon(src, opts = {}) { return this._build(src, opts, 'toon'); }
  flat(src, opts = {}) {
    const o = Object.assign({}, opts);
    if (o.albedo === undefined) o.albedo = 'flat';
    const m = this._build(src, o, 'flat');
    // The lit colour follows the sun's hue and the shade the sky's, unless the style gives them.
    m.userData.sl.flatAuto = { lit: opts.light === undefined || typeof opts.light === 'number', shade: opts.shade === undefined };
    return m;
  }

  // Wraps any material a style returns that isn't the kit's, so it still gets the one-sun lighting, shadows and fog.
  adopt(m) {
    if (!m || m.userData.sl) return m;
    const lit = m.isMeshStandardMaterial || m.isMeshLambertMaterial || m.isMeshPhongMaterial || m.isMeshToonMaterial;
    if (!lit && !m.isMeshBasicMaterial) return m;
    const u = { uSlFade: { value: 1 } };
    const prev = m.onBeforeCompile;
    m.fog = false;
    m.userData.sl = { mode: 'foreign', uniforms: u, kit: false };
    m.userData.slUniforms = u;
    m.onBeforeCompile = (shader, r) => {
      if (prev) prev.call(m, shader, r);
      Object.assign(shader.uniforms, SHARED, u);
      shader.vertexShader = patchVertex(shader.vertexShader);
      let fs = shader.fragmentShader;
      fs = fs.replace('void main() {', `varying vec3 vSlWorldPos;\nvarying vec4 vSlVC;\nuniform float uSlFade;\nuniform float uSlShadowStrength;\nuniform vec4 uSlCascade;\nuniform vec4 uSlShadowRadius;\n${NOISE}\n${FOG}\nfloat slShadow = 1.0; float slNdL = 0.0; vec3 slSunCol = vec3(0.0);\n` +
        (lit ? FRAG_PARS.slice(FRAG_PARS.indexOf('#if defined( USE_SHADOWMAP )')) : '') + '\nvoid main() {');
      if (lit) fs = fs.replace('#include <lights_fragment_begin>', lightsBegin(0).replace('vec3 iblIrradiance', 'vec3 iblIrradiance'));
      fs = fs.replace('#include <fog_fragment>', 'gl_FragColor.rgb = slApplyFog( gl_FragColor.rgb, cameraPosition, vSlWorldPos );');
      shader.fragmentShader = fs;
    };
    const key = 'slforeign:' + m.type;
    const prevKey = m.customProgramCacheKey;
    m.customProgramCacheKey = () => key + (prevKey ? prevKey.call(m) : '');
    return m;
  }

  // The shadow-pass material for a wind-swayed kit material (alpha-tested if the material is).
  depthFor(m) {
    if (!m.userData.sl || !m.userData.sl.wind) return null;
    if (m.userData.depthMat) return m.userData.depthMat;
    const d = new THREE.MeshDepthMaterial({ depthPacking: THREE.RGBADepthPacking });
    if (m.userData.sl.alpha) { d.map = m.map; d.alphaTest = 0.5; }
    d.side = m.side;
    d.onBeforeCompile = (shader) => {
      Object.assign(shader.uniforms, SHARED);
      shader.defines = Object.assign(shader.defines || {}, { SL_WIND: '' });
      shader.vertexShader = '#define SL_WIND\n' + patchVertex(shader.vertexShader);
      shader.fragmentShader = shader.fragmentShader.replace('void main() {', 'varying vec3 vSlWorldPos;\nvarying vec4 vSlVC;\nvoid main() {');
    };
    d.customProgramCacheKey = () => 'sldepthwind' + (d.map ? 'A' : '');
    m.userData.depthMat = d;
    return d;
  }

  // The normal-pass material (view-space normals), with the same wind, skinning, instancing and alpha test.
  normalFor(m) {
    if (m.userData.normalMat) return m.userData.normalMat;
    const sl = m.userData.sl || {};
    const alpha = !!(sl.alpha && m.map);
    const n = new THREE.ShaderMaterial({
      uniforms: Object.assign({ map: { value: alpha ? m.map : null } }, SHARED),
      defines: Object.assign({}, sl.wind ? { SL_WIND: '' } : {}, alpha ? { SL_ALPHA: '' } : {}),
      side: m.side,
      vertexShader: /* glsl */`
        #include <common>
        #include <batching_pars_vertex>
        #include <skinning_pars_vertex>
        varying vec3 vN;
        varying vec2 vUv2;
        void main() {
          #include <batching_vertex>
          #include <beginnormal_vertex>
          #include <skinbase_vertex>
          #include <skinnormal_vertex>
          #include <defaultnormal_vertex>
          vN = normalize(transformedNormal);
          vUv2 = uv;
          #include <begin_vertex>
          #include <skinning_vertex>
          #include <project_vertex>
        }`,
      fragmentShader: /* glsl */`
        varying vec3 vN;
        varying vec2 vUv2;
        uniform sampler2D map;
        void main() {
          #ifdef SL_ALPHA
          if (texture2D(map, vUv2).a < 0.5) discard;
          #endif
          vec3 n = normalize(vN) * (gl_FrontFacing ? 1.0 : -1.0);
          gl_FragColor = vec4(n * 0.5 + 0.5, 1.0);
        }`,
    });
    n.onBeforeCompile = (shader) => {
      shader.vertexShader = patchVertex(shader.vertexShader);
    };
    n.customProgramCacheKey = () => 'slnormal' + (sl.wind ? 'W' : '') + (alpha ? 'A' : '');
    m.userData.normalMat = n;
    return n;
  }
}
