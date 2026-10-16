"""Wendell Pike's portrait busts in Blender (Cycles): MakeHuman's CC0 base mesh shaped into Pike and dressed with the
machinery every hero shares (bust.py; EllisBlender.py is the worked example, Saved/Portraits/CONTRACT.md the brief).

Run (more in bust.py):
    "C:/Program Files/Blender Foundation/Blender 4.4/blender.exe" -b --factory-startup --python-exit-code 1
        --python Art/Portraits/Pike.py -- <A|B|C> [--size 1024] [--samples 80] [--out <dir>] [--no-aux] [--blend]

Wendell Pike, the Surveyor (1.68 m, stocky, fifty-five; Docs/Story.md "The heroes"):
  A  The Surveyor: brown bowler with brass goggles up on it, round spectacles, teal neckerchief, khaki field coat of
     pockets under a cartridge belt, pack straps, the tripod's brass-shod legs and the map tube up over the right
     shoulder; Revenant-cyan eyes behind the lenses.
  B  Bottom of the Ravine: the bowler dented, a lens cracked, a cut over the brow, dust and pallor, the goggles down
     round the neck, the coat torn, the brass spyglass clutched to the chest in his right hand; cyan eyes.
  C  The Mapmaker: in life, no hat, thin hair combed over, spectacles pushed up the forehead, a pencil behind the ear,
     the neckerchief loose, a rolled map under the arm, a measuring chain over the shoulder; brown eyes, lamplight.
Helpers bust.py lacks live here (candidates to merge): the GLASS and WOOD ids, head_matrix, place, tube, rod, ball,
disc, ribbon, over_shoulder, chain, wave, tufts, salt-and-pepper hair, glass, the living eye (no glow) and the arm
pose hook (_skin_with_arm, arm_pose, handless).
"""
import json
import math
import os
import sys

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mh  # noqa: E402
from bust import (  # noqa: E402
	ID, add_mod, body_bvh, brass_material, build_human, build_lapels, bump, buttons, bvh_of, canvas_material, chin_z, clump,
	curves_object, cut_vee, drape, eyeballs, frame2d, grid_object, grow, grow_combed, leather_material, lin, link,
	material, mesh_object, mix_rgb, neck_frame, noise, principled, run, shell, shoulder_z, strip_on, surface_point,
	world_points)

# The paint pass's settings per option (render.js reads these lines): dusk on the line; C by lamplight, no glow.
# PAINT A {"bgTop":"#10141c","bgBottom":"#05070b","bgGlow":"#3a4a5a","glowAt":[0.76,0.30],"sparks":"#ffb55c","dust":"#b8c4cc","glow":"#5ad8ff"}
# PAINT B {"bgTop":"#141418","bgBottom":"#06060a","bgGlow":"#4a4a52","glowAt":[0.60,0.14],"sparks":"#ffb55c","dust":"#d8c8b0","glow":"#5ad8ff"}
# PAINT C {"bgTop":"#16120e","bgBottom":"#07060a","bgGlow":"#6a4a2a","glowAt":[0.74,0.40],"sparks":"#ffc87a","dust":"#e0c49a","glow":"#ffd9a0"}

GLASS, WOOD = 14, 15   # the contract's ids past bust.ID: lenses, goggle glass; tripod legs, pencil
C_ROT = np.array([[1.0, 0, 0], [0, 0, -1.0], [0, 1.0, 0]])   # MakeHuman's axes -> Blender's (mh.to_blender unscaled)

# Pike's build: a stocky man. mh.py has only the "young" macro targets (no age or height ones): age comes from the
# head-age modifier, the eye bags and full cheeks below; height doesn't show in a bust.
MACRO = {"gender": 1.0, "muscle": 0.55, "weight": 0.9, "caucasian": 0.85, "african": 0.05, "asian": 0.1}

# Pike's face: fifty-five, round and weathered, a big bulbous nose, small sharp eyes, full cheeks, a short wide chin,
# a thick neck. Shared by every option.
FACE = {
	"head/head-age": 0.85, "head/head-oval": 0.35, "head/head-scale-vert": -0.2, "eyebrows/eyebrows-angle": -0.15,
	"chin/chin-width": 0.4, "chin/chin-height": -0.25, "chin/chin-prominent": -0.1, "chin/chin-jaw-drop": 0.15,
	"nose/nose-scale-horiz": 0.45, "nose/nose-scale-vert": 0.35, "nose/nose-point-width": 0.75, "nose/nose-hump": 0.2,
	"nose/nose-nostrils-width": 0.45, "nose/nose-width1": 0.3, "nose/nose-trans": 0.15,
	"mouth/mouth-scale-horiz": 0.1, "mouth/mouth-angles": -0.15, "forehead/forehead-temple": 0.25,
	"neck/neck-scale-horiz": 0.55, "neck/neck-scale-depth": 0.45, "torso/torso-scale-horiz": 0.3,
}
for _s in "lr":
	FACE.update({f"cheek/{_s}-cheek-volume": 0.55, f"cheek/{_s}-cheek-inner": 0.3, f"cheek/{_s}-cheek-bones": -0.15,
		f"eyes/{_s}-eye-scale": -0.35, f"eyes/{_s}-eye-bag": 0.6, f"eyes/{_s}-eye-height2": -0.3, f"ears/{_s}-ear-scale": 0.25})

CYAN = (0.25, 0.85, 1.0)
OPTIONS = {
	"A": {"label": "The Surveyor", "line": "Bowler and brass goggles, spectacles over Revenant eyes; the tripod and the map tube ride his pack.",
		"yaw": -24.0, "pitch": 3.0, "lids": (0.02, 0.012, 0.3), "eyes": "slit", "skin": (0.74, 0.60, 0.52), "glow": CYAN,
		"hat": "bowler", "goggles": "hat", "specs": "on", "hair": "under", "kerchief": "tied",
		"cloth": ((0.20, 0.17, 0.10), (0.55, 0.49, 0.33)), "belt": True, "pack": True,
		"world": (0.10, 0.12, 0.17), "cam": (2.06, -0.035),
		"lights": [("Key", (-0.80, -0.50, 0.30), 30.0, (1.0, 0.74, 0.50), 0.5), ("Rim", (1.3, 1.2, 0.5), 110.0, (0.45, 0.85, 1.0), 0.8),
			("Fill", (0.9, -1.6, -0.3), 10.0, (0.55, 0.65, 1.0), 1.5)]},
	"B": {"label": "Bottom of the Ravine", "line": "Dented bowler, a cracked lens and a cut brow; grave-dust on him, the spyglass held to his chest.",
		"yaw": -14.0, "pitch": -6.0, "lids": (0.006, 0.004, 0.0), "eyes": "slit", "skin": (0.70, 0.66, 0.64), "glow": CYAN,
		"hat": "dented", "goggles": "neck", "specs": "cracked", "hair": "under", "kerchief": "tied", "torn": True, "cut": True,
		"cloth": ((0.26, 0.24, 0.19), (0.66, 0.62, 0.52)), "belt": True, "spyglass": True,
		"arm": {"elbow": (-2.2, 3.3, 0.9), "wrist": (-0.5, 5.0, 1.95), "twist": -20.0, "curl": 60.0, "thumb": 30.0},
		"world": (0.16, 0.15, 0.15), "cam": (2.06, -0.035),
		"lights": [("Key", (-0.55, -0.35, 0.75), 30.0, (0.92, 0.88, 0.86), 0.5), ("Rim", (1.25, 1.1, 0.45), 95.0, (0.45, 0.85, 1.0), 0.8),
			("Bounce", (0.25, -0.9, -0.65), 6.0, (1.0, 0.6, 0.4), 1.2)]},
	"C": {"label": "The Mapmaker", "line": "In life, by lamplight: spectacles up, a pencil behind the ear, the chain on his shoulder and a map.",
		"yaw": 22.0, "pitch": 4.0, "lids": (0.016, 0.01, 0.2), "eyes": "life", "skin": (0.78, 0.60, 0.50), "glow": (1.0, 0.85, 0.63),
		"hat": None, "goggles": None, "specs": "up", "hair": "combover", "kerchief": "loose",
		"cloth": ((0.22, 0.17, 0.10), (0.58, 0.48, 0.32)), "pencil": True, "map": True, "chain": True,
		"world": (0.12, 0.10, 0.09), "cam": (2.02, -0.05),
		"lights": [("Key", (0.80, -0.55, -0.05), 30.0, (1.0, 0.78, 0.56), 0.5), ("Rim", (-1.25, 1.0, 0.3), 60.0, (1.0, 0.62, 0.32), 0.6),
			("Fill", (-0.9, -1.6, -0.3), 7.0, (0.5, 0.6, 1.0), 1.5)]},
}


