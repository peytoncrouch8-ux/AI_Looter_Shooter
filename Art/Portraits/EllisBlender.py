"""Ellis Ransom's portrait busts in Blender (Cycles), from MakeHuman's CC0 base mesh shaped into Ellis, on the
machinery every hero's bust shares (bust.py).

Run with the Blender binary or Blender's Python module bpy (more in bust.py):
    "C:/Program Files/Blender Foundation/Blender 4.4/blender.exe" -b --factory-startup --python-exit-code 1
        --python Art/Portraits/EllisBlender.py -- <A|B|C> [--size 1024] [--samples 96] [--out <dir>] [--no-aux] [--blend]
    python Art/Portraits/EllisBlender.py <A|B|C> [...]
Writes <out>/Ellis<opt>_color.png (RGBA) and <out>/Ellis<opt>_aux.png (R glow mask, B material id / 16, as the
march's pass 1), which `node Art/Portraits/render.js Ellis<opt> --passes <out>` paints and frames.
<out> defaults to Saved/ArtPreviews/Portraits.

The three options (Docs/Story.md: Ellis is a Revenant, "they" in all text, the HUD portrait's masked gunslinger):
  A  The Revenant: cattleman hat low, red bandana up, eyes burning cyan in the brim's shadow, duster collar up.
  B  Came Home at the Last: bandana down round the neck, face bare, grave-pale; hat pushed back.
  C  Dressed for the Grave: the burial clothes Tilly put on them, black coat and string tie, frost and grave dirt.
"""
import math
import os
import sys

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mh  # noqa: E402
from bust import (  # noqa: E402
	ID, add_mod, bandana_material, binding, body_bvh, brass_material, build_flower, build_human, build_lapels,
	build_shirt_collar, build_string_tie, buttons, bvh_of, canvas_material, chin_z, clump, curves_object, cut_vee,
	drape, felt_material, frame2d, grid_object, grow, grow_combed, hair_material, head_axis, leather_material, link,
	material, mesh_object, neck_frame, principled, run, shell, shoulder_z, strip_on, sun_pin, surface_point,
	world_points)
# Art/Models/Characters/Ellis.py reads these through this module (eb.close_lids, eb.head_frame, eb.sun_pin).
from bust import close_lids, head_frame  # noqa: E402,F401

# The paint pass's settings per option (render.js reads these lines).
# PAINT A {"bgTop":"#0d1622","bgBottom":"#05070b","bgGlow":"#2a3f57","glowAt":[0.72,0.30],"sparks":"#ff9f1c","dust":"#7fb6d6","glow":"#5ad8ff"}
# PAINT B {"bgTop":"#1c1418","bgBottom":"#07060a","bgGlow":"#6a3e26","glowAt":[0.30,0.34],"sparks":"#ffb050","dust":"#e0b080","glow":"#5ad8ff"}
# PAINT C {"bgTop":"#0a1018","bgBottom":"#040609","bgGlow":"#2a4458","glowAt":[0.66,0.26],"sparks":"#cfeeff","dust":"#9fbccc","glow":"#5ad8ff"}

# Ellis's build (mh.build's macro sliders): gender 0.5, neither (Ellis is "they"); lean, a little muscle.
MACRO = {"gender": 0.5, "muscle": 0.6, "weight": 0.42, "caucasian": 0.7, "african": 0.15, "asian": 0.15}

# Ellis's face: an androgynous young adult (gender 0.5, about 26), lean, high cheekbones, a straight strong nose,
# a firm narrow chin, brows that sit low. Shared by every option; options add their own touches.
FACE = {
	"cheek/l-cheek-bones": 0.55, "cheek/r-cheek-bones": 0.55,
	"cheek/l-cheek-volume": -0.45, "cheek/r-cheek-volume": -0.45,
	"cheek/l-cheek-inner": -0.3, "cheek/r-cheek-inner": -0.3,
	"chin/chin-prominent": 0.35, "chin/chin-width": -0.25, "chin/chin-bones": 0.3,
	"nose/nose-scale-horiz": -0.25, "nose/nose-point-width": -0.35, "nose/nose-hump": 0.25, "nose/nose-nostrils-width": -0.2,
	"mouth/mouth-scale-horiz": -0.1, "mouth/mouth-angles": -0.2, "mouth/mouth-upperlip-volume": -0.15,
	"eyebrows/eyebrows-angle": -0.35, "eyebrows/eyebrows-trans-down": 0.0,
	"head/head-square": 0.25, "head/head-oval": 0.3,
	"l-eye": None,
}
FACE.pop("l-eye")
for side in "lr":
	FACE[f"eyes/{side}-eye-bag"] = -0.3
	FACE[f"eyes/{side}-eye-scale"] = 0.15
	FACE[f"ears/{side}-ear-flap"] = -0.4

