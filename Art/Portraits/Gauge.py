"""Gauge, the Iron Hand: the railroad's clockwork track-laying engine as a painted-portrait bust in Blender (Cycles),
on the machinery every hero's bust shares (bust.py). Spec: Saved/Portraits/CONTRACT.md.

No MakeHuman body: Gauge is built from primitives (bevelled iron boxes, a lofted boiler, domes, brass bands, rivet
rows, pipes). bust.stage frames a MakeHuman head bone, so build() hands it a stand-in whose "head bone" is Gauge's
neck pivot, and no eyes to aim (the headlamp is aimed by turning the head). Run as Ellis's (more in bust.py):
    "C:/Program Files/Blender Foundation/Blender 4.4/blender.exe" -b --factory-startup --python-exit-code 1
        --python Art/Portraits/Gauge.py -- <A|B|C> [--size 1024] [--samples 96] [--out <dir>] [--no-aux] [--blend]
  A  The Track-Layer: working iron, the headlamp lit cyan under a scowling brow, stacks wisping, the needle in the red.
  B  Woken in the Wreck: rust, a pauldron torn to the works, a bent stack, the firebox sprung on the cyan ember, the
     lamp cracked but lit, the company's letters half gone.
  C  Company Iron: black enamel, polished brass, the crest, GAUGE on a level brow, a warm white lamp (before the ember).
Helpers the shared code lacks (Main may merge them into bust.py): GLASS (material id 14), render_aux_glow (the aux
pass reads a material's "looter_glow" mask and leaves "looter_aux_skip" volumes out), the primitive builders.
"""
import math
import os
import sys

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Euler, Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import bust  # noqa: E402
from bust import ID, add_mod, bump, lin, link, material, mix_rgb, noise, principled  # noqa: E402

GLASS = ID.get("glass", 14)   # bust.ID stops at black 13; the contract gives glass 14 (lamp, gauge face)

# The paint pass's settings per option (render.js reads these lines).
# PAINT A {"bgTop":"#140d0a","bgBottom":"#060403","bgGlow":"#5a2c12","glowAt":[0.70,0.30],"sparks":"#ff8a1c","dust":"#6b5a50","glow":"#5ad8ff"}
# PAINT B {"bgTop":"#170c08","bgBottom":"#060302","bgGlow":"#6a2a0e","glowAt":[0.32,0.62],"sparks":"#ff8a1c","dust":"#6b5a50","glow":"#5ad8ff"}
# PAINT C {"bgTop":"#120e0b","bgBottom":"#050403","bgGlow":"#4a3018","glowAt":[0.66,0.28],"sparks":"#ffc46a","dust":"#8a7a68","glow":"#ffe2a8"}

CYAN = (0.25, 0.85, 1.0)
WARM = (1.0, 0.89, 0.66)    # Gauge C's lamp, #ffe2a8
ANCHOR = Vector((0.0, 0.0, 1.62))   # the neck pivot: Gauge stands 1.92 m to the stack tops

# body/yaw/pitch/lean: the body's turn, the head's turn and tip, the body leant in over the camera (degrees); scale fits
# Gauge into Ellis's frame; scowl: the brow's V; hand: (wrist, fingers' aim, back of the hand's facing, curl).
# lids, skin and eyes are MakeHuman's and unused here.
OPTIONS = {
	"A": {"label": "The Track-Layer", "line": "Working iron, dulled and oiled: the headlamp burns Revenant cyan, the stacks wisp and the needle sits in the red.",
		"yaw": -30.0, "body": -32.0, "pitch": 11.0, "lean": 9.0, "scale": 0.7, "scowl": 11.0, "lids": None, "skin": None, "eyes": None,
		"glow": CYAN, "finish": "iron", "rust": 0.0, "soot": 0.6, "lamp": 0.9, "needle": 100.0, "letters": "T.R.R.", "steam": 0.7,
		"fire": 18.0, "hand": ((0.33, -0.30, -0.29), (-0.35, -0.3, 1.0), (0.5, -0.85, 0.2), 85.0),
		"world": (0.12, 0.09, 0.075), "cam": (2.10, 0.0),
		"lights": [("Key", (-0.80, -0.50, 0.35), 45.0, (1.0, 0.72, 0.48), 0.7), ("Rim", (1.4, 0.8, 0.9), 240.0, (0.45, 0.85, 1.0), 0.9),
			("Forge", (-1.2, 1.0, 0.4), 35.0, (1.0, 0.5, 0.2), 0.8), ("Fill", (0.9, -1.6, -0.3), 8.0, (0.55, 0.65, 1.0), 1.5),
			("Sky", (0.0, 0.3, 1.6), 22.0, (0.85, 0.87, 0.92), 2.5), ("Bounce", (0.0, -1.0, -1.0), 25.0, (1.0, 0.55, 0.3), 1.5)]},
	"B": {"label": "Woken in the Wreck", "line": "Rust-streaked and torn open to the works, a stack bent and the firebox sprung on the cyan ember that woke it.",
		"yaw": -26.0, "body": -24.0, "pitch": 12.0, "lean": 6.0, "scale": 0.68, "scowl": 13.0, "lids": None, "skin": None, "eyes": None,
		"glow": CYAN, "finish": "iron", "rust": 1.0, "soot": 1.0, "lamp": 0.9, "needle": 142.0, "letters": "T.R.R.", "steam": 1.0,
		"smoke": (0.36, 0.34, 0.33), "fire": 22.0, "torn": True, "bent": True, "cracked": True, "sprung": True,
		"world": (0.13, 0.08, 0.06), "cam": (2.10, -0.005),
		"lights": [("Key", (-0.85, -0.45, 0.30), 38.0, (1.0, 0.70, 0.45), 0.7), ("Rim", (1.25, 1.1, 0.45), 130.0, (0.45, 0.85, 1.0), 0.9),
			("Forge", (-1.2, 1.0, 0.2), 70.0, (1.0, 0.45, 0.15), 0.8), ("Fill", (0.9, -1.6, -0.3), 8.0, (0.55, 0.65, 1.0), 1.5)]},
	"C": {"label": "Company Iron", "line": "The day it left the works: black enamel, polished brass, the company crest and a warm, tidy headlamp.",
		"yaw": 16.0, "body": 30.0, "pitch": 4.0, "lean": 5.0, "scale": 0.7, "scowl": 0.0, "lids": None, "skin": None, "eyes": None,
		"glow": WARM, "finish": "enamel", "rust": 0.0, "soot": 0.0, "lamp": 0.9, "needle": -20.0, "letters": None, "crest": True,
		"nameplate": True, "steam": 0.3, "fire": 10.0, "world": (0.10, 0.085, 0.07), "cam": (2.10, 0.0),
		"lights": [("Key", (0.85, -0.45, 0.35), 40.0, (1.0, 0.80, 0.60), 0.7), ("Rim", (-1.3, 1.1, 0.45), 110.0, (1.0, 0.62, 0.32), 0.9),
			("Cool", (1.2, 1.0, 0.5), 50.0, (0.55, 0.75, 1.0), 0.8), ("Fill", (-0.9, -1.6, -0.3), 10.0, (0.6, 0.65, 0.9), 1.5),
			("Brow", (0.1, -0.7, 0.5), 8.0, (1.0, 0.85, 0.65), 0.5)]},
}

# ---------------------------------------------------------------- primitives (each under a parent Empty)

