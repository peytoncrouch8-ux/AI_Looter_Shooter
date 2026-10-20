"""The kit the hero portrait scripts share beyond bust.py's stage (Docs/Pipeline.md, "Character portraits"): bust.py is the
machinery (body, eyes, base materials, hair curves, drape, lights, render, the runner); this is what heroes built for
themselves and now share: shader shorthands, props and primitives, wires and chains, hair tufts, posing hooks. A hero
script imports from both and keeps only its look: FACE, MACRO, OPTIONS, the "# PAINT" lines and its costume builders.
Nothing here reads a hero's options.
"""
import json
import math
import os
import sys

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Euler, Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mh  # noqa: E402
from bust import (  # noqa: E402
	ID, add_mod, brass_material, bump, build_human, bvh_of, clump, curves_object, frame2d, grid_object, grow, grow_combed,
	leather_material, lin, link, material, mesh_object, mix_rgb, noise, principled, strip_on, surface_point)

GLASS, WOOD = ID["glass"], ID["wood"]   # lenses, goggle glass, a gauge's face; tripod legs, a pencil
C_ROT = np.array([[1.0, 0, 0], [0, 0, -1.0], [0, 1.0, 0]])   # MakeHuman's axes -> Blender's (mh.to_blender unscaled)


# ---------------------------------------------------------------- shader shorthands

def nd(nt, kind, ins=(), op=None):
	"""A shader node with its inputs set or linked: ins maps an input's name or index to a value or an output socket."""
	n = nt.nodes.new(kind)
	if op:
		n.operation = op
	for k, v in dict(ins).items():
		if isinstance(v, bpy.types.NodeSocket):
			nt.links.new(v, n.inputs[k])
		else:
			n.inputs[k].default_value = v
	return n


def ramp(nt, fac, stops):
	"""A colour ramp on fac through (position, colour) stops, in order; a colour is sRGB (3 values) or linear RGBA (4)."""
	r = nd(nt, "ShaderNodeValToRGB", {0: fac})
	el = r.color_ramp.elements
	for i, (p, c) in enumerate(stops):
		e = el[i] if i < 2 else el.new(p)
		e.position, e.color = p, (*lin(c), 1) if len(c) == 3 else c
	return r.outputs[0]


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


# ---------------------------------------------------------------- material makers

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


def emit_material(name, color, strength, pid):
	def build(nt):
		em = nt.nodes.new("ShaderNodeEmission")
		em.inputs["Color"].default_value = (*lin(color), 1)
		em.inputs["Strength"].default_value = strength
		return em
	return material(name, pid, build)


def living_eye(name, stops, spec=0.7, **kw):
	"""An eye as in life: no glow. Skin's id, not the eye's, so paint.js keeps it crisp while the aux pass (which blooms
	every eye-id pixel) leaves it out. stops: (position, colour) on the eyeball's z / r (-1..1; its +z looks at the camera)
	as ramp() takes them; kw: more Principled inputs (a clear coat)."""
	def build(nt):
		sep = nt.nodes.new("ShaderNodeSeparateXYZ")
		nt.links.new(nt.nodes.new("ShaderNodeTexCoord").outputs["Object"], sep.inputs[0])
		zr = nt.nodes.new("ShaderNodeMath")
		zr.operation = "DIVIDE"
		nt.links.new(sep.outputs["Z"], zr.inputs[0])
		zr.inputs[1].default_value = 0.0118
		b = principled(nt, ramp(nt, zr.outputs[0], stops), rough=0.08, spec=spec)
		for k, v in kw.items():
			b.inputs[k].default_value = v
		return b
	return material(name, ID["skin"], build)


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


# ---------------------------------------------------------------- places on the posed head and body

def head_matrix(h):
	"""Head-local metres in MakeHuman's axes (x the character's left, y up, z out of the face) -> world, as posed."""
	G = h["glob"]["head"]
	M = np.eye(4)
	M[:3, :3], M[:3, 3] = C_ROT @ G[:3, :3], 0.1 * C_ROT @ G[:3, 3]
	return Matrix(M.tolist())


def head_world(h, p):
	"""Rest head-space points (MakeHuman decimetres) -> world, as posed."""
	return mh.to_blender(mh.apply(h["glob"]["head"], np.atleast_2d(p)))


def head_pt(h, p):
	"""One rest head-space point (decimetres) -> posed world (metres), a Vector."""
	return Vector(mh.to_blender(mh.apply(h["glob"]["head"], np.asarray(p, dtype=np.float64)[None]))[0])


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


def body_ix(h):
	return np.unique(np.concatenate(h["group_faces"]("body")))