# ---------------------------------------------------------------- materials

def flat(name, pid, color, rough=0.5, spec=0.5, metal=0.0, **kw):
	return material(name, pid, lambda nt: principled(nt, color, rough, spec, metal, **kw))


def textured(name, pid, dark, light, scale=6.0, fine=400.0, rough=0.8, sheen=0.4, stretch=None):
	"""Two colours blotched by noise and a fine bump: felt, cloth, paper, wood (stretch draws the noise into grain)."""
	def build(nt):
		co = nt.nodes.new("ShaderNodeTexCoord").outputs["Object"]
		if stretch:
			mp = nt.nodes.new("ShaderNodeMapping")
			mp.inputs["Scale"].default_value = stretch
			nt.links.new(co, mp.inputs["Vector"])
			co = mp.outputs[0]
		b = principled(nt, mix_rgb(nt, noise(nt, scale, 6.0, 0.6, co).outputs["Fac"], dark, light), rough=rough, spec=0.3)
		b.inputs["Sheen Weight"].default_value = sheen
		nt.links.new(bump(nt, noise(nt, fine, 3.0, 0.7, co).outputs["Fac"], 0.12, 0.0005), b.inputs["Normal"])
		return b
	return material(name, pid, build)


def glass(tint=(0.95, 0.97, 0.96)):
	return flat("Glass", GLASS, tint, rough=0.02, spec=0.6, **{"Transmission Weight": 1.0, "IOR": 1.5})


def hair_objects(name, strands, rng, r0, r1, light=0.72):
	"""Grey hair as salt and pepper: most strands white-grey, the rest dark (one grey reads as a wig)."""
	def hair(color, nm):
		def build(nt):
			b = nt.nodes.new("ShaderNodeBsdfHairPrincipled")
			b.parametrization = "COLOR"
			b.inputs["Color"].default_value = (*lin(color), 1)
			b.inputs["Roughness"].default_value, b.inputs["Radial Roughness"].default_value = 0.42, 0.5
			return b
		return material(nm, ID["hair"], build)
	pick = rng.random(len(strands)) < light
	return [curves_object(name, [s for s, p in zip(strands, pick) if p], r0, r1, hair((0.72, 0.70, 0.66), "HairGrey")),
		curves_object(name + "Dark", [s for s, p in zip(strands, pick) if not p], r0, r1, hair((0.22, 0.19, 0.16), "HairDark"))]


def life_eye():
	"""C's eyes as in life: brown, no glow. Id skin, so paint.js keeps them crisp but the glow mask leaves them out."""
	def build(nt):
		sep = nt.nodes.new("ShaderNodeSeparateXYZ")
		nt.links.new(nt.nodes.new("ShaderNodeTexCoord").outputs["Object"], sep.inputs[0])
		zr = nt.nodes.new("ShaderNodeMath"); zr.operation = "DIVIDE"   # the eyeball's +z looks at the camera
		nt.links.new(sep.outputs["Z"], zr.inputs[0]); zr.inputs[1].default_value = 0.0118
		ramp = nt.nodes.new("ShaderNodeValToRGB")
		el = ramp.color_ramp.elements
		el[0].position, el[0].color = 0.80, (*lin((0.66, 0.62, 0.56)), 1)
		el[1].position, el[1].color = 0.95, (*lin((0.03, 0.02, 0.02)), 1)
		for pos, c in ((0.83, (0.12, 0.07, 0.04)), (0.86, (0.40, 0.24, 0.11)), (0.935, (0.25, 0.14, 0.06))):
			el.new(pos).color = (*lin(c), 1)
		nt.links.new(zr.outputs[0], ramp.inputs[0])
		return principled(nt, ramp.outputs[0], rough=0.08, spec=0.7)
	return material("EyeLife", ID["skin"], build)


# ---------------------------------------------------------------- geometry helpers

def head_matrix(h):
	"""Head-local metres in MakeHuman's axes (x the character's left, y up, z out of the face) -> world, as posed."""
	G = h["glob"]["head"]
	M = np.eye(4)
	M[:3, :3], M[:3, 3] = C_ROT @ G[:3, :3], 0.1 * C_ROT @ G[:3, 3]
	return Matrix(M.tolist())


def head_world(h, p):
	"""Rest head-space points (MakeHuman decimetres) -> world, as posed."""
	return mh.to_blender(mh.apply(h["glob"]["head"], np.atleast_2d(p)))


def bone_world(h, bone, end="head"):
	return mh.to_blender(mh.apply(h["glob"][bone], h["bones"][bone][end][None]))[0]


def place(obs, M):
	"""Carry objects built in a local frame into the world (matrix_basis: matrix_world lags until a depsgraph update)."""
	for o in obs:
		o.matrix_world = M @ o.matrix_basis
	return obs


def frame_matrix(origin, ex, ez):
	"""A frame at origin whose z is ez and whose x is ex (made square to ez)."""
	ez = np.asarray(ez, float) / np.linalg.norm(ez)
	ex = np.asarray(ex, float) - ez * (np.asarray(ex, float) @ ez)
	ex /= np.linalg.norm(ex)
	M = np.eye(4)
	M[:3, 0], M[:3, 1], M[:3, 2], M[:3, 3] = ex, np.cross(ez, ex), ez, origin
	return Matrix(M.tolist())


def tube(name, pts, radius, mat, cyclic=False):
	"""A round wire, strap or rope along a polyline (a bevelled curve)."""
	cu = bpy.data.curves.new(name, "CURVE")
	cu.dimensions, cu.bevel_depth, cu.bevel_resolution, cu.use_fill_caps = "3D", radius, 3, True
	sp = cu.splines.new("POLY")
	sp.points.add(len(pts) - 1)
	for p, q in zip(sp.points, pts):
		p.co = (q[0], q[1], q[2], 1.0)
	sp.use_cyclic_u = cyclic
	cu.materials.append(mat)
	return link(bpy.data.objects.new(name, cu))


def ring_pts(c, e1, e2, r, n=24, a0=0.0):
	return [c + (e1 * math.cos(t) + e2 * math.sin(t)) * r for t in a0 + np.linspace(0, 2 * math.pi, n, endpoint=False)]


def bm_object(name, bm, mat, smooth=True):
	me = bpy.data.meshes.new(name)
	bm.to_mesh(me)
	bm.free()
	if smooth:
		me.shade_smooth()
	ob = link(bpy.data.objects.new(name, me))
	ob.data.materials.append(mat)
	return ob


def rod(name, a, b, r1, r2, mat, seg=20):
	"""A cylinder or cone from a (radius r1) to b (radius r2); flat caps, smooth sides."""
	a, b = Vector(tuple(a)), Vector(tuple(b))
	bm = bmesh.new()
	bmesh.ops.create_cone(bm, cap_ends=True, segments=seg, radius1=r1, radius2=r2, depth=(b - a).length)
	for f in bm.faces:
		f.smooth = abs(f.normal.z) < 0.7
	ob = bm_object(name, bm, mat, False)
	ob.matrix_world = Matrix.Translation((a + b) * 0.5) @ (b - a).to_track_quat("Z", "Y").to_matrix().to_4x4()
	return ob


