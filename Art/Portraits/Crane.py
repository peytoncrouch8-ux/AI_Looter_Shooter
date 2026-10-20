"""Hollis Crane's portrait busts (the Unpaid: a marshal dead forty years and still walking) in Blender (Cycles), from
MakeHuman's CC0 base mesh, on the machinery every hero's bust shares (bust.py, bustkit.py; run it as EllisBlender.py):
	blender -b --factory-startup --python-exit-code 1 --python Art/Portraits/Crane.py -- <A|B|C> [--size N] [--samples N]
	node Art/Portraits/render.js Crane<X> --script Crane.py --passes <dir>
  A  The Marshal: slouch hat, coin on the left eye, cyan in the right socket, ragged cape, tin star, toll chain and hook.
  B  Forty Years Walking: bald crown and wild thin hair, both sockets cyan, cape in threads, brighter ember, chain slack.
  C  Lawman, Once: as buried: hat squared, mustache waxed, star polished, string tie, the coin; one faint eye, no bloom.
The stoop, the turned bust, the chain's links and the node shorthands are bustkit's.
"""
import math
import os
import sys

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
# looter_model's fixed-order sphere for the tin star (it reaches the game model through Tools/Blender/crane_gear.py)
_TOOLS = os.path.join(os.path.dirname(os.path.dirname(HERE)), "Tools", "Blender")
if _TOOLS not in sys.path:
	sys.path.append(_TOOLS)
import looter_model as lm  # noqa: E402
import mh  # noqa: E402
from bust import (  # noqa: E402
	ID, add_mod, bump, bvh_of, canvas_material, clump, curves_object, drape, frame2d, grid_object, grow, grow_combed,
	hair_material, leather_material, lin, link, material, mesh_object, mix_rgb, neck_frame, noise, principled, run, shell,
	shoulder_z, strip_on, surface_point, world_points)
from bustkit import (  # noqa: E402
	build_lapels, build_shirt_collar, build_string_tie, buttons, cut_vee, edit_faces, face_counts_as_head, head_pt,
	link_chain, link_shape, nd, point_light, ramp, stooped, tube_data, turn_scene)

# The paint pass's settings per option (render.js reads these lines): swamp and bone; C warmer, the burial parlour.
# PAINT A {"bgTop":"#0b1412","bgBottom":"#040807","bgGlow":"#2a4a40","glowAt":[0.70,0.30],"sparks":"#a8ffe0","dust":"#8fa89c","glow":"#5ad8ff"}
# PAINT B {"bgTop":"#08110f","bgBottom":"#030605","bgGlow":"#1d4642","glowAt":[0.30,0.32],"sparks":"#a8ffe0","dust":"#7f9a90","glow":"#5ad8ff"}
# PAINT C {"bgTop":"#11130f","bgBottom":"#050604","bgGlow":"#3e4234","glowAt":[0.68,0.28],"sparks":"#e6d2a0","dust":"#a8a690","glow":"#5ad8ff"}

# An old man, very thin (the fetched targets have no age macro: head-age and the face below age him).
MACRO = {"gender": 1.0, "muscle": 0.2, "weight": 0.0, "caucasian": 0.8, "african": 0.1, "asian": 0.1}

# A man who died at sixty: a long aged head, skin tight on the bone (cheekbones up, cheeks and temples sunk), a long
# hooked nose, thin lips turned down, a narrow jutting jaw, a heavy brow, bagged hooded eyes, big old ears, a stringy neck.
FACE = {
	"head/head-age": 1.0, "head/head-scale-vert": 0.35, "head/head-rectangular": 0.45, "forehead/forehead-temple": -1.0,
	"eyebrows/eyebrows-trans-forward": 0.6, "eyebrows/eyebrows-angle": -0.2, "chin/chin-prominent": 0.45, "chin/chin-bones": 0.3,
	"chin/chin-width": -0.35, "chin/chin-height": 0.3, "nose/nose-scale-vert": 0.45, "nose/nose-hump": 0.6, "nose/nose-curve": 0.4,
	"nose/nose-point-width": -0.3, "nose/nose-width1": -0.35, "nose/nose-trans": 0.25, "mouth/mouth-angles": -0.7,
	"mouth/mouth-upperlip-volume": -0.7, "mouth/mouth-lowerlip-volume": -0.5, "neck/neck-scale-horiz": -0.7,
	"neck/neck-scale-depth": -0.3, "torso/torso-scale-horiz": -0.35, "torso/torso-vshape": -0.4}
for _s in "lr":
	FACE.update({f"cheek/{_s}-cheek-volume": -1.0, f"cheek/{_s}-cheek-bones": 0.75, f"cheek/{_s}-cheek-inner": -0.7,
		f"eyes/{_s}-eye-bag": 0.6, f"eyes/{_s}-eye-eyefold": -0.4, f"eyes/{_s}-eye-scale": -0.1,
		f"ears/{_s}-ear-scale": 0.35, f"ears/{_s}-ear-flap": 0.15})
ONE_FILE = ("eyebrows/eyebrows-trans-forward",)   # targets kept as one file, no decr/incr pair: one-sided

