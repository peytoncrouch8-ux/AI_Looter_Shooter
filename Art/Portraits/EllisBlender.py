"""Ellis Ransom's portrait busts in Blender (Cycles), from MakeHuman's CC0 base mesh shaped into Ellis.

Run with Blender's Python module (bpy 5.2; in the cloud box: the venv in the session scratchpad):
    python Art/Portraits/EllisBlender.py <A|B|C> [--size 1024] [--samples 96] [--out <dir>]
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
import time

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mh  # noqa: E402

# The paint pass's settings per option (render.js reads these lines).
# PAINT A {"bgTop":"#0d1622","bgBottom":"#05070b","bgGlow":"#2a3f57","glowAt":[0.72,0.30],"sparks":"#ff9f1c","dust":"#7fb6d6","glow":"#5ad8ff"}
# PAINT B {"bgTop":"#1c1418","bgBottom":"#07060a","bgGlow":"#6a3e26","glowAt":[0.30,0.34],"sparks":"#ffb050","dust":"#e0b080","glow":"#5ad8ff"}
# PAINT C {"bgTop":"#0a1018","bgBottom":"#040609","bgGlow":"#2a4458","glowAt":[0.66,0.26],"sparks":"#cfeeff","dust":"#9fbccc","glow":"#5ad8ff"}

# Material ids shared with paint.js (and the march in lib.glsl).
ID = {"skin": 1, "eye": 2, "felt": 3, "leather": 4, "bandana": 5, "coat": 6, "shirt": 7, "brass": 8, "hair": 9,
	"feather": 10, "lead": 11, "linen": 12, "black": 13}

EYE_BACK = 0.022   # decimetres the eyeball's front sits behind the eye helper's cornea

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
		"lights": [("Key", (-0.72, -0.62, 0.22), 30.0, (1.0, 0.76, 0.52), 0.55), ("Rim", (1.25, 1.1, 0.45), 95.0, (0.45, 0.85, 1.0), 0.8),
			("Bounce", (0.25, -0.9, -0.65), 6.0, (1.0, 0.6, 0.4), 1.2)]},
	"C": {"label": "Dressed for the Grave", "yaw": 24.0, "pitch": 5.0, "lids": (0.022, 0.012, 0.35), "hat": "none", "bandana": "none",
		"hair": "full", "eyes": "iris", "skin": (0.70, 0.66, 0.68), "glow": (0.25, 0.85, 1.0),
		"coat": {"cape": False, "collar": "low", "lapels": True, "under": "shirt", "bandolier": False, "pin": None, "tie": True,
			"flower": True, "cloth": ((0.03, 0.03, 0.035), (0.11, 0.105, 0.115)), "frost": 1.0, "vee": 0.03},
		"world": (0.07, 0.09, 0.13), "cam": (1.98, -0.04),
		"lights": [("Key", (0.78, -0.50, 0.32), 30.0, (0.72, 0.84, 1.0), 0.5), ("Rim", (-1.25, 1.0, 0.25), 80.0, (1.0, 0.58, 0.28), 0.6),
			("Fill", (-0.9, -1.6, -0.3), 8.0, (0.5, 0.6, 1.0), 1.5)]},
}


def lin(c):
	"""sRGB pick -> linear, for Blender colour sockets."""
	return tuple(((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in c)


def reset():
	bpy.ops.wm.read_factory_settings(use_empty=True)


def link(ob):
	bpy.context.scene.collection.objects.link(ob)
	return ob


def mesh_object(name, verts, faces, smooth=True):
	me = bpy.data.meshes.new(name)
	me.from_pydata([tuple(v) for v in verts], [], [tuple(f) for f in faces])
	me.update()
	if smooth:
		me.shade_smooth()
	return link(bpy.data.objects.new(name, me))


def add_mod(ob, kind, **kw):
	m = ob.modifiers.new(kind.lower(), kind)
	for k, v in kw.items():
		setattr(m, k, v)
	return m


# ---------------------------------------------------------------- materials

def material(name, pass_id, build):
	mat = bpy.data.materials.new(name)
	mat.use_nodes = True
	nt = mat.node_tree
	for n in list(nt.nodes):
		nt.nodes.remove(n)
	out = nt.nodes.new("ShaderNodeOutputMaterial")
	shader = build(nt)
	nt.links.new(shader.outputs[0], out.inputs["Surface"])
	mat["looter_id"] = pass_id
	return mat


def principled(nt, color, rough=0.5, spec=0.5, metal=0.0, **kw):
	b = nt.nodes.new("ShaderNodeBsdfPrincipled")
	if isinstance(color, tuple):
		b.inputs["Base Color"].default_value = (*lin(color), 1.0)
	else:
		nt.links.new(color, b.inputs["Base Color"])
	b.inputs["Roughness"].default_value = rough
	b.inputs["Specular IOR Level"].default_value = spec
	b.inputs["Metallic"].default_value = metal
	for k, v in kw.items():
		b.inputs[k].default_value = v
	return b


def noise(nt, scale, detail=4.0, rough=0.55, coord=None):
	n = nt.nodes.new("ShaderNodeTexNoise")
	n.inputs["Scale"].default_value = scale
	n.inputs["Detail"].default_value = detail
	n.inputs["Roughness"].default_value = rough
	if coord is not None:
		nt.links.new(coord, n.inputs["Vector"])
	return n


def mix_rgb(nt, fac, a, b, blend="MIX"):
	m = nt.nodes.new("ShaderNodeMix")
	m.data_type = "RGBA"
	m.blend_type = blend
	for sock, val in ((m.inputs[0], fac), (m.inputs[6], a), (m.inputs[7], b)):
		if isinstance(val, (int, float)):
			sock.default_value = val
		elif isinstance(val, tuple):
			sock.default_value = (*lin(val), 1.0)
		else:
			nt.links.new(val, sock)
	return m.outputs[2]


def bump(nt, height_socket, strength, dist=0.002, into=None):
	b = nt.nodes.new("ShaderNodeBump")
	b.inputs["Strength"].default_value = strength
	b.inputs["Distance"].default_value = dist
	nt.links.new(height_socket, b.inputs["Height"])
	if into is not None:
		nt.links.new(into, b.inputs["Normal"])
	return b.outputs["Normal"]


def skin_material(opt):
	def build(nt):
		zones = nt.nodes.new("ShaderNodeAttribute")
		zones.attribute_name = "zones"
		mottle = noise(nt, 38.0, 6.0, 0.6)
		base = mix_rgb(nt, 0.18, opt["skin"], mottle.outputs["Color"], "OVERLAY")
		col = mix_rgb(nt, 1.0, base, zones.outputs["Color"], "MULTIPLY")
		pores = noise(nt, 900.0, 2.0, 0.5)
		lumps = noise(nt, 70.0, 4.0, 0.6)
		nrm = bump(nt, pores.outputs["Fac"], 0.06, 0.0006)
		nrm = bump(nt, lumps.outputs["Fac"], 0.08, 0.002, nrm)
		b = principled(nt, col, rough=0.48, spec=0.45)
		b.inputs["Subsurface Weight"].default_value = 0.35
		b.inputs["Subsurface Radius"].default_value = (1.0, 0.45, 0.35)
		b.inputs["Subsurface Scale"].default_value = 0.006
		nt.links.new(nrm, b.inputs["Normal"])
		return b
	return material("Skin", ID["skin"], build)


def eye_material(opt):
	"""Burning eyes. "slit": the whole visible eye glows (A, narrowed to slits); "iris": a pale dead sclera round a
	glowing cyan iris with a hot core (B, C)."""
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord")
		sep = nt.nodes.new("ShaderNodeSeparateXYZ")
		nt.links.new(tc.outputs["Object"], sep.inputs[0])
		# The eyeball's +z looks at the camera; its object z runs -r..r.
		zr = nt.nodes.new("ShaderNodeMath"); zr.operation = "DIVIDE"
		nt.links.new(sep.outputs["Z"], zr.inputs[0]); zr.inputs[1].default_value = 0.0118
		ramp = nt.nodes.new("ShaderNodeValToRGB")
		el = ramp.color_ramp.elements
		glow = lin(opt["glow"])
		if opt["eyes"] == "slit":
			el[0].position, el[0].color = 0.55, (*[c * 0.35 for c in glow], 1)
			el[1].position, el[1].color = 0.97, (0.85, 1.0, 1.0, 1)
			e = el.new(0.78); e.color = (*glow, 1)
			strength = 4.0
		else:
			el[0].position, el[0].color = 0.80, (0.035, 0.04, 0.045, 1)        # sclera: dim, it's lit by the scene
			el[1].position, el[1].color = 0.99, (*[min(1.0, c * 1.25) for c in glow], 1)
			e = el.new(0.83); e.color = (*[c * 0.25 for c in glow], 1)        # dark limbal ring
			e = el.new(0.88); e.color = (*[c * 0.8 for c in glow], 1)
			e = el.new(0.955); e.color = (*[c * 0.35 for c in glow], 1)       # the pupil, darker, with a hot core
			strength = 1.9
		nt.links.new(zr.outputs[0], ramp.inputs[0])
		em = nt.nodes.new("ShaderNodeEmission")
		em.inputs["Strength"].default_value = strength
		nt.links.new(ramp.outputs[0], em.inputs["Color"])
		if opt["eyes"] == "slit":
			return em
		sclera = principled(nt, (0.62, 0.64, 0.68), rough=0.15, spec=0.6)
		add = nt.nodes.new("ShaderNodeAddShader")
		nt.links.new(sclera.outputs[0], add.inputs[0])
		nt.links.new(em.outputs[0], add.inputs[1])
		return add
	return material("Eye", ID["eye"], build)


def felt_material():
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord")
		dust = noise(nt, 6.0, 6.0, 0.6, tc.outputs["Object"])
		col = mix_rgb(nt, dust.outputs["Fac"], (0.11, 0.095, 0.085), (0.30, 0.255, 0.21))
		fibre = noise(nt, 400.0, 3.0, 0.7, tc.outputs["Object"])
		nrm = bump(nt, fibre.outputs["Fac"], 0.15, 0.0005)
		b = principled(nt, col, rough=0.92, spec=0.2)
		b.inputs["Sheen Weight"].default_value = 0.6
		b.inputs["Sheen Roughness"].default_value = 0.4
		b.inputs["Sheen Tint"].default_value = (*lin((0.55, 0.48, 0.42)), 1)
		nt.links.new(nrm, b.inputs["Normal"])
		return b
	return material("Felt", ID["felt"], build)


def leather_material(color=(0.22, 0.13, 0.08)):
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord")
		n = noise(nt, 90.0, 6.0, 0.6, tc.outputs["Object"])
		col = mix_rgb(nt, 0.35, color, n.outputs["Color"], "OVERLAY")
		nrm = bump(nt, n.outputs["Fac"], 0.12, 0.0008)
		b = principled(nt, col, rough=0.45, spec=0.5)
		nt.links.new(nrm, b.inputs["Normal"])
		return b
	return material("Leather", ID["leather"], build)


# ---------------------------------------------------------------- the human

def head_frame(h):
	"""Measurements of the morphed head at rest (MakeHuman space), for fitting the hat and the hair."""
	w = h["weights"]("head")
	m = h["morphed"]
	hv = m[w > 0.5]
	eye_l = m[np.unique(np.concatenate(h["group_faces"]("helper-l-eye")))].mean(axis=0)
	eye_r = m[np.unique(np.concatenate(h["group_faces"]("helper-r-eye")))].mean(axis=0)
	eye_y = (eye_l[1] + eye_r[1]) * 0.5
	return {"verts": hv, "top": hv[:, 1].max(), "eye_y": eye_y, "eyes": (eye_l, eye_r),
		"front": hv[:, 2].max(), "back": hv[:, 2].min(), "half_w": np.abs(hv[:, 0]).max()}


def zones(h, verts, eyes, used):
	"""Painted colour zones as vertex colours: dark sunken eyes, a cold flush on nose, ears and cheeks,
	bloodless lips. Multiplied into the skin's colour."""
	n = len(verts)
	c = np.ones((n, 4))
	hw = h["weights"]("head")[used]
	for e in eyes:
		d = np.linalg.norm((verts - e) * np.array([1.0, 1.35, 1.0]), axis=1)
		k = np.clip(1.0 - (d - 0.12) / 0.32, 0.0, 1.0) ** 1.5
		c[:, :3] *= 1.0 - k[:, None] * np.array([0.42, 0.45, 0.32])
	front = verts[hw > 0.5]
	nose = front[np.argmax(front[:, 2])]
	d = np.linalg.norm(verts - nose, axis=1)
	k = np.clip(1.0 - d / 0.35, 0.0, 1.0)
	c[:, :3] *= 1.0 - k[:, None] * np.array([0.05, 0.16, 0.10])
	# lips: just under the nose, at the front
	lip = nose + np.array([0.0, -0.45, -0.12])
	d = np.linalg.norm((verts - lip) * np.array([0.8, 2.2, 1.0]), axis=1)
	k = np.clip(1.0 - d / 0.3, 0.0, 1.0) ** 0.7
	c[:, :3] *= 1.0 - k[:, None] * np.array([0.22, 0.18, 0.08])
	# ears and cheeks
	for sx in (-1.0, 1.0):
		cheek = np.array([0.42 * sx, nose[1] + 0.12, nose[2] - 0.55])
		d = np.linalg.norm(verts - cheek, axis=1)
		k = np.clip(1.0 - d / 0.4, 0.0, 1.0)
		c[:, :3] *= 1.0 - k[:, None] * np.array([0.02, 0.10, 0.06])
	return c


