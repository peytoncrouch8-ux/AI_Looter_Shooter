"""A barn cat for Ransom's Rest's porches (the 2026-10-14 "make it alive" polish; look sheet 44): a lean brown mackerel
tabby, as a small rig the game poses in code (UCreaturePoseAnimInstance), so a porch has someone home while the townsfolk
stay unseen. Stylized realism like Hob and the hens (Tools/Blender/looter_animals.py): one smooth skin from the nose to
the rump cast onto a soft sculpt (with the shoulder and thigh muscles the legs grow out of), slim legs and the tail as
smooth tubes, cupped ears, almond eyes with slit pupils, a leather nose, whiskers and a few tufts of fur where the
silhouette wants them (cheeks, chest, belly, hocks).
Refined the same day ("slightly more realistic"): a longer, leaner trunk, slimmer legs and paws, bigger ears, a finer
neck, a ticked and combed coat, and the sit and loaf fitted as a real cat holds them.

Model (/Game/Art/Creatures; skeletal; no hit zones, so shots pass through it, as through Hob):
  SK_BarnCat    25 cm at the withers, 40 cm nose to rump and a 25 cm tail, 5,170 triangles (built a size up in the
                numbers below and made at SCALE; the pose table is at the made size)
One material slot, MI_CatAtlas (the World master), on the texture set Art/Textures/CatAtlas (2048 x 1024 px: color,
normal, occlusion-roughness-metal), painted by this script every run (deterministic): the ticked agouti coat, the
mackerel stripes, the dorsal line, the forehead's M, the cheek lines, the pale chin, chest and belly, the ringed tail
with its dark tip, the barred legs.

Pivot: the ground under the middle of the body; the front faces Blender -Y (Unreal +X). LODs 50% and 25%.

Bones (parent):
  pelvis (root bone's child; at the hips), spine_1 (pelvis; the loins), spine_2 (spine_1; the ribs and shoulders),
  neck (spine_2), head (neck), ear_l, ear_r (head)
  tail_1 .. tail_6 (a chain from the pelvis)
  per side l/r: scapula (spine_2; the shoulder blade, top to the shoulder joint), upperarm (scapula; shoulder to elbow),
    forearm (upperarm; elbow to wrist), forepaw (forearm; wrist to toes)
  per side l/r: thigh (pelvis; hip to knee), shin (thigh; knee to hock), foot (shin; hock to the ball of the foot, the
    long metatarsus), hindpaw (foot; the toes)
The skin over the shoulders and thighs is weighted partly to the upper arm and thigh, so the haunch folds with the leg.
The pose table (stand, walk_a/walk_b, sit, loaf, loaf_watch, stretch, slink), in Unreal's component space as Amos's
(Tools/amos_poses.py's format), goes to Intermediate/AnimalsModel/Cat_poses.json. The legs are planted with the game's
two-bone IK (front: upperarm and forearm, elbow back; hind: thigh and shin, knee forward, then the foot to the ball).

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Cat.py -Preview      (renders into Saved/ArtPreviews/RansomsRest/Animals)
    -ScriptArgs check     the rest pose from four sides (work only); check,pose=sit one pose; previews=<folder> renders
    into a folder of Animals
"""
import math

import numpy as np
from mathutils import Matrix, Vector

import looter_animals as la
import looter_creatures as lc
import looter_textures as lt

ARGV = la.args()
PREVIEW = '--preview' in ARGV or lt.want_preview()
SET = 'CatAtlas'
# The shapes below were drawn a size up; the cat is made at this share of them (25 cm at the withers, a barn cat's).
SCALE = 0.84
SIZE = (2048, 1024)
PX_M, NORMAL_STRENGTH = 0.0004, 1.0
ATLAS = {
    'skin': (0.0, 0.5, 0.0, 1.0), 'leg_f': (0.5, 0.75, 0.5, 1.0), 'leg_h': (0.75, 1.0, 0.5, 1.0),
    'tail': (0.5, 1.0, 0.25, 0.5), 'paw': (0.5, 0.625, 0.125, 0.25), 'ear_out': (0.625, 0.6875, 0.125, 0.25),
    'ear_in': (0.6875, 0.75, 0.125, 0.25), 'fur': (0.75, 1.0, 0.125, 0.25), 'eye': (0.5, 0.5625, 0.0, 0.125),
    'nose': (0.5625, 0.625, 0.0, 0.125), 'whisker': (0.625, 0.6875, 0.0, 0.125),
}
FUR = ('agouti', 'pale', 'dark', 'cheek')


def fur_strip(kind):
    i = FUR.index(kind)
    return (i / 4.0, (i + 1) / 4.0)


# --- The shape (creature space: cm, x forward, y left, z up, the origin on the ground under the body) ---

def X(x):
    """A lean barn cat is long in the body: everything behind the ribs' middle (x = 3) is drawn 16% longer than the
    first, compact cat was."""
    return x if x >= 3.0 else 3.0 + (x - 3.0) * 1.16


HIND_DX = X(-12.6) + 12.6        # the hind legs move back with the hips, whole (not stretched)
TAIL_DX = X(-17.6) + 17.6


def back(p, dx):
    return (p[0] + dx, p[1], p[2])


def H(p):
    """The head, carried a little higher and further forward than the first cat's, as a cat holds it."""
    return (p[0] + 0.5, p[1], p[2] + 2.0)


SCULPT = [((X(-13.6), 0.0, 22.0), (6.6, 5.0, 5.8), (0.0, -4.0, 0.0)),     # the hips
          ((X(-5.0), 0.0, 21.4), (10.4, 4.9, 5.4), (0.0, 0.0, 0.0)),     # the loins, the belly tucked up
          ((4.6, 0.0, 20.6), (8.4, 5.7, 7.2), (0.0, 4.0, 0.0)),        # the ribs, deep at the chest
          ((8.2, 0.0, 24.6), (5.2, 5.0, 4.4), (0.0, 0.0, 0.0)),        # the withers
          ((11.0, 0.0, 17.9), (3.8, 4.1, 4.8), (0.0, 0.0, 0.0)),       # the breast
          (H((18.8, 0.0, 29.7)), (3.9, 3.8, 3.5), (0.0, 0.0, 0.0)),    # the skull
          (H((19.8, 0.0, 27.8)), (3.0, 4.0, 2.6), (0.0, 0.0, 0.0)),    # the cheeks
          (H((22.6, 0.0, 27.3)), (2.25, 2.0, 1.7), (0.0, 10.0, 0.0)),  # the muzzle
          (H((21.8, 0.0, 25.9)), (1.8, 1.4, 1.0), (0.0, 0.0, 0.0)),    # the chin
          (H((21.5, 0.0, 29.0)), (2.4, 1.8, 1.25), (0.0, 4.0, 0.0))]   # the brow and the bridge of the nose
# The muscles the legs grow out of, each side: the shoulder blade and upper arm, leaning back at the top; the thigh,
# from the hip down to the knee at the bottom of the flank (the haunch).
for _side in (1.0, -1.0):
    SCULPT += [((8.6, _side * 3.5, 19.0), (3.3, 1.95, 5.4), (0.0, -21.0, 0.0)),
               ((X(-12.6) + 2.8, _side * 3.8, 16.3), (4.6, 2.6, 6.0), (0.0, -34.0, 0.0))]
NECK = ((11.0, 0.0, 23.2), (14.0, 0.0, 26.8), H((16.2, 0.0, 27.9)))
NECK_R = (4.6, 4.1, 3.7)
SPINE = (H((24.2, 0.0, 27.4)), H((22.0, 0.0, 28.3)), H((19.4, 0.0, 29.3)), H((16.6, 0.0, 28.7)), (14.1, 0.0, 27.3),
         (10.6, 0.0, 22.8), (5.0, 0.0, 21.0), (X(-4.0), 0.0, 21.2), (X(-12.0), 0.0, 22.0), (X(-18.4), 0.0, 22.2))
