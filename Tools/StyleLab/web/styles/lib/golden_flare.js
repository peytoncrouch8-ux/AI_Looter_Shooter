// Golden Hour's lens: when the sun is in frame and not behind something, a restrained cinema-lens flare (a soft
// six-point glare round the sun, a faint warm veil, and a few coloured ghosts strung through the centre of the frame)
// whose strength follows how much of the sun's disc is clear sky. Scene-referred, before the bloom, so the bloom
// softens it like real glass.
// In Unreal: the built-in Lens Flare (Image Based) with a custom bokeh shape, intensity 0.15, threshold 8, tinted by
// the sun colour; or a post material doing this pass (one depth fetch ring + a handful of discs, ~0.05 ms).

const FRAG = /* glsl */`
uniform float uStrength, uGhosts, uGlare, uVeil;
uniform vec3 uFlareCol;
float sunClear(vec2 s) {
  float v = 0.0;
  for (int i = 0; i < 16; i++) {
    float a = float(i) * 0.3927;
    float r = (i < 8) ? 0.004 : 0.009;
    vec2 o = vec2(cos(a), sin(a)) * r * vec2(uResolution.y / uResolution.x, 1.0);
    vec2 p = s + o;
    float inside = step(0.0, p.x) * step(p.x, 1.0) * step(0.0, p.y) * step(p.y, 1.0);
    v += step(0.99999, rawDepth(clamp(p, 0.0, 1.0))) * inside;
  }
  return v / 16.0;
}
vec3 ghost(vec2 uv, vec2 c, float r, vec3 col, vec2 asp) {
  float d = length((uv - c) * asp);
  float disc = 1.0 - smoothstep(r * 0.82, r, d);
  float rim = smoothstep(r * 0.6, r * 0.95, d) * disc;
  return col * (disc * 0.55 + rim * 0.9);
}
vec4 effect(vec2 uv) {
  vec3 c = texture2D(tColor, uv).rgb;
  vec2 s = uSunScreen.xy;
  if (uSunScreen.z < 0.5 || s.x < -0.05 || s.x > 1.05 || s.y < -0.05 || s.y > 1.05) return vec4(c, 1.0);
  float vis = sunClear(s);
  if (vis <= 0.0) return vec4(c, 1.0);
  vec2 asp = vec2(uResolution.x / uResolution.y, 1.0);
  vec2 axis = vec2(0.5) - s;
  vec3 add = vec3(0.0);
  // ghosts along the line through the centre, sized and tinted like a multi-coated lens
  add += ghost(uv, s + axis * 0.62, 0.035, vec3(1.0, 0.72, 0.38), asp);
  add += ghost(uv, s + axis * 1.05, 0.07, vec3(0.45, 0.85, 0.75), asp) * 0.6;
  add += ghost(uv, s + axis * 1.32, 0.022, vec3(1.0, 0.55, 0.3), asp) * 1.2;
  add += ghost(uv, s + axis * 1.7, 0.12, vec3(0.62, 0.55, 1.0), asp) * 0.35;
  add += ghost(uv, s + axis * 2.05, 0.05, vec3(1.0, 0.85, 0.55), asp) * 0.6;
  add *= uGhosts;
  // glare: six soft spokes and a warm veil round the sun
  vec2 d = (uv - s) * asp;
  float rad = length(d);
  float ang = atan(d.y, d.x) + 0.3;
  float spokes = pow(abs(cos(ang * 3.0)), 60.0) * exp(-rad * 9.0) + pow(abs(cos(ang * 3.0 + 0.5)), 120.0) * exp(-rad * 14.0) * 0.6;
  add += uFlareCol * spokes * uGlare;
  add += uFlareCol * exp(-rad * 3.2) * uVeil;
  // fade as the sun nears the frame's edge
  float edge = smoothstep(0.0, 0.12, min(min(s.x, 1.0 - s.x), min(s.y, 1.0 - s.y)) + 0.05);
  c += add * uFlareCol * vis * edge * uStrength;
  return vec4(c, 1.0);
}`;

export function makeFlare(kit, o = {}) {
  const params = Object.assign({ strength: 1, ghosts: 0.12, glare: 1.2, veil: 0.25, color: '#ffc27a' }, o);
  return kit.post.custom({
    stage: 'hdr', fragment: FRAG, params, needsDepth: true,
    uniforms: { uStrength: { value: 1 }, uGhosts: { value: 0.12 }, uGlare: { value: 1 }, uVeil: { value: 0.2 }, uFlareCol: { value: new kit.THREE.Color() } },
    bind: (p, u) => {
      u.uStrength.value = p.strength; u.uGhosts.value = p.ghosts; u.uGlare.value = p.glare; u.uVeil.value = p.veil;
      u.uFlareCol.value.copy(kit.color(p.color));
    },
  });
}
