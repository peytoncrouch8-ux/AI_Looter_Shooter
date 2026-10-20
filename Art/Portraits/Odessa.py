"""Odessa Lark's portrait busts (the Cardsharp) in Blender (Cycles), from MakeHuman's CC0 base mesh, on the machinery
every hero's bust shares (bust.py, bustkit.py; EllisBlender.py is the worked example; Saved/Portraits/CONTRACT.md the brief).

    "C:/Program Files/Blender Foundation/Blender 4.4/blender.exe" -b --factory-startup --python-exit-code 1
        --python Art/Portraits/Odessa.py -- <A|B|C> [--size 1024] [--samples 96] [--out <dir>] [--no-aux] [--blend]
writes <out>/Odessa<opt>_color.png and _aux.png; `node Art/Portraits/render.js Odessa<opt> --script Odessa.py` paints.

  A  The Dealer: flat-crowned gambler hat, a card in its band, wine tailcoat, brocade waistcoat and watch chain, high
     collar and cravat, a black-gloved hand holding a card up beside the face; a half smile; eyes cyan.
  B  Marked: hat off, hair pinned with a lock loose, grave-pale, Ned's bullet mark at the left temple with threads of
     cyan in it; the coat torn at the shoulder, the cravat pulled loose; no smile.
  C  The Riverboat Queen, as in life: hat tipped back, velvet choker, waistcoat open a button, a fan of five cards low
     in frame, lamplight from below, brown eyes, a cheroot's smoke.
Playing cards are plain shapes made here (a rounded card; a heart, diamond, spade or club pip): no real deck's art.
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
import mh  # noqa: E402
from bust import (  # noqa: E402
	ID, add_mod, area_light, body_bvh, brass_material, build_human, bump, bvh_of, clump, curves_object, grid_object,
	hair_material, head_axis, leather_material, lin, material, mesh_object, mix_rgb, neck_frame, noise, principled, run, shell,
	strip_on, surface_point, zones)
from bustkit import (  # noqa: E402
	add_torus, ball, bm_object, body_co, body_ix, build_lapels, build_shirt_collar, buttons, cloth, cut_vee, edit_faces,
	emit_material, frustum, head_pt, living_eye, ramp, ring_band, set_body_co)

# The paint pass's settings per option (render.js reads these lines): wine and gold; C's glow is lamplight (no cyan).
# PAINT A {"bgTop":"#1a0d14","bgBottom":"#07050a","bgGlow":"#5a2a3a","glowAt":[0.70,0.30],"sparks":"#ffd27a","dust":"#c9a06a","glow":"#5ad8ff"}
# PAINT B {"bgTop":"#150c14","bgBottom":"#060409","bgGlow":"#432a42","glowAt":[0.30,0.30],"sparks":"#ffd27a","dust":"#a89cb0","glow":"#5ad8ff"}
# PAINT C {"bgTop":"#1e1009","bgBottom":"#080503","bgGlow":"#6a3a1a","glowAt":[0.40,0.72],"sparks":"#ffd27a","dust":"#e0b070","glow":"#ffc870"}

# A slight woman about thirty (mh's "young" targets are the only age it has): gender well toward female, thin, a
# little muscle; olive (a mostly Mediterranean mix).
MACRO = {"gender": 0.05, "muscle": 0.4, "weight": 0.3, "caucasian": 0.72, "african": 0.1, "asian": 0.18}

# Sharp and amused: high cheekbones over slight hollows, a narrow pointed chin, a fine straight nose, mouth corners up,
# arched brows, eyes tipped up at the outer corners; a slender neck reads long.
FACE = {
	"cheek/l-cheek-bones": 0.5, "cheek/r-cheek-bones": 0.5, "cheek/l-cheek-volume": -0.45, "cheek/r-cheek-volume": -0.45,
	"chin/chin-prominent": 0.15, "chin/chin-width": -0.55, "chin/chin-bones": -0.15,
	"nose/nose-scale-horiz": -0.25, "nose/nose-point-width": -0.3, "nose/nose-nostrils-width": -0.25, "nose/nose-hump": -0.1,
	"mouth/mouth-angles": 0.25, "mouth/mouth-cupidsbow": 0.35, "mouth/mouth-lowerlip-volume": 0.25, "mouth/mouth-upperlip-volume": 0.1,
	"eyebrows/eyebrows-angle": 0.3, "head/head-oval": 0.45, "head/head-invertedtriangular": 0.4,
	"neck/neck-scale-horiz": -0.55, "neck/neck-scale-depth": -0.35,
}
for side in "lr":
	FACE.update({f"eyes/{side}-eye-scale": 0.12, f"eyes/{side}-eye-corner1": 0.35, f"eyes/{side}-eye-bag": -0.45,
		f"ears/{side}-ear-scale": -0.2, f"ears/{side}-ear-flap": -0.35})
# "High brows": mh.build can't name this target (its modifiers end -decr/-incr or listed ends), so it goes on the
# posed body afterwards, turned with the head.
LATE_TARGETS = {"eyebrows/eyebrows-trans-up": 0.6}

CYAN = (0.25, 0.85, 1.0)
OPTIONS = {
	"A": {"label": "The Dealer", "line": "Hat low, a card in its band and another in her gloved hand; cyan eyes, a half smile.",
		"yaw": -24.0, "pitch": 4.0, "lids": (0.016, 0.012, -0.6), "eyes": "iris", "skin": (0.74, 0.68, 0.62), "glow": CYAN,
		"face": {"mouth/mouth-angles": 0.4}, "smirk": 1.6, "sunk": 0.7, "lips": (0.66, 0.40, 0.44),
		"hat": {"tip": 11.0, "cant": -5.0, "card": ("spade", 50.0)}, "hair": "low", "hand": "card",
		"collar": "high", "cravat": "tied", "vest_vee": 0.032, "vest_depth": 0.11, "button0": 0.10,
		"world": (0.16, 0.07, 0.10), "cam": (2.08, -0.03),
		"lights": [("Key", (-0.74, -0.46, 0.44), 32.0, (1.0, 0.74, 0.50), 0.5), ("Fill", (0.9, -1.6, -0.3), 10.0, (0.95, 0.6, 0.7), 1.5)],
		"rim": ((1.3, 1.2, 0.5), 105.0, (0.45, 0.85, 1.0), 0.8)},
	"B": {"label": "Marked", "line": "Hat gone, hair pinned but one lock loose, grave-pale; Ned's bullet burns cyan at her temple; coat torn.",
		"yaw": -28.0, "pitch": -2.0, "lids": (0.006, 0.004, 0.0), "eyes": "iris", "skin": (0.76, 0.74, 0.72), "glow": CYAN,
		"face": {"mouth/mouth-angles": -0.1}, "smirk": 0.0, "sunk": 1.0, "lips": (0.78, 0.66, 0.70),
		"hat": None, "hair": "high", "lock": True, "mark": True, "tear": True, "hand": None,
		"collar": "high", "cravat": "loose", "vest_vee": 0.032, "vest_depth": 0.11, "button0": 0.10,
		"world": (0.10, 0.08, 0.12), "cam": (2.05, -0.04),
		"lights": [("Key", (-0.85, -0.42, 0.28), 26.0, (1.0, 0.80, 0.62), 0.5), ("Rim", (1.25, 1.1, 0.45), 110.0, (0.45, 0.85, 1.0), 0.8),
			("Fill", (0.8, -1.6, -0.2), 8.0, (0.6, 0.65, 0.9), 1.5)]},
	"C": {"label": "The Riverboat Queen", "line": "As in life on the river: hat tipped back, velvet at the throat, five cards fanned in lamplight.",
		"yaw": 22.0, "pitch": 6.0, "lids": (0.014, 0.012, -0.5), "eyes": "living", "skin": (0.78, 0.62, 0.48), "glow": (1.0, 0.78, 0.44),
		"face": {"mouth/mouth-angles": 0.35}, "smirk": 0.6, "sunk": 0.25, "lips": (0.60, 0.24, 0.28),
		"hat": {"tip": -20.0, "cant": 6.0, "card": ("heart", -42.0)}, "hair": "low", "hand": "fan", "smoke": True, "choker": True,
		"collar": "open", "cravat": None, "vest_vee": 0.03, "vest_depth": 0.12, "button0": 0.13,
		"world": (0.18, 0.10, 0.06), "cam": (2.14, 0.0),
		"lights": [("Lamp", (-0.15, -0.75, -0.55), 36.0, (1.0, 0.70, 0.36), 0.35), ("Rim", (-1.2, 1.1, 0.45), 60.0, (0.55, 0.70, 1.0), 0.8),
			("Fill", (0.9, -1.5, 0.2), 6.0, (1.0, 0.7, 0.5), 1.5)]},
}


# ---------------------------------------------------------------- materials (Odessa's own)

def velvet_material(name, pid, dark, light, sheen, rough=0.72):
	"""Wool, felt or velvet: colour drifting in big soft patches, a fine weave, creases, a sheen at grazing angles."""
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord").outputs["Object"]
		b = principled(nt, mix_rgb(nt, noise(nt, 12.0, 5.0, 0.6, tc).outputs["Fac"], dark, light), rough=rough, spec=0.35)
		nrm = bump(nt, noise(nt, 700.0, 2.0, 0.5, tc).outputs["Fac"], 0.08, 0.0003)
		nt.links.new(bump(nt, noise(nt, 26.0, 5.0, 0.6, tc).outputs["Fac"], 0.22, 0.004, nrm), b.inputs["Normal"])
		b.inputs["Sheen Weight"].default_value, b.inputs["Sheen Roughness"].default_value = 0.8, 0.35
		b.inputs["Sheen Tint"].default_value = (*lin(sheen), 1)
		return b
	return material(name, pid, build)


def brocade_material():
	"""Gold brocade: swirling thread and medallions in bright gilt over darker old-gold silk. Linen's id, so the
	paint pass keeps the pattern crisp."""
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord").outputs["Object"]
		wave = nt.nodes.new("ShaderNodeTexWave")
		wave.wave_type = "RINGS"
		for k, v in (("Scale", 70.0), ("Distortion", 14.0), ("Detail", 2.0)):
			wave.inputs[k].default_value = v
		nt.links.new(tc, wave.inputs["Vector"])
		vor = nt.nodes.new("ShaderNodeTexVoronoi")
		vor.inputs["Scale"].default_value = 55.0
		nt.links.new(tc, vor.inputs["Vector"])
		k0, k1 = (0, 0, 0, 1), (1, 1, 1, 1)
		mx = nt.nodes.new("ShaderNodeMath")
		mx.operation = "MAXIMUM"
		nt.links.new(ramp(nt, wave.outputs["Fac"], [(0.62, k0), (0.70, (0.6, 0.6, 0.6, 1))]), mx.inputs[0])
		nt.links.new(ramp(nt, vor.outputs["Distance"], [(0.08, k1), (0.12, k0), (0.24, k0), (0.28, k1), (0.33, k0)]), mx.inputs[1])
		b = principled(nt, mix_rgb(nt, mx.outputs[0], (0.46, 0.31, 0.11), (0.95, 0.77, 0.38)), rough=0.45, spec=0.5)
		nt.links.new(mx.outputs[0], b.inputs["Metallic"])
		nt.links.new(bump(nt, mx.outputs[0], 0.35, 0.0008), b.inputs["Normal"])
		return b
	return material("Brocade", ID["linen"], build)


def living_eye_material():
	"""Brown eyes, wet, no light in them (C, as in life; bustkit.living_eye keeps them out of the glow mask)."""
	return living_eye("LivingEye", [(0.80, (0.74, 0.70, 0.66)), (0.83, (0.10, 0.05, 0.03)), (0.86, (0.36, 0.19, 0.08)),
		(0.925, (0.52, 0.31, 0.12)), (0.95, (0.02, 0.015, 0.012))], 0.6, **{"Coat Weight": 1.0})


def smoke_material():
	"""Thin pale smoke, mostly transparent and faintly lit from within so it reads on the dark, fading in and out
	along each ribbon; the coat's id, so the paint pass leaves it loose and soft."""
	def build(nt):
		tr = nt.nodes.new("ShaderNodeBsdfTransparent")
		d = principled(nt, (0.80, 0.72, 0.62), rough=1.0, spec=0.0, **{"Emission Strength": 0.3})
		d.inputs["Emission Color"].default_value = (*lin((0.9, 0.75, 0.55)), 1)
		fade = nt.nodes.new("ShaderNodeFloatCurve")
		pts = fade.mapping.curves[0].points
		pts[0].location, pts[1].location = (0.0, 0.0), (1.0, 0.0)
		for x, y in ((0.12, 0.18), (0.6, 0.14), (0.85, 0.05)):
			pts.new(x, y)
		fade.mapping.update()
		nt.links.new(nt.nodes.new("ShaderNodeHairInfo").outputs["Intercept"], fade.inputs["Value"])
		mix = nt.nodes.new("ShaderNodeMixShader")
		nt.links.new(fade.outputs[0], mix.inputs[0])
		nt.links.new(tr.outputs[0], mix.inputs[1])
		nt.links.new(d.outputs[0], mix.inputs[2])
		return mix
	return material("Smoke", ID["coat"], build)


