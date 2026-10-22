"""Painted-portrait busts in Blender (Cycles) from MakeHuman's CC0 base mesh: the machinery every hero's portrait
script shares (Ellis's: EllisBlender.py). Docs/Pipeline/Heroes.md, "Character portraits". bustkit.py is the second shared module:
the building blocks (shader shorthands, primitives, wires, chains, hair tufts, posing hooks) and the garment parts and
props (lapels, collar, tie, buttons, the sun pin) heroes dress their busts with.

A hero script (Art/Portraits/<Hero>.py; Ellis's EllisBlender.py) holds its FACE (mh face modifiers), MACRO (mh macro
sliders), OPTIONS (key -> option dict) and the "# PAINT <key> {...}" lines render.js reads; it builds the costume with the
helpers here and in bustkit and ends with
	def build(opt):
		h, hf, body, eyes = build_human(opt, FACE, MACRO, "<Hero>")
		...                                  # hat, hair (curves_object, grow, clump), clothes (shell, drape, strip_on)
		return h, eyes
	if __name__ == "__main__":
		run(OPTIONS, "<Hero>", build)
What the machinery reads from an option dict: yaw, pitch (the head's turn, degrees), lids (close_lids), skin (sRGB),
eyes ("slit": all glow, or "iris") and glow (sRGB), world (sRGB), cam (distance, height above the head bone, metres),
lights ([(name, offset from the camera's target, watts, sRGB, size)]). Everything else is the hero script's own.

Run it with the Blender binary (its arguments after "--") or Blender's Python module bpy (after the script):
	"C:/Program Files/Blender Foundation/Blender 4.4/blender.exe" -b --factory-startup --python-exit-code 1
		--python Art/Portraits/<Hero>Blender.py -- <key> [--size 1024] [--samples 96] [--out <dir>] [--no-aux] [--blend]
	python Art/Portraits/<Hero>Blender.py <key> [...]
Writes <out>/<Hero><key>_color.png (RGBA) and _aux.png (R glow mask, B material id / 16), which
`node Art/Portraits/render.js <Hero><key> --script <Hero>Blender.py --passes <out>` paints and frames.
<out> defaults to Saved/ArtPreviews/Portraits.
"""
import math
import os
import sys
import time

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mh  # noqa: E402

# Material ids shared with paint.js (and the march in lib.glsl).
ID = {"skin": 1, "eye": 2, "felt": 3, "leather": 4, "bandana": 5, "coat": 6, "shirt": 7, "brass": 8, "hair": 9,
	"feather": 10, "lead": 11, "linen": 12, "black": 13, "glass": 14, "wood": 15}

EYE_BACK = 0.022   # decimetres the eyeball's front sits behind the eye helper's cornea


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
			el[1].position, el[1].color = 0.97, (*[min(1.0, c * 1.3) for c in glow], 1)
			e = el.new(0.78); e.color = (*glow, 1)
			strength = 3.0
		else:
			el[0].position, el[0].color = 0.80, (0.035, 0.04, 0.045, 1)        # sclera: dim, it's lit by the scene
			el[1].position, el[1].color = 0.99, (*[min(1.0, c * 1.25) for c in glow], 1)
			e = el.new(0.83); e.color = (*[c * 0.25 for c in glow], 1)        # dark limbal ring
			e = el.new(0.88); e.color = (*[c * 0.8 for c in glow], 1)
			e = el.new(0.955); e.color = (*[c * 0.35 for c in glow], 1)       # the pupil, darker, with a hot core
			strength = 1.5
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


