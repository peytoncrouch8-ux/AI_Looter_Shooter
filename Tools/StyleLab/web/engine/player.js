// The player: a first-person walk on the heights (walk 5.1 m/s, sprint 7.9, eye 1.4 m, jump about 1 m), mouse look
// with pointer lock, and the Whisper Bullpup in view: sway, bob, a spring recoil, aim down the sight, automatic fire at
// about 10 shots a second, a 24-round magazine and a reload.
import * as THREE from 'three';
import { clamp, damp, DEG, dir3, ueYawOf, lerp } from './util.js';
import { GUN_LAYER } from './lighting.js';

const WALK = 5.1, SPRINT = 7.9, EYE = 1.4, JUMP_V = Math.sqrt(2 * 9.8 * 1.035), GRAV = 9.8;

export class Player {
  constructor(engine) {
    this.e = engine;
    this.pos = new THREE.Vector3();       // feet
    this.vel = new THREE.Vector3();
    this.yaw = 90; this.pitch = 0;        // UE degrees
    this.onGround = true;
    this.keys = new Set();
    this.health = 100; this.maxHealth = 100; this.level = 7; this.xp = 1240; this.xpNext = 2000;
    this.mag = 24; this.magMax = 24; this.reserve = 168;
    this.reloading = 0; this.fireCooldown = 0; this.firing = false; this.aiming = false; this.aim = 0;
    this.bob = 0; this.recoil = new THREE.Vector2(); this.recoilV = new THREE.Vector2(); this.kick = 0; this.kickV = 0;
    this.sway = new THREE.Vector2(); this.lookDelta = new THREE.Vector2();
    this.alive = true; this.respawnT = 0;
    this.gun = new THREE.Group(); this.gun.name = 'Viewmodel';
    this.gunPivot = new THREE.Group(); this.gunPivot.add(this.gun);
    this.muzzleLocal = new THREE.Vector3(0, 0, 0.5);
    this.viewScale = 0.85;
    this.hipOffset = new THREE.Vector3(0.155, -0.165, -0.36);
    this.adsOffset = new THREE.Vector3(0, -0.06, -0.25);
  }

  // The gun model under the camera, framed like a clean viewmodel bottom-right (no arms): its butt (the Stock socket)
  // just beside the cheek, the barrel reaching in toward the crosshair.
  buildGun() {
    const model = this.e.cloneModel('Gun_Bullpup', false);
    if (!model) return;
    model.traverse((o) => { o.layers.set(GUN_LAYER); if (o.isMesh) { o.castShadow = false; o.receiveShadow = true; o.frustumCulled = false; } });
    model.rotation.y = Math.PI;        // the model's front (+z) along the camera's view (-z)
    const rec = this.e.assets.model('Gun_Bullpup');
    const sockets = rec.sockets || {};
    const bounds = rec.info.bounds || [[-0.05, -0.1, 0], [0.05, 0.1, 0.7]];
    const len = bounds[1][2] - bounds[0][2];
    // a little under life size reads less dominant on screen (the viewmodel's own scale, as games do)
    const s = (len > 0.2 && len < 2.5 ? 1 : 0.7 / Math.max(len, 0.01)) * this.viewScale;
    model.scale.setScalar(s);
    const v = (a, d) => (a ? new THREE.Vector3(...a) : new THREE.Vector3(...d)).multiplyScalar(s);
    const rot = (p) => new THREE.Vector3(-p.x, p.y, -p.z);    // turned 180 degrees about y
    const stock = rot(v(sockets.Stock, [0, (bounds[0][1] + bounds[1][1]) * 0.3, bounds[0][2]]));
    this.aimLocal = rot(v(sockets.Aim, [0, bounds[1][1], len * 0.3])).sub(stock);
    this.muzzleLocal = rot(v(sockets.Muzzle, [0, 0, bounds[1][2]])).sub(stock);
    this.gripLocal = new THREE.Vector3();
    model.position.copy(stock).negate();
    this.gun.add(model);
    this.model = model;
    this.gripToAim = this.aimLocal.clone();
    this.hipOffset = new THREE.Vector3(0.152, -0.24, -0.3);
    this.adsOffset = new THREE.Vector3(0, 0, -0.2);
    this.e.fx.flashScale = this.viewScale;
    this.flash = this.e.fx.makeFlash(this.gun, this.muzzleLocal.clone());
    this.e.camera.add(this.gunPivot);
  }

  // Places the player at a shot's camera.
  placeAt(x, z, yaw, pitch) {
    this.pos.set(x, this.e.ground(x, z), z);
    this.vel.set(0, 0, 0);
    this.yaw = yaw; this.pitch = pitch;
  }

  onMouse(dx, dy) {
    const k = this.aiming ? 0.6 : 1;
    this.yaw = (this.yaw + dx * 0.09 * k + 360) % 360;
    this.pitch = clamp(this.pitch - dy * 0.09 * k, -85, 85);
    this.lookDelta.x += dx; this.lookDelta.y += dy;
  }