def eyeballs(h):
	"""Each eyeball (centre in the head's rest space, radius), from MakeHuman's eye helper: the helper runs deep
	into the skull, so the ball is sized from its front (the cornea)."""
	out = {}
	for side in ("l", "r"):
		ix = np.unique(np.concatenate(h["group_faces"](f"helper-{side}-eye")))
		pts = h["morphed"][ix]
		c0 = pts.mean(axis=0)
		front = (pts[:, 2] - c0[2]).max()
		r = 0.118
		out[side] = (c0 + np.array([0.0, 0.0, front - r - EYE_BACK]), r)
	return out


def close_lids(h, lids):
	"""Narrow the eyes into a glare: slide the upper lid down and the lower lid up over the eyeball (in the
	head's rest frame, weighted by MakeHuman's lid weights), keeping the skin just outside the ball.
	lids = (upper, lower in decimetres, how much more the upper drops at the inner corner)."""
	balls = eyeballs(h)
	g = h["glob"]["head"]
	gi = np.linalg.inv(g)
	v = h["verts"]
	for side, bone in (("l", "L"), ("r", "R")):
		c, r = balls[side]
		wu = h["weights"](f"orbicularis03.{bone}")
		wl = h["weights"](f"orbicularis04.{bone}")
		ix = np.nonzero((wu > 0.02) | (wl > 0.02))[0]
		loc = mh.apply(gi, v[ix])
		# angry: the upper lid comes down further toward the nose
		inner = 1.0 - np.clip((np.abs(loc[:, 0]) - (abs(c[0]) - 0.13)) / 0.26, 0.0, 1.0)
		loc[:, 1] -= wu[ix] * lids[0] * (1.0 + lids[2] * inner)
		loc[:, 1] += wl[ix] * lids[1]
		d = loc - c
		dist = np.linalg.norm(d, axis=1)
		inside = (dist < r + 0.012) & (d[:, 2] > 0)
		loc[inside] = c + d[inside] / dist[inside, None] * (r + 0.012)
		v[ix] = mh.apply(g, loc)
	return balls


