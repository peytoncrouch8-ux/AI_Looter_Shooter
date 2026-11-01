"""How each Tripo building (Art/Tripo/<Name>/<Name>.glb; specs Art/Tripo/specs/buildings.json) is fitted into the
scripted building it replaces (Tools/Blender/looter_tripo_buildings.py). The building scripts call
looter_tripo_buildings.apply(model, '<Name>') on their finished model; FITS maps each Tripo name to its fit here.
Tools/models.ps1 skips parts_*.py: this file is data for those scripts, not a model.

Every fit stretches the shell onto the scripted model's bounds, then moves the features the game stands on (a porch
deck, a stoop, a front wall) onto the scripted ones, cuts the shell where scripted story parts go back in, and keeps
the scripted UCX hulls and sockets (a chimney's Smoke socket follows the shell's own chimney top).
"""
import os

import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_tripo_buildings as tb


def _wall_y(model, xs, zs, start=-14.0):
    """The front wall's plane (y, Blender: the front is -Y) on model, from rays heading +Y at these x and z."""
    return tb.wall_plane(model, 'y', 1.0, [(x, z) for x in xs for z in zs], start)


def _smoke(obj, shell_top, near, why='onto the Tripo chimney top'):
    if shell_top:
        tb.move_socket(obj, 'Smoke', (shell_top[0], shell_top[1], shell_top[2] + 0.05), why)
    else:
        tb.log(f'{obj.name}: no chimney top found near {near}: SOCKET_Smoke left where it was')


# --- Landfall farm ---

FARM_PORCH = [(x, -4.7) for x in (-3.6, -2.6, -1.6, 1.6, 2.6, 3.6)]   # porch deck points, clear of the steps
FARM_WALL_X = (-3.9, -3.6, -1.3, -1.1, 1.1, 1.3, 3.6, 3.9)           # front wall points, clear of windows and door
FARM_WALL_Z = (1.0, 1.25, 3.3)
FARM_PIPE = ((-0.5, -1.1, 4.0), (0.55, -0.15, 9.0))      # Tripo's stovepipe (SOCKET_Smoke lands on it)
FARM_POST = ((1.03, -5.25, 0.62), (1.27, -5.01, 3.06))   # one scripted porch post (Farmhouse.py: x +-1.15, +-3.45)
FARM_POST_AT = Vector((1.15, -5.13, 0.6))                # its foot
FARM_STEPS = ((-0.9, -5.95, -0.05), (0.9, -5.22, 0.45))  # the scripted steps (two treads, x +-0.82, y -5.3..-5.9)
FARM_STEPS_BACK = -5.30                                  # where they meet the scripted deck's front edge


def _farm_roof_group(co, n):
    """Which plane a farmhouse shell vertex straightens onto: the main roof's two slopes (over the eaves, either side
    of the ridge at y 0), the porch roof (in front of the wall), each top or underside, and the porch deck's top;
    walls, fascias, the dormer's roof and the rest (normals off the slope) none."""
    if abs(n.z) < 0.35 or abs(n.x) > 0.4:   # walls and fascias; the dormer's roof (its slopes face +-X)
        return None
    face = 'top' if n.z > 0 else 'under'
    if co.z > 4.15 and -3.75 < co.y < 3.75 and abs(co.x) < 4.75:
        return ('roof', 'front' if co.y < 0.0 else 'back', face)
    if 2.55 < co.z < 4.05 and co.y < -3.2 and abs(co.x) < 4.75:
        return ('porch roof', face)
    if face == 'top' and 0.4 < co.z < 0.75 and -5.3 < co.y < -3.15 and abs(co.x) < 4.75:
        return ('porch deck', face)    # where Delia stands: level boards on its hull
    return None


def _percentile(values, share):
    values = sorted(values)
    return values[min(len(values) - 1, max(0, int(share * len(values))))]