def ball(name, at, r, mat, scale=(1, 1, 1)):
	bm = bmesh.new()
	bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=10, radius=r)
	ob = bm_object(name, bm, mat)
	ob.location, ob.scale = Vector(tuple(at)), scale
	return ob


def disc(name, ring, mat, sag=0.0008, centre=(0.0, 0.0)):
	"""A lens (or a shard of one, centre at the point of impact): a fan over (x, y) points in local xy, bowed sag
	toward +z; a full ring closes, a shard's arc does not."""
	r2 = max(x * x + y * y for x, y in ring)
	verts = [(x, y, sag * (1.0 - (x * x + y * y) / r2)) for x, y in [centre] + list(ring)]
	n = len(ring)
	ob = mesh_object(name, verts, [[0, 1 + i, 1 + (i + 1) % n] for i in range(n if centre == (0.0, 0.0) else n - 1)])
	add_mod(ob, "SOLIDIFY", thickness=0.0008, offset=0.0)
	ob.data.materials.append(mat)
	return ob


def smooth_path(p, it=6):
	p = np.array(p, dtype=np.float64)
	for _ in range(it):
		p[1:-1] = 0.25 * p[:-2] + 0.5 * p[1:-1] + 0.25 * p[2:]
	return p


def over_shoulder(bvh, x_top, x_front, x_back, z_front, z_back, zc, n=24, lift=0.0):
	"""A path lying on what bvh holds, from the chest at (x_front, z_front) up over the shoulder at x_top and down the
	back to (x_back, z_back): rays from the front, round the shoulder in the y-z plane about (x_top, 0, zc), from
	behind. Returns smoothed points (lifted off the surface) and normals."""
	pts, nrm = [], []

	def cast(o, d):
		hit = bvh.ray_cast(Vector(tuple(o)), Vector(tuple(d)).normalized(), 2.0)
		if hit[0] is not None:
			pts.append(np.array(hit[0]) + np.array(hit[1]) * lift)
			nrm.append(np.array(hit[1]))
	for t in np.linspace(0, 1, n, endpoint=False):
		cast((x_front + (x_top - x_front) * t, -0.8, z_front + (zc - z_front) * t), (0, 1, 0))
	for p in np.linspace(0, math.pi, n):
		d = np.array([0.0, -math.cos(p), math.sin(p)])
		cast(np.array([x_top, 0.0, zc]) + d * 0.8, -d)
	for t in np.linspace(0, 1, n)[1:]:
		cast((x_top + (x_back - x_top) * t, 0.8, zc + (z_back - zc) * t), (0, -1, 0))
	nrm = smooth_path(nrm, 3)
	return smooth_path(pts), nrm / np.linalg.norm(nrm, axis=1)[:, None]