# ---------------------------------------------------------------- the face: brows raised, the half smile, its paint

def late_targets(h, body, targets):
	used = body_ix(h)
	pos = -np.ones(len(h["verts"]), dtype=np.int64)
	pos[used] = np.arange(len(used))
	co = body_co(body)
	for rel, w in targets.items():
		ix, d = mh.load_target(rel)
		keep = pos[ix] >= 0
		co[pos[ix[keep]]] += mh.to_blender(d[keep] @ h["glob"]["head"][:3, :3].T) * w
	set_body_co(body, co)


def smirk(h, body, amount):
	"""Her left mouth corner drawn up and back: the half smile (mh's mouth-angles moves both corners)."""
	if amount <= 0:
		return
	used = body_ix(h)
	rest = h["morphed"][used]
	lips = rest[sum(h["weights"](b) for b in h["bones"] if b.startswith("oris"))[used] > 0.3]
	corner = lips[np.argmax(lips[:, 0])]                  # her left corner: the lips' outermost point
	k = np.clip(1.0 - np.linalg.norm((rest - corner) * np.array([1.0, 1.3, 1.0]), axis=1) / 0.22, 0, 1) ** 1.6 * amount
	move = mh.to_blender((h["glob"]["head"][:3, :3] @ np.array([0.015, 0.04, -0.015]))[None])[0]
	set_body_co(body, body_co(body) + k[:, None] * move)