# Where along the spine (t: 0 the nose, 1 the rump) each bone takes over the skin.
SKIN_BONES = (('head', 0.0), ('neck', 0.36), ('spine_2', 0.52), ('spine_1', 0.74), ('pelvis', 0.9))
HEAD_CENTER = Vector(H((19.6, 0.0, 28.6)))

# The legs: joints, then the tube's centre line and radii (front-back, side to side), slim as a lean cat's.
FRONT = dict(scapula=(7.4, 3.4, 26.2), shoulder=(10.2, 4.0, 18.8), elbow=(6.8, 4.2, 12.2), wrist=(8.0, 3.7, 2.4),
             toes=(10.6, 3.6, 0.4),
             tube=(((10.4, 3.7, 21.0), (2.5, 1.9)), ((10.0, 3.9, 18.4), (2.35, 1.85)), ((8.5, 4.05, 15.2), (1.95, 1.65)),
                   ((6.9, 4.1, 12.2), (1.58, 1.4)), ((7.15, 3.95, 9.2), (1.4, 1.22)), ((7.6, 3.8, 5.4), (1.12, 1.02)),
                   ((8.0, 3.7, 2.8), (1.0, 0.95)), ((8.3, 3.68, 1.3), (0.96, 0.92))),
             paw=((9.0, 3.65, 0.62), (1.75, 1.3, 0.95)))
HIND = dict(hip=back((-12.6, 4.2, 20.2), HIND_DX), knee=back((-7.0, 4.6, 12.0), HIND_DX),
            hock=back((-15.0, 4.1, 7.2), HIND_DX), ball=back((-12.4, 3.8, 1.2), HIND_DX), toes=back((-9.6, 3.7, 0.4), HIND_DX),
            tube=tuple((back(p, HIND_DX), r) for p, r in (
                ((-12.6, 3.3, 22.0), (3.4, 2.5)), ((-11.8, 3.7, 19.0), (3.4, 2.5)), ((-9.6, 4.1, 15.2), (2.65, 2.15)),
                ((-7.2, 4.3, 12.0), (1.7, 1.5)), ((-10.6, 4.2, 9.9), (1.4, 1.18)), ((-14.7, 4.05, 7.4), (1.02, 0.88)),
                ((-13.6, 3.9, 4.2), (0.9, 0.8)), ((-12.6, 3.8, 1.6), (0.86, 0.82)), ((-12.2, 3.78, 0.9), (0.84, 0.8)))),
            paw=(back((-11.2, 3.75, 0.6), HIND_DX), (1.85, 1.3, 0.92)))
TAIL = tuple(back(p, TAIL_DX) for p in ((-17.6, 0.0, 23.2), (-21.0, 0.0, 22.0), (-24.4, 0.0, 19.4), (-27.4, 0.0, 16.0),
                                        (-30.0, 0.0, 12.6), (-32.6, 0.0, 9.6), (-35.6, 0.0, 7.6), (-39.0, 0.0, 7.0),
                                        (-42.0, 0.0, 7.8)))
TAIL_R = (1.75, 1.6, 1.48, 1.42, 1.38, 1.34, 1.3, 1.24, 0.95)
EARS = dict(base=H((18.0, 3.0, 31.9)), front=(0.82, 0.5, 0.05), up=(-0.2, 0.4, 1.0), width=4.5, height=4.8, depth=1.3)


def bone_specs():
    V = Vector
    specs = [('pelvis', V((X(-13.0), 0.0, 21.8)), V((X(-5.0), 0.0, 21.6)), None),
             ('spine_1', V((X(-5.0), 0.0, 21.6)), V((3.0, 0.0, 22.2)), 'pelvis'),
             ('spine_2', V((3.0, 0.0, 22.2)), V((10.4, 0.0, 23.0)), 'spine_1'),
             ('neck', V(NECK[0]), V(NECK[2]), 'spine_2'),
             ('head', V(NECK[2]), V(H((24.0, 0.0, 28.2))), 'neck')]
    for side, sfx in la.SIDES:
        b = V((EARS['base'][0], side * EARS['base'][1], EARS['base'][2]))
        up = V((EARS['up'][0], side * EARS['up'][1], EARS['up'][2])).normalized()
        specs.append((f'ear_{sfx}', b, b + up * EARS['height'], 'head'))
    joints = [V(p) for p in TAIL]
    marks = (0, 1.5, 3.0, 4.5, 6.0, 7.2, 8.0)
    for k in range(6):
        a, c = marks[k], marks[k + 1]
        specs.append((f'tail_{k + 1}', point_on(joints, a), point_on(joints, c), 'pelvis' if k == 0 else f'tail_{k}'))
    for side, sfx in la.SIDES:
        m = lambda p: V((p[0], side * p[1], p[2]))
        specs += [(f'scapula_{sfx}', m(FRONT['scapula']), m(FRONT['shoulder']), 'spine_2'),
                  (f'upperarm_{sfx}', m(FRONT['shoulder']), m(FRONT['elbow']), f'scapula_{sfx}'),
                  (f'forearm_{sfx}', m(FRONT['elbow']), m(FRONT['wrist']), f'upperarm_{sfx}'),
                  (f'forepaw_{sfx}', m(FRONT['wrist']), m(FRONT['toes']), f'forearm_{sfx}'),
                  (f'thigh_{sfx}', m(HIND['hip']), m(HIND['knee']), 'pelvis'),
                  (f'shin_{sfx}', m(HIND['knee']), m(HIND['hock']), f'thigh_{sfx}'),
                  (f'foot_{sfx}', m(HIND['hock']), m(HIND['ball']), f'shin_{sfx}'),
                  (f'hindpaw_{sfx}', m(HIND['ball']), m(HIND['toes']), f'foot_{sfx}')]
    return specs


def point_on(points, f):
    """A point f of the way along a polyline's joints (f = joint index, fractional)."""
    k = min(int(f), len(points) - 2)
    return points[k].lerp(points[k + 1], f - k)


def seg_dist(p, a, b):
    """How far a point is from the segment a-b."""
    ab = b - a
    t = min(max((p - a).dot(ab) / max(ab.length_squared, 1e-9), 0.0), 1.0)
    return (p - (a + ab * t)).length


def skin_weights(t, p):
    """The skin's bones by how far along the spine, blended over a few percent at each boundary; over the shoulders and
    the thighs a share goes to the upper arm and the thigh, so the haunch and the point of the shoulder move with the
    leg instead of the leg sliding out of a still body."""
    out = {}
    for k, (bone, start) in enumerate(SKIN_BONES):
        end = SKIN_BONES[k + 1][1] if k + 1 < len(SKIN_BONES) else 2.0
        w = la.smooth(start - 0.04, start + 0.04, t) * (1.0 - la.smooth(end - 0.04, end + 0.04, t)) if k else \
            1.0 - la.smooth(end - 0.04, end + 0.04, t)
        if w > 1e-3:
            out[bone] = w
    p = Vector(p)
    extra = {}
    for side, sfx in la.SIDES:
        if p.y * side < 0.8:
            continue
        m = lambda q: Vector((q[0], side * q[1], q[2]))
        thigh = 0.45 * la.smooth(4.4, 2.2, seg_dist(p, m(HIND['hip']), m(HIND['knee'])))
        arm = 0.35 * la.smooth(3.6, 1.8, seg_dist(p, m(FRONT['shoulder']), m(FRONT['elbow'])))
        blade = 0.25 * la.smooth(3.6, 2.0, seg_dist(p, m(FRONT['scapula']), m(FRONT['shoulder'])))
        for bone, w in ((f'thigh_{sfx}', thigh), (f'upperarm_{sfx}', arm), (f'scapula_{sfx}', blade)):
            if w > 1e-3:
                extra[bone] = w
    total = sum(extra.values())
    if total > 0.75:
        extra = {b: w * 0.75 / total for b, w in extra.items()}
        total = 0.75
    out = {b: w * (1.0 - total) for b, w in out.items()}
    out.update(extra)
    return out


