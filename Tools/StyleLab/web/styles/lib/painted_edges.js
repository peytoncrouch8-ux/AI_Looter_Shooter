// Screen-space curvature for the hand-made styles (03 Painted Frontier's bevel highlights, 01 Clay Frontier's rounded
// edges): from the view-space normal buffer, convex edges catch a light band (brighter on the sun's side) and concave
// creases take a soft dark line, so hard-edged meshes read as if their edges were rounded and painted (or squeezed
// out of clay). Neighbours across a depth jump are ignored, so silhouettes don't light up.
export function edgePaint(kit, o = {}) {
  const THREE = kit.THREE;
  const params = {
    width: o.width ?? 2.2,            // px at 1080p at 1 m; shrinks with distance (a fixed world-size bevel)
    minWidth: o.minWidth ?? 1.0, maxWidth: o.maxWidth ?? 3.0,
    worldWidth: o.worldWidth ?? 6.0,  // px x metres: the band is worldWidth / distance pixels wide
    convex: o.convex ?? 0.6, concave: o.concave ?? 0.35, litBias: o.litBias ?? 0.35,
    tint: o.tint || '#fff1d8', creaseTint: o.creaseTint || '#3a2c48', fadeFar: o.fadeFar ?? 120,
  };
  const sunV = new THREE.Vector3();
  return kit.post.custom({
    stage: 'hdr', needsDepth: true, needsNormal: true, params,
    uniforms: {
      uW: { value: new THREE.Vector4(6, 1, 3, 120) }, uK: { value: new THREE.Vector3(0.6, 0.35, 0.35) },
      uSunV: { value: new THREE.Vector3(0, 1, 0) }, uTintE: { value: new THREE.Color(1, 1, 1) }, uTintC: { value: new THREE.Color(0.2, 0.15, 0.3) },
    },
    bind(p, u) {
      u.uW.value.set(p.worldWidth, p.minWidth, p.maxWidth, p.fadeFar);
      u.uK.value.set(p.convex, p.concave, p.litBias);
      sunV.copy(kit.sun.sunDir).transformDirection(kit.camera.matrixWorldInverse);
      u.uSunV.value.copy(sunV);
      u.uTintE.value.copy(kit.color(p.tint));
      u.uTintC.value.copy(kit.color(p.creaseTint));
    },
    fragment: /* glsl */`
      uniform vec4 uW; uniform vec3 uK; uniform vec3 uSunV, uTintE, uTintC;
      vec4 effect(vec2 uv) {
        vec3 c = texture2D(tColor, uv).rgb;
        float raw = rawDepth(uv);
        if (raw > 0.99999 || raw < 0.0011) return vec4(c, 1.0);
        float z = linearDepth(uv);
        float s = uResolution.y / 1080.0;
        float wpx = clamp(uW.x / max(z, 0.1), uW.y, uW.z) * s;
        vec2 o = wpx / uResolution;
        vec3 n = viewNormal(uv);
        vec2 uR = uv + vec2(o.x, 0.0), uL = uv - vec2(o.x, 0.0), uU = uv + vec2(0.0, o.y), uD = uv - vec2(0.0, o.y);
        float tol = z * 0.035 + 0.04;
        float wx = step(abs(linearDepth(uR) - z), tol) * step(abs(linearDepth(uL) - z), tol);
        float wy = step(abs(linearDepth(uU) - z), tol) * step(abs(linearDepth(uD) - z), tol);
        vec3 nr = viewNormal(uR), nl = viewNormal(uL), nu = viewNormal(uU), nd = viewNormal(uD);
        float curv = ((nr.x - nl.x) * wx + (nu.y - nd.y) * wy) * 0.5;
        float fade = 1.0 - smoothstep(uW.w * 0.5, uW.w, z);
        float vex = smoothstep(0.05, 0.6, curv) * fade;
        float cav = smoothstep(0.05, 0.6, -curv) * fade;
        float lit = clamp(dot(n, uSunV) * 0.5 + 0.5, 0.0, 1.0);
        float l = slLuma(c);
        c += uTintE * vex * uK.x * (uK.z + lit) * max(l, 0.05) * 1.5;
        c = mix(c, c * uTintC * 2.0, cav * uK.y);
        return vec4(c, 1.0);
      }`,
  });
}
