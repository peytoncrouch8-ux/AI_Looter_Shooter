"""Shared pieces of the Style Lab exporter (Tools/StyleLab/export): paths, the axis mapping and a write guard.

Axes: the level's data is Unreal centimetres (X north, Y east, Z up; yaw from +X toward +Y). The page works in three.js
metres: x = -Y/100, y = Z/100, z = X/100 (the shared contract). Blender models (front -Y) exported to glTF land in the
same frame: glTF (x, y, z) = Blender (x, z, -y), and a model's Unreal-local point (cm) is (-b.y, -b.x, b.z) * 100.
"""
import builtins
import hashlib
import json
import math
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
OUT = os.path.join(REPO, 'Saved', 'StyleLab', 'export')
WORK = os.path.join(REPO, 'Saved', 'StyleLab', 'work')
SANDBOX = os.path.join(WORK, 'sandbox')

# The region built in full detail (contract): UE cm.
REGION_UE = dict(x0=-4500.0, x1=9500.0, y0=-5500.0, y1=9500.0)
# The same in three metres [x0, z0, x1, z1].
REGION_THREE = [-REGION_UE['y1'] / 100.0, REGION_UE['x0'] / 100.0, -REGION_UE['y0'] / 100.0, REGION_UE['x1'] / 100.0]


def add_paths():
    for p in (os.path.join(REPO, 'Tools', 'Blender'), os.path.join(REPO, 'Art', 'Levels'),
              os.path.join(REPO, 'Tools', 'Unreal'), HERE):
        if p not in sys.path:
            sys.path.insert(0, p)


# --- Axes ---

def ue_to_three(p):
    """A UE point (cm) to three metres."""
    return [-p[1] / 100.0, p[2] / 100.0, p[0] / 100.0]


def ue_rotation_matrix(roll, pitch, yaw):
    """UE's FRotationMatrix as a 3x3 list of columns -> rows matrix acting on column vectors (columns are the
    rotated X, Y, Z axes)."""
    sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
    sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    sr, cr = math.sin(math.radians(roll)), math.cos(math.radians(roll))
    x = (cp * cy, cp * sy, sp)
    y = (sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp)
    z = (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp)
    return [[x[0], y[0], z[0]], [x[1], y[1], z[1]], [x[2], y[2], z[2]]]


# P maps a UE vector to a three vector: (x, y, z) -> (-y, z, x).
P = [[0.0, -1.0, 0.0], [0.0, 0.0, 1.0], [1.0, 0.0, 0.0]]


def _mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def _t(a):
    return [[a[j][i] for j in range(3)] for i in range(3)]


def ue_matrix_to_three(m):
    """A UE rotation (3x3, column vectors) to three's frame: P m P^T."""
    return _mul(_mul(P, m), _t(P))


def euler_xyz(m):
    """three.js Euler 'XYZ' angles (radians) of a rotation matrix (rows, column vectors), as Euler.setFromRotationMatrix."""
    m13 = m[0][2]
    y = math.asin(max(-1.0, min(1.0, m13)))
    if abs(m13) < 0.9999999:
        x = math.atan2(-m[1][2], m[2][2])
        z = math.atan2(-m[0][1], m[0][0])
    else:
        x = math.atan2(m[2][1], m[1][1])
        z = 0.0
    return [x, y, z]


def ue_scale_to_three(s):
    """A UE actor scale (along its local X, Y, Z) to three's local (x, y, z): (Y, Z, X)."""
    return [s[1], s[2], s[0]]


# --- Images ---

def dilate_rgb(rgba, threshold=128):
    """RGBA uint8 (H, W, 4) with every pixel whose alpha is under threshold given the colour of its nearest pixel that
    isn't (alpha kept): no dark or garbage fringes when a browser premultiplies or a mip level averages across the
    cut-out's edge."""
    import cv2
    import numpy as np
    out = rgba.copy()
    clear = out[..., 3] < threshold
    if not clear.any() or clear.all():
        return out
    _, labels = cv2.distanceTransformWithLabels(clear.astype(np.uint8), cv2.DIST_L2, 5,
                                                labelType=cv2.DIST_LABEL_PIXEL)
    solid = ~clear
    for c in range(3):
        lookup = np.zeros(labels.max() + 1, np.uint8)
        lookup[labels[solid]] = out[..., c][solid]
        out[..., c][clear] = lookup[labels[clear]]
    return out


# --- Files ---

def sha1_of_files(paths):
    h = hashlib.sha1()
    for p in sorted(paths):
        h.update(p.encode())
        with open(p, 'rb') as f:
            h.update(f.read())
    return h.hexdigest()