OPTIONS = {
	"A": {"label": "The Revenant", "yaw": -34.0, "pitch": 7.0, "lids": (0.034, 0.018, 0.5), "hat": "low", "bandana": "up",
		"hair": "under", "eyes": "slit", "skin": (0.70, 0.62, 0.60), "glow": (0.25, 0.85, 1.0),
		"coat": {"cape": True, "collar": "up", "lapels": False, "under": "vest", "bandolier": False, "pin": "collar",
			"cloth": ((0.12, 0.085, 0.06), (0.30, 0.235, 0.17)), "frost": 0.0, "vee": 0.045},
		"world": (0.10, 0.13, 0.20), "cam": (2.12, -0.005),
		"lights": [("Key", (-0.78, -0.48, 0.30), 30.0, (1.0, 0.72, 0.48), 0.5), ("Rim", (1.3, 1.2, 0.5), 115.0, (0.45, 0.85, 1.0), 0.8),
			("Fill", (0.9, -1.6, -0.3), 12.0, (0.55, 0.65, 1.0), 1.5)]},
	"B": {"label": "Came Home at the Last", "yaw": -18.0, "pitch": 1.0, "lids": (0.014, 0.008, 0.25), "hat": "back", "bandana": "down",
		"hair": "under", "eyes": "iris", "skin": (0.70, 0.64, 0.64), "glow": (0.25, 0.85, 1.0),
		"coat": {"cape": False, "collar": "low", "lapels": True, "under": "leathervest", "bandolier": True, "pin": "lapel",
			"cloth": ((0.16, 0.115, 0.08), (0.40, 0.31, 0.22)), "frost": 0.0, "vee": 0.05},
		"world": (0.20, 0.15, 0.12), "cam": (2.12, -0.012),
		"lights": [("Key", (-0.92, -0.38, 0.26), 30.0, (1.0, 0.76, 0.52), 0.5), ("Rim", (1.25, 1.1, 0.45), 95.0, (0.45, 0.85, 1.0), 0.8),
			("Bounce", (0.25, -0.9, -0.65), 6.0, (1.0, 0.6, 0.4), 1.2)]},
	"C": {"label": "Dressed for the Grave", "yaw": 24.0, "pitch": 5.0, "lids": (0.022, 0.012, 0.35), "hat": "none", "bandana": "none",
		"hair": "full", "eyes": "iris", "skin": (0.70, 0.66, 0.68), "glow": (0.25, 0.85, 1.0),
		"coat": {"cape": False, "collar": "low", "lapels": True, "under": "shirt", "bandolier": False, "pin": None, "tie": True,
			"flower": True, "cloth": ((0.03, 0.03, 0.035), (0.11, 0.105, 0.115)), "frost": 1.0, "vee": 0.03},
		"world": (0.07, 0.09, 0.13), "cam": (1.98, -0.04),
		"lights": [("Key", (0.85, -0.42, 0.32), 19.0, (0.74, 0.84, 1.0), 0.5), ("Rim", (-1.25, 1.0, 0.25), 80.0, (1.0, 0.58, 0.28), 0.6),
			("Fill", (-0.9, -1.6, -0.3), 8.0, (0.5, 0.6, 1.0), 1.5)]},
}


# ---------------------------------------------------------------- the hat

