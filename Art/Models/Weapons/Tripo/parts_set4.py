"""AR set 4 (Meridian: clean and precise; satin-white and graphite polymer, machined aluminium, fine brass, a cyan line)
from the user's Tripo models: Marksman body, Marksman barrel, Compensator muzzle, 20 magazine, RedDot sight, Cheek
stock. How Bullpup.py fits each (the spec format is in Bullpup.py, "Tripo parts"; these are the function form,
fn(bp, m)). Not a model: Tools\\models.ps1 skips parts_*.py. Helpers: Tools/Blender/looter_tripo_set4.py.

Boxes are the concept's size table (Art/Concepts/Weapons/AR/README.md, set 4; render_set4.js's bounds), gun space in
cm. Every file is muzzle -Y, left +X, up +Z. Colour variations: the white shell is GunPolymerSand_<set> (the gun's
shell paint), the graphite GunPolymerGrey_<set> (its fittings paint), both on paintable copies of the part's texture set;
the brushed aluminium, brass and rubber stay as Tripo painted them. The paint copies are polymer (metal out, roughness
at least 0.45) and tinted their own brightest colour, so unpainted they look as Tripo made them. Element glow: every
Gun*Glow piece below (ember slits, ports and barrel flute lines, the cyan lines, the dot) takes the gun's element colour;
the body's accent line is the rarity's. Maps 2048 (Tripo's size); LODs from Bullpup.py (50/20%).

Checked against the concepts (2026-10-27; previews in Saved/TRIPOFIT/set4/):
- Body: close to the concept. The painted cyan parting line and ember slits are too thin in the atlas to be glow faces,
  so a rarity accent line follows the painted one on both sides and ember inserts sit on the six slits; the nose (the
  barrel nut) sat 0.22 low and is bent up onto the bore; pinholes filled. The aluminium side panels' edges are ragged in
  Tripo's texture (white flecks show under a dark paint).
- Barrel: the three flutes a side are only painted; a glow line now runs in each side's middle flute. The gas block was
  a square tube open on both sides (the barrel showed through): aluminium covers close it. A pinhole filled.
- Muzzle: as drawn; its ember ports and bore become glow faces.
- Magazine: as drawn; the cyan base inlay (painted as triangle teeth) is redrawn as a glow line both sides.
- RedDot: Tripo closed the housing (lens faces at both ends): the windows are cut open, lined, glazed, and the dot is a
  GunReticle on SOCKET_Aim's line (measured: windows' centre v 8.98 at w 0, the concept's 8.95; Aim u 20.27). The
  dented sides are pressed flat; the cyan lines redrawn as glow. The end bezels stay a little ragged (Tripo's).
- Cheek stock: as drawn; slid so its seat plate's face is on the butt plate (u -0.3); the block's cyan line redrawn as
  glow (the line under the riser is too faint to find: left painted).
"""
import looter_tripo as tripo
import looter_tripo_set4 as t4

AXES = dict(forward='-Y', left='+X')


def _cubic(p0, p1, p2, p3, n=10):
    return [tuple((1 - t) ** 3 * a + 3 * (1 - t) ** 2 * t * b + 3 * (1 - t) * t * t * c + t ** 3 * d
                  for a, b, c, d in zip(p0, p1, p2, p3)) for t in (k / n for k in range(1, n + 1))]


def _load(bp, m, box):
    """The part's Tripo model fitted onto box, its source path kept (split() drops it)."""
    tr, obj, maps, mat = bp.tripo_load(m, box, **AXES)
    return obj, maps, mat, obj['TripoFile']


def _paints(bp, obj, maps, mat, source, glows, shell=True, fittings=True, more_fittings=()):
    """The white shell and graphite fittings (faces not already glow; more_fittings: faces to paint with the fittings
    whatever their colour) as paintable slots; returns {material: faces} with the glows, and the materials that keep
    Tripo's UVs."""
    set_name = mat.name[len('Gun'):]
    taken = set(f for faces in glows.values() for f in faces)
    white, grey = t4.shell_and_fittings(obj, maps['BC'], taken)
    more = set(more_fittings) - taken - set(white)
    grey = sorted(set(grey) | more)
    groups, keep = dict(glows), [mat]
    if shell and white:
        p = t4.paint(maps, set_name, bp.SHELL.name, t4.is_white, source, 'Paint')
        groups[p] = white
        keep.append(p)
        taken.update(white)
    if fittings and grey:
        p = t4.paint(maps, set_name, bp.lg.POLY_GREY.name, t4.is_graphite, source, 'Fit')
        groups[p] = grey
        keep.append(p)
        taken.update(grey)
    groups[mat] = [i for i in range(len(obj.data.polygons)) if i not in taken]
    return groups, keep


