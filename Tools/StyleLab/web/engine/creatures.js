// Spiders: a handful roam the town, notice the player within 20 m, scuttle in, leap and bite for 8. Three hits kill
// one: it flips onto its back, curls its legs and fades, and another comes later. The legs walk a procedural
// alternating-tetrapod gait on the rig's own bones (femur_<pair>_<side>, tibia_..., body, abdomen, head).
import * as THREE from 'three';
import { clamp, lerp, mulberry32, DEG } from './util.js';

const UP = new THREE.Vector3(0, 1, 0);

export class Spiders {
  constructor(engine) {
    this.e = engine;
    this.list = [];
    this.root = new THREE.Group(); this.root.name = 'Spiders';
    this.rand = mulberry32(1234);
    this.enabled = true;
  }

  init(count = 7) {
    const proto = this.e.assets.model('Spider');
    if (!proto) return;
    const b = proto.info.bounds || [[-0.8, 0, -0.8], [0.8, 0.9, 0.8]];
    this.size = { min: new THREE.Vector3(...b[0]), max: new THREE.Vector3(...b[1]) };
    this.height = this.size.max.y - this.size.min.y;
    this.reach = Math.max(this.size.max.z - this.size.min.z, this.size.max.x - this.size.min.x) * 0.5;
    // where they live: the scene's spawners in town first, then around Main Street
    const town = (p) => p[2] > -14 && p[2] < 14 && p[0] > -66 && p[0] < 12;
    const spawns = this.e.assets.scene.creatures.filter((c) => /spider/i.test(c.model || 'Spider') && !/mother|grave|boss/i.test(c.note || c.model || ''));
    let pts = spawns.filter((c) => town(c.p)).map((c) => new THREE.Vector3(...c.p));
    const street = [[6, 2], [-8, 3], [-22, -2], [-34, 1], [-4, 8], [-46, -3], [-16, 6], [-58, 2]];
    for (const [x, z] of street) if (pts.length < count) pts.push(new THREE.Vector3(x, 0, z));
    pts = pts.slice(0, count);
    this.homes = pts;
    for (let i = 0; i < pts.length; i++) {
      const obj = this.e.cloneModel('Spider', true);
      if (!obj) break;
      const s = this._make(obj, i);
      s.home.copy(pts[i]);
      this._respawn(s, true);
      this.list.push(s);
      this.root.add(obj);
    }
  }

  _make(obj, i) {
    // bones by their game names (the export prefixes them with the model's: Spider_femur_0_l)
    const bones = {};
    obj.traverse((o) => { if (o.isBone) bones[o.name.replace(/^Spider_+/, '')] = o; });
    const rest = {};
    obj.updateMatrixWorld(true);
    const rootInv = new THREE.Quaternion().copy(obj.quaternion).invert();
    for (const [n, bone] of Object.entries(bones)) {
      const wq = new THREE.Quaternion(); bone.getWorldQuaternion(wq);
      const pq = new THREE.Quaternion(); if (bone.parent) bone.parent.getWorldQuaternion(pq);
      const hip = new THREE.Vector3(); bone.getWorldPosition(hip); obj.worldToLocal(hip);
      let tip = null;
      const child = bone.children.find((c) => c.isBone);
      if (child) { tip = new THREE.Vector3(); child.getWorldPosition(tip); obj.worldToLocal(tip); }
      rest[n] = { q: bone.quaternion.clone(), pos: bone.position.clone(), parentQ: rootInv.clone().multiply(pq), hip, tip };
    }
    const legs = [];
    for (let pair = 0; pair < 4; pair++) for (const side of ['l', 'r']) {
      const f = bones[`femur_${pair}_${side}`], t = bones[`tibia_${pair}_${side}`];
      if (!f) continue;
      const r = rest[`femur_${pair}_${side}`];
      const dir = r.tip ? r.tip.clone().sub(r.hip) : new THREE.Vector3(side === 'l' ? 1 : -1, 0, 0);
      dir.y = 0; dir.normalize();
      legs.push({ f, t, fr: r, tr: t ? rest[`tibia_${pair}_${side}`] : null, pair, side, dir, liftAxis: new THREE.Vector3().crossVectors(dir, UP).normalize(),
        group: (pair + (side === 'l' ? 0 : 1)) % 2, sx: Math.sign(r.hip.x || (side === 'l' ? 1 : -1)) });
    }
    return {
      i, obj, bones, rest, legs, home: new THREE.Vector3(), pos: new THREE.Vector3(), vel: new THREE.Vector3(), yaw: 0,
      state: 'wander', t: 0, target: new THREE.Vector3(), phase: this.rand() * 6.28, hp: 3, flip: 0, curl: 0, fade: 1,
      leapFrom: new THREE.Vector3(), leapTo: new THREE.Vector3(), leapT: 0, cooldown: 0, flinch: 0, alive: true, deadT: 0,
      speed: 0, bite: 0,
    };
  }