def _farm_edges(shell, found):
    """Measures the straightened porch roof, porch deck and main roof (their extents and planes) and deletes the shell's
    ragged edge bands round them (Main's review, 2026-11-01: crumpled grey-white smears and shards at eye level): the
    side faces of the porch roof's front and ends, of the deck's front and ends, and of the main roof's eaves and
    gables, down to just under where the trim boards (_farm_trim) go. Tops and undersides stay."""
    planes = found['planes']
    me = shell.data
    me.update()
    groups = {}
    for v in me.vertices:
        key = _farm_roof_group(v.co, v.normal)
        if key and key[-1] == 'top':
            groups.setdefault(key[:-1] if key[0] == 'roof' else key[0], []).append(v.co.copy())

    def box(points):
        xs, ys = [c.x for c in points], [c.y for c in points]
        return (_percentile(xs, 0.005), _percentile(xs, 0.995), _percentile(ys, 0.005), _percentile(ys, 0.995))

    def z_of(key, x, y):
        a, b, c = planes[key]
        return a * x + b * y + c

    ex = {'porch': box(groups['porch roof']), 'deck': box(groups['porch deck']),
          'roof': box(groups[('roof', 'front')] + groups[('roof', 'back')])}
    # The main roof's ridge: where its two planes meet over x 0.
    fa, fb, fc = planes[('roof', 'front', 'top')]
    ba, bb, bc = planes[('roof', 'back', 'top')]
    ridge_y = (bc - fc) / (fb - bb)
    ex['ridge'] = (ridge_y, fb * ridge_y + fc)
    found['extent'] = ex
    px0, px1, py0, _ = ex['porch']
    dx0, dx1, _, _ = ex['deck']
    rx0, rx1, ry0, ry1 = ex['roof']
    edge = found['edge']
    gone = []
    for poly in me.polygons:
        c, n = poly.center, poly.normal
        # Shards hanging off the porch roof's front and ends, whatever way they face: the fascia covers that line.
        if (2.2 < c.z < z_of(('porch roof', 'top'), c.x, c.y) + 0.3 and c.y < -3.3
                and (c.y < py0 + 0.03 or c.x < px0 + 0.03 or c.x > px1 - 0.03)):
            gone.append(poly.index)
            continue
        if n.z > 0.6 or n.z < -0.35:
            continue
        if c.y < -3.2:
            top = z_of(('porch roof', 'top'), c.x, c.y)
            if (top - 0.22 < c.z < top + 0.15 and (c.y < py0 + 0.3 or c.x < px0 + 0.3 or c.x > px1 - 0.3)
                    and px0 - 0.4 < c.x < px1 + 0.4):
                gone.append(poly.index)
                continue
            if 0.25 < c.z < 0.58 and (c.y < edge + 0.3 or c.x < dx0 + 0.3 or c.x > dx1 - 0.3) \
                    and dx0 - 0.4 < c.x < dx1 + 0.4 and c.y > edge - 0.4:
                gone.append(poly.index)
                continue
        if c.z > 3.6 and not all(FARM_PIPE[0][k] <= c[k] <= FARM_PIPE[1][k] for k in range(3)):
            side = ('roof', 'front', 'top') if c.y < ridge_y else ('roof', 'back', 'top')
            top = z_of(side, c.x, c.y)
            near = c.y < ry0 + 0.3 or c.y > ry1 - 0.3 or c.x < rx0 + 0.2 or c.x > rx1 - 0.2                 or abs(c.y - ridge_y) < 0.12
            if near and top - 0.22 < c.z < top + 0.15:
                gone.append(poly.index)
    tb.delete_faces(shell, gone)
    tb.log(f'farmhouse: {len(gone)} ragged edge faces cut (porch roof x {px0:.2f}..{px1:.2f} front {py0:.2f}, deck x '
           f'{dx0:.2f}..{dx1:.2f} front {edge:.2f}, roof x {rx0:.2f}..{rx1:.2f} y {ry0:.2f}..{ry1:.2f}, ridge '
           f'{ridge_y:.2f} at {ex["ridge"][1]:.2f})')