def ribbon(name, pts, nrm, width, mat, thick=0.004):
	"""A strap along a path lying on a surface (points already lifted, their surface normals)."""
	side = np.cross(np.gradient(pts, axis=0), nrm)
	side /= np.linalg.norm(side, axis=1)[:, None]
	n = len(pts)
	ob = mesh_object(name, np.concatenate([pts - side * width / 2, pts + side * width / 2]), [[i, i + 1, n + i + 1, n + i] for i in range(n - 1)])
	add_mod(ob, "SOLIDIFY", thickness=thick, offset=1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(mat)
	return ob


def chain(name, pts, nrm, mat, pitch=0.022, L=0.03, w=0.011, r=0.0017):
	"""Oval wire links along a path, each turned 90 degrees from the last, as one mesh."""
	nu, nv, a = 20, 6, (L - w) / 2
	t = 2 * math.pi * np.arange(nu) / nu
	cl = np.stack([np.where(np.cos(t) >= 0, a, -a) + np.cos(t) * w / 2, np.sin(t) * w / 2], 1)   # the link's centreline
	tg = np.roll(cl, -1, 0) - np.roll(cl, 1, 0)
	tg /= np.linalg.norm(tg, axis=1)[:, None]
	s = 2 * math.pi * np.arange(nv) / nv
	tmpl = np.array([(x + ty * math.cos(q) * r, y - tx * math.cos(q) * r, math.sin(q) * r) for (x, y), (tx, ty) in zip(cl, tg) for q in s])
	tface = [[i * nv + j, i * nv + (j + 1) % nv, ((i + 1) % nu) * nv + (j + 1) % nv, ((i + 1) % nu) * nv + j] for i in range(nu) for j in range(nv)]
	arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
	verts, faces = [], []
	for k, sk in enumerate(np.arange(0, arc[-1], pitch)):
		p, nn = (np.array([np.interp(sk, arc, P[:, c]) for c in range(3)]) for P in (pts, nrm))
		i = min(np.searchsorted(arc, sk), len(pts) - 1)
		d = pts[i] - pts[max(i - 1, 0)]
		d /= np.linalg.norm(d) + 1e-9
		u = np.cross(nn, d)
		u /= np.linalg.norm(u) + 1e-9
		v = np.cross(d, u)
		if k % 2:
			u, v = v, -u
		faces += [[len(verts) + q for q in f] for f in tface]
		verts += list(p + tmpl[:, :1] * d + tmpl[:, 1:2] * u + tmpl[:, 2:] * v)
	ob = mesh_object(name, np.array(verts), faces)
	ob.data.materials.append(mat)
	return ob


# ---------------------------------------------------------------- B's right arm (a pose hook bust.py lacks)

ARM = {}   # extra pose for the next build: set by build() for B, read by _skin_with_arm
_skin0 = mh._skin


def turn(a, b):
	"""The axis and angle (degrees) that turn direction a onto direction b."""
	a, b = a / np.linalg.norm(a), b / np.linalg.norm(b)
	ax = np.cross(a, b)
	s = np.linalg.norm(ax)
	return (ax / s if s > 1e-9 else np.array([1.0, 0, 0])), math.degrees(math.atan2(s, a @ b))


def arm_pose(bones, pose, elbow, wrist, twist=0.0, curl=60.0, thumb=30.0):
	"""The right arm brought up: the upper arm turned toward elbow, the forearm toward wrist (MakeHuman decimetres;
	the arm keeps its lengths), the forearm twisted and the fingers curled about the knuckle line."""
	b = lambda n: bones[n]["head"]
	Rc = mh._axis_angle(*pose["clavicle.R"])
	sh = Rc @ (b("upperarm01.R") - b("clavicle.R")) + b("clavicle.R")
	u0, f0 = b("lowerarm01.R") - b("upperarm01.R"), b("wrist.R") - b("lowerarm01.R")
	el = sh + (np.asarray(elbow) - sh) / np.linalg.norm(np.asarray(elbow) - sh) * np.linalg.norm(u0)
	wr = el + (np.asarray(wrist) - el) / np.linalg.norm(np.asarray(wrist) - el) * np.linalg.norm(f0)
	ax1, d1 = turn(u0, Rc.T @ (el - sh))
	out = {"upperarm01.R": (ax1, d1), "lowerarm01.R": turn(f0, (Rc @ mh._axis_angle(ax1, d1)).T @ (wr - el)),
		"lowerarm02.R": (f0 / np.linalg.norm(f0), twist)}
	across = b("finger5-1.R") - b("finger2-1.R")
	across /= np.linalg.norm(across)
	for f in (2, 3, 4, 5):
		for j in (1, 2, 3):
			out[f"finger{f}-{j}.R"] = (across, curl * (0.8 if j == 1 else 1.0))
	tax = np.cross(b("finger1-3.R") - b("finger1-1.R"), across)
	out["finger1-2.R"] = out["finger1-3.R"] = (tax / np.linalg.norm(tax), thumb)
	return out


def _skin_with_arm(v, bones, pose):
	"""build_human poses only the arms down and the head; mh.build calls this with the morphed joints, where B's arm
	can be worked out."""
	if ARM:
		pose = dict(pose)
		pose.update(arm_pose(bones, pose, **ARM))
	return _skin0(v, bones, pose)


mh._skin = _skin_with_arm


def handless(body, h):
	"""The body without its right hand's faces, for B's garment shells, so the hand comes bare out of the sleeve.
	Only faces go: the vertices stay, so shell()'s per-vertex weights still line up."""
	w = np.zeros(len(h["verts"]))
	for bone, lst in json.load(open(os.path.join(mh.DATA, "rigs", "default_weights.mhw")))["weights"].items():
		if bone.endswith(".R") and bone.startswith(("wrist", "finger", "metacarpal")):
			for i, x in lst:
				w[i] += x
	w = w[np.unique(np.concatenate(h["group_faces"]("body")))]
	ob = link(bpy.data.objects.new("Handless", body.data.copy()))
	bm = bmesh.new()
	bm.from_mesh(ob.data)
	bmesh.ops.delete(bm, geom=[f for f in bm.faces if max(w[v.index] for v in f.verts) > 0.5], context="FACES_ONLY")
	bm.to_mesh(ob.data)
	bm.free()
	return ob


# ---------------------------------------------------------------- the bowler, the goggles, the spectacles

def build_bowler(h, hf, opt):
	"""A bowler: a domed crown a little belled above the band, a short brim rolled up at the sides, a grosgrain band,
	built in the head's rest space (decimetres) like Ellis's hat. B's is dusty, stove in and its brim knocked down.
	Returns where the goggles ride: a rest point on the crown's front, the crown's normal there, that row of it."""
	hv = hf["verts"]
	band_y = hf["eye_y"] + 0.44
	ring = hv[np.abs(hv[:, 1] - band_y) < 0.12]
	cz = (ring[:, 2].max() + ring[:, 2].min()) * 0.5
	rx, rz = np.abs(ring[:, 0]).max() + 0.08, (ring[:, 2].max() - ring[:, 2].min()) * 0.5 + 0.08
	H = hv[:, 1].max() - band_y + 0.30
	dented = opt["hat"] == "dented"
	ns, na = 96, 30
	ph = np.linspace(0, 2 * math.pi, ns, endpoint=False)
	rows = []
	for i in range(na):
		a = (i / na) * (math.pi / 2)
		r, y = math.cos(a) ** (2 / 2.3), math.sin(a) ** (2 / 2.3) * H
		r *= 1.0 + 0.05 * math.sin(math.pi * min(y / (0.6 * H), 1.0))
		rows.append(np.stack([np.sin(ph) * rx * r, np.full(ns, band_y + y), cz + np.cos(ph) * rz * r], 1))
	v = np.concatenate(rows + [np.array([[0.0, band_y + H, cz]])])
	if dented:   # stove in high on the left front
		k = 0.26 * np.exp(-(np.linalg.norm(v - np.array([0.42 * rx, band_y + 0.78 * H, cz + 0.45 * rz]), axis=1) / 0.34) ** 2)
		d = v - np.array([0.0, band_y, cz])
		v -= d / np.linalg.norm(d, axis=1)[:, None] * k[:, None]
	quads = lambda nr: [[i * ns + j, i * ns + (j + 1) % ns, (i + 1) * ns + (j + 1) % ns, (i + 1) * ns + j] for i in range(nr) for j in range(ns)]
	crown = mesh_object("Crown", head_world(h, v), quads(na - 1) + [[(na - 1) * ns + j, (na - 1) * ns + (j + 1) % ns, na * ns] for j in range(ns)])
	add_mod(crown, "SOLIDIFY", thickness=0.004, offset=-1.0)
	bv = []
	for s in np.linspace(0, 1, 11):
		for p in ph:
			y = 0.30 * s ** 2 * abs(math.sin(p)) ** 1.6 - 0.05 * s * math.cos(p) ** 2
			if dented:   # the brim knocked down on the right
				y -= 0.16 * s * math.exp(-((p + 0.5) / 0.6) ** 2)
			w = 0.40 + 0.10 * abs(math.cos(p))
			bv.append([math.sin(p) * (rx * 0.98 + w * s), band_y + y + 0.01, cz + math.cos(p) * (rz * 0.98 + w * s)])
	bv = np.array(bv)
	brim = mesh_object("Brim", head_world(h, bv), [f[::-1] for f in quads(10)])
	add_mod(brim, "SOLIDIFY", thickness=0.005, offset=0.0, use_rim=True)
	tone = ((0.20, 0.14, 0.10), (0.42, 0.34, 0.26)) if dented else ((0.10, 0.055, 0.03), (0.30, 0.18, 0.10))
	felt = textured("Bowler", ID["felt"], *tone, rough=0.85, sheen=0.5)
	for o in (crown, brim):
		add_mod(o, "SUBSURF", levels=1, render_levels=2)
		o.data.materials.append(felt)
	tube("BrimRoll", head_world(h, bv[-ns:]), 0.0045, felt, cyclic=True)
	bvv = np.concatenate([np.stack([np.sin(ph) * rx * 1.01, np.full(ns, band_y + y0), cz + np.cos(ph) * rz * 1.01], 1) for y0 in (0.02, 0.24)])
	band = mesh_object("HatBand", head_world(h, bvv), quads(1))
	add_mod(band, "SOLIDIFY", thickness=0.0015, offset=1.0)
	band.data.materials.append(textured("Grosgrain", ID["leather"], (0.05, 0.035, 0.03), (0.12, 0.08, 0.06), fine=900.0, rough=0.6))
	i = int(na * 0.22)
	t = (rows[i + 1][0] - rows[i][0]) / np.linalg.norm(rows[i + 1][0] - rows[i][0])
	return rows[i][0], np.array([0.0, -t[2], t[1]]), rows[i]


def goggles(M):
	"""Brass goggles in their own frame (metres; the lenses look along +z): two cups of smoked glass, rims, a bridge."""
	brass, gl = brass_material(), glass((0.62, 0.70, 0.50))
	parts = [tube("GoggleBridge", [(-0.011, 0.002, 0.008), (0.0, 0.007, 0.012), (0.011, 0.002, 0.008)], 0.0028, brass)]
	for sx in (-1.0, 1.0):
		c = np.array([sx * 0.031, 0.0, 0.0])
		lens = disc("GoggleLens", [(0.0185 * math.cos(t), 0.0185 * math.sin(t)) for t in np.linspace(0, 2 * math.pi, 32, endpoint=False)], gl, 0.002)
		lens.location = Vector(tuple(c + (0, 0, 0.0185)))
		parts += [rod("GoggleCup", c, c + (0, 0, 0.018), 0.021, 0.0195, brass), lens,
			tube("GoggleRim", ring_pts(c + (0, 0, 0.0195), np.array([1.0, 0, 0]), np.array([0, 1.0, 0]), 0.0198, 40), 0.0023, brass, cyclic=True)]
	return place(parts, M)


def build_goggles(h, seat, kerchief):
	"""The goggles pushed up on the bowler's crown with their strap round it (seat, A), or down round the neck,
	hanging at the throat over the neckerchief (B)."""
	strap = leather_material((0.16, 0.10, 0.06))
	if seat is not None:
		p, n, row = seat
		HM = head_matrix(h)
		goggles(HM @ frame_matrix(p * 0.1 + n * 0.012, (1, 0, 0), n))
		out = (row - row.mean(axis=0)) * np.array([1.0, 0.0, 1.0])
		out /= np.linalg.norm(out, axis=1)[:, None]
		pts, nrm = head_world(h, row + out * 0.03), (np.array(HM.to_3x3()) @ out.T).T
		return ribbon("GoggleStrap", np.vstack([pts, pts[:1]]), np.vstack([nrm, nrm[:1]]), 0.016, strap, 0.003)
	neck, cz = neck_frame(h), chin_z(h)
	pts = world_points(kerchief)
	rr = np.percentile(np.linalg.norm(pts[np.abs(pts[:, 2] - (cz - 0.06)) < 0.01][:, :2] - neck[:2], axis=1), 85) + 0.014
	goggles(frame_matrix((neck[0], neck[1] - rr - 0.008, cz - 0.085), (1, 0, 0), (0, -0.85, -0.5)))
	return tube("GoggleStrap", [(neck[0] + math.sin(t) * rr, neck[1] - math.cos(t) * rr, cz - 0.06 - 0.022 * max(math.cos(t), 0) ** 2)
		for t in np.linspace(0, 2 * math.pi, 48, endpoint=False)], 0.004, strap, cyclic=True)


def build_specs(h, hf, opt):
	"""Round spectacles in the head's frame (metres): thin brass rims, glass, a saddle bridge, temples back over the
	ears. "cracked": the left lens in shards round a point of impact, each knocked askew (the reflections break), one
	gone, bright hairline cracks; "up": swung about the ear tops up onto the forehead."""
	balls = eyeballs(h)
	brass, gl = brass_material(), glass()
	R, rng = 0.0175, np.random.default_rng(23)
	z0 = max(c[2] + r for c, r in balls.values()) * 0.1 + 0.011
	hv, ey = hf["verts"] * 0.1, hf["eye_y"] * 0.1
	skull = hv[(np.abs(hv[:, 1] - ey - 0.004) < 0.01) & (np.abs(hv[:, 2] - 0.025) < 0.012)]
	ear_x = np.abs(skull[:, 0]).max() + 0.002
	parts, inner = [], []
	circle = lambda r, a0=0.0, a1=2 * math.pi, n=48: [(r * math.cos(t), r * math.sin(t)) for t in np.linspace(a0, a1, n, endpoint=(a1 - a0 < 6.28))]
	for side in ("l", "r"):
		c = balls[side][0] * 0.1
		sx = 1.0 if c[0] > 0 else -1.0
		ML = Matrix.Translation((c[0] + sx * 0.002, c[1] + 0.001, z0)) @ Matrix.Rotation(sx * math.radians(6), 4, "Y") @ Matrix.Rotation(math.radians(6), 4, "X")
		parts.append(tube("SpecRim", [ML @ Vector((x, y, 0)) for x, y in circle(R)], 0.0010, brass, cyclic=True))
		if opt["specs"] == "cracked" and sx > 0:
			q, cuts = (0.004, 0.005), [0.2, 1.3, 2.2, 3.1, 4.3, 5.3, 0.2 + 2 * math.pi]
			crack = flat("Crack", GLASS, (0.95, 0.97, 1.0), rough=0.2)
			for k in range(6):
				e = (R * 0.97 * math.cos(cuts[k]), R * 0.97 * math.sin(cuts[k]))
				parts.append(tube("Crack", [ML @ Vector((q[0] + (e[0] - q[0]) * t, q[1] + (e[1] - q[1]) * t + 0.0008 * math.sin(t * 9), 0.0009)) for t in np.linspace(0, 1, 8)], 0.00035, crack))
				if k != 2:   # that shard is gone
					sh = disc("LensShard", circle(R * 0.97, cuts[k], cuts[k + 1], 10), gl, centre=q)
					tilt = Matrix.Rotation(rng.normal() * 0.16, 4, Vector((math.cos(cuts[k] + 0.6), math.sin(cuts[k] + 0.6), 0)))
					sh.matrix_world = ML @ Matrix.Translation((q[0], q[1], 0)) @ tilt @ Matrix.Translation((-q[0], -q[1], 0))
					parts.append(sh)
		else:
			lens = disc("Lens", circle(R), gl)
			lens.matrix_world = ML
			parts.append(lens)
		inner.append(np.array(ML @ Vector((-sx * R, 0.002, 0))))
		# the temple: from the rim's outer edge back along the skull (kept clear of it), over the ear, hooked behind it
		p0 = np.array(ML @ Vector((sx * R, 0.003, 0)))
		path = [p0, p0 + np.array([sx * 0.004, 0.0, -0.006])]
		for t in np.linspace(0.1, 1.0, 9):
			p = path[1] + (np.array([sx * ear_x, ey + 0.004, 0.004]) - path[1]) * t
			near = hv[(np.abs(hv[:, 1] - p[1]) < 0.015) & (np.abs(hv[:, 2] - p[2]) < 0.015)]
			if len(near) and p[2] > 0.02:
				p[0] = sx * max(abs(p[0]), np.abs(near[:, 0]).max() + 0.003)
			path.append(p)
		path = list(smooth_path(path, 3))
		if opt["specs"] != "up":   # pushed up, they ride on the temples: no hooks behind the ears
			path += [np.array([sx * (ear_x - 0.003), ey - 0.012, -0.012]), np.array([sx * (ear_x - 0.006), ey - 0.03, -0.016])]
		parts.append(tube("SpecTemple", path, 0.0009, brass))
	parts.append(tube("SpecBridge", [inner[0], (inner[0] + inner[1]) / 2 + np.array([0, 0.005, -0.004]), inner[1]], 0.0011, brass))
	M = head_matrix(h)
	if opt["specs"] == "up":
		pivot = Vector((0.0, ey + 0.004, 0.004))
		M = M @ Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(-27), 4, "X") @ Matrix.Translation(-pivot)
	return place(parts, M)