def above_hairline(hf, v):
	"""How far each rest-space point v sits above Odessa's hairline (decimetres; negative below it, and on the ears)."""
	ey = hf["eye_y"]
	cz = (hf["verts"][:, 2].max() + hf["verts"][:, 2].min()) * 0.5
	a = np.abs(np.degrees(np.arctan2(v[:, 0], v[:, 2] - cz)))
	s = np.clip((a - 28) / 30, 0, 1) ** 2 * (3 - 2 * np.clip((a - 28) / 30, 0, 1))   # high on the brow, low at the temples
	line = np.where(a < 105, ey + 0.58 - 0.44 * s, ey - 0.50)
	return np.where((np.abs(v[:, 0]) > 0.66) & (v[:, 1] < ey + 0.25), -1.0, v[:, 1] - line)


def repaint_face(h, hf, body, opt):
	"""bust.zones' dead-eyed colouring eased to opt["sunk"] (1 = the Revenant's, less for the living); the lips
	tinted; the scalp dark under the hair, so gaps between strands read as hair; a dark line along each upper lid."""
	used = body_ix(h)
	verts = h["verts"][used]
	c = zones(h, verts, [mh.apply(h["glob"]["head"], e) for e in hf["eyes"]], used)
	c[:, :3] = 1.0 - (1.0 - c[:, :3]) * opt["sunk"]
	hw = h["weights"]("head")[used]
	front = verts[hw > 0.5]
	lip = front[np.argmax(front[:, 2])] + np.array([0.0, -0.37, -0.20])     # under the nose's tip
	k = np.clip(1.0 - np.linalg.norm((verts - lip) * np.array([0.75, 2.4, 1.2]), axis=1) / 0.30, 0.0, 1.0) ** 0.6
	c[:, :3] *= 1.0 - k[:, None] * (1.0 - np.array(opt["lips"]))
	sc = np.clip(above_hairline(hf, h["morphed"][used]) / 0.06 + 0.7, 0, 1) * (hw > 0.5)
	c[:, :3] *= 1.0 - sc[:, None] * (1.0 - np.array([0.07, 0.05, 0.045]))
	for bone in ("orbicularis03.L", "orbicularis03.R"):
		c[:, :3] *= 1.0 - (np.clip((h["weights"](bone)[used] - 0.35) / 0.4, 0, 1) * 0.55)[:, None]
	body.data.color_attributes["zones"].data.foreach_set("color", c.astype(np.float32).ravel())


# ---------------------------------------------------------------- the hat