  reload() {
    if (this.reloading > 0 || this.mag === this.magMax || this.reserve <= 0) return;
    this.reloading = 1.9;
  }

  update(dt, still) {
    const cam = this.e.camera;
    if (!still) this._move(dt);
    // the camera at the eye
    const eye = this.pos.clone(); eye.y += EYE;
    cam.position.copy(eye);
    const look = dir3(this.yaw, this.pitch);
    cam.up.set(0, 1, 0);
    cam.lookAt(eye.clone().add(look));
    // ads zoom: UE keeps the horizontal FOV
    this.aim = still ? (this.aiming ? 1 : 0) : damp(this.aim, this.aiming ? 1 : 0, 14, dt);
    this.e.setHFov(lerp(this.e.baseHFov, this.e.baseHFov / 1.3, this.aim));
    // gun
    if (!still) this._gun(dt);
    this._placeGun(dt, still);
    cam.updateMatrixWorld(true);
  }

  _move(dt) {
    const k = this.keys;
    const fwd = (k.has('KeyW') ? 1 : 0) - (k.has('KeyS') ? 1 : 0);
    const side = (k.has('KeyD') ? 1 : 0) - (k.has('KeyA') ? 1 : 0);
    const sprint = k.has('ShiftLeft') || k.has('ShiftRight');
    const speed = (sprint && fwd > 0 && !this.aiming ? SPRINT : WALK) * (this.aiming ? 0.6 : 1);
    const f = dir3(this.yaw, 0), r = dir3(this.yaw + 90, 0);
    const want = new THREE.Vector3().addScaledVector(f, fwd).addScaledVector(r, side);
    if (want.lengthSq() > 0) want.normalize().multiplyScalar(speed);
    const accel = this.onGround ? 14 : 3;
    this.vel.x = damp(this.vel.x, want.x, accel, dt);
    this.vel.z = damp(this.vel.z, want.z, accel, dt);
    if (k.has('Space') && this.onGround) { this.vel.y = JUMP_V; this.onGround = false; }
    this.vel.y -= GRAV * dt;
    const next = this.pos.clone().addScaledVector(this.vel, dt);
    this.e.world.collide(next, 0.35, 1.8);
    // keep inside the region
    const H = this.e.heights;
    if (H) {
      const r0 = H.rect;
      next.x = clamp(next.x, r0[0] + 1, r0[2] - 1); next.z = clamp(next.z, r0[1] + 1, r0[3] - 1);
    }
    const g = this.e.ground(next.x, next.z);
    // too steep a step up is a wall
    if (this.onGround && g - this.pos.y > 0.6 && (g - this.pos.y) / Math.max(1e-3, Math.hypot(next.x - this.pos.x, next.z - this.pos.z)) > 1.6) {
      next.x = this.pos.x; next.z = this.pos.z;
    }
    const g2 = this.e.ground(next.x, next.z);
    if (next.y <= g2 + 0.02) { next.y = g2; this.vel.y = 0; this.onGround = true; }
    else if (this.onGround && next.y - g2 < 0.35 && this.vel.y <= 0) { next.y = g2; this.vel.y = 0; }
    else this.onGround = false;
    this.pos.copy(next);
    const hs = Math.hypot(this.vel.x, this.vel.z);
    if (this.onGround) this.bob += dt * hs * 1.25;
  }

  _gun(dt) {
    this.fireCooldown -= dt;
    if (this.reloading > 0) {
      this.reloading -= dt;
      if (this.reloading <= 0) {
        const take = Math.min(this.magMax - this.mag, this.reserve);
        this.mag += take; this.reserve -= take; this.reloading = 0;
      }
    }
    // a tap fires even when it is released before the next frame
    if ((this.firing || this.tap) && this.fireCooldown <= 0 && this.reloading <= 0 && this.alive) {
      this.tap = false;
      if (this.mag > 0) { this.fire(); this.fireCooldown += 0.1; if (this.fireCooldown < 0) this.fireCooldown = 0.1; }
      else { this.reload(); this.fireCooldown = 0.2; }
    }
    if (this.fireCooldown < 0) this.fireCooldown = 0;
    // springs
    const kS = 160, kD = 18;
    this.recoilV.addScaledVector(this.recoil, -kS * dt).multiplyScalar(Math.exp(-kD * dt));
    this.recoil.addScaledVector(this.recoilV, dt);
    this.kickV += (-this.kick * 260 - this.kickV * 22) * dt;
    this.kick += this.kickV * dt;
  }