def build_human(opt):
	pose = {
		"upperarm01.L": ((0, 0, 1), -38.0), "upperarm01.R": ((0, 0, 1), 38.0),
		"clavicle.L": ((0, 0, 1), -6.0), "clavicle.R": ((0, 0, 1), 6.0),
		"neck01": ((0, 1, 0), opt["yaw"] * 0.3), "neck02": ((0, 1, 0), opt["yaw"] * 0.3),
		"neck03": ((1, 0, 0), opt["pitch"] * 0.4),
		"head": ((0, 1, 0), opt["yaw"] * 0.4),
	}
	# the head's pitch goes on after its yaw, about its own axis
	h = mh.build(macro={"gender": 0.5, "muscle": 0.6, "weight": 0.42, "caucasian": 0.7, "african": 0.15, "asian": 0.15},
		face=FACE, pose=pose)
	g = h["glob"]["head"]
	hf = head_frame(h)
	balls = close_lids(h, opt["lids"])
	faces = h["group_faces"]("body")
	used = np.unique(np.concatenate(faces))
	remap = -np.ones(len(h["verts"]), dtype=np.int64)
	remap[used] = np.arange(len(used))
	verts = h["verts"][used]
	eyes_posed = [mh.apply(g, e) for e in hf["eyes"]]
	ob = mesh_object("Ellis", mh.to_blender(verts), [[remap[i] for i in f] for f in faces])
	col = zones(h, verts, eyes_posed, used)
	attr = ob.data.color_attributes.new("zones", "FLOAT_COLOR", "POINT")
	attr.data.foreach_set("color", col.astype(np.float32).ravel())
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(skin_material(opt))
	# Eyeballs where MakeHuman's eye helpers are, each turned to look at the camera.
	eye_obs = []
	for side in ("l", "r"):
		c, r = balls[side]
		c = mh.apply(g, c[None])[0]
		me = bpy.data.meshes.new(f"Eye.{side}")
		bm = bmesh.new()
		bmesh.ops.create_uvsphere(bm, u_segments=32, v_segments=16, radius=r * 0.1)
		bm.to_mesh(me)
		bm.free()
		me.shade_smooth()
		eo = link(bpy.data.objects.new(f"Eye.{side}", me))
		eo.location = Vector(mh.to_blender(c))
		eo.data.materials.append(eye_material(opt))
		eye_obs.append(eo)
	return h, hf, ob, eye_obs


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

def body_bvh(ob):
	"""A BVH of the body as posed (no subdivision), in world space."""
	dg = bpy.context.evaluated_depsgraph_get()
	bm = bmesh.new()
	bm.from_mesh(ob.data)
	bm.transform(ob.matrix_world)
	return BVHTree.FromBMesh(bm), bm


def curves_object(name, strands, radius_root, radius_tip, mat):
	"""A hair-curves object from a list of (k, 3) point arrays, radius tapering root to tip."""
	cv = bpy.data.hair_curves.new(name)
	sizes = [len(s) for s in strands]
	cv.add_curves(sizes)
	pts = np.concatenate(strands).astype(np.float32)
	cv.position_data.foreach_set("vector", pts.ravel())
	rad = np.concatenate([np.linspace(radius_root, radius_tip, len(s)) for s in strands]).astype(np.float32)
	ra = cv.attributes.get("radius") or cv.attributes.new("radius", "FLOAT", "POINT")
	ra.data.foreach_set("value", rad)
	cv.materials.append(mat)
	return link(bpy.data.objects.new(name, cv))