def build_human(opt, face, macro, name="Body"):
	"""The body posed for a bust (arms down, the head turned by opt["yaw"] and opt["pitch"], the lids narrowed by
	opt["lids"]) with its skin, and the eyeballs. face, macro: the hero's mh.build modifiers and sliders.
	Returns (h, hf, body, eyes): mh.build's human, head_frame(h), the body object, the two eyeball objects."""
	pose = {
		"upperarm01.L": ((0, 0, 1), -38.0), "upperarm01.R": ((0, 0, 1), 38.0),
		"clavicle.L": ((0, 0, 1), -6.0), "clavicle.R": ((0, 0, 1), 6.0),
		"neck01": ((0, 1, 0), opt["yaw"] * 0.3), "neck02": ((0, 1, 0), opt["yaw"] * 0.3),
		"neck03": ((1, 0, 0), opt["pitch"] * 0.4),
		"head": ((0, 1, 0), opt["yaw"] * 0.4),
	}
	# the head's pitch goes on after its yaw, about its own axis
	h = mh.build(macro=macro, face=face, pose=pose)
	g = h["glob"]["head"]
	hf = head_frame(h)
	balls = close_lids(h, opt["lids"])
	faces = h["group_faces"]("body")
	used = np.unique(np.concatenate(faces))
	remap = -np.ones(len(h["verts"]), dtype=np.int64)
	remap[used] = np.arange(len(used))
	verts = h["verts"][used]
	eyes_posed = [mh.apply(g, e) for e in hf["eyes"]]
	ob = mesh_object(name, mh.to_blender(verts), [[remap[i] for i in f] for f in faces])
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


# ---------------------------------------------------------------- hair and brows (Blender hair curves)

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


def clump(group, pull):
	"""One clump's strands drawn together toward their tips (pull: how far, at the tip, toward the clump's mean
	strand), which reads painterly. Returns the new strands."""
	m = np.mean(group, axis=0)
	out = []
	for s in group:
		t = np.linspace(0, 1, len(s))[:, None] ** 1.5
		out.append(s * (1 - pull * t) + m * pull * t)
	return out


# ---------------------------------------------------------------- cloth by draping, strips and shells

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


def neck_frame(h):
	g = h["glob"]["neck01"]
	b = h["bones"]["neck01"]
	base = mh.to_blender(mh.apply(g, b["head"][None]))[0]
	return base


def shoulder_z(h):
	return mh.to_blender(mh.apply(h["glob"]["shoulder01.L"], h["bones"]["shoulder01.L"]["head"][None]))[0][2]


def chin_z(h):
	used = np.unique(np.concatenate(h["group_faces"]("body")))
	hw = h["weights"]("head")
	v = h["verts"][(hw > 0.5)]
	w = mh.to_blender(v)
	front = w[w[:, 1] < np.percentile(w[:, 1], 25)]
	return front[:, 2].min()


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


# ---------------------------------------------------------------- stage, lights, camera, render

def stage(h, opt, size, samples):
	sc = bpy.context.scene
	sc.render.engine = "CYCLES"
	sc.cycles.device = "CPU"
	sc.cycles.samples = samples
	sc.cycles.use_denoising = True
	sc.cycles.seed = 0   # Blender's default, written down: the sampling pattern stays the same from render to render
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
	for name, loc, power, color, size_m in opt["lights"]:
		area_light(name, target, loc, power, color, size_m)
	return cam, target


def area_light(name, target, loc, power, color, size_m):
	"""An area light at loc from target (a world point), aimed at it."""
	ld = bpy.data.lights.new(name, "AREA")
	ld.energy = power
	ld.color = lin(color)
	ld.size = size_m
	lo = link(bpy.data.objects.new(name, ld))
	lo.location = target + Vector(loc)
	lo.rotation_euler = (target - lo.location).to_track_quat("-Z", "Y").to_euler()
	return lo


def point_eyes_at(eyes, cam):
	for e in eyes:
		e.rotation_euler = (cam.location - e.location).to_track_quat("Z", "Y").to_euler()


def render(path):
	bpy.context.scene.render.filepath = path
	bpy.ops.render.render(write_still=True)