  _respawn(s, first) {
    const r = this.rand;
    const h = this.homes[s.i % this.homes.length];
    s.pos.set(h.x + (r() - 0.5) * 6, 0, h.z + (r() - 0.5) * 6);
    s.pos.y = this._ground(s.pos.x, s.pos.z);
    s.yaw = r() * Math.PI * 2; s.hp = 3; s.state = 'wander'; s.t = 0; s.flip = 0; s.curl = 0; s.fade = 1;
    s.alive = true; s.obj.visible = true; s.cooldown = 0; s.deadT = 0; s.flinch = 0;
    this._setFade(s, 1);
    this._pickWander(s);
  }

  _ground(x, z) { return this.e.heights ? this.e.heights.height(x, z) : 0; }
  _pickWander(s) {
    const r = this.rand;
    s.target.set(s.home.x + (r() - 0.5) * 16, 0, s.home.z + (r() - 0.5) * 16);
    s.t = 2 + r() * 4;
  }

  _setFade(s, f) {
    s.fade = f;
    s.obj.traverse((o) => {
      if (!o.isMesh) return;
      const m = o.material;
      if (m && m.userData && m.userData.slUniforms && m.userData.slUniforms.uSlFade) m.userData.slUniforms.uSlFade.value = f;
    });
  }

  // Bullets: the nearest spider whose body sphere the ray passes through.
  hitTest(origin, dir, maxDist) {
    let best = null;
    const c = new THREE.Vector3();
    for (const s of this.list) {
      if (!s.alive || s.state === 'dead') continue;
      c.copy(s.pos); c.y += this.height * 0.55;
      const r = this.reach * 0.75;
      const oc = c.clone().sub(origin);
      const t = oc.dot(dir);
      if (t < 0 || t > maxDist) continue;
      const d2 = oc.lengthSq() - t * t;
      if (d2 > r * r) continue;
      const tt = t - Math.sqrt(r * r - d2);
      if (!best || tt < best.dist) best = { spider: s, dist: tt, point: origin.clone().addScaledVector(dir, tt) };
    }
    return best;
  }

  damage(s, amount = 1) {
    if (!s.alive || s.state === 'dead') return false;
    s.hp -= amount;
    s.flinch = 0.18;
    if (s.state === 'wander') { s.state = 'chase'; }
    if (s.hp <= 0) { s.state = 'dead'; s.deadT = 0; s.vel.set(0, 3.2, 0); return true; }
    return false;
  }

  // dt: the animation step (env.animStep quantises it); player: { pos (feet), alive }
  update(dt, player) {
    if (!this.enabled) return;
    for (const s of this.list) this._think(s, dt, player);
  }

