// Neon Frontier's sky: a synthwave dusk dome. Indigo zenith through violet and magenta to a hot orange band on the
// horizon, a huge retro sun with bars cut out of its lower half sinking behind the hills, thin plum cloud streaks lit
// pink along their edges, and stars. Colours are given as they should look on screen and inverse tone mapped here,
// the way the engine's own sky does it; the sun and its halo are scene-referred (bright enough to bloom).
import * as THREE from 'three';
import { NOISE, FOG } from '../../engine/glsl.js';
import { inverseToneMap } from '../../engine/tonecurve.js';

const VS = /* glsl */`
varying vec3 vDir;
void main() {
  vDir = normalize((modelMatrix * vec4(position, 0.0)).xyz);
  vec4 p = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
  gl_Position = p.xyww;
}`;

const FS = /* glsl */`
varying vec3 vDir;
uniform float uSlTime;
uniform vec3 uZen, uHigh, uMid, uLow, uHor, uGround, uBackHor;
uniform vec3 uSunTop, uSunMid, uSunBot, uHalo, uCloudDark, uCloudLit;
uniform vec3 uRSunDir;
uniform float uRSunR, uStripes, uStars, uHorizonFog;
${NOISE}
${FOG}

void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  vec3 sd = uRSunDir;
  // how much this direction faces the sun's side of the sky (0 behind, 1 toward)
  vec2 dh = normalize(d.xz + 1e-5), sh = normalize(sd.xz + 1e-5);
  float toward = dot(dh, sh) * 0.5 + 0.5;
  float towardK = pow(toward, 2.2);

  // the gradient: hot band on the horizon, magenta, violet, indigo; the far side of the sky stays deeper
  float up = max(h, 0.0);
  vec3 hor = mix(uBackHor, uHor, towardK);
  vec3 col = mix(hor, uLow, smoothstep(0.0, 0.10 + 0.06 * towardK, up));
  col = mix(col, uMid, smoothstep(0.08, 0.32, up));
  col = mix(col, uHigh, smoothstep(0.28, 0.62, up));
  col = mix(col, uZen, smoothstep(0.58, 1.0, up));
  // below the horizon: the dark land colour (the terrain covers most of it)
  col = mix(col, uGround, smoothstep(0.0, -0.06, h));
  // the far haze meets the fog, so the last hills melt into the sky (the fog's own colours, already scene-referred)
  vec3 fogC = slFogColorFor(d);
  col = mix(col, fogC, uHorizonFog * (1.0 - smoothstep(-0.02, 0.06, h)));

  // stars, fading out toward the horizon and the sun
  if (uStars > 0.0 && h > 0.05) {
    vec3 sp = d * 260.0;
    vec3 cell = floor(sp);
    float hs = slHash13(cell);
    if (hs > 0.982) {
      vec3 c = cell + vec3(slHash13(cell + 1.7), slHash13(cell + 3.1), slHash13(cell + 5.3));
      float dist = length(sp - c);
      float tw = 0.65 + 0.35 * sin(uSlTime * (1.5 + hs * 4.0) + hs * 70.0);
      float s = smoothstep(0.11, 0.0, dist) * (hs - 0.982) / 0.018 * tw;
      vec3 sc = mix(vec3(0.75, 0.85, 1.0), vec3(1.0, 0.75, 0.95), slHash13(cell + 9.1));
      col += sc * s * uStars * 2.2 * smoothstep(0.05, 0.35, h) * (1.0 - 0.8 * towardK * (1.0 - smoothstep(0.1, 0.5, h)));
    }
  }

  // the retro sun: a disc in the sun's tangent plane, gradient yellow to hot pink, bars cut into its lower half
  float cb = dot(d, sd);
  float sunMask = 0.0;
  if (cb > 0.0) {
    vec3 e = normalize(cross(vec3(0.0, 1.0, 0.0), sd));
    vec3 n = cross(sd, e);
    vec2 q = vec2(dot(d, e), dot(d, n)) / cb / tan(uRSunR);
    float r = length(q);
    float aa = fwidth(r) * 1.25 + 1e-4;
    float disc = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, r);
    float y = q.y * 0.5 + 0.5;                  // 0 at the foot, 1 at the top
    // bars: thin at the middle, thicker toward the foot
    float bars = 1.0;
    if (y < 0.56) {
      float k = y / 0.56;
      float band = fract(k * uStripes + 0.15);
      float gap = mix(0.62, 0.08, k);
      float aaB = fwidth(k * uStripes) * 1.2 + 1e-4;
      bars = smoothstep(gap - aaB, gap + aaB, band);
    }
    vec3 sc = mix(uSunBot, uSunMid, smoothstep(0.05, 0.55, y));
    sc = mix(sc, uSunTop, smoothstep(0.5, 0.98, y));
    // a hotter core so it blooms from the middle out
    sc *= 1.0 + 0.25 * exp(-r * r * 2.5);
    sunMask = disc * bars;
    // the halo: wide and soft, magenta to orange
    float halo = exp(-max(r - 1.0, 0.0) * 1.6) * (1.0 - disc * bars);
    float halo2 = exp(-max(r - 1.0, 0.0) * 0.45);
    col += uHalo * (halo * 0.55 + halo2 * 0.22);
    col = mix(col, sc, sunMask);
  }

  // thin cloud streaks low over the horizon, plum silhouettes with hot pink edges toward the sun
  if (h > -0.01 && h < 0.3) {
    float az = atan(d.x, d.z);
    vec2 p = vec2(az * 3.2, h * 46.0);
    float n1 = slFbm(vec2(p.x * 1.0 + 3.0, p.y * 0.55 + uSlTime * 0.004));
    float n2 = slFbm(vec2(p.x * 2.3 - 1.0, p.y * 1.1));
    float band = smoothstep(0.02, 0.07, h) * (1.0 - smoothstep(0.12, 0.26, h));
    float dens = smoothstep(0.56, 0.7, n1 * 0.75 + n2 * 0.35) * band;
    // the lit edge: where the density falls off on the side toward the sun's centre
    float edge = smoothstep(0.5, 0.6, n1 * 0.75 + n2 * 0.35) - smoothstep(0.6, 0.72, n1 * 0.75 + n2 * 0.35);
    vec3 cc = mix(uCloudDark, uCloudLit, clamp(edge * 1.5, 0.0, 1.0) * (0.25 + 0.75 * towardK));
    cc += uHalo * pow(max(cb, 0.0), 40.0) * 0.6;
    col = mix(col, cc, dens * 0.92);
  }
  gl_FragColor = vec4(col, 1.0);
}`;