def render_aux(path, samples=4):
	"""The aux pass for paint.js: every material swapped for an emission of (glow, 0, id / 16). The glow mask is the
	material's "looter_glow" (eyes 1 by default: lamp, ember and fire bloom); objects whose material has "looter_aux_skip"
	(steam: a volume that the pass would see as a solid cone) leave the pass."""
	sc = bpy.context.scene
	keep = (sc.cycles.samples, sc.cycles.use_denoising, sc.view_settings.view_transform, sc.view_settings.look)
	sc.cycles.samples = samples
	sc.cycles.use_denoising = False
	# Raw, not Standard: Standard applies the sRGB curve on the way to the PNG, which turned id / 16 into another id
	# (skin 1 read back as 4, coat 6 as 10), so paint.js tightened the wrong materials. Raw writes the values as they are.
	sc.view_settings.view_transform = "Raw"
	sc.view_settings.look = "None"
	swaps, hidden = [], [o for o in sc.objects if o.type == "LIGHT"]
	for ob in sc.objects:
		if ob.type not in ("MESH", "CURVES", "CURVE"):
			continue
		for slot in ob.material_slots:
			m = slot.material
			if m is None:
				continue
			if m.get("looter_aux_skip"):
				hidden.append(ob)
				continue
			pid = m.get("looter_id", 0)
			flat = bpy.data.materials.new(m.name + ".aux")
			flat.use_nodes = True
			nt = flat.node_tree
			for n in list(nt.nodes):
				nt.nodes.remove(n)
			em = nt.nodes.new("ShaderNodeEmission")
			glow = m.get("looter_glow", 1.0 if pid == ID["eye"] else 0.0)
			em.inputs["Color"].default_value = (glow, 0.0, pid / 16.0, 1.0)
			out = nt.nodes.new("ShaderNodeOutputMaterial")
			nt.links.new(em.outputs[0], out.inputs["Surface"])
			swaps.append((slot, m))
			slot.material = flat
	for lo in hidden:
		lo.hide_render = True
	strength = sc.world.node_tree.nodes["Background"].inputs["Strength"].default_value
	sc.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.0
	render(path)
	for slot, m in swaps:
		slot.material = m
	for lo in hidden:
		lo.hide_render = False
	sc.world.node_tree.nodes["Background"].inputs["Strength"].default_value = strength
	sc.cycles.samples, sc.cycles.use_denoising, sc.view_settings.view_transform, sc.view_settings.look = keep


def cli_args(argv=None):
	"""The script's own arguments: those after "--" under the Blender binary (blender -b --python <script> -- <args>,
	where argv holds Blender's own too), else everything after the script (python <script> <args>, the bpy module)."""
	argv = sys.argv if argv is None else argv
	return argv[argv.index("--") + 1:] if "--" in argv else argv[1:]


def run(options, prefix, build):
	"""A hero script's main: <key> [--size N] [--samples N] [--out dir] [--no-aux] [--aux-only] [--blend]. Builds
	options[key] (the first option without a key) on an empty scene with build(opt) -> (h, eyes), stages it, turns the
	eyes to the camera and renders <out>/<prefix><key>_color.png and _aux.png (and saves <prefix><key>.blend with
	--blend). --aux-only redoes just the aux pass (seconds, not minutes) when only paint.js's inputs changed."""
	args = cli_args()
	key = args[0] if args and not args[0].startswith("-") else next(iter(options))
	opt = dict(options[key])
	get = lambda name, d: args[args.index(name) + 1] if name in args else d
	size = int(get("--size", 1024))
	samples = int(get("--samples", 96))
	out = os.path.abspath(get("--out", os.path.join(HERE, "..", "..", "Saved", "ArtPreviews", "Portraits")))
	os.makedirs(out, exist_ok=True)
	t0 = time.time()
	reset()
	h, eyes = build(opt)
	cam, _ = stage(h, opt, size, samples)
	point_eyes_at(eyes, cam)
	print(f"built in {time.time() - t0:.1f} s")
	t1 = time.time()
	if "--aux-only" not in args:
		render(os.path.join(out, f"{prefix}{key}_color.png"))
		print(f"rendered in {time.time() - t1:.1f} s")
	if "--no-aux" not in args:
		render_aux(os.path.join(out, f"{prefix}{key}_aux.png"))
	if "--blend" in args:
		bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, f"{prefix}{key}.blend"))