def build_hat(h, hf, opt):
	"""A cattleman: tapered crown with the long crease and front pinches, a brim curled at the sides,
	a leather band. Built in the head's rest space (decimetres) and carried by the posed head."""
	if opt["hat"] == "none":
		return []
	hv = hf["verts"]
	band_y = hf["eye_y"] + (0.50 if opt["hat"] == "low" else 0.66)
	ring = hv[np.abs(hv[:, 1] - band_y) < 0.12]
	cx = 0.0
	cz = (ring[:, 2].max() + ring[:, 2].min()) * 0.5
	rx = np.abs(ring[:, 0]).max() + 0.14
	rz = (ring[:, 2].max() - ring[:, 2].min()) * 0.5 + 0.14
	H = 1.12
	tip = math.radians(9.0 if opt["hat"] == "low" else -14.0)
	pivot = np.array([cx, band_y, cz])

	def place(p):
		# tip the hat forward (or back) about the band's centre, then into the posed head
		c, s = math.cos(tip), math.sin(tip)
		q = p - pivot
		q = np.stack([q[:, 0], q[:, 1] * c - q[:, 2] * s, q[:, 1] * s + q[:, 2] * c], axis=1) + pivot
		return mh.to_blender(mh.apply(h["glob"]["head"], q))

	# Crown: a revolved, rounded-square profile, tapered, then creased.
	na, nseg = 36, 96
	rows = []
	for i in range(na + 1):
		a = (i / na) * (math.pi / 2)
		r = abs(math.cos(a)) ** (2 / 5.0)
		y = abs(math.sin(a)) ** (2 / 5.0)
		rows.append((r * (1.0 - 0.11 * y * y), y * H))
	verts = []
	for (r, y) in rows[:-1]:
		for j in range(nseg):
			ph = 2 * math.pi * j / nseg
			x = math.sin(ph) * rx * r
			z = math.cos(ph) * rz * r
			# teardrop plan: narrower toward the front near the top
			x *= 1.0 - 0.10 * max(math.cos(ph), 0.0) * (y / H)
			verts.append([x, band_y + y, cz + z])
	verts.append([0.0, band_y + H, cz])
	verts = np.array(verts)
	top = (verts[:, 1] - band_y) / H
	# The long crease down the top, deepest in the middle, and the pinches either side of the front.
	zn = (verts[:, 2] - cz) / rz
	crease = 0.30 * np.exp(-(verts[:, 0] / 0.20) ** 2) * np.clip((top - 0.55) / 0.45, 0, 1) ** 1.2 * (1.0 - 0.35 * zn ** 2)
	verts[:, 1] -= crease
	pinch = 0.17 * np.exp(-((zn - 0.62) / 0.28) ** 2) * np.clip((top - 0.35) / 0.65, 0, 1) ** 1.3
	verts[:, 0] -= np.sign(verts[:, 0]) * pinch
	faces = []
	for i in range(na - 1):
		for j in range(nseg):
			a = i * nseg + j
			b = i * nseg + (j + 1) % nseg
			faces.append([a, b, b + nseg, a + nseg])
	ctr = len(verts) - 1
	last = (na - 1) * nseg
	for j in range(nseg):
		faces.append([last + j, last + (j + 1) % nseg, ctr])
	crown = mesh_object("HatCrown", place(verts), faces)
	add_mod(crown, "SOLIDIFY", thickness=0.004, offset=-1.0)
	add_mod(crown, "SUBSURF", levels=1, render_levels=2)

	# Brim: an annulus from the crown's base out, curled up at the sides, dipping front and back.
	ns = 14
	bv, bf = [], []
	brx, brz = rx + 0.95, rz + 0.92
	for i in range(ns + 1):
		s = i / ns
		for j in range(nseg):
			ph = 2 * math.pi * j / nseg
			x = math.sin(ph) * (rx * 0.97 + (brx - rx) * s)
			z = math.cos(ph) * (rz * 0.97 + (brz - rz) * s)
			side = abs(math.sin(ph))
			y = 0.40 * (s ** 1.6) * side ** 2.2 - 0.13 * (s ** 2) * max(math.cos(ph), 0) ** 3 - 0.06 * (s ** 2) * max(-math.cos(ph), 0) ** 3
			bv.append([x, band_y + y + 0.01, cz + z])
	for i in range(ns):
		for j in range(nseg):
			a = i * nseg + j
			b = i * nseg + (j + 1) % nseg
			bf.append([a, a + nseg, b + nseg, b])
	brim = mesh_object("HatBrim", place(np.array(bv)), bf)
	add_mod(brim, "SOLIDIFY", thickness=0.0055, offset=0.0, use_rim=True)
	add_mod(brim, "SUBSURF", levels=1, render_levels=2)

	# Band: a strip round the crown's foot.
	vv, ff = [], []
	for k, y in enumerate((0.02, 0.21)):
		for j in range(nseg):
			ph = 2 * math.pi * j / nseg
			vv.append([math.sin(ph) * rx * 1.012, band_y + y, cz + math.cos(ph) * rz * 1.012])
	for j in range(nseg):
		ff.append([j, (j + 1) % nseg, (j + 1) % nseg + nseg, j + nseg])
	band = mesh_object("HatBand", place(np.array(vv)), ff)
	add_mod(band, "SOLIDIFY", thickness=0.0015, offset=1.0)
	add_mod(band, "SUBSURF", levels=1, render_levels=2)

	felt = felt_material()
	for o in (crown, brim):
		o.data.materials.append(felt)
	band.data.materials.append(leather_material((0.14, 0.08, 0.05)))
	return [crown, brim, band]