def build_hat(h, hf, opt):
	"""The flat-crowned gambler: a low straight crown with a flat top, a wide flat brim rolled up at its edge, a wine
	silk band with a card tucked in it. Built in the head's rest space (decimetres), then tipped and canted."""
	spec, hv = opt["hat"], hf["verts"]
	band_y = hf["eye_y"] + 0.46
	ring = hv[np.abs(hv[:, 1] - band_y) < 0.12]
	cz = (ring[:, 2].max() + ring[:, 2].min()) * 0.5
	rx, rz = np.abs(ring[:, 0]).max() + 0.12, (ring[:, 2].max() - ring[:, 2].min()) * 0.5 + 0.12
	tip, cant, H, nseg = math.radians(spec["tip"]), math.radians(spec["cant"]), 0.95, 96
	pivot = np.array([0.0, band_y, cz])

	def place(p):
		q = np.atleast_2d(p) - pivot
		c, s = math.cos(tip), math.sin(tip)
		q = np.stack([q[:, 0], q[:, 1] * c - q[:, 2] * s, q[:, 1] * s + q[:, 2] * c], axis=1)
		c, s = math.cos(cant), math.sin(cant)
		q = np.stack([q[:, 0] * c - q[:, 1] * s, q[:, 0] * s + q[:, 1] * c, q[:, 2]], axis=1)
		return mh.to_blender(mh.apply(h["glob"]["head"], q + pivot))

	def rings(rows, cap=False):
		# rows of (x scale, z scale, y) round the crown's oval; quads between rows, a fan to the centre on the last
		ph = np.linspace(0, 2 * math.pi, nseg, endpoint=False)
		v = [[math.sin(p) * sx, band_y + y, cz + math.cos(p) * sz] for sx, sz, y in rows for p in ph]
		n = len(rows)
		f = [[i * nseg + j, i * nseg + (j + 1) % nseg, (i + 1) * nseg + (j + 1) % nseg, (i + 1) * nseg + j] for i in range(n - 1) for j in range(nseg)]
		if cap:
			v.append([0.0, band_y + H, cz])
			f += [[(n - 1) * nseg + j, (n - 1) * nseg + (j + 1) % nseg, len(v) - 1] for j in range(nseg)]
		return place(np.array(v)), f
	felt = velvet_material("BlackFelt", ID["felt"], (0.010, 0.009, 0.011), (0.03, 0.027, 0.032), (0.30, 0.26, 0.30), rough=0.85)
	# crown: a squarish superellipse in profile (straight sides, flat top, tight round edge), a little tapered
	prof = [(math.cos(a) ** (1 / 6), math.sin(a) ** (1 / 6)) for a in np.linspace(0, math.pi / 2, 24, endpoint=False)]
	cloth(mesh_object("HatCrown", *rings([(rx * r * (1 - 0.05 * y), rz * r * (1 - 0.05 * y), y * H) for r, y in prof], True)), 0.004, felt, -1.0)
	# brim: flat and wide, its edge rolled up, a little more at the sides, a touch down at the front
	bv, bf = [], []
	for s in np.linspace(0, 1, 13):
		for p in np.linspace(0, 2 * math.pi, nseg, endpoint=False):
			y = 0.10 * s ** 5 + 0.10 * s ** 2 * math.sin(p) ** 2 - 0.03 * s ** 2 * max(math.cos(p), 0) ** 2
			bv.append([math.sin(p) * (rx * 0.97 + 0.80 * s), band_y + y + 0.01, cz + math.cos(p) * (rz * 0.97 + 0.86 * s)])
	bf = [[i * nseg + j, (i + 1) * nseg + j, (i + 1) * nseg + (j + 1) % nseg, i * nseg + (j + 1) % nseg] for i in range(12) for j in range(nseg)]
	brim = mesh_object("HatBrim", place(np.array(bv)), bf)
	add_mod(brim, "SOLIDIFY", thickness=0.005, offset=0.0, use_rim=True)
	add_mod(brim, "SUBSURF", levels=1, render_levels=2)
	brim.data.materials.append(felt)
	cloth(mesh_object("HatBand", *rings([(rx * 1.012, rz * 1.012, 0.02), (rx * 1.012, rz * 1.012, 0.24)])), 0.0015,
		velvet_material("HatBand", ID["bandana"], (0.20, 0.03, 0.06), (0.34, 0.06, 0.10), (0.8, 0.4, 0.45), 0.4))
	# The card tucked in the band at phi (degrees from the front toward her left), leaning out and canted.
	suit, phi = spec["card"]
	ph = math.radians(phi)
	out, tan = np.array([math.sin(ph), 0.0, math.cos(ph)]), np.array([math.cos(ph), 0.0, -math.sin(ph)])
	up = np.array([0.0, math.cos(0.245), 0.0]) + out * math.sin(0.245)
	tilt = math.radians(-18.0 if phi < 0 else 18.0)
	ax, az = tan * math.cos(tilt) + up * math.sin(tilt), up * math.cos(tilt) - tan * math.sin(tilt)
	centre = np.array([out[0] * rx * 1.03, band_y + 0.42, cz + out[2] * rz * 1.03])
	card("HatCard", suit, lambda p: place(centre + (p[:, 0:1] * ax - p[:, 1:2] * out + p[:, 2:3] * az) * 10.0))


# ---------------------------------------------------------------- cards and the gloved hand

CARD_W, CARD_H, CARD_T = 0.063, 0.088, 0.0006


def pip_shapes(suit, n=32):
	"""A suit pip's outline polygons in a unit box (x, z): plain shapes anyone draws, from no deck's artwork."""
	if suit == "diamond":
		return [[(0.0, 1.0), (0.66, 0.0), (0.0, -1.0), (-0.66, 0.0)]]
	t = np.linspace(0, 2 * math.pi, n, endpoint=False)
	hx = 16 * np.sin(t) ** 3 / 17
	hz = (13 * np.cos(t) - 5 * np.cos(2 * t) - 2 * np.cos(3 * t) - np.cos(4 * t)) / 17 + 0.15
	stem = [(0.0, -0.3), (0.28, -1.0), (-0.28, -1.0)]
	if suit == "heart":
		return [list(zip(hx, hz))]
	if suit == "spade":
		return [list(zip(hx * 0.9, -hz * 0.8 + 0.2)), stem]
	circ = lambda cx, cz: [(cx + 0.34 * math.cos(a), cz + 0.34 * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 18, endpoint=False)]
	return [circ(0.0, 0.5), circ(-0.38, 0.02), circ(0.38, 0.02), stem]


def card(name, suit, xf):
	"""A playing card: a thin rounded slab of ivory board, a red or black pip in the middle and small ones in two
	corners. xf maps the card's own points (metres; x across, z up, its face toward -y) into the world."""
	bm = bmesh.new()
	r = 0.0045
	outline = [(cx * (CARD_W / 2 - r) + r * math.cos(a), cz * (CARD_H / 2 - r) + r * math.sin(a))
		for cx, cz, a0 in ((1, 1, 0), (-1, 1, 90), (-1, -1, 180), (1, -1, 270)) for a in np.radians(a0 + np.linspace(0, 90, 6))]

	def face(pts, y, mi, flip=False):
		vs = [bm.verts.new(tuple(q)) for q in xf(np.array([[x, y, z] for x, z in pts]))]
		bm.faces.new(vs[::-1] if flip else vs).material_index = mi
		return vs
	front, back = face(outline, -CARD_T / 2, 0, True), face(outline, CARD_T / 2, 0)
	m = len(outline)
	for i in range(m):
		bm.faces.new((front[i], front[(i + 1) % m], back[(i + 1) % m], back[i]))
	for cx, cz, s, rot in ((0.0, 0.0, 0.0125, 1), (-CARD_W / 2 + 0.0075, CARD_H / 2 - 0.0115, 0.0042, 1), (CARD_W / 2 - 0.0075, -CARD_H / 2 + 0.0115, 0.0042, -1)):
		for poly in pip_shapes(suit):
			face([(cx + x * s * rot, cz + z * s * rot) for x, z in poly], -CARD_T / 2 - 0.00012, 1 if suit in ("heart", "diamond") else 2, True)
	inks = [((0.88, 0.84, 0.76), 0.35), ((0.66, 0.05, 0.07), 0.3), ((0.02, 0.02, 0.025), 0.3)]
	return bm_object(name, bm, [material(f"Card{i}", ID["linen"], lambda nt, c=c, r=r: principled(nt, c, rough=r, spec=0.5))
		for i, (c, r) in enumerate(inks)], smooth=False)