# ---------------------------------------------------------------- hair: chops, mustache, brows, the head

def wave(strands, rng, amp=0.0018, freq=2.5):
	"""Kink each strand a little (a sine across it, growing from the root): dead straight strands read as straw."""
	out = []
	for s in strands:
		p = np.cross(s[-1] - s[0], rng.normal(size=3))
		p /= np.linalg.norm(p) + 1e-9
		t = np.linspace(0, 1, len(s))[:, None]
		out.append(s + p * amp * np.sin(t * freq * math.pi + rng.random() * 6.28) * t)
	return out


def tufts(bvh, h, roots, direction, length, rng, n=40, spread=0.006, out_bias=0.5, pull=0.55, steps=12, clear=0.004):
	"""Clumps of strands from rest-space roots (decimetres) snapped onto the posed skin, grown along direction and to
	length (each a value, or a function of the root's rest point)."""
	out = []
	for r in roots:
		hit = bvh.find_nearest(Vector(tuple(head_world(h, r)[0])))
		base = np.array(hit[0]) + np.array(hit[1]) * 0.002
		d = Vector(tuple(direction(r) if callable(direction) else direction))
		L = (length(r) if callable(length) else length) * (0.85 + 0.3 * rng.random())
		group = [grow(bvh, Vector(tuple(base + rng.normal(size=3) * spread)), (d + Vector(tuple(rng.normal(size=3) * 0.22))).normalized(),
			L * (0.75 + 0.35 * rng.random()), steps, rng, out_bias * (0.6 + 0.8 * rng.random()), clear + 0.004 * rng.random()) for _ in range(n)]
		out += wave(clump(group, pull), rng)
	return out


def combed(bvh, h, roots, comb, length, rng, n, spread, pull, steps, clear):
	"""Clumps lying on the scalp along comb (a world vector, or a function of the root) and falling as they grow."""
	out = []
	for r in roots:
		hit = bvh.find_nearest(Vector(tuple(head_world(h, r)[0])))
		d = Vector(tuple(comb(r) if callable(comb) else comb)).normalized()
		out += clump([grow_combed(bvh, Vector(tuple(np.array(hit[0]) + rng.normal(size=3) * spread)), d, length * (0.7 + 0.6 * rng.random()), steps, rng, clear)
			for _ in range(n)], pull)
	return out


