// The frame after the scene: an HDR target (half float, 4x MSAA, depth texture), the passes a style lists, tone mapping
// and the output. Scene-referred passes (ao, bloom, halation, godrays, dof, custom with stage 'hdr') run first in the
// listed order, then tone mapping (env.toneMapping, env.exposure), then the display-referred passes in the listed order.
import * as THREE from 'three';
import { FullScreenQuad } from 'three/addons/postprocessing/Pass.js';
import { NOISE, TONEMAP } from './glsl.js';
import { srgbVec, linColor } from './util.js';

const VS = /* glsl */`varying vec2 vUv; void main() { vUv = uv; gl_Position = vec4(position.xy, 0.0, 1.0); }`;

const HEADER = /* glsl */`
precision highp float;
varying vec2 vUv;
uniform sampler2D tColor;
uniform sampler2D tDepth;
uniform sampler2D tNormal;
uniform vec2 uResolution;
uniform float uTime;
uniform vec3 uSunScreen;
uniform float uNear, uFar;
uniform mat4 uProj, uProjInv;
uniform float uDither;
${NOISE}
${TONEMAP}
float rawDepth(vec2 uv) { return texture2D(tDepth, uv).x; }
float linearDepth(vec2 uv) {
  float z = rawDepth(uv) * 2.0 - 1.0;
  return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}
vec3 viewPos(vec2 uv) { vec4 p = uProjInv * vec4(uv * 2.0 - 1.0, rawDepth(uv) * 2.0 - 1.0, 1.0); return p.xyz / p.w; }
vec3 viewNormal(vec2 uv) { return normalize(texture2D(tNormal, uv).xyz * 2.0 - 1.0); }
`;

function wrap(body) {
  return HEADER + body + /* glsl */`
void main() {
  vec4 c = effect(vUv);
  if (uDither > 0.5) c.rgb += (slIGN(gl_FragCoord.xy + fract(uTime * 7.13) * 0.0) - 0.5) / 255.0;
  gl_FragColor = c;
}`;
}

const hexOrNum = (v, def) => {
  if (v === undefined || v === null) return new THREE.Vector3(def, def, def);
  if (typeof v === 'number') return new THREE.Vector3(v, v, v);
  if (Array.isArray(v)) return new THREE.Vector3(v[0], v[1], v[2]);
  return srgbVec(v);
};

export class Post {
  constructor(renderer) {
    this.renderer = renderer;
    this.quad = new FullScreenQuad(null);
    this.common = {
      uResolution: { value: new THREE.Vector2(1, 1) }, uTime: { value: 0 }, uSunScreen: { value: new THREE.Vector3(0.5, 0.5, 0) },
      uNear: { value: 0.05 }, uFar: { value: 1000 }, uProj: { value: new THREE.Matrix4() }, uProjInv: { value: new THREE.Matrix4() },
    };
    this.passes = [];
    this.exposure = 1;
    this.toneMapping = 'aces';
    this.samples = 4;
    this.size = new THREE.Vector2(2, 2);
    this.targets = new Map();
    this.tonemapPass = this._toneMapPass();
    this.copyMat = this._mat(wrap('vec4 effect(vec2 uv) { return texture2D(tColor, uv); }'), {});
    this._build(2, 2);
  }

  _mat(fs, uniforms) {
    return new THREE.ShaderMaterial({
      uniforms: Object.assign({ tColor: { value: null }, tDepth: { value: null }, tNormal: { value: null }, uDither: { value: 0 } }, this.common, uniforms),
      vertexShader: VS, fragmentShader: fs, depthTest: false, depthWrite: false, toneMapped: false,
    });
  }

  _build(w, h) {
    if (this.sceneRT) { this.sceneRT.depthTexture.dispose(); this.sceneRT.dispose(); }
    const depthTexture = new THREE.DepthTexture(w, h, THREE.FloatType);
    depthTexture.format = THREE.DepthFormat;
    this.sceneRT = new THREE.WebGLRenderTarget(w, h, { type: THREE.HalfFloatType, samples: this.samples, depthTexture, depthBuffer: true });
    this.sceneRT.texture.name = 'scene';
    for (const t of this.targets.values()) t.dispose();
    this.targets.clear();
    if (this.normalRT) { this.normalRT.dispose(); this.normalRT = null; }
  }

  setSize(w, h) {
    w = Math.max(2, Math.floor(w)); h = Math.max(2, Math.floor(h));
    if (w === this.size.x && h === this.size.y) return;
    this.size.set(w, h);
    this._build(w, h);
    this.common.uResolution.value.set(w, h);
  }

  // A cached intermediate target: key, size divisor, format.
  target(key, div = 1, type = THREE.HalfFloatType) {
    const w = Math.max(1, Math.round(this.size.x / div)), h = Math.max(1, Math.round(this.size.y / div));
    let t = this.targets.get(key);
    if (t && (t.width !== w || t.height !== h)) { t.dispose(); t = null; }
    if (!t) {
      t = new THREE.WebGLRenderTarget(w, h, { type, depthBuffer: false, minFilter: THREE.LinearFilter, magFilter: THREE.LinearFilter });
      t.texture.generateMipmaps = false;
      this.targets.set(key, t);
    }
    return t;
  }

  normalTarget() {
    if (!this.normalRT) {
      this.normalRT = new THREE.WebGLRenderTarget(this.size.x, this.size.y, { type: THREE.UnsignedByteType, depthBuffer: true,
        minFilter: THREE.NearestFilter, magFilter: THREE.NearestFilter });
    }
    return this.normalRT;
  }

  // Draws a material into a target (null = the screen).
  draw(mat, target, input) {
    const r = this.renderer;
    if (input !== undefined) mat.uniforms.tColor.value = input;
    mat.uniforms.tDepth.value = this.sceneRT.depthTexture;
    mat.uniforms.tNormal.value = this.normalRT ? this.normalRT.texture : null;
    r.setRenderTarget(target);
    this.quad.material = mat;
    this.quad.render(r);
  }

  setPasses(list) {
    this.passes = (list || []).filter(Boolean).map((p) => (p.isPass ? p : null)).filter(Boolean);
    const grade = this.passes.find((p) => p.kind === 'grade');
    this.gradeExposure = grade ? grade : null;
  }

  needsNormal() { return this.passes.some((p) => p.enabled !== false && p.needsNormal); }

  // drawScene(target) renders the world (and the gun) into the HDR target; drawNormals(target) the normal pass.
  render(camera, drawScene, drawNormals) {
    const c = this.common;
    c.uNear.value = camera.near; c.uFar.value = camera.far;
    c.uProj.value.copy(camera.projectionMatrix); c.uProjInv.value.copy(camera.projectionMatrixInverse);
    drawScene(this.sceneRT);
    if (this.needsNormal()) drawNormals(this.normalTarget());
    const active = this.passes.filter((p) => p.enabled !== false);
    const hdr = active.filter((p) => p.stage === 'hdr'), ldr = active.filter((p) => p.stage !== 'hdr');
    let src = this.sceneRT.texture;
    let flip = 0;
    const next = () => this.target('pp' + (flip++ % 2));
    for (const p of hdr) { const dst = next(); p.render(this, src, dst); src = dst.texture; }
    // tone mapping
    const tm = this.tonemapPass;
    const gradeExp = this.gradeExposure ? (this.gradeExposure.params.exposure || 0) : 0;
    tm.uniforms.uExposure.value = this.exposure * Math.pow(2, gradeExp);
    tm.uniforms.uOp.value = { aces: 1, agx: 2, neutral: 3, none: 0, linear: 0 }[this.toneMapping] ?? 1;
    if (!ldr.length) { tm.uniforms.uDither.value = 1; this.draw(tm, null, src); return; }
    tm.uniforms.uDither.value = 0;
    let dst = next(); this.draw(tm, dst, src); src = dst.texture;
    ldr.forEach((p, i) => {
      const last = i === ldr.length - 1;
      const out = last ? null : next();
      p.render(this, src, out, last);
      if (!last) src = out.texture;
    });
  }

