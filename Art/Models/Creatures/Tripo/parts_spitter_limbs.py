"""The Venom Spitter from Tripo: measure() runs parts_spitter's analysis once; Place takes Tripo's space to creature space;
legs(), palps_fangs(), eyes_parts() build the limbs as clean lathes. Not a model (Tools\\models.ps1 skips parts_*.py).

Every part is built twice with one topology and one UV layout: a template where Tripo's shell is (its units, Blender
axes), which parts_spitter_look.py bakes Tripo's paint onto, and the final part in creature space (cm, x forward, y left,
z up, the origin on the ground under the thorax, as Spider.py's), which is exported.

The legs: Tripo made six, the rig drives eight (ASpiderCreature's gait, two-bone IK, rear and death curl). So no leg is
Tripo's own: all eight are lathes on the rig's bones in its IK rest pose (Spider.py's hips, knees and feet, unchanged),
shaped by one Tripo leg's measured profile (TEMPLATE_LEG: its claw, the swollen band down the tibia, the femur's taper)
and painted with its bake, each femur ending in Tripo's dark knee knob (a ball on the femur bone that the tibia turns in,
so any bend the IK makes stays closed). Left legs are built, right legs mirrored.
"""
import math

import numpy as np
from mathutils import Vector

import parts_spitter as ps

# Tripo's units to creature cm.
SX, SY, SZ = 3.7, 3.3, 3.1       # the cephalothorax: along, across, up (its hips spread over the rig's 51 cm)
SAC_SCALE = 2.45                 # the sac (Tripo's is half again the body's width: kept big, but the back femurs must clear it)
SAC_ACROSS = 0.86                # ... a little narrower across, so the back femurs rise past its flanks
SAC_SHIFT = (-20.0, 0.0, 15.0)   # cm: and moved back and up from where Tripo's sat on the carapace
SAC_TILT = 12.0                  # degrees the sac leans back about its root (its crown lower, its back longer)
LIMB_SCALE = 3.8                 # legs' radii (Tripo's femur 1.6 units -> 6.1 cm, the Spitter's thin legs)
SMALL_SCALE = 3.3                # palps' and fangs' radii
HIP_Z = 58.0                     # cm: the rig's hips (Spider.py RIDE + HIP_RADIUS[2]); Tripo's hips land here
HIP_X = 0.5                      # cm: the middle of the rig's hips along the body

TEMPLATE_LEG = 'back_left'       # the leg whose profile and paint every leg wears
KNEE_GAP = 2.5                   # units either side of Tripo's knee left to the knob
KNOB_SHARE = 1.38                # the knob's radius over the femur's at the knee
KNOB_STRETCH = 1.25              # ... and its length along the femur over that
FEMUR_START = -0.1               # the femur lathe starts this share of the bone inside the body
FEMUR_SIDES, TIBIA_SIDES, SMALL_SIDES = 12, 10, 8
FEMUR_ROWS, TIBIA_ROWS = 16, 26
ROOT_DEPTH = 0.8                 # units a palp's or fang's root sits under the head's skin
PEDICEL = (11.0, 7.0, 16)        # cm: the waist between body and sac, radius at the body, at the sac, sides


class Part:
    """One piece: faces (index tuples), V (final, creature cm), T (template, Tripo units; None when it isn't baked),
    uv (per face, its corners' (u, v) in 0..1 of its atlas region), the bone it moves with, its region and look."""

    def __init__(self, name, bone, region, look='bake'):
        self.name, self.bone, self.region, self.look = name, bone, region, look
        self.V, self.T, self.faces, self.uv = [], [], [], []

    def vert(self, final, template=None):
        self.V.append(np.asarray(final, float))
        self.T.append(None if template is None else np.asarray(template, float))
        return len(self.V) - 1

    def face(self, idx, uvs):
        self.faces.append(tuple(idx))
        self.uv.append([tuple(u) for u in uvs])

    def mirrored(self, name, bone):
        """The part mirrored across the body (y -> -y in creature space), faces turned back out; same UVs."""
        out = Part(name, bone, self.region, self.look)
        out.V = [v * np.array([1.0, -1.0, 1.0]) for v in self.V]
        out.T = list(self.T)
        out.faces = [f[::-1] for f in self.faces]
        out.uv = [u[::-1] for u in self.uv]
        return out


