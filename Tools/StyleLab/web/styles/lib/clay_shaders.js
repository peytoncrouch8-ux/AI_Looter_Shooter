// Clay Frontier's pieces: the plasticine surface (a material hook), the felt-and-clay ground, the macro lens (an
// autofocus depth of field) and the clay palette. Hooks run after lighting on `col` (see the Style Lab README), so the
// thumb-worked relief re-lights the colour from a perturbed normal instead of changing the lit normal: cheap, and the
// waxy sheen it adds is what sells plasticine.

// A world-space relief for clay: tool strokes that turn region by region like a thumb working round a form, and
// shallow oval thumb dents. Gives `vec3 Np` (the worked normal) from N and worldPos. `scale` sets the stroke size.
const RELIEF = (strength, scale, dents) => /* glsl */`
  float clDist = length(worldPos - cameraPosition);
  float clFade = 1.0 - smoothstep(${(5 * scale).toFixed(1)}, ${(25 * scale).toFixed(1)}, clDist);
  vec3 Np = N;
  float clCav = 0.0;
  if (clFade > 0.0) {
    vec3 an = abs(N);
    vec2 sp; vec3 Tw, Bw;
    if (an.y > an.x && an.y > an.z) { sp = worldPos.xz; Tw = vec3(1.0, 0.0, 0.0); Bw = vec3(0.0, 0.0, 1.0); }
    else if (an.x > an.z) { sp = worldPos.zy; Tw = vec3(0.0, 0.0, 1.0); Bw = vec3(0.0, 1.0, 0.0); }
    else { sp = worldPos.xy; Tw = vec3(1.0, 0.0, 0.0); Bw = vec3(0.0, 1.0, 0.0); }
    sp *= ${(1 / scale).toFixed(3)};
    float ang = slVNoise(sp * 0.8 + 3.7) * 6.2831853;
    mat2 R = mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
    vec2 q = R * sp;
    vec2 sc = vec2(20.0, 64.0);
    float e = 0.0045;
    float n0 = slVNoise(q * sc) + 0.45 * slVNoise(q * sc * 2.3 + 7.0);
    float nx = slVNoise((q + vec2(e, 0.0)) * sc) + 0.45 * slVNoise((q + vec2(e, 0.0)) * sc * 2.3 + 7.0);
    float ny = slVNoise((q + vec2(0.0, e)) * sc) + 0.45 * slVNoise((q + vec2(0.0, e)) * sc * 2.3 + 7.0);
    vec2 g = vec2(nx - n0, ny - n0) / e * 0.0065;
    g = vec2(dot(vec2(R[0][0], R[0][1]), g), dot(vec2(R[1][0], R[1][1]), g));
    clCav = (n0 - 0.72) * 0.6;
    ${dents ? `
    // thumb dents: a shallow oval in some 8 cm cells
    vec2 cell = floor(sp / 0.08);
    float hc = slHash12(cell + 11.0);
    if (hc < 0.42) {
      vec2 ctr = (cell + 0.5 + (slHash22(cell) - 0.5) * 0.35) * 0.08;
      float da = hc * 40.0;
      mat2 Rd = mat2(cos(da), sin(da), -sin(da), cos(da));
      vec2 dq = Rd * (sp - ctr) / vec2(0.034, 0.024);
      float rr = dot(dq, dq);
      if (rr < 1.0) {
        vec2 dg = 4.0 * (1.0 - rr) * dq / vec2(0.034, 0.024);
        dg = vec2(dot(Rd[0], dg), dot(Rd[1], dg));
        g += dg * 0.0055;
        clCav -= (1.0 - rr) * (1.0 - rr) * 0.5;
      }
    }` : ''}
    Np = normalize(N - (Tw * g.x + Bw * g.y) * ${strength.toFixed(3)} * clFade);
  }
`;

