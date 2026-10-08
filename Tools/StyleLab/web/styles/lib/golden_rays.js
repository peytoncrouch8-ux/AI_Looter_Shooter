// Golden Hour's sun shafts: the sky round the sun, blurred radially toward it at half resolution (as the kit's god
// rays), but composited by how much air lies in front of each pixel, so a spider leaping at four metres stays a dark
// silhouette with a golden rim instead of being washed over by shafts that are, physically, mostly behind it.
// In Unreal: Light Shafts (bloom) on the directional light with a post material that scales them by scene depth, or
// the stock light shafts with Occlusion Depth Range kept short.

const MASK = /* glsl */`
uniform float uThreshold;
vec4 effect(vec2 uv) {
  float sky = step(0.99999, rawDepth(uv));
  vec3 c = texture2D(tColor, uv).rgb;
  vec2 d = (uv - uSunScreen.xy) * vec2(uResolution.x / uResolution.y, 1.0);
  float near = exp(-dot(d, d) * 5.0);
  float b = max(slLuma(c) - uThreshold, 0.0);
  return vec4(c * sky * near * min(b, 8.0) / max(slLuma(c), 1e-3), 1.0);
}`;

const BLUR = /* glsl */`
uniform float uDecay, uDensity;
vec4 effect(vec2 uv) {
  const int S = 56;
  vec2 dir = (uv - uSunScreen.xy) * uDensity / float(S);
  vec2 p = uv - dir * slIGN(gl_FragCoord.xy);
  vec3 acc = vec3(0.0); float w = 1.0, tot = 0.0;
  for (int i = 0; i < S; i++) { acc += texture2D(tColor, p).rgb * w; tot += w; w *= uDecay; p -= dir; }
  return vec4(acc / max(tot, 1e-3), 1.0);
}`;

const COMP = /* glsl */`
uniform sampler2D tRays;
uniform float uStrength, uAir;
uniform vec3 uRayColor;
vec4 effect(vec2 uv) {
  vec4 c = texture2D(tColor, uv);
  float vis = uSunScreen.z * (1.0 - smoothstep(0.6, 1.3, length(uSunScreen.xy - 0.5) * 2.0));
  // the air in front of this pixel: little for the gun and a near spider, all of it for the sky and the far street
  float z = rawDepth(uv) >= 0.99999 ? 1e4 : linearDepth(uv);
  float air = 1.0 - exp(-z / uAir);
  c.rgb += texture2D(tRays, uv).rgb * uRayColor * uStrength * vis * air * 4.0;
  return c;
}`;

export function makeRays(kit, o = {}) {
  const THREE = kit.THREE;
  const params = Object.assign({ strength: 0.3, decay: 0.97, density: 0.9, color: '#ffc884', threshold: 0, air: 18 }, o);
  const mask = kit.post.custom({ stage: 'hdr', fragment: MASK, needsDepth: true, params,
    uniforms: { uThreshold: { value: 0 } }, bind: (p, u) => { u.uThreshold.value = p.threshold; } });
  const blur = kit.post.custom({ stage: 'hdr', fragment: BLUR, params,
    uniforms: { uDecay: { value: 0.97 }, uDensity: { value: 0.9 } }, bind: (p, u) => { u.uDecay.value = p.decay; u.uDensity.value = p.density; } });
  const comp = kit.post.custom({ stage: 'hdr', fragment: COMP, needsDepth: true, params,
    uniforms: { tRays: { value: null }, uStrength: { value: 0.3 }, uAir: { value: 18 }, uRayColor: { value: new THREE.Color() } },
    bind: (p, u) => { u.uStrength.value = p.strength; u.uAir.value = p.air; u.uRayColor.value.copy(kit.color(p.color)); } });
  return {
    isPass: true, kind: 'custom', stage: 'hdr', needsDepth: true, needsNormal: false, enabled: true, params, uniforms: comp.uniforms,
    render(pp, src, dst) {
      const a = pp.target('golden_rays_a', 2), b = pp.target('golden_rays_b', 2);
      mask.render(pp, src, a, false);
      blur.render(pp, a.texture, b, false);
      comp.uniforms.tRays.value = b.texture;
      comp.render(pp, src, dst, false);
    },
  };
}