# --- Building ---

class Cat:
    def __init__(self):
        self.part = la.Part('BarnCat', ATLAS, SIZE)
        elements = [la.element(c, s, r) for c, s, r in SCULPT]
        n = [Vector(p) for p in NECK]
        for t in (0.0, 0.25, 0.5, 0.75, 1.0):
            k = min(int(t * 2), 1)
            c = n[k].lerp(n[k + 1], t * 2 - k)
            r = la.lerp(NECK_R[0], NECK_R[2], t)
            elements.append(la.element(c, (r, r * 0.95, r)))
        self.sculpt = la.Sculpt(elements, resolution=0.4)
        self.build_skin()
        self.build_head()
        self.build_ears()
        self.build_legs()
        self.build_tail()
        self.build_tufts()

    def build_skin(self):
        S = 38
        ts = [i / (S - 1) for i in range(S)]

        def up_at(t):
            return Vector((0.1, 0.0, 1.0)) if t < 0.3 else la.UP
        self.skin = la.cast_skin(self.part, self.sculpt, SPINE, ts, around=22, up_at=up_at, weights_at=skin_weights,
                                 region='skin', cap_push=0.15, reach=2.6)
        self.field = la.WeightField(self.part)

    def weigh(self, point_fn):
        return lambda t: self.field.at(point_fn(t, 0.0, 0))

    def build_head(self):
        part = self.part
        head = {'head': 1.0}
        # Eyes: almond, set forward in the face under the brow, only their front showing.
        for side, sfx in la.SIDES:
            d = Vector((0.9, side * 0.47, 0.12)).normalized()
            hit, normal = self.sculpt.cast(HEAD_CENTER, d)
            normal = (normal if normal is not None else d).lerp(d, 0.4).normalized()
            center = hit - normal * 0.42
            up = (la.UP - normal * la.UP.dot(normal)).normalized()
            rot = Matrix((normal, up.cross(normal), up)).transposed()
            la.ellipsoid(part, center, (0.8, 0.86, 0.62), 'eye', rot=rot, segs=12, rings=7, weights=head)
            if side > 0:
                self.eye_at = hit
        # The nose leather: a soft triangle at the front of the muzzle.
        hit, normal = self.sculpt.cast(H((21.8, 0.0, 27.7)), (1.0, 0.0, 0.05))
        self.nose_at = hit
        rot = normal.to_track_quat('X', 'Z').to_matrix()
        la.ellipsoid(part, hit + normal * 0.05, (0.42, 0.66, 0.46), 'nose', rot=rot, segs=8, rings=5, weights=head,
                     deform=lambda a, s, u: (a, s * (0.55 + 0.45 * la.smooth(-1.0, 0.8, u)), u))
        # Whiskers: five a side from the pads, sweeping out and back, the lower ones drooping.
        pad = Vector(H((22.4, 0.0, 26.8)))
        for side, _ in la.SIDES:
            for k in range(5):
                d = Vector((0.25, side * 1.0, 0.25 - 0.14 * k)).normalized()
                root, normal = self.sculpt.cast(pad, d)
                if root is None:
                    continue
                L = 5.8 - 0.25 * abs(k - 1.5)
                out = Vector((-0.05 - 0.05 * k, side * 1.0, 0.1 - 0.12 * k)).normalized()
                pts = [root - normal * 0.1, root + out * L * 0.5 + Vector((-0.4, 0.0, 0.0)),
                       root + out * L + Vector((-1.4, 0.0, -0.35 - 0.12 * k))]
                la.tube(part, pts, [0.05, 0.038, 0.03], 'whisker', sides=3, weights=head, caps=(False, True),
                        cap_push=0.5)
        self.chin_at, _ = self.sculpt.cast(H((21.2, 0.0, 26.6)), (0.3, 0.0, -1.0))
        self.top_at, _ = self.sculpt.cast(H((19.0, 0.0, 28.3)), (0.0, 0.0, 1.0))

    def build_ears(self):
        """Cupped triangles, big as a cat's are: a furred back, a pale inside, thin at the rim and the tip; tufts
        inside."""
        part = self.part
        cols, rows = 7, 5
        for side, sfx in la.SIDES:
            base = Vector((EARS['base'][0], side * EARS['base'][1], EARS['base'][2]))
            up = Vector((EARS['up'][0], side * EARS['up'][1], EARS['up'][2])).normalized()
            front = Vector((EARS['front'][0], side * EARS['front'][1], EARS['front'][2]))
            front = (front - up * front.dot(up)).normalized()
            across = up.cross(front)
            W, H, D = EARS['width'], EARS['height'], EARS['depth']

            def point(u, v, layer):
                # A little convex along its edges, the tip rounded, the cup deepest low in the middle.
                half = 0.5 * W * (1.0 - v) ** 0.8 * (1.0 + 0.1 * math.sin(math.pi * v))
                cup = D * (1.0 - u * u) * (1.0 - v) ** 0.6
                # (Never quite zero: the rim joining the two sides keeps a real edge, not faces without area.)
                thick = 0.22 * (1.0 - v) * (1.0 - abs(u) ** 3) + 0.08
                p = base + across * (u * half) + up * (v * H) - front * (cup - 0.4)
                return p - front * (thick if layer else 0.0)
            ear = f'ear_{sfx}'
            for layer, region, out_sign in ((0, 'ear_in', 1.0), (1, 'ear_out', -1.0)):
                grid = []
                for r in range(rows):
                    v = r / rows
                    row = []
                    for c in range(cols):
                        u = -1.0 + 2.0 * c / (cols - 1)
                        w = {ear: la.smooth(0.0, 0.35, v), 'head': 1.0 - la.smooth(0.0, 0.35, v)}
                        row.append((part.vert(point(u, v, layer), w), u, v))
                    grid.append(row)
                tip = part.vert(point(0.0, 1.0, layer), {ear: 1.0})
                uv = lambda u, v: part.at(region, 0.5 + 0.5 * u, v)
                for r in range(rows - 1):
                    for c in range(cols - 1):
                        q = (grid[r][c][0], grid[r][c + 1][0], grid[r + 1][c + 1][0], grid[r + 1][c][0])
                        part.face(q, [uv(grid[r][c][1], grid[r][c][2]), uv(grid[r][c + 1][1], grid[r][c + 1][2]),
                                      uv(grid[r + 1][c + 1][1], grid[r + 1][c + 1][2]), uv(grid[r + 1][c][1], grid[r + 1][c][2])],
                                  out=front * out_sign)
                for c in range(cols - 1):
                    t3 = (grid[-1][c][0], grid[-1][c + 1][0], tip)
                    part.face(t3, [uv(grid[-1][c][1], grid[-1][c][2]), uv(grid[-1][c + 1][1], grid[-1][c][2]), uv(0.0, 1.0)],
                              out=front * out_sign)
                if layer == 0:
                    inner = [row[0][0] for row in grid] + [tip] + [row[-1][0] for row in reversed(grid)]
                else:
                    outer = [row[0][0] for row in grid] + [tip] + [row[-1][0] for row in reversed(grid)]
            # The rim: inside and outside joined round the edge.
            for k in range(len(inner) - 1):
                q = (inner[k], inner[k + 1], outer[k + 1], outer[k])
                mid = sum((v.co for v in q), Vector()) / 4.0
                part.face(q, [part.at('ear_out', 0.05, 0.1), part.at('ear_out', 0.1, 0.12), part.at('ear_out', 0.1, 0.2),
                              part.at('ear_out', 0.05, 0.2)], out=mid - (base + up * H * 0.35))
            # Tufts of pale fur inside the ear, and the furnishings at its base.
            for k in range(4):
                u = (k - 1.5) * 0.3
                root = point(u, 0.1, 0) + front * 0.15
                d = (up + across * u * 0.6 + front * 0.25).normalized()
                line, nfn = la.straight(root, d, front, 1.8 - 0.3 * abs(k - 1.5), curl=0.2)
                la.card(part, 'fur', fur_strip('pale'), (0.0, 0.4, 0.75), line, normal=nfn,
                        weigh=lambda t, e=ear: {e: 1.0}, shape='pointed', W=0.45)

    def build_legs(self):
        """Slim legs as smooth tubes (their centre lines eased through the joints, so a bent knee is round), each
        growing out of its muscle in the sculpt; round paws."""
        part = self.part
        for side, sfx in la.SIDES:
            m = lambda p: Vector((p[0], side * p[1], p[2]))
            ws = [{f'scapula_{sfx}': 1.0}, {f'upperarm_{sfx}': 0.5, f'scapula_{sfx}': 0.5},
                  {f'upperarm_{sfx}': 1.0}, {f'upperarm_{sfx}': 0.5, f'forearm_{sfx}': 0.5}, {f'forearm_{sfx}': 1.0},
                  {f'forearm_{sfx}': 1.0}, {f'forearm_{sfx}': 0.6, f'forepaw_{sfx}': 0.4},
                  {f'forearm_{sfx}': 0.3, f'forepaw_{sfx}': 0.7}]
            # (The right legs' V runs the other way round, so V = 0.75 is the inner side of every leg: it's paler.)
            vr = (0.0, 1.0) if side > 0 else (1.0, 0.0)
            pts, radii, wts = la.path_rings([m(p) for p, _ in FRONT['tube']], [r for _, r in FRONT['tube']], ws, per=3)
            la.tube(part, pts, radii, 'leg_f', sides=10, weights=wts, ref=(1.0, 0.0, 0.0), caps=(True, False), v_range=vr)
            c, r = FRONT['paw']
            la.ellipsoid(part, m(c), r, 'paw', rot=la.turn(side * -4.0, 4.0, 0.0), segs=10, rings=6,
                         weights={f'forepaw_{sfx}': 0.85, f'forearm_{sfx}': 0.15}, deform=paw_shape)
            ws = [{'pelvis': 0.8, f'thigh_{sfx}': 0.2}, {f'thigh_{sfx}': 0.7, 'pelvis': 0.3}, {f'thigh_{sfx}': 1.0},
                  {f'thigh_{sfx}': 0.5, f'shin_{sfx}': 0.5}, {f'shin_{sfx}': 1.0}, {f'shin_{sfx}': 0.5, f'foot_{sfx}': 0.5},
                  {f'foot_{sfx}': 1.0}, {f'foot_{sfx}': 0.6, f'hindpaw_{sfx}': 0.4}, {f'foot_{sfx}': 0.3, f'hindpaw_{sfx}': 0.7}]
            pts, radii, wts = la.path_rings([m(p) for p, _ in HIND['tube']], [r for _, r in HIND['tube']], ws, per=3)
            la.tube(part, pts, radii, 'leg_h', sides=10, weights=wts, ref=(1.0, 0.0, 0.0), caps=(True, False), v_range=vr)
            c, r = HIND['paw']
            la.ellipsoid(part, m(c), r, 'paw', rot=la.turn(side * -3.0, 2.0, 0.0), segs=10, rings=6,
                         weights={f'hindpaw_{sfx}': 0.85, f'foot_{sfx}': 0.15}, deform=paw_shape)

    def build_tail(self):
        joints = [Vector(p) for p in TAIL]
        # Each tail bone's share by how far along the joints (bone k spans marks[k-1]..marks[k]).
        marks = (0, 1.5, 3.0, 4.5, 6.0, 7.2, 8.0)

        def weights(f):
            out = {}
            for k in range(6):
                a, c = marks[k], marks[k + 1]
                mid_prev = (marks[k - 1] + a) * 0.5 if k else -10.0
                mid = (a + c) * 0.5
                mid_next = (c + marks[k + 2]) * 0.5 if k + 2 < len(marks) else 99.0
                w = la.smooth(mid_prev, mid, f) * (1.0 - la.smooth(mid, mid_next, f)) if k else 1.0 - la.smooth(mid, mid_next, f)
                if w > 1e-3:
                    out[f'tail_{k + 1}'] = w
            return out
        ws = [weights(i) for i in range(len(joints))]
        ws[0] = {'pelvis': 0.4, 'tail_1': 0.6}
        pts, radii, wts = la.path_rings(joints, list(TAIL_R), ws, per=2)
        la.tube(self.part, pts, radii, 'tail', sides=10, weights=wts, ref=(0.0, 0.0, 1.0), caps=(True, True),
                cap_push=0.9)

    def build_tufts(self):
        """Fur breaking the silhouette: the cheek ruff, the chest, the belly's fringe, the fronts of the shoulders."""
        part = self.part
        spots = []
        for side, _ in la.SIDES:
            for k in range(4):
                spots.append((H((20.2 - 1.0 * k, 0.0, 27.5 - 0.6 * k)), (-0.2, side, -0.25), (-1.0, side * 0.3, -0.6), 1.8,
                              'cheek'))
            for k in range(5):
                spots.append(((X(5.0 - 4.0 * k), 0.0, 17.6), (0.0, side * 0.5, -1.0), (-0.6, side * 0.2, -1.0), 1.5, 'pale'))
            # Over the shoulders' fronts, where the forelegs come out of the chest.
            for k in range(3):
                spots.append(((9.0, 0.0, 19.5 - 1.6 * k), (0.55, side * 1.0, -0.1), (0.1, side * 0.15, -1.0), 1.8, 'agouti'))
        for i, (origin, d, along, L, kind) in enumerate(spots):
            hit, normal = self.sculpt.cast(origin, d, 30.0)
            if hit is None:
                continue
            a = Vector(along)
            a = (a - normal * a.dot(normal)).normalized()
            j = float(la.hash01(i, 3.0)) - 0.5
            line, _ = la.straight(hit, a, normal, L * (1.0 + 0.3 * j))
            point, nfn = la.hug(line, self.sculpt, lambda t: 0.05 + 0.45 * t * t)
            la.card(part, 'fur', fur_strip(kind), (0.0, 0.4, 0.75), point, normal=nfn, weigh=self.weigh(point),
                    shape='pointed', W=1.2 * (1.0 - 0.3 * j))

    def landmarks(self):
        skin = self.skin
        return dict(eye=la.skin_landmark(skin, self.eye_at), nose=la.skin_landmark(skin, self.nose_at),
                    chin=la.skin_landmark(skin, self.chin_at), top=la.skin_landmark(skin, self.top_at),
                    ear=la.skin_landmark(skin, Vector(EARS['base'])),
                    nape=la.skin_landmark(skin, Vector(H((15.4, 0.0, 30.4)))),
                    withers=la.skin_landmark(skin, Vector((8.0, 0.0, 28.6))),
                    rump=la.skin_landmark(skin, Vector((X(-18.0), 0.0, 24.6))))

    def finish(self, material):
        obj = self.part.finish(material, scale=SCALE)
        self.rig = la.Rig(bone_specs(), scale=SCALE)
        self.rig.bind(obj)
        lt.bake_vertex_ao(obj, samples=16, distance=0.06, ground=True)
        self.rig.arm['LODs'] = '50,25'
        self.rig.arm['LODScreens'] = '0.1,0.04'
        self.obj = obj
        return obj