// The plasticine shading on top of the toon ramp: the relief re-lit, a waxy two-lobe sheen that breaks up along the
// strokes, a red-orange glow in the terminator and the penumbra (light through clay), and a slow hue drift so no two
// walls are the same batch of clay.
// ramp: four linear [r, g, b] stops; when given, the albedo is gradient-mapped through them by its lightness (a trim
// sheet's dark stripe, siding, plaster and tin each become one of the set's clay colours), keeping a little of its hue.
export function clayHook({ strength = 1.0, scale = 1.0, dents = true, sheen = 1.0, glow = 1.0, drift = 1.0, ramp = null } = {}) {
  const v = (c) => `vec3(${c.map((x) => x.toFixed(4)).join(', ')})`;
  return /* glsl */`
  ${ramp ? `
  {
    vec3 a0 = max(diffuseColor.rgb, vec3(0.004));
    vec3 lightIn = col / a0;
    float t = clamp(pow(slLuma(a0) / 0.42, 0.55), 0.0, 1.0);
    vec3 g = mix(${v(ramp[0])}, ${v(ramp[1])}, smoothstep(0.0, 0.38, t));
    g = mix(g, ${v(ramp[2])}, smoothstep(0.32, 0.66, t));
    g = mix(g, ${v(ramp[3])}, smoothstep(0.62, 0.95, t));
    vec3 keep = a0 * (slLuma(g) / slLuma(a0));
    col = lightIn * mix(g, keep, 0.3);
  }` : ''}
  ${RELIEF(strength, scale, dents)}
  float clLit = shadow * smoothstep(-0.1, 0.25, NdL);
  col *= 1.0 + (dot(Np, L) - dot(N, L)) * 1.1 * clLit + (Np.y - N.y) * 0.35;
  // the worked surface holds a little dirt in its dents and catches light on its ridges, lit or not
  col *= 1.0 + clCav * 0.3 * clFade;
  // light scattered inside the clay keeps its shadows rich: chroma up where the sun doesn't reach
  { float l0 = slLuma(col); col = max(mix(vec3(l0), col, 1.0 + 0.4 * (1.0 - clLit)), 0.0); }
  vec3 clH = normalize(L + V);
  float clS1 = pow(max(dot(Np, clH), 0.0), 22.0);
  float clS2 = pow(max(dot(N, clH), 0.0), 5.0);
  col += slSunCol * (clS1 * 0.16 + clS2 * 0.06) * ${sheen.toFixed(3)} * clLit;
  // a soft sky sheen at grazing angles (the fill light on wax)
  col += vec3(0.55, 0.65, 0.85) * pow(1.0 - max(dot(Np, V), 0.0), 4.0) * 0.06 * ${sheen.toFixed(3)} * ao;
  float clTerm = smoothstep(-0.4, 0.0, NdL) * (1.0 - smoothstep(0.0, 0.42, NdL));
  float clPen = clamp(shadow * (1.0 - shadow) * 4.0, 0.0, 1.0) * step(0.0, NdL);
  col += diffuseColor.rgb * vec3(1.0, 0.36, 0.14) * slSunCol * (clTerm * shadow * 0.10 + clPen * 0.07) * ${glow.toFixed(3)};
  // the clay batch: hue, chroma and value drift slowly through the world
  float clHd = slVNoise(worldPos.xz * 0.055 + worldPos.y * 0.03 + 17.0) - 0.5;
  float clHd2 = slVNoise(worldPos.xz * 0.11 + 41.0) - 0.5;
  vec3 clHsv = slRgb2Hsv(max(col, 0.0));
  clHsv.x = fract(clHsv.x + clHd * 0.045 * ${drift.toFixed(3)});
  clHsv.y = clamp(clHsv.y * (1.0 + clHd2 * 0.5 * ${drift.toFixed(3)}), 0.0, 1.0);
  col = slHsv2Rgb(clHsv) * (1.0 + clHd2 * 0.2 * ${drift.toFixed(3)});
  `;
}