def _farm_trim(found):
    """Straight boards on the house trim sheet (hewn beam faces, MI_HouseTrim, like the posts) where _farm_edges cut the
    shell: a fascia along the porch roof's front and both ends and a ledger where it meets the wall, a rim board round
    the deck's front and ends, fascias along the main roof's eaves and rake boards up both gables and a ridge board. Built with
    looter_buildings (its own occlusion bake); returns the object to join."""
    planes = found['planes']
    ex = found['extent']

    def z_of(key, x, y):
        a, b, c = planes[key]
        return a * x + b * y + c

    m = kit.Model('_TripoTrim', seed=11)
    wall = -3.12
    # The porch roof: fascia 0.24 deep from 2 cm over its top plane, front and ends; a ledger along the wall.
    px0, px1, py0, _ = ex['porch']
    pr = ('porch roof', 'top')
    fz = z_of(pr, 0.0, py0) - 0.1
    m.board((px0 - 0.04, py0 - 0.02, fz), (px1 + 0.04, py0 - 0.02, fz), 0.24, 0.04, face=(0.0, -1.0, 0.0), uv='C')
    for x, f in ((px0 - 0.02, -1.0), (px1 + 0.02, 1.0)):
        m.board((x, py0 - 0.04, fz), (x, wall, z_of(pr, x, wall) - 0.1), 0.24, 0.04, face=(f, 0.0, 0.0), uv='C')
    lz = z_of(pr, 0.0, wall - 0.04) + 0.07
    m.board((px0, wall - 0.035, lz), (px1, wall - 0.035, lz), 0.14, 0.03, face=(0.0, -1.0, 0.0), uv='C')
    # The deck: a rim board 0.32 tall, its top flush with the boards.
    dx0, dx1, _, _ = ex['deck']
    edge = found['edge']
    dz = z_of(('porch deck', 'top'), 0.0, edge) - 0.15
    m.board((dx0 - 0.04, edge - 0.02, dz), (dx1 + 0.04, edge - 0.02, dz), 0.32, 0.04, face=(0.0, -1.0, 0.0), uv='C')
    for x, f in ((dx0 - 0.02, -1.0), (dx1 + 0.02, 1.0)):
        m.board((x, edge - 0.04, dz), (x, wall, dz), 0.32, 0.04, face=(f, 0.0, 0.0), uv='C')
    # The main roof: eave fascias and rake boards, 0.3 deep from 2 cm over the roof planes.
    rx0, rx1, ry0, ry1 = ex['roof']
    ridge_y, ridge_z = ex['ridge']
    front, back = ('roof', 'front', 'top'), ('roof', 'back', 'top')
    ez0 = z_of(front, 0.0, ry0) - 0.13
    ez1 = z_of(back, 0.0, ry1) - 0.13
    m.board((rx0 - 0.04, ry0 - 0.02, ez0), (rx1 + 0.04, ry0 - 0.02, ez0), 0.3, 0.04, face=(0.0, -1.0, 0.0), uv='C')
    m.board((rx0 - 0.04, ry1 + 0.02, ez1), (rx1 + 0.04, ry1 + 0.02, ez1), 0.3, 0.04, face=(0.0, 1.0, 0.0), uv='C')
    for x, f in ((rx0 - 0.02, -1.0), (rx1 + 0.02, 1.0)):
        m.board((x, ry0 - 0.03, ez0), (x, ridge_y, ridge_z - 0.13), 0.3, 0.04, face=(f, 0.0, 0.0), uv='C')
        m.board((x, ridge_y, ridge_z - 0.13), (x, ry1 + 0.03, ez1), 0.3, 0.04, face=(f, 0.0, 0.0), uv='C')
    # A ridge board over the seam where the two straightened slopes meet.
    m.board((rx0 - 0.04, ridge_y, ridge_z + 0.03), (rx1 + 0.04, ridge_y, ridge_z + 0.03), 0.34, 0.05,
            face=(0.0, 0.0, 1.0), uv='C')
    trim = m.finish(ao=True, ground=True, preview=False, fallback=None)
    return trim


