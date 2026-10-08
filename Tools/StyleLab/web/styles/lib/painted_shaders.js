// Painted Frontier's material pieces: light painted into every surface the way a texture artist would (warm lifted
// tops, cool dark undersides, a dark-to-light gradient up each object from the ground, convex edges caught, brush-sized
// blotches of colour), the albedo regrouped into a few painted value bands, and shade pushed to blue-violet. Hooks run
// after lighting on `col`; the albedo work divides the light out (col / albedo), repaints the albedo and puts it back.

// The ground's height under any point, from the terrain's heights (a half-float texture built once), so a hook can
// tell how high above the ground a fragment is: the "dark feet, light heads" gradient.
let HEIGHTS = null;
export function groundHeights(kit) {
  if (HEIGHTS) return HEIGHTS;
  const THREE = kit.THREE;
  const t = kit.manifest.terrain && kit.manifest.terrain.heights;
  let hf = null;
  try { hf = window.lab && window.lab.engine && window.lab.engine.heights; } catch (e) { hf = null; }
  const uniforms = { uPtH: { value: null }, uPtHRect: { value: new THREE.Vector4(0, 0, 1, 1) }, uPtHOn: { value: 0 } };
  if (hf && hf.data && t) {
    const N = 512, step = hf.w / N;
    const data = new Uint16Array(N * N);
    for (let r = 0; r < N; r++) {
      for (let c = 0; c < N; c++) {
        // the mean of the cells this texel covers
        let s = 0, n = 0;
        for (let y = 0; y < step; y++) for (let x = 0; x < step; x++) { s += hf.data[(r * step + y) * hf.w + c * step + x]; n++; }
        data[r * N + c] = THREE.DataUtils.toHalfFloat(s / n);
      }
    }
    const tex = new THREE.DataTexture(data, N, N, THREE.RedFormat, THREE.HalfFloatType);
    tex.minFilter = THREE.LinearFilter; tex.magFilter = THREE.LinearFilter;
    tex.wrapS = tex.wrapT = THREE.ClampToEdgeWrapping;
    tex.flipY = false;
    tex.needsUpdate = true;
    uniforms.uPtH.value = tex;
    uniforms.uPtHRect.value.set(...hf.rect);
    uniforms.uPtHOn.value = 1;
  } else {
    const tex = new THREE.DataTexture(new Uint16Array([0]), 1, 1, THREE.RedFormat, THREE.HalfFloatType);
    tex.needsUpdate = true;
    uniforms.uPtH.value = tex;
  }
  HEIGHTS = {
    uniforms,
    pars: 'uniform sampler2D uPtH; uniform vec4 uPtHRect; uniform float uPtHOn;\n'
      + 'float ptAbove(vec3 wp) { vec2 uv = (wp.xz - uPtHRect.xy) / (uPtHRect.zw - uPtHRect.xy);\n'
      + '  if (uPtHOn < 0.5 || uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) return 6.0;\n'
      + '  return wp.y - texture2D(uPtH, uv).r; }\n',
  };
  return HEIGHTS;
}

// The normal map read at a softer mip (a LOD bias), so only a set's big shapes survive as relief (planks, stones),
// for the kit's `_extra.normal` slot: kept by the copies the engine makes for batched meshes, unlike swapping the
// material's normalMap after building it. Clay Frontier uses it too.
export function biasedNormalChunk(THREE, bias) {
  const chunk = THREE.ShaderChunk.normal_fragment_maps;
  const b = Number(bias).toFixed(2);
  const out = chunk.split('texture2D( normalMap, vNormalMapUv )').join(`texture( normalMap, vNormalMapUv, ${b} )`);
  return out + `
#ifdef SL_UPNORMAL
  normal = normalize(mix(normal, (viewMatrix * vec4(0.0, 1.0, 0.0, 0.0)).xyz, uSlUpNormal));
#endif
`;
}