// opts: { exposure, toneMapping, sunAzimuth, sunElevation, sunSize (deg), stripes, colours as '#hex' }
export function neonSky(kit, o) {
  const op = o.toneMapping || 'aces', ex = o.exposure ?? 1;
  const inv = (hex) => inverseToneMap(new THREE.Color(hex), op, ex);
  // the sun's colours are what its face should show; k > 1 pushes them past the bloom threshold
  const hdr = (hex, k) => inv(hex).multiplyScalar(k);
  const DEG = Math.PI / 180;
  const a = o.sunAzimuth * DEG, p = o.sunElevation * DEG;
  const dir = new THREE.Vector3(-Math.sin(a) * Math.cos(p), Math.sin(p), Math.cos(a) * Math.cos(p));
  const uniforms = Object.assign({
    uZen: { value: inv(o.zenith) }, uHigh: { value: inv(o.high) }, uMid: { value: inv(o.mid) }, uLow: { value: inv(o.low) },
    uHor: { value: inv(o.horizon) }, uBackHor: { value: inv(o.backHorizon) }, uGround: { value: inv(o.ground) },
    uSunTop: { value: hdr(o.sunTop, o.sunBright) }, uSunMid: { value: hdr(o.sunMid, o.sunBright) },
    uSunBot: { value: hdr(o.sunBottom, o.sunBright) }, uHalo: { value: hdr(o.halo, o.haloBright) },
    uCloudDark: { value: inv(o.cloudDark) }, uCloudLit: { value: hdr(o.cloudLit, o.cloudBright) },
    uRSunDir: { value: dir }, uRSunR: { value: o.sunSize * 0.5 * DEG }, uStripes: { value: o.stripes },
    uStars: { value: o.stars }, uHorizonFog: { value: o.horizonFog },
  }, kit.shared);
  return new THREE.ShaderMaterial({
    uniforms, vertexShader: VS, fragmentShader: FS, side: THREE.BackSide, depthWrite: false, depthTest: true,
    toneMapped: false, fog: false,
  });
}