CYAN = (0.25, 0.85, 1.0)
# turn: the whole bust about the vertical (degrees, + toward the picture's right) so the stoop shows; yaw: the head's
# turn on top of it (the face's net turn = turn + yaw); stoop: the upper spine's forward bend, pitch looks back up out
# of it. sockets: (left eye "coin" or "glow", glow weight). coat: cloth tones, vee, shirt colour, tear size, ember light
# watts, ember and star places (metres right of the neck, below it).
OPTIONS = {
	"A": {"label": "The Marshal", "line": "Slouch hat and a ferryman's coin on one eye, cyan in the other; the toll chain hooked over his shoulder.",
		"turn": 20.0, "yaw": -40.0, "pitch": -40.0, "stoop": 24.0, "lids": (0.014, 0.008, 0.3), "eyes": "slit", "skin": (0.50, 0.58, 0.48),
		"glow": CYAN, "sockets": ("coin", 1.0), "sunken": 1.0, "hat": "slouch", "hair": "long", "mustache": "droop",
		"cape": "ragged", "chain": "hooked", "ember": "tear", "star": "tin",
		"coat": {"cloth": ((0.035, 0.05, 0.065), (0.15, 0.19, 0.23)), "vee": 0.035, "shirt": (0.52, 0.50, 0.42), "tear": 0.026,
			"ember_w": 0.5, "ember_z": 0.13, "star": (0.088, 0.125)},
		"world": (0.06, 0.10, 0.09), "cam": (2.1, -0.03),
		"lights": [("Key", (-0.80, -0.55, 0.15), 26.0, (1.0, 0.80, 0.60), 0.5), ("Rim", (1.25, 1.15, 0.45), 80.0, (0.45, 0.88, 1.0), 0.8),
			("Fill", (0.9, -1.6, -0.3), 8.0, (0.5, 0.7, 0.8), 1.5)]},
	"B": {"label": "Forty Years Walking", "line": "Hat gone, hair wild and thin, both sockets burning; the coat open on the ember, the chain slack.",
		"turn": 20.0, "yaw": 12.0, "pitch": -45.0, "stoop": 28.0, "lids": (0.002, 0.0, 0.1), "eyes": "slit", "skin": (0.42, 0.53, 0.47),
		"glow": CYAN, "sockets": ("glow", 1.0), "sunken": 1.5, "hat": "none", "hair": "wild", "mustache": "droop",
		"cape": "threads", "chain": "slack", "ember": "open", "star": "tin",
		"face": {"cheek/l-cheek-inner": -1.0, "cheek/r-cheek-inner": -1.0, "eyes/l-eye-bag": 0.9, "eyes/r-eye-bag": 0.9},
		"coat": {"cloth": ((0.06, 0.07, 0.08), (0.20, 0.23, 0.25)), "vee": 0.085, "shirt": (0.44, 0.43, 0.36), "tear": 0.045,
			"ember_w": 1.2, "ember_z": 0.13, "star": (0.115, 0.12)},
		"world": (0.05, 0.09, 0.09), "cam": (2.1, -0.03),
		"lights": [("Key", (0.82, -0.50, 0.22), 24.0, (1.0, 0.84, 0.68), 0.5), ("Rim", (-1.3, 1.1, 0.45), 90.0, (0.45, 0.88, 1.0), 0.8),
			("Fill", (-0.9, -1.6, -0.3), 7.0, (0.5, 0.7, 0.8), 1.5)]},
	"C": {"label": "Lawman, Once", "line": "As he was buried: hat squared, mustache waxed, the star polished and the coin laid on his eye.",
		"turn": -15.0, "yaw": -5.0, "pitch": -22.0, "stoop": 10.0, "lids": (0.012, 0.008, 0.2), "eyes": "slit", "skin": (0.64, 0.64, 0.60),
		"glow": CYAN, "sockets": ("coin", 0.3), "sunken": 0.45, "hat": "squared", "hair": "combed", "mustache": "waxed",
		"cape": "none", "chain": "none", "ember": "warm", "star": "polished",
		"coat": {"cloth": ((0.025, 0.03, 0.04), (0.09, 0.11, 0.14)), "vee": 0.06, "shirt": (0.80, 0.78, 0.73), "tie": True,
			"lapels": True, "tear": 0.0, "ember_w": 0.4, "ember_z": 0.12, "star": (0.10, 0.11)},
		"world": (0.09, 0.09, 0.08), "cam": (2.05, -0.03),
		"lights": [("Key", (-0.82, -0.55, 0.10), 24.0, (1.0, 0.80, 0.58), 0.5), ("Rim", (1.25, 1.1, 0.4), 80.0, (0.55, 0.85, 1.0), 0.7),
			("Fill", (0.9, -1.6, -0.3), 9.0, (0.6, 0.65, 0.75), 1.5)]},
}


# ---------------------------------------------------------------- materials

def metal_material(name, dark, light, rough, rust=0.0, scale=60.0):
	"""Iron, tin: two tones mottled by world position (the links share one mesh), rust in patches."""
	def b(nt):
		pos = nd(nt, "ShaderNodeNewGeometry").outputs["Position"]
		col = mix_rgb(nt, noise(nt, scale, 6.0, 0.6, pos).outputs["Fac"], dark, light)
		if rust > 0:
			mr = nd(nt, "ShaderNodeMapRange", {"Value": noise(nt, scale * 0.3, 5.0, 0.6, pos).outputs["Fac"], "From Min": 0.5, "From Max": 0.62, "To Max": rust})
			col = mix_rgb(nt, mr.outputs[0], col, (0.36, 0.16, 0.07))
		return principled(nt, col, rough=rough, spec=0.5, metal=0.9)
	return material(name, ID["lead"], b)


def shirt_material(base):
	"""Old linen, mottled with stains."""
	return material("Shirt", ID["shirt"], lambda nt: principled(nt, mix_rgb(nt, noise(nt, 9.0, 6.0, 0.6,
		nd(nt, "ShaderNodeTexCoord").outputs["Object"]).outputs["Fac"], tuple(x * 0.62 for x in base), base), rough=0.75, spec=0.3, **{"Sheen Weight": 0.2}))


def ember_material(P, size, color, strength):
	"""The ember behind the torn shirt: a white-hot heart in the Revenant's cyan, cracking toward its edge."""
	def b(nt):
		obj = nd(nt, "ShaderNodeTexCoord").outputs["Object"]
		d = nd(nt, "ShaderNodeVectorMath", {0: obj, 1: tuple(P)}, "DISTANCE")
		m = nd(nt, "ShaderNodeMath", {0: noise(nt, 140.0, 5.0, 0.65, obj).outputs["Fac"], 1: size * 0.9, 2: d.outputs["Value"]}, "MULTIPLY_ADD")
		mr = nd(nt, "ShaderNodeMapRange", {"Value": m.outputs[0], "From Max": size * 2.4})
		col = ramp(nt, mr.outputs[0], ((0.23, (1, 1, 1)), (0.42, color), (0.62, tuple(c * 0.4 for c in color)), (1.0, (0, 0, 0))))
		return nd(nt, "ShaderNodeEmission", {"Color": col, "Strength": strength})
	return material("Ember", ID["eye"], b)   # the eye's id: the aux pass marks it to bloom


# ---------------------------------------------------------------- the face: skin on the bone, the sockets, the coin