def paw_shape(a, s, u):
    """A cat's round paw: flat underneath, the toes swelling at the front."""
    if u < 0.0:
        u *= 0.45
    toes = 0.08 * max(0.0, a) * (0.5 + 0.5 * math.cos(s * 7.5))
    return a * (1.0 + 0.05 * max(0.0, u)), s, u + toes * max(0.0, u + 0.3)


# --- The atlas ---

rgb = lt.rgb
MARKS = {}


def coat(shape, seed, along_axis=0, fine=1.0):
    """The agouti ground: every hair banded dark and light, so the coat is finely ticked; combed along its lie, in
    soft locks, with a warm-cool breakup across the body (never a flat colour, never a wood grain). fine (a number or
    a mask) scales the hairs' contrast: short fur, as on the face, shows them less. Returns the colour and the fur's
    relief (for the normals and for combing the markings' edges)."""
    tick = la.strand_noise(shape, seed, along_axis, 0.6, 3.5)        # single hairs: light tips, dark bands
    lock = la.strand_noise(shape, seed + 1, along_axis, 2.2, 10.0)   # locks of fur lying together
    broad = lt.noise(shape, seed + 2, 40.0, 40.0, octaves=2)
    c = lt.mix(rgb(0x6c5843), rgb(0x9c8664), lt.smooth(-1.5, 1.5, 0.45 * lock + 0.65 * broad))
    c = lt.mix(c, rgb(0x8c6a46), lt.smooth(0.2, 1.6, lt.noise(shape, seed + 3, 60.0, 60.0)) * 0.35)
    c = c * (1.0 + (0.13 * tick + 0.04 * lock) * fine)[..., None]
    return c, (0.6 * lock + 0.4 * tick) * fine