class Place:
    """Tripo's space to creature space: the cephalothorax scaled per axis from Tripo's hips' middle onto the rig's; the sac
    uniformly from its root (the junction), leaning back."""

    def __init__(self, hips, junction_point):
        self.anchor = np.array([0.0, hips[:, 1].mean(), hips[:, 2].mean()])
        self.root = np.asarray(junction_point, float)
        self.root_c = self.body(self.root)                  # the pedicel's body end: the abdomen's and sac's pivot
        self.mouth_c = self.root_c + np.array(SAC_SHIFT)    # the sac's mouth, where the pedicel enters it
        t = math.radians(SAC_TILT)
        # Lean back = turn about the creature's y (left) axis, crown toward -x.
        self.tilt = np.array([[math.cos(t), 0.0, -math.sin(t)], [0.0, 1.0, 0.0], [math.sin(t), 0.0, math.cos(t)]])

    def body(self, p):
        p = np.asarray(p, float)
        d = p - self.anchor
        out = np.stack([-d[..., 1] * SX + HIP_X, d[..., 0] * SY, d[..., 2] * SZ + HIP_Z], axis=-1)
        return out

    def body_dir(self, v):
        v = np.asarray(v, float)
        return np.stack([-v[..., 1] * SX, v[..., 0] * SY, v[..., 2] * SZ], axis=-1)

    def sac(self, p):
        d = np.asarray(p, float) - self.root
        c = np.stack([-d[..., 1], d[..., 0] * SAC_ACROSS, d[..., 2]], axis=-1) * SAC_SCALE
        return self.mouth_c + c @ self.tilt.T


def measure():
    """Tripo's model loaded, levelled and measured: everything the builders need."""
    obj, maps = ps.load()
    _col, tips = ps.level(obj, maps)
    co, nbr = ps.graph(obj)
    col = ps.vertex_colours(obj, maps['BC'])
    legs = []
    for t in tips:
        tb = ps.tube(obj, nbr, co, t, base_count=36, base_radius=2.1, after=45.0)
        tb['extra'] = ps.LEG_EXTRA
        tb['hip'] = tb['line'][-1]
        legs.append(tb)
    fang, palp = ps.head_tips(co, col, legs)
    fang_t = ps.tube(obj, nbr, co, fang, base_count=30, base_radius=1.2, after=1.0, step=0.25)
    palp_t = ps.tube(obj, nbr, co, palp, base_count=30, base_radius=1.4, after=3.0, step=0.4)
    tree = ps.tree_of(obj)
    sac = ps.sac_shell(obj, col, tree)
    hips = np.array([t['hip'] for t in legs])
    centre = np.array([0.0, hips[:, 1].mean() - 3.0, hips[:, 2].mean() + 3.0])
    keep = ps.body_mask(obj, legs + [fang_t, palp_t], sac, col, centre)
    body = ps.body_shell(obj, keep, centre, tree)
    point, _axis = ps.junction(sac, centre)
    # The template leg: the left one farthest back (x > 0 is the spider's left).
    left = [t for t in legs if t['hip'][0] > 0.0]
    template = max(left, key=lambda t: t['hip'][1]) if TEMPLATE_LEG == 'back_left' else left[0]
    return {'obj': obj, 'maps': maps, 'col': col, 'legs': legs, 'template': template, 'fang': fang_t, 'palp': palp_t,
            'sac': sac, 'body': body, 'keep': keep, 'eyes': ps.eyes(obj, col, keep), 'place': Place(hips, point),
            'tree': tree}


# --- Lathes ---

def frames(centres, ref=None, plane=None, pole=None):
    """Tangents and tops along a centre line. With a leg's plane (its normal) the top lies in that plane, square to the
    tangent, on the pole's side (Spider.py's ring_frame: a steep femur never runs parallel to it); else the top is ref
    made square to the tangent."""
    c = np.asarray(centres, float)
    tang = np.gradient(c, axis=0)
    tang /= np.linalg.norm(tang, axis=1, keepdims=True)
    if plane is not None:
        top = np.cross(np.asarray(plane, float), tang)
        if np.mean(top @ np.asarray(pole, float)) < 0.0:
            top = -top
    else:
        ref = np.broadcast_to(np.asarray(ref, float), c.shape)
        top = ref - tang * np.sum(ref * tang, axis=1, keepdims=True)
    top /= np.linalg.norm(top, axis=1, keepdims=True)
    return tang, top