  _think(s, dt, player) {
    if (dt <= 0) { this._pose(s, 0); return; }
    const toP = new THREE.Vector3().subVectors(player.pos, s.pos); toP.y = 0;
    const dist = toP.length();
    s.cooldown = Math.max(0, s.cooldown - dt);
    s.flinch = Math.max(0, s.flinch - dt);
    let moveSpeed = 0;
    switch (s.state) {
      case 'wander': {
        s.t -= dt;
        const d = new THREE.Vector3().subVectors(s.target, s.pos); d.y = 0;
        if (d.length() < 0.6 || s.t <= 0) { this._pickWander(s); s.t += 1.5; }
        else moveSpeed = s.t > 1.2 ? 1.4 : 0;
        if (moveSpeed) this._steer(s, d, moveSpeed, dt);
        if (player.alive && dist < 20) s.state = 'chase';
        break;
      }
      case 'chase': {
        if (!player.alive || dist > 32) { s.state = 'wander'; this._pickWander(s); break; }
        if (dist < 4.6 && s.cooldown <= 0) {
          s.state = 'leap'; s.leapT = 0;
          s.leapFrom.copy(s.pos);
          s.leapTo.copy(player.pos).addScaledVector(toP.normalize(), -0.9);
          s.leapTo.y = this._ground(s.leapTo.x, s.leapTo.z);
          break;
        }
        if (dist > 2.2) { moveSpeed = 5.2; this._steer(s, toP, moveSpeed, dt); }
        else this._face(s, toP, dt);
        break;
      }
      case 'leap': {
        s.leapT += dt / 0.55;
        const k = Math.min(1, s.leapT);
        s.pos.lerpVectors(s.leapFrom, s.leapTo, k);
        s.pos.y = lerp(s.leapFrom.y, s.leapTo.y, k) + Math.sin(Math.PI * k) * 1.3;
        this._face(s, toP, dt * 3);
        if (k >= 1) {
          s.state = 'chase'; s.cooldown = 1.3;
          const d = Math.hypot(player.pos.x - s.pos.x, player.pos.z - s.pos.z);
          if (d < 1.9 && player.alive) { s.bite = 0.25; this.e.onPlayerBitten(8, s); }
        }
        moveSpeed = 0;
        break;
      }
      case 'dead': {
        s.deadT += dt;
        s.flip = Math.min(1, s.flip + dt / 0.35);
        s.curl = Math.min(1, s.curl + dt / 0.5);
        s.vel.y -= 14 * dt;
        s.pos.y += s.vel.y * dt;
        const g = this._ground(s.pos.x, s.pos.z) + this.height * 0.35 * s.flip;
        if (s.pos.y < g) { s.pos.y = g; s.vel.y = 0; }
        if (s.deadT > 1.6) this._setFade(s, clamp(1 - (s.deadT - 1.6) / 0.9, 0, 1));
        if (s.deadT > 2.6) { s.obj.visible = false; s.alive = false; }
        break;
      }
    }
    if (!s.alive && s.state === 'dead') {
      s.deadT += 0;   // waits for respawn
      s.respawn = (s.respawn || 0) + dt;
      if (s.respawn > 7) { s.respawn = 0; this._respawn(s); }
    }
    s.bite = Math.max(0, s.bite - dt);
    if (s.state !== 'leap' && s.state !== 'dead') {
      const gy = this._ground(s.pos.x, s.pos.z);
      s.pos.y = gy;
      const p = s.pos.clone();
      this.e.world.collide(p, this.reach * 0.6, this.height);
      s.pos.x = p.x; s.pos.z = p.z;
    }
    s.speed = moveSpeed;
    s.phase += dt * moveSpeed / Math.max(0.25, this.reach * 0.55) * Math.PI;
    this._pose(s, dt);
  }

  _steer(s, d, speed, dt) {
    d = d.clone(); d.y = 0;
    if (d.lengthSq() < 1e-6) return;
    d.normalize();
    this._face(s, d, dt);
    const fwd = new THREE.Vector3(Math.sin(s.yaw), 0, Math.cos(s.yaw));
    const k = Math.max(0, fwd.dot(d));
    s.pos.addScaledVector(fwd, speed * k * dt);
  }
  _face(s, d, dt) {
    const want = Math.atan2(d.x, d.z);
    let da = want - s.yaw; da = Math.atan2(Math.sin(da), Math.cos(da));
    s.yaw += da * Math.min(1, dt * 7);
  }