  // One shot: the ray from the eye, its hit on the world or a spider, the flash, the tracer and the impact.
  fire(opts = {}) {
    const cam = this.e.camera;
    cam.updateMatrixWorld(true);
    this.mag = Math.max(0, this.mag - 1);
    const origin = cam.position.clone();
    const dir = cam.getWorldDirection(new THREE.Vector3());
    const spread = (this.aiming ? 0.002 : 0.012) * (opts.noSpread ? 0 : 1);
    if (spread) {
      const r = this.e.rand;
      dir.add(new THREE.Vector3(r() - 0.5, r() - 0.5, r() - 0.5).multiplyScalar(spread * 2)).normalize();
    }
    const muzzleW = this.gun.localToWorld(this.muzzleLocal.clone());
    const worldHit = this.e.world.raycast(origin, dir, 250);
    const spiderHit = this.e.spiders.hitTest(origin, dir, worldHit ? worldHit.dist : 250);
    let end;
    if (spiderHit) {
      end = spiderHit.point;
      const kill = this.e.spiders.damage(spiderHit.spider, 1);
      this.e.fx.impact(end, dir.clone().negate(), 'flesh');
      this.e.hud && this.e.hud.onHitMarker(kill);
      if (kill) { this.xp += 160; this.e.hud && this.e.hud.onXp(160); }
    } else if (worldHit) {
      end = worldHit.point;
      this.e.fx.impact(end, worldHit.normal, 'world', this.e.groundColorAt(end));
    } else {
      end = origin.clone().addScaledVector(dir, 250);
    }
    this.e.fx.muzzleFlash(muzzleW, 1);
    this.e.fx.tracer(muzzleW, end);
    this.e.hud && this.e.hud.onFire();
    // recoil: the view climbs a little, the gun kicks back
    if (!opts.noRecoil) {
      const climb = this.aiming ? 0.35 : 0.6;
      this.pitch = clamp(this.pitch + climb * (0.7 + this.e.rand() * 0.6), -85, 85);
      this.yaw += (this.e.rand() - 0.5) * 0.35;
      this.recoilV.x += 1.6 + this.e.rand() * 0.8; this.recoilV.y += (this.e.rand() - 0.5) * 1.2;
      this.kickV += 2.2;
    }
    return { end, spider: spiderHit, world: worldHit };
  }

  _placeGun(dt, still) {
    const hs = Math.hypot(this.vel.x, this.vel.z);
    const moveAmt = still ? 0 : clamp(hs / WALK, 0, 1.4) * (this.onGround ? 1 : 0.3);
    const a = this.aim;
    // sway follows the mouse a little behind
    this.sway.x = damp(this.sway.x, clamp(-this.lookDelta.x * 0.0009, -0.04, 0.04), 10, dt || 0.016);
    this.sway.y = damp(this.sway.y, clamp(this.lookDelta.y * 0.0009, -0.04, 0.04), 10, dt || 0.016);
    this.lookDelta.set(0, 0);
    const bobX = Math.sin(this.bob * Math.PI) * 0.012 * moveAmt * (1 - a * 0.85);
    const bobY = -Math.abs(Math.cos(this.bob * Math.PI)) * 0.012 * moveAmt * (1 - a * 0.85);
    const breathe = still ? 0 : Math.sin(this.e.time * 1.6) * 0.0016 * (1 - a);
    // ADS: the sight's aim point on the view axis
    const hip = this.hipOffset;
    // the aim socket on the view axis, adsOffset.z ahead of the eye
    const ads = new THREE.Vector3(-this.aimLocal.x, -this.aimLocal.y, this.adsOffset.z - this.aimLocal.z);
    const p = hip.clone().lerp(ads, a);
    p.x += bobX + this.sway.x; p.y += bobY + this.sway.y + breathe;
    p.z += this.kick * 0.06;
    this.gunPivot.position.copy(p);
    // a slight cant at the hip
    const reload = this.reloading > 0 ? Math.sin(clamp(1 - this.reloading / 1.9, 0, 1) * Math.PI) : 0;
    // at the hip the muzzle rises and turns in a little toward the crosshair; aiming squares it up
    this.gunPivot.rotation.set(
      (0.025 + this.recoil.x * 0.05 + reload * 0.5) * (1 - a) + this.sway.y * 1.5,
      (0.05 + this.recoil.y * 0.03) * (1 - a) + this.sway.x * 1.5,
      (0.03 * (1 - a) + reload * 0.35), 'YXZ');
  }

  state() {
    return {
      yawDeg: this.yaw, pos: [this.pos.x, this.pos.z], health: this.health, maxHealth: this.maxHealth, level: this.level,
      xp: this.xp, xpNext: this.xpNext, mag: this.mag, magMax: this.magMax, reserve: this.reserve, slot: 0,
      slots: [{ icon: 'rifle', rarity: 3, name: 'WHISPER BULLPUP', fireMode: 'AUTO' }, { icon: 'shotgun', rarity: 1, name: 'RANCHHAND', fireMode: 'PUMP' }, null],
      reloading: this.reloading > 0, lowAmmo: this.mag <= 6, reloadSeconds: 1.9,
    };
  }
}