def build_hair(h, hf, body, opt):
	"""Grey mutton chops from the temples down over the jaw, joined across the upper lip by a big mustache, the chin
	bare; bushy brows; under the bowler, short hair round the back; bare-headed (C), a balding crown with a fringe
	and thin strands from a low parting on the left combed across. Roots are picked on the face and scalp at rest
	(MakeHuman decimetres) and grown on the posed body."""
	rng = np.random.default_rng(17)
	bvh, bm = body_bvh(body)
	gr = C_ROT @ h["glob"]["head"][:3, :3]
	ey = hf["eye_y"]
	v = h["morphed"][np.unique(np.concatenate(h["group_faces"]("body")))]
	v = v[v[:, 1] > ey - 1.4]
	ax, ang = np.abs(v[:, 0]), np.degrees(np.arctan2(np.abs(v[:, 0]), v[:, 2] - 0.45))
	top = ey - 0.64 + 0.82 * np.clip((ang - 35) / 65, 0, 1)
	chops = (ax > 0.42) & (ang > 30) & (ang < 104) & (v[:, 1] > ey - np.where(ang < 60, 0.8, 1.2)) & (v[:, 1] < top)
	corner = (ax > 0.30) & (ax < 0.55) & (v[:, 1] > ey - 0.72) & (v[:, 1] < ey - 0.52) & (v[:, 2] > 0.9)
	mous = (ax < 0.42) & (v[:, 1] > ey - 0.63) & (v[:, 1] < ey - 0.47) & (v[:, 2] > 1.1)
	pick = lambda pts, k: pts[rng.choice(len(pts), min(k, len(pts)), replace=False)]
	fwd, down = gr @ np.array([0.0, 0.0, 1.0]), np.array([0.0, 0.0, -1.0])
	out = tufts(bvh, h, pick(v[chops | corner], 160), lambda r: down + 0.2 * fwd + 0.35 * gr @ np.array([np.sign(r[0]), 0.0, 0.0]),
		lambda r: 0.026 + 0.018 * np.clip((ey - r[1]) / 0.9, 0, 1), rng, out_bias=0.3, spread=0.007)
	out += tufts(bvh, h, pick(v[mous], 70), lambda r: gr @ np.array([np.sign(r[0]) * (0.6 + 2.2 * min(abs(r[0]) / 0.4, 1.0)), -0.8, 0.5]),
		lambda r: 0.02 + 0.014 * min(abs(r[0]) / 0.35, 1.0), rng, out_bias=0.6, spread=0.005, pull=0.45)
	hair_objects("Whiskers", out, rng, 0.0013, 0.0003)
	hv = hf["verts"]
	cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5
	if opt["hair"] == "under":
		a = np.radians(-165 + 330 * (np.arange(56) + rng.random(56) * 0.6) / 56)
		a = a[np.abs(a) > math.radians(98)]
		roots = np.stack([np.sin(a) * 0.72, ey + 0.40 - 0.05 * rng.random(len(a)), cz - 0.1 + np.cos(a) * 0.86], 1)
		hair_objects("Hair", tufts(bvh, h, roots, down, 0.026, rng, spread=0.009, out_bias=0.12), rng, 0.0013, 0.0003)
	else:
		a = np.abs(np.degrees(np.arctan2(hv[:, 0], hv[:, 2] - cz)))
		fringe = hv[(a > 78) & (hv[:, 1] > ey - 0.15) & (hv[:, 1] < ey + 0.62) & ~((np.abs(hv[:, 0]) > 0.82) & (hv[:, 1] < ey + 0.3))]
		out = combed(bvh, h, pick(fringe, 60), lambda r: gr @ np.array([np.sign(r[0]) * 0.25, -1.0, -0.45]), 0.025, rng, 22, 0.007, 0.5, 8, 0.002)
		part = []
		for z in -0.35 + 1.25 * np.arange(22) / 21 + rng.normal(size=22) * 0.03:
			p = hv[(np.abs(hv[:, 2] - z) < 0.07) & (hv[:, 0] > 0.45) & (hv[:, 0] < 0.75)]
			if len(p):
				part.append(p[np.argmax(p[:, 1])])
		out += combed(bvh, h, part, gr @ np.array([-1.0, 0.2, -0.25]), 0.11, rng, 7, 0.004, 0.25, 16, 0.0018)
		hair_objects("Hair", out, rng, 0.0012, 0.0003, 0.8)
	# brows: short strands laid along the brow ridge, low, flicking up and out at the ends
	w = lambda p: Vector(tuple(head_world(h, p)[0]))
	out = []
	for e in hf["eyes"]:
		sx = 1.0 if e[0] > 0 else -1.0
		for t, jit, L in zip(rng.random(380), rng.normal(size=380), 0.009 + 0.007 * rng.random(380)):
			x, y = e[0] + sx * (-0.20 + 0.46 * t), e[1] + 0.17 + 0.05 * math.sin(t * math.pi * 0.8) - 0.03 * (1 - t) + jit * 0.016
			p, q = w([x, y, hf["front"] + 0.3]), w([x, y, hf["front"] - 1.0])
			hit = bvh.ray_cast(p, (q - p).normalized(), (q - p).length)
			if hit[0] is not None:
				along = w([x + sx * 0.5, y + 0.10 + 0.12 * t, hf["front"]]) - w([x, y, hf["front"]])
				along = (along - hit[1] * along.dot(hit[1])).normalized()
				out.append(np.array([tuple(hit[0] + hit[1] * (0.0006 + 0.002 * math.sin(i / 3 * math.pi)) + along * (L * i / 3)) for i in range(4)]))
	hair_objects("Brows", out, rng, 0.0006, 0.00015, 0.6)
	bm.free()


# ---------------------------------------------------------------- clothes