def glove(M, mat, thumb_fwd=0.0):
	"""A black-gloved hand gripping a card's bottom edge, in the card's own frame (M: card frame -> world): palm and
	heel below the card facing us, fingers behind it, the thumb across its face; spheres and capsules unioned by a
	voxel remesh and smoothed into one glove. Then its flared cuff and the shirt's cuff."""
	bm = bmesh.new()

	def blob(c, r, scale=(1.0, 1.0, 1.0)):
		bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=12, radius=r, matrix=Matrix.Translation(c) @ Matrix.Diagonal((*scale, 1.0)))

	def cap(p0, p1, r0, r1):
		p0, p1 = Vector(p0), Vector(p1)
		m = Matrix.Translation((p0 + p1) * 0.5) @ (p1 - p0).to_track_quat("Z", "Y").to_matrix().to_4x4()
		bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=r0, radius2=r1, depth=(p1 - p0).length, matrix=m)
		blob(p0, r0)
		blob(p1, r1)
	blob(Vector((0.003, 0.016, -0.086)), 1.0, (0.035, 0.0135, 0.046))           # palm
	blob(Vector((-0.020, 0.011, -0.104)), 1.0, (0.015, 0.012, 0.024))           # the thumb's pad
	for x, lean in ((-0.021, 0.004), (-0.007, 0.001), (0.007, -0.001), (0.020, -0.004)):
		cap((x, 0.0095, -0.044), (x + lean, 0.0095, 0.016), 0.0084, 0.0074)
	ty = -thumb_fwd
	cap((-0.026, 0.008, -0.104), (-0.039, -0.001 + ty, -0.068), 0.0112, 0.0100)
	cap((-0.039, -0.001 + ty, -0.068), (-0.022, -0.0096 + ty, -0.036), 0.0100, 0.0090)
	cap((0.003, 0.018, -0.118), (0.006, 0.028, -0.17), 0.021, 0.023)             # wrist
	bm.transform(M)
	g = bm_object("Glove", bm, [mat])
	add_mod(g, "REMESH", mode="VOXEL", voxel_size=0.0014, use_smooth_shade=True)
	add_mod(g, "SMOOTH", factor=1.0, iterations=6)
	linen = material("CuffLinen", ID["linen"], lambda nt: principled(nt, (0.82, 0.80, 0.75), rough=0.6, spec=0.3))
	return [g, frustum("GloveCuff", M @ Vector((0.004, 0.026, -0.145)), M @ Vector((0.008, 0.036, -0.20)), 0.029, 0.038, mat),
		frustum("ShirtCuff", M @ Vector((0.008, 0.036, -0.19)), M @ Vector((0.010, 0.042, -0.225)), 0.040, 0.041, linen)]


def hand_and_cards(h, eyes, opt, wine):
	"""A: one card held up beside the face. C: a fan of five held low before the chest. The wine sleeve runs out of
	frame below."""
	leather = leather_material((0.03, 0.026, 0.03))
	xf = lambda F: (lambda p: np.array([tuple(F @ Vector(q)) for q in p]))
	if opt["hand"] == "card":
		mid = (eyes[0].location + eyes[1].location) * 0.5
		M = Matrix.Translation(mid + Vector((0.125, -0.06, -0.05))) @ Matrix.Rotation(-0.24, 4, "Y")
		card("HandCard", "heart", xf(M))
		parts = glove(M, leather)
	else:
		M = Matrix.Translation(Vector(neck_frame(h)) + Vector((-0.11, -0.13, -0.05))) @ Matrix.Rotation(0.18, 4, "Y") @ Matrix.Rotation(-0.35, 4, "X")
		for k, suit in enumerate(("club", "diamond", "spade", "heart", "heart")):
			pivot = Matrix.Translation((0.0, -0.0011 * k, -0.034)) @ Matrix.Rotation(math.radians(-36 + 18 * k), 4, "Y") @ Matrix.Translation((0.0, 0.0, 0.034))
			card(f"FanCard{k}", suit, xf(M @ pivot))
		parts = glove(M, leather, thumb_fwd=0.006)
	return parts + [frustum("Sleeve", M @ Vector((0.010, 0.042, -0.215)), M @ Vector((0.03, 0.09, -0.52)), 0.046, 0.056, wine)]


# ---------------------------------------------------------------- hair, brows, lashes

def hair_lay(hf, root, target, steps):
	"""A strand combed from root to target round the head (rest space): its direction from the head's centre slerps
	from one to the other through a midway direction while its distance blends, so it wraps over the skull, not
	through it. Forehead to nape (nearly opposite): the midway is pushed up over the crown, never down the face."""
	C = np.array([0.0, hf["eye_y"] + 0.15, (hf["front"] + hf["back"]) * 0.5 - 0.1])
	a, b = root - C, target - C
	ra, rb = np.linalg.norm(a), np.linalg.norm(b)
	ua, ub = a / ra, b / rb
	m = ua + ub
	m = m + np.array([0.0, 1.0, -0.3]) * max(0.0, 1.3 - np.linalg.norm(m))
	m /= np.linalg.norm(m)

	def slerp(p, q, t):
		om = math.acos(np.clip(p @ q, -1, 1))
		return p if om < 1e-4 else (math.sin((1 - t) * om) * p + math.sin(t * om) * q) / math.sin(om)
	return np.array([C + (slerp(ua, m, 2 * t) if t < 0.5 else slerp(m, ub, 2 * t - 1)) * (ra + (rb - ra) * t)
		for t in np.linspace(0, 1, steps)])