// Brush strokes: dabs in three tones (cool dark, base, warm light) laid along a slowly turning direction, on the plane
// a surface mostly faces. Gives `float ptStroke` in -1..1. `len`/`wid` are a stroke's length and width in metres.
const STROKES = (len, wid, fadeFar) => /* glsl */`
    float ptStroke = 0.0;
    {
      vec3 an = abs(N);
      vec2 sp = an.y > max(an.x, an.z) ? worldPos.xz : (an.x > an.z ? worldPos.zy : worldPos.xy);
      float sDist = length(worldPos - cameraPosition);
      float sFade = 1.0 - smoothstep(${(fadeFar * 0.4).toFixed(1)}, ${fadeFar.toFixed(1)}, sDist);
      if (sFade > 0.0) {
        float ang = (slVNoise(sp * 0.12 + 1.7) - 0.5) * 2.4 + (an.y > max(an.x, an.z) ? 0.0 : 0.0);
        vec2 q = mat2(cos(ang), sin(ang), -sin(ang), cos(ang)) * sp;
        float st = slVNoise(q / vec2(${len.toFixed(3)}, ${wid.toFixed(3)})) * 0.65 + slVNoise(q / vec2(${(len * 0.5).toFixed(3)}, ${(wid * 0.5).toFixed(3)}) + 5.3) * 0.35;
        ptStroke = (smoothstep(0.36, 0.4, st) + smoothstep(0.62, 0.66, st) - 1.0) * sFade;
      }
    }
`;

const V3 = (a) => `vec3(${a.map((x) => x.toFixed(3)).join(', ')})`;

// o: { bands (painted value groups, 0 = off), blotch (brush-blotch strength), warmTop, coolUnder, feet (how dark the
// foot of an object goes), feetHeight (m), shadeTint ([r,g,b] multiplier in shade), shadeAmount, heights (bool) }
export function paintHook(o = {}) {
  const bands = o.bands ?? 4;
  return /* glsl */`
  {
    vec3 ptAlb = max(diffuseColor.rgb, vec3(0.004));
    vec3 ptLight = col / ptAlb;
    vec3 alb = ptAlb;
    // brush-sized blotches: value and hue wander at the scale of a brush dab
    vec3 bp = worldPos * ${(o.blotchScale ?? 1.4).toFixed(3)};
    float b1 = slFbm(bp.xz + bp.y * 0.73 + vec2(bp.y * 0.31, 0.0));
    float b2 = slVNoise(bp.zy * 1.9 + bp.x * 0.4 + 9.0);
    ${bands > 0 ? `
    // regroup the albedo's values into a few painted bands (soft-edged, the edges wandering with the brush)
    float l = slLuma(alb);
    float lb = log2(l + 0.003) * ${(bands / 4).toFixed(3)} + (b1 - 0.5) * 0.6;
    float f = fract(lb);
    float lq = floor(lb) + smoothstep(0.35, 0.65, f);
    float lt = exp2(lq / ${(bands / 4).toFixed(3)}) - 0.003;
    alb *= mix(1.0, lt / max(l, 0.003), ${(o.bandAmount ?? 0.7).toFixed(3)});` : ''}
    alb *= 1.0 + (b1 - 0.5) * ${(o.blotch ?? 0.28).toFixed(3)};
    ${(o.strokes ?? 0.1) > 0 ? STROKES(o.strokeLen ?? 0.45, o.strokeWid ?? 0.07, o.strokeFar ?? 45) + `
    alb *= 1.0 + ptStroke * ${(o.strokes ?? 0.1).toFixed(3)};
    alb *= mix(vec3(1.0), ptStroke > 0.0 ? vec3(1.04, 1.0, 0.93) : vec3(0.95, 0.97, 1.06), abs(ptStroke));` : ''}
    vec3 hsv = slRgb2Hsv(alb); hsv.x = fract(hsv.x + (b2 - 0.5) * 0.035); hsv.y = clamp(hsv.y * (1.0 + (b2 - 0.5) * 0.3), 0.0, 1.0);
    alb = slHsv2Rgb(hsv);
    col = alb * ptLight;
    // painted light: tops lifted and warmed, undersides darkened and cooled
    float up = N.y;
    col *= mix(vec3(1.0), ${V3(o.warmTop ?? [1.16, 1.06, 0.88])}, clamp(up, 0.0, 1.0) * 0.55);
    col *= mix(vec3(1.0), ${V3(o.coolUnder ?? [0.62, 0.66, 0.92])}, clamp(-up, 0.0, 1.0) * 0.8);
    ${(o.bevel ?? 1) > 0 ? `
    // painted bevels: the texture's relief catches the light on its upper edges and goes dark underneath, as if the
    // light had been painted in from above (every plank gets a lit top edge and a shadowed lip)
    vec3 ptNg = normalize(inverseTransformDirection(normalize(vNormal), viewMatrix));
    vec3 ptD = N - ptNg * dot(N, ptNg);
    col *= 1.0 + clamp((ptD.y * 1.8 + dot(ptD, L) * 0.9) * ${(o.bevel ?? 1).toFixed(3)}, -0.45, 0.6);` : ''}
    ${o.heights ? `
    // dark feet, light heads: a gradient up each object from the ground
    float hh = ptAbove(worldPos);
    col *= mix(${(o.feet ?? 0.62).toFixed(3)}, 1.06, smoothstep(-0.05, ${(o.feetHeight ?? 3.2).toFixed(3)}, hh));` : ''}
    // shade goes blue-violet, never grey, and stays light: a painted sky fill
    float lit = shadow * smoothstep(-0.05, 0.3, NdL);
    col = mix(col, col * ${V3(o.shadeTint ?? [0.80, 0.74, 1.28])}, (1.0 - lit) * ${(o.shadeAmount ?? 0.55).toFixed(3)});
    col += alb * vec3(0.30, 0.30, 0.52) * (1.0 - lit) * ${(o.fill ?? 0.55).toFixed(3)} * ao;
  }
  `;
}