def build_kerchief(h, body, over, loose):
	"""The teal neckerchief round the throat, its point down over the chest, a knot with two short tails; loose (C):
	slipped low and open."""
	pts = np.concatenate([world_points(body)] + [world_points(o) for o in over])
	neck, cz = neck_frame(h), chin_z(h)
	pts = pts[np.linalg.norm(pts[:, :2] - neck[:2], axis=1) < 0.11]
	bf, bl = frame2d(np.array([0.0, -1.0]))
	th = np.linspace(-math.pi, math.pi, 120, endpoint=False)
	top = lambda t: cz - 0.04 + 0.025 * (0.5 - 0.5 * math.cos(t)) - (0.035 if loose else 0.0) * max(math.cos(t), 0.0)
	bot = lambda t: top(t) - (0.045 + (0.11 if loose else 0.05) * max(math.cos(t), 0.0) ** 3)
	X, Y, Z, _ = drape(pts, neck[:2] + bf * 0.012, bf, th, top, bot, 40, 0.012, hb=math.radians(5), zb=0.01, smooth=(3, 2), wrap=True)
	v = np.linspace(0, 1, Z.shape[0])[:, None]
	sag = 0.006 * np.sin(v * 16 + th[None] * 2) + 0.004 * np.sin(th[None] * 9 + v * 3)
	ob = grid_object("Kerchief", X + sag * (np.cos(th) * bf[0] + np.sin(th) * bl[0])[None], Y + sag * (np.cos(th) * bf[1] + np.sin(th) * bl[1])[None], Z, wrap=True)
	add_mod(ob, "SOLIDIFY", thickness=0.0016, offset=1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	mat = textured("Kerchief", ID["bandana"], (0.03, 0.22, 0.24), (0.10, 0.44, 0.44), scale=3.0, fine=1100.0, rough=0.8, sheen=0.45)
	ob.data.materials.append(mat)
	i = np.argmin(np.abs(th))
	k = np.array([X[2, i] - 0.004 * loose, Y[2, i] - 0.010, Z[2, i] - 0.010])
	return ob, [ball("KerchiefKnot", k, 0.015, mat, (1.3, 0.75, 0.9))] + [ribbon("KerchiefTail", np.array([k + np.array([s * (0.004 + 0.014 * t), -0.008 - 0.006 * t, -0.006 - 0.05 * t])
		for t in np.linspace(0, 1, 8)]), np.tile([0.0, -1.0, 0.0], (8, 1)), 0.018, mat, 0.002) for s in (-1.0, 1.0)]


def tear(ob, centre, radius, rng):
	"""Rip a ragged hole in a garment (its faces near centre, the edge wandering)."""
	bm = bmesh.new()
	bm.from_mesh(ob.data)
	ph = rng.random(3) * 6.28
	cut = []
	for f in bm.faces:
		d = f.calc_center_median() - Vector(centre)
		a = math.atan2(d.z, d.x)
		if d.length < radius * (0.75 + 0.18 * math.sin(3 * a + ph[0]) + 0.1 * math.sin(7 * a + ph[1]) + 0.06 * math.sin(13 * a + ph[2])):
			cut.append(f)
	bmesh.ops.delete(bm, geom=cut, context="FACES")
	bm.to_mesh(ob.data)
	bm.free()


def build_coat(h, body, opt):
	"""The khaki field coat over a brown waistcoat: open at the throat with lapels, a low rolled collar, patch pockets
	with buttoned flaps on the chest. B's is torn at the left shoulder and leaves the right hand bare."""
	neck, sz = neck_frame(h), shoulder_z(h)
	bf, bl = frame2d(np.array([0.0, -1.0]))
	cloth, brass = canvas_material(*opt["cloth"], name="Khaki"), brass_material()
	src = handless(body, h) if opt.get("spyglass") else body
	under, _ = shell(src, h, "Under", 0.007, 0.0, neck[2] + 0.005, 0.3)
	add_mod(under, "SUBSURF", levels=1, render_levels=2)
	under.data.materials.append(canvas_material((0.09, 0.055, 0.035), (0.22, 0.14, 0.08), "Waistcoat", ID["black"]))
	coat, _ = shell(src, h, "Coat", 0.018, 0.024, neck[2] + 0.02)
	if src is not body:
		bpy.data.objects.remove(src)
	vee = 0.042
	cut_vee(coat, neck, vee)
	if opt.get("torn"):
		tear(coat, (neck[0] + 0.19, neck[1] - 0.05, sz - 0.03), 0.06, np.random.default_rng(4))
	add_mod(coat, "SOLIDIFY", thickness=0.005, offset=1.0)
	add_mod(coat, "SUBSURF", levels=1, render_levels=2)
	coat.data.materials.append(cloth)
	nv = world_points(body)
	nv = nv[np.abs(nv[:, 2] - neck[2]) < 0.012]
	top_r = np.percentile(np.linalg.norm(nv[:, :2] - neck[:2], axis=1), 60) + 0.03
	cv = []
	for t in np.linspace(0, 1, 12):
		for a in np.linspace(math.radians(40), math.radians(320), 120):
			back = 0.5 - 0.5 * math.cos(a)
			r = top_r * (0.92 + 0.24 * t) + 0.004 * math.sin(a * 9 + t * 2) - 0.012 * (1 - back) * t
			d = math.cos(a) * bf + math.sin(a) * bl
			cv.append([neck[0] + d[0] * r, neck[1] + d[1] * r, neck[2] - 0.03 + (0.028 + 0.034 * back) * t])
	cv = np.array(cv)
	collar = grid_object("Collar", *(cv[:, k].reshape(12, -1) for k in range(3)))
	add_mod(collar, "SOLIDIFY", thickness=0.005, offset=1.0)
	add_mod(collar, "SUBSURF", levels=1, render_levels=2)
	collar.data.materials.append(cloth)
	parts = [under, coat, collar] + build_lapels(coat, neck, vee, cloth, under) + buttons(under, neck, 5, 0.04, brass)
	cb = bvh_of(coat)
	for sx in (-1.0, 1.0):   # chest pockets: the patch, its flap standing a little proud, a brass button
		x0, x1 = neck[0] + sx * 0.105, neck[0] + sx * 0.205
		parts.append(strip_on(cb, [[(x0 + (x1 - x0) * i / 5, sz - 0.12 + 0.13 * j / 7, 0.0) for i in range(6)] for j in range(8)], "Pocket", 0.003, cloth, 0.003))
		parts.append(strip_on(cb, [[(x0 - sx * 0.004 + (x1 - x0 + sx * 0.008) * i / 5, sz + 0.012 - 0.045 * j / 4, 0.004 * j / 4) for i in range(6)] for j in range(5)], "Flap", 0.006, cloth, 0.003))
		sp = surface_point(cb, (x0 + x1) / 2, sz - 0.026)
		if sp:
			parts.append(ball("Button", sp[0] + sp[1] * 0.012, 0.0055, brass, (1, 1, 0.6)))
	return parts, coat, collar, under


def build_belt(over, sz):
	"""The cartridge belt from the right shoulder down across the chest, brass rounds standing in its loops."""
	pts, nrm = over_shoulder(bvh_of(*over), -0.10, 0.24, 0.17, sz - 0.15, sz - 0.42, sz - 0.04, lift=0.005)
	belt = ribbon("CartridgeBelt", pts, nrm, 0.052, leather_material((0.26, 0.16, 0.08)), 0.006)
	brass, lead = brass_material(), flat("Lead", ID["lead"], (0.58, 0.52, 0.48), rough=0.35, metal=1.0)
	arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
	for i in np.searchsorted(arc, np.arange(0.02, arc[-1], 0.024)):
		if i < 1 or pts[i, 1] > -0.02 or pts[i, 2] > sz - 0.05:
			continue   # only down the front, below the shoulder
		up = np.cross(pts[i] - pts[i - 1], nrm[i])
		up *= (1.0 if up[2] > 0 else -1.0) / np.linalg.norm(up)
		c = pts[i] + nrm[i] * 0.012
		rod("Case", c - up * 0.02, c + up * 0.02, 0.0058, 0.0058, brass, 12)
		rod("Bullet", c + up * 0.02, c + up * 0.034, 0.0052, 0.0016, lead, 12)
	return belt


def build_pack(over, sz, hz):
	"""The pack's shoulder straps with brass buckles; on the pack, the tripod's three wooden legs rising behind the
	right shoulder in the key light, brass-shod at the top and strapped together, the map tube upright beside them."""
	bvh, leather, brass = bvh_of(*over), leather_material((0.20, 0.12, 0.07)), brass_material()
	for sx in (-1.0, 1.0):
		pts, nrm = over_shoulder(bvh, sx * 0.15, sx * 0.16, sx * 0.15, sz - 0.42, sz - 0.30, sz - 0.04, lift=0.008)
		ribbon("PackStrap", pts, nrm, 0.046, leather, 0.005)
		k = np.argmin(np.abs(pts[:, 2] - (sz - 0.10)) + (pts[:, 1] > 0) * 9)
		t = (pts[k + 1] - pts[k - 1]) / np.linalg.norm(pts[k + 1] - pts[k - 1])
		c, s = pts[k] + nrm[k] * 0.007, np.cross(t, nrm[k])
		tube("Buckle", [c + s * a * 0.029 + t * b * 0.019 for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))], 0.0024, brass, cyclic=True)
	wood = textured("TripodWood", WOOD, (0.34, 0.20, 0.10), (0.62, 0.42, 0.22), scale=5.0, fine=300.0, rough=0.45, sheen=0.0, stretch=(1.0, 1.0, 0.08))
	base, top = np.array([-0.11, 0.14, sz - 0.30]), np.array([-0.235, 0.12, hz + 0.12])
	ax = (top - base) / np.linalg.norm(top - base)
	e1 = np.cross(ax, (0, 1, 0))
	e1 /= np.linalg.norm(e1)
	e2 = np.cross(ax, e1)
	for o in ring_pts(np.zeros(3), e1, e2, 1.0, 3, 0.5):   # turned so all three legs show
		p, q = base + o * 0.014, top + o * 0.024
		d = (q - p) / np.linalg.norm(q - p)
		rod("TripodLeg", p, q, 0.013, 0.011, wood)
		rod("Ferrule", q - d * 0.05, q, 0.0118, 0.0118, brass)
		rod("Shoe", q, q + d * 0.045, 0.0112, 0.004, brass)
		rod("Clamp", p + (q - p) * 0.55, p + (q - p) * 0.55 + d * 0.03, 0.0135, 0.0135, brass)
	tube("TripodStrap", ring_pts(base + (top - base) * 0.8, e1, e2, 0.04), 0.006, leather_material((0.18, 0.11, 0.06)), cyclic=True)
	a, b = np.array([-0.04, 0.20, sz - 0.26]), np.array([-0.15, 0.17, hz + 0.07])   # between the head and the tripod, in the light
	d = (b - a) / np.linalg.norm(b - a)
	rod("MapTube", a, b, 0.034, 0.034, leather_material((0.55, 0.36, 0.18)), 28)
	rod("TubeCap", b - d * 0.04, b + d * 0.012, 0.0365, 0.0355, brass, 28)
	for f in (0.6, 0.85):
		rod("TubeBand", a + (b - a) * f, a + (b - a) * f + d * 0.014, 0.036, 0.036, brass, 28)