def build_hair(h, hf, body, opt):
	"""Dark hair pinned up: combed back from the hairline into a knot (low at the nape under a hat, high at the crown
	without one); B lets one lock loose down the far cheek. Combed strands are grown from the knot out, so their thin
	ends make a soft hairline (curves taper root to tip)."""
	rng = np.random.default_rng(17)
	bvh, bm = body_bvh(body)
	hv, ey = hf["verts"], hf["eye_y"]
	cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5
	high = opt["hair"] == "high"
	by, rb = ey + (0.74 if high else -0.22), (0.48 if high else 0.34)
	near = hv[(np.abs(hv[:, 1] - by) < 0.1) & (np.abs(hv[:, 0]) < 0.25)]
	bun = np.array([0.0, by, near[:, 2].min() - rb * (0.35 if high else 0.25)])
	scalp = hv[above_hairline(hf, hv) > 0]

	def lie(p, c, snap):
		# combed hair lies at c off the scalp (snap); toward the knot it may stand off, never sink in
		hit = bvh.find_nearest(p)
		return p if hit[0] is None or (not snap and (p - hit[0]).dot(hit[1]) >= c) else hit[0] + hit[1] * c
	combed = []
	for r in scalp[rng.choice(len(scalp), min(420, len(scalp)), replace=False)]:
		tgt = bun + rng.normal(size=3) * rb * 0.35
		group = []
		for k in range(8):
			world = mh.to_blender(mh.apply(h["glob"]["head"], hair_lay(hf, r + rng.normal(size=3) * 0.05, tgt + rng.normal(size=3) * 0.04, 18)))
			clear = 0.0025 + 0.0015 * rng.random()
			group.append(np.array([tuple(lie(Vector(p), 0.0008 + clear * min(1.0, i / 4.0) ** 2, i < 15)) for i, p in enumerate(world)]))
		combed += [s[::-1] for s in clump(group, 0.3)]   # gathered toward the knot, then turned to taper there
	# the knot: coils of hair wound round its centre in tilted planes
	c = head_pt(h, bun)
	axis = (c - head_pt(h, [0.0, by, cz])).normalized()
	coils = []
	for k in range(80):
		ax = (axis + Vector(rng.normal(size=3)) * 0.9).normalized()
		u = ax.orthogonal().normalized()
		v = ax.cross(u)
		rad, turns, ph = rb * 0.1 * (0.55 + 0.45 * rng.random()), 0.9 + 0.6 * rng.random(), rng.random() * 6.28
		for s in range(14):
			off = Vector(rng.normal(size=3)) * 0.0025
			coils.append(np.array([tuple(c + off + (u * math.cos(an) + v * math.sin(an)) * rad * (1 + 0.06 * math.sin(3 * an)) + ax * 0.004 * math.sin(an * 0.5))
				for an in ph + np.linspace(0, 1, 30) * turns * 2 * math.pi]))
	mat = hair_material(0.97, 0.12, 0.4)
	out = [curves_object("Hair", combed, 0.0026, 0.0008, mat), curves_object("Knot", coils, 0.0024, 0.0012, mat)]
	if opt.get("lock"):
		# one lock come loose at her right temple, curling down past the cheek to the jaw
		root = head_pt(h, [-0.46, ey + 0.50, hf["front"] - 0.50])
		lat = (head_pt(h, [-1.0, ey, cz]) - head_pt(h, [0.0, ey, cz])).normalized()
		fwd = (head_pt(h, [0.0, ey, cz + 1.0]) - head_pt(h, [0.0, ey, cz])).normalized()
		group = []
		for k in range(44):
			r0, ph = root + Vector(rng.normal(size=3)) * 0.002, rng.normal() * 0.25
			group.append(np.array([tuple(lie(r0 + Vector((0, 0, -0.16 * t)) + lat * (0.008 + 0.012 * t + 0.006 * t * math.cos(an))
				+ fwd * (0.010 * math.sin(math.pi * t) + 0.006 * t * math.sin(an)), 0.004, False))
				for t, an in ((t, 2 * math.pi * 2.2 * t + ph) for t in np.linspace(0, 1, 30))]))
		out.append(curves_object("Lock", clump(group, 0.8), 0.0022, 0.0005, mat))
	bm.free()
	return out


def build_brows(h, hf, body):
	"""High, arched, groomed brows: short fine strands along the ridge, the arch two-thirds out."""
	rng = np.random.default_rng(3)
	bvh, bm = body_bvh(body)
	strands = []
	for e in hf["eyes"]:
		sx = 1.0 if e[0] > 0 else -1.0
		for k in range(150):
			t = rng.random()
			x = e[0] + sx * (-0.18 + 0.44 * t)
			y = e[1] + 0.22 + 0.09 * math.sin(t * math.pi * 0.8) - 0.03 * t * t + rng.normal() * 0.009 * (1.2 - t)
			p, q = head_pt(h, [x, y, hf["front"] + 0.3]), head_pt(h, [x, y, hf["front"] - 1.0])
			hit = bvh.ray_cast(p, (q - p).normalized(), (q - p).length)
			if hit[0] is None:
				continue
			along = head_pt(h, [x + sx * 0.5, y + 0.10 - 0.12 * t, hf["front"]]) - head_pt(h, [x, y, hf["front"]])
			along = (along - hit[1] * along.dot(hit[1])).normalized()
			L = 0.005 + 0.003 * rng.random()
			strands.append(np.array([tuple(hit[0] + hit[1] * (0.0005 + 0.001 * math.sin(i / 3 * math.pi)) + along * (L * i / 3)) for i in range(4)]))
	bm.free()
	return curves_object("Brows", strands, 0.00022, 0.00007, hair_material(0.95, 0.15, 0.4))


def build_lashes(h, hf, body, eyes):
	"""Upper lashes: short dark curls from the lid's edge over each eyeball, turning up."""
	rng = np.random.default_rng(4)
	co = body_co(body)
	fwd = head_axis(h, hf)[1]
	strands = []
	for e in eyes:
		c = np.array(e.location)
		rel = co - c
		d = np.linalg.norm(rel, axis=1)
		for p in co[(d < 0.0140) & (d > 0.0118) & (rel[:, 2] > 0.002) & (rel[:, :2] @ fwd[:2] > 0.005)]:
			root = Vector(p) + Vector(rng.normal(size=3)) * 0.0003
			d1 = ((Vector(p) - Vector(c)).normalized() + Vector((0, 0, 0.4))).normalized()
			L = 0.0035 + 0.002 * rng.random()
			strands.append(np.array([tuple(root + d1 * (L * i / 4) + Vector((0, 0, 0.001 * (i / 4) ** 2))) for i in range(5)]))
	return curves_object("Lashes", strands, 0.0002, 0.00006, hair_material(0.98, 0.1, 0.35))


# ---------------------------------------------------------------- jewellery, the mark

def earrings(h, gold):
	"""Gold drops: a bead at each lobe (the lowest of the head's outermost points) and a teardrop under it."""
	used, m = body_ix(h), h["morphed"]
	w = h["weights"]("head")
	for sx in (-1.0, 1.0):
		pts = m[used[(w[used] > 0.5) & (m[used, 0] * sx > 0)]]
		far = pts[np.abs(pts[:, 0]) > np.abs(pts[:, 0]).max() * 0.85]
		top = head_pt(h, far[np.argmin(far[:, 1])] + np.array([sx * -0.04, -0.03, 0.0]))
		ball("EarBead", top, 0.0034, gold)
		ball("EarDrop", top + Vector((0, 0, -0.014)), 0.0056, gold, (0.8, 0.8, 1.6))