# ---------------------------------------------------------------- hair and brows

def build_hair(h, hf, body, opt, length_back=0.20, length_side=0.13):
	"""A shaggy crop falling from under the hat over the ears to the collar, in clumps; a lock over the brow."""
	rng = np.random.default_rng(7)
	bvh, bm = body_bvh(body)
	g = h["glob"]["head"]
	hv = hf["verts"]
	band_y = hf["eye_y"] + 0.48
	cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5 - 0.1
	centre = np.array([0.0, band_y, cz])
	down_world = Vector((0, 0, -1))
	strands = []
	nclump = 64
	for c in range(nclump):
		ang = math.radians(-150 + 300 * (c + rng.random() * 0.6) / nclump)   # 0 is the face; skip the forehead
		if abs(math.degrees(ang)) < 42:
			continue
		back = min(1.0, abs(math.degrees(ang)) / 150.0)
		L = (length_side + (length_back - length_side) * back ** 1.5) * (0.85 + 0.3 * rng.random())
		dirx, dirz = math.sin(ang), math.cos(ang)
		# the clump's root: on the scalp just under the band
		r = np.array([dirx * 0.7, 0.0, dirz * 0.9]) + centre
		guide_root = mh.to_blender(mh.apply(g, r[None]))[0]
		hit = bvh.find_nearest(Vector(guide_root))
		base = np.array(hit[0]) + np.array(hit[1]) * 0.006
		for k in range(40):
			off = rng.normal(size=3) * 0.010
			root = base + off
			strand = grow(bvh, Vector(root), down_world, L * (0.75 + 0.35 * rng.random()), 12, rng, 0.3 + 0.3 * rng.random(), 0.006 + 0.008 * rng.random())
			# clump: pull each strand toward the clump's mean toward the tip
			strands.append(strand)
	# gather into clumps: strands of one clump converge toward their tip
	out = []
	for i in range(0, len(strands), 40):
		out += clump(strands[i:i + 40], 0.78)
	# A lock out from under the brim over the left brow.
	for k in range(60):
		r = np.array([0.38 + rng.normal() * 0.05, band_y - 0.02, hv[:, 2].max() - 0.25 + rng.normal() * 0.03])
		root = mh.to_blender(mh.apply(g, r[None]))[0]
		tip_dir = Vector(mh.to_blender(mh.apply(g, np.array([[0.55, band_y - 0.6, hv[:, 2].max() + 0.1]])))[0]) - Vector(root)
		strand = grow(bvh, Vector(root), tip_dir.normalized(), tip_dir.length * (0.8 + 0.3 * rng.random()), 8, rng, 0.25)
		out.append(strand)
	bm.free()
	return curves_object("Hair", out, 0.0024, 0.0005, hair_material())


def build_hair_full(h, hf, body, opt):
	"""A full head of hair for a bare head: parted on the left, combed back and over as Tilly laid it out,
	grown out of order since; collar-length, in clumps."""
	rng = np.random.default_rng(5)
	bvh, bm = body_bvh(body)
	g = h["glob"]["head"]
	gr = g[:3, :3]
	hv = hf["verts"]
	ey = hf["eye_y"]
	cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5
	ang = np.degrees(np.arctan2(hv[:, 0], hv[:, 2] - cz))
	a = np.abs(ang)
	line = np.where(a < 45, ey + 0.62, np.where(a < 105, ey + 0.30 + (a - 45) / 60 * -0.05, ey - 0.55))
	scalp = hv[(hv[:, 1] > line) & ~((np.abs(hv[:, 0]) > 0.66) & (hv[:, 1] < ey + 0.25))]
	part_x = 0.24
	out = []
	picks = scalp[rng.choice(len(scalp), 150, replace=False)]
	for r in picks:
		side = 1.0 if r[0] > part_x else -1.0
		front = np.clip((r[2] - cz) / 0.9, -1, 1)
		comb = np.array([side * 0.75, -0.25, -0.55 - 0.3 * max(front, 0)])
		comb /= np.linalg.norm(comb)
		comb_w = Vector(mh.to_blender((gr @ comb)[None])[0]).normalized()
		L = 0.085 + 0.07 * rng.random() + (0.03 if r[2] < cz else 0.0)
		root_w = mh.to_blender(mh.apply(g, r[None]))[0]
		hit = bvh.find_nearest(Vector(root_w))
		base = np.array(hit[0]) + np.array(hit[1]) * 0.004
		group = []
		for k in range(28):
			root = base + rng.normal(size=3) * 0.007
			group.append(grow_combed(bvh, Vector(root), comb_w, L * (0.8 + 0.3 * rng.random()), 12, rng, 0.004 + 0.006 * rng.random()))
		out += clump(group, 0.7)
	bm.free()
	return curves_object("Hair", out, 0.0022, 0.0005, hair_material())