def _cyan_lines(bp, obj, maps, lines, r=0.06):
    """The painted cyan lines (a few px wide in the atlas, so Tripo's triangles cut them into teeth) redrawn: each
    line's faces give a polyline (lines: [(side, u0, u1, keep)], looter_tripo_set4.line), the faces take the colour
    round them, and a GunCyanGlow rod follows the polyline just proud of the surface. Returns the rods (added after the
    mesh is split)."""
    cyan = tripo.faces_where(obj, maps['BC'], t4.is_cyan_line, corners=False)
    rods, used = [], set()
    for side, u0, u1, keep in lines:
        pts = [(u, v, w + side * 0.025) for u, v, w in t4.line(obj, cyan, side, u0, u1, step=0.5, keep=keep)]
        if len(pts) > 1:
            rods.append(bp.lg.pipe(pts, r, mat=bp.CYAN, sides=6))
        for i in cyan:
            p = tripo.gun_uvw(obj.data.polygons[i].center)
            if p[2] * side > 0.0 and u0 <= p[0] <= u1 and (keep is None or keep(p)):
                used.add(i)
    t4.repaint_like_neighbours(obj, used)
    bp.lt._log(f'Tripo: {len(used)} of {len(cyan)} cyan faces repainted under {len(rods)} glow lines')
    return rods


def body(bp, m):
    lg = bp.lg
    obj, maps, mat, source = _load(bp, m, ((-0.3, bp.BARREL_AT[0]), (-16.1, bp.SIGHT_AT[1]), 5.5))
    # The nose: the nut's face measured 0.22 low against the bore; bend the snout up onto it (from u 44 to 50, behind it
    # the rail and the rest stay).
    for v in obj.data.vertices:
        u = -v.co.y / lg.CM
        k = min(1.0, max(0.0, (u - 44.0) / 6.0))
        v.co.z += 0.22 * k * k * (3 - 2 * k) * lg.CM
    obj.data.update()
    t4.fill_holes(obj)
    bvh = t4.tree(obj)
    groups, keep = _paints(bp, obj, maps, mat, source, {})
    t4.add(bp, m, obj, groups, keep)
    # The accent line along the parting line (render_set4.js ACC4, the painted cyan line), both sides, just proud.
    acc = [(0.8, -2.2)] + _cubic((0.8, -2.2), (8, -2.0), (15, -3.2), (23, -3.0)) + \
        _cubic((23, -3.0), (31, -2.9), (41, -1.1), (50.8, -0.1))
    for side in (-1.0, 1.0):
        pts = []
        for u, v in acc:
            w = t4.surface_w(bvh, u, v, side)
            if w is not None:
                pts.append((u, v, w + side * 0.02))
        if len(pts) > 2:
            bp.g.add(m, lg.pipe(pts, 0.07, mat=bp.ACCENT, sides=6))
        # Six ember inserts on the nose's heat slits (render_set4.js: u 40.2 + 1.18 i, raked).
        for i in range(6):
            u, k = 40.2 + i * 1.18, i * 0.04
            w = t4.surface_w(bvh, u + 0.33, 1.12, side)
            if w is not None:
                bp.g.add(m, lg.slab([(u + 0.06, 0.62 + k), (u + 0.32, 0.63 + k), (u + 0.6, 1.64 - k), (u + 0.34, 1.65 - k)],
                                    0.05, w=w + side * 0.01, bevel=0.0, mat=bp.EMBER, round=0.1))
    bp.body_sockets(m)