def body_co(body):
	co = np.empty(len(body.data.vertices) * 3)
	body.data.vertices.foreach_get("co", co)
	return co.reshape(-1, 3)


def set_body_co(body, co):
	body.data.vertices.foreach_set("co", co.ravel())
	body.data.update()


# ---------------------------------------------------------------- objects from bmesh, wires, cloth

def bm_object(name, bm, mat=None, smooth=True, parent=None, at=None):
	"""bm as a linked object (bm is freed). mat: one material, a list of them, or None. parent: an object to hang it
	under; at: the object's own transform (its Object coordinates stay local: lens, dial and flame materials need that),
	else the mesh is already in the parent's space."""
	me = bpy.data.meshes.new(name)
	bm.to_mesh(me)
	bm.free()
	if smooth:
		me.shade_smooth()
	ob = link(bpy.data.objects.new(name, me))
	ob.parent = parent
	if at is not None:
		ob.matrix_basis = at
	for m in (mat if isinstance(mat, (list, tuple)) else [mat]):
		if m is not None:
			ob.data.materials.append(m)
	return ob


def cloth(ob, thick, mat, offset=1.0):
	"""Thickness, smoothing and a material: what every sheet of cloth gets."""
	add_mod(ob, "SOLIDIFY", thickness=thick, offset=offset)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	ob.data.materials.append(mat)
	return ob


def ball(name, at, r, mat, scale=(1, 1, 1), normal=None):
	"""A uv sphere at at, scaled; normal turns its z along a surface normal."""
	bm = bmesh.new()
	bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=10, radius=r)
	ob = bm_object(name, bm, mat)
	ob.location, ob.scale = Vector(tuple(at)), scale
	if normal is not None:
		ob.rotation_euler = normal.to_track_quat("Z", "Y").to_euler()
	return ob


def add_torus(bm, R, r, m, nu=14, nv=6):
	"""A torus (ring R, tube r) into bm, placed by the 4x4 m: bmesh has no torus primitive."""
	vs = [bm.verts.new(m @ Vector(((R + r * math.cos(v)) * math.cos(u), (R + r * math.cos(v)) * math.sin(u), r * math.sin(v))))
		for u in np.linspace(0, 2 * math.pi, nu, endpoint=False) for v in np.linspace(0, 2 * math.pi, nv, endpoint=False)]
	for i in range(nu):
		for j in range(nv):
			i2, j2 = (i + 1) % nu, (j + 1) % nv
			bm.faces.new((vs[i * nv + j], vs[i2 * nv + j], vs[i2 * nv + j2], vs[i * nv + j2]))


def frustum(name, p0, p1, r0, r1, mat):
	"""An open tapered tube from p0 to p1 (sleeves, cuffs)."""
	bm = bmesh.new()
	d = p1 - p0
	m = Matrix.Translation((p0 + p1) * 0.5) @ d.to_track_quat("Z", "Y").to_matrix().to_4x4()
	bmesh.ops.create_cone(bm, cap_ends=False, segments=24, radius1=r0, radius2=r1, depth=d.length, matrix=m)
	return cloth(bm_object(name, bm), 0.003, mat)


def ring_band(name, c, rows, mat, thick=0.002):
	"""A band round a vertical axis at c (x, y): rows of (radius(theta), z(theta)) functions, theta 0 at the front."""
	th = np.linspace(0, 2 * math.pi, 72, endpoint=False)
	pts = np.array([[c[0] + math.sin(t) * r(t), c[1] - math.cos(t) * r(t), z(t)] for r, z in rows for t in th])
	n = len(rows)
	return cloth(grid_object(name, pts[:, 0].reshape(n, -1), pts[:, 1].reshape(n, -1), pts[:, 2].reshape(n, -1), wrap=True), thick, mat)


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


def tube_data(name, pts, radius, mat, cyclic=False):
	"""A round wire, strap or rope along a polyline, as bevelled curve data (many objects can share it). radius: one
	value, or one per point."""
	per_point = not np.isscalar(radius)
	cu = bpy.data.curves.new(name, "CURVE")
	cu.dimensions, cu.bevel_depth, cu.bevel_resolution, cu.use_fill_caps = "3D", 1.0 if per_point else radius, 3, True
	sp = cu.splines.new("POLY")
	sp.points.add(len(pts) - 1)
	sp.use_cyclic_u = cyclic
	for k, (p, q) in enumerate(zip(sp.points, pts)):
		p.co = (*q, 1.0)
		if per_point:
			p.radius = radius[k]
	cu.materials.append(mat)
	return cu