def deepen(body, h, hf, k):
	"""Skin tight on the bone: the cheeks under the cheekbones and the temples pushed in (metres along the normal),
	the zones painted darker there and round the sockets, a cold green in the hollows (k: how far gone)."""
	used = np.unique(np.concatenate(h["group_faces"]("body")))
	loc = mh.apply(np.linalg.inv(h["glob"]["head"]), h["verts"][used])
	attr = body.data.color_attributes["zones"]
	c = np.empty(len(used) * 4, np.float32)
	attr.data.foreach_get("color", c)
	c = c.reshape(-1, 4)
	push = np.zeros(len(used))
	for e in hf["eyes"]:
		sx = math.copysign(1.0, e[0])
		for at, rad, sq, tint, depth in (((0, 0.02, 0.05), 0.30, (1.0, 1.15, 1.0), (0.50, 0.46, 0.42), 0.0),
				((sx * 0.12, -0.45, -0.30), 0.30, (1.0, 0.8, 1.0), (0.34, 0.26, 0.30), 0.0045),
				((sx * 0.40, 0.30, -0.55), 0.32, (1.0, 1.0, 1.0), (0.26, 0.20, 0.24), 0.0025)):
			s = np.clip(1.0 - np.linalg.norm((loc - (e + np.array(at))) * np.array(sq), axis=1) / rad, 0.0, 1.0)
			c[:, :3] *= 1.0 - np.minimum(s ** 1.4 * k, 1.0)[:, None] * np.array(tint)
			push += depth * min(k, 1.3) * (s * s * (3 - 2 * s))
	attr.data.foreach_set("color", c.ravel())
	co, nr = np.empty(len(used) * 3), np.empty(len(used) * 3)
	body.data.vertices.foreach_get("co", co)
	body.data.vertices.foreach_get("normal", nr)
	body.data.vertices.foreach_set("co", co - nr * np.repeat(push, 3))
	body.data.update()
	for n in body.data.materials[0].node_tree.nodes:
		if n.type == "BSDF_PRINCIPLED":
			n.inputs["Subsurface Weight"].default_value = 0.12   # dead flesh: little light under the skin


def build_coin(eye, axes, bvh, tarnish):
	"""The ferryman's coin over the left eye: a bevelled disc resting on brow and cheekbone (a plane fitted to the
	socket's rim), its worn face a robed figure poling a boat under a lantern in a ring of beads (no real money's)."""
	fwd, up, left = axes
	R, T = 0.0172, 0.0024
	P = []
	for k in range(16):
		a = 2 * math.pi * k / 16
		hit = bvh.ray_cast(eye.location + (left * math.cos(a) + up * math.sin(a)) * R * 0.9 + fwd * 0.08, -fwd, 0.16)
		if hit[0] is not None:
			P.append(np.array(hit[0]))
	P = np.array(P)
	c0 = P.mean(axis=0)
	n = np.linalg.svd(P - c0)[2][2]
	n = n if n @ np.array(fwd) > 0 else -n
	z = Vector(n)
	y = (up - z * up.dot(z)).normalized()
	M = Matrix.Translation(Vector(c0 + n * (((P - c0) @ n).max() - 0.0008))) @ (Matrix((y.cross(z), y, z)).transposed() @ Matrix.Rotation(0.25, 3, "Z")).to_4x4()
	nr, ns = 34, 112
	r = np.linspace(0, 1, nr + 1)[1:][:, None]
	ph = np.linspace(0, 2 * math.pi, ns, endpoint=False)[None, :]
	X, Y = r * np.cos(ph), r * np.sin(ph)
	ss = lambda a, b, x: np.clip((x - a) / (b - a), 0, 1) ** 2 * (3 - 2 * np.clip((x - a) / (b - a), 0, 1))
	rim = ss(0.84, 0.9, r) * (1 - ss(0.95, 1.0, r)) + 0.35 * ss(0.95, 1.0, r) * (1 - ss(0.99, 1.0, r))
	bead = sum(np.exp(-((X - 0.76 * math.cos(a)) ** 2 + (Y - 0.76 * math.sin(a)) ** 2) / 0.0012) for a in np.linspace(0, 2 * math.pi, 30, endpoint=False))
	motif = np.maximum.reduce([   # hull, pole, lantern, robe, head: soft-edged shapes
		ss(-0.03, 0.03, np.minimum.reduce([Y + 0.36 - 0.55 * X ** 2, -0.16 - Y, 0.52 - np.abs(X)])),
		ss(-0.02, 0.02, np.minimum.reduce([0.035 - np.abs(X - 0.2 + 0.12 * (Y + 0.16)), Y + 0.16, 0.42 - Y])),
		ss(-0.02, 0.02, 0.085 - np.hypot(X - 0.15, Y - 0.49)),
		ss(-0.03, 0.03, np.minimum.reduce([0.17 * (0.32 - Y) / 0.48 - np.abs(X + 0.12), Y + 0.16, 0.32 - Y])),
		ss(-0.02, 0.02, 0.075 - np.hypot(X + 0.12, Y - 0.38))])
	motif *= 1.0 - tarnish * 0.6 * (0.5 + 0.5 * np.sin(5 * X + 3 * Y + 1.0) * np.cos(4 * Y - 2 * X))   # rubbed smooth
	Z = 0.00055 * rim + 0.00022 * bead * (1 - ss(0.84, 0.88, r)) + 0.0004 * motif
	verts = np.concatenate([[[0, 0, 0.0004 * motif.max()]], np.stack([X * R, Y * R, Z], -1).reshape(-1, 3),
		np.stack([np.cos(ph[0]) * R, np.sin(ph[0]) * R, np.full(ns, -T)], 1), [[0, 0, -T]]])
	o, e, bk = 1 + (nr - 1) * ns, 1 + nr * ns, 1 + nr * ns + ns
	faces = [[0, 1 + (j + 1) % ns, 1 + j] for j in range(ns)] + [[o + j, o + (j + 1) % ns, e + (j + 1) % ns, e + j] for j in range(ns)]
	faces += [[1 + i * ns + j, 1 + i * ns + (j + 1) % ns, 1 + (i + 1) * ns + (j + 1) % ns, 1 + (i + 1) * ns + j] for i in range(nr - 1) for j in range(ns)]
	faces += [[bk, e + j, e + (j + 1) % ns] for j in range(ns)]
	ob = mesh_object("Coin", verts, faces)
	relief = np.concatenate([[1.0], (Z / Z.max()).ravel(), np.zeros(ns + 1)])
	ob.data.color_attributes.new("relief", "FLOAT_COLOR", "POINT").data.foreach_set("color", np.repeat(relief, 4).astype(np.float32))
	ob.matrix_world = M

	def b(nt):
		gold = mix_rgb(nt, noise(nt, 2600.0, 4.0, 0.6).outputs["Fac"], (0.70, 0.50, 0.20), (0.95, 0.76, 0.36))
		a = nd(nt, "ShaderNodeAttribute")
		a.attribute_name = "relief"
		fac = nd(nt, "ShaderNodeMapRange", {"Value": a.outputs["Fac"], "To Min": 0.45})   # the field dull, the design rubbed bright
		col = mix_rgb(nt, tarnish * 0.3, mix_rgb(nt, fac.outputs[0], (0.36, 0.28, 0.12), gold), (0.22, 0.24, 0.13))   # verdigris
		return principled(nt, col, rough=0.36, spec=0.5, metal=1.0)   # rough enough to catch the key under the brim
	ob.data.materials.append(material("Coin", ID["brass"], b))
	return ob