def barrel(bp, m):
    lg = bp.lg
    u0 = bp.BARREL_AT[0]
    obj, maps, mat, source = _load(bp, m, ((u0, u0 + 24.0), (-1.6, 1.6), 3.2))
    t4.fill_holes(obj)
    bvh = t4.tree(obj)
    # The gas block: Tripo made it a square tube open on both sides (the barrel shows through); aluminium covers close it.
    block = [p for p in (tripo.gun_uvw(v.co) for v in obj.data.vertices)
             if u0 + 6.0 < p[0] < u0 + 16.0 and (p[1] ** 2 + p[2] ** 2) ** 0.5 > 0.97]
    covers = []
    if block:
        bu0, bu1 = min(p[0] for p in block), max(p[0] for p in block)
        bv0, bv1 = min(p[1] for p in block), max(p[1] for p in block)
        bw = max(abs(p[2]) for p in block)
        alu = t4.uv_at(obj, bvh, (bu0 + bu1) * 0.5, bv1, 0.0)
        for side in (-1.0, 1.0):
            covers.append(t4.patch(lg.box(bu0 + 0.06, bu1 - 0.06, bv0 + 0.06, bv1 - 0.06, 0.04, w=side * (bw - 0.03),
                                          bevel=0.0), mat, alu))
        bp.lt._log(f'Tripo {m}: gas block u {bu0:.2f}-{bu1:.2f}, v {bv0:.2f}-{bv1:.2f}, w {bw:.2f}: side covers')
    t4.add(bp, m, obj, {mat: list(range(len(obj.data.polygons)))}, [mat])
    bp.g.add(m, *covers)
    # A heat line in the middle flute of each side (the flutes are only painted): u 3.6-9.15 and 11.85-19.4 from the nut.
    for side in (-1.0, 1.0):
        for a, b in ((3.6, 9.15), (11.85, 19.4)):
            ws = [t4.surface_w(bvh, u0 + a + (b - a) * t, 0.0, side) for t in (0.2, 0.5, 0.8)]
            ws = [w for w in ws if w is not None]
            if ws:
                w = sum(ws) / len(ws)
                bp.g.add(m, lg.box(u0 + a + 0.15, u0 + b - 0.15, -0.07, 0.07, 0.04, w=w + side * 0.005, bevel=0.0,
                                   mat=bp.EMBER))
    bp.g.socket(m, 'Muzzle', (u0 + 24.0, 0.0))


def muzzle(bp, m):
    u0 = bp.MUZZLE_AT[0]
    obj, maps, mat, source = _load(bp, m, ((u0, u0 + 5.0), (-1.04, 1.04), 2.08))
    ember = tripo.faces_where(obj, maps['BC'], tripo.is_ember, share=0.6)
    rest = [i for i in range(len(obj.data.polygons)) if i not in set(ember)]
    t4.add(bp, m, obj, {mat: rest, bp.EMBER: ember}, [mat])
    bp.g.socket(m, 'Muzzle', (u0 + 5.0, 0.0))


def magazine(bp, m):
    obj, maps, mat, source = _load(bp, m, ((10.4, 19.55), (-16.9, bp.MAG_AT[1]), 2.9))
    base = lambda p: p[1] < -15.4   # the inlay along the base plate (the index ticks stay painted)
    rods = _cyan_lines(bp, obj, maps, [(1.0, 10.0, 20.0, base), (-1.0, 10.0, 20.0, base)])
    groups, keep = _paints(bp, obj, maps, mat, source, {}, shell=False)
    t4.add(bp, m, obj, groups, keep)
    bp.g.add(m, *rods)