def farmhouse(obj):
    """Delia's house (SM_Farmhouse_Ransom): the porch deck at the scripted deck's height (she stands and fights there),
    the front wall on the scripted wall's plane; the door opening, the lamplit hall behind it, the crepe, the stool
    under SOCKET_Plate and the porch lantern (SOCKET_Light_Porch) stay the scripted ones.
    Main's review (2026-11-01): Tripo's porch posts (stretched paint) are the scripted post, copied to where Tripo's
    stood and fitted from the deck to its porch roof, with box hulls; the roof's corrugation ridges (cardboard close
    up) are straightened onto the two roof planes and the porch roof's (dormer and stovepipe left); Tripo's lumpy front
    steps are the scripted steps, set against Tripo's deck edge, their scripted hulls moved with them."""
    found = {}

    def anchors(shell, scripted):
        z_from = tb.deck_height(shell, FARM_PORCH, 2.3)
        z_to = tb.deck_height(scripted, FARM_PORCH, 2.3)
        y_from = _wall_y(shell, FARM_WALL_X, FARM_WALL_Z)
        y_to = _wall_y(scripted, FARM_WALL_X, FARM_WALL_Z)
        tb.log(f'farmhouse: porch deck {z_from} -> {z_to}, front wall {y_from} -> {y_to}')
        return {'z': [(z_from, z_to)], 'y': [(y_from, y_to)]}

    def extra(shell, scripted):
        found['smoke'] = tb.protrusion_top(shell, scripted, (2.75, -1.25), radius=4.5)
        # The roofs straightened first (the decimation then has flat planes to work on).
        report = tb.flatten(shell, _farm_roof_group, threshold=0.2, exclude=[FARM_PIPE])
        # The porch roof's top once more with a wider reach: pale shards a little proud of it pulled down flush.
        report.update(tb.flatten(shell, lambda co, n: ('porch roof', 'top') if _farm_roof_group(co, n) == (
            'porch roof', 'top') else None, threshold=0.35))
        found['planes'] = {key: coef for key, (count, rms, coef) in report.items()}
        for key, (count, rms, coef) in sorted(report.items()):
            tb.log(f'farmhouse: flattened {key}: {count} vertices, rms {rms} m off the plane z = {coef}')
        # A lump Tripo left on the porch roof (a grey pyramid over the left post): whatever stands proud of it goes.
        a, b, c0 = found['planes'].get(('porch roof', 'top'), (0.0, 0.213, 4.013))
        lumps = [p.index for p in shell.data.polygons
                 if p.center.y < -3.25 and abs(p.center.x) < 4.8
                 # (under the main roof's eave, y over -3.7, only what stays clear of it)
                 and 0.05 < p.center.z - (a * p.center.x + b * p.center.y + c0) < (1.2 if p.center.y < -3.7 else 0.5)]
        tb.delete_faces(shell, lumps)
        tb.log(f'farmhouse: {len(lumps)} faces proud of the porch roof removed')
        # Tripo's posts: found, their roof undersides measured, then cut out.
        posts = tb.posts(shell, ((-4.7, -6.0, 0.55), (4.7, -3.35, 2.75)))
        bvh = tb.tree(shell)
        found['posts'] = []
        for lo, hi in posts:
            mid = (lo + hi) * 0.5
            up = tb.probe(shell, (mid.x, mid.y, 2.4), (0, 0, 1), 3.0, bvh)
            top = up[0].z if up else 2.95
            found['posts'].append((Vector((mid.x, mid.y, 0.0)), top, lo, hi))
        for at, top, lo, hi in found['posts']:
            tb.cut_box(shell, ((lo.x - 0.08, lo.y - 0.08, -0.3), (hi.x + 0.08, hi.y + 0.08, top - 0.01)))
            # and whatever of it pokes up through the porch roof (a grey stub over the right-hand posts)
            tb.cut_box(shell, ((lo.x - 0.12, lo.y - 0.12, top + 0.08), (hi.x + 0.12, hi.y + 0.12, top + 0.8)))
        where = [(round(p.x, 2), round(p.y, 2), round(t, 2)) for p, t, _, _ in found['posts']]
        tb.log(f'farmhouse: Tripo posts replaced at {where}')
        # The deck's front edge (rays down either side of the steps), then Tripo's steps cut away in front of it.
        edge = None
        y = -5.0
        while y > -6.2:
            hits = [tb.probe(shell, (x, y, 0.9), (0, 0, -1), 1.0, bvh) for x in (-1.6, 1.6)]
            if all(h and h[0].z > 0.45 for h in hits):
                edge = y
            y -= 0.02
        found['edge'] = edge if edge is not None else FARM_STEPS_BACK
        cut = tb.cut_box(shell, ((-1.2, -6.3, -0.2), (1.2, found['edge'] - 0.01, 0.53)))
        tb.log(f'farmhouse: deck front edge y {found["edge"]:.2f}; Tripo steps cut ({cut} faces)')
        _farm_edges(shell, found)

    def more_pieces(shell, scripted):
        pieces = []
        for at, top, lo, hi in found['posts']:
            # The scripted post (foot 0.6, top 3.06), from the ground (Tripo's ran down past the deck) to its porch roof.
            scale = (top + 0.02) / (FARM_POST[1][2] - FARM_POST_AT.z)
            m = (Matrix.Translation((at.x, at.y, 0.0)) @ Matrix.Diagonal((1.0, 1.0, scale, 1.0))
                 @ Matrix.Translation(-FARM_POST_AT))
            pieces.append(tb.copy_faces(scripted, [FARM_POST], m, '_TripoPost'))
        dy = found['edge'] - FARM_STEPS_BACK
        pieces.append(tb.copy_faces(scripted, [FARM_STEPS], Matrix.Translation((0.0, dy, 0.0)), '_TripoSteps'))
        pieces.append(_farm_trim(found))
        return pieces

    def retexture(shell, scripted):
        # Tripo's paint has the corrugation's waves painted in, so straightening the roof alone still read as
        # cardboard: its top faces go onto the trim sheet's weathered boards, running down the slope (a board roof,
        # as the town's sheds have), at world scale. LOOTER_FARMHOUSE_ROOF=tripo keeps Tripo's paint.
        if os.environ.get('LOOTER_FARMHOUSE_ROOF', 'trim') != 'trim':
            return
        me = shell.data
        tops = []
        for p in me.polygons:
            c, n = p.center, p.normal
            if all(FARM_PIPE[0][k] <= c[k] <= FARM_PIPE[1][k] for k in range(3)):
                continue
            key = _farm_roof_group(c, n)
            plane = found['planes'].get(key) if key and key[-1] == 'top' and key[0] != 'porch deck' else None
            # Only faces lying on their straightened plane (the dormer's cheeks and roof stay Tripo's).
            if plane and abs(c.z - (plane[0] * c.x + plane[1] * c.y + plane[2])) < 0.08:
                tops.append(p.index)
        tb.log(f'farmhouse: roof tops onto the trim sheet: {tb.onto_trim(shell, tops, "A", rotate=True, seed=7, axis=(1.0, 0.0, 0.0))} faces')
        # Whatever on the porch roof's top still wears Tripo's paint is a shard (its boards are all on the trim sheet
        # now, its fascia and ends cut): gone.
        pa, pb, pc = found['planes'][('porch roof', 'top')]
        px0, px1, _, _ = found['extent']['porch']
        shards = [p.index for p in me.polygons
                  if p.material_index == 0 and p.center.y < -3.25 and px0 - 0.3 < p.center.x < px1 + 0.3
                  and -0.02 < p.center.z - (pa * p.center.x + pb * p.center.y + pc)
                  < (0.9 if p.center.y < -3.7 else 0.45)]          # (the main eave's underside starts 0.6 up)
        tb.delete_faces(shell, shards)
        tb.log(f'farmhouse: {len(shards)} Tripo shards left on the porch roof removed')

    tb.swap(obj, 'FarmhouseRansom', anchors=anchors, extra=extra, faces_limit=17000, fallback=100.0,
            variant=os.environ.get('LOOTER_FARMHOUSE_PAINT', ''), more_pieces=more_pieces, retexture=retexture,
            hulls_keep=[((-3.2, -4.75, 0.56), (-1.8, -3.45, 1.9)),     # the rocking chair's and the stool's hulls
                        ((0.72, -3.65, 0.5), (1.28, -3.05, 1.12)),
                        FARM_STEPS],                                  # the steps' hulls (moved below)
            hulls_drop=[((-4.0, -5.4, 1.0), (4.0, -4.8, 2.6))],       # the scripted porch posts (x +-1.15, +-3.45)
            cut=[((-0.66, -4.2, 0.6), (0.66, -2.6, 2.85)),     # the door opening
                 ((-1.45, -4.2, 0.6), (0.66, -3.17, 2.85))],    # Tripo's own screen door, swung out to the left
            keep=[((-0.6, -3.0, 0.5), (1.25, -1.1, 3.35)),       # the hall (Farmhouse.py HALL_*: x -0.51..1.15)
                  ((-0.8, -3.4, 0.64), (0.8, -2.9, 3.4)),        # door casing, crepe, the open door
                  ((0.72, -3.65, 0.62), (1.28, -3.05, 1.12)),    # the stool under the plate (SOCKET_Plate)
                  ((0.72, -3.7, 2.2), (1.2, -3.1, 3.05)),        # the porch lantern (SOCKET_Light_Porch)
                  ((-1.5, -3.75, 0.56), (-0.7, -3.1, 1.75)),     # her broom and pail
                  ((-3.2, -4.75, 0.56), (-1.8, -3.45, 1.9))])    # her rocking chair (Farmhouse.py CHAIR: Abel's hat
                                                                 # hangs on it, build_area_landfall.py HAT_CHAIR)
    if 'posts' in found:
        # Box hulls round the scripted posts where they now stand; the steps' hulls moved with the steps.
        for at, top, lo, hi in found['posts']:
            tb.add_box_hull(obj, Vector((at.x - 0.09, at.y - 0.09, 0.55)), Vector((at.x + 0.09, at.y + 0.09, top)))
        dy = found['edge'] - FARM_STEPS_BACK
        for hull in [c for c in obj.children if c.name.startswith('UCX_')]:
            lo, hi = tb._hull_box(hull)
            mid = (lo + hi) * 0.5
            if all(FARM_STEPS[0][k] <= mid[k] <= FARM_STEPS[1][k] for k in range(3)):
                hull.data.transform(Matrix.Translation((0.0, dy, 0.0)))
    _smoke(obj, found.get('smoke'), (2.75, -1.25))
    return obj


