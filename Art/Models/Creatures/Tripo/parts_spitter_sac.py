"""The Venom Spitter from Tripo: the cephalothorax's and the sac's shells as parts, and what grows on the sac. Not a model
(Tools\\models.ps1 skips parts_*.py); Spitter.py runs it.

Tripo painted the sac opaque lime with brown vein ridges and a brown collar of tongues where it grows out of the body.
The game's sac is the Gel master (MI_SpitterSac: see-through facing the eye, dense at its edges, its glow driven by
ASpitterSpider), which takes no texture, so what was paint is built:
  - the gel shell (the sac bone; SAC_GRID cube-sphere on Tripo's sac shape),
  - a venom core inside it (darker, opaque, CORE_SHARE of its size) that the gel is seen over,
  - veins: a Voronoi net on the sphere (VEIN_SEEDS cells, like Tripo's big cells), half-sunk tubes along its edges,
  - the collar: a hide slab over the sac's mouth, its edge drawn into COLLAR_TONGUES pointed tongues reaching up the
    sac, thick at the mouth and thin at the points.
All on the sac bone (they swell with it), painted in parts_spitter_look.py.
"""
import math

import bmesh
import bpy
import numpy as np

import parts_spitter as ps
from parts_spitter_limbs import Part

CORE_SHARE = 0.8
CORE_GRID = 8
VEIN_SEEDS = 14
VEIN_SEGMENTS = 7
VEIN_SIDES = 5
VEIN_RADIUS = (0.5, 0.32)          # Tripo units, at a cell corner and mid-edge (the sac is ~17 units round)
COLLAR_TONGUES = 9
COLLAR_BASE = 20.0                 # degrees from the mouth the collar covers between tongues
COLLAR_TONGUE = 26.0               # degrees more at a tongue's point
COLLAR_SHARP = 3.0                 # how pointed the tongues are (the power on their profile)
VEIN_WANDER = 0.05                 # radians a vein strays off its straight arc (organic, not a ball's seams)
COLLAR_THICK = 0.55                # units at the mouth (0.12 at the points: clear of the gel)
COLLAR_ROWS, COLLAR_COLS = 8, 45
HEAD_SPLIT_X = 22.0                # cm: the shell forward of this is the head bone's (eyes, chelicerae, face)


def cube_uvs(n):
    """Each cube-sphere quad's corner UVs (cube_sphere's order), its six faces laid 3 x 2 over 0..1."""
    out = []
    face = 0
    for axis in range(3):
        for sign in (-1.0, 1.0):
            col, row = face % 3, face // 3
            for i in range(n):
                for j in range(n):
                    corners = [(i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)]
                    if not ((sign > 0) == (axis != 1)):
                        corners = corners[::-1]
                    out.append([((col + a / n) / 3.0, (row + b / n) / 2.0) for a, b in corners])
            face += 1
    return out


def shell_part(name, bone_of, region, look, final, template, quads, n):
    part = Part(name, bone_of, region, look)
    for f, t in zip(final, template if template is not None else [None] * len(final)):
        part.vert(f, t)
    for q, uv in zip(quads, cube_uvs(n)):
        part.face(q, uv)
    return part


def body_parts(m):
    """The cephalothorax shell, split by HEAD_SPLIT_X into the head's part and the body's (both rigid: neither moves
    alone)."""
    shell, place = m['body'], m['place']
    template = ps.shell_points(shell)
    final = place.body(template)
    whole = shell_part('Carapace', 'body', 'body', 'bake', final, template, shell['quads'], ps.BODY_GRID)
    head = Part('Head', 'head', 'body')
    body = Part('Carapace', 'body', 'body')
    for part, keep in ((head, lambda x: x >= HEAD_SPLIT_X), (body, lambda x: x < HEAD_SPLIT_X)):
        part.V, part.T = whole.V, whole.T
        for f, uv in zip(whole.faces, whole.uv):
            if keep(np.mean([whole.V[i][0] for i in f])):
                part.face(f, uv)
    return {'head': [head], 'body': [body]}


def sac_radius(sac, dirs):
    """The sac's radius along unit directions (Tripo space): the three nearest cube-sphere rays, weighted."""
    dots = dirs @ sac['dirs'].T
    near = np.argsort(-dots, axis=1)[:, :3]
    w = np.take_along_axis(dots, near, axis=1)
    w = np.maximum(w - w[:, 2:3] + 1e-4, 0.0) ** 2
    return np.sum(sac['radius'][near] * w, axis=1) / np.sum(w, axis=1)