def sight(bp, m):
    """The enclosed dot: Tripo modeled the windows as lens faces on a closed box. Their faces go; a dark liner joins the
    two openings, glass sits in both and the red dot just behind the front glass; SOCKET_Aim is in the rear window on the
    windows' centre line (the dot on the crosshair)."""
    lg = bp.lg
    v0 = bp.SIGHT_AT[1]
    obj, maps, mat, source = _load(bp, m, ((19.9, 26.75), (v0, v0 + 5.35), 4.0))
    me = obj.data
    lens = [i for i in tripo.faces_where(obj, maps['BC'], t4.is_lens, share=0.5) if abs(me.polygons[i].normal.y) > 0.6]
    rear = [i for i in lens if tripo.gun_uvw(me.polygons[i].center)[0] < 21.6]
    front = [i for i in lens if tripo.gun_uvw(me.polygons[i].center)[0] > 25.0]
    (ru0, rv0, rw0), (ru1, rv1, rw1) = t4.bounds(obj, rear)
    (fu0, fv0, fw0), (fu1, fv1, fw1) = t4.bounds(obj, front)
    vc = (max(rv0, fv0) + min(rv1, fv1)) * 0.5
    wc = (max(rw0, fw0) + min(rw1, fw1)) * 0.5
    t4.move(obj, dw=-wc)   # the windows on the bore's plane
    half_v = (min(rv1, fv1) - max(rv0, fv0)) * 0.5
    half_w = (min(rw1, fw1) - max(rw0, fw0)) * 0.5
    bp.lt._log(f'Tripo {m}: windows rear u {ru0:.2f}-{ru1:.2f}, front u {fu0:.2f}-{fu1:.2f}; centre v {vc:.2f}, w {wc:+.2f} '
               f'(moved to 0); opening {2 * half_w:.2f} x {2 * half_v:.2f}; {len(rear)} + {len(front)} lens faces cut')
    # Everything on the end walls inside the windows goes, not only the faces painted lens blue (the rim's mixed faces
    # left shards in the sight picture).
    cut = set(rear + front)
    for p in obj.data.polygons:
        u, v, w = tripo.gun_uvw(p.center)
        if abs(p.normal.y) > 0.5 and abs(v - vc) < half_v - 0.04 and abs(w) < half_w - 0.04 \
                and (u < ru1 + 0.3 or u > fu0 - 0.3):
            cut.add(p.index)
    t4.delete_faces(obj, sorted(cut))
    low = lambda p: p[1] < v0 + 1.6 and abs(p[2]) > 1.2   # the accent line low on each side
    rods = _cyan_lines(bp, obj, maps, [(1.0, 20.0, 27.0, low), (-1.0, 20.0, 27.0, low)])
    # The housing's flat sides came out dented: pressed flat between the end bezels (the knobs, cap and tray stand
    # proud and stay).
    planes = t4.flatten_sides(obj, 20.7, 25.9, v0 + 1.7, v0 + 4.3)
    bp.lt._log(f'Tripo {m}: sides flattened at w {planes}')
    groups, keep = _paints(bp, obj, maps, mat, source, {}, shell=False)
    t4.add(bp, m, obj, groups, keep)
    bp.g.add(m, *rods)
    # The liner: four thin walls from the rear opening to the front one, just inside them.
    ua, ub = ru1 - 0.15, fu0 + 0.15
    hv, hw, t = half_v - 0.05, half_w - 0.05, 0.06
    for v in (vc - hv, vc + hv):
        bp.g.add(m, lg.box(ua, ub, v - t * 0.5, v + t * 0.5, 2 * hw, bevel=0.0, mat=lg.BLACK))
    for side in (-1.0, 1.0):
        bp.g.add(m, lg.box(ua, ub, vc - hv, vc + hv, t, w=side * hw, bevel=0.0, mat=lg.BLACK))
    for u in (ru1 - 0.25, fu0 + 0.1):
        bp.g.add(m, lg.box(u, u + 0.06, vc - hv, vc + hv, 2 * hw, bevel=0.0, mat=lg.LENS))
    bp.g.add(m, lg.pin(fu0 - 0.25, vc, -0.06, 0.06, 0.06, mat=bp.RETICLE, segments=10))
    bp.g.socket(m, 'Aim', (ru0 + 0.1, vc))


def stock(bp, m):
    obj, maps, mat, source = _load(bp, m, ((-10.5, -0.3), (-10.05, 4.6), 5.65))
    face = tripo.probe_u(obj, [(-8.0, 0.0), (-6.0, 1.0), (-1.0, -1.0), (1.5, 0.0)])   # the seat plate, off the boss
    t4.move(obj, du=(-0.3 - face) if face is not None else 0.0, dw=-0.225)   # concept w -3.05..2.6 (the knob on the right)
    bp.lt._log(f'Tripo {m}: seat plate face at u {face:.2f}, moved onto the butt plate (u -0.3)')
    block = lambda p: p[1] < 1.0      # the line round the block
    comb = lambda p: p[1] > 2.5       # the line under the cheek riser
    rods = _cyan_lines(bp, obj, maps, [(1.0, -11.0, 0.0, block), (-1.0, -11.0, 0.0, block),
                                       (1.0, -11.0, 0.0, comb), (-1.0, -11.0, 0.0, comb)])
    groups, keep = _paints(bp, obj, maps, mat, source, {})
    t4.add(bp, m, obj, groups, keep)
    bp.g.add(m, *rods)


PARTS = {
    'Body_Marksman': body,
    'Barrel_Marksman': barrel,
    'Muzzle_Compensator': muzzle,
    'Magazine_20': magazine,
    'Sight_RedDot': sight,
    'Stock_Cheek': stock,
}