def bullet_mark(h, hf, body):
	"""Ned's bullet at her left temple: a dark puckered hole, a scar ring, and threads of cyan light running out of
	it over the skin (the eye's id, so the paint pass blooms them)."""
	rng = np.random.default_rng(23)
	bvh, bm = body_bvh(body)
	p0, p1 = head_pt(h, [1.6, hf["eye_y"] + 0.13, 1.15]), head_pt(h, [0.0, hf["eye_y"] + 0.13, 0.80])   # the temple, below the hairline
	at, n, _, _ = bvh.ray_cast(p0, (p1 - p0).normalized(), (p1 - p0).length)
	if at is None:
		bm.free()
		return
	glow = emit_material("MarkGlow", CYAN, 7.0, ID["eye"])
	ball("Hole", at - n * 0.0006, 0.0055, material("Wound", ID["skin"], lambda nt: principled(nt, (0.10, 0.025, 0.035), rough=0.5, spec=0.4)), (1, 1, 0.35), n)
	bmr = bmesh.new()
	add_torus(bmr, 0.0072, 0.0018, n.to_track_quat("Z", "Y").to_matrix().to_4x4())
	bm_object("ScarRing", bmr, [material("Scar", ID["skin"], lambda nt: principled(nt, (0.58, 0.44, 0.45), rough=0.5, spec=0.4))]).location = at
	ball("MarkCore", at + n * 0.0005, 0.0016, glow)
	threads = []
	for k in range(5):
		p, d = Vector(at), Vector(rng.normal(size=3))
		pts = [tuple(p)]
		for s in range(14):
			hn = bvh.find_nearest(p)[1]
			d = (d + Vector(rng.normal(size=3)) * 0.6).normalized()
			p = p + (d - hn * d.dot(hn)).normalized() * (0.0022 + 0.0014 * rng.random())
			hp = bvh.find_nearest(p)
			p = hp[0] + hp[1] * 0.0005
			pts.append(tuple(p))
		threads.append(np.array(pts))
	curves_object("MarkThreads", threads, 0.0006, 0.0002, glow)
	bm.free()


# ---------------------------------------------------------------- the clothes

def neck_section(h, z0, dz=0.008):
	"""The neck's centre (x, y) and outer radius at the height z0 (world), from the body's own neck vertices."""
	used = body_ix(h)
	nw = h["weights"]("neck01") + h["weights"]("neck02") + h["weights"]("neck03")
	nv = mh.to_blender(h["verts"][used[nw[used] > 0.5]])
	band = nv[np.abs(nv[:, 2] - z0) < dz]
	c = band[:, :2].mean(axis=0)
	return c, np.percentile(np.linalg.norm(band[:, :2] - c, axis=1), 92)


def build_clothes(h, body, opt, wine):
	"""Shirt, brocade waistcoat and wine tailcoat as shells of the body: the waistcoat in a V closing below the
	cravat (cut_vee only widens, as an open coat does), the tailcoat open in a wide V with lapels and a rolled
	collar; the high white collar and black cravat (A, B) or an open neck and choker (C); the watch chain."""
	neck = neck_frame(h)
	gold = brass_material()
	high = opt["collar"] == "high"
	tone = (0.90, 0.88, 0.83) if high else (0.80, 0.76, 0.68)
	linen = material("Linen", ID["linen"], lambda nt: principled(nt, tone, rough=0.6, spec=0.3, **{"Sheen Weight": 0.2}))
	shirt = cloth(shell(body, h, "Shirt", 0.004, 0.0, neck[2] + (0.0 if high else -0.008), 0.3)[0], 0.002, linen)
	vest = shell(body, h, "Waistcoat", 0.009, 0.002, neck[2] - 0.012, 0.3)[0]
	vee, depth = opt["vest_vee"], opt["vest_depth"]
	edit_faces(vest, lambda c: abs(c.x - neck[0]) > 0.105 or (neck[1] - c.y > 0.02 and neck[2] - c.z < depth
		and abs(c.x - neck[0]) < vee * (1.0 - max(neck[2] - c.z, 0.0) / depth) ** 0.8))      # armholes and the V
	cloth(vest, 0.003, brocade_material())
	buttons(vest, neck, 4, 0.034, gold, radius=0.0048, x=0.004, z0=opt["button0"])
	coat = shell(body, h, "Coat", 0.012, 0.008, neck[2] + 0.02)[0]
	cut_vee(coat, neck, 0.05)
	if opt.get("tear"):
		tear(coat, h, wine)
	cloth(coat, 0.004, wine)
	build_lapels(coat, neck, 0.05, wine, vest)
	top_r = neck_section(h, neck[2] + 0.02)[1] + 0.026
	cv = []
	for t in np.linspace(0, 1, 10):            # the rolled collar standing behind the neck, open at the front
		for th in np.linspace(math.radians(42), math.radians(318), 110):
			back = 0.5 - 0.5 * math.cos(th)
			r = top_r * (0.95 + 0.20 * t) - 0.01 * (1 - back) * t
			cv.append([neck[0] + math.sin(th) * r, neck[1] - math.cos(th) * r, neck[2] - 0.02 + (0.03 + 0.035 * back) * t])
	cv = np.array(cv)
	cloth(grid_object("CoatCollar", *(cv[:, i].reshape(10, -1) for i in range(3))), 0.005, wine)
	if high:
		nr = build_shirt_collar(h, neck, linen, height=0.072)[1]
		cravat(shirt, neck, nr, opt["cravat"] == "loose")
	if opt.get("choker"):
		c, r = neck_section(h, neck[2] + 0.02)
		velvet = velvet_material("Velvet", ID["black"], (0.012, 0.01, 0.014), (0.03, 0.025, 0.03), (0.5, 0.45, 0.55), 0.8)
		ring_band("Choker", c, [(lambda t: r + 0.0025, lambda t, dz=dz: neck[2] + 0.02 + dz) for dz in (-0.007, 0.0, 0.007)], velvet)
		ball("ChokerDrop", Vector((c[0], c[1] - r - 0.0055, neck[2] + 0.007)), 0.0045, gold, (0.8, 0.5, 1.3))
	watch_chain(vest, neck, gold, opt["button0"])


def tear(coat, h, wine):
	"""B: the coat ripped open over her left shoulder, ragged, threads hanging; the shirt shows through."""
	rng = np.random.default_rng(31)
	c = Vector(mh.to_blender(mh.apply(h["glob"]["shoulder01.L"], h["bones"]["shoulder01.L"]["head"][None]))[0]) + Vector((0.03, -0.035, 0.045))
	lim = lambda d: 0.062 * (1 + 0.35 * math.sin(math.atan2(d.z, d.x) * 3 + 1.0) + 0.2 * math.sin(math.atan2(d.z, d.x) * 7))
	edge = [p for p in edit_faces(coat, lambda p: (p - c).length < lim(p - c)) if (p - c).length < 0.08]
	threads = []
	for i in (rng.choice(len(edge), min(40, len(edge)), replace=False) if edge else []):
		d = (Vector(rng.normal(size=3)) * 0.4 + Vector((0, -0.2, -1.0))).normalized()
		L = 0.006 + 0.012 * rng.random()
		threads.append(np.array([tuple(edge[i] + d * (L * k / 4) + Vector((0, 0, -0.002 * (k / 4) ** 2))) for k in range(5)]))
	if threads:
		curves_object("Threads", threads, 0.0006, 0.0003, wine)