def sockets(eyes, axes, opt, bvh):
	"""The coin on the left eye (A, C), or it burns too (B); the right ball set deeper with a cyan lamp in the socket.
	C's faint eye takes skin's id: painted crisp, left out of the bloom."""
	fwd = axes[0]
	left_kind, w = opt["sockets"]
	for i, e in enumerate(eyes):
		if i == 0 and left_kind == "coin":
			e.data.materials[0] = material("DeadEye", ID["black"], lambda nt: principled(nt, (0.05, 0.05, 0.05), rough=0.4))
			build_coin(e, axes, bvh, 1.0 if w >= 1.0 else 0.15)
			continue
		e.location -= fwd * 0.0035
		e.scale = (0.9, 0.9, 0.9)
		if w < 1.0:
			e.data.materials[0] = material("FaintEye", ID["skin"], lambda nt: nd(nt, "ShaderNodeEmission",
				{"Color": (*lin(tuple(c * 0.7 for c in opt["glow"])), 1), "Strength": 0.9}))
		point_light(f"Socket{i}", e.location + fwd * 0.011, 0.035 * w, opt["glow"], 0.003)


# ---------------------------------------------------------------- hat, hair, mustache, brows

def build_hat(h, hf, opt):
	"""The slouch hat: a low round crown pushed in on top, a wide soft brim drooping at the sides and back, battered,
	a bite out of it (A); squared and brushed for the burial (C). In the head's rest space (decimetres), as Ellis's."""
	if opt["hat"] == "none":
		return []
	sq = opt["hat"] == "squared"
	hv = hf["verts"]
	band_y = hf["eye_y"] + 0.55
	ring = hv[np.abs(hv[:, 1] - band_y) < 0.12]
	cz = (ring[:, 2].max() + ring[:, 2].min()) * 0.5
	rx, rz = np.abs(ring[:, 0]).max() + 0.13, (ring[:, 2].max() - ring[:, 2].min()) * 0.5 + 0.13
	H = 1.05 if sq else 0.92
	rot = np.array(Matrix.Rotation(math.radians(3 if sq else 5), 3, "X") @ Matrix.Rotation(math.radians(0 if sq else -5), 3, "Z"))
	pivot = np.array([0.0, band_y, cz])
	place = lambda p: mh.to_blender(mh.apply(h["glob"]["head"], (p - pivot) @ rot.T + pivot))
	ph0 = np.random.default_rng(31).random(3) * 6.283
	batter = lambda ph, t: 0.0 if sq else np.sin(3 * ph + ph0[0] + 2 * t) * 0.5 + np.sin(5 * ph + ph0[1]) * 0.3 + np.sin(2 * ph + ph0[2] + 3 * t) * 0.4
	na, nseg = 30, 96
	phs = np.linspace(0, 2 * math.pi, nseg, endpoint=False)
	ex = 2 / 5.0 if sq else 2 / 3.0   # squarer crown for the burial, rounder slouch
	verts = []
	for i in range(na):
		a = (i / na) * (math.pi / 2)
		r, y = abs(math.cos(a)) ** ex, abs(math.sin(a)) ** ex * H
		r *= 1.0 - 0.10 * (y / H) ** 2
		for ph in phs:
			k = r * (1.0 + 0.03 * batter(ph, y))
			verts.append([math.sin(ph) * rx * k * (1 - 0.08 * max(math.cos(ph), 0) * y / H), band_y + y, cz + math.cos(ph) * rz * k])
	verts = np.array(verts + [[0.0, band_y + H, cz]])
	top, zn = (verts[:, 1] - band_y) / H, (verts[:, 2] - cz) / rz
	if sq:   # a proper crease and front pinches
		verts[:, 1] -= 0.26 * np.exp(-(verts[:, 0] / 0.2) ** 2) * np.clip((top - 0.55) / 0.45, 0, 1) ** 1.2 * (1 - 0.35 * zn ** 2)
		verts[:, 0] -= np.sign(verts[:, 0]) * 0.15 * np.exp(-((zn - 0.62) / 0.28) ** 2) * np.clip((top - 0.35) / 0.65, 0, 1) ** 1.3
	else:    # pushed in on top, off-centre, as if sat on
		verts[:, 1] -= 0.22 * np.exp(-((verts[:, 0] + 0.08) / 0.4) ** 2 - (zn / 0.8) ** 2) * np.clip((top - 0.5) / 0.5, 0, 1)
	faces = [[i * nseg + j, i * nseg + (j + 1) % nseg, (i + 1) * nseg + (j + 1) % nseg, (i + 1) * nseg + j] for i in range(na - 1) for j in range(nseg)]
	crown = mesh_object("HatCrown", place(verts), faces + [[(na - 1) * nseg + j, (na - 1) * nseg + (j + 1) % nseg, na * nseg] for j in range(nseg)])
	add_mod(crown, "SOLIDIFY", thickness=0.004, offset=-1.0)
	# Brim: drooping at sides and back (slouch) or curled at the sides (squared); the front kept clear of the eyes.
	bv = []
	for s in np.linspace(0, 1, 17):
		for ph in phs:
			f, bk, sd = max(math.cos(ph), 0.0), max(-math.cos(ph), 0.0), abs(math.sin(ph))
			wid = 0.95 if sq else 1.05 - 0.15 * f
			y = (0.26 * s ** 1.6 * sd ** 2.2 - 0.08 * s ** 2 * f ** 3 - 0.05 * s ** 2 * bk ** 3) if sq else \
				-(0.10 * f ** 2 + 0.34 * sd ** 2 + 0.30 * bk ** 2) * s ** 1.5 + 0.07 * batter(ph, s) * s ** 2
			bv.append([math.sin(ph) * (rx * 0.97 + wid * s), band_y + y + 0.01, cz + math.cos(ph) * (rz * 0.97 + wid * s)])
	bf = [[i * nseg + j, (i + 1) * nseg + j, (i + 1) * nseg + (j + 1) % nseg, i * nseg + (j + 1) % nseg] for i in range(16) for j in range(nseg)]
	if not sq:   # a bite torn out of the brim on the left
		bf = [f for k, f in enumerate(bf) if not ((k // nseg) / 16 > 0.55 + 2.5 * abs(phs[k % nseg] - 1.25))]
	brim = mesh_object("HatBrim", place(np.array(bv)), bf)
	add_mod(brim, "SOLIDIFY", thickness=0.005, offset=0.0, use_rim=True)
	band = mesh_object("HatBand", place(np.array([[math.sin(p) * rx * 1.012, band_y + y, cz + math.cos(p) * rz * 1.012] for y in (0.02, 0.18) for p in phs])),
		[[j, (j + 1) % nseg, (j + 1) % nseg + nseg, j + nseg] for j in range(nseg)])
	add_mod(band, "SOLIDIFY", thickness=0.0015, offset=1.0)
	dark, light = ((0.07, 0.07, 0.065), (0.20, 0.19, 0.17)) if sq else ((0.08, 0.08, 0.07), (0.27, 0.26, 0.22))

	def felt(nt):
		obj = nd(nt, "ShaderNodeTexCoord").outputs["Object"]
		b = principled(nt, mix_rgb(nt, noise(nt, 6.0, 6.0, 0.6, obj).outputs["Fac"], dark, light), rough=0.92, spec=0.2,
			**{"Sheen Weight": 0.6, "Sheen Roughness": 0.4})
		nt.links.new(bump(nt, noise(nt, 400.0, 3.0, 0.7, obj).outputs["Fac"], 0.15, 0.0005), b.inputs["Normal"])
		return b
	fm = material("Felt", ID["felt"], felt)
	for o in (crown, brim, band):
		add_mod(o, "SUBSURF", levels=1, render_levels=2)
		o.data.materials.append(fm if o is not band else leather_material((0.06, 0.05, 0.045)))
	return [crown, brim, band]


def build_hair(h, hf, opt, bvh):
	"""Long white hair: lank from under the brim (A), combed (C); wild and thin round a bald crown, frizzed (B)."""
	rng = np.random.default_rng(7)
	hv, ey = hf["verts"], hf["eye_y"]
	cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5
	to_w = lambda p: head_pt(h, p)
	out = []
	if opt["hair"] != "wild":
		ring = hv[np.abs(hv[:, 1] - (ey + 0.46)) < 0.05]
		ra = np.degrees(np.arctan2(ring[:, 0], ring[:, 2] - cz))
		for c in range(60):
			ang = -165 + 330 * (c + rng.random() * 0.7) / 60
			if abs(ang) < 60:
				continue   # the face
			L = (0.15 + 0.15 * min(1.0, abs(ang) / 165) ** 1.3) * (0.85 + 0.3 * rng.random())
			hit = bvh.find_nearest(to_w(ring[np.argmin(np.abs((ra - ang + 180) % 360 - 180))]))
			base = np.array(hit[0]) + np.array(hit[1]) * 0.005
			group = [grow(bvh, Vector(base + rng.normal(size=3) * 0.009), Vector((0, 0, -1)), L * (0.75 + 0.35 * rng.random()), 14, rng,
				0.3 + 0.3 * rng.random(), 0.005 + 0.006 * rng.random()) for _ in range(24)]
			if opt["hair"] != "combed":   # lank and kinked, not a wig's sheet
				ph, amp = rng.random() * 6.283, 0.004 + 0.006 * rng.random()
				group = [s + amp * np.sin(np.linspace(0, 9, len(s)) + ph)[:, None] * np.linspace(0, 1, len(s))[:, None] * np.array([1.0, 0.6, 0.0]) for s in group]
			out += clump(group, 0.85 if opt["hair"] == "combed" else 0.8)
	else:
		scalp = hv[(hv[:, 1] > ey + 0.15) & ((hv[:, 2] < cz + 0.35) | (hv[:, 1] > ey + 0.75))]
		ang = np.degrees(np.arctan2(scalp[:, 0], scalp[:, 2] - cz))
		scalp = scalp[(np.abs(ang) > 50) & (scalp[:, 1] < hv[:, 1].max() - 0.3)]   # the crown and the brow gone bald
		for r in scalp[rng.choice(len(scalp), 60, replace=False)]:
			out_d = (to_w(r) - to_w([0, ey + 0.1, cz])).normalized()
			hit = bvh.find_nearest(to_w(r))
			base = np.array(hit[0]) + np.array(hit[1]) * 0.004
			fly = rng.random() < 0.2
			L = (0.06 + 0.06 * rng.random()) if fly else (0.12 + 0.13 * rng.random())
			group = []
			for _ in range(5 if fly else 10):
				root = Vector(base + rng.normal(size=3) * 0.007)
				s = grow_combed(bvh, root, (out_d + Vector((0, 0, 0.5))).normalized(), L, 12, rng, 0.006) if fly else \
					grow(bvh, root, (out_d * 0.35 + Vector((0, 0, -1))).normalized(), L * (0.7 + 0.5 * rng.random()), 12, rng, 0.5, 0.006)
				t = np.linspace(0, 1, len(s))[:, None] ** 1.5   # frizz: the tips wander, the roots stay put
				group.append(s + 0.016 * t * np.sin(t * np.array([7.0, 9.0, 5.0]) + rng.random(3) * 6.283))
			out += clump(group, 0.5)
	return curves_object("Hair", out, 0.0019 if opt["hair"] == "wild" else 0.0022, 0.0004, hair_material(0.03, 0.15, 0.45))


def build_mustache(h, hf, opt, bvh):
	"""The white mustache drooping past the mouth to the jaw (A, B); waxed into tight points for the burial (C)."""
	rng = np.random.default_rng(13)
	gr, to_w = h["glob"]["head"][:3, :3], lambda p: head_pt(h, p)
	nose = hf["verts"][np.argmax(hf["verts"][:, 2])]
	waxed = opt["mustache"] == "waxed"
	groups = {}
	for c in range(30):
		x = -0.31 + 0.62 * (c + 0.5) / 30 + rng.normal() * 0.01
		ax, sx = min(1.0, abs(x) / 0.31), math.copysign(1.0, x)
		group = groups.setdefault((sx, min(1, int(ax * 2))) if waxed else c, [])
		for k in range(22):
			y = nose[1] - 0.20 - 0.09 * rng.random() - 0.06 * ax ** 2
			p, q = to_w([x, y, nose[2] + 0.3]), to_w([x, y, nose[2] - 1.0])
			hit = bvh.ray_cast(p, (q - p).normalized(), (q - p).length)
			if hit[0] is None:
				continue
			comb = np.array([sx * (0.9 + 0.4 * ax), -0.35, 0.35]) if waxed else np.array([sx * (0.2 + 0.8 * ax), -1.0, 0.4])
			comb_w = Vector(mh.to_blender(gr @ (comb / np.linalg.norm(comb)))).normalized()
			L = (0.028 + (0.05 if waxed else 0.045) * ax) * (0.8 + 0.3 * rng.random())
			group.append(grow_combed(bvh, hit[0] + hit[1] * 0.001, comb_w, L, 10, rng, 0.0015 + 0.0015 * rng.random()))
	out = [s for g in groups.values() if g for s in clump(g, 0.95 if waxed else 0.6)]
	return curves_object("Mustache", out, 0.0011, 0.0002, hair_material(0.03, 0.15, 0.42))


def build_brows(h, hf, opt, bvh):
	"""Brows: long wiry white hairs on a heavy ridge, the outer ends drooping as old men's do."""
	rng = np.random.default_rng(3)
	to_w, fr = lambda p: head_pt(h, p), hf["front"]
	strands = []
	for e in hf["eyes"]:
		sx = math.copysign(1.0, e[0])
		for k in range(220):
			t = rng.random()
			x = e[0] + sx * (-0.22 + 0.5 * t)
			y = e[1] + 0.21 + 0.05 * math.sin(t * math.pi * 0.8) - 0.03 * (1 - t) - 0.05 * t ** 3 + rng.normal() * 0.014
			p, q = to_w([x, y, fr + 0.3]), to_w([x, y, fr - 1.0])
			hit = bvh.ray_cast(p, (q - p).normalized(), (q - p).length)
			if hit[0] is None:
				continue
			along = to_w([x + sx * 0.5, y + 0.04 - 0.16 * t, fr]) - to_w([x, y, fr])
			along = (along - hit[1] * along.dot(hit[1])).normalized()
			L = 0.008 + 0.010 * rng.random()
			strands.append(np.array([tuple(hit[0] + along * (L * i / 4) + hit[1] * (0.0005 + 0.003 * math.sin(i / 2))) for i in range(5)]))
	return curves_object("Brows", strands, 0.0003, 0.0001, hair_material(0.04, 0.15, 0.5))


# ---------------------------------------------------------------- clothes

def tear(ob, centre, size, rng):
	"""Rip a hole in a garment over centre: the cloth there cut finer, then a ragged hole, taller than wide."""
	ph = rng.random(3) * 6.283

	def hole(c):
		d = c - centre
		th = math.atan2(d.z, d.x)
		rr = size * (1 + 0.32 * math.sin(5 * th + ph[0]) + 0.18 * math.sin(11 * th + ph[1]) + 0.1 * math.sin(17 * th + ph[2]))
		return d.y < 0.03 and (d.x / 0.8) ** 2 + (d.z / 1.3) ** 2 < rr * rr
	edit_faces(ob, hole, lambda bm: bmesh.ops.subdivide_edges(
		bm, edges=[e for e in bm.edges if (e.verts[0].co - centre).length < size * 2.5], cuts=2, use_grid_fill=True))


def build_cape(pts, neck, sz, cloth, worn, rng):
	"""The shoulder cape, draped over the coat (over the body it tents off a stooped chin), open wide in front, its hem
	in pointed strips, slit and holed (A); worn to threads (B). Returns it, its top edge and its angles."""
	bf, bl = frame2d(np.array([0.0, -1.0]))
	thetas = np.linspace(math.radians(52), math.radians(308), 160)
	n = len(thetas)
	knots = np.cumsum(rng.integers(2, 7, size=80))
	knots = knots[knots < n]
	cut = np.interp(np.arange(n), knots, rng.random(len(knots)) * (0.08 if worn else 0.05)) + (np.arange(n) % 2) * 0.004
	X, Y, Z, R = drape(pts, neck[:2], bf, thetas, lambda th: neck[2] - 0.005,
		lambda th: sz - 0.10 - 0.03 * abs(math.sin(th)) - cut[np.argmin(np.abs(thetas - th))], 60, 0.016,
		hb=math.radians(4), zb=0.012, smooth=(3, 3))
	v = np.linspace(0, 1, Z.shape[0])[:, None]
	flute = 0.03 * v ** 1.4 + 0.018 * np.sin(thetas[None] * 10 + 0.9 * np.sin(v * 3)) * v ** 1.1 + 0.006 * np.sin(thetas[None] * 27 + v * 5) * v
	X = X + flute * (np.cos(thetas)[None] * bf[0] + np.sin(thetas)[None] * bl[0])
	Y = Y + flute * (np.cos(thetas)[None] * bf[1] + np.sin(thetas)[None] * bl[1])
	ob = grid_object("Cape", X, Y, Z)
	nz, nt = Z.shape
	slits = {i: int(nz * (1 - rng.uniform(0.15, 0.55 if worn else 0.3))) for i in rng.choice(nt - 1, 18 if worn else 8, replace=False)}
	holes = rng.random((14 if worn else 4, 3)) * np.array([nt - 1, nz * 0.6, 1]) + np.array([0, nz * 0.35, 0])
	bm = bmesh.new()
	bm.from_mesh(ob.data)
	dead = [f for k, f in enumerate(bm.faces) for j, i in [divmod(k, nt - 1)]
		if (i in slits and j >= slits[i]) or any(((i - c[0]) / 2.2) ** 2 + ((j - c[1]) / 2.5) ** 2 < 1.0 + c[2] for c in holes)]
	bmesh.ops.delete(bm, geom=dead, context="FACES")
	bm.to_mesh(ob.data)
	bm.free()
	add_mod(ob, "SOLIDIFY", thickness=0.004, offset=1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(cloth)
	if worn:
		hem = np.stack([X[-1], Y[-1], Z[-1]], 1)
		curves_object("Threads", [np.array([hem[i] + np.array([rng.normal() * 0.003 * s, rng.normal() * 0.003 * s, -L * s]) for s in np.linspace(0, 1, 6)])
			for i, L in zip(rng.choice(nt, 40, replace=False), 0.02 + 0.06 * rng.random(40))], 0.0012, 0.0006, cloth)
	return ob, np.stack([X[0], Y[0], Z[0]], 1), thetas


def tin_star(at, normal, polished):
	"""A marshal's six-point star with a ball on every point and a raised boss, cut from tin."""
	R, r, T = 0.029, 0.0145, 0.0022
	bm = bmesh.new()
	ang = [math.pi / 2 + k * math.pi / 6 for k in range(12)]
	vs = [bm.verts.new((math.cos(a) * (R if k % 2 == 0 else r), math.sin(a) * (R if k % 2 == 0 else r), 0.0)) for k, a in enumerate(ang)]
	ext = bmesh.ops.extrude_face_region(bm, geom=[bm.faces.new(vs)])
	bmesh.ops.translate(bm, verts=[e for e in ext["geom"] if isinstance(e, bmesh.types.BMVert)], vec=(0, 0, T))
	bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
	for a in ang[::2]:
		lm.uv_sphere(bm, 12, 8, 0.0042, matrix=Matrix.Translation((math.cos(a) * R, math.sin(a) * R, T * 0.5)))
	bmesh.ops.create_cone(bm, cap_ends=True, segments=32, radius1=r * 0.62, radius2=r * 0.5, depth=0.0016, matrix=Matrix.Translation((0, 0, T + 0.0008)))
	lm.fixed_order(bm)     # the extrude's faces come in another order each call (the game's star is made from this)
	me = bpy.data.meshes.new("Star")
	bm.to_mesh(me)
	bm.free()
	ob = link(bpy.data.objects.new("Star", me))
	add_mod(ob, "BEVEL", width=0.0005, segments=2, limit_method="ANGLE")
	ob.location = at
	ob.rotation_euler = normal.to_track_quat("Z", "Y").to_euler()
	ob.data.materials.append(metal_material("Tin", (0.70, 0.71, 0.72), (0.93, 0.94, 0.95), 0.16) if polished
		else metal_material("Tin", (0.22, 0.23, 0.21), (0.60, 0.61, 0.58), 0.45, 0.0, 400.0))
	return ob


def build_clothes(h, hf, body, opt):
	"""Shirt (torn on the ember in A, B), slate coat, cape (A, B), collar, lapels and tie (C), tin star; the pieces."""
	c = opt["coat"]
	rng = np.random.default_rng(41)
	neck, sz = neck_frame(h), shoulder_z(h)
	bf, bl = frame2d(np.array([0.0, -1.0]))
	cloth = canvas_material(*c["cloth"], name="Coat")
	P = surface_point(bvh_of(body), neck[0] + 0.03, neck[2] - c["ember_z"])[0]   # over the heart, left of the breastbone
	shirt, _ = shell(body, h, "Shirt", 0.006, 0.0, neck[2] - 0.005, 0.3)
	if c["tear"] > 0:
		tear(shirt, P, c["tear"], rng)
		s = c["tear"] * 1.4
		rows = [[(P.x + (i / 8 - 0.5) * 2 * s, P.z + (j / 8 - 0.5) * 2.6 * s, 0.0) for i in range(9)] for j in range(9)]
		strip_on(bvh_of(body), rows, "Ember", 0.002, ember_material(P, c["tear"], opt["glow"], 6.0 if opt["ember"] == "open" else 4.0), 0.0008)
		point_light("EmberLight", P + Vector((0, -0.04, 0.01)), c["ember_w"], opt["glow"], 0.012)
	add_mod(shirt, "SOLIDIFY", thickness=0.0015, offset=1.0)
	add_mod(shirt, "SUBSURF", levels=1, render_levels=2)
	shirt.data.materials.append(shirt_material(c["shirt"]))
	if opt["ember"] == "warm":   # C: the ember only a warmth under the shirt, no bloom
		point_light("Warmth", P + Vector((0, -0.03, 0)), c["ember_w"], (1.0, 0.55, 0.25), 0.02)
	if c.get("tie"):   # buried in his Sunday shirt: stand collar, string tie, a row of jet buttons
		black = material("Tie", ID["black"], lambda nt: principled(nt, (0.03, 0.03, 0.035), rough=0.35))
		_, nr = build_shirt_collar(h, neck, shirt.data.materials[0])
		build_string_tie(shirt, neck, nr, black)
		buttons(shirt, neck, 4, 0.045, black, 0.0045, -0.012, 0.15)
	coat, _ = shell(body, h, "Coat", 0.016, 0.02, neck[2] + 0.02)
	cut_vee(coat, neck, c["vee"])
	add_mod(coat, "SOLIDIFY", thickness=0.004, offset=1.0)
	add_mod(coat, "SUBSURF", levels=1, render_levels=2)
	coat.data.materials.append(cloth)
	out = {"shirt": shirt, "coat": coat, "cape": None, "lapels": []}
	if opt["cape"] != "none":
		out["cape"], top, thetas = build_cape(world_points(coat), neck, sz, cloth, opt["cape"] == "threads", rng)
	else:
		thetas = np.linspace(math.radians(40), math.radians(320), 120)
		top = np.concatenate([neck[:2] + (np.cos(thetas)[:, None] * bf + np.sin(thetas)[:, None] * bl) * 0.086, np.full((len(thetas), 1), neck[2] - 0.03)], 1)
	# The collar: a short funnel from the cape's top edge (or the coat's neck) in toward the neck, higher at the back.
	d, back = top[:, :2] - neck[:2], 0.5 - 0.5 * np.cos(thetas)
	cv = np.stack([np.concatenate([neck[:2] + d * (1 - 0.16 * t), (top[:, 2] + (0.012 + 0.03 * back) * t)[:, None]], 1) for t in np.linspace(0, 1, 10)])
	out["collar"] = collar = grid_object("Collar", cv[..., 0], cv[..., 1], cv[..., 2])
	add_mod(collar, "SOLIDIFY", thickness=0.005, offset=1.0)
	add_mod(collar, "SUBSURF", levels=1, render_levels=2)
	collar.data.materials.append(cloth)
	if c.get("lapels"):
		out["lapels"] = build_lapels(coat, neck, c["vee"], cloth, shirt)
	# The star on the left breast, on whatever is outermost there (the cape's edge in B).
	sp = surface_point(bvh_of(coat, *out["lapels"], *([out["cape"]] if out["cape"] else [])), neck[0] + c["star"][0], neck[2] - c["star"][1])
	if sp:
		tin_star(sp[0] + sp[1] * 0.003, sp[1], opt["star"] == "polished")
	return out


# ---------------------------------------------------------------- the toll chain and its hook

def hook(origin, e1, e2, L, Rb, r0, mat, extra=0.45):
	"""A J hook in the plane (e1, e2): the shank runs from the eye at origin along -e2 for L, bends through a half
	circle of radius Rb toward e1 and on by extra radians, tapering to a point; an eye ring at origin."""
	c = origin - e2 * L + e1 * Rb
	pts = [origin - e2 * (L * t) for t in np.linspace(0, 1, 8)] + [c + e1 * (Rb * math.cos(a)) + e2 * (Rb * math.sin(a)) for a in np.linspace(math.pi, 2 * math.pi + extra, 28)[1:]]
	n = len(pts)
	hk = link(bpy.data.objects.new("Hook", tube_data("Hook", pts, [r0 * min(1.0, max(0.12, 1.0 - (i - n * 0.55) / (n * 0.45))) for i in range(n)], mat)))
	eye = link(bpy.data.objects.new("HookEye", link_shape(mat, 0.002, 0.008, 0.0035)))
	eye.matrix_world = Matrix.Translation(origin + e2 * 0.008) @ Matrix((e1, e2, e1.cross(e2))).transposed().to_4x4()
	return [hk, eye]


def build_chain(h, opt, surfaces):
	"""The toll chain over the right shoulder: taut to the left hip, its hook caught in it (A); slack, hook free (B)."""
	if opt["chain"] == "none":
		return []
	bvh = bvh_of(*surfaces)
	sz = shoulder_z(h)
	iron = metal_material("Iron", (0.10, 0.10, 0.10), (0.36, 0.35, 0.33), 0.42, 1.0)
	lift, x = 0.010, -0.14
	f = bvh.ray_cast(Vector((x, -0.6, sz - 0.04)), Vector((0, 1, 0)), 1.5)[0]
	b = bvh.ray_cast(Vector((x, 0.6, sz - 0.04)), Vector((0, -1, 0)), 1.5)[0]
	t = bvh.ray_cast(Vector((x, (f.y + b.y) * 0.5, sz + 0.5)), Vector((0, 0, -1)), 1.0)[0]
	path, nrm = [], []

	def lay(hit):
		if hit and hit[0] is not None:
			path.append(np.array(hit[0] + hit[1] * lift))
			nrm.append(np.array(hit[1]))
	C = Vector((x, (f.y + b.y) * 0.5, t.z - (b.y - f.y) * 0.5))
	for a in np.linspace(-0.4, math.pi, 24):   # over the shoulder, back to front, hugging it
		d = Vector((0, math.cos(a), math.sin(a)))
		lay(bvh.ray_cast(C + d * 0.4, -d, 0.5))
	S, z0 = Vector(path[-1]), path[-1][2]
	up, side = Vector((0, 0, 1)), Vector((1, 0, 0))
	if opt["chain"] == "hooked":
		for s in np.linspace(0, 1, 50)[1:]:
			lay(surface_point(bvh, x + (0.24 - x) * s, z0 - (z0 - (sz - 0.42)) * s))
		Rb, L, r0 = 0.026, 0.06, 0.0068
		# hang it in front of whatever bulges below the shoulder (the cape flares), not inside it
		ys = [sp[0].y for zz in np.linspace(S.z - 0.09, S.z, 6) for xx in (S.x, S.x + Rb * 2) for sp in [surface_point(bvh, xx, zz)] if sp]
		o = Vector((S.x, min(ys + [S.y]) - 0.01, S.z)) - up * (Rb - r0 - 0.003 + L) - side * Rb
		parts = hook(o, side, -up, L, Rb, r0, iron, 0.7) + link_chain("Link", [np.array(o - up * (0.012 + 0.02 * k)) for k in range(6)], [(0, -1.0, 0)] * 6, iron)
	else:
		for s in np.linspace(0, 1, 16)[1:]:
			lay(surface_point(bvh, x + 0.05 * s ** 2, z0 - (z0 - (sz - 0.03)) * s))
		parts = hook(Vector(path[-1]) + Vector((0, -0.006, -0.012)), side, up, 0.045, 0.024, 0.0055, iron, 0.5)
	return parts + link_chain("Link", path, nrm, iron)


# ---------------------------------------------------------------- the bust

def build(opt):
	"""Crane's bust for one option, on an empty scene. Returns (h, eyes) for bust.run."""
	mh.ONE_SIDED.update(ONE_FILE)
	h, hf, body, eyes = stooped(opt, {**FACE, **opt.get("face", {})}, MACRO, "Crane")
	deepen(body, h, hf, opt["sunken"])
	face_counts_as_head(h)
	g = h["glob"]["head"][:3, :3]
	axes = [Vector(mh.to_blender(g @ np.array(a, float))).normalized() for a in ((0, 0, 1), (0, 1, 0), (1, 0, 0))]   # forward, up, left
	cl = build_clothes(h, hf, body, opt)
	surfaces = [body, cl["shirt"], cl["coat"], cl["collar"], *cl["lapels"]] + ([cl["cape"]] if cl["cape"] else [])
	hooks = [p for p in build_chain(h, opt, surfaces) if p.name.startswith("Hook")]
	build_hat(h, hf, opt)
	build_hair(h, hf, opt, bvh_of(*surfaces, *hooks))
	face = bvh_of(body)
	build_mustache(h, hf, opt, face)
	build_brows(h, hf, opt, face)
	sockets(eyes, axes, opt, face)
	turn_scene(h, opt["turn"])
	return h, eyes


def main():
	run(OPTIONS, "Crane", build)


if __name__ == "__main__":
	main()