# ---------------------------------------------------------------- props

def build_spyglass(h):
	"""B's brass spyglass, shut, in the right fist held to the chest: the grip is read off the posed hand (the
	knuckle line is its axis), the eyepiece end up past the thumb."""
	k2, k5 = bone_world(h, "finger2-1.R"), bone_world(h, "finger5-1.R")
	tips = (bone_world(h, "finger3-3.R", "tail") + bone_world(h, "finger4-3.R", "tail")) * 0.5
	ax = (k2 - k5) / np.linalg.norm(k2 - k5)
	c = ((k2 + k5) * 0.5 + tips) * 0.5
	r = float(np.clip(np.linalg.norm((k2 + k5) * 0.5 - tips) * 0.42, 0.015, 0.021))
	brass = brass_material()
	at = lambda s: c + ax * s
	for name, s0, s1, r0, r1, mat in (("Barrel", -0.12, 0.035, r, r, brass), ("Grip", -0.085, 0.025, r + 0.0012, r + 0.0012, leather_material((0.12, 0.07, 0.04))),
			("Objective", -0.135, -0.115, r + 0.0025, r + 0.0025, brass), ("ObjGlass", -0.136, -0.134, r, r, glass()), ("Ring", 0.031, 0.042, r * 0.9, r * 0.9, brass),
			("Draw", 0.035, 0.07, r * 0.8, r * 0.8, brass), ("Eyepiece", 0.07, 0.085, r * 0.66, r * 0.78, brass)):
		rod(name, at(s0), at(s1), r0, r1, mat, 28)


def build_cut(h, hf):
	"""B's cut over the left brow from the fall, a trickle of blood from its end (id skin: kept crisp)."""
	hv, ey = hf["verts"] * 0.1, hf["eye_y"] * 0.1
	blood = flat("Cut", ID["skin"], (0.42, 0.035, 0.025), rough=0.25, spec=0.6)

	def on_skin(x, y):
		near = hv[(np.abs(hv[:, 0] - x) < 0.006) & (np.abs(hv[:, 1] - y) < 0.006)]
		return np.array([x, y, near[:, 2].max() + 0.0012])
	cut = [on_skin(0.010 + 0.034 * t, ey + 0.029 + 0.006 * math.sin(t * 3.0) - 0.004 * t) for t in np.linspace(0, 1, 12)]
	trickle = [on_skin(0.04 + 0.006 * t + 0.002 * math.sin(t * 4), ey + 0.026 - 0.03 * t) for t in np.linspace(0, 1, 10)]
	place([tube("Cut", cut, 0.0034, blood), tube("Trickle", trickle, 0.0017, blood)], head_matrix(h))


def build_pencil(h, hf):
	"""C's cedar pencil tucked over the right ear, sharpened point forward."""
	hv, ey = hf["verts"] * 0.1, hf["eye_y"] * 0.1
	skull = hv[(np.abs(hv[:, 1] - ey - 0.004) < 0.01) & (np.abs(hv[:, 2]) < 0.015)]
	c = np.array([-(np.abs(skull[:, 0]).max() + 0.016), ey + 0.006, 0.006])
	d = np.array([-0.12, -0.3, 1.0]) / np.linalg.norm([-0.12, -0.3, 1.0])
	cedar = textured("Pencil", WOOD, (0.62, 0.36, 0.16), (0.78, 0.50, 0.26), scale=30.0, fine=300.0, rough=0.5, sheen=0.0)
	place([rod("Pencil", c - d * 0.06, c + d * 0.065, 0.0042, 0.0042, cedar, 6), rod("PencilCone", c + d * 0.065, c + d * 0.083, 0.0042, 0.0012, cedar, 6),
		rod("PencilLead", c + d * 0.083, c + d * 0.088, 0.0012, 0.0002, flat("Graphite", ID["black"], (0.05, 0.05, 0.055), rough=0.3, metal=0.6), 6)], head_matrix(h))


def build_map(sz):
	"""C's rolled map clamped under the left arm, its end poking forward: the roll, the inner turns showing, a tie."""
	paper = textured("Paper", ID["linen"], (0.62, 0.55, 0.42), (0.86, 0.80, 0.66), scale=8.0, fine=600.0, rough=0.8, sheen=0.1)
	a, b = np.array([0.22, 0.10, sz - 0.05]), np.array([0.07, -0.26, sz - 0.085])
	d = (b - a) / np.linalg.norm(b - a)
	rod("MapRoll", a, b, 0.03, 0.03, paper, 28)
	for r, s in ((0.023, 0.01), (0.015, 0.02), (0.008, 0.012)):
		rod("MapTurn", b - d * 0.01, b + d * s, r, r * 0.97, paper, 24)
	e1 = np.cross(d, (0, 0, 1))
	e1 /= np.linalg.norm(e1)
	tube("MapTie", ring_pts(b - d * 0.09, e1, np.cross(d, e1), 0.032), 0.0018, flat("Twine", ID["leather"], (0.36, 0.12, 0.08), rough=0.8), cyclic=True)


def build_chain(over, sz):
	"""C's measuring chain coiled over the right shoulder: three loops of wire links, a brass tally on each."""
	bvh, iron, brass = bvh_of(*over), flat("Iron", ID["lead"], (0.36, 0.35, 0.34), rough=0.45, metal=1.0), brass_material()
	for k in range(3):
		pts, nrm = over_shoulder(bvh, -0.125 - 0.018 * k, -0.07 - 0.06 * k, -0.16, sz - 0.42, sz - 0.36, sz - 0.04, lift=0.008 + 0.004 * k)
		chain("Chain", pts, nrm, iron)
		i = np.argmin(np.abs(pts[:, 2] - (sz - 0.06 - 0.05 * k)) + (pts[:, 1] > 0) * 9)
		ball("Tally", pts[i] + nrm[i] * 0.006, 0.009, brass, (1, 1, 0.35))


# ---------------------------------------------------------------- the bust

def build(opt):
	"""Pike's bust for one option, on an empty scene. Returns (h, eyes) for bust.run."""
	ARM.clear()
	ARM.update(opt.get("arm", {}) if opt.get("spyglass") else {})
	h, hf, body, eyes = build_human(opt, FACE, MACRO, "Pike")
	ARM.clear()
	if opt["eyes"] == "life":
		for e in eyes:
			e.data.materials.clear()
			e.data.materials.append(life_eye())
	sz, hz = shoulder_z(h), mh.to_blender(mh.apply(h["glob"]["head"], h["bones"]["head"]["head"][None]))[0][2]
	seat = build_bowler(h, hf, opt) if opt["hat"] else None
	build_specs(h, hf, opt)
	build_hair(h, hf, body, opt)
	parts, coat, collar, under = build_coat(h, body, opt)
	kerchief, knot = build_kerchief(h, body, [under, coat, collar], opt["kerchief"] == "loose")
	over = parts + [kerchief] + knot   # the waistcoat too: the coat is open down the front
	if opt.get("belt"):
		over.append(build_belt(over, sz))
	if opt.get("pack"):
		build_pack(over, sz, hz)
	if opt["goggles"]:
		build_goggles(h, seat if opt["goggles"] == "hat" else None, kerchief)
	if opt.get("spyglass"):
		build_spyglass(h)
	if opt.get("cut"):
		build_cut(h, hf)
	if opt.get("pencil"):
		build_pencil(h, hf)
	if opt.get("map"):
		build_map(sz)
	if opt.get("chain"):
		build_chain(over, sz)
	return h, eyes


def main():
	run(OPTIONS, "Pike", build)


if __name__ == "__main__":
	main()