def _slerp(a, b, t):
    om = math.acos(max(-1.0, min(1.0, float(np.dot(a, b)))))
    if om < 1e-6:
        return a
    return (math.sin((1.0 - t) * om) * a + math.sin(t * om) * b) / math.sin(om)


def _fibonacci(n, jitter=0.18, seed=5):
    rng = np.random.default_rng(seed)
    pts = []
    for k in range(n):
        z = 1.0 - 2.0 * (k + 0.5) / n
        a = k * math.pi * (3.0 - math.sqrt(5.0))
        r = math.sqrt(1.0 - z * z)
        p = np.array([r * math.cos(a), r * math.sin(a), z]) + rng.normal(0.0, jitter, 3)
        pts.append(p / np.linalg.norm(p))
    return np.array(pts)


def voronoi_edges(seeds):
    """The spherical Voronoi net of unit seeds: [(a, b)] unit-vector pairs (each cell edge's ends)."""
    bm = bmesh.new()
    for p in seeds:
        bm.verts.new(p)
    bmesh.ops.convex_hull(bm, input=bm.verts)
    bm.faces.ensure_lookup_table()
    centre = {}
    for f in bm.faces:
        n = np.array(f.normal)
        c = np.mean([np.array(v.co) for v in f.verts], axis=0)
        centre[f.index] = n if np.dot(n, c) > 0 else -n
    edges = []
    for e in bm.edges:
        if len(e.link_faces) == 2:
            a, b = e.link_faces
            edges.append((centre[a.index], centre[b.index]))
    bm.free()
    return edges


def sac_parts(m):
    """{'sac': [shell (gel), core, veins, collar]}."""
    sac, place = m['sac'], m['place']
    c = sac['centre']
    shell_t = ps.shell_points(sac)
    shell = shell_part('Sac', 'sac', 'gel', 'gel', place.sac(shell_t), None, sac['quads'], ps.SAC_GRID)
    cdirs, cquads, _adj = ps.cube_sphere(CORE_GRID)
    core_t = c + cdirs * (sac_radius(sac, cdirs) * CORE_SHARE)[:, None]
    core = shell_part('SacCore', 'sac', 'core', 'core', place.sac(core_t), None, cquads, CORE_GRID)
    return {'sac': [shell, core, veins(sac, place), collar(m), pedicel(m)]}


def pedicel(m):
    """The waist between the body and the sac: a short tapered tube from inside the carapace's back into the sac's
    mouth, hide-coloured like the collar over its end; on the sac bone, which swells and shrinks from its body end."""
    from parts_spitter_limbs import PEDICEL, frames, lathe
    place = m['place']
    into_body = place.body(m['body']['centre']) - place.root_c
    into_sac = place.sac(m['sac']['centre']) - place.mouth_c
    p0 = place.root_c + into_body / np.linalg.norm(into_body) * 12.0
    p1 = place.mouth_c + into_sac / np.linalg.norm(into_sac) * 12.0
    r0, r1, sides = PEDICEL
    ts = np.linspace(0.0, 1.0, 7)
    centres = p0[None, :] + np.outer(ts, p1 - p0)
    radii = (r0 + (r1 - r0) * ts) * (1.0 + 0.12 * np.sin(math.pi * ts))
    rows = (centres, frames(centres, ref=np.array([0.0, 1.0, 0.0])), radii)
    part = Part('Pedicel', 'sac', 'collar', 'collar')
    lathe(part, rows, rows, sides, ts)
    return part