def hair_material(melanin=0.96, redness=0.2, rough=0.4):
	def build(nt):
		b = nt.nodes.new("ShaderNodeBsdfHairPrincipled")
		b.parametrization = "MELANIN"
		b.inputs["Melanin"].default_value = melanin
		b.inputs["Melanin Redness"].default_value = redness
		b.inputs["Roughness"].default_value = rough
		b.inputs["Radial Roughness"].default_value = 0.4
		b.inputs["Coat"].default_value = 0.1
		return b
	return material("Hair", ID["hair"], build)


def grow(bvh, root, down, length, steps, rng, out_bias, clear=0.004):
	"""One strand from root: falls along down, draped over the body (slides along it when touching),
	kept just clear of the skin, wandering a little."""
	pts = [root]
	p = Vector(root)
	step = length / steps
	wob = Vector((rng.normal(), rng.normal(), rng.normal())) * 0.12
	for k in range(steps):
		d = (Vector(down) + wob * (k / steps)).normalized()
		hit = bvh.find_nearest(p)
		if hit[0] is not None and (p - hit[0]).length < clear * 3.0:
			n = hit[1]
			if d.dot(n) < 0:
				d = (d - n * d.dot(n)).normalized()   # slide down the surface
			d = (d + n * out_bias * 0.25).normalized()
		p = p + d * step
		hit = bvh.find_nearest(p)
		if hit[0] is not None and (p - hit[0]).dot(hit[1]) < clear:
			p = hit[0] + hit[1] * clear
		pts.append(Vector(p))
	return np.array([tuple(q) for q in pts])


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
		group = strands[i:i + 40]
		m = np.mean([s for s in group], axis=0)
		for s in group:
			t = np.linspace(0, 1, len(s))[:, None] ** 1.5
			out.append(s * (1 - 0.78 * t) + m * 0.78 * t)
	# A lock out from under the brim over the left brow.
	for k in range(60):
		r = np.array([0.38 + rng.normal() * 0.05, band_y - 0.02, hv[:, 2].max() - 0.25 + rng.normal() * 0.03])
		root = mh.to_blender(mh.apply(g, r[None]))[0]
		tip_dir = Vector(mh.to_blender(mh.apply(g, np.array([[0.55, band_y - 0.6, hv[:, 2].max() + 0.1]])))[0]) - Vector(root)
		strand = grow(bvh, Vector(root), tip_dir.normalized(), tip_dir.length * (0.8 + 0.3 * rng.random()), 8, rng, 0.25)
		out.append(strand)
	bm.free()
	return curves_object("Hair", out, 0.0024, 0.0005, hair_material())


def grow_combed(bvh, root, comb, length, steps, rng, clear=0.004):
	"""A strand that starts along comb (lying on the scalp) and turns to fall with gravity as it grows."""
	pts = [root]
	p = Vector(root)
	step = length / steps
	wob = Vector((rng.normal(), rng.normal(), rng.normal())) * 0.10
	for k in range(steps):
		t = k / steps
		d = (Vector(comb) * (1.0 - t) ** 1.5 + Vector((0, 0, -1)) * t + wob * t).normalized()
		hit = bvh.find_nearest(p)
		if hit[0] is not None and (p - hit[0]).length < clear * 3.0:
			n = hit[1]
			if d.dot(n) < 0:
				d = (d - n * d.dot(n)).normalized()
		p = p + d * step
		hit = bvh.find_nearest(p)
		if hit[0] is not None and (p - hit[0]).dot(hit[1]) < clear:
			p = hit[0] + hit[1] * clear
		pts.append(Vector(p))
	return np.array([tuple(q) for q in pts])


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
		m = np.mean(group, axis=0)
		for s_ in group:
			t = np.linspace(0, 1, len(s_))[:, None] ** 1.5
			out.append(s_ * (1 - 0.7 * t) + m * 0.7 * t)
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
		m = np.mean(group, axis=0)
		for s_ in group:
			t = np.linspace(0, 1, len(s_))[:, None] ** 1.5
			out.append(s_ * (1 - 0.65 * t) + m * 0.65 * t)
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



# ---------------------------------------------------------------- cloth by draping

def world_points(ob, levels=1):
	"""World-space vertices of ob evaluated with its modifiers (subdivision at viewport level)."""
	dg = bpy.context.evaluated_depsgraph_get()
	ev = ob.evaluated_get(dg)
	me = ev.to_mesh()
	pts = np.empty(len(me.vertices) * 3, dtype=np.float64)
	me.vertices.foreach_get("co", pts)
	pts = pts.reshape(-1, 3)
	mw = np.array(ob.matrix_world)
	pts = pts @ mw[:3, :3].T + mw[:3, 3]
	ev.to_mesh_clear()
	return pts


def frame2d(fwd):
	f = np.array([fwd[0], fwd[1]], dtype=np.float64)
	f /= np.linalg.norm(f)
	return f, np.array([-f[1], f[0]])   # forward, the character's left


def drape(pts, centre, fwd, thetas, z_top, z_bot, n_z, offset, hb=math.radians(3.0), zb=0.006, smooth=(2, 2), wrap=False):
	"""Cloth hung round a vertical axis: each column (angle theta from the forward direction) runs from
	z_top(theta) down to z_bot(theta); at every height it sits offset outside the widest part of pts at or
	above that height in its column, so it tents over noses and shoulders and hangs straight below them."""
	f, l = frame2d(fwd)
	rel = pts[:, :2] - centre
	ang = np.arctan2(rel @ l, rel @ f)
	rad = np.linalg.norm(rel, axis=1)
	nt = len(thetas)
	R = np.zeros((n_z, nt))
	Z = np.zeros((n_z, nt))
	for i, th in enumerate(thetas):
		zt, zbo = z_top(th), z_bot(th)
		zs = np.linspace(zt, zbo, n_z)
		Z[:, i] = zs
		d = np.abs((ang - th + math.pi) % (2 * math.pi) - math.pi)
		sel = (d < hb) & (pts[:, 2] < zt + zb) & (pts[:, 2] > zbo - zb)
		if not sel.any():
			continue
		zp, rp = pts[sel, 2], rad[sel]
		col = np.zeros(n_z)
		dz = (zt - zbo) / max(n_z - 1, 1)
		idx = np.clip(np.round((zt - zp) / max(dz, 1e-6)).astype(int), 0, n_z - 1)
		np.maximum.at(col, idx, rp)
		R[:, i] = np.maximum.accumulate(col)
	# fill columns that saw nothing from their neighbours, then smooth
	for i in range(nt):
		if R[:, i].max() == 0:
			j = i - 1 if i > 0 else i + 1
			R[:, i] = R[:, j]
	for _ in range(smooth[0]):
		R = 0.25 * np.roll(R, 1, 1) + 0.5 * R + 0.25 * np.roll(R, -1, 1) if wrap else np.concatenate([R[:, :1], 0.25 * R[:, :-2] + 0.5 * R[:, 1:-1] + 0.25 * R[:, 2:], R[:, -1:]], 1)
	for _ in range(smooth[1]):
		R = np.concatenate([R[:1], 0.25 * R[:-2] + 0.5 * R[1:-1] + 0.25 * R[2:], R[-1:]], 0)
	R = R + offset
	cosT, sinT = np.cos(thetas), np.sin(thetas)
	X = centre[0] + R * (cosT * f[0] + sinT * l[0])
	Y = centre[1] + R * (cosT * f[1] + sinT * l[1])
	return X, Y, Z, R