  // Root transform and the legs.
  _pose(s, dt) {
    const o = s.obj;
    const n = this.e.heights ? this.e.heights.normal(s.pos.x, s.pos.z) : UP;
    const qYaw = new THREE.Quaternion().setFromAxisAngle(UP, s.yaw);
    const qTilt = new THREE.Quaternion().setFromUnitVectors(UP, n.clone().lerp(UP, 0.4).normalize());
    const q = qTilt.multiply(qYaw);
    if (s.flip > 0) {
      const fwd = new THREE.Vector3(0, 0, 1).applyQuaternion(qYaw);
      q.premultiply(new THREE.Quaternion().setFromAxisAngle(fwd, Math.PI * smoothStep(s.flip)));
    }
    if (s.state === 'leap') {
      const right = new THREE.Vector3(1, 0, 0).applyQuaternion(qYaw);
      q.premultiply(new THREE.Quaternion().setFromAxisAngle(right, -0.35 * Math.sin(Math.PI * Math.min(1, s.leapT))));
    }
    if (s.flinch > 0) {
      const right = new THREE.Vector3(1, 0, 0).applyQuaternion(qYaw);
      q.premultiply(new THREE.Quaternion().setFromAxisAngle(right, 0.25 * s.flinch / 0.18));
    }
    o.quaternion.copy(q);
    o.position.copy(s.pos);
    if (s.flip > 0) o.position.y += this.height * 0.5 * Math.sin(Math.PI * Math.min(1, s.flip));
    o.updateMatrix();
    if (!s.legs.length) {
      // a rig without leg bones (or a static mesh): a scuttling bob
      o.position.y += Math.abs(Math.sin(s.phase * 2)) * 0.04 * Math.min(1, s.speed);
      o.updateMatrix();
      return;
    }
    const moving = Math.min(1, s.speed / 1.5);
    const leaping = s.state === 'leap' ? Math.sin(Math.PI * Math.min(1, s.leapT)) : 0;
    const qa = new THREE.Quaternion(), qb = new THREE.Quaternion(), tmp = new THREE.Quaternion();
    for (const L of s.legs) {
      const r = L.fr;
      const ph = s.phase + (L.group ? Math.PI : 0) + L.pair * 0.35;
      const lift = Math.max(0, Math.sin(ph)) * 0.42 * moving + leaping * (L.pair < 2 ? 0.6 : -0.25) + s.curl * 1.1 + (s.bite > 0 && L.pair === 0 ? 0.7 : 0);
      const f = -Math.cos(ph) * moving;
      const swing = -L.sx * f * 0.32 + (leaping ? -L.sx * (L.pair < 2 ? 0.35 : -0.35) * leaping : 0) - L.sx * s.curl * 0.4 * (L.pair < 2 ? 1 : -1);
      qa.setFromAxisAngle(L.liftAxis, lift);
      qb.setFromAxisAngle(UP, swing);
      const R = qb.multiply(qa);                 // model space rotation about the hip
      // into the bone's frame: parentRest^-1 * R * parentRest * restLocal
      tmp.copy(r.parentQ).invert().multiply(R).multiply(r.parentQ);
      L.f.quaternion.copy(tmp).multiply(r.q);
      if (L.t) {
        const rt = L.tr;
        const bend = lift * 0.55 + s.curl * 0.9;
        qa.setFromAxisAngle(L.liftAxis, -bend);
        tmp.copy(rt.parentQ).invert().multiply(qa).multiply(rt.parentQ);
        L.t.quaternion.copy(tmp).multiply(rt.q);
      }
    }
    const body = s.bones.body;
    if (body) {
      const rb = s.rest.body;
      body.position.copy(rb.pos);
      body.position.y += Math.sin(s.phase * 2) * 0.02 * moving;
    }
    const abd = s.bones.abdomen;
    if (abd) {
      const ra = s.rest.abdomen;
      qa.setFromAxisAngle(new THREE.Vector3(1, 0, 0), Math.sin(s.phase * 2 + 1) * 0.06 * moving + s.curl * 0.3);
      tmp.copy(ra.parentQ).invert().multiply(qa).multiply(ra.parentQ);
      abd.quaternion.copy(tmp).multiply(ra.q);
    }
  }

  // A posed spider for a still: forward distance f and right offset r from the camera (metres), state, leap fraction.
  pose(list, camPos, yawDeg) {
    const a = yawDeg * DEG;
    const fwd = new THREE.Vector3(-Math.sin(a), 0, Math.cos(a)), right = new THREE.Vector3(-Math.cos(a), 0, -Math.sin(a));
    this.list.forEach((s, i) => {
      const p = list[i];
      if (!p) { s.obj.visible = false; s.alive = false; s.state = 'hidden'; return; }
      s.alive = true; s.obj.visible = true; s.hp = 3; s.flip = 0; s.curl = 0; this._setFade(s, 1);
      s.pos.copy(camPos).addScaledVector(fwd, p.f).addScaledVector(right, p.r || 0);
      s.pos.y = this._ground(s.pos.x, s.pos.z);
      const face = new THREE.Vector3().subVectors(camPos, s.pos); face.y = 0;
      s.yaw = Math.atan2(face.x, face.z) + (p.turn || 0) * DEG;
      s.phase = p.phase ?? i * 1.3;
      s.speed = p.speed ?? 3;
      s.state = p.state || 'chase';
      if (s.state === 'leap') {
        s.leapT = p.leap ?? 0.5;
        s.pos.y += Math.sin(Math.PI * s.leapT) * 1.3;
        s.speed = 0;
      }
      this._pose(s, 0);
    });
  }
}

const smoothStep = (t) => t * t * (3 - 2 * t);
