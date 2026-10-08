// Inkslinger's GLSL: the world-space hatching every material runs (a kit `hook`), the ink contour pass and the comic
// muzzle star. Hatching is drawn in world space at about 6 cm spacing and steps to coarser spacing with distance (each
// step drops every other line, as an inker would), so lines stay 5 px or more apart on screen and never swim.

// The hatch strokes: three planar projections blended by the normal, two directions, two spacing levels. Strokes wobble
// per line, break where a pen would lift, and only live in the toon ramp's mid and shadow bands (`slLevel`); deep
// shade and baked occlusion take the cross-hatch. `crevices` adds the "inked texture": the texture set's own dark
// details (plank seams, cracks) are inked where they fall well below the texture's local mean; `terrain` does the same
// for the ground's detail textures and draws the ground's darker tones as dry pen strokes near the camera.
function hatchHook({ density = 1, crevices = true, far = [70, 170], grime = 1, terrain = false, tintK = 1 } = {}) {
  return /* glsl */`
{
  // one stroke family: lines across direction ANG at base spacing S (metres); the spacing doubles wherever the lines
  // would come closer than about 5.5 px on screen (measured across the lines, so ground seen edge-on stays crisp), the
  // two neighbouring levels blended
  #define INK_LINE(P2, DX2, DY2, S, ANG, WID, OUT) { \\
    vec2 _d = vec2(cos(ANG), sin(ANG)); \\
    vec2 _n = vec2(-_d.y, _d.x); \\
    float _u = dot(P2, _n); \\
    float _a = dot(P2, _d); \\
    float _fw = abs(dot(DX2, _n)) + abs(dot(DY2, _n)); \\
    float _lv = max(log2(_fw * 5.5 / (S)), 0.0); \\
    float _l0 = floor(_lv); float _fr = _lv - _l0; \\
    OUT = 0.0; \\
    for (int _k = 0; _k < 2; _k++) { \\
      float _s = (S) * exp2(_l0 + float(_k)); \\
      float _v = _u / _s; float _al = _a / _s; float _id = floor(_v); \\
      _v += (slVNoise(vec2(_al * 0.16, _id * 1.7)) - 0.5) * 0.5; \\
      float _f = abs(fract(_v) - 0.5) * 2.0; \\
      float _brk = smoothstep(0.22, 0.42, slVNoise(vec2(_al * 0.07 + _id * 3.1, _id * 0.73))); \\
      float _wd = (WID) * _brk * (0.75 + 0.5 * slVNoise(vec2(_al * 0.35, _id * 2.3))); \\
      float _aa = _fw / _s * 0.9 + 1e-3; \\
      OUT += (_k == 0 ? 1.0 - _fr : _fr) * (1.0 - smoothstep(_wd - _aa, _wd + _aa, _f)); \\
    } }
  vec3 inkC = vec3(0.0085, 0.0052, 0.0042);
  float camD = length(worldPos - cameraPosition);
  float farK = 1.0 - smoothstep(${far[0].toFixed(1)}, ${far[1].toFixed(1)}, camD);
  float level = slLevel;
  float mid = (1.0 - smoothstep(0.8, 0.9, level)) * farK;
  float deep = (1.0 - smoothstep(0.5, 0.6, level)) * farK;
  float cav = smoothstep(0.78, 0.45, ao) * ${grime.toFixed(2)} * farK;
  // the colourist's shade: a rust core where the light turns, a cool plum in the deep shade (at their own lightness)
  {
    float lum = slLuma(col);
    vec3 rust = vec3(0.42, 0.1, 0.045), plum = vec3(0.13, 0.1, 0.2);
    float core = mid * (1.0 - deep);
    col = mix(col, rust * (lum / slLuma(rust)), core * ${(0.3 * tintK).toFixed(3)});
    col = mix(col, plum * (lum / slLuma(plum)), deep * ${(0.4 * tintK).toFixed(3)});
  }
  float hatch = 0.0;
  // derivatives outside any branch (they are undefined in non-uniform control flow)
  vec3 dWx = dFdx(worldPos), dWy = dFdy(worldPos);
  if (mid > 0.001) {
    vec3 tw = pow(abs(N), vec3(6.0)); tw /= (tw.x + tw.y + tw.z);
    vec2 pX = worldPos.zy, pY = worldPos.xz, pZ = worldPos.xy;
    float wA = (0.3 + 0.1 * deep) * ${density.toFixed(2)};
    float wB = 0.3 * deep * ${density.toFixed(2)};
    float wC = 0.22 * cav;
    float a0, b0, c0;
    float hA = 0.0, hB = 0.0, hC = 0.0;
    // each plane: direction A at about 50 degrees, B across it (deep shade), C steep (grime in cavities)
    if (tw.x > 0.01) {
      INK_LINE(pX, dWx.zy, dWy.zy, 0.06, 0.87, wA, a0); hA += tw.x * a0;
      if (deep > 0.0) { INK_LINE(pX, dWx.zy, dWy.zy, 0.06, -0.6, wB, b0); hB += tw.x * b0; }
      if (cav > 0.0) { INK_LINE(pX, dWx.zy, dWy.zy, 0.042, 1.35, wC, c0); hC += tw.x * c0; }
    }
    if (tw.y > 0.01) {
      INK_LINE(pY, dWx.xz, dWy.xz, 0.06, 0.87, wA, a0); hA += tw.y * a0;
      if (deep > 0.0) { INK_LINE(pY, dWx.xz, dWy.xz, 0.06, -0.6, wB, b0); hB += tw.y * b0; }
      if (cav > 0.0) { INK_LINE(pY, dWx.xz, dWy.xz, 0.042, 1.35, wC, c0); hC += tw.y * c0; }
    }
    if (tw.z > 0.01) {
      INK_LINE(pZ, dWx.xy, dWy.xy, 0.06, 0.87, wA, a0); hA += tw.z * a0;
      if (deep > 0.0) { INK_LINE(pZ, dWx.xy, dWy.xy, 0.06, -0.6, wB, b0); hB += tw.z * b0; }
      if (cav > 0.0) { INK_LINE(pZ, dWx.xy, dWy.xy, 0.042, 1.35, wC, c0); hC += tw.z * c0; }
    }
    hatch = max(max(hA * mid, hB * deep), hC);
  }
  ${crevices ? /* glsl */`
  #ifdef USE_MAP
  {
    vec3 tb = texture(map, vMapUv, 0.0).rgb;
    vec3 tm = texture(map, vMapUv, 5.0).rgb;
    float crev = smoothstep(0.58, 0.32, slLuma(tb) / max(slLuma(tm), 0.015)) * farK;
    hatch = max(hatch, crev * 0.85);
  }
  #endif` : ''}
  ${terrain ? /* glsl */`
  {
    // the ground's own texture inked: pebbles and cracks well below the local mean
    vec2 tuvG = worldPos.xz / uTTile.x, tuvD = worldPos.xz / uTTile.y;
    float tb = mix(slLuma(texture(uTGrassBC, tuvG).rgb), slLuma(texture(uTDirtBC, tuvD).rgb), tDirt);
    float tm = mix(slLuma(texture(uTGrassBC, tuvG, 5.0).rgb), slLuma(texture(uTDirtBC, tuvD, 5.0).rgb), tDirt);
    float near = (1.0 - smoothstep(14.0, 40.0, camD)) * (1.0 - tSteep);
    float crev = smoothstep(0.5, 0.28, tb / max(tm, 0.015)) * near;
    hatch = max(hatch, crev * 0.75);
    // the ground's tone drawn as dry pen strokes: where the detail texture darkens, short strokes across the ground
    float tone = smoothstep(0.92, 0.72, tb / max(tm, 0.015)) * near;
    float gs;
    INK_LINE(worldPos.xz, dWx.xz, dWy.xz, 0.075, 0.25, 0.26, gs);
    hatch = max(hatch, gs * tone * 0.8);
    // the area's macro map inked the same way: wheel ruts, path edges and patch borders drawn as broken pen lines
    vec2 muv = clamp((worldPos.xz - uTRect.xy) / (uTRect.zw - uTRect.xy), 0.0, 1.0);
    float mb = slLuma(texture(uTMacro, muv, 0.0).rgb), mm = slLuma(texture(uTMacro, muv, 4.0).rgb);
    float rut = smoothstep(0.86, 0.7, mb / max(mm, 0.015));
    rut *= smoothstep(0.25, 0.5, slVNoise(worldPos.xz * 1.7)) * (1.0 - smoothstep(25.0, 90.0, camD));
    hatch = max(hatch, rut * 0.7);
  }` : ''}
  // spotted blacks: the deepest shade in crevices falls to ink
  float spot = deep * smoothstep(0.62, 0.35, ao) * 0.55 * ${Math.min(grime, 1).toFixed(2)};
  col = mix(col, inkC, clamp(max(hatch * 0.92, spot), 0.0, 1.0));
  #undef INK_LINE
}`;
}

export const INK_HOOK = hatchHook({});
// the viewmodel: its texture's dark parts are paint, not cracks, so no crevice ink
export const INK_HOOK_GUN = hatchHook({ crevices: false, grime: 0.5 });
export const INK_HOOK_SOFT = hatchHook({ density: 0.6, crevices: false, far: [40, 110], grime: 0.0 });
export const INK_HOOK_TERRAIN = hatchHook({ density: 0.9, crevices: false, far: [60, 150], grime: 0, terrain: true });

// The ink contour pass: silhouettes from inverse depth (zero on any plane, so flat ground draws no lines) sampled at a
// radius that swells near and thins far (about 3 px at arm's length, 1 px past 60 m), modulated along the line by
// world-space noise so strokes swell and taper; creases from the normals at half that width. Lines boil at 8 drawings a
// second (a hand-drawn tell; frozen in stills). Far lines melt into the colour behind them.
export const INK_OUTLINE = /* glsl */`
uniform mat4 uCamWorld;
uniform float uWNear, uWFar, uBoil, uCrease, uSilFar, uCreaseFar;
uniform vec3 uInk;
float invz(vec2 uv) { return 1.0 / linearDepth(uv); }
vec4 effect(vec2 uv) {
  vec3 c = texture2D(tColor, uv).rgb;
  float raw = rawDepth(uv);
  if (raw >= 0.99999) return vec4(c, 1.0);
  float d0 = linearDepth(uv);
  vec3 wp = (uCamWorld * vec4(viewPos(uv), 1.0)).xyz;
  float scale = uResolution.y / 1080.0;
  float boil = floor(uTime * 8.0) * uBoil;
  float n = slVNoise(wp.xz * 1.1 + vec2(wp.y * 0.9, boil * 7.13)) * 0.7 + slVNoise(wp.xz * 4.0 + wp.y * 2.3 + boil) * 0.3;
  float W = mix(uWFar, uWNear, 1.0 - smoothstep(2.5, 55.0, d0)) * (0.55 + 0.9 * n) * scale;
  float w0 = 1.0 / d0;
  vec2 px = max(W, 0.75) / uResolution;
  float sil = 0.0;
  vec2 D0 = vec2(1.0, 0.0), D1 = vec2(0.0, 1.0), D2 = vec2(0.7071, 0.7071), D3 = vec2(0.7071, -0.7071);
  for (int i = 0; i < 4; i++) {
    vec2 dir = i == 0 ? D0 : i == 1 ? D1 : i == 2 ? D2 : D3;
    vec2 o = dir * px;
    float a = invz(uv + o), b = invz(uv - o);
    float lap = (a + b - 2.0 * w0) / w0;
    sil = max(sil, smoothstep(0.025, 0.08, -lap));
  }
  sil *= 1.0 - smoothstep(uSilFar * 0.35, uSilFar, d0);
  float crease = 0.0;
  if (uCrease > 0.0) {
    vec2 pi = max(W * 0.5, 0.75) / uResolution;
    vec3 nC = viewNormal(uv);
    vec3 n1 = viewNormal(uv + vec2(pi.x, 0.0)), n2 = viewNormal(uv - vec2(pi.x, 0.0));
    vec3 n3 = viewNormal(uv + vec2(0.0, pi.y)), n4 = viewNormal(uv - vec2(0.0, pi.y));
    float nd = max(max(1.0 - dot(nC, n1), 1.0 - dot(nC, n2)), max(1.0 - dot(nC, n3), 1.0 - dot(nC, n4)));
    crease = smoothstep(0.28, 0.55, nd) * uCrease * (1.0 - smoothstep(uCreaseFar * 0.4, uCreaseFar, d0));
  }
  float e = max(sil, crease);
  vec3 lc = mix(uInk, c * 0.42, smoothstep(30.0, 260.0, d0));
  return vec4(mix(c, lc, clamp(e, 0.0, 1.0)), 1.0);
}`;

// The muzzle flash as a comic star: uneven jagged spikes, a white-hot core, yellow and orange rings and an ink rim,
// drawn opaque over the scene (normal blending) so the ink can be black. Swapped onto the engine's flash card while the
// style is active and put back when it leaves.
export const INK_FLASH_FS = /* glsl */`
varying vec2 vUv; varying vec3 vP; uniform vec3 uColor; uniform float uI, uSeed;
void main() {
  vec2 c = (vUv - 0.5) * 2.0;
  float r = length(c);
  float ang = atan(c.y, c.x) + uSeed * 6.2831853;
  float k = ang / 6.2831853 * 9.0;
  float id = floor(k), f = fract(k);
  float h = fract(sin((id + floor(uSeed * 13.0)) * 91.7) * 4375.5453);
  float len = 0.5 + 0.48 * h;
  float spike = mix(0.26, len, pow(1.0 - abs(f - 0.5) * 2.0, 1.6));
  float s = spike * (0.6 + 0.4 * uI);
  float aa = fwidth(r) * 1.2 + 1e-3;
  float outer = 1.0 - smoothstep(s + 0.07 - aa, s + 0.07 + aa, r);
  float body = 1.0 - smoothstep(s - aa, s + aa, r);
  float mid = 1.0 - smoothstep(s * 0.66 - aa, s * 0.66 + aa, r);
  float core = 1.0 - smoothstep(s * 0.36 - aa, s * 0.36 + aa, r);
  vec3 orange = vec3(1.0, 0.36, 0.05), yellow = uColor;
  vec3 col = vec3(0.008, 0.005, 0.004);
  col = mix(col, orange * 3.0, body);
  col = mix(col, yellow * 5.0, mid);
  col = mix(col, vec3(6.0, 5.8, 5.2), core);
  float a = outer * step(0.02, uI);
  if (a < 0.01) discard;
  gl_FragColor = vec4(col, a);
}`;