def lathe(part, final, template, sides, us, tip=None, round_end=True):
    """A tube on part: final/template are (centres, (tangents, tops), radii) each; us the rows' U (0..1 along the
    region). Its near end closes to a low point inside what it grows from; its far end to tip (final, template) or a
    rounded cap."""
    rings = []
    ends = []
    for c, (tang, top), r in (final, template):
        c = np.asarray(c, float)
        side = np.cross(tang, top)
        ring = []
        for i in range(len(c)):
            ring.append([c[i] + (top[i] * math.cos(a) + side[i] * math.sin(a)) * r[i]
                         for a in (-math.pi + 2.0 * math.pi * j / sides for j in range(sides))])
        rings.append(ring)
        hub0 = c[0] - tang[0] * r[0] * 0.5
        if round_end:
            shoulder = [c[-1] + tang[-1] * r[-1] * 0.62 + (top[-1] * math.cos(a) + side[-1] * math.sin(a)) * r[-1] * 0.78
                        for a in (-math.pi + 2.0 * math.pi * j / sides for j in range(sides))]
            hub1 = c[-1] + tang[-1] * r[-1] * 0.98
        else:
            shoulder, hub1 = None, None
        ends.append((hub0, shoulder, hub1))
    fin, tem = rings
    rows = len(fin)
    idx = [[part.vert(fin[i][j], tem[i][j]) for j in range(sides)] for i in range(rows)]
    if ends[0][1] is not None and tip is None:
        idx.append([part.vert(ends[0][1][j], ends[1][1][j]) for j in range(sides)])
        us = list(us) + [us[-1]]
    for i in range(len(idx) - 1):
        for j in range(sides):
            j1 = (j + 1) % sides
            v0, v1 = j / sides, (j + 1) / sides
            part.face((idx[i][j], idx[i + 1][j], idx[i + 1][j1], idx[i][j1]),
                      [(us[i], v0), (us[i + 1], v0), (us[i + 1], v1), (us[i], v1)])
    h0 = part.vert(ends[0][0], ends[1][0])
    for j in range(sides):
        j1 = (j + 1) % sides
        part.face((h0, idx[0][j1], idx[0][j]), [(us[0], 0.5), (us[0], (j + 1) / sides), (us[0], j / sides)])
    far = tip if tip is not None else (ends[0][2], ends[1][2])
    h1 = part.vert(far[0], far[1])
    for j in range(sides):
        j1 = (j + 1) % sides
        part.face((h1, idx[-1][j], idx[-1][j1]), [(us[-1], 0.5), (us[-1], j / sides), (us[-1], (j + 1) / sides)])


def ellipsoid(part, centre_f, centre_t, axis_f, axis_t, top_f, top_t, radius_f, radius_t, stretch, rings=8, segs=12):
    """A ball (the knee knob, an eye) on part, its poles along axis, latitude-longitude UVs (U along the axis)."""
    def frame(axis, top):
        a = np.asarray(axis, float) / np.linalg.norm(axis)
        t = np.asarray(top, float) - a * np.dot(top, a)
        t /= np.linalg.norm(t)
        return a, t, np.cross(a, t)

    fa, ft, fs = frame(axis_f, top_f)
    ta, tt, ts = frame(axis_t, top_t)
    grid = []
    for i in range(1, rings):
        th = math.pi * i / rings
        row = []
        for j in range(segs):
            ph = -math.pi + 2.0 * math.pi * j / segs
            along, ring = -math.cos(th) * stretch, math.sin(th)
            pf = centre_f + (fa * along + (ft * math.cos(ph) + fs * math.sin(ph)) * ring) * radius_f
            pt = centre_t + (ta * along + (tt * math.cos(ph) + ts * math.sin(ph)) * ring) * radius_t
            row.append(part.vert(pf, pt))
        grid.append(row)
    for i in range(len(grid) - 1):
        u0, u1 = i / (rings - 2), (i + 1) / (rings - 2)
        for j in range(segs):
            j1 = (j + 1) % segs
            part.face((grid[i][j], grid[i + 1][j], grid[i + 1][j1], grid[i][j1]),
                      [(u0, j / segs), (u1, j / segs), (u1, (j + 1) / segs), (u0, (j + 1) / segs)])
    for pole, row, u, flip in ((-1.0, grid[0], 0.0, True), (1.0, grid[-1], 1.0, False)):
        h = part.vert(centre_f + fa * pole * stretch * radius_f, centre_t + ta * pole * stretch * radius_t)
        for j in range(segs):
            j1 = (j + 1) % segs
            tri = (h, row[j1], row[j]) if flip else (h, row[j], row[j1])
            part.face(tri, [(u, 0.5), (u, (j1 if flip else j) / segs), (u, (j if flip else j1) / segs)])