def write_json(path, data, indent=None):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + '.tmp'
    with open(tmp, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, indent=indent, separators=(',', ':') if indent is None else (',', ': '), sort_keys=False)
    os.replace(tmp, path)


def r(v, n=4):
    """Rounded floats (lists too), so the JSON is short and stable."""
    if isinstance(v, (list, tuple)):
        return [r(x, n) for x in v]
    out = round(float(v), n)
    return 0.0 if out == 0.0 else out


# --- The write guard: model scripts paint textures and write notes into the repository as they run (the spider's
# texture set, DenDressing.placement.json, the farmhouse's screen weave). The lab must leave the repository as it is, so
# every write under the repository outside Saved/StyleLab goes to Saved/StyleLab/work/sandbox instead. ---

_ALLOWED = (os.path.join(REPO, 'Saved', 'StyleLab'),)
_real_open = builtins.open
_real_copyfile, _real_copy, _real_copy2 = shutil.copyfile, shutil.copy, shutil.copy2
_real_replace, _real_rename = os.replace, os.rename
_real_remove, _real_unlink = os.remove, os.unlink
_real_makedirs = os.makedirs
REDIRECTED = []


def _inside_repo(path):
    try:
        p = os.path.abspath(os.fspath(path))
    except TypeError:
        return None
    if isinstance(p, bytes):
        p = p.decode()
    if p == REPO or not p.startswith(REPO + os.sep):
        return None
    if any(p == a or p.startswith(a + os.sep) for a in _ALLOWED):
        return None
    return p


def _redirect(path):
    p = _inside_repo(path)
    if p is None:
        return path
    new = os.path.join(SANDBOX, os.path.relpath(p, REPO))
    _real_makedirs(os.path.dirname(new), exist_ok=True)
    REDIRECTED.append(os.path.relpath(p, REPO))
    return new


def _sandboxed(path):
    """The sandbox copy of a repository path written earlier in this run, or None."""
    p = _inside_repo(path)
    if p is None:
        return None
    new = os.path.join(SANDBOX, os.path.relpath(p, REPO))
    return new if os.path.lexists(new) else None


_real_exists, _real_isfile = os.path.exists, os.path.isfile


def install_write_guard(tag='default'):
    """From here on this process writes nothing into the repository outside Saved/StyleLab: such writes land in
    Saved/StyleLab/work/sandbox/<tag> (emptied first), and read back from there."""
    global SANDBOX
    SANDBOX = os.path.join(WORK, 'sandbox', tag)
    if os.path.isdir(SANDBOX):
        shutil.rmtree(SANDBOX)

    def guarded_open(file, mode='r', *args, **kwargs):
        if isinstance(file, (str, bytes, os.PathLike)):
            if any(c in mode for c in 'wax+'):
                file = _redirect(file)
            else:
                # A file this run wrote (into the sandbox) reads back from there.
                file = _sandboxed(file) or file
        return _real_open(file, mode, *args, **kwargs)

    os.path.exists = lambda p: _real_exists(p) or bool(isinstance(p, (str, os.PathLike)) and _sandboxed(p))
    os.path.isfile = lambda p: _real_isfile(p) or bool(isinstance(p, (str, os.PathLike)) and _sandboxed(p))

    def guarded_copy(real):
        def copy(src, dst, *args, **kwargs):
            if os.path.isdir(dst):
                dst = os.path.join(dst, os.path.basename(src))
            return real(src, _redirect(dst), *args, **kwargs)
        return copy

    def guarded_move(real):
        def move(src, dst, *args, **kwargs):
            if _inside_repo(src) is not None:
                # Never move a repository file: copy it to the sandbox instead.
                return _real_copyfile(src, _redirect(dst))
            return real(src, _redirect(dst), *args, **kwargs)
        return move

    def guarded_remove(real):
        def remove(path, *args, **kwargs):
            if _inside_repo(path) is not None:
                return None
            return real(path, *args, **kwargs)
        return remove

    def guarded_makedirs(name, mode=0o777, exist_ok=False):
        if _inside_repo(name) is not None:
            if os.path.isdir(name):
                return None
            name = _redirect(os.path.join(name, '_'))
            name = os.path.dirname(name)
        return _real_makedirs(name, mode, exist_ok=exist_ok)

    import io
    builtins.open = guarded_open
    io.open = guarded_open
    shutil.copyfile = guarded_copy(_real_copyfile)
    shutil.copy = guarded_copy(_real_copy)
    shutil.copy2 = guarded_copy(_real_copy2)
    os.replace = guarded_move(_real_replace)
    os.rename = guarded_move(_real_rename)
    os.remove = guarded_remove(_real_remove)
    os.unlink = guarded_remove(_real_unlink)
    os.makedirs = guarded_makedirs
    sys.dont_write_bytecode = True