// The ground repainted (shared by the three cartoon styles): the terrain's own blend weights (tDirt, tRock, tSteep)
// pick a style's dirt, grass and rock colours, and `keep` (0-1) keeps that much of the macro map's light and dark so
// the ground still has the area's patches. Colours are sRGB hex; the light already on `col` is kept.
export function groundRecolor(kit, { dirt, grass, rock, keep = 0.6, lref = 0.15 } = {}) {
  const v = (hex) => { const c = kit.color(hex); return `vec3(${c.r.toFixed(4)}, ${c.g.toFixed(4)}, ${c.b.toFixed(4)})`; };
  return /* glsl */`
  {
    vec3 rgAlb = max(diffuseColor.rgb, vec3(0.004));
    vec3 rgLight = col / rgAlb;
    float wD = clamp(tDirt, 0.0, 1.0), wR = clamp(max(tRock, tSteep), 0.0, 1.0);
    vec3 rgT = mix(mix(${v(grass)}, ${v(dirt)}, wD), ${v(rock)}, wR);
    float rgVar = clamp(pow(slLuma(rgAlb) / ${lref.toFixed(3)}, ${keep.toFixed(3)}), 0.45, 1.7);
    col = rgLight * rgT * rgVar;
  }
  `;
}

// The ground: big brushed patches of warm and cool, sun-baked lit ground, violet shade.
export function paintGroundHook() {
  return /* glsl */`
  {
    float dist = length(worldPos - cameraPosition);
    float b1 = slFbm(worldPos.xz * 0.16);
    float b2 = slFbm(worldPos.xz * 0.9 + 7.0);
    float b3 = slVNoise(worldPos.xz * vec2(2.8, 0.9) + 3.0);
    col *= 1.0 + (b1 - 0.5) * 0.30 + (b2 - 0.5) * 0.16 * (1.0 - smoothstep(30.0, 120.0, dist)) + (b3 - 0.5) * 0.08;
    vec3 hsv = slRgb2Hsv(max(col, 0.0)); hsv.x = fract(hsv.x + (b1 - 0.5) * 0.05); col = slHsv2Rgb(hsv);
    ${STROKES(1.1, 0.16, 90)}
    col *= 1.0 + ptStroke * 0.1;
    col *= mix(vec3(1.0), ptStroke > 0.0 ? vec3(1.05, 1.0, 0.9) : vec3(0.93, 0.96, 1.08), abs(ptStroke));
    float lit = shadow * smoothstep(-0.05, 0.3, NdL);
    col *= mix(vec3(0.82, 0.82, 1.2), vec3(1.07, 1.0, 0.9), lit) ;
  }
  `;
}