# --- The legs on the rig's bones ---

def _profile(t, s0, s1, n):
    """n stations from arc length s0 to s1 on a tube: (centres, radii)."""
    s = np.linspace(s0, s1, n)
    c = np.stack([np.interp(s, t['s'], t['line'][:, i]) for i in range(3)], axis=1)
    return c, np.interp(s, t['s'], t['radius'])


def _pole(hip_xy_out):
    return np.array([0.0, 0.0, 1.0]) + hip_xy_out * 0.4


def legs(m, bones):
    """Eight legs (left built, right mirrored): {bone: [Part]} for femur_<p>_<s> (shaft and knob) and tibia_<p>_<s>.
    bones: Spider.py's list [(name, head, tail, parent)] in creature cm."""
    t = m['template']
    k = ps.knee(t)
    s_knee, s_hip = t['s'][k], t['s'][-1]
    hip_t, knee_t, tip_t = t['line'][-1], t['line'][k], t['line'][0]
    out_t = np.array([hip_t[0], hip_t[1] - m['body']['centre'][1], 0.0])
    out_t /= np.linalg.norm(out_t)
    pole_t = np.array([0.0, 0.0, 1.0]) + out_t * 0.4
    # Template rows: the femur from the hip end to the knob, the tibia from the knob to the claw.
    fc_t, fr_t = _profile(t, s_hip, s_knee + KNEE_GAP, FEMUR_ROWS)
    tc_t, tr_t = _profile(t, s_knee - KNEE_GAP, t['s'][1], TIBIA_ROWS)
    fr_t = np.maximum(fr_t, np.median(fr_t) * 0.8)
    us_f = np.linspace(0.0, 1.0, FEMUR_ROWS)
    us_t = np.linspace(0.0, 1.0, TIBIA_ROWS + 1)[:-1]
    r_knee = float(np.median(fr_t[-5:]))
    plane_t = np.cross(knee_t - hip_t, tip_t - knee_t)
    plane_t /= np.linalg.norm(plane_t)
    ff_t, tf_t = frames(fc_t, plane=plane_t, pole=pole_t), frames(tc_t, plane=plane_t, pole=pole_t)
    kdir_t = (knee_t - fc_t[-4]) / np.linalg.norm(knee_t - fc_t[-4])
    by = {name: (np.asarray(head), np.asarray(tail)) for name, head, tail, _p in bones}
    parts = {}
    for pair in range(4):
        hip, knee_c = by[f'femur_{pair}_l']
        _k, foot = by[f'tibia_{pair}_l']
        out = np.array([hip[0], hip[1], 0.0])
        out /= np.linalg.norm(out)
        pole = _pole(out)
        flen, tlen = np.linalg.norm(knee_c - hip), np.linalg.norm(foot - knee_c)
        fdir, tdir = (knee_c - hip) / flen, (foot - knee_c) / tlen
        fbend = pole - fdir * np.dot(pole, fdir)
        fbend /= np.linalg.norm(fbend)
        tbend = pole - tdir * np.dot(pole, tdir)
        tbend /= np.linalg.norm(tbend)
        tf = FEMUR_START + (1.0 - FEMUR_START) * us_f
        fc = hip + np.outer(tf, knee_c - hip) + np.outer(np.sin(math.pi * np.clip(tf, 0.0, 1.0)), fbend) * flen * 0.05
        tt = us_t
        tc = knee_c + np.outer(tt, foot - knee_c) + np.outer(np.sin(math.pi * tt), tbend) * tlen * 0.06
        plane = np.cross(knee_c - hip, foot - knee_c)
        plane /= np.linalg.norm(plane)
        femur = Part(f'Femur{pair}L', f'femur_{pair}_l', 'femur')
        lathe(femur, (fc, frames(fc, plane=plane, pole=pole), fr_t * LIMB_SCALE), (fc_t, ff_t, fr_t), FEMUR_SIDES, us_f)
        knob = Part(f'Knob{pair}L', f'femur_{pair}_l', 'knob')
        top_f, top_t = np.cross(plane, fdir), np.cross(plane_t, kdir_t)
        top_f, top_t = top_f * np.sign(top_f @ pole), top_t * np.sign(top_t @ pole_t)
        ellipsoid(knob, knee_c - fdir * r_knee * LIMB_SCALE * 0.3, knee_t - kdir_t * r_knee * 0.3, fdir, kdir_t,
                  top_f, top_t, r_knee * KNOB_SHARE * LIMB_SCALE, r_knee * KNOB_SHARE, KNOB_STRETCH)
        tibia = Part(f'Tibia{pair}L', f'tibia_{pair}_l', 'tibia')
        lathe(tibia, (tc, frames(tc, plane=plane, pole=pole), tr_t * LIMB_SCALE), (tc_t, tf_t, tr_t), TIBIA_SIDES, us_t,
              tip=(foot, tip_t))
        for part in (femur, knob, tibia):
            parts.setdefault(part.bone, []).append(part)
            right = part.bone.replace('_l', '_r')
            parts.setdefault(right, []).append(part.mirrored(part.name[:-1] + 'R', right))
    return parts