def paint_skin(U, V, seed):
    shape = U.shape
    mk = MARKS
    d, s = lc.body_axes(U, V)
    eye, nose, top, ear, nape, withers, rump = (mk[k] for k in ('eye', 'nose', 'top', 'ear', 'nape', 'withers', 'rump'))
    cm_v = eye['cm_v']
    head = lt.smooth(nape['v'] - 0.01, nape['v'] + 0.02, V)
    c, hair = coat(shape, seed, fine=1.0 - 0.55 * head)
    comb = la.strand_noise(shape, seed + 7, 0, 0.9, 7.0)            # combs the markings' edges along the hair

    # Distances in cm on the head, from the landmarks.
    def dist(lm, u_scale=1.0, mirror=True):
        du = np.abs(U - lm['u'])
        if mirror:
            du = np.minimum(du, np.abs(U - (1.0 - lm['u'])))
        return du * lm['cm_u'] * u_scale, (V - lm['v']) * cm_v
    # The pale underside: belly, chest, throat, chin, its edge feathered into the coat; faint spots on the belly.
    under = lt.smooth(-0.26, -0.7, d + 0.025 * comb)
    pale = lt.mix(rgb(0xd6c8aa), rgb(0xe2d6bc), lt.smooth(-0.8, -1.0, d))
    c = lt.mix(c, pale * (1.0 + 0.05 * hair)[..., None], under * 0.88)
    spots = lt.smooth(0.9, 1.6, lt.noise(shape, seed + 11, 9.0, 14.0)) * lt.smooth(-0.55, -0.8, d) * (1.0 - head)
    c = lt.mix(c, rgb(0x6a5440), spots * 0.4)
    # Mackerel stripes round the body from the spine down the sides: narrow, unbroken but wavering, their edges combed
    # by the fur, fading out on the belly.
    wave = 0.006 * np.sin(2.0 * np.pi * U * 3.0) + 0.004 * lt.noise(shape, seed + 3, 30.0, 8.0) + 0.0022 * comb
    period = 2.5 / cm_v
    ph = la.frac((V + wave) / period)
    taper = 0.12 + 0.06 * lt.smooth(-0.3, 0.8, d)                  # wider over the back, thin down the sides
    stripe = 1.0 - lt.smooth(taper, taper + 0.1, np.abs(ph - 0.5))
    broken = lt.smooth(-1.1, 0.0, lt.noise(shape, seed + 4, 12.0, 6.0))
    sides = lt.smooth(-0.45, 0.05, d)
    torso = lt.smooth(rump['v'] - 0.01, rump['v'] + 0.03, V) * lt.smooth(nape['v'] + 0.02, nape['v'] - 0.03, V)
    c = lt.mix(c, rgb(0x2e2319) * (1.0 + 0.1 * hair)[..., None], stripe * broken * sides * torso * 0.82)
    # Necklaces round the throat.
    neck = lt.smooth(withers['v'] - 0.01, withers['v'] + 0.02, V) * (1.0 - head)
    c = lt.mix(c, rgb(0x3a2c20), stripe * neck * 0.55 * lt.smooth(0.3, -0.2, d))
    # The dark line down the back, combed at its edges.
    dorsal = lt.smooth(0.93, 0.98, d + 0.012 * comb) * lt.smooth(rump['v'] - 0.03, rump['v'] + 0.02, V) * (1.0 - head)
    c = lt.mix(c, rgb(0x2a2018), dorsal * 0.8)
    # The head: the M on the forehead, lines back over the crown, the cheek lines, a thin dark rim round each eye with a
    # pale surround, the pale muzzle and chin, the mouth.
    eu, ev = dist(eye)
    eye_r = np.hypot(eu, ev * 0.85)
    c = lt.mix(c, rgb(0xcdbd9e), (1.0 - lt.smooth(0.82, 1.15, eye_r)) * lt.smooth(0.66, 0.76, eye_r) * head * 0.45)
    c = lt.mix(c, rgb(0x1e1712), (1.0 - lt.smooth(0.62, 0.72, eye_r)) * head)
    crown_u = np.abs(U - 0.5) * top['cm_u']
    crown_v = (V - top['v']) * cm_v
    lines = 0.0
    for off, strength in ((0.0, 0.7), (0.62, 1.0), (1.25, 0.6)):
        x = crown_u - off - 0.1 * np.maximum(crown_v, 0.0) * (off > 0) + 0.03 * comb
        lines = np.maximum(lines, (1.0 - lt.smooth(0.07, 0.16, np.abs(x))) * strength)
    m_zone = lt.smooth(nape['v'] - 0.005, nape['v'] + 0.02, V) * lt.smooth(eye['v'] + 1.4 / cm_v, eye['v'] + 0.7 / cm_v, V)
    c = lt.mix(c, rgb(0x2e231a), lines * m_zone * lt.smooth(0.6, 0.9, d) * 0.7)
    cheek_line = 1.0 - lt.smooth(0.12, 0.26, np.abs((eu - 1.2) * 0.6 + ev * 0.9 + 0.6))
    c = lt.mix(c, rgb(0x2a1f17), cheek_line * lt.smooth(-0.5, 0.1, ev) * lt.smooth(-3.8, -1.0, ev) * head *
               lt.smooth(-0.6, 0.2, d) * 0.7)
    nu, nv = dist(nose, mirror=False)
    muzzle = (1.0 - lt.smooth(1.3, 2.2, np.hypot(nu * 0.7, nv))) * lt.smooth(0.2, -0.3, d + 0.2)
    c = lt.mix(c, rgb(0xe0d4ba), np.clip(muzzle + under * head, 0.0, 1.0) * head * 0.85)
    mouth = (1.0 - lt.smooth(0.05, 0.12, np.abs(nv + 0.9 + 0.25 * nu ** 2))) * lt.smooth(1.4, 0.6, nu) * \
        lt.smooth(-0.1, -0.5, d)
    c = lt.mix(c, rgb(0x3a2a24), mouth * head * 0.8)
    # Shorter, finer fur on the face.
    rough = np.full(shape, 0.84, np.float32) - 0.04 * head
    height = 0.00016 * hair * (1.0 - 0.5 * head)
    occl = 1.0 - 0.12 * lt.smooth(-0.4, -0.9, d)
    return c, rough, height, occl


