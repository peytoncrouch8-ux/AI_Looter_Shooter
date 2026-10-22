"""Face planes carved into a MakeHuman head after its sliders: what the heroes' portraits show and MakeHuman's smooth
base mesh leaves soft (Crane's heavy brow, cheekbones and folds; Odessa's cheekbones, jaw and lips). Used by
crane_common.Figure and Odessa.py's Figure (Docs/Pipeline/CharacterModels.md "Face planes").

A feature is a soft bump (+) or groove (-) along the skin's normal: a ridge along a line or a blob round a point, placed
from landmarks measured on the head (eyes, nose tip, mouth, chin) in the head's rest space (MakeHuman: decimetres, x his
left, y up, z out of the face), mirrored for both sides. Depths are in metres at the game's size. Only the face moves:
nothing within the lids (the eyes and the glow sockets stay as built), nothing over LID_TOP above the eyes (the hat's
band is seated 3.8-5 cm over them), nothing on the skull, neck or ears. The skeleton is MakeHuman's joints, computed
before this, and every face vertex is the head bone's: the rig is untouched.
"""
import numpy as np

import looter_garments as gm
import mh

LID = (0.20, 0.115)      # the lids' ellipse round each eye's centre (dm, x and y): left as built
LID_FADE = 0.07          # ... carving fades in over this beyond it (dm; a feature reaching the lids ends before them)
LID_TOP = (0.25, 0.32)   # carving fades out between these heights over the eyes (dm): the hat's band sits higher


def landmarks(h, hf, used):
    """The head's landmarks at rest (dm): eyes, eye_y, nose tip, mouth (middle and corner half-width), chin's front,
    the jaw's width a little under the mouth, and the face's front z."""
    rest = h['morphed']
    w_face, w_oris = _face_weights(h)
    head = rest[np.intersect1d(used, np.nonzero(w_face > 0.5)[0])]
    nose = head[np.argmax(head[:, 2])]
    lips = rest[np.intersect1d(used, np.nonzero(w_oris > 0.3)[0])]
    mouth = np.array([0.0, float(np.median(lips[:, 1])), float(lips[:, 2].max())])
    corner = float(np.abs(lips[:, 0]).max())
    mid = head[np.abs(head[:, 0]) < 0.06]
    below = mid[(mid[:, 1] < mouth[1] - 0.22) & (mid[:, 1] > mouth[1] - 0.75)]
    chin = below[np.argmax(below[:, 2])]
    jaw_y = mouth[1] - 0.30
    sl = head[np.abs(head[:, 1] - jaw_y) < 0.04]
    jaw = sl[np.argmax(np.abs(sl[:, 0]))]
    return dict(eyes=[np.asarray(e) for e in hf['eyes']], eye_y=float(hf['eye_y']), nose=nose, mouth=mouth,
                corner=corner, chin=chin, jaw=np.array([abs(jaw[0]), jaw[1], jaw[2]]), front=float(nose[2]))


def _face_weights(h):
    """Per MakeHuman vertex: the head's share (the head bone and every bone under it: the face's, eyes', jaw's and
    tongue's, which the game's rig gives the head) and the lips' (the orbicularis oris bones'). Cached on h."""
    if '_face_w' not in h:
        import json
        import os
        weights = json.load(open(os.path.join(mh.DATA, 'rigs', 'default_weights.mhw')))['weights']
        bones = h['bones']

        def under_head(b):
            while b:
                if b == 'head':
                    return True
                b = bones[b]['parent'] if b in bones else None
            return False
        n = len(h['verts'])
        face, oris = np.zeros(n), np.zeros(n)
        for bone, lst in weights.items():
            for i, x in lst:
                if under_head(bone):
                    face[i] += x
                if bone.startswith('oris'):
                    oris[i] += x
        h['_face_w'] = (face, oris)
    return h['_face_w']