def grid_object(name, X, Y, Z, wrap=False):
	n_z, nt = X.shape
	verts = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
	faces = []
	for j in range(n_z - 1):
		for i in range(nt if wrap else nt - 1):
			a, b = j * nt + i, j * nt + (i + 1) % nt
			faces.append([a, b, b + nt, a + nt])
	return mesh_object(name, verts, faces)


def bandana_material():
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord")
		vor = nt.nodes.new("ShaderNodeTexVoronoi")
		vor.inputs["Scale"].default_value = 95.0
		nt.links.new(tc.outputs["Object"], vor.inputs["Vector"])
		ramp = nt.nodes.new("ShaderNodeValToRGB")
		el = ramp.color_ramp.elements
		el[0].position, el[0].color = 0.0, (1, 1, 1, 1)
		el[1].position, el[1].color = 0.12, (0, 0, 0, 1)
		e = el.new(0.30); e.color = (0, 0, 0, 1)
		e = el.new(0.34); e.color = (1, 1, 1, 1)
		e = el.new(0.39); e.color = (0, 0, 0, 1)
		nt.links.new(vor.outputs["Distance"], ramp.inputs[0])
		fade = noise(nt, 3.0, 4.0, 0.6, tc.outputs["Object"])
		red = mix_rgb(nt, fade.outputs["Fac"], (0.52, 0.06, 0.05), (0.66, 0.17, 0.12))
		pat = nt.nodes.new("ShaderNodeMath"); pat.operation = "MULTIPLY"
		nt.links.new(ramp.outputs[0], pat.inputs[0]); pat.inputs[1].default_value = 0.17
		col = mix_rgb(nt, pat.outputs[0], red, (0.88, 0.80, 0.68))
		dust = noise(nt, 9.0, 5.0, 0.6, tc.outputs["Object"])
		col = mix_rgb(nt, 0.25, col, dust.outputs["Color"], "OVERLAY")
		weave = noise(nt, 1200.0, 2.0, 0.5, tc.outputs["Object"])
		nrm = bump(nt, weave.outputs["Fac"], 0.12, 0.0004)
		b = principled(nt, col, rough=0.85, spec=0.25)
		b.inputs["Sheen Weight"].default_value = 0.35
		b.inputs["Subsurface Weight"].default_value = 0.1
		b.inputs["Subsurface Radius"].default_value = (1.0, 0.2, 0.1)
		b.inputs["Subsurface Scale"].default_value = 0.002
		nt.links.new(nrm, b.inputs["Normal"])
		return b
	return material("Bandana", ID["bandana"], build)


def canvas_material(dark=(0.12, 0.085, 0.06), light=(0.30, 0.235, 0.17), name="Canvas", pid=None, frost=0.0):
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord")
		geo = nt.nodes.new("ShaderNodeNewGeometry")
		sep = nt.nodes.new("ShaderNodeSeparateXYZ")
		nt.links.new(geo.outputs["Normal"], sep.inputs[0])
		blot = noise(nt, 4.0, 6.0, 0.62, tc.outputs["Object"])
		up = nt.nodes.new("ShaderNodeMapRange")
		nt.links.new(sep.outputs["Z"], up.inputs["Value"])
		up.inputs["From Min"].default_value = -0.2
		up.inputs["From Max"].default_value = 1.0
		fac = nt.nodes.new("ShaderNodeMath"); fac.operation = "MULTIPLY_ADD"
		nt.links.new(blot.outputs["Fac"], fac.inputs[0]); fac.inputs[1].default_value = 0.7
		nt.links.new(up.outputs[0], fac.inputs[2])
		col = mix_rgb(nt, fac.outputs[0], dark, light)
		weave = nt.nodes.new("ShaderNodeTexWave")
		weave.inputs["Scale"].default_value = 260.0
		nt.links.new(tc.outputs["Object"], weave.inputs["Vector"])
		creases = noise(nt, 30.0, 6.0, 0.65, tc.outputs["Object"])
		nrm = bump(nt, weave.outputs["Fac"], 0.06, 0.0003)
		nrm = bump(nt, creases.outputs["Fac"], 0.25, 0.004, nrm)
		if frost > 0:
			# rime on everything facing the sky, broken up
			fm = nt.nodes.new("ShaderNodeMapRange")
			nt.links.new(sep.outputs["Z"], fm.inputs["Value"])
			fm.inputs["From Min"].default_value = 0.35
			fm.inputs["From Max"].default_value = 0.85
			fn = noise(nt, 60.0, 8.0, 0.7, tc.outputs["Object"])
			fx = nt.nodes.new("ShaderNodeMath"); fx.operation = "MULTIPLY"
			nt.links.new(fm.outputs[0], fx.inputs[0]); nt.links.new(fn.outputs["Fac"], fx.inputs[1])
			fy = nt.nodes.new("ShaderNodeMapRange")
			nt.links.new(fx.outputs[0], fy.inputs["Value"])
			fy.inputs["From Min"].default_value = 0.22
			fy.inputs["From Max"].default_value = 0.42
			fy.inputs["To Max"].default_value = frost
			col = mix_rgb(nt, fy.outputs[0], col, (0.80, 0.88, 0.95))
		b = principled(nt, col, rough=0.72, spec=0.35)
		b.inputs["Sheen Weight"].default_value = 0.3
		nt.links.new(nrm, b.inputs["Normal"])
		return b
	return material(name, ID["coat"] if pid is None else pid, build)