def cravat(shirt, neck, nr, loose):
	"""The black cravat: a band round the collar's foot, a knot at the throat, a puffed fall down the shirt front
	into the waistcoat, a gold stick pin. Loose (B): the band slipped and sagging, the knot dragged down and aside."""
	silk = material("Silk", ID["bandana"], lambda nt: principled(nt, (0.02, 0.018, 0.022), rough=0.28, spec=0.5, **{"Sheen Weight": 0.3}))
	drop = 0.04 if loose else 0.0
	ring_band("CravatBand", neck, [(lambda t: nr + 0.004 + (0.012 if loose else 0.0) * (0.5 + 0.5 * math.cos(t)),
		lambda t, f=f: neck[2] + 0.001 + (0.024 if loose else 0.010) * f - drop * max(math.cos(t), 0) ** 2) for f in np.linspace(0, 1, 4)], silk)
	kx = neck[0] + (0.014 if loose else 0.0)
	knot = Vector((kx, neck[1] - nr - 0.011, neck[2] + 0.008 - drop))
	ball("CravatKnot", knot, 0.011, silk, (1.4, 0.55, 0.8))
	rows = []
	for t in np.linspace(0, 1, 16):
		w, x0 = 0.012 + 0.024 * t ** 0.6, kx + (0.014 * t if loose else 0.0)
		# puffed: the silk stands off the shirt in the middle and in two soft folds
		rows.append([(x0 + (i / 6 - 0.5) * 2 * w, knot.z - 0.004 - t * 0.10, (0.004 + 0.006 * t) * math.sin(math.pi * i / 6) + 0.0015 * math.sin(math.pi * i / 2) * t) for i in range(7)])
	fall = strip_on(bvh_of(shirt), rows, "CravatFall", 0.0015, silk, 0.0015)
	sp = None if loose else surface_point(bvh_of(fall), kx + 0.002, knot.z - 0.035)
	if sp:
		ball("StickPin", sp[0] + sp[1] * 0.002, 0.0035, brass_material())


def watch_chain(vest, neck, gold, z_btn):
	"""A gold watch chain from a waistcoat button across to the pocket on her right, hanging in a shallow curve:
	small oval links turned alternately flat and edge-up, the way a chain lies."""
	bvh = bvh_of(vest)
	a, b = np.array([neck[0] + 0.004, neck[2] - z_btn]), np.array([neck[0] - 0.062, neck[2] - z_btn + 0.006])
	pts = [sp for sp in (surface_point(bvh, *(a + (b - a) * t - np.array([0.0, 0.028 * math.sin(math.pi * t)]))) for t in np.linspace(0, 1, 34)) if sp]
	bm = bmesh.new()
	for k, (p, nrm) in enumerate(pts):
		tan = (pts[min(k + 1, len(pts) - 1)][0] - pts[max(k - 1, 0)][0]).normalized()
		side = nrm if k % 2 else tan.cross(nrm)
		side = (side - tan * side.dot(tan)).normalized()
		m = Matrix((tan, side, tan.cross(side))).transposed().to_4x4()
		m.translation = p + nrm * 0.004
		add_torus(bm, 0.0026, 0.0007, m @ Matrix.Diagonal((1.3, 1.0, 1.0, 1.0)), 10, 5)
	bm_object("WatchChain", bm, [gold])


def build_smoke(h):
	"""C: a cheroot's smoke (the cheroot below frame): three soft ribbons twisting round each other as they rise at
	the right, widening, drifting in toward her and curling over at the top."""
	o = Vector(mh.to_blender(mh.apply(h["glob"]["head"], h["bones"]["head"]["head"])))
	strands = []
	for k in range(3):
		pts = []
		for t in np.linspace(0, 1, 70):
			amp, an, curl = 0.004 + 0.03 * t ** 1.3, 2 * math.pi * 1.6 * t + k * 2.1, max(t - 0.75, 0.0) * 4
			pts.append((o.x + 0.19 - 0.07 * t * t + amp * math.sin(an) - 0.03 * math.sin(curl * 2.5) * curl,
				o.y - 0.15 + 0.5 * amp * math.cos(an), o.z - 0.36 + 0.46 * t + 0.02 * math.cos(curl * 2.5) * curl))
		strands.append(np.array(pts))
	curves_object("Smoke", strands, 0.0015, 0.011, smoke_material())


# ---------------------------------------------------------------- the bust

def rim_sparing(h, opt, spare):
	"""The cyan rim as bust.stage would place it (offset from the head bone raised by cam[1]), light-linked to skip
	spare: under A's brim it turned her dark hair pale grey."""
	loc, power, color, size = opt["rim"]
	target = Vector(mh.to_blender(mh.apply(h["glob"]["head"], h["bones"]["head"]["head"]))) + Vector((0.0, 0.0, opt["cam"][1]))
	lo = area_light("Rim", target, loc, power, color, size)
	skip = bpy.data.collections.new("RimSkips")
	lo.light_linking.receiver_collection = skip
	for o in spare:
		skip.objects.link(o)
	for co in skip.collection_objects:
		co.light_linking.link_state = "EXCLUDE"


def build(opt):
	"""Odessa's bust for one option, on an empty scene. Returns (h, eyes) for bust.run."""
	h, hf, body, eyes = build_human(opt, {**FACE, **opt.get("face", {})}, MACRO, "Odessa")
	late_targets(h, body, LATE_TARGETS)
	smirk(h, body, opt.get("smirk", 0.0))
	repaint_face(h, hf, body, opt)
	if opt["eyes"] == "living":
		lm = living_eye_material()
		for e in eyes:
			e.data.materials.clear()
			e.data.materials.append(lm)
	hair = build_hair(h, hf, body, opt)
	if opt.get("rim"):
		rim_sparing(h, opt, hair)
	build_brows(h, hf, body)
	build_lashes(h, hf, body, eyes)
	if opt.get("hat"):
		build_hat(h, hf, opt)
	wine = velvet_material("Wine", ID["coat"], (0.12, 0.010, 0.055), (0.26, 0.025, 0.11), (0.70, 0.36, 0.62))
	build_clothes(h, body, opt, wine)
	earrings(h, brass_material())
	if opt.get("mark"):
		bullet_mark(h, hf, body)
	if opt.get("hand"):
		hand_and_cards(h, eyes, opt, wine)
	if opt.get("smoke"):
		build_smoke(h)
	return h, eyes


def main():
	run(OPTIONS, "Odessa", build)


if __name__ == "__main__":
	main()