def palps_fangs(m):
    """The palps and fangs rebuilt where Tripo has them (lathes on their measured lines, a little sunk into the head):
    {bone: [Part]}, left built, right mirrored."""
    place = m['place']
    parts = {}
    for key, bone, region, root_in in (('palp', 'palp', 'palp', 1.2), ('fang', 'fang', 'fang', 1.0)):
        t = m[key]
        line, r, s = t['line'], t['radius'], t['s']
        n = max(8, min(24, int(s[-1] / 0.5)))
        c_t, r_t = _profile(t, s[-1], s[1], n)
        r_t = np.maximum(r_t, 0.12)
        # Root pushed on into the head so no seam shows when the fang turns.
        back = c_t[0] - c_t[1]
        c_t[0] = c_t[0] + back / np.linalg.norm(back) * root_in
        # The rebuilt head closed over where the root was (its rays there were filled): reach on in until under its skin.
        shell = m['body']
        d = c_t[0] - shell['centre']
        dist = float(np.linalg.norm(d))
        under = shell['radius'][int(np.argmax(shell['dirs'] @ (d / dist)))] - ROOT_DEPTH
        if dist > under:
            c_t[0] = shell['centre'] + d / dist * under
        fwd_t = np.array([0.0, -1.0, 0.0])
        c_f = place.body(c_t)
        part = Part(f'{key.title()}L', f'{bone}_l', region)
        us = np.linspace(0.0, 1.0, n + 1)[:-1]
        lathe(part, (c_f, frames(c_f, ref=place.body_dir(fwd_t)), r_t * SMALL_SCALE), (c_t, frames(c_t, ref=fwd_t), r_t),
              SMALL_SIDES, us, tip=(place.body(line[0]), line[0]))
        parts[f'{bone}_l'] = [part]
        parts[f'{bone}_r'] = [part.mirrored(f'{key.title()}R', f'{bone}_r')]
    return parts


def roots(m):
    """Where the moved bones belong on this body (creature cm): each fang's and palp's root."""
    place = m['place']
    out = {}
    for key in ('fang', 'palp'):
        t = m[key]
        c = place.body(t['line'][-1])
        out[f'{key}_l'] = c
        out[f'{key}_r'] = c * np.array([1.0, -1.0, 1.0])
    return out


def eyes_parts(m):
    """Clean glossy eyes where Tripo painted them (its clusters), sat on the final head: [Part] on the head bone."""
    place = m['place']
    shell = m['body']
    part = Part('Eyes', 'head', 'eye', look='eye')
    for c, r in m['eyes']:
        r = max(r, 0.35)
        for side in (1.0, -1.0):
            ct = np.array([c[0] * side, c[1], c[2]])
            d = ct - shell['centre']
            d /= np.linalg.norm(d)
            i = int(np.argmax(shell['dirs'] @ d))
            surf_t = shell['centre'] + d * shell['radius'][i]
            surf_f = place.body(surf_t)
            normal = place.body_dir(d)
            normal /= np.linalg.norm(normal)
            rf = r * SMALL_SCALE
            ellipsoid(part, surf_f - normal * rf * 0.35, surf_t - d * r * 0.35, normal, d,
                      np.array([0.0, 0.0, 1.0]), np.array([0.0, 0.0, 1.0]), rf, r, 1.0, rings=6, segs=10)
    return [part]