def head_axis(h, hf):
	"""The head's vertical axis (x, y), its forward direction and the eye height, posed, in metres."""
	g = h["glob"]["head"]
	eyes = [mh.to_blender(mh.apply(g, e[None]))[0] for e in hf["eyes"]]
	mid = (eyes[0] + eyes[1]) * 0.5
	f = mh.to_blender(mh.apply(g, (hf["eyes"][0] + hf["eyes"][1])[None] * 0.5 + np.array([[0, 0, 1.0]])))[0] - mid
	f[2] = 0
	f /= np.linalg.norm(f)
	centre = mid[:2] - f[:2] * 0.085
	return centre, f, mid[2]


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


def neck_frame(h):
	g = h["glob"]["neck01"]
	b = h["bones"]["neck01"]
	base = mh.to_blender(mh.apply(g, b["head"][None]))[0]
	return base


def shoulder_z(h):
	return mh.to_blender(mh.apply(h["glob"]["shoulder01.L"], h["bones"]["shoulder01.L"]["head"][None]))[0][2]



def bvh_of(*obs):
	"""One BVH over the evaluated (modified) meshes of obs, in world space."""
	dg = bpy.context.evaluated_depsgraph_get()
	bm = bmesh.new()
	for ob in obs:
		ev = ob.evaluated_get(dg)
		me = ev.to_mesh()
		tmp = bmesh.new()
		tmp.from_mesh(me)
		tmp.transform(ob.matrix_world)
		me2 = bpy.data.meshes.new("tmp")
		tmp.to_mesh(me2)
		tmp.free()
		bm.from_mesh(me2)
		bpy.data.meshes.remove(me2)
		ev.to_mesh_clear()
	tree = BVHTree.FromBMesh(bm)
	bm.free()
	return tree


def brass_material():
	def build(nt):
		tc = nt.nodes.new("ShaderNodeTexCoord")
		n = noise(nt, 120.0, 6.0, 0.6, tc.outputs["Object"])
		col = mix_rgb(nt, n.outputs["Fac"], (0.62, 0.45, 0.20), (0.86, 0.68, 0.36))
		rough = nt.nodes.new("ShaderNodeMapRange")
		nt.links.new(n.outputs["Fac"], rough.inputs["Value"])
		rough.inputs["To Min"].default_value = 0.22
		rough.inputs["To Max"].default_value = 0.5
		b = principled(nt, col, rough=0.3, spec=0.5, metal=1.0)
		nt.links.new(rough.outputs[0], b.inputs["Roughness"])
		return b
	return material("Brass", ID["brass"], build)


def binding(name, line, width, mat=None):
	"""A narrow strip (a hem binding) along a polyline, hanging down by width."""
	n = len(line)
	verts = np.concatenate([line + np.array([0, 0, width * 0.5]), line - np.array([0, 0, width * 0.5])])
	faces = [[i, i + 1, n + i + 1, n + i] for i in range(n - 1)]
	ob = mesh_object(name, verts, faces)
	add_mod(ob, "SOLIDIFY", thickness=0.006, offset=0.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(mat or leather_material((0.12, 0.07, 0.045)))
	return ob


def sun_pin(at, normal):
	"""Saint Ada's sun-ring: a brass ring with twelve short rays round a small ember."""
	bm = bmesh.new()
	ring = bmesh.ops.create_circle(bm, cap_ends=False, segments=32, radius=0.011)
	bmesh.ops.delete(bm, geom=bm.verts[:], context="VERTS")
	bm.free()
	parts = []
	me = bpy.data.meshes.new("SunPin")
	bm = bmesh.new()
	# the ring: a torus
	torus = bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=32, radius1=0.012, radius2=0.012, depth=0.003)
	for k in range(12):
		a = 2 * math.pi * k / 12
		ray = bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=True, segments=4, radius1=0.0025, radius2=0.0, depth=0.008,
			matrix=Matrix.Translation((math.cos(a) * 0.0155, math.sin(a) * 0.0155, 0)) @ Matrix.Rotation(math.pi / 2, 4, "Y") @ Matrix.Rotation(-a, 4, "X"))
	bm.to_mesh(me)
	bm.free()
	ob = link(bpy.data.objects.new("SunPin", me))
	ob.location = at
	ob.rotation_euler = normal.to_track_quat("Z", "Y").to_euler()
	ob.data.materials.append(brass_material())
	return ob



def chin_z(h):
	used = np.unique(np.concatenate(h["group_faces"]("body")))
	hw = h["weights"]("head")
	v = h["verts"][(hw > 0.5)]
	w = mh.to_blender(v)
	front = w[w[:, 1] < np.percentile(w[:, 1], 25)]
	return front[:, 2].min()


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


def surface_point(bvh, x, z, y0=-0.6):
	"""Where a ray from the front (along +y) at (x, z) meets a surface: (point, normal) or None."""
	hit = bvh.ray_cast(Vector((x, y0, z)), Vector((0, 1, 0)), 1.5)
	return None if hit[0] is None else (hit[0], hit[1])