def M(loc=(0, 0, 0), rot=(0, 0, 0), scale=None):
	"""A transform: an optional scale, then rotation in degrees (XYZ), then placed at loc."""
	m = Matrix.Translation(loc) @ Euler([math.radians(a) for a in rot], "XYZ").to_matrix().to_4x4()
	return m @ Matrix.Diagonal((*scale, 1)) if scale else m

def empty(name, loc, rot=(0, 0, 0), parent=None):
	e = link(bpy.data.objects.new(name, None))
	e.location, e.rotation_euler, e.parent = loc, Euler([math.radians(a) for a in rot], "XYZ"), parent
	return e

def obj(name, bm, mat, parent, at=None, smooth=True):
	"""bm as an object under parent. at None: the mesh is already in the parent's space; else at is the object's own
	transform, so its Object coordinates stay local (the lens, dial and flame materials need that)."""
	me = bpy.data.meshes.new(name)
	bm.to_mesh(me)
	bm.free()
	if smooth:
		me.shade_smooth()
	ob = link(bpy.data.objects.new(name, me))
	ob.parent = parent
	if at is not None:
		ob.matrix_basis = at
	if mat is not None:
		ob.data.materials.append(mat)
	return ob

def bevel(ob, w, seg=3):
	# hardened normals keep the plates' faces flat and only the bevels round
	add_mod(ob, "BEVEL", width=w, segments=seg, limit_method="ANGLE", harden_normals=True)
	return ob

def box(name, half, mat, parent, at=None, taper=(1.0, 1.0), bev=0.008):
	"""A box of half-sizes half, its top face scaled by taper, bevelled."""
	bm = bmesh.new()
	bmesh.ops.create_cube(bm, size=2.0)
	for v in bm.verts:
		k = taper if v.co.z > 0 else (1.0, 1.0)
		v.co = Vector((v.co.x * half[0] * k[0], v.co.y * half[1] * k[1], v.co.z * half[2]))
	bm.transform(at or Matrix())
	ob = obj(name, bm, mat, parent)
	return bevel(ob, bev) if bev else ob

def cyl(name, r, depth, mat, parent, at=None, segs=40, r2=None, bev=0.003, caps=True):
	"""A cylinder (or cone, r2) along its local z, centred."""
	bm = bmesh.new()
	bmesh.ops.create_cone(bm, cap_ends=caps, cap_tris=False, segments=segs, radius1=r, radius2=r if r2 is None else r2,
		depth=depth, matrix=at or Matrix())
	ob = obj(name, bm, mat, parent)
	return bevel(ob, bev, 2) if bev else ob

def grid(name, rows, mat, parent, close=True, cap_top=None):
	"""A quad grid from rows of points (each row a ring when close); cap_top: a point the last row fans to."""
	nr, nc = len(rows), len(rows[0])
	bm = bmesh.new()
	vs = [bm.verts.new(tuple(p)) for row in rows for p in row]
	faces = [(j * nc + i, j * nc + (i + 1) % nc, (j + 1) * nc + (i + 1) % nc, (j + 1) * nc + i)
		for j in range(nr - 1) for i in range(nc if close else nc - 1)]
	if cap_top is not None:
		vs.append(bm.verts.new(tuple(cap_top)))
		faces += [((nr - 1) * nc + i, (nr - 1) * nc + (i + 1) % nc, len(vs) - 1) for i in range(nc)]
	for f in faces:
		try:
			bm.faces.new([vs[i] for i in f])
		except ValueError:   # a face that exists already: skip it
			pass
	bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
	return obj(name, bm, mat, parent)

def superellipse(rx, ry, cx, cy, z, n, p):
	t = np.linspace(0, 2 * math.pi, n, endpoint=False)
	c, s = np.cos(t), np.sin(t)
	return np.stack([cx + rx * np.sign(c) * np.abs(c) ** (2 / p), cy + ry * np.sign(s) * np.abs(s) ** (2 / p), np.full(n, z)], 1)

def torus(name, R, r, mat, parent, at=None, seg=56, tseg=10):
	rows = [[((R + r * math.cos(b)) * math.cos(a), (R + r * math.cos(b)) * math.sin(a), r * math.sin(b))
		for b in np.linspace(0, 2 * math.pi, tseg, endpoint=False)] for a in np.linspace(0, 2 * math.pi, seg, endpoint=False)]
	ob = grid(name, rows + [rows[0]], mat, parent)
	ob.data.transform(at or Matrix())
	return ob

