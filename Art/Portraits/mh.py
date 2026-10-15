"""MakeHuman's CC0 base mesh, shaped and posed in numpy (no MakeHuman code: only its assets, fetched by
Art/ThirdParty/MakeHuman/fetch.py). Used by EllisBlender.py; plain Python + numpy, so it runs anywhere.

    human = build(macro={...}, face={...}, pose={...})

macro: gender 0..1 (0.5 is neither), muscle and weight 0..1 (0.5 average), ethnic mix (african/asian/caucasian).
face:  modifier name -> -1..1, e.g. {"cheek/l-cheek-bones": 0.6}; negative picks the modifier's min target.
pose:  bone name -> (axis xyz, degrees) in MakeHuman space, applied down the bone's chain by linear blend skinning
       with MakeHuman's default weights.

MakeHuman space: decimetres, +y up, +z out of the face, +x the character's left. to_blender() converts.
"""
import json
import math
import os

import numpy as np

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ThirdParty", "MakeHuman")

# Which modifier files are one-sided (a single target, 0..1).
ONE_SIDED = {"head/head-oval", "head/head-square", "head/head-invertedtriangular", "head/head-diamond", "head/head-rectangular"}
# min/max file ends per modifier; anything not listed is decr/incr.
ENDS = {
	"forehead-trans": ("backward", "forward"), "eyebrows-angle": ("down", "up"), "nose-trans": ("backward", "forward"),
	"nose-curve": ("concave", "convex"), "mouth-angles": ("down", "up"), "eye-eyefold": ("down", "up"), "eye-corner1": ("down", "up"),
}


def load_obj(path):
	verts, uvs, faces, fuv, groups = [], [], [], [], []
	group = None
	with open(path) as f:
		for line in f:
			if line.startswith("v "):
				verts.append([float(x) for x in line.split()[1:4]])
			elif line.startswith("vt "):
				uvs.append([float(x) for x in line.split()[1:3]])
			elif line.startswith("g "):
				group = line.split()[1]
			elif line.startswith("f "):
				parts = [t.split("/") for t in line.split()[1:]]
				faces.append([int(p[0]) - 1 for p in parts])
				fuv.append([int(p[1]) - 1 if len(p) > 1 and p[1] else -1 for p in parts])
				groups.append(group)
	return np.array(verts, dtype=np.float64), np.array(uvs, dtype=np.float64), faces, fuv, groups


def load_target(rel):
	idx, d = [], []
	with open(os.path.join(DATA, "targets", rel + ".target")) as f:
		for line in f:
			if not line.strip() or line.startswith("#"):
				continue
			p = line.split()
			idx.append(int(p[0]))
			d.append([float(p[1]), float(p[2]), float(p[3])])
	return np.array(idx, dtype=np.int64), np.array(d, dtype=np.float64)


def _apply(v, rel, w):
	if abs(w) < 1e-6:
		return
	i, d = load_target(rel)
	if len(i):
		v[i] += d * w


def _three(value):
	"""0..1 slider -> weights of its (min, average, max) targets."""
	if value >= 0.5:
		hi = (value - 0.5) * 2.0
		return {"min": 0.0, "average": 1.0 - hi, "max": hi}
	lo = (0.5 - value) * 2.0
	return {"min": lo, "average": 1.0 - lo, "max": 0.0}