def barn(obj):
    """The Ransom barn (SM_Barn_Ransom): no sockets; its hulls are the scripted ones."""
    tb.swap(obj, 'BarnRansom', faces_limit=11000, fallback=100.0)
    return obj


def cottage(obj):
    """The settler's cottage (SM_Cottage_Ransom): the stoop at the scripted stoop's height; SOCKET_Light stays in the
    front room, SOCKET_Smoke goes to the shell's chimney top."""
    found = {}
    stoop = [(-1.4, -3.6), (-0.7, -3.6), (-1.0, -3.8)]

    def anchors(shell, scripted):
        z_from = tb.deck_height(shell, stoop, 1.6)
        z_to = tb.deck_height(scripted, stoop, 1.6)
        y_from = _wall_y(shell, (1.6, 2.0, 2.2), (1.0, 2.2))
        y_to = _wall_y(scripted, (1.6, 2.0, 2.2), (1.0, 2.2))
        tb.log(f'cottage: stoop {z_from} -> {z_to}, front wall {y_from} -> {y_to}')
        return {'z': [(z_from, z_to)], 'y': [(y_from, y_to)]}

    def extra(shell, scripted):
        found['smoke'] = tb.protrusion_top(shell, scripted, (-1.0, 1.9), radius=4.0)

    tb.swap(obj, 'CottageRansom', anchors=anchors, extra=extra, faces_limit=10000, fallback=100.0,
            hulls_drop=[((-2.0, -3.8, 1.0), (0.0, -3.55, 2.6))],      # the scripted stoop posts (x -1.89, -0.13)
            post_region=((-2.9, -4.3, 0.4), (2.9, -3.25, 2.6)), post_band=(1.0, 2.0))
    _smoke(obj, found.get('smoke'), (-1.0, 1.9))
    return obj