// The ground: green felt where grass grows (fibres, a velvet sheen at grazing angles, a soft nap), smoothed clay on
// the roads with tool strokes, and sculpted clay with pressed strata on the slopes and cliffs. Uses the terrain
// material's own blend weights (tDirt, tRock, tSteep).
export function feltHook() {
  return /* glsl */`
  ${RELIEF(0.38, 7.0, true)}
  float fDirt = clamp(max(tDirt, 0.0), 0.0, 1.0);
  float fRock = clamp(max(tRock, tSteep), 0.0, 1.0);
  float fFelt = (1.0 - fDirt) * (1.0 - fRock);
  float dist = length(worldPos - cameraPosition);
  // felt: fibres at two scales, fading to its mean before they alias
  float ff = 1.0 - smoothstep(10.0, 70.0, dist);
  float fib = slVNoise(worldPos.xz * 48.0) * 0.55 + slVNoise(worldPos.xz * vec2(120.0, 26.0) + 3.0) * 0.45;
  float nap = slFbm(worldPos.xz * 0.7);
  float feltMod = 1.0 + ((fib - 0.5) * 0.16 * ff + (nap - 0.5) * 0.2) * fFelt;
  // the clay parts take the worked relief
  float clayW = 1.0 - fFelt;
  vec3 Nc = normalize(mix(N, Np, clayW));
  // pressed strata on the steep clay
  float strata = sin(worldPos.y * 7.0 + slVNoise(worldPos.xz * 0.5) * 6.0);
  float stF = fRock * (1.0 - smoothstep(20.0, 90.0, dist));
  Nc = normalize(Nc + vec3(0.0, strata * 0.12 * stF, 0.0));
  float lit = shadow * smoothstep(-0.1, 0.25, NdL);
  col *= feltMod;
  col *= 1.0 + (dot(Nc, L) - dot(N, L)) * 1.1 * lit + (Nc.y - N.y) * 0.3;
  // velvet: felt brightens toward grazing view and in the light
  float graze = pow(1.0 - max(dot(N, V), 0.0), 3.0);
  col += diffuseColor.rgb * slSunCol * graze * 0.10 * fFelt * lit;
  col *= 1.0 - 0.08 * fFelt * (1.0 - graze);
  // waxy sheen on the clay roads and rocks
  vec3 H = normalize(L + V);
  col += slSunCol * pow(max(dot(Nc, H), 0.0), 20.0) * 0.05 * clayW * lit;
  // light through clay at the shadow's edge
  float pen = clamp(shadow * (1.0 - shadow) * 4.0, 0.0, 1.0);
  col += diffuseColor.rgb * vec3(1.0, 0.36, 0.14) * slSunCol * pen * 0.05;
  float hd = slVNoise(worldPos.xz * 0.05 + 4.0) - 0.5;
  col *= 1.0 + hd * 0.12;
  `;
}