def build(macro=None, face=None, pose=None):
	m = {"gender": 0.5, "muscle": 0.5, "weight": 0.5, "african": 0.0, "asian": 0.0, "caucasian": 1.0}
	m.update(macro or {})
	v, uv, faces, fuv, groups = load_obj(os.path.join(DATA, "3dobjs", "base.obj"))
	rest = v.copy()
	genders = {"female": 1.0 - m["gender"], "male": m["gender"]}
	eth = np.array([m["african"], m["asian"], m["caucasian"]], dtype=np.float64)
	eth = eth / max(eth.sum(), 1e-6)
	for e, ew in zip(("african", "asian", "caucasian"), eth):
		for g, gw in genders.items():
			_apply(v, f"macrodetails/{e}-{g}-young", ew * gw)
	mus, wei = _three(m["muscle"]), _three(m["weight"])
	for g, gw in genders.items():
		for mk, mw in mus.items():
			for wk, ww in wei.items():
				if mk == "average" and wk == "average":
					continue   # empty by definition
				_apply(v, f"macrodetails/universal-{g}-young-{mk}muscle-{wk}weight", gw * mw * ww)
	for name, val in (face or {}).items():
		if name in ONE_SIDED:
			_apply(v, name, val)
			continue
		group, target = name.split("/")
		key = target[2:] if target[:2] in ("l-", "r-") else target
		lo, hi = ENDS.get(key, ("decr", "incr"))
		_apply(v, f"{name}-{hi if val > 0 else lo}", abs(val))

	skel = json.load(open(os.path.join(DATA, "rigs", "default.mhskel")))
	joints = {k: v[np.array(ix)].mean(axis=0) for k, ix in skel["joints"].items()}
	bones = {}
	for name, b in skel["bones"].items():
		bones[name] = {"head": joints[b["head"]], "tail": joints[b["tail"]], "parent": b["parent"]}

	morphed = v.copy()
	glob = {name: np.eye(4) for name in bones}
	if pose:
		v, glob = _skin(v, bones, pose)

	def group_faces(prefix):
		return [f for f, g in zip(faces, groups) if g and g.startswith(prefix)]

	return {
		"verts": v, "morphed": morphed, "rest": rest, "uv": uv, "faces": faces, "fuv": fuv, "groups": groups,
		"bones": bones, "glob": glob, "group_faces": group_faces,
		"weights": lambda bone: _bone_weights(bone, len(v)),
	}


def _axis_angle(axis, deg):
	a = np.array(axis, dtype=np.float64)
	a /= np.linalg.norm(a)
	t = math.radians(deg)
	c, s = math.cos(t), math.sin(t)
	x, y, z = a
	return np.array([
		[c + x * x * (1 - c), x * y * (1 - c) - z * s, x * z * (1 - c) + y * s],
		[y * x * (1 - c) + z * s, c + y * y * (1 - c), y * z * (1 - c) - x * s],
		[z * x * (1 - c) - y * s, z * y * (1 - c) + x * s, c + z * z * (1 - c)]])


def _skin(v, bones, pose):
	"""Linear blend skinning: each posed bone turns about its head, carrying its children."""
	weights = json.load(open(os.path.join(DATA, "rigs", "default_weights.mhw")))["weights"]
	glob = {}

	def transform(name):
		if name in glob:
			return glob[name]
		b = bones[name]
		parent = transform(b["parent"]) if b["parent"] else np.eye(4)
		local = np.eye(4)
		if name in pose:
			axis, deg = pose[name]
			r = _axis_angle(axis, deg)
			h = b["head"]
			local[:3, :3] = r
			local[:3, 3] = h - r @ h
		glob[name] = parent @ local
		return glob[name]

	for name in bones:
		transform(name)
	out = np.zeros_like(v)
	wsum = np.zeros(len(v))
	vh = np.c_[v, np.ones(len(v))]
	for name, lst in weights.items():
		if name not in glob or not lst:
			continue
		arr = np.array(lst, dtype=np.float64)
		ix = arr[:, 0].astype(np.int64)
		w = arr[:, 1]
		out[ix] += (vh[ix] @ glob[name].T)[:, :3] * w[:, None]
		wsum[ix] += w
	# Anything unweighted (helpers) stays put; weights are normalised per vertex.
	moved = wsum > 1e-6
	res = v.copy()
	res[moved] = out[moved] / wsum[moved][:, None]
	return res, glob


def _bone_weights(bone, n):
	"""Per-vertex weight of one bone (0 where it has none)."""
	weights = json.load(open(os.path.join(DATA, "rigs", "default_weights.mhw")))["weights"]
	w = np.zeros(n)
	for i, x in weights.get(bone, []):
		w[i] = x
	return w


def apply(m, p):
	"""A 4x4 transform applied to points (n, 3)."""
	p = np.asarray(p, dtype=np.float64)
	return p @ m[:3, :3].T + m[:3, 3]


def to_blender(p):
	"""MakeHuman decimetres (+y up, +z forward) -> Blender metres (+z up, the character facing -y)."""
	p = np.asarray(p, dtype=np.float64)
	return np.stack([p[..., 0], -p[..., 2], p[..., 1]], axis=-1) * 0.1