def veins(sac, place):
    """Half-sunk tubes along the Voronoi net's edges, thicker at the cells' corners."""
    part = Part('SacVeins', 'sac', 'vein', 'vein')
    c = sac['centre']
    rng = np.random.default_rng(21)
    for a, b in voronoi_edges(_fibonacci(VEIN_SEEDS)):
        ts = np.linspace(0.0, 1.0, VEIN_SEGMENTS + 1)
        # Each vein strays off its arc in a slow S (zero at the cell corners, where veins meet).
        across = np.cross(a, b)
        across /= max(np.linalg.norm(across), 1e-6)
        wander = VEIN_WANDER * rng.uniform(0.5, 1.0) * rng.choice((-1.0, 1.0))
        bend = wander * np.sin(math.pi * ts) * (1.0 + 0.6 * np.sin(2.0 * math.pi * ts + rng.uniform(0.0, 6.3)))
        dirs = np.array([_slerp(a, b, t) for t in ts]) + np.outer(bend, across)
        dirs /= np.linalg.norm(dirs, axis=1, keepdims=True)
        r = sac_radius(sac, dirs)
        centres = c + dirs * r[:, None]
        thick = VEIN_RADIUS[1] + (VEIN_RADIUS[0] - VEIN_RADIUS[1]) * (2.0 * np.abs(ts - 0.5)) ** 2
        rows = []
        for i, (p, d) in enumerate(zip(centres, dirs)):
            tang = centres[min(i + 1, len(centres) - 1)] - centres[max(i - 1, 0)]
            tang /= np.linalg.norm(tang)
            side = np.cross(tang, d)
            side /= np.linalg.norm(side)
            ring = []
            for j in range(VEIN_SIDES):
                ang = 2.0 * math.pi * j / VEIN_SIDES
                q = p + (d * math.cos(ang) + side * math.sin(ang)) * thick[i]
                ring.append(part.vert(place.sac(q)))
            rows.append(ring)
        for i in range(len(rows) - 1):
            for j in range(VEIN_SIDES):
                j1 = (j + 1) % VEIN_SIDES
                u0, u1 = i / VEIN_SEGMENTS, (i + 1) / VEIN_SEGMENTS
                part.face((rows[i][j], rows[i + 1][j], rows[i + 1][j1], rows[i][j1]),
                          [(u0, j / VEIN_SIDES), (u1, j / VEIN_SIDES), (u1, (j + 1) / VEIN_SIDES), (u0, (j + 1) / VEIN_SIDES)])
    return part


def collar(m):
    """A hide slab over the sac's mouth (round the axis toward the body), its edge in pointed tongues."""
    sac, place = m['sac'], m['place']
    c = sac['centre']
    axis = m['body']['centre'] - c
    axis /= np.linalg.norm(axis)
    e1 = np.cross(axis, [1.0, 0.0, 0.0])
    if np.linalg.norm(e1) < 0.1:
        e1 = np.cross(axis, [0.0, 0.0, 1.0])
    e1 /= np.linalg.norm(e1)
    e2 = np.cross(axis, e1)
    part = Part('SacCollar', 'sac', 'collar', 'collar')
    sheets = []
    for sheet, (off_root, off_tip) in enumerate(((COLLAR_THICK, 0.12), (-0.5, -0.06))):
        grid = []
        for j in range(COLLAR_COLS):
            phi = 2.0 * math.pi * j / COLLAR_COLS
            saw = abs(((phi * COLLAR_TONGUES / (2.0 * math.pi)) % 1.0) - 0.5) * 2.0   # 0 at a tongue's point, 1 between
            reach = math.radians(COLLAR_BASE + COLLAR_TONGUE * (1.0 - saw) ** COLLAR_SHARP)
            col = []
            for k in range(COLLAR_ROWS + 1):
                s = k / COLLAR_ROWS
                th = reach * s
                d = axis * math.cos(th) + (e1 * math.cos(phi) + e2 * math.sin(phi)) * math.sin(th)
                r = float(sac_radius(sac, d[None, :])[0])
                off = off_root + (off_tip - off_root) * s ** 1.5
                col.append(part.vert(place.sac(c + d * (r + off))))
            grid.append(col)
        sheets.append(grid)
    outer, inner = sheets
    for j in range(COLLAR_COLS):
        j1 = (j + 1) % COLLAR_COLS
        v0, v1 = j / COLLAR_COLS, (j + 1) / COLLAR_COLS
        for k in range(COLLAR_ROWS):
            u0, u1 = k / COLLAR_ROWS, (k + 1) / COLLAR_ROWS
            part.face((outer[j][k], outer[j][k + 1], outer[j1][k + 1], outer[j1][k]),
                      [(u0, v0), (u1, v0), (u1, v1), (u0, v1)])
            part.face((inner[j][k], inner[j1][k], inner[j1][k + 1], inner[j][k + 1]),
                      [(u0, v0), (u0, v1), (u1, v1), (u1, v0)])
        # The tongue's edge: outer to inner.
        k = COLLAR_ROWS
        part.face((outer[j][k], inner[j][k], inner[j1][k], outer[j1][k]), [(1.0, v0), (0.98, v0), (0.98, v1), (1.0, v1)])
    return part


def hull_split(sac_part, place, sac):
    """The sac's hit hull is its upper part, the abdomen's its lower (so the belly isn't a critical spot): the final
    shell's vertices above and below the sac's middle (with a little overlap)."""
    centre_z = place.sac(sac['centre'])[2]
    pts = np.array(sac_part.V)
    span = pts[:, 2].max() - pts[:, 2].min()
    return pts[pts[:, 2] > centre_z - 0.2 * span * 0.5], pts[pts[:, 2] < centre_z + 0.1 * span * 0.5]