  _toneMapPass() {
    return this._mat(wrap(/* glsl */`
      uniform float uExposure; uniform int uOp;
      vec4 effect(vec2 uv) {
        vec3 c = max(texture2D(tColor, uv).rgb, 0.0) * uExposure;
        if (uOp == 1) c = slACES(c); else if (uOp == 2) c = slAgX(c); else if (uOp == 3) c = slNeutral(c);
        return vec4(slToSRGB(c), 1.0);
      }`), { uExposure: { value: 1 }, uOp: { value: 1 } });
  }
}

// ------------------------------------------------------------------------------------------------ the passes
function single(post, kind, stage, body, uniforms, params, bind, flags = {}) {
  const mat = post._mat(wrap(body), uniforms);
  const pass = {
    isPass: true, kind, stage, params, uniforms: mat.uniforms, material: mat, enabled: true,
    needsDepth: !!flags.needsDepth, needsNormal: !!flags.needsNormal,
    render(pp, src, dst, last) {
      if (bind) bind(pass.params, mat.uniforms);
      mat.uniforms.uDither.value = last ? 1 : 0;
      pp.draw(mat, dst, src);
    },
  };
  return pass;
}

export function makePostKit(post) {
  const P = {};

  // ---------------- ambient occlusion (GTAO, half resolution, depth-aware blur and upsample)
  P.ao = (o = {}) => {
    const params = { radius: o.radius ?? 1.4, intensity: o.intensity ?? 1.0, power: o.power ?? 1.6, halfRes: o.halfRes ?? true,
      color: o.color || '#000000', maxPixels: o.maxPixels ?? 90 };
    const gtao = post._mat(wrap(/* glsl */`
      uniform float uRadius, uMaxPx, uPower, uDepthErr;
      uniform vec2 uFullRes;
      vec3 vpos(vec2 uv) { return viewPos(uv); }
      // normals from depth over a baseline that widens with distance: far away the depth buffer's steps would
      // otherwise tilt the normals in terraces, which the AO draws as bands across distant ground
      vec3 nrm(vec2 uv, vec3 P) {
        vec2 px = clamp(-P.z / 50.0, 1.0, 6.0) / uFullRes;
        vec3 r = vpos(uv + vec2(px.x, 0.0)) - P, l = P - vpos(uv - vec2(px.x, 0.0));
        vec3 t = vpos(uv + vec2(0.0, px.y)) - P, b = P - vpos(uv - vec2(0.0, px.y));
        vec3 dx = abs(r.z) < abs(l.z) ? r : l; vec3 dy = abs(t.z) < abs(b.z) ? t : b;
        return normalize(cross(dx, dy));
      }
      vec4 effect(vec2 uv) {
        float d = rawDepth(uv);
        if (d >= 0.99999 || d < 0.0011) return vec4(1.0, 1e6, 0.0, 1.0);   // sky, or the gun (its depth is squeezed)
        vec3 P = vpos(uv);
        vec3 N = nrm(uv, P);
        vec3 V = normalize(-P);
        float projScale = uProj[1][1] * 0.5 * uFullRes.y;
        float rpx = min(uRadius * projScale / -P.z, uMaxPx);
        if (rpx < 1.5) return vec4(1.0, -P.z, 0.0, 1.0);
        float n1 = slIGN(gl_FragCoord.xy);
        float n2 = fract(n1 * 1.6180339 + slHash12(gl_FragCoord.xy) * 0.5);
        float fr = 0.615 * uRadius, ff = uRadius * (1.0 - 0.615);
        float fMul = -1.0 / fr, fAdd = ff / fr + 1.0;
        const int SL = 3; const int ST = 6;
        float vis = 0.0;
        for (int s = 0; s < SL; s++) {
          float phi = (float(s) + n1) * 3.14159265 / float(SL);
          vec2 om = vec2(cos(phi), sin(phi));
          vec3 dv = vec3(om, 0.0);
          vec3 od = dv - dot(dv, V) * V;
          vec3 ax = normalize(cross(od, V));
          vec3 pn = N - ax * dot(N, ax);
          float pl = length(pn);
          float cn = clamp(dot(pn, V) / max(pl, 1e-4), -1.0, 1.0);
          float n = sign(dot(od, pn)) * acos(cn);
          float low0 = cos(n + 1.5707963), low1 = cos(n - 1.5707963);
          float h0c = low0, h1c = low1;
          for (int j = 0; j < ST; j++) {
            float st = (float(j) + n2) / float(ST);
            st = st * st;
            vec2 off = om * (st * rpx + 1.0) / uFullRes;
            vec3 s0 = vpos(uv + off) - P, s1 = vpos(uv - off) - P;
            float l0 = length(s0), l1 = length(s1);
            // the depth buffer's step at this distance, kept out of the horizons
            float c0 = dot(s0 / l0, V) - uDepthErr * P.z * P.z / l0, c1 = dot(s1 / l1, V) - uDepthErr * P.z * P.z / l1;
            c0 = mix(low0, c0, clamp(l0 * fMul + fAdd, 0.0, 1.0));
            c1 = mix(low1, c1, clamp(l1 * fMul + fAdd, 0.0, 1.0));
            h0c = max(h0c, c0); h1c = max(h1c, c1);
          }
          float h0 = -acos(clamp(h1c, -1.0, 1.0)), h1 = acos(clamp(h0c, -1.0, 1.0));
          h0 = n + clamp(h0 - n, -1.5707963, 1.5707963);
          h1 = n + clamp(h1 - n, -1.5707963, 1.5707963);
          float sn = sin(n);
          float a0 = (cn + 2.0 * h0 * sn - cos(2.0 * h0 - n)) * 0.25;
          float a1 = (cn + 2.0 * h1 * sn - cos(2.0 * h1 - n)) * 0.25;
          vis += pl * (a0 + a1);
        }
        vis = clamp(vis / float(SL), 0.0, 1.0);
        return vec4(pow(vis, uPower), -P.z, 0.0, 1.0);
      }`), { uRadius: { value: 1 }, uMaxPx: { value: 90 }, uPower: { value: 1.6 }, uFullRes: { value: new THREE.Vector2() }, uDepthErr: { value: 0 } });
    const blur = post._mat(wrap(/* glsl */`
      uniform vec2 uTexel;
      vec4 effect(vec2 uv) {
        vec4 c0 = texture2D(tColor, uv);
        float z = c0.g; float sum = 0.0, w = 0.0;
        for (int y = -2; y <= 1; y++) for (int x = -2; x <= 1; x++) {
          vec4 s = texture2D(tColor, uv + (vec2(x, y) + 0.5) * uTexel);
          float k = max(0.0, 1.0 - abs(s.g - z) / max(0.06 * z, 0.05));
          sum += s.r * k; w += k;
        }
        return vec4(w > 0.0 ? sum / w : c0.r, z, 0.0, 1.0);
      }`), { uTexel: { value: new THREE.Vector2() } });
    const comp = post._mat(wrap(/* glsl */`
      uniform sampler2D tAO; uniform vec2 uAOTexel; uniform float uIntensity; uniform vec3 uAOColor;
      vec4 effect(vec2 uv) {
        vec4 c = texture2D(tColor, uv);
        float z = linearDepth(uv);
        vec2 base = uv - 0.5 * uAOTexel;
        float sum = 0.0, w = 0.0;
        for (int y = 0; y <= 1; y++) for (int x = 0; x <= 1; x++) {
          vec4 s = texture2D(tAO, base + vec2(x, y) * uAOTexel);
          float k = 1.0 / (0.002 + abs(s.g - z) / max(z, 0.1));
          sum += s.r * k; w += k;
        }
        float ao = w > 0.0 ? sum / w : 1.0;
        ao = mix(1.0, ao, uIntensity);
        c.rgb = c.rgb * mix(uAOColor, vec3(1.0), ao);
        return c;
      }`), { tAO: { value: null }, uAOTexel: { value: new THREE.Vector2() }, uIntensity: { value: 1 }, uAOColor: { value: new THREE.Color(0, 0, 0) } });
    const pass = {
      isPass: true, kind: 'ao', stage: 'hdr', needsDepth: true, needsNormal: false, enabled: true, params,
      uniforms: comp.uniforms, materials: { gtao, blur, comp },
      render(pp, src, dst) {
        const div = params.halfRes ? 2 : 1;
        const a = pp.target('ao_a', div), b = pp.target('ao_b', div);
        gtao.uniforms.uRadius.value = params.radius; gtao.uniforms.uMaxPx.value = params.maxPixels;
        gtao.uniforms.uPower.value = params.power; gtao.uniforms.uFullRes.value.copy(pp.size);
        // a 24-bit depth step at distance z is about z^2 / (near * 2^24); three steps' worth is ignored
        gtao.uniforms.uDepthErr.value = 3.0 / (pp.common.uNear.value * 16777216);
        pp.draw(gtao, a, src);
        blur.uniforms.uTexel.value.set(1 / a.width, 1 / a.height);
        pp.draw(blur, b, a.texture);
        comp.uniforms.tAO.value = b.texture; comp.uniforms.uAOTexel.value.set(1 / b.width, 1 / b.height);
        comp.uniforms.uIntensity.value = params.intensity;
        comp.uniforms.uAOColor.value.copy(linColor(params.color));
        pp.draw(comp, dst, src);
      },
    };
    return pass;
  };

  // ---------------- bloom (and halation: the same mip chain, one tinted wide glow)
  const DOWN = /* glsl */`
    uniform vec2 uTexel; uniform float uThreshold, uKnee; uniform int uFirst;
    vec3 karis(vec3 c) { return c / (1.0 + slLuma(c)); }
    vec3 pre(vec3 c) {
      float br = max(c.r, max(c.g, c.b));
      float rq = clamp(br - uThreshold + uKnee, 0.0, 2.0 * uKnee);
      rq = rq * rq / (4.0 * uKnee + 1e-4);
      return c * max(rq, br - uThreshold) / max(br, 1e-4);
    }
    vec4 effect(vec2 uv) {
      vec2 t = uTexel;
      vec3 a = texture2D(tColor, uv + t * vec2(-2, 2)).rgb, b = texture2D(tColor, uv + t * vec2(0, 2)).rgb, c = texture2D(tColor, uv + t * vec2(2, 2)).rgb;
      vec3 d = texture2D(tColor, uv + t * vec2(-2, 0)).rgb, e = texture2D(tColor, uv).rgb, f = texture2D(tColor, uv + t * vec2(2, 0)).rgb;
      vec3 g = texture2D(tColor, uv + t * vec2(-2, -2)).rgb, h = texture2D(tColor, uv + t * vec2(0, -2)).rgb, i = texture2D(tColor, uv + t * vec2(2, -2)).rgb;
      vec3 j = texture2D(tColor, uv + t * vec2(-1, 1)).rgb, k = texture2D(tColor, uv + t * vec2(1, 1)).rgb;
      vec3 l = texture2D(tColor, uv + t * vec2(-1, -1)).rgb, m = texture2D(tColor, uv + t * vec2(1, -1)).rgb;
      vec3 o;
      if (uFirst == 1) {
        vec3 g0 = karis((a + b + d + e) * 0.25), g1 = karis((b + c + e + f) * 0.25), g2 = karis((d + e + g + h) * 0.25), g3 = karis((e + f + h + i) * 0.25), g4 = karis((j + k + l + m) * 0.25);
        o = g4 * 0.5 + (g0 + g1 + g2 + g3) * 0.125;
        o = o / max(1.0 - slLuma(o), 1e-3);
        o = pre(min(o, vec3(200.0)));
      } else {
        o = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
      }
      return vec4(o, 1.0);
    }`;
  const UP = /* glsl */`
    uniform sampler2D tLow; uniform vec2 uTexel; uniform float uRadius;
    vec4 effect(vec2 uv) {
      vec2 t = uTexel * 1.0;
      vec3 s = texture2D(tLow, uv + t * vec2(-1, 1)).rgb + 2.0 * texture2D(tLow, uv + t * vec2(0, 1)).rgb + texture2D(tLow, uv + t * vec2(1, 1)).rgb
        + 2.0 * texture2D(tLow, uv + t * vec2(-1, 0)).rgb + 4.0 * texture2D(tLow, uv).rgb + 2.0 * texture2D(tLow, uv + t * vec2(1, 0)).rgb
        + texture2D(tLow, uv + t * vec2(-1, -1)).rgb + 2.0 * texture2D(tLow, uv + t * vec2(0, -1)).rgb + texture2D(tLow, uv + t * vec2(1, -1)).rgb;
      return vec4(texture2D(tColor, uv).rgb + s / 16.0 * uRadius, 1.0);
    }`;
  function glowChain(kind, params, levels, composite) {
    const down = post._mat(wrap(DOWN), { uTexel: { value: new THREE.Vector2() }, uThreshold: { value: 1 }, uKnee: { value: 0.5 }, uFirst: { value: 0 } });
    const up = post._mat(wrap(UP), { tLow: { value: null }, uTexel: { value: new THREE.Vector2() }, uRadius: { value: 0.7 } });
    const comp = post._mat(wrap(composite), { tGlow: { value: null }, uStrength: { value: 0.4 }, uTint: { value: new THREE.Color(1, 1, 1) } });
    return {
      isPass: true, kind, stage: 'hdr', needsDepth: false, needsNormal: false, enabled: true, params, uniforms: comp.uniforms,
      render(pp, src, dst) {
        const n = typeof levels === 'function' ? levels() : levels;
        const mips = [];
        let input = src, w = pp.size.x, h = pp.size.y;
        for (let i = 0; i < n; i++) {
          const t = pp.target(kind + 'd' + i, Math.pow(2, i + 1));
          down.uniforms.uTexel.value.set(1 / w, 1 / h);
          down.uniforms.uFirst.value = i === 0 ? 1 : 0;
          down.uniforms.uThreshold.value = params.threshold;
          down.uniforms.uKnee.value = Math.max(0.05, params.threshold * 0.5);
          pp.draw(down, t, input);
          mips.push(t); input = t.texture; w = t.width; h = t.height;
        }
        let low = mips[n - 1];
        for (let i = n - 2; i >= 0; i--) {
          const t = pp.target(kind + 'u' + i, Math.pow(2, i + 1));
          up.uniforms.tLow.value = low.texture;
          up.uniforms.uTexel.value.set(1 / low.width, 1 / low.height);
          up.uniforms.uRadius.value = params.radius;
          pp.draw(up, t, mips[i].texture);
          low = t;
        }
        comp.uniforms.tGlow.value = low.texture;
        comp.uniforms.uStrength.value = params.strength;
        comp.uniforms.uTint.value.copy(linColor(params.color || params.tint || '#ffffff'));
        pp.draw(comp, dst, src);
      },
    };
  }
  P.bloom = (o = {}) => {
    const params = { threshold: o.threshold ?? 1.0, strength: o.strength ?? 0.35, radius: o.radius ?? 0.75, tint: o.tint || '#ffffff' };
    return glowChain('bloom', params, 6, /* glsl */`
      uniform sampler2D tGlow; uniform float uStrength; uniform vec3 uTint;
      vec4 effect(vec2 uv) { vec4 c = texture2D(tColor, uv); c.rgb += texture2D(tGlow, uv).rgb * uStrength * uTint / 6.0; return c; }`);
  };
  P.halation = (o = {}) => {
    const params = { threshold: o.threshold ?? 0.8, radius: o.radius ?? 0.9, color: o.color || '#ff5a2a', strength: o.strength ?? 0.35 };
    return glowChain('halation', params, () => Math.max(2, Math.min(6, Math.round(2 + params.radius * 3))), /* glsl */`
      uniform sampler2D tGlow; uniform float uStrength; uniform vec3 uTint;
      vec4 effect(vec2 uv) {
        vec4 c = texture2D(tColor, uv);
        vec3 g = texture2D(tGlow, uv).rgb;
        c.rgb += slLuma(g) * uTint * uStrength / 4.0;
        return c;
      }`);
  };

  // ---------------- god rays (sky behind the sun, blurred toward it)
  P.godrays = (o = {}) => {
    // threshold: only what is brighter than this (scene-referred luminance, before exposure) shines, with a soft knee;
    // the old default of 0 let a whole bright sky shine and washed it white. depthFade (metres, 0 = off): geometry
    // farther than this joins the sky as a source (hazy distance glowing toward the sun). spread: how far round the
    // sun on screen the sources reach (6; smaller is wider).
    const params = { strength: o.strength ?? 0.5, decay: o.decay ?? 0.965, density: o.density ?? 0.85, color: o.color || '#ffd9a0',
      threshold: o.threshold ?? 1.2, samples: o.samples ?? 48, depthFade: o.depthFade ?? 0, spread: o.spread ?? 6 };
    const mask = post._mat(wrap(/* glsl */`
      uniform float uThreshold, uDepthFade, uSpread;
      vec4 effect(vec2 uv) {
        float raw = rawDepth(uv);
        float src = step(0.99999, raw);
        if (uDepthFade > 0.0 && src < 0.5 && raw > 0.0011) src = smoothstep(uDepthFade, uDepthFade * 2.0, linearDepth(uv));
        vec3 c = texture2D(tColor, uv).rgb;
        vec2 d = (uv - uSunScreen.xy) * vec2(uResolution.x / uResolution.y, 1.0);
        float near = exp(-dot(d, d) * uSpread);
        float l = slLuma(c);
        float knee = max(uThreshold * 0.35, 0.05);
        float b = l - uThreshold + knee;
        b = b <= 0.0 ? 0.0 : (b < 2.0 * knee ? b * b / (4.0 * knee) : l - uThreshold);
        return vec4(c * src * near * min(b, 8.0) / max(l, 1e-3), 1.0);
      }`), { uThreshold: { value: 1.2 }, uDepthFade: { value: 0 }, uSpread: { value: 6 } });
    const blur = post._mat(wrap(/* glsl */`
      uniform float uDecay, uDensity; uniform int uSamples;
      vec4 effect(vec2 uv) {
        vec2 dir = (uv - uSunScreen.xy) * uDensity / float(uSamples);
        vec2 p = uv; vec3 acc = vec3(0.0); float w = 1.0, tot = 0.0;
        float j = slIGN(gl_FragCoord.xy);
        p -= dir * j;
        for (int i = 0; i < 96; i++) { if (i >= uSamples) break; acc += texture2D(tColor, p).rgb * w; tot += w; w *= uDecay; p -= dir; }
        return vec4(acc / max(tot, 1e-3), 1.0);
      }`), { uDecay: { value: 0.96 }, uDensity: { value: 0.85 }, uSamples: { value: 48 } });
    const comp = post._mat(wrap(/* glsl */`
      uniform sampler2D tRays; uniform float uStrength; uniform vec3 uRayColor;
      vec4 effect(vec2 uv) {
        vec4 c = texture2D(tColor, uv);
        float vis = uSunScreen.z * (1.0 - smoothstep(0.6, 1.3, length(uSunScreen.xy - 0.5) * 2.0));
        c.rgb += texture2D(tRays, uv).rgb * uRayColor * uStrength * vis * 4.0;
        return c;
      }`), { tRays: { value: null }, uStrength: { value: 0.5 }, uRayColor: { value: new THREE.Color() } });
    return {
      isPass: true, kind: 'godrays', stage: 'hdr', needsDepth: true, needsNormal: false, enabled: true, params, uniforms: comp.uniforms,
      render(pp, src, dst) {
        const a = pp.target('gr_a', 2), b = pp.target('gr_b', 2);
        mask.uniforms.uThreshold.value = params.threshold;
        mask.uniforms.uDepthFade.value = params.depthFade; mask.uniforms.uSpread.value = params.spread;
        pp.draw(mask, a, src);
        blur.uniforms.uDecay.value = params.decay; blur.uniforms.uDensity.value = params.density; blur.uniforms.uSamples.value = params.samples;
        pp.draw(blur, b, a.texture);
        comp.uniforms.tRays.value = b.texture; comp.uniforms.uStrength.value = params.strength;
        comp.uniforms.uRayColor.value.copy(linColor(params.color));
        pp.draw(comp, dst, src);
      },
    };
  };

  // ---------------- depth of field (by linear depth; the near gun blurs by nearBlur)
  P.dof = (o = {}) => {
    const params = { focus: o.focus ?? 12, range: o.range ?? 8, blur: o.blur ?? 6, nearBlur: o.nearBlur ?? 0.4, farOnly: o.farOnly ?? false };
    const coc = post._mat(wrap(/* glsl */`
      uniform float uFocus, uRange, uBlur, uNearBlur, uFarOnly;
      vec4 effect(vec2 uv) {
        float z = linearDepth(uv);
        float dz = abs(z - uFocus) - uRange * 0.5;
        float c = clamp(dz / max(uFocus * 0.8, 1.0), 0.0, 1.0) * uBlur;
        if (z < uFocus) c *= (z < 1.2) ? uNearBlur : 0.6 * (1.0 - uFarOnly);
        return vec4(texture2D(tColor, uv).rgb, c);
      }`), { uFocus: { value: 12 }, uRange: { value: 8 }, uBlur: { value: 6 }, uNearBlur: { value: 0.4 }, uFarOnly: { value: 0 } });
    const gather = post._mat(wrap(/* glsl */`
      uniform vec2 uTexel;
      vec4 effect(vec2 uv) {
        vec4 c0 = texture2D(tColor, uv);
        float r0 = c0.a;
        if (r0 < 0.5) return vec4(c0.rgb, 1.0);
        vec3 acc = c0.rgb; float w = 1.0;
        float ga = 2.39996323;
        for (int i = 1; i < 24; i++) {
          float rr = sqrt(float(i) / 24.0) * r0;
          vec2 off = vec2(cos(float(i) * ga), sin(float(i) * ga)) * rr * uTexel;
          vec4 s = texture2D(tColor, uv + off);
          float k = smoothstep(rr - 1.0, rr + 1.0, s.a + 0.5);
          acc += s.rgb * k; w += k;
        }
        return vec4(acc / w, 1.0);
      }`), { uTexel: { value: new THREE.Vector2() } });
    return {
      isPass: true, kind: 'dof', stage: 'hdr', needsDepth: true, needsNormal: false, enabled: true, params, uniforms: gather.uniforms,
      render(pp, src, dst) {
        const a = pp.target('dof_a', 1);
        coc.uniforms.uFocus.value = params.focus; coc.uniforms.uRange.value = params.range;
        coc.uniforms.uBlur.value = params.blur * pp.size.y / 1080; coc.uniforms.uNearBlur.value = params.nearBlur;
        coc.uniforms.uFarOnly.value = params.farOnly ? 1 : 0;
        pp.draw(coc, a, src);
        gather.uniforms.uTexel.value.set(1 / pp.size.x, 1 / pp.size.y);
        pp.draw(gather, dst, a.texture);
      },
    };
  };

  // ---------------- outlines (inverse-depth second difference: zero on any plane, so flat ground draws no lines)
  P.outline = (o = {}) => {
    const params = { width: o.width ?? 1, color: o.color || '#1a1410', colorFromScene: o.colorFromScene ?? 0,
      depthEdge: o.depthEdge ?? 1, normalEdge: o.normalEdge ?? 1, fadeNear: o.fadeNear ?? 60, fadeFar: o.fadeFar ?? 400,
      innerLines: o.innerLines ?? 1, opacity: o.opacity ?? 1 };
    const needsNormal = (params.normalEdge > 0 && params.innerLines > 0);
    const pass = single(post, 'outline', 'ldr', /* glsl */`
      uniform float uWidth, uDepthEdge, uNormalEdge, uFadeNear, uFadeFar, uInner, uFromScene, uOpacity, uHasNormal;
      uniform vec3 uLineColor;
      float invz(vec2 uv) { return 1.0 / linearDepth(uv); }
      vec4 effect(vec2 uv) {
        vec3 c = texture2D(tColor, uv).rgb;
        vec2 o = uWidth / uResolution;
        float d0 = linearDepth(uv);
        float w0 = 1.0 / d0;
        float wl = invz(uv - vec2(o.x, 0.0)), wr = invz(uv + vec2(o.x, 0.0));
        float wd = invz(uv - vec2(0.0, o.y)), wu = invz(uv + vec2(0.0, o.y));
        float lap = min(wl + wr - 2.0 * w0, wd + wu - 2.0 * w0) / w0;
        float dEdge = smoothstep(0.02, 0.07, -lap) * uDepthEdge;
        // silhouettes against the sky
        float sky = step(0.99999, rawDepth(uv));
        float skyN = max(max(step(0.99999, rawDepth(uv - vec2(o.x, 0.0))), step(0.99999, rawDepth(uv + vec2(o.x, 0.0)))),
                         max(step(0.99999, rawDepth(uv - vec2(0.0, o.y))), step(0.99999, rawDepth(uv + vec2(0.0, o.y)))));
        dEdge = max(dEdge, (1.0 - sky) * skyN * uDepthEdge);
        float nEdge = 0.0;
        if (uHasNormal > 0.5 && sky < 0.5) {
          vec3 n = viewNormal(uv);
          vec3 nr = viewNormal(uv + vec2(o.x, 0.0)), nu = viewNormal(uv + vec2(0.0, o.y));
          vec3 nl = viewNormal(uv - vec2(o.x, 0.0)), nd = viewNormal(uv - vec2(0.0, o.y));
          float nd1 = max(max(1.0 - dot(n, nr), 1.0 - dot(n, nu)), max(1.0 - dot(n, nl), 1.0 - dot(n, nd)));
          nEdge = smoothstep(0.25, 0.5, nd1) * uNormalEdge * uInner;
        }
        float fade = 1.0 - smoothstep(uFadeNear, uFadeFar, d0);
        float e = clamp(max(dEdge, nEdge) * fade, 0.0, 1.0) * uOpacity;
        vec3 lc = mix(uLineColor, c * 0.25, uFromScene);
        return vec4(mix(c, lc, e), 1.0);
      }`, { uWidth: { value: 1 }, uDepthEdge: { value: 1 }, uNormalEdge: { value: 1 }, uFadeNear: { value: 60 }, uFadeFar: { value: 400 },
      uInner: { value: 1 }, uFromScene: { value: 0 }, uOpacity: { value: 1 }, uHasNormal: { value: 0 }, uLineColor: { value: new THREE.Vector3() } },
    params, (p, u) => {
      u.uWidth.value = p.width * (post.size.y / 1080); u.uDepthEdge.value = p.depthEdge; u.uNormalEdge.value = p.normalEdge;
      u.uFadeNear.value = p.fadeNear; u.uFadeFar.value = p.fadeFar; u.uInner.value = p.innerLines; u.uFromScene.value = p.colorFromScene;
      u.uOpacity.value = p.opacity; u.uLineColor.value.copy(srgbVec(p.color)); u.uHasNormal.value = post.normalRT ? 1 : 0;
    }, { needsDepth: true, needsNormal });
    return pass;
  };

  // ---------------- generalized Kuwahara with polynomial sector weights (painterly)
  P.kuwahara = (o = {}) => {
    const params = { radius: o.radius ?? 4, sectors: 8, sharpness: o.sharpness ?? 8, halfRes: !!o.halfRes };
    const mat = post._mat(wrap(/* glsl */`
      uniform float uRadius, uQ; uniform vec2 uTexel;
      vec4 effect(vec2 uv) {
        vec3 m[8]; vec3 s[8]; float W[8];
        for (int k = 0; k < 8; k++) { m[k] = vec3(0.0); s[k] = vec3(0.0); W[k] = 0.0; }
        float r = uRadius;
        float zeta = 2.0 / r, eta = (zeta + cos(3.14159265 / 8.0)) / (sin(3.14159265 / 8.0) * sin(3.14159265 / 8.0));
        for (int y = -6; y <= 6; y++) for (int x = -6; x <= 6; x++) {
          vec2 v = vec2(float(x), float(y)) / r;
          if (dot(v, v) > 1.0) continue;
          vec3 c = texture2D(tColor, uv + vec2(float(x), float(y)) * uTexel).rgb;
          float w[8]; float sum = 0.0;
          float vxx = zeta - eta * v.x * v.x, vyy = zeta - eta * v.y * v.y, z;
          z = max(0.0, v.y + vxx); w[0] = z * z; sum += w[0];
          z = max(0.0, -v.x + vyy); w[2] = z * z; sum += w[2];
          z = max(0.0, -v.y + vxx); w[4] = z * z; sum += w[4];
          z = max(0.0, v.x + vyy); w[6] = z * z; sum += w[6];
          vec2 v2 = 0.70710678 * vec2(v.x - v.y, v.x + v.y);
          vxx = zeta - eta * v2.x * v2.x; vyy = zeta - eta * v2.y * v2.y;
          z = max(0.0, v2.y + vxx); w[1] = z * z; sum += w[1];
          z = max(0.0, -v2.x + vyy); w[3] = z * z; sum += w[3];
          z = max(0.0, -v2.y + vxx); w[5] = z * z; sum += w[5];
          z = max(0.0, v2.x + vyy); w[7] = z * z; sum += w[7];
          float g = exp(-3.125 * dot(v, v)) / max(sum, 1e-5);
          for (int k = 0; k < 8; k++) { float wk = w[k] * g; m[k] += c * wk; s[k] += c * c * wk; W[k] += wk; }
        }
        vec3 outc = vec3(0.0); float ws = 0.0;
        for (int k = 0; k < 8; k++) {
          if (W[k] <= 0.0) continue;
          vec3 mk = m[k] / W[k];
          vec3 sk = abs(s[k] / W[k] - mk * mk);
          float sig = sk.r + sk.g + sk.b;
          float wk = 1.0 / (1.0 + pow(1000.0 * sig, uQ * 0.5));
          outc += mk * wk; ws += wk;
        }
        return vec4(ws > 0.0 ? outc / ws : texture2D(tColor, uv).rgb, 1.0);
      }`), { uRadius: { value: 4 }, uQ: { value: 8 }, uTexel: { value: new THREE.Vector2() } });
    return {
      isPass: true, kind: 'kuwahara', stage: 'ldr', needsDepth: false, needsNormal: false, enabled: true, params, uniforms: mat.uniforms,
      render(pp, src, dst, last) {
        const r = Math.max(1, Math.min(6, params.radius * (pp.size.y / 1080)));
        mat.uniforms.uRadius.value = params.halfRes ? Math.max(1, r / 2) : r;
        mat.uniforms.uQ.value = params.sharpness;
        if (params.halfRes) {
          const a = pp.target('kw', 2);
          mat.uniforms.uTexel.value.set(1 / a.width, 1 / a.height);
          mat.uniforms.uDither.value = 0;
          pp.draw(mat, a, src);
          post.copyMat.uniforms.uDither.value = last ? 1 : 0;
          pp.draw(post.copyMat, dst, a.texture);
        } else {
          mat.uniforms.uTexel.value.set(1 / pp.size.x, 1 / pp.size.y);
          mat.uniforms.uDither.value = last ? 1 : 0;
          pp.draw(mat, dst, src);
        }
      },
    };
  };

  // ---------------- colour grading (display-referred; exposure is folded into tone mapping)
  P.grade = (o = {}) => {
    const params = { exposure: o.exposure ?? 0, contrast: o.contrast ?? 1, saturation: o.saturation ?? 1, vibrance: o.vibrance ?? 0,
      lift: o.lift ?? 0, gamma: o.gamma ?? 1, gain: o.gain ?? 1, temperature: o.temperature ?? 0, tint: o.tint ?? 0,
      shadows: o.shadows || null, highlights: o.highlights || null, splitBalance: o.splitBalance ?? 0, curve: o.curve ?? 0,
      splitAmount: o.splitAmount ?? 0.35 };
    return single(post, 'grade', 'ldr', /* glsl */`
      uniform float uContrast, uSat, uVib, uTemp, uTintGM, uBalance, uCurve, uSplit;
      uniform vec3 uLift, uGamma, uGain, uShadowCol, uHighCol; uniform float uHasSplit;
      vec4 effect(vec2 uv) {
        vec3 c = texture2D(tColor, uv).rgb;
        // white balance in linear light
        vec3 lin = slToLinear(c);
        vec3 wb = vec3(1.0 + uTemp * 0.18 + uTintGM * 0.06, 1.0 - uTintGM * 0.12, 1.0 - uTemp * 0.18 + uTintGM * 0.06);
        lin *= wb;
        float l = slLuma(lin);
        float sat = max(max(lin.r, lin.g), lin.b) - min(min(lin.r, lin.g), lin.b);
        lin = mix(vec3(l), lin, uSat * (1.0 + uVib * (1.0 - sat)));
        c = slToSRGB(max(lin, 0.0));
        // lift / gamma / gain (ASC-like)
        c = clamp(c * uGain + uLift * (1.0 - c), 0.0, 1.0);
        c = pow(c, 1.0 / max(uGamma, vec3(0.05)));
        // contrast around mid grey, and an S-curve
        c = (c - 0.5) * uContrast + 0.5;
        c = mix(c, c * c * (3.0 - 2.0 * c), uCurve);
        // split toning
        if (uHasSplit > 0.5) {
          float y = slLuma(clamp(c, 0.0, 1.0));
          float t = smoothstep(0.0, 1.0, y + uBalance * 0.5);
          vec3 tone = mix(uShadowCol, uHighCol, t);
          c = mix(c, c * tone * 2.0, uSplit * (1.0 - abs(y - 0.5)));
        }
        return vec4(clamp(c, 0.0, 1.0), 1.0);
      }`, {
      uContrast: { value: 1 }, uSat: { value: 1 }, uVib: { value: 0 }, uTemp: { value: 0 }, uTintGM: { value: 0 }, uBalance: { value: 0 },
      uCurve: { value: 0 }, uSplit: { value: 0.35 }, uLift: { value: new THREE.Vector3() }, uGamma: { value: new THREE.Vector3(1, 1, 1) },
      uGain: { value: new THREE.Vector3(1, 1, 1) }, uShadowCol: { value: new THREE.Vector3(0.5, 0.5, 0.5) },
      uHighCol: { value: new THREE.Vector3(0.5, 0.5, 0.5) }, uHasSplit: { value: 0 },
    }, params, (p, u) => {
      u.uContrast.value = p.contrast; u.uSat.value = p.saturation; u.uVib.value = p.vibrance; u.uTemp.value = p.temperature;
      u.uTintGM.value = p.tint; u.uBalance.value = p.splitBalance; u.uCurve.value = p.curve; u.uSplit.value = p.splitAmount;
      // numbers or [r, g, b] are the raw factors; a '#hex' says what black (lift), mid grey (gamma) or white (gain) becomes
      u.uLift.value.copy(hexOrNum(p.lift, 0)); u.uGain.value.copy(hexOrNum(p.gain, 1));
      u.uGamma.value.copy(hexOrNum(p.gamma, 1));
      if (typeof p.gamma === 'string') {
        const g = u.uGamma.value;
        g.set(Math.log(0.5) / Math.log(Math.min(0.98, Math.max(0.02, g.x))), Math.log(0.5) / Math.log(Math.min(0.98, Math.max(0.02, g.y))),
          Math.log(0.5) / Math.log(Math.min(0.98, Math.max(0.02, g.z))));
      }
      u.uHasSplit.value = (p.shadows || p.highlights) ? 1 : 0;
      u.uShadowCol.value.copy(p.shadows ? srgbVec(p.shadows) : new THREE.Vector3(0.5, 0.5, 0.5));
      u.uHighCol.value.copy(p.highlights ? srgbVec(p.highlights) : new THREE.Vector3(0.5, 0.5, 0.5));
    });
  };

  P.grain = (o = {}) => {
    const params = { amount: o.amount ?? 0.04, size: o.size ?? 1.5, colored: o.colored ?? 0.2 };
    return single(post, 'grain', 'ldr', /* glsl */`
      uniform float uAmount, uSize, uColored;
      vec4 effect(vec2 uv) {
        vec3 c = texture2D(tColor, uv).rgb;
        vec2 p = gl_FragCoord.xy / uSize;
        float t = floor(uTime * 24.0);
        float n = slVNoise(p + t * 17.31) + slVNoise(p * 2.1 - t * 11.7) * 0.5 - 0.75;
        vec3 nc = vec3(n) + (vec3(slVNoise(p + 3.1 + t), slVNoise(p + 7.7 - t), slVNoise(p + 11.3 + t * 2.0)) - 0.5) * uColored;
        float y = slLuma(c);
        float resp = 4.0 * y * (1.0 - y) * 0.75 + 0.25;
        return vec4(clamp(c + nc * uAmount * resp * 2.0, 0.0, 1.0), 1.0);
      }`, { uAmount: { value: 0.04 }, uSize: { value: 1.5 }, uColored: { value: 0.2 } }, params, (p, u) => {
      u.uAmount.value = p.amount; u.uSize.value = Math.max(0.5, p.size * post.size.y / 1080); u.uColored.value = p.colored;
    });
  };

  P.vignette = (o = {}) => {
    const params = { amount: o.amount ?? 0.3, softness: o.softness ?? 0.5, color: o.color || '#000000', roundness: o.roundness ?? 0.6 };
    return single(post, 'vignette', 'ldr', /* glsl */`
      uniform float uAmount, uSoft, uRound; uniform vec3 uVColor;
      vec4 effect(vec2 uv) {
        vec3 c = texture2D(tColor, uv).rgb;
        vec2 d = (uv - 0.5) * vec2(mix(uResolution.x / uResolution.y, 1.0, uRound), 1.0);
        float r = length(d) * 1.4142;
        float v = smoothstep(1.0 - uSoft, 1.0 + uSoft * 0.2, r);
        return vec4(mix(c, uVColor, v * uAmount), 1.0);
      }`, { uAmount: { value: 0.3 }, uSoft: { value: 0.5 }, uRound: { value: 0.6 }, uVColor: { value: new THREE.Vector3() } }, params, (p, u) => {
      u.uAmount.value = p.amount; u.uSoft.value = p.softness; u.uRound.value = p.roundness; u.uVColor.value.copy(srgbVec(p.color));
    });
  };

  P.chromatic = (o = {}) => {
    const params = { amount: o.amount ?? 0.0025 };
    return single(post, 'chromatic', 'ldr', /* glsl */`
      uniform float uAmount;
      vec4 effect(vec2 uv) {
        vec2 d = uv - 0.5;
        float k = dot(d, d) * 4.0;
        vec2 off = d * uAmount * k;
        return vec4(texture2D(tColor, uv + off).r, texture2D(tColor, uv).g, texture2D(tColor, uv - off).b, 1.0);
      }`, { uAmount: { value: 0.0025 } }, params, (p, u) => { u.uAmount.value = p.amount * 10.0; });
  };

  P.tiltshift = (o = {}) => {
    const params = { focus: o.focus ?? 0.5, band: o.band ?? 0.15, blur: o.blur ?? 6 };
    return single(post, 'tiltshift', 'ldr', /* glsl */`
      uniform float uFocus, uBand, uBlur;
      vec4 effect(vec2 uv) {
        float k = smoothstep(uBand * 0.5, uBand * 0.5 + 0.25, abs(uv.y - uFocus)) * uBlur;
        if (k < 0.3) return vec4(texture2D(tColor, uv).rgb, 1.0);
        vec3 acc = vec3(0.0); float w = 0.0; float ga = 2.39996323;
        for (int i = 0; i < 20; i++) {
          float rr = sqrt((float(i) + 0.5) / 20.0) * k;
          vec2 off = vec2(cos(float(i) * ga), sin(float(i) * ga)) * rr / uResolution;
          acc += texture2D(tColor, uv + off).rgb; w += 1.0;
        }
        return vec4(acc / w, 1.0);
      }`, { uFocus: { value: 0.5 }, uBand: { value: 0.15 }, uBlur: { value: 6 } }, params, (p, u) => {
      u.uFocus.value = p.focus; u.uBand.value = p.band; u.uBlur.value = p.blur * post.size.y / 1080;
    });
  };

  P.sharpen = (o = {}) => {
    const params = { amount: o.amount ?? 0.3 };
    return single(post, 'sharpen', 'ldr', /* glsl */`
      uniform float uAmount;
      vec4 effect(vec2 uv) {
        vec2 t = 1.0 / uResolution;
        vec3 c = texture2D(tColor, uv).rgb;
        vec3 n = texture2D(tColor, uv + vec2(0.0, t.y)).rgb, s = texture2D(tColor, uv - vec2(0.0, t.y)).rgb;
        vec3 e = texture2D(tColor, uv + vec2(t.x, 0.0)).rgb, w = texture2D(tColor, uv - vec2(t.x, 0.0)).rgb;
        vec3 mn = min(c, min(min(n, s), min(e, w))), mx = max(c, max(max(n, s), max(e, w)));
        vec3 amp = sqrt(clamp(min(mn, 1.0 - mx) / max(mx, 1e-4), 0.0, 1.0));
        vec3 k = -amp * mix(0.125, 0.2, uAmount);
        vec3 r = (c + (n + s + e + w) * k) / (1.0 + 4.0 * k);
        return vec4(mix(c, clamp(r, 0.0, 1.0), clamp(uAmount * 2.0, 0.0, 1.0)), 1.0);
      }`, { uAmount: { value: 0.3 } }, params, (p, u) => { u.uAmount.value = p.amount; });
  };

  P.posterize = (o = {}) => {
    const params = { levels: o.levels ?? 6, dither: o.dither ?? 0.0 };
    return single(post, 'posterize', 'ldr', /* glsl */`
      uniform float uLevels, uPDither;
      vec4 effect(vec2 uv) {
        vec3 c = texture2D(tColor, uv).rgb;
        float d = (slIGN(gl_FragCoord.xy) - 0.5) * uPDither;
        return vec4(floor(c * uLevels + 0.5 + d) / uLevels, 1.0);
      }`, { uLevels: { value: 6 }, uPDither: { value: 0 } }, params, (p, u) => { u.uLevels.value = Math.max(2, p.levels); u.uPDither.value = p.dither; });
  };

  P.hatch = (o = {}) => {
    const params = { scale: o.scale ?? 6, angle: o.angle ?? 45, density: o.density ?? 1, color: o.color || '#2a2420',
      lightResponse: o.lightResponse ?? 1, width: o.width ?? 0.3 };
    return single(post, 'hatch', 'ldr', /* glsl */`
      uniform float uScale, uAngle, uDensity, uResp, uLineW; uniform vec3 uHColor;
      float lines(vec2 p, float ang, float thick) {
        vec2 d = vec2(cos(ang), sin(ang));
        float v = dot(p, vec2(-d.y, d.x)) / uScale;
        v += slVNoise(p / (uScale * 6.0)) * 0.6;
        float f = abs(fract(v) - 0.5) * 2.0;
        float aa = fwidth(v) * 2.0;
        return 1.0 - smoothstep(thick - aa, thick + aa, f);
      }
      vec4 effect(vec2 uv) {
        vec3 c = texture2D(tColor, uv).rgb;
        float y = slLuma(c);
        float dark = clamp((1.0 - y) * uResp * uDensity, 0.0, 1.2);
        vec2 p = gl_FragCoord.xy * (1080.0 / uResolution.y);
        float a = radians(uAngle);
        float h = 0.0;
        h = max(h, lines(p, a, uLineW * smoothstep(0.15, 0.45, dark)));
        h = max(h, lines(p, a + 1.5708, uLineW * smoothstep(0.4, 0.7, dark)));
        h = max(h, lines(p, a + 0.7854, uLineW * smoothstep(0.62, 0.9, dark)));
        h = max(h, lines(p * 0.5, a - 0.7854, uLineW * smoothstep(0.85, 1.1, dark)));
        return vec4(mix(c, uHColor, h * 0.85), 1.0);
      }`, { uScale: { value: 6 }, uAngle: { value: 45 }, uDensity: { value: 1 }, uResp: { value: 1 }, uLineW: { value: 0.3 }, uHColor: { value: new THREE.Vector3() } },
    params, (p, u) => {
      u.uScale.value = p.scale; u.uAngle.value = p.angle; u.uDensity.value = p.density; u.uResp.value = p.lightResponse;
      u.uLineW.value = p.width; u.uHColor.value.copy(srgbVec(p.color));
    });
  };

  P.fxaa = (o = {}) => {
    const params = { subpix: o.subpix ?? 0.6 };
    return single(post, 'fxaa', 'ldr', /* glsl */`
      uniform float uSubpix;
      float lum(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }
      vec4 effect(vec2 uv) {
        vec2 t = 1.0 / uResolution;
        vec3 rgbM = texture2D(tColor, uv).rgb;
        float lM = lum(rgbM);
        float lN = lum(texture2D(tColor, uv + vec2(0.0, t.y)).rgb), lS = lum(texture2D(tColor, uv - vec2(0.0, t.y)).rgb);
        float lE = lum(texture2D(tColor, uv + vec2(t.x, 0.0)).rgb), lW = lum(texture2D(tColor, uv - vec2(t.x, 0.0)).rgb);
        float mn = min(lM, min(min(lN, lS), min(lE, lW))), mx = max(lM, max(max(lN, lS), max(lE, lW)));
        float range = mx - mn;
        if (range < max(0.0312, mx * 0.125)) return vec4(rgbM, 1.0);
        float lNW = lum(texture2D(tColor, uv + vec2(-t.x, t.y)).rgb), lNE = lum(texture2D(tColor, uv + t).rgb);
        float lSW = lum(texture2D(tColor, uv - t).rgb), lSE = lum(texture2D(tColor, uv + vec2(t.x, -t.y)).rgb);
        float sub = clamp(abs((2.0 * (lN + lS + lE + lW) + lNW + lNE + lSW + lSE) / 12.0 - lM) / range, 0.0, 1.0);
        sub = smoothstep(0.0, 1.0, sub); sub = sub * sub * uSubpix;
        float edgeH = abs(lNW + lNE - 2.0 * lN) + 2.0 * abs(lW + lE - 2.0 * lM) + abs(lSW + lSE - 2.0 * lS);
        float edgeV = abs(lNW + lSW - 2.0 * lW) + 2.0 * abs(lN + lS - 2.0 * lM) + abs(lNE + lSE - 2.0 * lE);
        bool horz = edgeH >= edgeV;
        float l1 = horz ? lS : lW, l2 = horz ? lN : lE;
        float g1 = abs(l1 - lM), g2 = abs(l2 - lM);
        float step = horz ? t.y : t.x;
        float gScaled; float lEdge;
        if (g1 >= g2) { step = -step; gScaled = 0.25 * g1; lEdge = 0.5 * (l1 + lM); } else { gScaled = 0.25 * g2; lEdge = 0.5 * (l2 + lM); }
        vec2 cur = uv; if (horz) cur.y += step * 0.5; else cur.x += step * 0.5;
        vec2 off = horz ? vec2(t.x, 0.0) : vec2(0.0, t.y);
        vec2 p1 = cur - off, p2 = cur + off;
        float e1 = lum(texture2D(tColor, p1).rgb) - lEdge, e2 = lum(texture2D(tColor, p2).rgb) - lEdge;
        bool r1 = abs(e1) >= gScaled, r2 = abs(e2) >= gScaled;
        for (int i = 0; i < 10; i++) {
          if (r1 && r2) break;
          if (!r1) { p1 -= off * 1.5; e1 = lum(texture2D(tColor, p1).rgb) - lEdge; r1 = abs(e1) >= gScaled; }
          if (!r2) { p2 += off * 1.5; e2 = lum(texture2D(tColor, p2).rgb) - lEdge; r2 = abs(e2) >= gScaled; }
        }
        float d1 = horz ? uv.x - p1.x : uv.y - p1.y, d2 = horz ? p2.x - uv.x : p2.y - uv.y;
        bool dir1 = d1 < d2; float dist = min(d1, d2); float len = d1 + d2;
        float pixOff = -dist / len + 0.5;
        bool smaller = lM < lEdge;
        bool ok = ((dir1 ? e1 : e2) < 0.0) != smaller;
        float fin = max(ok ? pixOff : 0.0, sub);
        vec2 fuv = uv; if (horz) fuv.y += fin * step; else fuv.x += fin * step;
        return vec4(texture2D(tColor, fuv).rgb, 1.0);
      }`, { uSubpix: { value: 0.6 } }, params, (p, u) => { u.uSubpix.value = p.subpix; });
  };

  // custom: { fragment (defines vec4 effect(vec2 uv)), uniforms, needsDepth, needsNormal, stage: 'ldr'|'hdr' }
  P.custom = (o = {}) => {
    const params = o.params || {};
    const pass = single(post, 'custom', o.stage === 'hdr' ? 'hdr' : 'ldr', o.fragment || 'vec4 effect(vec2 uv) { return texture2D(tColor, uv); }',
      o.uniforms || {}, params, o.bind || null, { needsDepth: !!o.needsDepth, needsNormal: !!o.needsNormal });
    return pass;
  };

  return P;
}