def strip_on(bvh, rows, name, lift, mat, thick=0.004):
	"""A grid of (x, z) rows projected onto a surface from the front and lifted off it along the normal."""
	verts, faces = [], []
	nr, nc = len(rows), len(rows[0])
	for j, row in enumerate(rows):
		for i, (x, z, extra) in enumerate(row):
			sp = surface_point(bvh, x, z)
			if sp is None:
				sp = (Vector((x, 0.0, z)), Vector((0, -1, 0)))
			verts.append(tuple(sp[0] + sp[1] * (lift + extra)))
	for j in range(nr - 1):
		for i in range(nc - 1):
			a = j * nc + i
			faces.append([a, a + 1, a + nc + 1, a + nc])
	ob = mesh_object(name, np.array(verts), faces)
	add_mod(ob, "SOLIDIFY", thickness=thick, offset=1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(mat)
	return ob


def build_lapels(coat_ob, neck, vee, mat, under_ob=None):
	"""Lapels folded back either side of the coat's opening, notched below the collar."""
	bvh = bvh_of(coat_ob, under_ob) if under_ob else bvh_of(coat_ob)
	parts = []
	for side in (-1.0, 1.0):
		rows = []
		for j in range(14):
			t = j / 13
			z = neck[2] - 0.005 - t * 0.20
			halfw = vee + (neck[2] - z) * 0.16
			w = 0.075 * (1 - t) ** 0.9 * (0.82 if t < 0.12 else 1.0)
			row = []
			for i in range(5):
				u = i / 4
				x = neck[0] + side * (halfw - 0.006 + w * u)
				row.append((x, z, 0.004 * (1 - u)))   # the edge by the opening rolls up a little
			rows.append(row)
		parts.append(strip_on(bvh, rows, "Lapel", 0.003, mat))
	return parts


def build_shirt_collar(h, neck, mat, height=0.042):
	"""A starched stand collar round the neck, open a finger's width at the front."""
	bf, bl = frame2d(np.array([0.0, -1.0]))
	nw = h["weights"]("neck01") + h["weights"]("neck02")
	nv = mh.to_blender(h["verts"][nw > 0.6])
	near = nv[np.abs(nv[:, 2] - (neck[2] + 0.02)) < 0.015]
	rel = near[:, :2] - neck[:2]
	nr = np.percentile(np.linalg.norm(rel, axis=1), 90) + 0.007
	thetas = np.linspace(math.radians(12), math.radians(348), 100)
	X = np.zeros((6, len(thetas))); Y = np.zeros_like(X); Z = np.zeros_like(X)
	for j in range(6):
		t = j / 5
		for i, th in enumerate(thetas):
			d = math.cos(th) * bf + math.sin(th) * bl
			r = nr + 0.003 * t
			X[j, i] = neck[0] + d[0] * r
			Y[j, i] = neck[1] + d[1] * r
			Z[j, i] = neck[2] - 0.004 + height * t * (0.85 + 0.15 * (0.5 - 0.5 * math.cos(th)))
	ob = grid_object("ShirtCollar", X, Y, Z)
	add_mod(ob, "SOLIDIFY", thickness=0.0025, offset=1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(mat)
	return ob, nr


def build_string_tie(shirt_ob, neck, nr, mat):
	"""A black string tie: a small bow at the collar and two tails down the shirt front."""
	bvh = bvh_of(shirt_ob)
	parts = []
	front = Vector((neck[0], neck[1] - nr - 0.006, neck[2] + 0.006))
	for side in (-1.0, 1.0):
		bm = bmesh.new()
		bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=8, radius=0.011)
		me = bpy.data.meshes.new("TieLoop")
		bm.to_mesh(me)
		bm.free()
		me.shade_smooth()
		lo = link(bpy.data.objects.new("TieLoop", me))
		lo.location = front + Vector((side * 0.013, -0.002, 0.0))
		lo.scale = (1.2, 0.35, 0.55)
		lo.rotation_euler = (0.0, side * 0.35, 0.0)
		lo.data.materials.append(mat)
		parts.append(lo)
		rows = []
		for j in range(12):
			t = j / 11
			z = front.z - 0.006 - t * 0.12
			x = neck[0] + side * (0.004 + 0.012 * t) + 0.004 * math.sin(t * 5 + side)
			rows.append([(x - 0.0025, z, 0.0), (x + 0.0025, z, 0.0)])
		rows = [list(r) for r in zip(*rows)]
		rows = [[rows[0][j], rows[1][j]] for j in range(12)]
		parts.append(strip_on(bvh, rows, "TieTail", 0.0025, mat, 0.0015))
	bm = bmesh.new()
	bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=0.0065)
	me = bpy.data.meshes.new("TieKnot")
	bm.to_mesh(me)
	bm.free()
	me.shade_smooth()
	ko = link(bpy.data.objects.new("TieKnot", me))
	ko.location = front + Vector((0, -0.003, 0))
	ko.data.materials.append(mat)
	parts.append(ko)
	return parts


def build_flower(at, normal):
	"""A dried flower in the buttonhole, gone black: a rosette of curled petals on a short stem."""
	rng = np.random.default_rng(2)
	mat = material("DeadFlower", ID["linen"], lambda nt: principled(nt, (0.10, 0.07, 0.09), rough=0.6, spec=0.3))
	parts = []
	q = normal.to_track_quat("Z", "Y")
	for k in range(9):
		a = 2 * math.pi * k / 9 + rng.random() * 0.3
		ring = 0.006 if k < 6 else 0.002
		bm = bmesh.new()
		bmesh.ops.create_uvsphere(bm, u_segments=10, v_segments=6, radius=0.007)
		me = bpy.data.meshes.new("Petal")
		bm.to_mesh(me)
		bm.free()
		me.shade_smooth()
		po = link(bpy.data.objects.new("Petal", me))
		off = Vector((math.cos(a) * ring, math.sin(a) * ring, 0.002 * k / 9))
		po.location = at + q @ off
		po.scale = (1.0, 0.55, 0.25)
		po.rotation_euler = (q @ Matrix.Rotation(a, 3, "Z").to_quaternion()).to_euler()
		po.data.materials.append(mat)
		parts.append(po)
	return parts


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


def shell(body, h, name, offset_top, offset_loose, top_z, keep_neck=0.25):
	"""The torso and arms pushed out along their normals: a garment's base, cut off below top_z."""
	neck_w = h["weights"]("neck01") + h["weights"]("neck02") + h["weights"]("neck03") + h["weights"]("head")
	used = np.unique(np.concatenate(h["group_faces"]("body")))
	neck_w = neck_w[used]
	sz = shoulder_z(h)
	ob = link(bpy.data.objects.new(name, body.data.copy()))
	bm = bmesh.new()
	bm.from_mesh(ob.data)
	bm.verts.ensure_lookup_table()
	bmesh.ops.delete(bm, geom=[v for v in bm.verts if neck_w[v.index] > keep_neck or v.co.z > top_z], context="VERTS")
	bm.normal_update()
	for v in bm.verts:
		loose = np.clip((sz - v.co.z) / 0.5, 0, 1)
		v.co += v.normal * (offset_top + offset_loose * loose)
	bm.to_mesh(ob.data)
	bm.free()
	ob.modifiers.clear()
	ob.data.materials.clear()
	return ob, bm


def cut_vee(ob, neck, vee):
	"""Open a garment at the front in a V from the collar down."""
	bf, bl = frame2d(np.array([0.0, -1.0]))
	bm = bmesh.new()
	bm.from_mesh(ob.data)
	cut = []
	for fc in bm.faces:
		c = fc.calc_center_median()
		rel = np.array([c.x - neck[0], c.y - neck[1]])
		halfw = vee + max(neck[2] - c.z, 0) * 0.16
		if rel @ bf > 0.02 and abs(rel @ bl) < halfw:
			cut.append(fc)
	bmesh.ops.delete(bm, geom=cut, context="FACES")
	bm.to_mesh(ob.data)
	bm.free()