# --- Ransom's Rest ---

WM_HUB = Vector((0.0, -0.98, 8.38))   # Homestead.py WM_HUB: SOCKET_Fan, where RR_WindmillFan hangs and turns


def windmill(obj, fan=None):
    """The timber windpump (SM_RR_Windmill) and its wheel (SM_RR_WindmillFan). Tripo's windpump is one piece: scaled
    evenly so its hub stands on SOCKET_Fan (the hub's height sets the scale), its wheel cut off into the fan model
    (pivot on the hub, facing -Y, as the game turns it), the rest is the tower; the tower's hulls are refitted to it."""
    found = {}

    def fit(shell, scripted):
        me = shell.data
        lo, hi = tb.bounds(shell)
        size = hi - lo
        # The wheel: the front slab of the top third (Tripo faces it front, +X, turned to -Y by orient()).
        wheel = [v.co for v in me.vertices if v.co.z > lo.z + 0.68 * size.z and v.co.y < lo.y + 0.15 * size.y]
        w_lo = Vector((min(c[k] for c in wheel) for k in range(3)))
        w_hi = Vector((max(c[k] for c in wheel) for k in range(3)))
        radius = max(w_hi.x - w_lo.x, w_hi.z - w_lo.z) * 0.5
        scale = (WM_HUB.z - 0.0) / ((w_lo.z + w_hi.z) * 0.5 - lo.z)
        hub = Vector(((w_lo.x + w_hi.x) * 0.5, w_hi.y - 0.08 / scale, (w_lo.z + w_hi.z) * 0.5))
        for v in me.vertices:
            v.co = (v.co - Vector((hub.x, hub.y, lo.z))) * scale + Vector((WM_HUB.x, WM_HUB.y, 0.0))
        me.update()
        # The wheel's faces: in front of the head (y under the slab's back), within its radius of the hub.
        back = WM_HUB.y + 0.08 + 0.02
        found['fan'] = [p.index for p in me.polygons
                        if p.center.y < back and (Vector((p.center.x, 0, p.center.z)) - Vector((0, 0, WM_HUB.z))).length
                        < radius * scale * 1.04]
        tb.log(f'windmill: scale {scale:.3f}, wheel radius {radius * scale:.2f} m (the scripted wheel 1.7), '
               f'{len(found["fan"])} wheel faces, bounds now {tuple(round(v, 2) for v in tb.bounds(shell)[0])}'
               f'..{tuple(round(v, 2) for v in tb.bounds(shell)[1])}')

    def extra(shell, scripted):
        if fan is None:
            return
        wheel = bpy.data.objects.new('_TripoWheel', shell.data.copy())
        bpy.context.scene.collection.objects.link(wheel)
        keep = set(found['fan'])
        tb.delete_faces(wheel, [i for i in range(len(wheel.data.polygons)) if i not in keep])
        tb.delete_faces(shell, sorted(keep))
        found['wheel'] = wheel

    tb.swap(obj, 'WindmillRR', fit=fit, extra=extra, faces_limit=12000, fallback=100.0)
    wheel = found.get('wheel')
    if fan is not None and wheel is not None:
        # The wheel's own model: its mesh round the hub, the tower's material (the same Tripo set).
        wheel.data.transform(Matrix.Translation(-WM_HUB))
        for attr in list(wheel.data.color_attributes):
            wheel.data.color_attributes.remove(attr)
        while len(wheel.data.uv_layers) > 1:
            wheel.data.uv_layers.remove(wheel.data.uv_layers[-1])
        wheel.data.uv_layers[0].name = 'UVMap'
        wheel.data.materials.clear()
        wheel.data.materials.append(obj.data.materials[0])
        for poly in wheel.data.polygons:
            poly.material_index = 0
        tb.sharp_by_angle(wheel)
        old = fan.data
        wheel.data.name = fan.name
        fan.data = wheel.data
        bpy.data.objects.remove(wheel)
        if not old.users:
            bpy.data.meshes.remove(old)
        fan['TripoSource'] = obj.get('TripoSource', '')
        tb.log(f'windmill: {fan.name} now the Tripo wheel, {tb.triangles(fan)} triangles')
    return obj