def _seg_dist(P, a, b, scale):
    """Distance of points P from the segment a-b, each axis divided by scale (an ellipsoidal capsule)."""
    P, a, b = P / scale, a / scale, b / scale
    ab = b - a
    t = np.clip(((P - a) @ ab) / max(float(ab @ ab), 1e-12), 0.0, 1.0)
    return np.linalg.norm(P - (a + t[:, None] * ab), axis=1)


def _smooth(x):
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3.0 - 2.0 * x)


def carve(h, hf, used, features, scale, log=print, name='face'):
    """Moves h['verts'] (posed) and h['morphed'] (rest) of the face by features: a list of
    ('ridge', a, b, radii, depth) or ('blob', c, None, radii, depth): a, b, c points as _at reads them, radii (dm,
    x y z) the falloff's reach, depth (m, + out of the skin). A feature whose points are 'side' is mirrored. scale:
    the body's (game metres per MakeHuman decimetre x 10). Returns the landmarks."""
    L = landmarks(h, hf, used)
    g = h['glob']['head']
    R = g[:3, :3]
    rest = h['morphed']
    loc = rest[used]
    hw = _face_weights(h)[0][used]
    # where carving may reach: the face (the head's own), round the lids not in them, under the band's line
    mask = _smooth((hw - 0.55) / 0.3)
    for e in L['eyes']:
        d = np.linalg.norm((loc[:, :2] - e[:2]) / np.array(LID), axis=1)
        mask *= _smooth((d - 1.0) / (LID_FADE / LID[1]))
    mask *= 1.0 - _smooth((loc[:, 1] - L['eye_y'] - LID_TOP[0]) / (LID_TOP[1] - LID_TOP[0]))
    # the face's front half only (the ears and the skull's sides behind it stay)
    mask *= _smooth((loc[:, 2] - (L['front'] - 1.05)) / 0.25)
    push = np.zeros(len(used))
    for kind, a, b, radii, depth in features:
        for sx in ((-1.0, 1.0) if _sided(a) or _sided(b) else (1.0,)):
            pa = _surface(_at(a, L, sx), loc)
            pb = _surface(_at(b, L, sx), loc) if b is not None else pa
            d = _seg_dist(loc, pa, pb, np.asarray(radii, float))
            push += depth * _smooth(1.0 - d)
    push *= mask
    nrm = gm.vertex_normals(h['verts'], h['group_faces']('body'))[used]
    move = nrm * (push / scale * 10.0)[:, None]          # metres at the game's size -> MakeHuman dm
    h['verts'][used] += move
    rest[used] += move @ R                                 # back into the head's rest space (R's inverse: its transpose)
    log(f'{name}: carved {int((np.abs(push) > 2e-4).sum())} points, out {push.max() * 1000:.1f} mm, '
        f'in {-push.min() * 1000:.1f} mm')
    return L


def _surface(p, loc, reach=0.07):
    """A feature point (point, dz, snap) placed: snapped, its z the face's front surface at its x, y (the frontmost
    rest point within reach) plus dz; else the landmark's own z plus dz (the jaw's edge, seen from the side)."""
    q, dz, snap = p
    q = q.copy()
    near = loc[np.linalg.norm(loc[:, :2] - q[:2], axis=1) < reach] if snap else ()
    if len(near):
        q[2] = near[:, 2].max()
    q[2] += dz
    return q


def _sided(p):
    return p is not None and p[0] == 'side'


def _at(p, L, sx):
    """A feature point ('side'|'mid', landmark, (dx, dy[, dz])): the landmark (an eye: the one on side sx; the jaw:
    its side sx) moved by dx (mirrored by side) and dy. Returns (point, dz, snapped to the surface)."""
    _kind, mark, off = p
    base = np.asarray(L['eyes'][0 if sx > 0 else 1] if mark == 'eye' else L[mark], float).copy()
    if mark in ('eye', 'jaw'):
        base[0] = abs(base[0]) * sx
    dx, dy, dz = (tuple(off) + (0.0,))[:3]
    return base + np.array([dx * sx, dy, 0.0]), dz, mark != 'jaw'