def tube(name, pts, radius, mat, cyclic=False):
	"""tube_data as an object."""
	return link(bpy.data.objects.new(name, tube_data(name, pts, radius, mat, cyclic)))


def ring_pts(c, e1, e2, r, n=24, a0=0.0):
	return [c + (e1 * math.cos(t) + e2 * math.sin(t)) * r for t in a0 + np.linspace(0, 2 * math.pi, n, endpoint=False)]


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


def edit_faces(ob, doomed, prepare=None):
	"""Delete the faces of ob whose centre doomed(centre) says to (prepare(bm) may reshape the mesh first, e.g. cut it
	finer); returns the cut edge's points. A garment's tear is this with a ragged outline."""
	bm = bmesh.new()
	bm.from_mesh(ob.data)
	if prepare:
		prepare(bm)
	bmesh.ops.delete(bm, geom=[f for f in bm.faces if doomed(f.calc_center_median())], context="FACES")
	edge = [v.co.copy() for v in bm.verts if v.is_boundary]
	bm.to_mesh(ob.data)
	bm.free()
	return edge


def point_light(name, loc, power, color, radius=0.004, parent=None):
	ld = bpy.data.lights.new(name, "POINT")
	ld.energy, ld.color, ld.shadow_soft_size = power, lin(color), radius
	lo = link(bpy.data.objects.new(name, ld))
	lo.parent, lo.location = parent, loc
	return lo


# ---------------------------------------------------------------- chains

def link_shape(mat, straight=0.012, rr=0.0075, wire=0.0034):
	"""One chain link: a stadium in the xy plane, long axis y (curve data for many objects to share)."""
	u = np.linspace(0, 2 * math.pi, 32, endpoint=False)
	c = np.stack([rr * np.cos(u), rr * np.sin(u) + straight * 0.5 * np.clip(np.sin(u) * 3, -1, 1), np.zeros(32)], 1)
	return tube_data("Link", c, [wire] * 32, mat, True)


def link_chain(name, path, normals, mat, pitch=0.021):
	"""Links every pitch (a link's inner length) along a path, each a quarter turn from the last, flat ones on the surface;
	one object per link, sharing one curve."""
	path, normals = np.asarray(path, float), np.asarray(normals, float)
	s = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(path, axis=0), axis=1))])
	q = np.arange(0, s[-1], pitch)
	P, Nn = (np.stack([np.interp(q, s, a[:, i]) for i in range(3)], 1) for a in (path, normals))
	me = link_shape(mat)
	out = []
	for k in range(len(P) - 1):
		t = Vector(P[k + 1] - P[k]).normalized()
		z = Vector(Nn[k])
		z = (z - t * z.dot(t)).normalized()
		x = t.cross(z)
		if k % 2:
			x, z = z, -x
		ob = link(bpy.data.objects.new(name, me))
		ob.matrix_world = Matrix.Translation(Vector((P[k] + P[k + 1]) * 0.5)) @ Matrix((x, t, z)).transposed().to_4x4()
		out.append(ob)
	return out


def mesh_chain(name, pts, nrm, mat, pitch=0.022, L=0.03, w=0.011, r=0.0017):
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


# ---------------------------------------------------------------- hair tufts

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


# ---------------------------------------------------------------- posing: a stoop, a turned bust, a raised arm

def stooped(opt, face, macro, name):
	"""bust.build_human with the upper spine bent forward by opt["stoop"] and the shoulders hunched (mh.build wrapped)."""
	s = opt.get("stoop", 0.0)
	extra = {"spine02": ((1, 0, 0), s * 0.4), "spine01": ((1, 0, 0), s * 0.6), "clavicle.L": ((0, 0, 1), 3.0), "clavicle.R": ((0, 0, 1), -3.0)}
	orig = mh.build
	mh.build = lambda macro=None, face=None, pose=None: orig(macro=macro, face=face, pose={**(pose or {}), **extra})
	try:
		return build_human(opt, face, macro, name)
	finally:
		mh.build = orig


def face_counts_as_head(h):
	"""shell() keeps the jaw (its own bone) once a stoop drops it below a garment's top: "head" now counts every bone
	under the head."""
	wts = json.load(open(os.path.join(mh.DATA, "rigs", "default_weights.mhw")))["weights"]
	under = lambda b: b == "head" or (h["bones"][b]["parent"] is not None and under(h["bones"][b]["parent"]))
	acc = np.zeros(len(h["verts"]))
	for b in filter(under, h["bones"]):
		for i, x in wts.get(b, []):
			acc[i] += x
	orig = h["weights"]
	h["weights"] = lambda bone: np.minimum(acc, 1.0) if bone == "head" else orig(bone)


