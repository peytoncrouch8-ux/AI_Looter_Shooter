// A muzzle flash of a style's own: while the style is active, the gun's flash cards (the engine's three crossed quads
// on the viewmodel, with uniforms uColor, uI, uSeed) draw this style's fragment shader instead of the engine's star;
// deactivate puts the engine's shader back. Used by 01 Clay Frontier (a cotton-wool burst) and 03 Painted Frontier
// (a chunky painted burst).
function findFlash(kit) {
  let hit = null;
  if (!kit.viewmodel) return null;
  kit.viewmodel.traverse((o) => {
    if (!hit && o.isMesh && o.material && o.material.uniforms && o.material.uniforms.uSeed && o.material.uniforms.uI) hit = o;
  });
  return hit;
}

export function swapFlash(kit, fragmentShader) {
  const m = findFlash(kit);
  if (!m) return;
  const mat = m.material;
  if (!mat.userData.slOrigFrag) mat.userData.slOrigFrag = mat.fragmentShader;
  mat.fragmentShader = fragmentShader;
  mat.needsUpdate = true;
}

export function restoreFlash(kit) {
  const m = findFlash(kit);
  if (!m) return;
  const mat = m.material;
  if (mat.userData.slOrigFrag && mat.fragmentShader !== mat.userData.slOrigFrag) {
    mat.fragmentShader = mat.userData.slOrigFrag;
    mat.needsUpdate = true;
  }
}

// Clay: a burst of cotton-wool puffs round a hot core, each puff with a soft rounded body.
export const COTTON_FLASH = /* glsl */`
varying vec2 vUv; varying vec3 vP; uniform vec3 uColor; uniform float uI, uSeed;
float fh(float n) { return fract(sin(n * 91.7 + uSeed * 13.1) * 43758.5453); }
void main() {
  vec2 c = (vUv - 0.5) * 2.0;
  float a = 0.0, shade = 0.0;
  for (int i = 0; i < 8; i++) {
    float fi = float(i);
    float ang = fi * 0.785 + fh(fi) * 0.6;
    vec2 p = vec2(cos(ang), sin(ang)) * (0.32 + 0.22 * fh(fi + 7.0));
    float r = 0.24 + 0.16 * fh(fi + 3.0);
    float d = length(c - p) / r;
    float body = 1.0 - smoothstep(0.82, 1.0, d);
    float lit = 0.65 + 0.35 * clamp(1.0 - length(c - p - vec2(-0.25, 0.3) * r) / r, 0.0, 1.0);
    if (body > a) { a = body; shade = lit; }
  }
  float core = 1.0 - smoothstep(0.0, 0.42, length(c));
  a = max(a, core);
  vec3 col = uColor * a * shade * 3.2 + vec3(1.0, 0.96, 0.82) * core * 3.5;
  gl_FragColor = vec4(col * uI, 1.0);
}`;

// Painted: a fat four-lobed burst with a painted orange rim and a cream core, hard-edged like a brush shape.
export const PAINTED_FLASH = /* glsl */`
varying vec2 vUv; varying vec3 vP; uniform vec3 uColor; uniform float uI, uSeed;
void main() {
  vec2 c = (vUv - 0.5) * 2.0;
  float r = length(c), ang = atan(c.y, c.x) + uSeed * 6.2831853;
  float lobe = 0.48 + 0.42 * pow(abs(cos(ang * 2.0)), 3.0) + 0.08 * sin(ang * 7.0);
  float body = 1.0 - smoothstep(lobe - 0.06, lobe, r);
  float inner = 1.0 - smoothstep(lobe * 0.55 - 0.05, lobe * 0.55, r);
  vec3 col = uColor * body * 3.0 + vec3(1.0, 0.95, 0.75) * inner * 3.0;
  gl_FragColor = vec4(col * uI, 1.0);
}`;