def paint_leg(U, V, seed, hind):
    """U along the leg (0 the top), V round it (0.5 the front)."""
    shape = U.shape
    c, hair = coat(shape, seed, along_axis=1)
    comb = la.strand_noise(shape, seed + 7, 1, 0.9, 7.0)
    inner = lt.smooth(0.2, 0.04, np.abs(V - 0.75))
    c = lt.mix(c, rgb(0xcfc0a2), np.clip(lt.smooth(0.4, 0.95, U) * 0.25 + inner * 0.55, 0.0, 1.0))
    bars = 1.0 - lt.smooth(0.1, 0.24, np.abs(la.frac(U * (6.0 if hind else 5.0) + 0.04 * lt.noise(shape, seed + 2, 8.0, 8.0)
                                                    + 0.02 * comb) - 0.5))
    c = lt.mix(c, rgb(0x2e231a), bars * (1.0 - inner) * lt.smooth(0.0, 0.2, U) * lt.smooth(0.98, 0.8, U) * 0.72)
    if hind:
        # The dark hock line down the back of the hind leg.
        c = lt.mix(c, rgb(0x2a2018), lt.smooth(0.1, 0.02, np.minimum(V, 1.0 - V)) * lt.smooth(0.55, 0.7, U) * 0.8)
    return c, np.full(shape, 0.84, np.float32), 0.00012 * hair, np.ones(shape, np.float32)


def paint(region, U, V, k):
    shape = U.shape
    seed = 400 + k
    if region == 'skin':
        return paint_skin(U, V, seed)
    if region in ('leg_f', 'leg_h'):
        return paint_leg(U, V, seed, region == 'leg_h')
    if region == 'tail':
        c, hair = coat(shape, seed, along_axis=1)
        comb = la.strand_noise(shape, seed + 7, 1, 0.9, 7.0)
        under = lt.smooth(0.3, 0.05, np.minimum(V, 1.0 - V))
        c = lt.mix(c, rgb(0xb8a688), under * 0.45)
        rings = 1.0 - lt.smooth(0.18, 0.32, np.abs(la.frac(U / 0.1 + 1.2 * U * U + 0.015 * comb) - 0.5))
        c = lt.mix(c, rgb(0x2a2018), rings * lt.smooth(0.05, 0.2, U) * 0.82)
        c = lt.mix(c, rgb(0x1e1712), lt.smooth(0.86, 0.92, U + 0.01 * comb))
        return c, np.full(shape, 0.84, np.float32), 0.00012 * hair, np.ones(shape, np.float32)
    if region == 'paw':
        c, hair = coat(shape, seed)
        d, s = lc.body_axes(U, V)
        c = lt.mix(c, rgb(0xcdbd9e), lt.smooth(0.55, 0.9, V) * 0.5)
        toes = (1.0 - lt.smooth(0.03, 0.08, np.abs(la.frac(s * 2.0 + 0.5) - 0.5))) * lt.smooth(0.6, 0.8, V) * lt.smooth(-0.2, 0.4, d)
        c = lt.mix(c, rgb(0x3a2c22), toes * 0.6)
        pads = lt.smooth(-0.55, -0.8, d)
        c = lt.mix(c, rgb(0x4a3631), pads)
        return c, np.where(pads > 0.5, 0.5, 0.84).astype(np.float32), 0.0001 * hair, 1.0 - 0.3 * pads
    if region == 'ear_out':
        c, hair = coat(shape, seed)
        c = lt.mix(c, rgb(0x2a2018), lt.smooth(0.55, 0.9, V) * 0.65)
        return c, np.full(shape, 0.84, np.float32), 0.0001 * hair, np.ones(shape, np.float32)
    if region == 'ear_in':
        hair = la.strand_noise(shape, seed, 0, 0.8, 6.0)
        c = lt.mix(rgb(0xb08e80), rgb(0xd6c4b0), lt.smooth(0.2, 0.7, np.abs(U - 0.5) * 2.0))
        c = lt.mix(c, rgb(0x6a5248), lt.smooth(0.6, 1.0, V) * 0.5)
        return c * (1.0 + 0.05 * hair)[..., None], np.full(shape, 0.62, np.float32), 0.00008 * hair, \
            1.0 - 0.4 * (1.0 - lt.smooth(0.0, 0.5, V))
    if region == 'fur':
        k4 = np.clip(np.floor(U * 4.0), 0, 3).astype(np.int64)
        x = (la.frac(U * 4.0) - 0.5) * 2.0
        hair = lt.noise(shape, seed, 0.7, 8.0)
        base, _ = coat(shape, seed + 1)
        looks = [base, np.broadcast_to(rgb(0xd9ccb0), shape + (3,)), np.broadcast_to(rgb(0x3a2c20), shape + (3,)),
                 lt.mix(base, rgb(0xcbbd9e), 0.45)]
        c = np.zeros(shape + (3,), np.float32)
        for i in range(4):
            c[k4 == i] = looks[i][k4 == i]
        c = lt.mix(c, c * 0.72, 1.0 - lt.smooth(0.0, 0.4, V))
        c = c * (1.0 + 0.1 * hair)[..., None]
        return c, np.full(shape, 0.86, np.float32), 0.00015 * hair * (1.0 - np.abs(x)), 1.0 - 0.3 * (1.0 - lt.smooth(0.0, 0.4, V))
    if region == 'eye':
        r = 1.0 - V
        ang = 2.0 * np.pi * (U - 0.5)
        horiz = r * np.abs(np.sin(ang))
        n = lt.noise(shape, seed, 2.0, 2.0)
        iris = lt.mix(rgb(0xc2a03a), rgb(0x8a9444), lt.smooth(0.08, 0.36, r))
        iris = lt.mix(iris, iris * 0.8, (0.5 + 0.5 * np.sin(ang * 31.0 + 2.0 * n)) * 0.3)
        c = lt.mix(iris, rgb(0x2a2414), lt.smooth(0.34, 0.42, r))
        pupil = (1.0 - lt.smooth(0.05, 0.075, horiz)) * (1.0 - lt.smooth(0.3, 0.34, r))
        c = lt.mix(c, rgb(0x080706), pupil)
        return c, np.full(shape, 0.05, np.float32), np.zeros(shape, np.float32), np.ones(shape, np.float32)
    if region == 'nose':
        c = lt.mix(rgb(0x94625a), rgb(0xae7a72), lt.smooth(0.3, 0.8, V))
        grain = lt.noise(shape, seed + 1, 1.2, 1.2)
        return c * (1.0 + 0.04 * grain)[..., None], np.full(shape, 0.5, np.float32), 0.00004 * grain, \
            np.ones(shape, np.float32)
    if region == 'whisker':
        return np.broadcast_to(rgb(0xece6da), shape + (3,)).copy(), np.full(shape, 0.5, np.float32), \
            np.zeros(shape, np.float32), np.ones(shape, np.float32)
    raise ValueError(region)


# --- Poses ---