def build_fringe(h, hf, body, opt):
	"""Under a pushed-back hat: a short fringe swept to the right out of the band's front."""
	rng = np.random.default_rng(9)
	bvh, bm = body_bvh(body)
	g = h["glob"]["head"]
	gr = g[:3, :3]
	hv = hf["verts"]
	band_y = hf["eye_y"] + 0.62
	out = []
	for c in range(9):
		x = -0.36 + 0.72 * (c + 0.5) / 9 + rng.normal() * 0.02
		front = hv[np.abs(hv[:, 0] - x) < 0.05]
		front = front[np.abs(front[:, 1] - band_y) < 0.08]
		if not len(front):
			continue
		r = front[np.argmax(front[:, 2])]
		comb = np.array([-0.85, -0.45, 0.30])
		comb /= np.linalg.norm(comb)
		comb_w = Vector(mh.to_blender((gr @ comb)[None])[0]).normalized()
		root_w = mh.to_blender(mh.apply(g, r[None]))[0]
		group = []
		for k in range(26):
			root = np.array(root_w) + rng.normal(size=3) * 0.006
			group.append(grow_combed(bvh, Vector(root), comb_w, 0.035 + 0.03 * rng.random(), 8, rng, 0.003 + 0.004 * rng.random()))
		out += clump(group, 0.65)
	bm.free()
	return curves_object("Fringe", out, 0.0021, 0.0005, hair_material())


def build_brows(h, hf, body, opt):
	"""Brows: short strands laid along the brow ridge, low and drawn in toward the nose."""
	rng = np.random.default_rng(3)
	bvh, bm = body_bvh(body)
	g = h["glob"]["head"]
	strands = []
	for e in hf["eyes"]:
		sx = 1.0 if e[0] > 0 else -1.0
		for k in range(170):
			t = rng.random()
			# along the brow: inner end low, outer end a little higher, an arch toward the outer third
			x = e[0] + sx * (-0.20 + 0.42 * t)
			y = e[1] + 0.20 + 0.06 * math.sin(t * math.pi * 0.8) - 0.03 * (1 - t) + rng.normal() * 0.012
			p = mh.to_blender(mh.apply(g, np.array([[x, y, hf["front"] + 0.3]])))[0]
			q = mh.to_blender(mh.apply(g, np.array([[x, y, hf["front"] - 1.0]])))[0]
			d = Vector(q) - Vector(p)
			hit = bvh.ray_cast(Vector(p), d.normalized(), d.length)
			if hit[0] is None:
				continue
			root = hit[0] + hit[1] * 0.0005
			along = Vector(mh.to_blender(mh.apply(g, np.array([[x + sx * 0.5, y + 0.12 - 0.08 * t, hf["front"]]])))[0]) - Vector(mh.to_blender(mh.apply(g, np.array([[x, y, hf["front"]]])))[0])
			along = (along - hit[1] * along.dot(hit[1])).normalized()
			L = 0.006 + 0.004 * rng.random()
			pts = [root + along * (L * i / 3) + hit[1] * (0.0012 * math.sin(i / 3 * math.pi)) for i in range(4)]
			strands.append(np.array([tuple(p) for p in pts]))
	bm.free()
	return curves_object("Brows", strands, 0.00025, 0.00008, hair_material(0.9, 0.3, 0.4))


# ---------------------------------------------------------------- the bandana