def dome(name, rad, mat, parent, at, keep=None, thick=0.01, seg=56, rings=16):
	"""Half an ellipsoid (rad = rx, ry, rz) opening down; keep(x, y, z) on the unit dome drops the faces it refuses."""
	bm = bmesh.new()
	rows = [[bm.verts.new((rad[0] * math.cos(ph) * math.cos(t), rad[1] * math.cos(ph) * math.sin(t), rad[2] * math.sin(ph)))
		for t in np.linspace(0, 2 * math.pi, seg, endpoint=False)] for ph in np.linspace(0, math.pi / 2, rings, endpoint=False)]
	pole = bm.verts.new((0, 0, rad[2]))
	for i in range(rings):
		for j in range(seg):
			k = (j + 1) % seg
			vs = [rows[i][j], rows[i][k], pole] if i == rings - 1 else [rows[i][j], rows[i][k], rows[i + 1][k], rows[i + 1][j]]
			c = sum((v.co for v in vs), Vector()) / len(vs)
			if keep is None or keep(c.x / rad[0], c.y / rad[1], c.z / rad[2]):
				bm.faces.new(vs)
	bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
	bm.transform(at)
	ob = obj(name, bm, mat, parent)
	add_mod(ob, "SOLIDIFY", thickness=thick, offset=-1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	return ob

def rivets(name, pts, nrms, r, mat, parent):
	"""Round-headed rivets: one mesh of squashed spheres, each standing on its normal."""
	bm = bmesh.new()
	for p, n in zip(pts, nrms):
		q = Vector(n).normalized().to_track_quat("Z", "Y").to_matrix().to_4x4()
		bmesh.ops.create_uvsphere(bm, u_segments=10, v_segments=6, radius=r, matrix=Matrix.Translation(Vector(p)) @ q @ Matrix.Diagonal((1, 1, 0.6, 1)))
	return obj(name, bm, mat, parent)

def pipe(name, pts, r, mat, parent):
	cu = bpy.data.curves.new(name, "CURVE")
	cu.dimensions, cu.bevel_depth, cu.bevel_resolution, cu.use_fill_caps = "3D", r, 4, True
	sp = cu.splines.new("BEZIER")
	sp.bezier_points.add(len(pts) - 1)
	for bp, p in zip(sp.bezier_points, pts):
		bp.co, bp.handle_left_type, bp.handle_right_type = p, "AUTO", "AUTO"
	cu.materials.append(mat)
	ob = link(bpy.data.objects.new(name, cu))
	ob.parent = parent
	return ob

def text(name, body, size, mat, parent, at, extrude=0.0015):
	"""Raised letters as a mesh (the aux pass and paint.js see meshes, not font objects)."""
	cu = bpy.data.curves.new(name, "FONT")
	cu.body, cu.size, cu.extrude, cu.align_x, cu.align_y = body, size, extrude, "CENTER", "CENTER"
	tmp = link(bpy.data.objects.new(name + ".font", cu))
	me = bpy.data.meshes.new_from_object(tmp.evaluated_get(bpy.context.evaluated_depsgraph_get()))
	bpy.data.objects.remove(tmp)
	me.materials.clear()
	me.materials.append(mat)
	ob = link(bpy.data.objects.new(name, me))
	ob.parent, ob.matrix_basis = parent, at
	return ob

def gear(name, r, teeth, thick, mat, parent, at):
	"""A spur gear: a toothed disc on a hub."""
	bm = bmesh.new()
	prof = [(r * rr * math.cos(t), r * rr * math.sin(t)) for k in range(teeth)
		for da, rr in ((0.0, 0.84), (0.18, 0.84), (0.28, 1.0), (0.62, 1.0), (0.72, 0.84)) for t in [2 * math.pi * (k + da) / teeth]]
	top = [bm.verts.new((x, y, thick / 2)) for x, y in prof]
	bot = [bm.verts.new((x, y, -thick / 2)) for x, y in prof]
	bm.faces.new(top)
	bm.faces.new(bot[::-1])
	n = len(prof)
	for i in range(n):
		bm.faces.new([bot[i], bot[(i + 1) % n], top[(i + 1) % n], top[i]])
	bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
	bmesh.ops.create_cone(bm, cap_ends=True, segments=24, radius1=r * 0.3, radius2=r * 0.3, depth=thick * 2.2)
	bm.transform(at)
	return bevel(obj(name, bm, mat, parent), 0.0015, 1)

def point_light(name, parent, loc, power, color, radius=0.03):
	ld = bpy.data.lights.new(name, "POINT")
	ld.energy, ld.color, ld.shadow_soft_size = power, lin(color), radius
	lo = link(bpy.data.objects.new(name, ld))
	lo.parent, lo.location = parent, loc
	return lo

# ---------------------------------------------------------------- shader helpers and materials

def feed(nt, sock, v):
	"""Set an input socket to a value, or link a socket into it."""
	if isinstance(v, (int, float, tuple)):
		sock.default_value = v
	else:
		nt.links.new(v, sock)

def math_node(nt, operation, *ins):
	n = nt.nodes.new("ShaderNodeMath")
	n.operation = operation
	for k, v in enumerate(ins):
		feed(nt, n.inputs[k], v)
	return n.outputs[0]

def mrange(nt, src, a, b, lo=0.0, hi=1.0):
	"""A clamped Map Range: src from a..b to lo..hi."""
	n = nt.nodes.new("ShaderNodeMapRange")
	nt.links.new(src, n.inputs["Value"])
	for k, v in zip(("From Min", "From Max", "To Min", "To Max"), (a, b, lo, hi)):
		n.inputs[k].default_value = v
	return n.outputs[0]

def coords(nt):
	return nt.nodes.new("ShaderNodeTexCoord").outputs["Object"]

def radial(nt, co):
	"""(distance from the object's z axis, z)."""
	sep = nt.nodes.new("ShaderNodeSeparateXYZ")
	nt.links.new(co, sep.inputs[0])
	xy = nt.nodes.new("ShaderNodeCombineXYZ")
	nt.links.new(sep.outputs[0], xy.inputs[0])
	nt.links.new(sep.outputs[1], xy.inputs[1])
	ln = nt.nodes.new("ShaderNodeVectorMath")
	ln.operation = "LENGTH"
	nt.links.new(xy.outputs[0], ln.inputs[0])
	return ln.outputs["Value"], sep.outputs[2]

def emission(nt, color, strength):
	em = nt.nodes.new("ShaderNodeEmission")
	feed(nt, em.inputs["Color"], color)
	feed(nt, em.inputs["Strength"], strength)
	return em

def mix_shader(nt, fac, a, b):
	mx = nt.nodes.new("ShaderNodeMixShader")
	feed(nt, mx.inputs[0], fac)
	nt.links.new(a.outputs[0], mx.inputs[1])
	nt.links.new(b.outputs[0], mx.inputs[2])
	return mx

def ao_grime(nt, col, dist, dark):
	"""Grime in the crevices: col darkened toward dark where ambient occlusion closes in."""
	ao = nt.nodes.new("ShaderNodeAmbientOcclusion")
	ao.inputs["Distance"].default_value = dist
	return mix_rgb(nt, ao.outputs["AO"], dark, col)

def iron_mat(opt, name="Iron", tone=1.0):
	"""Working iron: dark, oiled, metal; rust patches and rain streaks over it in B."""
	def build(nt):
		co = coords(nt)
		big, fine = noise(nt, 3.0, 6.0, 0.6, co).outputs["Fac"], noise(nt, 55.0, 8.0, 0.65, co).outputs["Fac"]
		col = mix_rgb(nt, big, tuple(c * tone for c in (0.20, 0.205, 0.21)), tuple(c * tone for c in (0.36, 0.36, 0.37)))
		b = principled(nt, ao_grime(nt, col, 0.035, (0.02, 0.016, 0.012)), rough=0.45, spec=0.5, metal=0.9)
		nt.links.new(mrange(nt, fine, 0.0, 1.0, 0.30, 0.62), b.inputs["Roughness"])
		nt.links.new(bump(nt, fine, 0.12, 0.0015), b.inputs["Normal"])
		if opt["rust"] <= 0:
			return b
		mp = nt.nodes.new("ShaderNodeMapping")   # rain streaks: noise stretched down the plates
		nt.links.new(co, mp.inputs["Vector"])
		mp.inputs["Scale"].default_value = (16.0, 16.0, 1.4)
		patch = mrange(nt, noise(nt, 4.0, 8.0, 0.7, co).outputs["Fac"], 0.50, 0.62, 0.0, opt["rust"])
		streak = mrange(nt, noise(nt, 2.5, 6.0, 0.6, mp.outputs[0]).outputs["Fac"], 0.56, 0.70, 0.0, opt["rust"] * 0.8)
		rust = principled(nt, mix_rgb(nt, fine, (0.28, 0.11, 0.045), (0.58, 0.27, 0.10)), rough=0.9, spec=0.3)
		return mix_shader(nt, math_node(nt, "MAXIMUM", patch, streak), b, rust)
	return material(name, ID["lead"], build)

def enamel_mat(color=(0.012, 0.012, 0.014), pid=ID["black"], name="Enamel"):
	"""Company enamel: glossy under a clear coat."""
	return material(name, pid, lambda nt: principled(nt, color, rough=0.22, spec=0.5, **{"Coat Weight": 1.0, "Coat Roughness": 0.06}))

def brass_mat(opt):
	"""Brass: polished in C, oiled in A, tarnished brown in B."""
	polish = opt["finish"] == "enamel"

	def build(nt):
		co = coords(nt)
		col = mix_rgb(nt, noise(nt, 90.0, 6.0, 0.6, co).outputs["Fac"], (0.66, 0.48, 0.21), (0.90, 0.72, 0.38))
		tarnish = 0.0 if polish else (0.8 if opt["rust"] > 0 else 0.15)
		col = mix_rgb(nt, mrange(nt, noise(nt, 6.0, 6.0, 0.65, co).outputs["Fac"], 0.4, 0.65, 0.0, tarnish), col, (0.20, 0.17, 0.09))
		return principled(nt, ao_grime(nt, col, 0.02, (0.05, 0.035, 0.015)), rough=0.16 if polish else 0.34, spec=0.5, metal=1.0)
	return material("Brass", ID["brass"], build)

def flat_mat(name, color, pid, rough=0.8):
	return material(name, pid, lambda nt: principled(nt, color, rough=rough, spec=0.4))

def glow_mat(name, color, strength, pid, mask):
	"""Light: an emission the aux pass marks with mask (0..1) for paint.js to bloom."""
	m = material(name, pid, lambda nt: emission(nt, (*lin(color), 1.0), strength))
	m["looter_glow"] = mask
	return m

def lens_mat(opt, R):
	"""The headlamp's lens: a hot core in a saturated glow, ringed like a locomotive lamp's so it reads as a lamp and
	not an eyeball, a hint of glass; cracked in B. AgX greys out bright saturated colour, so only the core burns hot."""
	def build(nt):
		co = coords(nt)
		r = math_node(nt, "DIVIDE", radial(nt, co)[0], R)
		ramp = nt.nodes.new("ShaderNodeValToRGB")
		g, el = lin(opt["glow"]), ramp.color_ramp.elements
		el[0].position, el[0].color = 0.0, (*[min(1.0, c * 0.6 + 0.4) for c in g], 1)
		el[1].position, el[1].color = 1.0, (*[c * 0.2 for c in g], 1)
		el.new(0.12).color = (*g, 1)
		el.new(0.7).color = (*[c * 0.6 for c in g], 1)
		nt.links.new(r, ramp.inputs[0])
		rings = math_node(nt, "MULTIPLY_ADD", math_node(nt, "SINE", math_node(nt, "MULTIPLY", r, 40.0)), 0.18, 0.82)
		col = mix_rgb(nt, 1.0, ramp.outputs[0], rings, "MULTIPLY")
		if opt.get("cracked"):
			vor = nt.nodes.new("ShaderNodeTexVoronoi")
			vor.feature = "DISTANCE_TO_EDGE"
			vor.inputs["Scale"].default_value = 45.0
			nt.links.new(co, vor.inputs["Vector"])
			col = mix_rgb(nt, 1.0, col, mrange(nt, vor.outputs["Distance"], 0.0, 0.05, 0.08, 1.0), "MULTIPLY")
		em = emission(nt, col, mrange(nt, r, 0.0, 0.3, opt["lamp"] * 4.0, opt["lamp"]))
		gl = nt.nodes.new("ShaderNodeBsdfGlossy")
		gl.inputs["Roughness"].default_value, gl.inputs["Color"].default_value = 0.25, (0.03, 0.03, 0.03, 1)   # more, and the sky light greys the glow
		add = nt.nodes.new("ShaderNodeAddShader")
		nt.links.new(em.outputs[0], add.inputs[0])
		nt.links.new(gl.outputs[0], add.inputs[1])
		return add
	m = material("Lens", GLASS, build)
	m["looter_glow"] = 1.0
	return m

def dial_mat(opt, R):
	"""The pressure gauge's face, painted into an image: ivory enamel, ticks, a red arc at the top of the scale."""
	S = 256
	v, u = (np.mgrid[0:S, 0:S] + 0.5) / S * 2 - 1
	r, a = np.hypot(u, v), np.degrees(np.arctan2(u, v))      # a: 0 at the top, clockwise positive
	img = np.ones((S, S, 4))
	img[..., :3] = np.array([0.88, 0.82, 0.68]) * (1 - 0.25 * r[..., None] ** 3)
	if opt["rust"] > 0:
		img[..., :3] *= 0.75 + 0.25 * np.cos(u * 7 + v * 3)[..., None] ** 2
	on = np.abs(a) <= 135
	ticks = on & (((np.abs(((a + 135) % 27) - 13.5) > 12.3) & (r > 0.66)) | ((np.abs(((a + 135) % 6.75) - 3.375) > 2.9) & (r > 0.76))) & (r < 0.86)
	img[ticks | ((r > 0.88) & (r < 0.92))] = (0.08, 0.07, 0.06, 1)
	img[on & (a > 75) & (ticks | ((r > 0.60) & (r < 0.70)))] = (0.72, 0.07, 0.04, 1)
	im = bpy.data.images.new("Dial", S, S, alpha=True)
	im.pixels.foreach_set(img.astype(np.float32).ravel())

	def build(nt):
		mp = nt.nodes.new("ShaderNodeMapping")
		nt.links.new(coords(nt), mp.inputs["Vector"])
		mp.inputs["Scale"].default_value, mp.inputs["Location"].default_value = (0.5 / R, 0.5 / R, 1), (0.5, 0.5, 0)
		tx = nt.nodes.new("ShaderNodeTexImage")
		tx.image, tx.extension = im, "EXTEND"
		nt.links.new(mp.outputs[0], tx.inputs["Vector"])
		return principled(nt, tx.outputs["Color"], rough=0.4, spec=0.5, **{"Coat Weight": 1.0, "Coat Roughness": 0.03})   # the coat: its glass
	return material("Dial", GLASS, build)

def paint_mat(opt):
	"""The company's letters in cream paint, worn through in patches (half gone in B)."""
	def build(nt):
		co = coords(nt)
		wear = math_node(nt, "MULTIPLY_ADD", noise(nt, 9.0, 4.0, 0.6, co).outputs["Fac"], 0.7, noise(nt, 70.0, 6.0, 0.7, co).outputs["Fac"])
		kept = math_node(nt, "GREATER_THAN", wear, 0.82 if opt["rust"] > 0 else 0.4)   # wear runs about 0.7 to 0.95: B keeps about half
		return mix_shader(nt, kept, nt.nodes.new("ShaderNodeBsdfTransparent"), principled(nt, (0.90, 0.82, 0.62), rough=0.7, spec=0.3))
	return material("Paint", ID["linen"], build)

def steam_mat(dens, H, r0, r1, color):
	"""Steam in a cone (r0 at the bottom, r1 at the top, H tall, centred on its origin): a volume soft at its edges,
	thinning upward, broken up by noise. Marked so render_aux_glow leaves it out (the aux pass would see a solid cone)."""
	mat = bpy.data.materials.new("Steam")
	mat.use_nodes = True
	nt = mat.node_tree
	nt.nodes.clear()
	co = coords(nt)
	rad, z = radial(nt, co)
	t = math_node(nt, "MULTIPLY_ADD", z, 1.0 / H, 0.5)
	r = math_node(nt, "DIVIDE", rad, math_node(nt, "MULTIPLY_ADD", t, r1 - r0, r0))
	fall = math_node(nt, "POWER", math_node(nt, "MAXIMUM", math_node(nt, "SUBTRACT", 1.0, r), 0.0), 1.5)
	fade = math_node(nt, "MAXIMUM", math_node(nt, "SUBTRACT", 1.0, t), 0.0)
	wisps = mrange(nt, noise(nt, 7.0, 3.0, 0.55, co).outputs["Fac"], 0.42, 0.72)
	vol = nt.nodes.new("ShaderNodeVolumePrincipled")
	vol.inputs["Color"].default_value = (*lin(color), 1)
	nt.links.new(math_node(nt, "MULTIPLY", math_node(nt, "MULTIPLY", fall, fade), math_node(nt, "MULTIPLY", wisps, dens)), vol.inputs["Density"])
	nt.links.new(vol.outputs[0], nt.nodes.new("ShaderNodeOutputMaterial").inputs["Volume"])
	mat["looter_aux_skip"] = True
	return mat

def flame_mat(H):
	"""A tongue of flame on a cone H tall centred on its origin: yellow-hot at the root, red and fading out at the tip."""
	def build(nt):
		t = math_node(nt, "MULTIPLY_ADD", radial(nt, coords(nt))[1], 1.0 / H, 0.5)
		u = math_node(nt, "SUBTRACT", 1.0, t)
		em = emission(nt, mix_rgb(nt, t, (1.0, 0.7, 0.25), (0.9, 0.15, 0.02)), math_node(nt, "MULTIPLY", u, 2.5))
		return mix_shader(nt, math_node(nt, "MULTIPLY", math_node(nt, "POWER", u, 2.0), 0.85), nt.nodes.new("ShaderNodeBsdfTransparent"), em)
	m = material("Flame", ID["black"], build)
	m["looter_glow"] = 0.5
	return m

def materials(opt):
	"""The option's palette: iron plates (A, B) or black enamel over them (C); brass; soot; the fire's lights."""
	enamel = opt["finish"] == "enamel"
	m = {"iron": iron_mat(opt, tone=1.25 if enamel else 1.0), "dark": iron_mat(opt, "IronDark", 0.55),
		"brass": brass_mat(opt), "soot": flat_mat("Soot", (0.03, 0.027, 0.025), ID["black"], 0.9),
		"red": flat_mat("Needle", (0.62, 0.05, 0.03), ID["black"], 0.35)}
	m["plate"] = enamel_mat() if enamel else m["iron"]
	m["fire"] = glow_mat("Fire", (1.0, 0.48, 0.12), 2.0, ID["black"], 0.5)   # any hotter and AgX turns orange white
	m["hearth"] = glow_mat("Hearth", (0.75, 0.16, 0.03), 0.6, ID["black"], 0.3)
	m["mouth"] = glow_mat("MouthGlow", (1.0, 0.55, 0.2) if enamel else (1.0, 0.48, 0.12), 2.2, ID["black"], 0.3)
	m["ember"] = glow_mat("Ember", tuple(min(1.0, c * 0.85 + 0.15) for c in opt["glow"]), 2.5, ID["eye"], 1.0)
	return m

# ---------------------------------------------------------------- the head

LAMP_Z = 0.165   # the headlamp's centre above the neck pivot

def face_y(z):
	"""The cranium's front at height z (it leans back toward the crown)."""
	return 0.005 - 0.112 * (1 - 0.10 * (z - 0.045) / 0.23)

def build_head(opt, m, head):
	"""A squared iron head: the headlamp set in it like one eye under a riveted brow (a scowl in A and B, level in C with
	the GAUGE nameplate), cheek guards, a heavy jaw with the grille mouth, bolts for ears."""
	P, B = m["plate"], m["brass"]
	box("Cranium", (0.112, 0.112, 0.115), P, head, M((0, 0.005, 0.16)), taper=(0.86, 0.9), bev=0.022)
	box("Ridge", (0.016, 0.09, 0.012), P, head, M((0, 0.01, 0.272)), bev=0.007)
	# the jaw juts: the grille mouth sits in it, the furnace's light behind its bars
	box("Muzzle", (0.118, 0.04, 0.034), P, head, M((0, -0.084, 0.072), (-4, 0, 0)), taper=(0.92, 1.0), bev=0.014)
	box("MouthGlow", (0.074, 0.0012, 0.012), m["mouth"], head, M((0, -0.1245, 0.072)), bev=0)
	for k, x in enumerate(np.linspace(-0.066, 0.066, 9)):
		cyl(f"Bar{k}", 0.0045, 0.028, m["iron"], head, M((x, -0.128, 0.072)), segs=12, bev=0)
	for z in (0.056, 0.088):
		box("Lip", (0.082, 0.006, 0.0042), B, head, M((0, -0.127, z)), bev=0.002)
	# the headlamp in its socket: a brass bezel, the lens just behind it, a little domed
	fy, R = face_y(LAMP_Z), 0.052
	cyl("Socket", R + 0.006, 0.01, m["soot"], head, M((0, fy - 0.001, LAMP_Z), (90, 0, 0)), bev=0)
	torus("Bezel", R + 0.004, 0.0085, B, head, M((0, fy - 0.007, LAMP_Z), (90, 0, 0)))
	bm = bmesh.new()
	vs = [[bm.verts.new((R * s * math.cos(t), R * s * math.sin(t), 0.009 * (1 - s * s))) for t in np.linspace(0, 2 * math.pi, 48, endpoint=False)]
		for s in np.linspace(1.0, 0.05, 10)]
	for j in range(len(vs) - 1):
		for i in range(48):
			bm.faces.new([vs[j][i], vs[j][(i + 1) % 48], vs[j + 1][(i + 1) % 48], vs[j + 1][i]])
	bm.faces.new(vs[-1])
	obj("Lens", bm, lens_mat(opt, R), head, M((0, fy - 0.006, LAMP_Z), (90, 0, 0)))
	point_light("LampLight", head, (0, fy - 0.07, LAMP_Z), opt["lamp"] * 4.0, opt["glow"], 0.02)   # washes the brow and cheeks
	# the brow: a riveted ledge in two plates dipping to a V over the lamp, the scowl the one eye glares from under;
	# the scowl is the ember's, so company iron (C) wears it level, with the nameplate on it
	k = math.tan(math.radians(opt["scowl"]))
	for s in (-1, 1):
		box("Brow", (0.064, 0.03, 0.015), P, head, M((s * 0.056, fy - 0.016, LAMP_Z + 0.064), (14, -s * opt["scowl"], 0)), bev=0.006)
		if not opt.get("nameplate"):
			rivets("BrowRivets", [(s * (0.02 + 0.035 * j), fy - 0.047, LAMP_Z + 0.057 + (0.035 * j - 0.036) * k) for j in range(3)], [(0, -1, -0.2)] * 3, 0.0045, B, head)
		box("Cheek", (0.014, 0.022, 0.062), P, head, M((s * 0.098, fy - 0.012, 0.142), (0, 0, s * -8)), bev=0.006)
		cyl("Ear", 0.032, 0.022, m["iron"], head, M((s * 0.112, 0.0, 0.15), (0, 90, 0)))
		cyl("EarCap", 0.019, 0.014, B, head, M((s * 0.125, 0.0, 0.15), (0, 90, 0)), segs=6, bev=0.002)
		rivets("EarRivets", [(s * 0.124, 0.025 * math.cos(a), 0.15 + 0.025 * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 5, endpoint=False)], [(s, 0, 0)] * 5, 0.0035, B, head)
	if opt.get("nameplate"):
		box("Nameplate", (0.08, 0.003, 0.014), B, head, M((0, fy - 0.05, LAMP_Z + 0.058), (-4, 0, 0)), bev=0.002)
		text("Name", "GAUGE", 0.026, enamel_mat(name="NameEnamel"), head, M((0, fy - 0.0535, LAMP_Z + 0.0575), (86, 0, 0)), 0.0012)
	# rivets down the face's edges and along the crown's front
	pts = [(s * 0.088, face_y(z) - 0.002, z) for s in (-1, 1) for z in np.linspace(0.11, 0.25, 6)]
	pts += [(x, face_y(0.262) + 0.006, 0.268) for x in np.linspace(-0.07, 0.07, 6)]
	rivets("HeadRivets", pts, [(0, -1, 0)] * 12 + [(0, -0.7, 0.7)] * 6, 0.0045, B, head)

# ---------------------------------------------------------------- the body

TORSO = [(-0.80, 0.25, 0.19, 0.02), (-0.55, 0.285, 0.21, 0.01), (-0.30, 0.30, 0.215, 0.0), (-0.12, 0.29, 0.205, 0.0),
	(-0.045, 0.25, 0.185, 0.01), (-0.005, 0.17, 0.14, 0.02), (0.012, 0.10, 0.10, 0.02)]   # (z, half-width, half-depth, y)

def torso_at(z):
	return [float(np.interp(z, [r[0] for r in TORSO], [r[k] for r in TORSO])) for k in (1, 2, 3)]

def chest_y(x, z, p=3.2):
	"""The boiler's front at (x, z)."""
	rx, ry, cy = torso_at(z)
	return cy - ry * max(0.0, 1 - abs(x / rx) ** p) ** (1 / p)

def band(name, z, h, mat, parent, grow=0.008, n_rivets=20):
	"""A brass band round the boiler, riveted."""
	rx, ry, cy = torso_at(z)
	ob = grid(name, [superellipse(rx + grow, ry + grow, 0, cy, z + d, 72, 3.2) for d in (-h, h)], mat, parent)
	add_mod(ob, "SOLIDIFY", thickness=0.004, offset=-1.0)
	bevel(ob, 0.0015, 1)
	ring = superellipse(rx + grow + 0.002, ry + grow + 0.002, 0, cy, z, n_rivets, 3.2)
	rivets(name + "Rivets", ring, [(p[0] / (rx * rx), (p[1] - cy) / (ry * ry), 0) for p in ring], 0.0055, mat, parent)

def build_pauldron(opt, m, body, s, torn):
	"""A domed pauldron over a skirt and a lame, a brass rim and rivets; in B its front and top torn away along a
	jagged line (facing the camera) to the works: gears, a piston to the arm, a hose, the furnace's light."""
	at = M((s * 0.32, 0.0, -0.035), (0, s * 20, 0))
	rad = (0.17, 0.18, 0.165)
	keep = (lambda x, y, z: -y * 0.8 + z * 0.4 - s * x * 0.3 + 0.12 * math.sin(math.atan2(y, x) * 9) + 0.08 * math.sin(z * 29) < 0.38) if torn else None
	dome(f"Pauldron{s}", rad, m["plate"], body, at, keep, 0.012)
	if not torn:
		torus(f"PauldronRim{s}", 1.0, 0.012, m["brass"], body, at @ Matrix.Diagonal((rad[0], rad[1], 1.0, 1)))
		ph = math.radians(24)
		pts = [(rad[0] * math.cos(ph) * math.cos(t), rad[1] * math.cos(ph) * math.sin(t), rad[2] * math.sin(ph)) for t in np.linspace(0, 2 * math.pi, 18, endpoint=False)]
		pts += [(0.0, rad[1] * math.cos(p), rad[2] * math.sin(p)) for p in np.radians([40, 60, 80, 100, 120, 140])]
		rot = at.to_3x3()
		rivets(f"PauldronRivets{s}", [at @ Vector(p) for p in pts], [rot @ Vector((p[0] / rad[0] ** 2, p[1] / rad[1] ** 2, p[2] / rad[2] ** 2)) for p in pts], 0.0062, m["brass"], body)
	else:
		dome("Inner", tuple(r * 0.45 for r in rad), m["dark"], body, at, None, 0.01)
		for r, t, loc, rot in ((0.075, 16, (0.0, -0.07, 0.09), (53, 0, 0)), (0.045, 11, (0.085, -0.095, 0.045), (53, 0, 8)),
				(0.035, 9, (-0.075, -0.1, 0.035), (53, 0, 15))):
			gear("Gear", r, t, 0.016, m["brass"], body, at @ M(loc, rot))
		a, b = at @ Vector((-0.03, -0.02, 0.12)), at @ Vector((0.06, -0.16, -0.06))
		q = (b - a).to_track_quat("Z", "Y").to_matrix().to_4x4()
		cyl("PistonCase", 0.024, (b - a).length * 0.55, m["iron"], body, Matrix.Translation(a + (b - a) * 0.275) @ q)
		cyl("PistonRod", 0.011, (b - a).length * 0.6, m["brass"], body, Matrix.Translation(a + (b - a) * 0.7) @ q, segs=16)
		pipe("Hose", [at @ Vector(p) for p in ((-0.08, -0.02, 0.08), (0.0, -0.14, 0.04), (0.1, -0.12, -0.05))], 0.008, m["dark"], body)
		point_light("WorksGlow", body, at @ Vector((0.0, -0.04, 0.03)), 3.0, (1.0, 0.5, 0.18), 0.03)
	for name, (k0, z0), (k1, z1) in (("Skirt", (1.0, 0.0), (1.07, -0.055)), ("Lame", (1.0, -0.05), (1.05, -0.105))):
		ob = grid(f"{name}{s}", [[(rad[0] * kk * math.cos(t), rad[1] * kk * math.sin(t), zz) for t in np.linspace(0, 2 * math.pi, 56, endpoint=False)]
			for kk, zz in ((k0, z0), (k1, z1))], m["plate"], body)
		ob.data.transform(at)
		add_mod(ob, "SOLIDIFY", thickness=0.01, offset=-1.0)
	cyl(f"Arm{s}", 0.10, 0.4, m["iron"], body, M((s * 0.36, 0.0, -0.32)), bev=0)

def build_stack(opt, m, body, s, bent):
	"""A smokestack close behind the head (both flank it inside the picture), splayed out a little, flared at the crown;
	kinked in B; steam (or B's smoke) drifting off it on the wind."""
	base, tilt, h = Vector((s * 0.18, 0.15, -0.08)), Euler((0, math.radians(s * 9), 0)).to_matrix(), 0.33
	segs = [(base, tilt, h)]
	if bent:
		mid = base + tilt @ Vector((0, 0, h * 0.55))
		segs = [(base, tilt, h * 0.55), (mid, Euler((math.radians(-14), math.radians(s * 38), 0)).to_matrix(), h * 0.45)]
		torus("Kink", 0.038, 0.008, m["dark"], body, Matrix.Translation(mid) @ tilt.to_4x4())
	for k, (p, rot, L) in enumerate(segs):
		cyl(f"Stack{s}{k}", 0.036, L, m["iron"], body, Matrix.Translation(p + rot @ Vector((0, 0, L / 2))) @ rot.to_4x4(), bev=0)
	p, rot, L = segs[-1]
	top, R4 = p + rot @ Vector((0, 0, L)), rot.to_4x4()
	at = lambda dz: Matrix.Translation(top + rot @ Vector((0, 0, dz))) @ R4
	cyl("StackBand", 0.039, 0.016, m["brass"], body, at(-0.06))
	cyl("Crown", 0.036, 0.04, m["iron"] if opt["soot"] < 0.5 else m["soot"], body, at(0.02), r2=0.056, caps=False)
	cyl("CrownLip", 0.058, 0.012, m["brass"] if opt["finish"] == "enamel" else m["soot"], body, at(0.042))
	cyl("Flue", 0.05, 0.004, m["soot"], body, at(0.047), bev=0)
	if opt["steam"] > 0:
		# in world space, on one wind blowing to the right, so the wisps drift across the top of the picture
		k = opt["scale"]
		H, lean = 0.4 * k, Euler((math.radians(-8), math.radians(58), 0)).to_matrix()
		bm = bmesh.new()
		bmesh.ops.create_cone(bm, cap_ends=True, segments=24, radius1=0.037 * k, radius2=0.14 * k, depth=H)
		obj("Steam", bm, steam_mat(opt["steam"] * 12.0, H, 0.035 * k, 0.133 * k, opt.get("smoke", (0.72, 0.72, 0.74))), None,
			Matrix.Translation(body.matrix_basis @ (top + rot @ Vector((0, 0, 0.045))) + lean @ Vector((0, 0, H / 2))) @ lean.to_4x4())

def build_firebox(opt, m, body):
	"""The firebox high in the belly under the chest band (its top shows at the picture's bottom edge): a riveted door
	with glowing louvres; in B sprung on its hinge on a dark hearth, coals, licking flames and the cyan ember."""
	z = -0.255
	fy = chest_y(0, z)
	box("FireFrame", (0.118, 0.012, 0.082), m["dark"], body, M((0, fy - 0.006, z)), bev=0.006)
	if opt.get("sprung"):
		box("FireMouth", (0.092, 0.002, 0.064), m["hearth"], body, M((0, fy - 0.0185, z)), bev=0)
		rng = np.random.default_rng(4)
		for k in range(10):
			bm = bmesh.new()
			bmesh.ops.create_icosphere(bm, subdivisions=1, radius=0.012 + 0.01 * rng.random())
			obj("Coal", bm, m["fire"] if k < 3 else m["hearth"], body, M((-0.075 + 0.15 * rng.random(), fy - 0.022, z - 0.05 + 0.02 * rng.random())), smooth=False)
		for k in range(9):
			x, H = -0.075 + 0.15 * k / 8 + 0.008 * rng.normal(), 0.05 + 0.06 * rng.random()
			bm = bmesh.new()
			bmesh.ops.create_cone(bm, cap_ends=True, segments=12, radius1=0.016, radius2=0.002, depth=H)
			obj("Flame", bm, flame_mat(H), body, M((x, fy - 0.026, z - 0.045 + H / 2), (rng.normal() * 6, rng.normal() * 12, 0), (1, 0.45, 1)))
		bm = bmesh.new()
		bmesh.ops.create_icosphere(bm, subdivisions=1, radius=0.027)
		bm.transform(Matrix.Diagonal((0.8, 0.7, 1.25, 1)))   # a shard of ember, not a ball
		obj("Ember", bm, m["ember"], body, M((0.012, fy - 0.036, z + 0.012), (10, 25, 15)), smooth=False)
		point_light("EmberLight", body, (0.01, fy - 0.09, z + 0.03), opt["fire"] * 0.4, opt["glow"], 0.03)
	hinge = empty("Door", (-0.09, fy - 0.022, z), (0, 0, -112 if opt.get("sprung") else 0), body)
	box("FireDoor", (0.09, 0.007, 0.064), m["soot"] if opt.get("sprung") else m["iron"], hinge, M((0.09, 0, 0)), bev=0.005)
	for k in range(5):
		box("Louvre", (0.0065, 0.0015, 0.026), m["fire"], hinge, M((0.045 + 0.022 * k, -0.007, 0.012)), bev=0)
	cyl("Knob", 0.011, 0.016, m["brass"], hinge, M((0.158, -0.012, -0.02), (90, 0, 0)), segs=16)
	for zz in (0.04, -0.04):
		cyl("Hinge", 0.008, 0.03, m["brass"], hinge, M((0.0, -0.004, zz)), segs=12)
	pts = [(x, -0.008, zz) for x in (0.012, 0.168) for zz in np.linspace(-0.05, 0.05, 4)]
	rivets("DoorRivets", pts, [(0, -1, 0)] * len(pts), 0.0042, m["brass"], hinge)
	point_light("FireLight", body, (0.03, fy - 0.16, z + 0.05), opt["fire"], (1.0, 0.48, 0.16), 0.06)

def build_gauge(opt, m, body):
	"""The pressure gauge for a heart, on the left breast: brass case and bezel, the ivory dial, the needle, a pipe."""
	x, z, R = 0.11, -0.068, 0.06
	fy = chest_y(x, z)
	cyl("GaugeCase", R + 0.01, 0.036, m["brass"], body, M((x, fy - 0.012, z), (90, 0, 0)))
	torus("GaugeBezel", R + 0.005, 0.0068, m["brass"], body, M((x, fy - 0.031, z), (90, 0, 0)))
	bm = bmesh.new()
	bmesh.ops.create_circle(bm, cap_ends=True, segments=48, radius=R)
	obj("Dial", bm, dial_mat(opt, R), body, M((x, fy - 0.0305, z), (90, 0, 0)))
	bm = bmesh.new()
	vs = [bm.verts.new((px * R, py * R, 0)) for px, py in ((-0.067, -0.23), (0.067, -0.23), (0.021, 0.885), (-0.021, 0.885))]
	bmesh.ops.solidify(bm, geom=[bm.faces.new(vs)], thickness=0.0015)
	obj("Needle", bm, m["red"], body, M((x, fy - 0.0325, z), (90, 0, 0)) @ Matrix.Rotation(math.radians(-opt["needle"]), 4, "Z"), smooth=False)
	cyl("Hub", 0.0055, 0.006, m["brass"], body, M((x, fy - 0.033, z), (90, 0, 0)), segs=16)
	pipe("GaugePipe", [(x, fy - 0.01, z - R - 0.005), (x + 0.05, fy + 0.004, z - R - 0.02), (x + 0.12, fy + 0.03, z - 0.07)], 0.006, m["brass"], body)

def build_crest(m, body):
	"""Company iron's crest (C): a brass-rimmed shield of red enamel with an eight-spoked driving wheel."""
	x, z = -0.12, -0.075
	fy = chest_y(x, z)
	outline = [(-0.06, 0.06), (0.06, 0.06), (0.06, -0.006), (0.0, -0.075), (-0.06, -0.006)]
	# the brass back stands proud of the boiler and the red enamel proud of the brass, so the rim shows round it
	for grow, front, mat in ((0.008, 0.003, m["brass"]), (0.0, 0.008, enamel_mat((0.62, 0.06, 0.04), ID["coat"], "RedEnamel"))):
		bm = bmesh.new()
		vs = [bm.verts.new((px * (1 + grow / 0.06), py * (1 + grow / 0.06), 0)) for px, py in outline]
		bmesh.ops.solidify(bm, geom=[bm.faces.new(vs)], thickness=0.008)
		bevel(obj("Crest", bm, mat, body, M((x, fy - front, z), (90, 0, 0)), smooth=False), 0.0015, 1)
	torus("Wheel", 0.03, 0.004, m["brass"], body, M((x, fy - 0.012, z - 0.004), (90, 0, 0)))
	cyl("WheelHub", 0.008, 0.006, m["brass"], body, M((x, fy - 0.012, z - 0.004), (90, 0, 0)), segs=16)
	for k in range(4):
		box("Spoke", (0.03, 0.002, 0.0022), m["brass"], body, M((x, fy - 0.012, z - 0.004), (0, 45 * k, 0)), bev=0)

def build_hand(m, body, wrist, aim, back, curl):
	"""A three-fingered riveted fist (two fingers and a thumb, block segments on brass hinge pins) on a banded forearm,
	half again a man's size. Built with the fingers along +z and the back of the hand toward -y, then turned so the
	fingers point along aim and the back of the hand faces back."""
	z = Vector(aim).normalized()
	y = -(Vector(back) - z * z.dot(Vector(back))).normalized()
	hand = empty("Hand", wrist, (0, 0, 0), body)
	hand.matrix_basis = Matrix.Translation(wrist) @ Matrix((y.cross(z), y, z)).transposed().to_4x4() @ Matrix.Scale(1.35, 4)
	box("Palm", (0.056, 0.034, 0.05), m["plate"], hand, M((0, 0, 0.05)), taper=(1.0, 0.85), bev=0.014)
	rivets("HandRivets", [(x, -0.035, zz) for x in (-0.03, 0.03) for zz in (0.025, 0.07)], [(0, -1, 0)] * 4, 0.0055, m["brass"], hand)
	for x, lens, thumb in ((-0.028, (0.046, 0.04, 0.032), False), (0.028, (0.046, 0.04, 0.032), False), (0.06, (0.042, 0.036), True)):
		j = M((x, 0, 0.035), (0, 50, 0)) if thumb else M((x, 0, 0.1))
		for L in lens:
			j = j @ Matrix.Rotation(math.radians(-curl * (0.7 if thumb else 1.0)), 4, "X")
			cyl("Pin", 0.011, 0.054, m["brass"], hand, j @ M((0, 0, 0), (0, 90, 0)), segs=12)
			box("Finger", (0.024, 0.022, L / 2), m["plate"], hand, j @ M((0, 0, L / 2)), taper=(0.9, 0.9), bev=0.008)
			j = j @ Matrix.Translation((0, 0, L))
	cyl("Forearm", 0.05, 0.26, m["iron"], hand, M((0, 0.005, -0.12)), r2=0.06)
	for zz in (-0.02, -0.16):
		torus("Cuff", 0.052 - zz * 0.06, 0.008, m["brass"], hand, M((0, 0.005, zz)))

def build_body(opt, m, body):
	"""The boiler chest with its band, seams and letters (or crest), the gauge, the firebox; the collar the head sits
	in; the drum across the back; pauldrons, stacks, the whistle on the shoulder and the hand."""
	P, B = m["plate"], m["brass"]
	grid("Boiler", [superellipse(rx, ry, 0.0, cy, z, 72, 3.2) for z, rx, ry, cy in TORSO], P, body, cap_top=(0.0, TORSO[-1][3], TORSO[-1][0] + 0.004))
	band("Band", -0.15, 0.017, B, body)
	pts = [(s * 0.2, chest_y(0.2, z) - 0.003, z) for s in (-1, 1) for z in list(np.linspace(-0.03, -0.13, 4)) + list(np.linspace(-0.21, -0.41, 6))]
	rivets("SeamRivets", pts, [(p[0] * 2, -1, 0) for p in pts], 0.005, B, body)   # the plates' seams down the chest
	cyl("Collar", 0.102, 0.07, m["iron"], body, M((0, 0.01, 0.0)))
	torus("CollarRing", 0.103, 0.008, B, body, M((0, 0.01, 0.032)))
	torus("NeckSeam", 0.088, 0.006, m["mouth"], body, M((0, 0.01, 0.04)))   # the furnace's light in the gap under the head
	cyl("Drum", 0.11, 0.44, P, body, M((0, 0.16, -0.035), (0, 90, 0)))
	for s in (-1, 1):
		cyl("NeckPiston", 0.012, 0.07, B, body, M((s * 0.07, 0.03, 0.05)), segs=16)
		torus("DrumBand", 0.112, 0.009, B, body, M((s * 0.2, 0.16, -0.035), (0, 90, 0)))
		build_pauldron(opt, m, body, s, opt.get("torn") and s == 1)
		build_stack(opt, m, body, s, opt.get("bent") and s == -1)
	if opt.get("letters"):
		x, z = -0.1, -0.09   # across the right breast, opposite the gauge
		ob = text("Letters", opt["letters"], 0.056, paint_mat(opt), body, M((x, chest_y(x, z) - 0.0012, z), (90, 0, 0)), 0.0006)
		ob.data.transform(ob.matrix_basis)
		ob.matrix_basis = Matrix()
		for v in ob.data.vertices:   # bent round the boiler, or its ends sink in or stand off
			v.co.y += chest_y(v.co.x, v.co.z) - chest_y(x, z)
	if opt.get("crest"):
		build_crest(m, body)
	build_gauge(opt, m, body)
	build_firebox(opt, m, body)
	# the steam whistle on the shoulder: a brass bell on a stand-pipe, its slit, a pull lever
	x, y = 0.15, 0.10
	cyl("WhistlePipe", 0.011, 0.06, B, body, M((x, y, 0.0)), segs=16)
	cyl("WhistleBell", 0.022, 0.075, B, body, M((x, y, 0.072)))
	cyl("WhistleSlit", 0.0225, 0.007, m["soot"], body, M((x, y, 0.044)), bev=0)
	dome("WhistleCap", (0.022, 0.022, 0.016), B, body, M((x, y, 0.1095)), thick=0.004)
	box("Lever", (0.03, 0.004, 0.004), m["iron"], body, M((x + 0.03, y, 0.02), (0, -20, 0)), bev=0.002)
	if opt.get("hand"):
		build_hand(m, body, *opt["hand"])

# ---------------------------------------------------------------- the bust

def stand_in():
	"""What bust.stage reads of a MakeHuman human: the head bone's head (MakeHuman decimetres, +y up, +z forward) under
	its posed transform. Gauge's is the neck pivot."""
	return {"glob": {"head": np.eye(4)}, "bones": {"head": {"head": np.array([ANCHOR.x * 10, ANCHOR.z * 10, -ANCHOR.y * 10])}}}

def build(opt):
	"""Gauge's bust for one option on an empty scene. Returns (h, eyes) for bust.run: the stand-in and no eyes."""
	m = materials(opt)
	body = empty("Body", ANCHOR, (opt["lean"], 0, opt["body"]))   # leant in over the camera: it looms
	head = empty("Head", ANCHOR, (opt["pitch"], 0, opt["yaw"]))
	# the frame is Ellis's (0.6 m at 2.1 m): Gauge is scaled into it so the head reads at about Ellis's size and the
	# pauldrons, stacks, firebox and hand fit round it inside the portrait's window
	body.scale = head.scale = (opt["scale"],) * 3
	build_body(opt, m, body)
	build_head(opt, m, head)
	return stand_in(), []

def render_aux_glow(path, samples=4):
	"""bust.render_aux, but the glow mask is each material's "looter_glow" (eyes 1 by default), so the lamp, ember and
	fire bloom, and objects with a "looter_aux_skip" material (the steam) leave the pass."""
	sc = bpy.context.scene
	keep = (sc.cycles.samples, sc.cycles.use_denoising, sc.view_settings.view_transform, sc.view_settings.look)
	sc.cycles.samples, sc.cycles.use_denoising, sc.view_settings.view_transform, sc.view_settings.look = samples, False, "Raw", "None"  # Raw as bust.render_aux: Standard sRGB-encoded the ids
	swaps, hidden = [], [o for o in sc.objects if o.type == "LIGHT"]
	for ob in sc.objects:
		for slot in (ob.material_slots if ob.type in ("MESH", "CURVES", "CURVE") else []):
			mt = slot.material
			if mt is not None and mt.get("looter_aux_skip"):
				hidden.append(ob)
			elif mt is not None:
				pid = mt.get("looter_id", 0)
				glow = mt.get("looter_glow", 1.0 if pid == ID["eye"] else 0.0)
				swaps.append((slot, mt))
				slot.material = material(mt.name + ".aux", pid, lambda nt: emission(nt, (glow, 0.0, pid / 16.0, 1.0), 1.0))
	for o in hidden:
		o.hide_render = True
	bg = sc.world.node_tree.nodes["Background"].inputs["Strength"]
	strength, bg.default_value = bg.default_value, 0.0
	bust.render(path)
	for slot, mt in swaps:
		slot.material = mt
	for o in hidden:
		o.hide_render = False
	bg.default_value = strength
	sc.cycles.samples, sc.cycles.use_denoising, sc.view_settings.view_transform, sc.view_settings.look = keep

def main():
	bust.render_aux = render_aux_glow   # run() looks it up in bust when it calls it
	bust.run(OPTIONS, "Gauge", build)

if __name__ == "__main__":
	main()