def turn_scene(h, deg):
	"""Turn everything about the vertical through the head bone (the camera's target): a stoop shows in three-quarter."""
	p = mh.to_blender(mh.apply(h["glob"]["head"], h["bones"]["head"]["head"]))
	M = Matrix.Translation((p[0], p[1], 0)) @ Matrix.Rotation(math.radians(deg), 4, "Z") @ Matrix.Translation((-p[0], -p[1], 0))
	for ob in bpy.context.scene.objects:
		ob.matrix_world = M @ ob.matrix_world


def axis_between(a, b):
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
	ax1, d1 = axis_between(u0, Rc.T @ (el - sh))
	out = {"upperarm01.R": (ax1, d1), "lowerarm01.R": axis_between(f0, (Rc @ mh._axis_angle(ax1, d1)).T @ (wr - el)),
		"lowerarm02.R": (f0 / np.linalg.norm(f0), twist)}
	across = b("finger5-1.R") - b("finger2-1.R")
	across /= np.linalg.norm(across)
	for f in (2, 3, 4, 5):
		for j in (1, 2, 3):
			out[f"finger{f}-{j}.R"] = (across, curl * (0.8 if j == 1 else 1.0))
	tax = np.cross(b("finger1-3.R") - b("finger1-1.R"), across)
	out["finger1-2.R"] = out["finger1-3.R"] = (tax / np.linalg.norm(tax), thumb)
	return out


ARM = {}   # extra pose for the next build_human: set it (arm_pose's elbow, wrist ...) before, clear it after
_skin0 = mh._skin


def _skin_with_arm(v, bones, pose):
	"""build_human poses only the arms down and the head; mh.build calls this with the morphed joints, where the raised
	arm can be worked out."""
	if ARM:
		pose = dict(pose)
		pose.update(arm_pose(bones, pose, **ARM))
	return _skin0(v, bones, pose)


def use_arm_pose():
	"""Hook ARM into mh.build (a hero that raises an arm calls this once, at import)."""
	mh._skin = _skin_with_arm


def handless(body, h):
	"""The body without its right hand's faces, for garment shells, so the hand comes bare out of the sleeve. Only
	faces go: the vertices stay, so shell()'s per-vertex weights still line up."""
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


# ---------------------------------------------------------------- primitives under a parent Empty (Gauge's iron)

def M(loc=(0, 0, 0), rot=(0, 0, 0), scale=None):
	"""A transform: an optional scale, then rotation in degrees (XYZ), then placed at loc."""
	m = Matrix.Translation(loc) @ Euler([math.radians(a) for a in rot], "XYZ").to_matrix().to_4x4()
	return m @ Matrix.Diagonal((*scale, 1)) if scale else m


def empty(name, loc, rot=(0, 0, 0), parent=None):
	e = link(bpy.data.objects.new(name, None))
	e.location, e.rotation_euler, e.parent = loc, Euler([math.radians(a) for a in rot], "XYZ"), parent
	return e


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
	ob = bm_object(name, bm, mat, True, parent)
	return bevel(ob, bev) if bev else ob


def cyl(name, r, depth, mat, parent, at=None, segs=40, r2=None, bev=0.003, caps=True):
	"""A cylinder (or cone, r2) along its local z, centred."""
	bm = bmesh.new()
	bmesh.ops.create_cone(bm, cap_ends=caps, cap_tris=False, segments=segs, radius1=r, radius2=r if r2 is None else r2,
		depth=depth, matrix=at or Matrix())
	ob = bm_object(name, bm, mat, True, parent)
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
	return bm_object(name, bm, mat, True, parent)


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
	ob = bm_object(name, bm, mat, True, parent)
	add_mod(ob, "SOLIDIFY", thickness=thick, offset=-1.0)
	add_mod(ob, "SUBSURF", levels=1, render_levels=2)
	return ob


def rivets(name, pts, nrms, r, mat, parent):
	"""Round-headed rivets: one mesh of squashed spheres, each standing on its normal."""
	bm = bmesh.new()
	for p, n in zip(pts, nrms):
		q = Vector(n).normalized().to_track_quat("Z", "Y").to_matrix().to_4x4()
		bmesh.ops.create_uvsphere(bm, u_segments=10, v_segments=6, radius=r, matrix=Matrix.Translation(Vector(p)) @ q @ Matrix.Diagonal((1, 1, 0.6, 1)))
	return bm_object(name, bm, mat, True, parent)


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
	return bevel(bm_object(name, bm, mat, True, parent), 0.0015, 1)


# ---------------------------------------------------------------- garment parts and props (on shell() and strip_on())

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