def build_bandana_up(h, hf, body, over=()):
	pts = np.concatenate([world_points(body)] + [world_points(o) for o in over])
	centre, fwd, eye_z = head_axis(h, hf)
	bridge = eye_z - 0.022
	span = math.radians(104)
	thetas = np.linspace(-span, span, 105)
	top = lambda th: bridge - 0.055 * (th / span) ** 2
	bot = lambda th: bridge - (0.30 - 0.19 * abs(th / span) ** 1.1)
	X, Y, Z, R = drape(pts, centre, fwd, thetas, top, bot, 70, 0.004, smooth=(2, 2))
	# Folds: fanning down from the nose, sagging across the bib.
	n_z = Z.shape[0]
	v = np.linspace(0, 1, n_z)[:, None]
	th = thetas[None, :]
	rng = np.random.default_rng(11)
	ph = rng.random(6) * 6.28
	fold = 0.0035 * np.sin(th * 13 + 1.5 * np.sin(v * 4 + ph[0])) * np.clip((v - 0.12) / 0.3, 0, 1)
	fold += 0.004 * np.sin(v * 18 + th * 3 + ph[1]) * np.clip((v - 0.55) / 0.3, 0, 1)
	f, l = frame2d(fwd)
	dirx = np.cos(th) * f[0] + np.sin(th) * l[0]
	diry = np.cos(th) * f[1] + np.sin(th) * l[1]
	X = X + fold * dirx
	Y = Y + fold * diry
	ob = grid_object("Bandana", X, Y, Z)
	add_mod(ob, "SOLIDIFY", thickness=0.0015, offset=1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(bandana_material())
	return ob


def build_bandana_down(h, hf, body, over=()):
	"""The bandana worn down: loose round the neck, its point falling over the chest."""
	pts = np.concatenate([world_points(body)] + [world_points(o) for o in over])
	neck = neck_frame(h)
	cz = chin_z(h)
	# only what's round the neck: the bandana is knotted snug, not spread over the shoulders
	pts = pts[np.linalg.norm(pts[:, :2] - neck[:2], axis=1) < 0.098]
	bf, bl = frame2d(np.array([0.0, -1.0]))
	thetas = np.linspace(-math.pi, math.pi, 120, endpoint=False)
	top = lambda th: cz - 0.045 - 0.025 * (0.5 - 0.5 * math.cos(th)) * -1.0
	bot = lambda th: top(th) - (0.04 + 0.075 * max(math.cos(th), 0.0) ** 3)
	centre = neck[:2] + bf * 0.012
	X, Y, Z, R = drape(pts, centre, bf, thetas, top, bot, 40, 0.012, hb=math.radians(5), zb=0.01, smooth=(3, 2), wrap=True)
	v = np.linspace(0, 1, Z.shape[0])[:, None]
	sag = 0.006 * np.sin(v * 16 + thetas[None] * 2) + 0.004 * np.sin(thetas[None] * 9 + v * 3)
	dx = np.cos(thetas)[None] * bf[0] + np.sin(thetas)[None] * bl[0]
	dy = np.cos(thetas)[None] * bf[1] + np.sin(thetas)[None] * bl[1]
	ob = grid_object("Bandana", X + sag * dx, Y + sag * dy, Z, wrap=True)
	add_mod(ob, "SOLIDIFY", thickness=0.0016, offset=1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(bandana_material())
	return ob


# ---------------------------------------------------------------- the coat

def build_bandolier(over, neck, sz):
	"""A bandolier over the left shoulder to the right hip: a leather strap, brass cartridges in its loops."""
	bvh = bvh_of(*over)
	bf, bl = frame2d(np.array([0.0, -1.0]))
	a = np.array([neck[0] + bl[0] * 0.13, sz + 0.02])
	b = np.array([neck[0] - bl[0] * 0.20, sz - 0.45])
	n = 40
	rows = []
	cent = []
	for j in range(n):
		t = j / (n - 1)
		x, z = a + (b - a) * t
		perp = np.array([-(b - a)[1], (b - a)[0]]) / np.linalg.norm(b - a)
		row = []
		for i in range(3):
			u = (i / 2 - 0.5) * 0.05
			row.append((x + perp[0] * u, z + perp[1] * u, 0.0))
		rows.append(row)
		cent.append((x, z, perp))
	parts = [strip_on(bvh, rows, "Bandolier", 0.008, leather_material((0.20, 0.12, 0.07)), 0.005)]
	brass = brass_material()
	lead = material("Lead", ID["lead"], lambda nt: principled(nt, (0.72, 0.52, 0.42), rough=0.35, spec=0.5, metal=1.0))
	for j in range(3, n - 4, 2):
		x, z, perp = cent[j]
		sp = surface_point(bvh, x, z)
		if sp is None:
			continue
		p0, nrm = sp
		up = Vector((perp[0], 0.0, perp[1]))
		up = (up - nrm * up.dot(nrm)).normalized()
		for kind, length, rad, shift, mat in (("Case", 0.05, 0.0058, 0.0, brass), ("Bullet", 0.016, 0.0052, 0.033, lead)):
			bm = bmesh.new()
			bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=rad, radius2=rad * (0.35 if kind == "Bullet" else 1.0), depth=length)
			me = bpy.data.meshes.new(kind)
			bm.to_mesh(me)
			bm.free()
			me.shade_smooth()
			co = link(bpy.data.objects.new(kind, me))
			co.location = p0 + nrm * 0.019 + up * (0.006 + shift)
			co.rotation_euler = up.to_track_quat("Z", "Y").to_euler()
			co.data.materials.append(mat)
			parts.append(co)
	return parts


def build_coat(h, hf, body, opt):
	"""The coat as the option asks: the duster with a cape and its collar up (A), the duster open with lapels
	and a bandolier (B), the black burial coat over a white shirt and string tie (C)."""
	c = opt["coat"]
	parts = []
	neck = neck_frame(h)
	sz = shoulder_z(h)
	bf, bl = frame2d(np.array([0.0, -1.0]))   # the body faces -y; the head turns, the camera stays put
	cloth = canvas_material(c["cloth"][0], c["cloth"][1], frost=c["frost"])
	brass = brass_material()
	# Under the coat: a waistcoat (A black, B leather) or the burial shirt (C).
	under, _ = shell(body, h, "Under", 0.007, 0.0, neck[2] - (0.01 if c["under"] != "shirt" else -0.005), 0.3)
	add_mod(under, "SUBSURF", levels=1, render_levels=2)
	if c["under"] == "shirt":
		linen = material("Linen", ID["linen"], lambda nt: principled(nt, (0.78, 0.76, 0.72), rough=0.7, spec=0.3, **{"Sheen Weight": 0.2}))
		under.data.materials.append(linen)
	elif c["under"] == "leathervest":
		under.data.materials.append(leather_material((0.20, 0.12, 0.075)))
	else:
		under.data.materials.append(canvas_material((0.05, 0.05, 0.055), (0.13, 0.12, 0.12), "Vest", ID["black"]))
	parts.append(under)
	if c["under"] != "shirt":
		parts += buttons(under, neck, 5, 0.045, brass)
	# The coat itself.
	coat, _ = shell(body, h, "Coat", 0.016, 0.02, neck[2] + 0.02)
	cut_vee(coat, neck, c["vee"] + (0.0 if c["under"] != "shirt" else -0.01))
	add_mod(coat, "SOLIDIFY", thickness=0.004, offset=1.0)
	add_mod(coat, "SUBSURF", levels=1, render_levels=2)
	coat.data.materials.append(cloth)
	parts.append(coat)
	pts = world_points(body)
	top_r = 0.079
	if c["cape"]:
		# The cape: a bell hung from the collar over the shoulders, flaring in flutes, open at the front.
		thetas = np.linspace(math.radians(24), math.radians(336), 160)
		top = lambda th: neck[2] - 0.005
		bot = lambda th: sz - 0.105 - 0.03 * abs(math.sin(th)) + 0.012 * math.sin(th * 7.0)
		X, Y, Z, R = drape(pts, neck[:2], bf, thetas, top, bot, 60, 0.036, hb=math.radians(4), zb=0.012, smooth=(3, 3))
		v = np.linspace(0, 1, Z.shape[0])[:, None]
		flute = 0.03 * v ** 1.4 + 0.018 * np.sin(thetas[None, :] * 10 + 0.9 * np.sin(v * 3)) * v ** 1.1 + 0.006 * np.sin(thetas[None, :] * 27 + v * 5) * v
		dirx = np.cos(thetas)[None] * bf[0] + np.sin(thetas)[None] * bl[0]
		diry = np.cos(thetas)[None] * bf[1] + np.sin(thetas)[None] * bl[1]
		cape_ob = grid_object("Cape", X + flute * dirx, Y + flute * diry, Z)
		hem = np.stack([X[-1] + flute[-1] * dirx[0], Y[-1] + flute[-1] * diry[0], Z[-1]], 1)
		parts.append(binding("CapeHem", hem, 0.016))
		add_mod(cape_ob, "SOLIDIFY", thickness=0.004, offset=1.0)
		add_mod(cape_ob, "SUBSURF", levels=1, render_levels=2)
		cape_ob.data.materials.append(cloth)
		parts.append(cape_ob)
		top_r = R[0].max()
	# Collar: turned up high round the jaw (A), or a low rolled collar behind the neck (B, C).
	up = c["collar"] == "up"
	top_r += 0.004
	thetas = np.linspace(math.radians(26 if up else 40), math.radians(334 if up else 320), 120)
	rows = 12
	cv = []
	for j in range(rows):
		t = j / (rows - 1)
		for th in thetas:
			back = 0.5 - 0.5 * math.cos(th)
			hgt = (0.075 + 0.055 * back) if up else (0.025 + 0.03 * back)
			r = top_r * (0.92 + (0.10 if up else 0.22) * t) + 0.004 * math.sin(th * 9 + t * 2) - 0.012 * (1 - back) * t
			d = math.cos(th) * bf + math.sin(th) * bl
			cv.append([neck[0] + d[0] * r, neck[1] + d[1] * r, neck[2] - 0.03 + hgt * t])
	cv = np.array(cv)
	collar = grid_object("Collar", cv[:, 0].reshape(rows, -1), cv[:, 1].reshape(rows, -1), cv[:, 2].reshape(rows, -1))
	add_mod(collar, "SOLIDIFY", thickness=0.005, offset=1.0)
	add_mod(collar, "SUBSURF", levels=1, render_levels=2)
	collar.data.materials.append(cloth)
	parts.append(collar)
	if c["lapels"]:
		parts += build_lapels(coat, neck, c["vee"], cloth, under)
	# Saint Ada's sun-ring, Pa's keeper's badge, on the collar (A) or the left lapel (B).
	if c.get("pin") == "collar":
		th = math.radians(40)
		d = math.cos(th) * bf + math.sin(th) * bl
		r = top_r * 0.99 + 0.006
		parts.append(sun_pin(Vector((neck[0] + d[0] * r, neck[1] + d[1] * r, neck[2] + 0.02)), Vector((d[0], d[1], 0.15)).normalized()))
	elif c.get("pin") == "lapel":
		sp = surface_point(bvh_of(coat), neck[0] + bl[0] * 0.10, neck[2] - 0.06)
		if sp:
			parts.append(sun_pin(sp[0] + sp[1] * 0.012, sp[1]))
	if c["under"] == "shirt":
		h["_body"] = body
		black = material("Tie", ID["black"], lambda nt: principled(nt, (0.03, 0.03, 0.035), rough=0.35, spec=0.5))
		linen = under.data.materials[0]
		sc, nr = build_shirt_collar(h, neck, linen)
		parts.append(sc)
		if c.get("tie"):
			parts += build_string_tie(under, neck, nr, black)
	if c.get("flower"):
		sp = surface_point(bvh_of(coat), neck[0] + bl[0] * 0.085, neck[2] - 0.075)
		if sp:
			parts += build_flower(sp[0] + sp[1] * 0.012, sp[1])
	return parts


# ---------------------------------------------------------------- the bust

def build(opt):
	"""Ellis's bust for one option, on an empty scene: body and eyes, hat, hair, brows, the clothes, the bandana and
	the bandolier. Returns (h, eyes) for bust.run."""
	h, hf, body, eyes = build_human(opt, FACE, MACRO, "Ellis")
	build_hat(h, hf, opt)
	if opt["hair"] == "full":
		build_hair_full(h, hf, body, opt)
	else:
		build_hair(h, hf, body, opt)
		if opt["hat"] == "back":
			build_fringe(h, hf, body, opt)
	build_brows(h, hf, body, opt)
	clothes = build_coat(h, hf, body, opt)
	under = [o for o in clothes if o.name.startswith(("Under", "Coat", "Lapel", "Collar"))]
	band = None
	if opt["bandana"] == "up":
		band = build_bandana_up(h, hf, body, [o for o in under if o.name.startswith("Under")])
	elif opt["bandana"] == "down":
		band = build_bandana_down(h, hf, body, under)
	if opt["coat"].get("bandolier"):
		build_bandolier(under + ([band] if band else []), neck_frame(h), shoulder_z(h))
	return h, eyes


def main():
	run(OPTIONS, "Ellis", build)


if __name__ == "__main__":
	main()