def buttons(target_ob, neck, count, step, mat, radius=0.0055, x=0.006, z0=0.07):
	bvh = bvh_of(target_ob)
	out = []
	for k in range(count):
		sp = surface_point(bvh, neck[0] + x, neck[2] - z0 - k * step)
		if sp is None:
			continue
		bmb = bmesh.new()
		bmesh.ops.create_uvsphere(bmb, u_segments=16, v_segments=8, radius=radius)
		me = bpy.data.meshes.new("Button")
		bmb.to_mesh(me)
		bmb.free()
		me.shade_smooth()
		bo = link(bpy.data.objects.new("Button", me))
		bo.location = sp[0] + sp[1] * 0.003
		bo.scale = (1, 1, 0.6)
		bo.rotation_euler = sp[1].to_track_quat("Z", "Y").to_euler()
		bo.data.materials.append(mat)
		out.append(bo)
	return out


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


# ---------------------------------------------------------------- stage, lights, camera, render

def stage(h, opt, size, samples):
	sc = bpy.context.scene
	sc.render.engine = "CYCLES"
	sc.cycles.device = "CPU"
	sc.cycles.samples = samples
	sc.cycles.use_denoising = True
	sc.render.resolution_x = sc.render.resolution_y = size
	sc.render.film_transparent = True
	sc.render.image_settings.file_format = "PNG"
	sc.render.image_settings.color_mode = "RGBA"
	try:
		sc.view_settings.view_transform = "AgX"
		sc.view_settings.look = "AgX - Medium High Contrast"
	except Exception:
		pass
	world = bpy.data.worlds.new("World")
	sc.world = world
	world.use_nodes = True
	world.node_tree.nodes["Background"].inputs["Color"].default_value = (*lin(opt["world"]), 1)
	world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.22

	head = Vector(mh.to_blender(mh.apply(h["glob"]["head"], h["bones"]["head"]["head"])))
	target = head + Vector((0.0, 0.0, opt["cam"][1]))
	cam_data = bpy.data.cameras.new("Cam")
	cam_data.lens = 85.0
	cam_data.sensor_fit = "VERTICAL"
	cam_data.sensor_height = 24.0
	cam = link(bpy.data.objects.new("Cam", cam_data))
	cam.location = target + Vector((0.0, -opt["cam"][0], 0.02))
	cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
	sc.camera = cam

	def area(name, loc, power, color, size_m):
		ld = bpy.data.lights.new(name, "AREA")
		ld.energy = power
		ld.color = lin(color)
		ld.size = size_m
		lo = link(bpy.data.objects.new(name, ld))
		lo.location = target + Vector(loc)
		lo.rotation_euler = (target - lo.location).to_track_quat("-Z", "Y").to_euler()
		return lo
	for name, loc, power, color, size_m in opt["lights"]:
		area(name, loc, power, color, size_m)
	return cam, target


def point_eyes_at(eyes, cam):
	for e in eyes:
		e.rotation_euler = (cam.location - e.location).to_track_quat("Z", "Y").to_euler()


def render(path):
	bpy.context.scene.render.filepath = path
	bpy.ops.render.render(write_still=True)


def render_aux(path, samples=4):
	"""The aux pass for paint.js: every material swapped for an emission of (glow, 0, id / 16)."""
	sc = bpy.context.scene
	keep = (sc.cycles.samples, sc.cycles.use_denoising, sc.view_settings.view_transform, sc.view_settings.look)
	sc.cycles.samples = samples
	sc.cycles.use_denoising = False
	sc.view_settings.view_transform = "Standard"
	sc.view_settings.look = "None"
	swaps = []
	for ob in sc.objects:
		if ob.type not in ("MESH", "CURVES", "CURVE"):
			continue
		for slot in ob.material_slots:
			m = slot.material
			if m is None:
				continue
			pid = m.get("looter_id", 0)
			flat = bpy.data.materials.new(m.name + ".aux")
			flat.use_nodes = True
			nt = flat.node_tree
			for n in list(nt.nodes):
				nt.nodes.remove(n)
			em = nt.nodes.new("ShaderNodeEmission")
			glow = 1.0 if pid == ID["eye"] else 0.0
			em.inputs["Color"].default_value = (glow, 0.0, pid / 16.0, 1.0)
			out = nt.nodes.new("ShaderNodeOutputMaterial")
			nt.links.new(em.outputs[0], out.inputs["Surface"])
			swaps.append((slot, m))
			slot.material = flat
	lights = [o for o in sc.objects if o.type == "LIGHT"]
	for lo in lights:
		lo.hide_render = True
	strength = sc.world.node_tree.nodes["Background"].inputs["Strength"].default_value
	sc.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.0
	render(path)
	for slot, m in swaps:
		slot.material = m
	for lo in lights:
		lo.hide_render = False
	sc.world.node_tree.nodes["Background"].inputs["Strength"].default_value = strength
	sc.cycles.samples, sc.cycles.use_denoising, sc.view_settings.view_transform, sc.view_settings.look = keep


def main():
	args = sys.argv[1:]
	key = args[0] if args and not args[0].startswith("-") else "A"
	opt = dict(OPTIONS[key])
	get = lambda name, d: args[args.index(name) + 1] if name in args else d
	size = int(get("--size", 1024))
	samples = int(get("--samples", 96))
	out = os.path.abspath(get("--out", os.path.join(HERE, "..", "..", "Saved", "ArtPreviews", "Portraits")))
	os.makedirs(out, exist_ok=True)
	t0 = time.time()
	reset()
	h, hf, body, eyes = build_human(opt)
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
	cam, _ = stage(h, opt, size, samples)
	point_eyes_at(eyes, cam)
	print(f"built in {time.time() - t0:.1f} s")
	t1 = time.time()
	render(os.path.join(out, f"Ellis{key}_color.png"))
	print(f"rendered in {time.time() - t1:.1f} s")
	if "--no-aux" not in args:
		render_aux(os.path.join(out, f"Ellis{key}_aux.png"))
	if "--blend" in args:
		bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, f"Ellis{key}.blend"))


if __name__ == "__main__":
	main()