// The macro lens: an autofocus depth of field. It focuses on the nearest thing just under the crosshair and blurs by
// how many octaves of distance a point is from that focus (a thin lens with a big aperture over a small set: the
// miniature tell), so the street, the knoll and the overview all read as a tabletop. The gun blurs a touch.
export function macroLens(kit, o = {}) {
  const params = { blur: o.blur ?? 7, plateau: o.plateau ?? 0.85, ramp: o.ramp ?? 2.2, near: o.near ?? 1.0, gun: o.gun ?? 1.4,
    minFocus: o.minFocus ?? 3, maxFocus: o.maxFocus ?? 260 };
  return kit.post.custom({
    stage: 'hdr', needsDepth: true, params,
    uniforms: { uK: { value: 7 }, uPlateau: { value: 0.85 }, uRamp: { value: 2.2 }, uNearK: { value: 1 }, uGun: { value: 1.4 },
      uFocusR: { value: new kit.THREE.Vector2(3, 260) } },
    bind(p, u) {
      u.uK.value = p.blur; u.uPlateau.value = p.plateau; u.uRamp.value = p.ramp; u.uNearK.value = p.near; u.uGun.value = p.gun;
      u.uFocusR.value.set(p.minFocus, p.maxFocus);
    },
    fragment: /* glsl */`
      uniform float uK, uPlateau, uRamp, uNearK, uGun;
      uniform vec2 uFocusR;
      float fd(vec2 uv) { return rawDepth(uv) < 0.0011 ? 1e4 : linearDepth(uv); }
      float focusDist() {
        // the median of five depths round the crosshair: what the player is looking at, not a stray post
        float a = fd(vec2(0.5, 0.40)), b = fd(vec2(0.43, 0.42)), c = fd(vec2(0.57, 0.42)), d = fd(vec2(0.5, 0.46)), e = fd(vec2(0.5, 0.36));
        float t;
        if (a > b) { t = a; a = b; b = t; } if (d > e) { t = d; d = e; e = t; } if (a > c) { t = a; a = c; c = t; }
        if (b > c) { t = b; b = c; c = t; } if (a > d) { t = a; a = d; d = t; } if (c > d) { t = c; c = d; d = t; }
        if (b > e) { t = b; b = e; e = t; } if (b > c) { t = b; b = c; c = t; }
        return clamp(c, uFocusR.x, uFocusR.y);
      }
      float coc(vec2 uv, float zf) {
        if (rawDepth(uv) < 0.0011) return uGun;
        float z = linearDepth(uv);
        float r = abs(log2(z / zf));
        float c = clamp((r - uPlateau) / uRamp, 0.0, 1.0);
        c = c * c * (3.0 - 2.0 * c);
        return c * uK * (z < zf ? uNearK : 1.0);
      }
      vec4 effect(vec2 uv) {
        float s = uResolution.y / 1080.0;
        float zf = focusDist();
        vec3 c0 = texture2D(tColor, uv).rgb;
        float r0 = coc(uv, zf) * s;
        if (r0 < 0.5) return vec4(c0, 1.0);
        vec3 acc = c0; float w = 1.0;
        float rot = slIGN(gl_FragCoord.xy) * 6.2831853;
        for (int i = 1; i < 28; i++) {
          float rr = sqrt(float(i) / 28.0) * r0;
          float an = float(i) * 2.39996323 + rot;
          vec2 off = vec2(cos(an), sin(an)) * rr / uResolution;
          vec3 sc = texture2D(tColor, uv + off).rgb;
          float sr = coc(uv + off, zf) * s;
          float k = smoothstep(rr - 1.0, rr + 1.0, sr + 0.5);
          acc += sc * k; w += k;
        }
        return vec4(acc / w, 1.0);
      }`,
  });
}

// The palette: each material's representative colour turned into a clay of the same family: chroma up, darks lifted
// (plasticine is never black), hue nudged warm, with a target lightness band per role.
const ROLE_PLATES = {
  wood: { s: [1.9, 0.16], l: [0.34, 0.66], warm: 5 },
  stone: { s: [1.4, 0.08], l: [0.50, 0.76], warm: 2 },
  rock: { s: [1.9, 0.18], l: [0.42, 0.70], warm: 8 },
  ground: { s: [1.4, 0.08], l: [0.36, 0.62], warm: 4 },
  metal: { s: [1.2, 0.04], l: [0.22, 0.50], warm: -6 },
  bark: { s: [1.4, 0.08], l: [0.24, 0.48], warm: 4 },
  foliage: { s: [1.45, 0.10], l: [0.26, 0.50], warm: -4 },
  grass: { s: [1.4, 0.08], l: [0.40, 0.62], warm: -2 },
  fabric: { s: [1.3, 0.06], l: [0.30, 0.82], warm: 2 },
  paper: { s: [1.2, 0.05], l: [0.55, 0.82], warm: 2 },
  gun: { s: [1.35, 0.06], l: [0.20, 0.74], warm: 0 },
  creature: { s: [1.0, 0.0], l: [0.0, 1.0], warm: 0 },
  other: { s: [1.3, 0.06], l: [0.25, 0.75], warm: 2 },
};
export function clayColor(THREE, hex, role) {
  const p = ROLE_PLATES[role] || ROLE_PLATES.other;
  const c = new THREE.Color(hex || '#888888');
  const hsl = {}; c.getHSL(hsl);
  let h = hsl.h * 360;
  // browns and ochres drift a little toward a warm terracotta; greens stay sap-green
  if (h < 60 || h > 330) h += p.warm;
  else if (h > 60 && h < 160) h -= p.warm * 0.5;
  const s = Math.min(0.88, hsl.s * p.s[0] + p.s[1]);
  const l = p.l[0] + (p.l[1] - p.l[0]) * Math.min(1, hsl.l / 0.62);
  return '#' + new THREE.Color().setHSL(((h % 360) + 360) % 360 / 360, s, l).getHexString();
}