def cat_poses(cat):
    rig = cat.rig
    V = Vector
    rest_wrist = {sfx: V((FRONT['wrist'][0], side * FRONT['wrist'][1], FRONT['wrist'][2])) for side, sfx in la.SIDES}
    rest_ball = {sfx: V((HIND['ball'][0], side * HIND['ball'][1], HIND['ball'][2])) for side, sfx in la.SIDES}
    rest_hock = {sfx: V((HIND['hock'][0], side * HIND['hock'][1], HIND['hock'][2])) for side, sfx in la.SIDES}
    arm_len = (V(FRONT['elbow']) - V(FRONT['shoulder'])).length + (V(FRONT['wrist']) - V(FRONT['elbow'])).length
    foot_len = (V(HIND['ball']) - V(HIND['hock'])).length
    # The underside of the rump (on the hips' sculpt, carried by the pelvis): what a sitting cat sits on.
    hc, hs = Vector(SCULPT[0][0]), SCULPT[0][1]
    rump = [hc + V((hs[0] * math.cos(a), y, hs[2] * math.sin(a))) * 0.92
            for a in (math.radians(d) for d in range(150, 300, 10)) for y in (-2.0, 0.0, 2.0)]

    def front(pose, sfx, wrist, pole=(-1.0, 0.0, 0.0), paw=None):
        rig.plant(pose, f'upperarm_{sfx}', f'forearm_{sfx}', wrist, V(pole))
        if paw is None:
            rig.level(pose, f'forepaw_{sfx}')
        else:
            rig.aim(pose, f'forepaw_{sfx}', paw)
        return pose

    def hind(pose, sfx, ball, hock_offset=None, pole=(1.0, 0.0, 0.2)):
        offset = hock_offset if hock_offset is not None else rest_hock[sfx] - rest_ball[sfx]
        hock = ball + offset
        rig.plant(pose, f'thigh_{sfx}', f'shin_{sfx}', hock, V(pole))
        rig.aim(pose, f'foot_{sfx}', ball - hock)
        rig.level(pose, f'hindpaw_{sfx}')
        return pose

    def stand(pose, moves=None):
        moves = moves or {}
        for _, sfx in la.SIDES:
            front(pose, sfx, rest_wrist[sfx] + moves.get('f' + sfx, V()))
            hind(pose, sfx, rest_ball[sfx] + moves.get('h' + sfx, V()))
        return pose

    def tail(pose, angles):
        for k, a in enumerate(angles):
            pose[f'tail_{k + 1}'] = a
        return pose

    def fit(lo, hi, err, steps=20):
        """The value in lo..hi where err (rising with it) crosses zero."""
        for _ in range(steps):
            mid = (lo + hi) * 0.5
            if err(mid) > 0.0:
                hi = mid
            else:
                lo = mid
        return (lo + hi) * 0.5

    def grounded(pose, points, bone, height, more=()):
        """Moves the pelvis down (or up) until the lowest of points (rest space, carried by bone; more: other (points,
        bone) pairs) is at height."""
        move = list(pose.get('pelvis@move', (0.0, 0.0, 0.0)))
        pose['pelvis@move'] = tuple(move)
        low = min(rig.where(pose, b, p).z for pts, b in [(points, bone)] + list(more) for p in pts)
        move[2] += height - low
        pose['pelvis@move'] = tuple(move)
        return pose
    poses = {'stand': stand({})}
    # The walk's two contacts (a lateral sequence, as cats walk): left fore and right hind reaching, the others pushing.
    for name, lead in (('walk_a', 'l'), ('walk_b', 'r')):
        other = 'r' if lead == 'l' else 'l'
        p = {'pelvis@move': (0.0, 0.0, -0.4), 'spine_2': (3.0 if lead == 'l' else -3.0, 0.0, 0.0),
             'neck': (0.0, 6.0, 0.0), 'head': (0.0, -6.0, 0.0)}
        stand(p, {'f' + lead: V((5.0, 0.0, 0.3)), 'f' + other: V((-5.0, 0.0, 1.6)),
                  'h' + other: V((5.5, 0.0, 0.0)), 'h' + lead: V((-5.5, 0.0, 1.5))})
        # (A tail bone's positive pitch raises it: the tail points back.)
        tail(p, [(0.0, 34.0, 0.0), (0.0, 16.0, 0.0), (0.0, 8.0, 0.0), (0.0, 2.0, 0.0), (0.0, -6.0, 0.0), (0.0, -10.0, 0.0)])
        poses[name] = p
    # Sitting: the rump down on the boards, the body raised, the chest up over the front legs, which stand straight and
    # close together; the hind legs folded beside the body, the hocks on the boards and the hind feet flat beside the
    # front paws; the tail round to the front.
    sit = {'pelvis': (0.0, -44.0, 0.0), 'spine_1': (0.0, -4.0, 0.0), 'neck': (0.0, 0.0, 0.0)}
    grounded(sit, rump, 'pelvis', 0.5)
    # The chest bends down at the shoulders until the front legs reach the boards nearly straight.
    shoulder_h = 0.98 * arm_len + FRONT['wrist'][2]

    def sit_err(b2):
        trial = dict(sit)
        trial['spine_2'] = (0.0, b2, 0.0)
        return shoulder_h - rig.where(trial, 'upperarm_l').z
    sit['spine_2'] = (0.0, fit(-30.0, 60.0, sit_err), 0.0)
    la.log(f'BarnCat sit: pelvis at {rig.where(sit, "pelvis").z:.1f}, chest bent {sit["spine_2"][1]:.1f} degrees, shoulder '
           f'{rig.where(sit, "upperarm_l").z:.1f} (wants {shoulder_h:.1f})')
    rig.aim(sit, 'neck', (0.45, 0.0, 1.0))
    rig.aim(sit, 'head', (1.0, 0.0, -0.12))
    for side, sfx in la.SIDES:
        sh = rig.where(sit, f'upperarm_{sfx}')
        front(sit, sfx, V((sh.x + 0.8, side * 2.5, FRONT['wrist'][2])), pole=(-1.0, 0.0, 0.0))
        hip = rig.where(sit, f'thigh_{sfx}')
        hock = V((hip.x + 0.6, side * 4.6, 1.0))
        ball = hock + V((foot_len - 0.3, -side * 0.4, 0.0))
        hind(sit, sfx, ball, hock_offset=hock - ball, pole=(1.0, side * 0.25, 0.7))
    tail(sit, [(30.0, 46.0, 0.0), (36.0, 14.0, 0.0), (34.0, 4.0, 0.0), (32.0, 0.0, 0.0), (28.0, -4.0, 0.0), (22.0, 0.0, 0.0)])
    poses['sit'] = sit
    # Loafing: lying on its belly in one smooth loaf, the front paws folded back under the chest, the hind legs folded
    # under the haunches, the head up, the tail along its flank.
    # The chest rests on the folded forearms; the body sinks until its underside nearly meets the boards, the haunches
    # pressed against the folded hind legs under it.
    # (The body tips back a little so the deep chest and the shallower hips both come down to the boards.)
    loaf = {'pelvis': (0.0, -7.0, 0.0), 'spine_1': (0.0, 1.0, 0.0)}
    chest = [V((x, y, z)) for x in (6.0, 9.0, 11.0) for y in (-1.5, 0.0, 1.5) for z in (13.2, 13.6)]
    hips = [V((X(x), y, 16.6)) for x in (-16.0, -13.0, -10.0) for y in (-1.5, 0.0, 1.5)]
    grounded(loaf, chest, 'spine_2', 0.8, more=[(hips, 'pelvis')])
    rig.aim(loaf, 'neck', (0.55, 0.0, 0.85))
    rig.aim(loaf, 'head', (1.0, 0.0, -0.1))
    for side, sfx in la.SIDES:
        sh = rig.where(loaf, f'upperarm_{sfx}')
        front(loaf, sfx, V((sh.x + 3.2, side * 1.7, 0.9)), pole=(-1.0, 0.0, -0.3), paw=V((-1.0, 0.0, -0.2)))
        hip = rig.where(loaf, f'thigh_{sfx}')
        hock = V((hip.x - 1.6, side * 3.7, 0.95))
        ball = hock + V((foot_len - 0.2, -side * 0.6, -0.05))
        hind(loaf, sfx, ball, hock_offset=hock - ball, pole=(1.0, side * 0.3, 0.2))
    tail(loaf, [(34.0, 14.0, 0.0), (28.0, 6.0, 0.0), (26.0, 3.0, 0.0), (24.0, 0.0, 0.0), (22.0, 0.0, 0.0), (18.0, 0.0, 0.0)])
    poses['loaf'] = loaf
    watch = dict(loaf)
    watch['ear_l'], watch['ear_r'] = (8.0, 0.0, 0.0), (-8.0, 0.0, 0.0)
    rig.aim(watch, 'neck', (0.75, 0.35, 0.6))
    rig.aim(watch, 'head', (0.6, 0.8, -0.06))
    poses['loaf_watch'] = watch
    # The stretch (a play bow): the front legs out ahead on the boards, the chest down, the hips high, the tail up.
    stretch = {'pelvis@move': (0.0, 0.0, 1.0), 'pelvis': (0.0, 16.0, 0.0), 'spine_1': (0.0, 6.0, 0.0), 'spine_2': (0.0, 4.0, 0.0),
               'neck': (0.0, -20.0, 0.0), 'head': (0.0, -6.0, 0.0), 'ear_l': (0.0, 0.0, -10.0), 'ear_r': (0.0, 0.0, 10.0)}
    for side, sfx in la.SIDES:
        sh_s = rig.where(stretch, f'upperarm_{sfx}')
        front(stretch, sfx, V((sh_s.x + 11.0, side * 3.4, 1.3)), pole=(-1.0, 0.0, 0.6), paw=V((1.0, 0.0, -0.05)))
        hind(stretch, sfx, rest_ball[sfx] + V((-1.5, 0.0, 0.0)))
    tail(stretch, [(0.0, 62.0, 0.0), (0.0, 22.0, 0.0), (0.0, 10.0, 0.0), (0.0, -4.0, 0.0), (0.0, -12.0, 0.0), (0.0, -16.0, 0.0)])
    poses['stretch'] = stretch
    # The slink: low and quick under the porch, ears back, the tail low and straight.
    slink = {'pelvis@move': (0.0, 0.0, -5.0), 'neck': (0.0, 14.0, 0.0), 'head': (0.0, -12.0, 0.0),
             'ear_l': (-24.0, 0.0, -30.0), 'ear_r': (24.0, 0.0, 30.0)}
    stand(slink, {'fl': V((6.0, 0.0, 0.6)), 'fr': V((-4.0, 0.0, 2.0)), 'hr': V((6.0, 0.0, 0.0)), 'hl': V((-4.0, 0.0, 2.0))})
    tail(slink, [(0.0, 18.0, 0.0), (0.0, 14.0, 0.0), (0.0, 10.0, 0.0), (0.0, 8.0, 0.0), (0.0, 4.0, 0.0), (0.0, 0.0, 0.0)])
    poses['slink'] = slink
    return poses


