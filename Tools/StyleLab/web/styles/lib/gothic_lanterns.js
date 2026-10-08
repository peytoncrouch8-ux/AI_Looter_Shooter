// Ember Gothic's lanterns: iron lamp posts down Main Street, one at the chapel yard's gate and the gravekeeper's on
// boot hill, each a lit lamp the engine's light pool can pick (so the nearest six cast warm pools), and Pa's lantern
// at the player's hip (a light that rides with the camera, so it is right in stills too).
// In Unreal: a lamp-post actor (static mesh + point light, no shadows on Medium, 6-8 m attenuation) placed by the
// area build along the street edges; the hip lantern is a point light on the character.

// [x, ground y, z] in three metres (heights from the export's heights.bin), and the side the arm reaches toward
const POSTS = [
  [-7, 0.15, 7.0, -1], [-21, 0.1, -7.0, 1], [-37, 0.4, 7.5, -1], [-47, 0.45, -7.5, 1], [-58, 0.18, 7.0, -1],
  [3, -0.06, -6.5, 1],
  [15.4, 7.97, 55.7, 1],       // the chapel yard's gate
  [0.5, 3.31, 44.5, -1],       // the gravekeeper's lantern among the graves
  [-17, 0.28, 22, 1], [-8, 2.98, 33, -1], [8, 5.78, 51, 1],   // the road from town up past boot hill to the chapel
  [-12, 0.37, 20, -1], [-23, 0.34, 19, 1],                       // the back lots at the town's north edge
];

import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';

let shared = null;
let merged = null;     // the posts merged into one mesh per material (built once: three draws instead of ~150)

function materials(kit) {
  if (shared) return shared;
  const iron = kit.pbr({ name: 'GothicLanternIron', master: 'World', set: null, color: '#1c1b1a', roughness: 0.45, metallic: 0.85, role: 'metal' },
    { envIntensity: 0.6 });
  const wood = kit.pbr({ name: 'GothicLanternPost', master: 'World', set: 'WoodPlanks', color: '#3a3026', roughness: 0.85, metallic: 0, role: 'wood' },
    { saturation: 0.5, value: 0.55 });
  const glass = kit.pbr({ name: 'GothicLanternGlass', master: 'World', set: null, color: '#ffcf8a', roughness: 0.3, metallic: 0, role: 'glow',
    glow: 9, glowColor: '#ffb162' }, {});
  shared = { iron, wood, glass };
  return shared;
}

function lanternMesh(kit, m) {
  const THREE = kit.THREE;
  const g = new THREE.Group();
  const box = (w, h, d, mat, x, y, z) => {
    const b = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
    b.position.set(x, y, z); b.castShadow = true; b.receiveShadow = true;
    g.add(b);
    return b;
  };
  // the cage: four corner bars, a cap and a base around a glowing chimney of glass
  box(0.15, 0.24, 0.15, m.glass, 0, 0, 0).castShadow = false;
  for (const [sx, sz] of [[1, 1], [1, -1], [-1, 1], [-1, -1]]) box(0.022, 0.3, 0.022, m.iron, sx * 0.085, 0, sz * 0.085);
  box(0.24, 0.04, 0.24, m.iron, 0, -0.15, 0);
  const cap = new THREE.Mesh(new THREE.ConeGeometry(0.18, 0.14, 4), m.iron);
  cap.rotation.y = Math.PI / 4; cap.position.y = 0.22; cap.castShadow = true;
  g.add(cap);
  box(0.03, 0.12, 0.03, m.iron, 0, 0.33, 0);
  return g;
}

// Builds the posts under kit.styleRoot and returns the lamp records to add to kit.lights.
export function buildLanterns(kit, opts = {}) {
  const THREE = kit.THREE;
  const m = materials(kit);
  const lamps = [];
  const height = opts.height ?? 2.75;
  const parts = new THREE.Group();
  POSTS.forEach(([x, y, z, side], i) => {
    const post = new THREE.Group();
    post.position.set(x, y, z);
    const pole = new THREE.Mesh(new THREE.BoxGeometry(0.13, height, 0.13), m.wood);
    pole.position.y = height / 2; pole.castShadow = true; pole.receiveShadow = true;
    post.add(pole);
    const arm = new THREE.Mesh(new THREE.BoxGeometry(0.06, 0.06, 0.62), m.iron);
    arm.position.set(0, height - 0.12, side * 0.28); arm.castShadow = true;
    post.add(arm);
    const brace = new THREE.Mesh(new THREE.BoxGeometry(0.04, 0.42, 0.04), m.iron);
    brace.position.set(0, height - 0.32, side * 0.16); brace.rotation.x = side * 0.75; brace.castShadow = true;
    post.add(brace);
    const lantern = lanternMesh(kit, m);
    lantern.position.set(0, height - 0.48, side * 0.52);
    post.add(lantern);
    parts.add(post);
    const p = new THREE.Vector3(x, y + height - 0.48, z + side * 0.52);
    lamps.push({ p, color: kit.color(opts.color || '#ffad5c'), intensity: opts.intensity ?? 10, range: opts.range ?? 9,
      note: 'lantern (Ember Gothic)', phase: i * 2.3, scale: 1, colorOverride: null });
  });
  if (!merged) {
    parts.updateMatrixWorld(true);
    const buckets = new Map();
    parts.traverse((o) => {
      if (!o.isMesh) return;
      const g = o.geometry.clone().applyMatrix4(o.matrixWorld);
      if (!buckets.has(o.material)) buckets.set(o.material, []);
      buckets.get(o.material).push(g);
    });
    merged = [];
    for (const [mat, list] of buckets) {
      const mesh = new THREE.Mesh(mergeGeometries(list, false), mat);
      mesh.castShadow = mat !== m.glass; mesh.receiveShadow = true;
      mesh.name = 'GothicLanterns_' + mat.name;
      merged.push(mesh);
    }
  }
  for (const mesh of merged) kit.styleRoot.add(mesh);
  return lamps;
}

// Pa's lantern: a warm light at the hip, a child of the camera.
export function hipLantern(kit, o = {}) {
  const THREE = kit.THREE;
  const l = new THREE.PointLight(kit.color(o.color || '#ffb36a'), o.intensity ?? 6, o.distance ?? 14, 2);
  l.position.set(o.x ?? 0.32, o.y ?? -0.85, o.z ?? -0.35);
  l.layers.enable(1);      // the gun's layer too, so the lantern warms the gun
  l.userData.fogScale = o.fogScale ?? 0.35;
  l.name = 'PaLantern';
  return l;
}
