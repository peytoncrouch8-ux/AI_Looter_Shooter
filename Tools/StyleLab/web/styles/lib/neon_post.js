// Neon Frontier's glowing contours: a scene-referred (HDR) pass that traces silhouettes and hard creases in thin light
// before the bloom, so the lines glow. Silhouettes come from depth jumps (this pixel is the near side of an edge),
// creases from the second difference of inverse depth (zero on any plane, so flat ground stays clean). Lines fade with
// distance; edges against the sky carry farther, so ridgelines and rooftops stay traced on the horizon.
//
// Vegetation is marked by the style's foliage materials in the scene colour's alpha (0.5; everything else writes 1),
// so leaves and grass are traced only against the sky: a canopy keeps one clean outline instead of a scribble of
// leaf clumps, grass never fizzes, and a spider standing in grass keeps its outline. A jump also has to be big in
// metres as well as in proportion, and a pixel with something solid clearly nearer a few pixels away is left alone.
export const VEG_ALPHA = 0.5;

export const CONTOUR_FRAG = /* glsl */`
uniform vec3 uLineCol;
uniform float uLineW, uLineK, uCrease, uFadeNear, uFadeFar, uSkyNear, uSkyFar, uMinJump, uRing, uVegSky;
float nld(vec2 uv) { return rawDepth(uv) >= 0.99999 ? 1e6 : linearDepth(uv); }
float vegAt(vec2 uv) { return step(texture2D(tColor, uv).a, 0.75); }
vec4 effect(vec2 uv) {
  vec4 c4 = texture2D(tColor, uv);
  vec3 c = c4.rgb;
  float raw = rawDepth(uv);
  if (raw >= 0.99999 || raw < 0.0011) return vec4(c, 1.0);   // the sky, or the gun (its depth is squeezed)
  float veg0 = step(c4.a, 0.75);
  float d0 = linearDepth(uv);
  vec2 o = uLineW / uResolution;
  float jump = 0.0, toSky = 0.0;
  for (int i = 0; i < 8; i++) {
    float a = float(i) * 0.7853982;
    float dn = nld(uv + vec2(cos(a), sin(a)) * o);
    float dj = dn - d0;
    jump = max(jump, min(dj / d0, 1.0) * smoothstep(uMinJump * 0.5, uMinJump, dj));
    toSky = max(toSky, step(1e5, dn));
  }
  // clutter: something solid clearly nearer within a few pixels (vegetation in front doesn't count)
  float nearer = 0.0;
  vec2 r = o * uRing;
  for (int i = 0; i < 8; i++) {
    float a = float(i) * 0.7853982 + 0.3927;
    vec2 p = uv + vec2(cos(a), sin(a)) * r;
    float dn = nld(p);
    nearer = max(nearer, min((d0 - dn) / d0, 1.0) * step(0.25, d0 - dn) * (1.0 - vegAt(p)));
  }
  float clean = 1.0 - smoothstep(0.03, 0.08, nearer);
  float sil = smoothstep(0.05, 0.16, jump) * (1.0 - veg0);
  float w0 = 1.0 / d0;
  float wl = 1.0 / nld(uv - vec2(o.x, 0.0)), wr = 1.0 / nld(uv + vec2(o.x, 0.0));
  float wd = 1.0 / nld(uv - vec2(0.0, o.y)), wu = 1.0 / nld(uv + vec2(0.0, o.y));
  float lap = min(wl + wr - 2.0 * w0, wd + wu - 2.0 * w0) / w0;
  float crease = smoothstep(0.015, 0.05, -lap) * uCrease * (1.0 - veg0);
  float fade = 1.0 - smoothstep(uFadeNear, uFadeFar, d0);
  float fadeSky = 1.0 - smoothstep(uSkyNear, uSkyFar, d0);
  float e = max(max(sil, crease) * fade, toSky * fadeSky * mix(1.0, uVegSky, veg0)) * clean;
  c += uLineCol * e * uLineK;
  return vec4(c, 1.0);
}`;

export function contourPass(kit, o = {}) {
  const THREE = kit.THREE;
  const params = { width: o.width ?? 1.4, strength: o.strength ?? 2.0, color: o.color || '#3fd8ff', crease: o.crease ?? 0.7,
    fadeNear: o.fadeNear ?? 25, fadeFar: o.fadeFar ?? 110, skyNear: o.skyNear ?? 150, skyFar: o.skyFar ?? 900,
    minJump: o.minJump ?? 0.7, ring: o.ring ?? 3.0, vegSky: o.vegSky ?? 0.6 };
  return kit.post.custom({
    stage: 'hdr', needsDepth: true, params,
    fragment: CONTOUR_FRAG,
    uniforms: {
      uLineCol: { value: new THREE.Color() }, uLineW: { value: 1.4 }, uLineK: { value: 2 }, uCrease: { value: 0.7 },
      uFadeNear: { value: 25 }, uFadeFar: { value: 110 }, uSkyNear: { value: 150 }, uSkyFar: { value: 900 },
      uMinJump: { value: 0.7 }, uRing: { value: 3 }, uVegSky: { value: 0.6 },
    },
    bind(p, u) {
      const h = (u.uResolution && u.uResolution.value) ? u.uResolution.value.y : 1080;
      u.uLineCol.value.copy(kit.color(p.color));
      u.uLineW.value = Math.max(1, p.width * h / 1080);
      u.uLineK.value = p.strength; u.uCrease.value = p.crease;
      u.uFadeNear.value = p.fadeNear; u.uFadeFar.value = p.fadeFar; u.uSkyNear.value = p.skyNear; u.uSkyFar.value = p.skyFar;
      u.uMinJump.value = p.minJump; u.uRing.value = p.ring; u.uVegSky.value = p.vegSky;
    },
  });
}

// Options for a kit material that marks itself as vegetation for the contour pass: the scene colour's alpha carries
// the mark (three forces alpha to 1 on opaque materials, so OPAQUE is undefined for these; they are still drawn
// opaque, without blending). Alpha-to-coverage would turn the mark into holes, so the style switches it off for
// these materials in activate().
export const VEG_EXTRA = { pars: '\n#undef OPAQUE\n', key: 'neonveg' };
export const VEG_MARK = `\n  diffuseColor.a = ${VEG_ALPHA.toFixed(2)};\n`;