NOTES = {
    'stand': 'standing, as modelled', 'walk_a': 'the walk: left fore and right hind reaching forward (contact)',
    'walk_b': 'the walk: right fore and left hind reaching (contact); a cycle is walk_a -> walk_b -> walk_a, the lifted '
              'paws arcing forward between',
    'sit': 'sitting up on the boards, front legs straight, hind feet flat beside the front paws, the tail round them',
    'loaf': 'lying on its belly, paws folded under (the porch idle)',
    'loaf_watch': 'loafing, the head up and turned to its left (toward the player), ears forward',
    'stretch': 'a stretch on waking: front legs out, chest down, hips and tail up',
    'slink': 'the low quick run under the porch at a gunshot, ears back (blend with walk_a/walk_b at speed)',
}


# --- Build ---

cat = Cat()
MARKS.update(cat.landmarks())
la.paint_atlas(ATLAS, paint, SIZE, PX_M, SET, NORMAL_STRENGTH)
MATERIAL = lt.material(SET)
cat.finish(MATERIAL)
la.log(f'BarnCat: {la.tris(cat.obj)} triangles, {len(cat.rig.order)} bones, materials '
       f'{", ".join(m.name for m in cat.obj.data.materials)}')
_withers = max((v.co.z for v in cat.obj.data.vertices if -0.1 * SCALE < v.co.y < -0.05 * SCALE and abs(v.co.x) < 0.03),
               default=0.0)
la.log(f'BarnCat: {100.0 * _withers:.1f} cm at the withers')
POSES = cat_poses(cat)
if __name__ != '__sheet__':
    la.write_poses('Cat', cat.rig.table(POSES, NOTES))


# --- Previews ---

def check():
    la.backface_culling([MATERIAL])
    studio = la.Studio(samples=16)
    la.hide([cat.rig.arm])
    for pose in [a[5:] for a in ARGV if a.startswith('pose=')] or ['']:
        p = POSES[pose] if pose else {}
        objs = [studio.snapshot(cat.rig, cat.obj, p, at=((k - 1.5) * 0.62, 0.0, 0.0), yaw=heading, name=f'Cat{k}')
                for k, heading in enumerate((90.0, 0.0, -90.0, 145.0))]
        studio.shoot('_check_Cat' + ('_' + pose if pose else ''), objs, (0.0, -1.0, 0.12), lens=60.0, size=(2000, 700),
                     margin=1.03)
        studio.clear()


def previews():
    la.backface_culling([MATERIAL])
    studio = la.Studio()
    la.hide([cat.rig.arm])
    deck = la.plank_deck(studio, -1.2, 1.2, -0.45, 0.55, 0.0001)
    report = {'BarnCat': dict(triangles=la.tris(cat.obj), bones=len(cat.rig.order),
                              materials=[m.name for m in cat.obj.data.materials])}
    objs = [studio.snapshot(cat.rig, cat.obj, POSES['loaf'], at=(-0.26, 0.0, 0.0), yaw=-35.0, name='loaf'),
            studio.snapshot(cat.rig, cat.obj, POSES['sit'], at=(0.26, 0.16, 0.0), yaw=-110.0, name='sit')]
    studio.shoot('Cat', objs, (-0.4, -1.0, 0.32), lens=70.0, size=(1600, 1000), margin=1.03, height_bias=0.45)
    studio.clear()
    deck = la.plank_deck(studio, -1.6, 1.6, -0.5, 0.6, 0.0001)
    objs = [studio.snapshot(cat.rig, cat.obj, POSES[p], at=((k - 1.5) * 0.62, 0.0, 0.0), yaw=-90.0 + (10.0 if k == 3 else 0.0),
                            name=p) for k, p in enumerate(('walk_a', 'walk_b', 'stretch', 'slink'))]
    studio.shoot('Cat_Poses', objs, (-0.08, -1.0, 0.22), lens=55.0, size=(2000, 760), margin=1.04, height_bias=0.4)
    studio.clear()
    deck = la.plank_deck(studio, -1.0, 1.0, -0.5, 0.6, 0.0001)
    objs = [studio.snapshot(cat.rig, cat.obj, POSES['loaf_watch'], at=(0.0, 0.0, 0.0), yaw=-20.0, name='watch')]
    studio.shoot('Cat_Head', objs, None, lens=90.0, size=(1600, 1000), eye=Vector((0.13, -0.8, 0.36)),
                 target=Vector((0.02, -0.1, 0.17)))
    studio.clear()
    la.write_report('Cat', report)


if PREVIEW and __name__ != '__sheet__':
    if 'check' in ARGV:
        check()
    else:
        previews()