def _front(obj, name, wall_xs, faces_limit):
    """A Main Street false front (FrontsRR.py): stretched onto the scripted model's bounds, its facade moved onto the
    scripted facade's plane (the Decal sockets lie on it, the boardwalk runs along it); hulls refitted."""
    def anchors(shell, scripted):
        y_from = _wall_y(shell, wall_xs, (1.0, 2.5))
        y_to = _wall_y(scripted, wall_xs, (1.0, 2.5))
        tb.log(f'{name}: facade {y_from} -> {y_to}')
        return {'y': [(y_from, y_to)]}

    found = {}

    def extra(shell, scripted):
        found['smoke'] = tb.protrusion_top(shell, scripted, tuple(tb.socket(scripted, 'Smoke').location)[:2], radius=1.5)

    tb.swap(obj, name, anchors=anchors, extra=extra, faces_limit=faces_limit, fallback=100.0)
    if found.get('smoke'):
        _smoke(obj, found['smoke'], None)
    return obj


def assay(obj):
    """The assay office (SM_RR_Front_Assay): closed, shuttered and chained. NOT HOOKED (2026-11-01): fitted and judged
    beside the scripted front, it lost what the story reads (PROVIDENCE MINING CO. / ASSAY OFFICE lettered, the pick and
    hammer, the barred iron shutters, the chained door) for a blank board, and Main Street's other fronts are scripted:
    FrontsRR.py keeps its own. To try it: tripo_buildings.apply(BUILT[0][1], 'FrontAssay') after BUILT there."""
    return _front(obj, 'FrontAssay', (-3.0, -2.6, 2.6, 3.0), 10000)


def barber(obj):
    """The barber's (SM_RR_Front_Barber): closed, blinds drawn. NOT HOOKED (2026-11-01): Tripo's has no false front (a
    plain gable), a blank sign for BARBER / SHAVE . BATH . TEETH, and stone-grey sides; FrontsRR.py keeps its own. To
    try it: tripo_buildings.apply(BUILT[2][1], 'FrontBarber') after BUILT there."""
    return _front(obj, 'FrontBarber', (-0.9, 3.6, 4.4), 8000)


FITS = {
    'FarmhouseRansom': farmhouse,
    'BarnRansom': barn,
    'CottageRansom': cottage,
    'WindmillRR': windmill,
    'FrontAssay': assay,
    'FrontBarber': barber,
}
