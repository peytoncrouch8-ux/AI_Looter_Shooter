"""The graves kit of Ransom's Rest (Docs/Areas/RansomsRest.md): the headboards, crosses and mounds of the Chapel of
Saint Ada's churchyard (about 50 boards), Boot Hill and the Ransom family plot; Ellis's dug-up grave and Abel's frosted
one; the keeper's grave at the crossroads at the root of Gravewind Point; the churchyard's iron fence and the family
plot's white picket fence; coffins for the Sink's floor; a mourning wreath and a black crepe swag for doors.

Small instanced props (scripted, Art/README.md): Nanite off, LODs at 50% and 25%, baked vertex occlusion. Pivots stand
on the ground; fronts face -Y (a headboard's lettered face, a fence's outside).

  Grave_Headboard_OldA..D   weathered boards (grey pine or dark oak), leaning, their carving worn to illegible marks
  Grave_Headboard_FreshA/B  new whitewashed boards, upright, a black painted cross or a name gone illegible
  Grave_Cross_A             a weathered oak cross
  Grave_Cross_B             a wrought-iron cross with a ring, on a fieldstone foot
  Grave_MoundFresh          a fresh mound of dirt (put a headboard at its +Y end); a respawn grave
  Grave_Ellis               Ellis's grave, dug open from inside: heaped dirt, the coffin's lid smashed, the headboard
                            ELLIS RANSOM / CAME HOME AT THE LAST knocked askew; the player wakes in the coffin
  Grave_Abel                Abel's grave: a whole mound under frost in the afternoon sun, black dead flowers, the
                            headboard ABEL RANSOM / KEEPER OF SAINT ADA / HE HELD THE DOOR rimed with frost
  Grave_Keeper              the keeper's grave at the crossroads: an oak board on two posts under a little roof, a
                            carved lantern, an iron lantern hook, a fieldstone kerb round a sunken mound; a respawn grave
  Fence_IronSection         2 m of the churchyard's iron fence: a post at x = 0, rails and spear-topped pickets to x = 2
  Fence_IronPost            the heavy corner post (corners, run ends, beside the gate)
  Fence_IronGate            a 2 m gate: heavy posts at x = 0 and x = 2, the two leaves standing open inward (+Y)
  Fence_PicketSection       2 m of the family plot's picket fence: a post at x = 0, rails, whitewashed pickets to x = 2
  Fence_PicketGate          a 2 m unit: a gate leaf hung on the post at x = 0, standing ajar inward, then pickets
  Fence_PicketPost          a single post: ends a picket run
  Coffin_Closed             an old pine coffin, lid nailed down, iron handles (the Sink's floor, the undertaker's)
  Coffin_Broken             an old coffin broken open: lid smashed in two, a side stove in, dirt inside (the Sink)
  Wreath                    a mourning wreath of dark evergreen with a black crepe bow; its pivot is the nail it
                            hangs from, its back on the door
  CrepeSwag                 a black crepe swag for a door's head with rosettes and tails; its pivot is the middle of
                            its top, its back on the wall (the chapel's and the depot's SOCKET_Crepe)

Chaining (as Fences.py): a section's pivot is on the ground at its post (x = 0) and it runs along +X to x = 2 m, where
the next piece starts; its rails end inside the next piece's post. Turn the next piece at the joint for a corner; end a
run with the post (Fence_IronPost, Fence_PicketPost) at its last x = 2 m. A gate is a 2 m unit like a section.

Sockets: SOCKET_Interact in front of every headboard and cross (and on the graves); SOCKET_Respawn where the body rises
(Grave_MoundFresh, Grave_Ellis, Grave_Keeper), facing out of the grave's foot (-Y); SOCKET_Loot in Ellis's coffin
(the Common Bullpup after "Skip the tutorial"); SOCKET_Lantern under the keeper's lantern hook.
Collision: a slim box round each board or cross; boxes for fence runs and gate posts, the open leaves, the coffins and
the keeper's posts; none on the mounds, the wreath and the crepe (Collision 'None'); low boxes round Ellis's heap.
"""
import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
import looter_chapel as lc
import looter_model as lm
import looter_props as lp
import looter_textures as lt

TRIM = lp.trim_material()                                                     # weathered wood, stone, iron straps
PAINT_BLACK = lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622)      # lettering, the painted cross
DIRT = lt.material('GroundDirt')                                              # grave dirt
FROST = lt.material('Polymer', name='GraveFrost', tint=0xd4e0ea)              # rime on Abel's grave (new)
IRON = lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a)              # the iron fence, crosses, handles
CREPE = lt.material('Polymer', name='MourningCrepe', tint=0x161518)           # crepe, ribbons and bows
GRANITE = lt.material('RockGranite', MossAmount=0.6)                          # fieldstones (as Rocks.py)
LEAVES = lt.material('FoliagePalette')                                        # the wreath's evergreen (as the town's)

TILE = {PAINT_BLACK: 'PaintWorn', DIRT: 'GroundDirt', FROST: 'Polymer', IRON: 'MetalWorn', CREPE: 'Polymer',
        GRANITE: 'RockGranite'}
PREVIEW = 'RansomsRest/Chapel'


# --- Parts ---

def tile(obj, mat, seed=0):
    """Puts a tileable material on a part and maps it at world scale."""
    lt.assign(obj, mat)
    lt.box_uv(obj, TILE[mat], seed=seed, along='long')
    return obj


def wood(obj, seed, axis=(0.0, 0.0, 1.0), strip='Siding', slot=None):
    """Weathered wood on the trim sheet, the grain along axis (one board, no end joints)."""
    return lp.grain(obj, strip, axis=axis, seed=seed, slot=slot)


def whitewash(obj, seed):
    """Lime whitewash on new boards and pickets: the plaster strip (G) turned upright, on its clean stretch (4.1-5.2 m
    along it, between the spalls), so a board from 25 cm under the ground to 1 m up shows no stains."""
    rnd = random.Random(seed)
    lt.assign(obj, TRIM)
    lt.trim_uv(obj, None, 'G', u_offset=4.32 + rnd.uniform(-0.04, 0.04), rotate=True, v_offset=rnd.uniform(0.0, 0.2))
    return obj


def flat(outline, thick, y=0.0):
    """A flat piece: outline (x, z) counterclockwise seen from the front, thick centred on y (front toward -Y)."""
    obj = lp.mesh_object(kit._prism([outline], thick))
    obj.data.transform(Matrix.Translation((0.0, y - thick * 0.5, 0.0)))
    return obj


def extrude(points, z0, z1):
    """A plan outline (x, y) extruded from z0 up to z1."""
    obj = lp.mesh_object(kit._prism([[(x, -y) for x, y in points]], z1 - z0))
    obj.data.transform(Matrix.Translation((0.0, 0.0, z0)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return obj


def lettering(text, size, at, tolerance=0.04):
    """Black painted lettering in Rye, a hair proud of a board face at at (x, y, z), facing -Y."""
    bm = lc.lettering(text, size, tolerance=tolerance)
    if bm is None:
        return None
    obj = lp.mesh_object(bm)
    obj.data.transform(Matrix.Translation(at))
    return tile(obj, PAINT_BLACK)


def transform(parts, matrix):
    for p in parts:
        if p is not None:
            p.data.transform(matrix)
    return parts


def strip_marks(rnd, width, z0, count, y, seed):
    """A few short dark strips on a board's face: a carved name worn illegible."""
    marks = []
    for k in range(count):
        w = width * rnd.uniform(0.35, 0.7)
        x = rnd.uniform(-0.5, 0.5) * (width - w) * 0.6
        mark = lp.block((w, 0.006, 0.014 + rnd.uniform(-0.003, 0.003)), (x, y - 0.002, z0 - k * 0.06), (0.0, rnd.uniform(-3, 3), 0.0))
        marks.append(lp.grain(mark, 'Beams', axis=(1.0, 0.0, 0.0), seed=seed + k, slot=3))
    return marks


def finish(obj, ao=0.35, lods='50,25', collision=None, smooth=35.0):
    """Shading, baked occlusion and the small-prop export settings: no Nanite, LODs."""
    lp.finish(obj, ao=ao, nanite=False, smooth=smooth)
    obj['LODs'] = lods
    if collision:
        obj['Collision'] = collision
    return obj


def socket(obj, name, at, turn=0.0):
    lm.socket(obj, name, at, (0.0, 0.0, turn))


# --- Board outlines (x across, z up, the foot buried) ---

def round_top(w, h, bury, crown=0.5, steps=5):
    r = w * 0.5
    shoulder = h - crown * r
    pts = [(-r, -bury), (r, -bury), (r, shoulder)]
    pts += [(r * math.cos(math.pi * k / steps), shoulder + crown * r * math.sin(math.pi * k / steps)) for k in range(1, steps)]
    return pts + [(-r, shoulder)]


def peak_top(w, h, bury, peak):
    r = w * 0.5
    return [(-r, -bury), (r, -bury), (r, h - peak), (0.0, h), (-r, h - peak)]


def shoulder_top(w, h, bury, step=0.07, crown=0.12):
    """The shoulders slope out a few millimetres to shed rain. (Level, their four corners would lie in one line, and
    the fill could join three of them into a flat triangle, which has no tangents.)"""
    r = w * 0.5
    s = h - step - crown
    return [(-r, -bury), (r, -bury), (r, s - 0.004), (r - step, s), (r - step, h - crown), (0.0, h),
            (-r + step, h - crown), (-r + step, s), (-r, s - 0.004)]


def broken_top(w, h, bury, rnd):
    r = w * 0.5
    return [(-r, -bury), (r, -bury), (r, h * 0.62), (r * 0.45, h * 0.7), (r * 0.15, h * 0.64), (-r * 0.2, h * 0.86),
            (-r * 0.55, h * 0.8), (-r, h * 0.92)]


# --- Headboards ---

def headboard(name, seed, outline, thick=0.045, lean=(0.0, 0.0), turn=0.0, marks=2, cleat=False, nails=True,
              split=False, fresh=False, painted=None, strip="Siding", slot=None):
    """A grave's headboard: one board (or two, split down the middle) with the given outline, a cleat across its back
    holding a split board together, nails, worn carving; leaning by lean (degrees: back, sideways)."""
    rnd = random.Random(seed)
    xs = [x for x, z in outline]
    w = max(xs) - min(xs)
    h = max(z for x, z in outline)
    parts = []
    if split:
        # Two boards, their meeting line a hair open.
        for side, sign in ((0, -1.0), (1, 1.0)):
            board = flat(clip_half(outline, sign), thick)
            parts.append(whitewash(board, seed + side) if fresh else wood(board, seed + side, strip=strip, slot=slot))
    else:
        board = flat(outline, thick)
        parts.append(whitewash(board, seed) if fresh else wood(board, seed, strip=strip, slot=slot))
    if cleat or split:
        c = lp.block((w * 0.86, 0.025, 0.09), (0.0, thick * 0.5 + 0.0125, h * 0.45))
        parts.append(wood(c, seed + 7, axis=(1.0, 0.0, 0.0)))
    if nails:
        for x in (-w * 0.3, w * 0.3):
            parts.append(lp.nail((x, -thick * 0.5, h * 0.45), (0.0, -1.0, 0.0), size=0.02))
    if marks:
        parts += strip_marks(rnd, w * 0.8, h * 0.68, marks, -thick * 0.5, seed + 20)
    if painted == 'cross':
        for size, at in (((0.035, 0.004, 0.3), (0.0, -thick * 0.5 - 0.002, h * 0.62)),
                         ((0.2, 0.004, 0.035), (0.0, -thick * 0.5 - 0.002, h * 0.68))):
            parts.append(tile(lp.block(size, at), PAINT_BLACK, seed + 30))
    elif painted == 'lines':
        for k, (wl, zl) in enumerate(((0.26, 0.66), (0.18, 0.58), (0.22, 0.5))):
            parts.append(tile(lp.block((wl, 0.003, 0.022), (rnd.uniform(-0.02, 0.02), -thick * 0.5 - 0.0015,
                                                                  h * zl)), PAINT_BLACK, seed + 40 + k))
    lean_m = Matrix.LocRotScale(None, Matrix.Rotation(math.radians(turn), 3, 'Z').to_quaternion() @
                                (Matrix.Rotation(math.radians(lean[1]), 3, 'Y') @
                                 Matrix.Rotation(math.radians(-lean[0]), 3, 'X')).to_quaternion(), None)
    transform(parts, lean_m)
    obj = lp.join(name, parts)
    hull = [lean_m @ Vector((x, y, z)) for x in (min(xs), max(xs)) for y in (-thick * 0.5 - 0.01, thick * 0.5 + 0.03)
            for z in (0.0, h)]
    lp.hull_points(obj, hull)
    socket(obj, 'Interact', lean_m @ Vector((0.0, -thick * 0.5 - 0.02, min(h * 0.6, 0.6))))
    return finish(obj)


def clip_half(outline, sign):
    """The half of an outline on one side of x = 0 (sign: -1 the left, +1 the right), the cut a hair open."""
    gap = 0.003 * sign
    out = []
    n = len(outline)
    for i in range(n):
        a, b = outline[i], outline[(i + 1) % n]
        if a[0] * sign >= 0.0:
            out.append(a if abs(a[0]) > 0.003 else (gap, a[1]))
        if (a[0] * sign >= 0.0) != (b[0] * sign >= 0.0):
            t = a[0] / (a[0] - b[0])
            out.append((gap, a[1] + (b[1] - a[1]) * t))
    return out


# --- Crosses ---

def cross_wood(name, seed):
    rnd = random.Random(seed)
    parts = []
    up = lp.block((0.1, 0.07, 1.32), (0.0, 0.0, 0.43))
    parts.append(wood(up, seed, strip='Beams'))
    arm = lp.block((0.58, 0.06, 0.09), (0.0, -0.045, 0.84))
    parts.append(wood(arm, seed + 1, axis=(1.0, 0.0, 0.0), strip='Beams'))
    for x in (-0.025, 0.025):
        parts.append(lp.nail((x, -0.075, 0.84), (0.0, -1.0, 0.0), size=0.022))
    parts += strip_marks(rnd, 0.3, 0.86, 1, -0.075, seed + 9)
    lean = Matrix.Rotation(math.radians(rnd.uniform(-6.0, 6.0)), 4, 'Y') @ Matrix.Rotation(math.radians(-5.0), 4, 'X')
    transform(parts, lean)
    obj = lp.join(name, parts)
    lp.hull_points(obj, [lean @ Vector((x, y, z)) for x in (-0.06, 0.06) for y in (-0.05, 0.05) for z in (0.0, 1.1)])
    lp.hull_points(obj, [lean @ Vector((x, y, z)) for x in (-0.3, 0.3) for y in (-0.08, 0.02) for z in (0.79, 0.89)])
    socket(obj, 'Interact', lean @ Vector((0.0, -0.1, 0.6)))
    return finish(obj)


def cross_iron(name, seed):
    """A wrought-iron cross with a ring round the crossing, set in a block of fieldstone."""
    parts = []
    base = lp.block((0.34, 0.26, 0.24), (0.0, 0.0, 0.07), bevel=0.02)
    parts.append(lp.trim(base, 'Stone', seed=seed))
    bar = 0.035
    parts.append(tile(lp.block((bar, bar, 0.95), (0.0, 0.0, 0.62)), IRON, seed))
    parts.append(tile(lp.block((0.46, bar, bar), (0.0, 0.0, 0.86)), IRON, seed + 1))
    for x, z in ((0.0, 1.11), (-0.245, 0.86), (0.245, 0.86)):   # trefoil ends
        parts.append(tile(lp.block((0.07, bar * 1.1, 0.07), (x, 0.0, z), (0.0, 45.0, 0.0)), IRON, seed + 2))
    ring_parts = []
    for k in range(10):
        a0, a1 = 2.0 * math.pi * k / 10, 2.0 * math.pi * (k + 1) / 10
        p0 = Vector((0.15 * math.cos(a0), 0.0, 0.86 + 0.15 * math.sin(a0)))
        p1 = Vector((0.15 * math.cos(a1), 0.0, 0.86 + 0.15 * math.sin(a1)))
        seg = lp.block(((p1 - p0).length + 0.012, 0.022, 0.022), (0.0, 0.0, 0.0))
        lp.place(seg, (p0 + p1) * 0.5, (0.0, -math.degrees(math.atan2(p1.z - p0.z, p1.x - p0.x)), 0.0))
        ring_parts.append(tile(seg, IRON, seed + 3))
    parts += ring_parts
    tilt = Matrix.Rotation(math.radians(3.0), 4, 'Y')
    transform(parts, tilt)
    obj = lp.join(name, parts)
    lp.hull_box(obj, (0.36, 0.28, 0.2), (0.0, 0.0, 0.1))
    lp.hull_points(obj, [tilt @ Vector((x, y, z)) for x in (-0.28, 0.28) for y in (-0.03, 0.03) for z in (0.2, 1.15)])
    socket(obj, 'Interact', (0.0, -0.18, 0.6))
    return finish(obj)


# --- Mounds ---

def mound_mesh(length, width, height, seed, nx=10, ny=6, sunk=0.0):
    """A mound of dirt (x across, y along the grave), rounded and lumpy, its skirt sunk 6 cm into the ground."""
    rnd = random.Random(seed)
    off = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
    bm = lp.new_bmesh()
    grid = []
    for i in range(nx + 1):
        row = []
        for j in range(ny + 1):
            u, v = i / nx * 2.0 - 1.0, j / ny * 2.0 - 1.0
            edge = max(abs(u), abs(v)) >= 0.999
            x, y = u * width * 0.5, v * length * 0.5
            shape = max(0.0, 1.0 - u * u) ** 0.75 * max(0.0, 1.0 - v * v) ** 0.45
            z = -0.06 if edge else height * shape - sunk * shape + 0.025 * noise.noise(Vector((x * 4.0, y * 4.0, 0.0)) + off)
            if not edge:   # an uneven grid, so the facets do not line up in rows
                x += 0.3 * width / nx * noise.noise(Vector((x * 6.0, y * 6.0, 1.0)) + off)
                y += 0.3 * length / ny * noise.noise(Vector((x * 6.0, y * 6.0, 7.0)) + off)
            row.append(bm.verts.new((x, y, z)))
        grid.append(row)
    for i in range(nx):
        for j in range(ny):
            bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='ALTERNATE')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for f in bm.faces:
        if f.normal.z < 0.0:
            f.normal_flip()
    obj = lp.mesh_object(bm)
    tile(obj, DIRT, seed)
    return obj


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def frost_shell(z_at, size, seed):
    """Rime on a mound of size (x, y) (z_at: its lc.surface_of()): a thin shell a few millimetres over the dirt
    (lc.draped_shell) inside the outline of a noise field, traced on a 1.5 cm grid and softened, so its edge never
    follows the mound's triangles. The field falls away from the crown, which stays whole but for a few holes; down
    the upper sides the noise breaks the edge into lobes, bays and holes, and past it small raised spots leave
    flecks, more of them near the edge and fewer down the flanks; none on the steep ends. The shell thickens inward
    from an edge only a hair over the dirt (0.8 mm to 6 mm) and shades with the mound's own smooth normals."""
    rnd = random.Random(seed)
    off = Vector((rnd.uniform(0.0, 100.0), rnd.uniform(0.0, 100.0), rnd.uniform(0.0, 100.0)))
    hx, hy = size[0] * 0.5, size[1] * 0.5
    gx, gy = hx + 0.025, hy + 0.025   # the grid, a little past the mound
    octaves = ((0.3, 3.0), (0.35, 6.0), (0.45, 10.0), (0.35, 15.0), (0.15, 21.0))   # (amount, waves a metre)

    def rime(x, y):
        p = Vector((x, y, 0.0))
        n = sum(a * noise.noise(p * k + off + Vector((0.0, 0.0, i * 7.0))) for i, (a, k) in enumerate(octaves))
        u, v = x / hx, y / hy
        d = math.hypot(u, 0.7 * v) + smoothstep(0.7, 0.9, abs(v))   # 0 on the crown; the steep ends far out
        n *= 0.5 + 0.5 * smoothstep(0.1, 0.45, d)                    # so the crown is nearly whole
        steep = smoothstep(0.55, 0.8, z_at.normal(x, y).z)
        return 0.45 - d + n - (1.0 - steep) * 1.5

    spots = []
    for _ in range(4000):
        if len(spots) >= 45:
            break
        x, y = rnd.uniform(-gx, gx), rnd.uniform(-gy, gy)
        z = z_at(x, y)
        if z is None or z < 0.02:
            continue
        v = rime(x, y)
        if not -0.65 < v < -0.12 or rnd.random() > smoothstep(-0.65, -0.12, v):
            continue
        if any((x - sx) ** 2 + (y - sy) ** 2 < 0.045 ** 2 for sx, sy, _, _ in spots):
            continue
        spots.append((x, y, rnd.uniform(0.02, 0.04), -v + rnd.uniform(0.1, 0.25)))   # (x, y, radius, height)

    def field(x, y):
        z = z_at(x, y)
        if z is None or z < 0.01:
            return -1.0
        v = rime(x, y)
        for sx, sy, r, a in spots:
            q = ((x - sx) ** 2 + (y - sy) ** 2) / (r * r)
            if q < 1.0:
                v += a * (1.0 - q) ** 2
        return v

    loops = []
    for loop in lc.contour_loops(field, gx, gy, 0.015):
        area = lc.loop_area(loop)
        if area > 0.0003 or area < -0.0006:   # no specks under 3 cm2, no pinholes under 6 cm2
            loop = lc.tidy_loop(loop, 0.002)
            if len(loop) >= 3:
                loops.append(loop)
    # Points inside carry the shell over the mound's curve, thicker inward: a coarse grid well inside the edge, and
    # the middle of every fleck big enough to dome.
    region = lc.bounded(loops)
    near = {}
    for p in (p for loop in loops for p in loop):
        near.setdefault((int(p.x // 0.03), int(p.y // 0.03)), []).append(p)

    def clear(p, r):
        cx, cy = int(p.x // 0.03), int(p.y // 0.03)
        return all((q - p).length >= r for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                   for q in near.get((cx + dx, cy + dy), ()))
    inner, step = [], 0.085
    for i in range(round(2.0 * gx / step) + 1):
        for j in range(round(2.0 * gy / step) + 1):
            p = Vector((-gx + i * step + (step * 0.5 if j % 2 else 0.0), -gy + j * step))
            v = field(p.x, p.y)
            if v > 0.08 and clear(p, 0.015) and lc.inside(region, p.x, p.y):
                inner.append((p, 0.0025 + 0.0035 * smoothstep(0.05, 0.4, v)))
    for loop in loops:
        if lc.loop_area(loop) > 0.0006:
            c = sum(loop, Vector((0.0, 0.0))) / len(loop)
            if lc.inside(lc.bounded([loop]), c.x, c.y) and clear(c, 0.008):
                inner.append((c, 0.0025))
    obj = lp.mesh_object(lc.draped_shell(z_at, loops, inner))
    for poly in obj.data.polygons:
        poly.use_smooth = True
    obj.data.normals_split_custom_set_from_vertices([z_at.normal(v.co.x, v.co.y) for v in obj.data.vertices])
    lt.assign(obj, FROST)
    lt.box_uv(obj, TILE[FROST], seed=seed)   # one projection for every face (no per-face turn), so no seams show
    return obj


def octahedron(size, center, rotation):
    """A small eight-sided lump (size x, y, z), turned then moved to center: a dead flower head, a clod of dirt."""
    bm = lp.new_bmesh()
    sx, sy, sz = (s * 0.5 for s in size)
    v = [bm.verts.new(p) for p in ((sx, 0.0, 0.0), (0.0, sy, 0.0), (-sx, 0.0, 0.0), (0.0, -sy, 0.0), (0.0, 0.0, sz),
                                   (0.0, 0.0, -sz))]
    for k in range(4):
        bm.faces.new((v[k], v[(k + 1) % 4], v[4]))
        bm.faces.new((v[(k + 1) % 4], v[k], v[5]))
    return lp.place(lp.mesh_object(bm), center, rotation)


def clods(rnd, count, area, z_fn, size=(0.05, 0.11)):
    """Lumps of dirt lying about."""
    parts = []
    for k in range(count):
        x, y = rnd.uniform(-area[0], area[0]), rnd.uniform(-area[1], area[1])
        s = rnd.uniform(*size)
        c = lp.block((s, s * rnd.uniform(0.7, 1.2), s * 0.6), (x, y, z_fn(x, y) + s * 0.15),
                     (rnd.uniform(-20, 20), rnd.uniform(-20, 20), rnd.uniform(0, 90)))
        lp.rough(c, s * 0.15, 9.0, rnd.randint(0, 999))
        parts.append(tile(c, DIRT, rnd.randint(0, 999)))
    return parts


def mound_fresh(name, seed):
    rnd = random.Random(seed)
    m = mound_mesh(1.95, 0.95, 0.3, seed)
    parts = [m] + clods(rnd, 5, (0.6, 1.15), lambda x, y: 0.0)
    obj = lp.join(name, parts)
    socket(obj, 'Respawn', (0.0, 0.0, 0.02))
    socket(obj, 'Interact', (0.0, -1.1, 0.4))
    return finish(obj, ao=0.4, collision='None')


# --- Coffins ---

COFFIN = [(-0.97, -0.17), (-0.35, -0.31), (0.97, -0.21), (0.97, 0.21), (-0.35, 0.31), (-0.97, 0.17)]   # head at -x


def coffin_parts(seed, lid=True, broken=False):
    """A toe-pincher pine coffin (head toward -x), weathered: the box, its lid, iron handles."""
    rnd = random.Random(seed)
    parts = []
    H = 0.4
    if not broken:
        body = extrude(COFFIN, 0.0, H)
        parts.append(wood(body, seed, axis=(1.0, 0.0, 0.0)))
    else:
        # Six side boards and a floor; one side stove in.
        n = len(COFFIN)
        for k in range(n):
            a, b = Vector(COFFIN[k] + (0.0,)), Vector(COFFIN[(k + 1) % n] + (0.0,))
            d = (b - a)
            length = d.length
            mid = (a + b) * 0.5
            normal = Vector((d.y, -d.x, 0.0)).normalized()
            board = lp.block((length + 0.02, 0.03, H), (0.0, 0.0, H * 0.5))
            wood(board, seed + k, axis=(1.0, 0.0, 0.0))
            yaw = math.degrees(math.atan2(d.y, d.x))
            tilt = (rnd.uniform(12.0, 22.0) if k == 2 else 0.0)
            lp.place(board, (0.0, 0.0, 0.0), (-tilt, 0.0, 0.0))
            lp.place(board, mid - normal * 0.015, (0.0, 0.0, yaw))
            parts.append(board)
        floor = extrude([(x * 0.97, y * 0.94) for x, y in COFFIN], 0.0, 0.03)
        parts.append(wood(floor, seed + 9, axis=(1.0, 0.0, 0.0)))
        dirt = extrude([(x * 0.85, y * 0.8) for x, y in COFFIN], 0.03, 0.09)
        lp.rough(dirt, 0.02, 6.0, seed)
        parts.append(tile(dirt, DIRT, seed))
    if lid:
        top = extrude([(x * 1.03, y * 1.06) for x, y in COFFIN], H, H + 0.035)
        parts.append(wood(top, seed + 3, axis=(1.0, 0.0, 0.0)))
    side_y = lambda x: 0.17 + (x + 0.97) / 0.62 * 0.14 if x < -0.35 else 0.31 - (x + 0.35) / 1.32 * 0.1
    for x in (-0.55, 0.35):
        for sy in (-1.0, 1.0):
            handle = lp.mesh_object(kit._box((0.18, 0.024, 0.035), drop=('+y',)))
            handle.data.transform(Matrix.Rotation(math.radians(180.0 if sy > 0 else 0.0), 4, 'Z'))
            handle.data.transform(Matrix.Translation((x, sy * (side_y(x) + 0.012), H * 0.56)))
            parts.append(tile(handle, IRON, seed + 5))
    return parts


def coffin_closed(name, seed):
    parts = coffin_parts(seed)
    obj = lp.join(name, parts)
    lp.hull_points(obj, [(x * 1.03, y * 1.06, z) for x, y in COFFIN for z in (0.0, 0.435)])
    return finish(obj, ao=0.3)


def lid_piece(points, seed, at, rot):
    piece = extrude(points, 0.0, 0.035)
    wood(piece, seed, axis=(1.0, 0.0, 0.0))
    return lp.place(piece, at, rot)


def coffin_broken(name, seed):
    rnd = random.Random(seed)
    parts = coffin_parts(seed, lid=False, broken=True)
    # The lid smashed in two: the head half lies across the box, askew; the foot half on the ground beside it.
    head = [(-1.0, -0.18), (-0.36, -0.33), (0.05, -0.31), (-0.12, 0.0), (0.12, 0.31), (-0.36, 0.33), (-1.0, 0.18)]
    foot = [(0.0, -0.3), (1.0, -0.22), (1.0, 0.22), (0.2, 0.3), (0.02, 0.02)]
    parts.append(lid_piece(head, seed + 11, (0.05, 0.12, 0.41), (rnd.uniform(-4, 4), 7.0, 14.0)))
    parts.append(lid_piece(foot, seed + 12, (0.45, -0.72, 0.0), (0.0, 0.0, -31.0)))
    for k in range(3):   # splinters
        s = lp.block((rnd.uniform(0.15, 0.3), 0.025, 0.012), (rnd.uniform(-0.5, 0.8), rnd.uniform(-0.75, -0.45), 0.006),
                     (0.0, 0.0, rnd.uniform(0.0, 180.0)))
        parts.append(wood(s, seed + 20 + k, axis=(1.0, 0.0, 0.0)))
    obj = lp.join(name, parts)
    lp.hull_points(obj, [(x, y, z) for x, y in COFFIN for z in (0.0, 0.42)])
    return finish(obj, ao=0.3)


# --- The named graves ---

def named_board(lines, seed, w, h, thick=0.05, frost=False):
    """A new whitewashed headboard (two boards) with a peaked top, a cleat behind, and black Rye lettering: lines are
    (text, size, z) from the top. frost rimes its top and edges."""
    rnd = random.Random(seed)
    outline = peak_top(w, h, 0.25, 0.14)
    parts = []
    for side, sign in ((0, -1.0), (1, 1.0)):
        board = flat(clip_half(outline, sign), thick)
        parts.append(whitewash(board, seed + side))
    cleat = lp.block((w * 0.9, 0.028, 0.09), (0.0, thick * 0.5 + 0.014, h * 0.3))
    parts.append(wood(cleat, seed + 5, axis=(1.0, 0.0, 0.0)))
    for x in (-w * 0.32, w * 0.32):
        parts.append(lp.nail((x, -thick * 0.5, h * 0.3), (0.0, -1.0, 0.0), size=0.02))
    for text, size, z in lines:
        parts.append(lettering(text, size, (0.0, -thick * 0.5 - 0.0012, z), tolerance=0.05 if size >= 0.06 else 0.065))
    if frost:
        # Rime along the top and down the edges, thickest on the peak, ragged underneath.
        pts = outline[2:]
        cap = [(x * 1.02, z + 0.01) for x, z in pts]
        under = [(x * 0.98, z - 0.035 - 0.025 * rnd.random()) for x, z in reversed(pts)]
        rim = flat(cap + under, thick + 0.016)
        parts.append(tile(rim, FROST, seed + 7))
        for sx in (-1.0, 1.0):
            length = h * rnd.uniform(0.45, 0.6)
            edge = lp.block((0.016, thick + 0.012, length), (sx * (w * 0.5 + 0.004), 0.0, h - 0.14 - length * 0.5))
            parts.append(tile(edge, FROST, seed + 8))
    return parts


def grave_ellis(name, seed):
    """Ellis's grave, dug open from inside a week after the burial: dirt heaped all round, the coffin's lid smashed up
    through it, the headboard knocked askew."""
    rnd = random.Random(seed)
    parts = []
    # The spoil: the grave's dirt shovelled out round the hole and thrown back up through, heaped high on the left
    # (-x), low at the foot where Ellis crawled out, lumpy at every scale, its edge running out raggedly under the
    # grass. Rings from the hollow's lip (t = 0) to the outer edge (t = 1): a steep inner wall, a rounded crest, a long
    # outer slope.
    bm = lp.new_bmesh()
    rings = []
    seg = 36
    profile = ((0.0, 0.03), (0.1, 0.17), (0.21, 0.28), (0.3, 0.33), (0.39, 0.33), (0.5, 0.28), (0.62, 0.19),
               (0.75, 0.1), (0.88, 0.035), (1.0, -0.02))
    last = len(profile) - 1
    for r_k, (t, z) in enumerate(profile):
        ring = []
        for k in range(seg):
            a = 2.0 * math.pi * k / seg
            c, s = math.cos(a), math.sin(a)
            p = Vector((c * 2.0, s * 2.0, seed))
            # The outer rings wander in lobes: tongues of dirt spilled onto the grass.
            wob = 1.0 + 0.08 * noise.noise(p) + 0.5 * max(0.0, t - 0.55) * noise.noise(p * 2.3 + Vector((0.0, 0.0, 3.0)))
            x, y = (0.6 + 0.8 * t) * c * wob, (1.08 + 0.8 * t) * s * wob
            zz = z
            if 0 < r_k < last:
                pile = 1.0 + 0.5 * math.cos(a - math.pi) + 0.3 * noise.noise(Vector((c * 3.0, s * 3.0, 4.0 + seed)))
                foot = 1.0 - 0.6 * smoothstep(-0.3, -0.95, s)
                lumps = (0.08 * noise.noise(Vector((x * 3.5, y * 3.5, seed + 11.0)))
                         + 0.045 * noise.noise(Vector((x * 8.0, y * 8.0, seed + 17.0))))
                zz = max(0.01, z * max(0.25, pile) * foot + lumps * min(1.0, z / 0.2))
            ring.append(bm.verts.new((x, y, zz)))
        rings.append(ring)
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(seg):
            bm.faces.new((r0[k], r0[(k + 1) % seg], r1[(k + 1) % seg], r1[k]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for f in bm.faces:
        f.normal_update()
    if sum(f.normal.z for f in bm.faces) < 0.0:
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    heap = tile(lp.mesh_object(bm), DIRT, seed)
    parts.append(heap)
    z_at = lc.surface_of(heap)
    # Loose dirt over the bottom of the hollow (so no grass shows round the coffin).
    fb = lp.new_bmesh()
    fb.faces.new([fb.verts.new((0.64 * math.cos(2.0 * math.pi * k / 24), 1.14 * math.sin(2.0 * math.pi * k / 24), 0.015))
                  for k in range(24)])
    parts.append(tile(lp.mesh_object(fb), DIRT, seed + 1))
    # Clods: lumps lying on the heap, half sunk in it, and crumbs thrown out onto the grass; tipped well over, so an
    # edge or a face is uppermost instead of a peak.
    for k in range(36):
        a = rnd.uniform(0.0, 2.0 * math.pi)
        if k < 22:
            t, size = rnd.uniform(0.05, 0.85), rnd.uniform(0.07, 0.2)
        else:
            t, size = rnd.uniform(1.02, 1.4), rnd.uniform(0.03, 0.07)
        x, y = (0.6 + 0.8 * t) * math.cos(a), (1.08 + 0.8 * t) * math.sin(a)
        ground = z_at(x, y) if t < 1.0 else None
        lump = octahedron((size, size * rnd.uniform(0.7, 1.1), size * rnd.uniform(0.55, 0.8)),
                          (x, y, (ground if ground is not None else 0.0) + size * 0.12),
                          (rnd.uniform(-60, 60), rnd.uniform(-60, 60), rnd.uniform(0, 180)))
        lp.rough(lump, size * 0.18, 12.0, seed + 30 + k)
        parts.append(tile(lump, DIRT, seed + 30 + k))
    # A few stones turned up with the dirt.
    for k in range(6):
        a = rnd.uniform(0.0, 2.0 * math.pi)
        t = rnd.uniform(0.15, 0.9)
        x, y = (0.6 + 0.8 * t) * math.cos(a), (1.08 + 0.8 * t) * math.sin(a)
        r, h = rnd.uniform(0.045, 0.08), rnd.uniform(0.05, 0.08)
        stone, _ = lp.lathe([(r, -0.03), (r * 1.05, h * 0.45), (r * 0.6, h), (0.0, h * 1.08)], segments=5)
        stone.data.transform(Matrix.Diagonal((1.0, rnd.uniform(0.6, 0.85), 1.0, 1.0)))
        lp.rough(stone, 0.012, 14.0, seed + 50 + k)
        lp.place(stone, (x, y, (z_at(x, y) or 0.0) - 0.02), (rnd.uniform(-15, 15), rnd.uniform(-15, 15), rnd.uniform(0, 180)))
        parts.append(lp.trim(stone, 'Stone', seed=seed + 50 + k))
    # The coffin in the hollow (head toward the headboard, +Y), open, a side stove in, its lid smashed up and out.
    parts += transform(coffin_parts(seed + 3, lid=False, broken=True), Matrix.Rotation(math.radians(90.0), 4, 'Z'))
    head = [(-1.0, -0.18), (-0.36, -0.33), (0.05, -0.31), (-0.12, 0.0), (0.12, 0.31), (-0.36, 0.33), (-1.0, 0.18)]
    lid = lid_piece(head, seed + 11, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0))
    lp.place(lid, (0.0, 0.0, 0.0), (0.0, -34.0, 82.0))
    lp.place(lid, (0.62, 0.35, 0.36))
    parts.append(lid)
    # The headboard at the head (+Y), knocked back and askew.
    board = named_board([('ELLIS RANSOM', 0.072, 0.62), ('CAME HOME', 0.05, 0.5), ('AT THE LAST', 0.05, 0.43)],
                        seed + 20, 0.56, 0.86)
    board_m = Matrix.Translation((0.1, 1.95, 0.0)) @ Matrix.Rotation(math.radians(-7.0), 4, 'Z') @ \
        Matrix.Rotation(math.radians(5.0), 4, 'Y') @ Matrix.Rotation(math.radians(-12.0), 4, 'X')
    transform(board, board_m)
    parts += board
    obj = lp.join(name, parts)
    lp.hull_points(obj, [board_m @ Vector((x, y, z)) for x in (-0.29, 0.29) for y in (-0.04, 0.06) for z in (0.0, 0.86)])
    for x, y, sx, sy in ((0.0, 1.45, 1.3, 0.4), (0.0, -1.45, 1.3, 0.4), (0.95, 0.0, 0.4, 2.2), (-0.95, 0.0, 0.4, 2.2)):
        lp.hull_box(obj, (sx, sy, 0.3), (x, y, 0.15))
    socket(obj, 'Respawn', (0.0, -0.1, 0.12))
    socket(obj, 'Loot', (0.0, 0.1, 0.16))
    socket(obj, 'Interact', board_m @ Vector((0.0, -0.06, 0.55)))
    return finish(obj, ao=0.45)


def grave_abel(name, seed):
    """Abel's grave beside Ellis's: the mound whole, frost lying on it in the afternoon sun, the flowers on it gone
    black; the headboard rimed with frost."""
    rnd = random.Random(seed)
    mound = mound_mesh(1.95, 0.95, 0.28, seed, nx=10, ny=8)
    z_at = lc.surface_of(mound)
    parts = [mound, frost_shell(z_at, (0.95, 1.95), seed + 3)]
    # Dead flowers: a tied bunch laid on the mound over the frost, the stems fanning out from the tie to small
    # shrivelled heads, all gone black.
    tx0, ty0 = 0.05, -0.1
    tie_z = (z_at(tx0, ty0) or 0.27) + 0.01
    for k in range(6):
        a = math.radians(-30.0 + k * 12.0 + rnd.uniform(-4.0, 4.0))
        length = rnd.uniform(0.16, 0.24)
        hx, hy = tx0 + math.sin(a) * length, ty0 - math.cos(a) * length
        base = Vector((tx0 + rnd.uniform(-0.01, 0.01), ty0 + rnd.uniform(-0.01, 0.01), tie_z))
        tip = Vector((hx, hy, (z_at(hx, hy) or 0.25) + 0.012))
        stem = lp.sweep([base, base.lerp(tip, 0.5) + Vector((0.0, 0.0, 0.012)), tip], lp.ngon(0.004, 4))
        parts.append(tile(stem, PAINT_BLACK, seed + k))
        for j, size in enumerate((0.024, 0.016)):
            at = tip + Vector((rnd.uniform(-0.012, 0.012), rnd.uniform(-0.012, 0.012), 0.005))
            head = octahedron((size * 1.4, size * 1.1, size), at, (rnd.uniform(0, 40), rnd.uniform(0, 40), rnd.uniform(0, 90)))
            lp.rough(head, size * 0.2, 30.0, seed + 10 + k * 2 + j)
            parts.append(tile(head, PAINT_BLACK, seed + 10 + k * 2 + j))
    board = named_board([('ABEL RANSOM', 0.068, 0.7), ('KEEPER OF', 0.048, 0.58), ('SAINT ADA', 0.048, 0.51),
                         ('HE HELD THE DOOR', 0.038, 0.42)], seed + 20, 0.6, 0.92, frost=True)
    board_m = Matrix.Translation((0.0, 1.05, 0.0)) @ Matrix.Rotation(math.radians(2.0), 4, 'Y')
    transform(board, board_m)
    parts += board
    obj = lp.join(name, parts)
    lp.hull_points(obj, [board_m @ Vector((x, y, z)) for x in (-0.31, 0.31) for y in (-0.04, 0.06) for z in (0.0, 0.92)])
    socket(obj, 'Interact', board_m @ Vector((0.0, -0.06, 0.55)))
    finish(obj, ao=0.4)
    return lc.fade_open_edges(obj, FROST, 0.55)   # the rime's edge thins to grey over the dirt


def grave_keeper(name, seed):
    """The keeper's grave at the crossroads: a heavy oak board between two posts under a little shingled roof, a
    lantern carved in it and an iron hook for the keeper's lantern; a kerb of fieldstones round a sunken mound."""
    rnd = random.Random(seed)
    parts = []
    by = 1.1   # the board stands at the grave's head (+Y)
    for x in (-0.42, 0.42):
        post = lp.block((0.13, 0.13, 1.45), (x, by, 0.55))
        parts.append(wood(post, seed + int(x * 10), strip='Beams'))
    board = flat(shoulder_top(0.72, 1.05, 0.0, step=0.05, crown=0.1), 0.07, y=by)
    lp.place(board, (0.0, 0.0, 0.18))
    parts.append(wood(board, seed + 3, axis=(0.0, 0.0, 1.0), strip='Beams'))
    # The roof: two short boards meeting over the posts, an oak ridge.
    for sx in (-1.0, 1.0):
        roof = lp.block((0.56, 0.36, 0.03), (sx * 0.24, by, 1.42), (0.0, sx * 32.0, 0.0))
        parts.append(lp.trim(roof, 'Shingles', seed=seed + 5))
    ridge = lp.block((0.06, 0.38, 0.06), (0.0, by, 1.57), (0.0, 45.0, 0.0))
    parts.append(wood(ridge, seed + 6, axis=(0.0, 1.0, 0.0), strip='Beams'))
    # A lantern burnt into the board: its ring, a pitched cap, the body's frame and glass bar, a wide foot.
    fy = by - 0.035 - 0.003
    grooves = [((0.035, 0.03), (0.0, 1.0)), ((0.13, 0.024), (-0.055, 0.935), -30.0), ((0.13, 0.024), (0.055, 0.935), 30.0),
               ((0.2, 0.026), (0.0, 0.885)), ((0.026, 0.22), (-0.085, 0.765)), ((0.026, 0.22), (0.085, 0.765)),
               ((0.014, 0.2), (0.0, 0.765)), ((0.2, 0.026), (0.0, 0.645)), ((0.26, 0.03), (0.0, 0.61))]
    for groove in grooves:
        (gw, gh), (gx, gz) = groove[0], groove[1]
        g = lp.block((gw, 0.008, gh), (gx, fy, gz), (0.0, groove[2] if len(groove) > 2 else 0.0, 0.0))
        parts.append(lp.grain(g, 'Beams', axis=(1.0, 0.0, 0.0) if gw > gh else (0.0, 0.0, 1.0), seed=seed + 9,
                              slot=3))
    # The hook, an iron strap bent out from the right post.
    hook = lp.block((0.3, 0.025, 0.035), (0.42 + 0.17, by - 0.0, 1.1))
    parts.append(lp.grain(hook, 'Iron', axis=(1.0, 0.0, 0.0), seed=seed + 10))
    tip = lp.block((0.025, 0.025, 0.08), (0.72, by, 1.07))
    parts.append(lp.grain(tip, 'Iron', axis=(0.0, 0.0, 1.0), seed=seed + 11))
    # The kerb: fieldstones round the plot, and the sunken mound inside it.
    for k in range(14):
        t = k / 14.0
        if t < 0.2:
            x, y = -0.52 + t / 0.2 * 1.04, -0.95
        elif t < 0.5:
            x, y = 0.52, -0.95 + (t - 0.2) / 0.3 * 1.9
        elif t < 0.7:
            x, y = 0.52 - (t - 0.5) / 0.2 * 1.04, 0.95
        else:
            x, y = -0.52, 0.95 - (t - 0.7) / 0.3 * 1.9
        # A rounded fieldstone, half sunk: a squat lathe, squashed and roughed up.
        s, hgt = rnd.uniform(0.09, 0.13), rnd.uniform(0.09, 0.13)
        stone, _ = lp.lathe([(s, -0.04), (s * 1.05, hgt * 0.45), (s * 0.6, hgt), (0.0, hgt * 1.08)],
                            segments=5)
        stone.data.transform(Matrix.Diagonal((1.0, rnd.uniform(0.65, 0.85), 1.0, 1.0)))
        lp.rough(stone, 0.018, 11.0, seed + k)
        lp.place(stone, (x + rnd.uniform(-0.025, 0.025), y + rnd.uniform(-0.02, 0.02), 0.0), (0.0, 0.0, rnd.uniform(0, 180)))
        parts.append(tile(stone, GRANITE, seed + k))
    parts.append(mound_mesh(1.7, 0.95, 0.12, seed + 40, nx=6, ny=6))
    obj = lp.join(name, parts)
    for x in (-0.42, 0.42):
        lp.hull_box(obj, (0.14, 0.14, 1.45), (x, by, 0.73))
    lp.hull_box(obj, (0.72, 0.08, 0.95), (0.0, by, 0.66))
    socket(obj, 'Respawn', (0.0, -0.1, 0.05))
    socket(obj, 'Interact', (0.0, by - 0.1, 0.75))
    socket(obj, 'Lantern', (0.72, by, 1.02))   # under the hook's tip: a lantern's hanging point goes here
    return finish(obj, ao=0.4)


# --- Fences ---

def spear(x, z, size, seed):
    """A spear tip on a picket: a four-sided point, open underneath (it sits on the picket's end)."""
    bm = lp.new_bmesh()
    r = size * 1.1
    base = [bm.verts.new((x + r * math.cos(a), r * math.sin(a), z)) for a in (0.0, math.pi * 0.5, math.pi, math.pi * 1.5)]
    apex = bm.verts.new((x, 0.0, z + size * 3.2))
    for k in range(4):
        bm.faces.new((base[k], base[(k + 1) % 4], apex))
    return tile(lp.mesh_object(bm), IRON, seed)


def rod(x, y, z0, z1, size, mat, seed, ends=False):
    """An upright bar of square section, its ends left open (in the ground, under a cap or a point)."""
    obj = lp.mesh_object(kit._box((size, size, z1 - z0), drop=() if ends else ('-z', '+z')))
    obj.data.transform(Matrix.Translation((x, y, (z0 + z1) * 0.5)))
    return tile(obj, mat, seed)


def iron_post(x, height, size, seed, cap=True):
    parts = [tile(lp.block((size, size, height + 0.12), (x, 0.0, (height - 0.12) * 0.5)), IRON, seed)]
    if cap:
        parts.append(tile(lp.block((size * 1.6, size * 1.6, 0.03), (x, 0.0, height + 0.015)), IRON, seed + 1))
        ball, _ = lp.lathe([(0.0, 0.0), (size * 0.5, 0.01), (size * 0.75, size * 0.7), (size * 0.5, size * 1.3),
                            (0.0, size * 1.45)], segments=6)
        lp.place(ball, (x, 0.0, height + 0.03))
        parts.append(tile(ball, IRON, seed + 2))
    return parts


def iron_pickets(x0, x1, z0, top, step, seed, rails=(0.14, 0.98)):
    parts = []
    for z in rails:
        rail = lp.mesh_object(kit._box((x1 - x0, 0.014, 0.04), drop=('-x', '+x')))
        rail.data.transform(Matrix.Translation(((x0 + x1) * 0.5, 0.0, z)))
        parts.append(tile(rail, IRON, seed))
    n = max(1, int(round((x1 - x0) / step)))
    for k in range(n):
        x = x0 + (k + 0.5) * (x1 - x0) / n
        parts.append(rod(x, 0.0, z0, top, 0.017, IRON, seed + k))
        parts.append(spear(x, top, 0.017, seed + k))
    return parts


def fence_iron_section(name, seed):
    parts = iron_post(0.0, 1.18, 0.055, seed) + iron_pickets(0.0, 2.0, -0.05, 1.08, 0.14, seed + 3)
    obj = lp.join(name, parts)
    lp.hull_box(obj, (2.0, 0.06, 1.2), (1.0, 0.0, 0.6))
    return finish(obj, ao=0.25)


def fence_iron_post(name, seed):
    parts = iron_post(0.0, 1.32, 0.1, seed)
    base = lp.block((0.22, 0.22, 0.22), (0.0, 0.0, 0.04), bevel=0.015)
    parts.append(lp.trim(base, 'Stone', seed=seed))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (0.22, 0.22, 1.4), (0.0, 0.0, 0.7))
    return finish(obj, ao=0.25)


def fence_iron_gate(name, seed):
    """A double gate between heavy posts at x = 0 and x = 2 m, the leaves hung on them standing open inward."""
    parts = iron_post(0.0, 1.5, 0.1, seed) + iron_post(2.0, 1.5, 0.1, seed + 3)
    for x in (0.0, 2.0):
        base = lp.block((0.22, 0.22, 0.22), (x, 0.0, 0.04), bevel=0.015)
        parts.append(lp.trim(base, 'Stone', seed=seed + int(x)))
    leaves = []
    for hinge, sign, angle in ((0.06, 1.0, 72.0), (1.94, -1.0, -58.0)):
        w = 0.9
        leaf = []
        for z in (0.12, 0.62, 1.1):
            leaf.append(tile(lp.block((w, 0.016, 0.035), (w * 0.5, 0.0, z)), IRON, seed + 10))
        for x in (0.02, w - 0.02):
            leaf.append(rod(x, 0.0, 0.04, 1.16, 0.03, IRON, seed + 11, ends=True))
        for k in range(6):
            x = (k + 0.5) * w / 6
            leaf.append(rod(x, 0.0, 0.12, 1.12, 0.016, IRON, seed + 12 + k))
            leaf.append(spear(x, 1.12, 0.016, seed + 20 + k))
        m = Matrix.Translation((hinge, 0.02, 0.0)) @ Matrix.Rotation(math.radians(angle), 4, 'Z') @ \
            Matrix.Diagonal((sign, 1.0, 1.0, 1.0))
        transform(leaf, m)
        if sign < 0:   # mirrored: turn its faces back out
            for p in leaf:
                fb = bmesh.new()
                fb.from_mesh(p.data)
                bmesh.ops.reverse_faces(fb, faces=fb.faces[:])
                fb.to_mesh(p.data)
                fb.free()
        parts += leaf
        leaves.append(m)
    obj = lp.join(name, parts)
    for x in (0.0, 2.0):
        lp.hull_box(obj, (0.22, 0.22, 1.6), (x, 0.0, 0.8))
    for m in leaves:
        lp.hull_points(obj, [m @ Vector((x, y, z)) for x in (0.0, 0.9) for y in (-0.03, 0.03) for z in (0.05, 1.2)])
    return finish(obj, ao=0.25)


PICKET = [(-0.036, -0.08), (0.036, -0.08), (0.036, 0.9), (0.0, 0.97), (-0.036, 0.9)]


def pickets(x0, x1, seed, front=-0.03, step=0.14, rnd=None):
    parts = []
    n = max(1, int(round((x1 - x0) / step)))
    for k in range(n):
        x = x0 + (k + 0.5) * (x1 - x0) / n
        p = flat([(px, pz * (1.0 + (rnd.uniform(-0.03, 0.03) if rnd else 0.0))) for px, pz in PICKET], 0.022, y=front)
        lp.place(p, (x, 0.0, 0.0), (0.0, rnd.uniform(-1.5, 1.5) if rnd else 0.0, 0.0))
        parts.append(whitewash(p, seed + k))
    return parts


def picket_post(x, seed, height=1.05):
    post = lp.block((0.09, 0.09, height + 0.15), (x, 0.0, (height - 0.15) * 0.5))
    for v in post.data.vertices:
        if v.co.z > height - 0.05:
            v.co.z += 0.03 * (v.co.y / 0.045)
    return wood(post, seed)


def picket_rails(x0, x1, seed):
    parts = []
    for z in (0.22, 0.7):
        rail = lp.block((x1 - x0, 0.035, 0.085), ((x0 + x1) * 0.5, 0.0, z))
        parts.append(wood(rail, seed + int(z * 10), axis=(1.0, 0.0, 0.0)))
    return parts


def fence_picket_section(name, seed):
    rnd = random.Random(seed)
    parts = [picket_post(0.0, seed)] + picket_rails(0.0, 2.0, seed + 1) + pickets(0.06, 1.98, seed + 5, rnd=rnd)
    obj = lp.join(name, parts)
    lp.hull_box(obj, (2.0, 0.1, 1.05), (1.0, -0.005, 0.52))
    return finish(obj, ao=0.25)


def fence_picket_post(name, seed):
    obj = lp.join(name, [picket_post(0.0, seed)])
    lp.hull_box(obj, (0.1, 0.1, 1.08), (0.0, 0.0, 0.54))
    return finish(obj, ao=0.25)


def fence_picket_gate(name, seed):
    """A gate leaf hung on the post at x = 0, standing ajar inward; its latch post at 1.05 m, pickets on to 2 m."""
    rnd = random.Random(seed)
    parts = [picket_post(0.0, seed), picket_post(1.05, seed + 1, height=1.12)]
    parts += picket_rails(1.05, 2.0, seed + 2) + pickets(1.1, 1.98, seed + 5, rnd=rnd, step=0.14)
    leaf = []
    w = 0.94
    for z in (0.22, 0.7):
        rail = lp.block((w, 0.035, 0.085), (w * 0.5 + 0.03, 0.0, z))
        leaf.append(wood(rail, seed + 30 + int(z * 10), axis=(1.0, 0.0, 0.0)))
    brace = lp.block((0.7, 0.03, 0.07), (w * 0.5 + 0.03, 0.0, 0.46), (0.0, -34.0, 0.0))
    leaf.append(wood(brace, seed + 40, axis=(1.0, 0.0, 0.0)))
    leaf += pickets(0.06, w, seed + 50, rnd=rnd, step=0.135)
    leaf.append(lp.grain(lp.block((0.16, 0.012, 0.05), (0.08, -0.04, 0.62)), 'Iron', axis=(1.0, 0.0, 0.0), seed=seed))
    m = Matrix.Translation((0.05, 0.0, 0.03)) @ Matrix.Rotation(math.radians(34.0), 4, 'Z')
    transform(leaf, m)
    parts += leaf
    obj = lp.join(name, parts)
    lp.hull_box(obj, (0.1, 0.1, 1.08), (0.0, 0.0, 0.54))
    lp.hull_box(obj, (1.0, 0.1, 1.05), (1.52, -0.005, 0.52))
    lp.hull_points(obj, [m @ Vector((x, y, z)) for x in (0.0, w) for y in (-0.05, 0.03) for z in (0.0, 1.0)])
    socket(obj, 'Interact', m @ Vector((0.5, -0.12, 0.7)))
    return finish(obj, ao=0.25)


# --- Mourning ---

def wreath(name, seed):
    """A wreath of dark evergreen sprigs round a core, a black crepe bow with tails at its foot. It hangs from its
    pivot (the nail, 3 cm above the ring's top), its back against the door."""
    rnd = random.Random(seed)
    R, cz = 0.2, -0.23
    bm = lp.new_bmesh()
    rings = []
    for i in range(12):
        a = 2.0 * math.pi * i / 12
        radial = Vector((math.cos(a), 0.0, math.sin(a)))
        c = Vector((0.0, -0.045, cz)) + radial * R
        rings.append([bm.verts.new(c + radial * (0.045 * math.cos(b)) + Vector((0.0, -0.04 * math.sin(b), 0.0)))
                      for b in (math.pi * (0.25 + 0.5 * j) for j in range(4))])
    for i in range(12):
        r0, r1 = rings[i], rings[(i + 1) % 12]
        for j in range(4):
            bm.faces.new((r0[j], r1[j], r1[(j + 1) % 4], r0[(j + 1) % 4]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    core = lp.mesh_object(bm)
    lt.assign(core, LEAVES)
    parts = [core]
    sprigs = []
    for k in range(54):
        a = 2.0 * math.pi * (k + rnd.uniform(-0.3, 0.3)) / 54
        radial = Vector((math.cos(a), 0.0, math.sin(a)))
        tangent = Vector((-math.sin(a), 0.0, math.cos(a)))
        ring_r = R + (0.03, -0.028, 0.0)[k % 3] + rnd.uniform(-0.008, 0.008)
        base = Vector((0.0, -0.085 - rnd.uniform(0.0, 0.02), cz)) + radial * ring_r
        tip = base + tangent * rnd.uniform(0.075, 0.095) + radial * rnd.uniform(-0.015, 0.02) + Vector((0.0, -0.02, 0.0))
        mid = base.lerp(tip, 0.45) + Vector((0.0, -0.014, 0.0))
        side = radial * rnd.uniform(0.02, 0.026)
        sb = lp.new_bmesh()
        sb.faces.new([sb.verts.new(p) for p in (base, mid - side, tip, mid + side)])
        sprig = lp.mesh_object(sb)
        lt.assign(sprig, LEAVES)
        sprigs.append(sprig)
    parts += sprigs
    # The nail it hangs from.
    parts.append(tile(lp.block((0.016, 0.03, 0.016), (0.0, -0.01, 0.0)), IRON, seed))
    # The bow at the foot of the ring: two loops, a knot, two tails.
    bz = cz - R - 0.02
    for sx in (-1.0, 1.0):
        loop = lp.sweep([Vector((0.0, -0.13, bz)), Vector((sx * 0.07, -0.15, bz + 0.05)), Vector((sx * 0.13, -0.14, bz)),
                         Vector((sx * 0.07, -0.15, bz - 0.04)), Vector((0.0, -0.13, bz))],
                        [(-0.03, 0.0), (0.03, 0.0), (0.03, 0.008), (-0.03, 0.008)], up=(0.0, -1.0, 0.0))
        parts.append(tile(loop, CREPE, seed + 1))
        tail = lp.sweep([Vector((sx * 0.01, -0.13, bz)), Vector((sx * 0.05, -0.135, bz - 0.12)),
                         Vector((sx * 0.08, -0.13, bz - 0.24))], [(-0.024, 0.0), (0.024, 0.0), (0.024, 0.006),
                                                                  (-0.024, 0.006)], up=(0.0, -1.0, 0.0))
        parts.append(tile(tail, CREPE, seed + 2))
    knot = lp.block((0.05, 0.03, 0.05), (0.0, -0.145, bz), bevel=0.01)
    parts.append(tile(knot, CREPE, seed + 3))
    obj = lp.join(name, parts)
    # The evergreen: the darkest ends of the palette's deep greens (never a loot green), no wind.
    leaf_faces = [p.index for p in obj.data.polygons if obj.data.materials[p.material_index] == LEAVES]
    lt.swatch_uv(obj, leaf_faces, 'GrassDeep', axis='long')
    uv = obj.data.uv_layers.active.data
    for i in leaf_faces:
        for li in obj.data.polygons[i].loop_indices:
            uv[li].uv.x *= 0.35
    # The ring under the sprigs (the joined mesh's first 48 faces and vertices: 12 sections of 4) is mapped across its
    # long axis, where some of its faces come out with no UV area (no tangents in Unreal). Spread each face across
    # the swatch instead, by the corner's place round the ring's section and a nudge every other section; the colors
    # stay those of the same dark end of the swatch.
    rows = len(lt.PALETTE)
    v_lo, v_hi = (lt.PALETTE.index('GrassDeep') + 0.15) / rows, (lt.PALETTE.index('GrassDeep') + 0.85) / rows
    for poly in obj.data.polygons[:48]:
        for li in poly.loop_indices:
            vi = obj.data.loops[li].vertex_index
            section, corner = divmod(vi, 4)
            uv[li].uv.x += 0.01 * (section % 2)
            uv[li].uv.y = v_lo + (v_hi - v_lo) * (0.15 + 0.2 * corner + 0.1 * (section % 2))
    finish(obj, ao=0.15, collision='None', smooth=60.0)
    col = lt._read_col(obj.data)
    col[:, 0] = 0.0          # no sway
    col[:, 1] = 0.5
    lt._write_col(obj.data, col)
    return obj


def crepe_swag(name, seed, width=1.3, sag=0.22):
    """Black crepe draped across a door's head: one swag of cloth gathered at two rosettes, bellying out and sagging
    in the middle with folds along the drape, and two ribbon tails hanging from each rosette. Its pivot is the middle
    of its top, its back against the wall."""
    rnd = random.Random(seed)
    half = width * 0.5
    cols, rows = 12, 5
    bm = lp.new_bmesh()
    layers = []
    for layer, dy in ((0, 0.0), (1, 0.006)):
        grid = []
        for i in range(cols + 1):
            u = i / cols
            belly = math.sin(math.pi * u)
            top, bottom = -0.035 - 0.04 * belly, -0.07 - sag * belly
            col = []
            for j in range(rows + 1):
                v = j / rows
                fold = 0.012 * math.sin(2.0 * math.pi * (v * 2.5 + 0.15)) * belly
                y = -0.012 - 0.035 * belly * math.sin(math.pi * (0.25 + 0.75 * v)) - fold + dy
                col.append(bm.verts.new((-half + width * u, y, top + (bottom - top) * v)))
            grid.append(col)
        for i in range(cols):
            for j in range(rows):
                quad = (grid[i][j], grid[i][j + 1], grid[i + 1][j + 1], grid[i + 1][j])
                bm.faces.new(quad if layer == 0 else quad[::-1])
        layers.append(grid)
    bm.normal_update()
    if list(bm.faces)[0].normal.y > 0.0:      # the front layer faces out (-Y), the back layer the wall
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    parts = [tile(lp.mesh_object(bm), CREPE, seed)]
    for sx in (-1.0, 1.0):
        x = sx * half
        # A rosette of gathered crepe and its button.
        rose, _ = lp.lathe([(0.0, 0.016), (0.035, 0.012), (0.075, 0.0), (0.075, -0.006), (0.0, -0.006)], segments=8)
        lp.place(rose, (0.0, 0.0, 0.0), (90.0, 0.0, 22.5))
        lp.place(rose, (x, -0.03, -0.05))
        parts.append(tile(rose, CREPE, seed + 1))
        button = lp.block((0.04, 0.02, 0.04), (x, -0.05, -0.05), (0.0, 45.0, 0.0), bevel=0.008)
        parts.append(tile(button, CREPE, seed + 2))
        # Two ribbon tails against the wall, one a little longer.
        for k, dx in enumerate((-0.022, 0.026)):
            z_end = -0.42 - 0.07 * k
            path = [Vector((x + dx, -0.03, -0.08)), Vector((x + dx * 1.5 + rnd.uniform(-0.01, 0.01), -0.035, -0.22)),
                    Vector((x + dx * 1.8 + rnd.uniform(-0.015, 0.015), -0.03, z_end))]
            tail = lp.sweep(path, [(-0.028, 0.0), (0.028, 0.0), (0.028, 0.004), (-0.028, 0.004)], up=(0.0, -1.0, 0.0))
            parts.append(tile(tail, CREPE, seed + 3 + k))
    obj = lp.join(name, parts)
    return finish(obj, ao=0.2, collision='None', smooth=50.0)


# --- The kit ---

models = [
    headboard('Grave_Headboard_OldA', 11, round_top(0.44, 0.78, 0.22), lean=(9.0, 3.0), turn=2.0, marks=3, slot=0),
    headboard('Grave_Headboard_OldB', 12, peak_top(0.5, 0.72, 0.22, 0.12), lean=(-4.0, -8.0), turn=-4.0, split=True,
              strip='Beams', marks=2),
    headboard('Grave_Headboard_OldC', 13, broken_top(0.42, 0.8, 0.22, random.Random(13)), lean=(13.0, 2.0), turn=6.0,
              slot=3, marks=1, nails=False),
    headboard('Grave_Headboard_OldD', 14, shoulder_top(0.38, 0.9, 0.22), lean=(-6.0, 5.0), turn=-3.0, cleat=True,
              strip='Beams', marks=2),
    headboard('Grave_Headboard_FreshA', 15, peak_top(0.5, 0.82, 0.24, 0.13), lean=(1.5, 0.5), split=True, fresh=True,
              marks=0, painted='cross'),
    headboard('Grave_Headboard_FreshB', 16, round_top(0.42, 0.76, 0.24, crown=0.6), lean=(-1.0, 1.0), fresh=True,
              marks=0, painted='lines'),
    cross_wood('Grave_Cross_A', 21),
    cross_iron('Grave_Cross_B', 22),
    mound_fresh('Grave_MoundFresh', 31),
    grave_ellis('Grave_Ellis', 41),
    grave_abel('Grave_Abel', 42),
    grave_keeper('Grave_Keeper', 43),
    fence_iron_section('Fence_IronSection', 51),
    fence_iron_post('Fence_IronPost', 52),
    fence_iron_gate('Fence_IronGate', 53),
    fence_picket_section('Fence_PicketSection', 61),
    fence_picket_gate('Fence_PicketGate', 62),
    fence_picket_post('Fence_PicketPost', 63),
    coffin_closed('Coffin_Closed', 71),
    coffin_broken('Coffin_Broken', 72),
    wreath('Wreath', 81),
    crepe_swag('CrepeSwag', 82),
]

if lt.want_preview():
    # Each model on its own, then the kit in rows.
    for obj in models:
        print(f'GRAVES: {obj.name}: {lp.tri_count(obj)} triangles, '
              f'{len([c for c in obj.children if c.name.startswith("UCX_")])} hulls, '
              f'materials {", ".join(m.name for m in obj.data.materials)}', flush=True)
    rows = [models[0:8], models[8:12], models[12:18], models[18:22]]
    y = 0.0
    for row in rows:
        x = 0.0
        for obj in row:
            lo = min((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            hi = max((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            obj.location.x += x - lo
            x += hi - lo + 0.5
        for obj in row:
            obj.location.x -= x * 0.5
            obj.location.y = y
            if obj.name in ('Wreath', 'CrepeSwag'):   # hung from their nails: stand them on the ground here
                obj.location.z = -min(Vector(c).z for c in obj.bound_box)
        y += 3.2
    bpy.context.view_layer.update()
    # The ground at the pivots, as in the game: buried feet and the mounds' sunk skirts go under it.
    lt.preview(models, lt.preview_path(PREVIEW, 'Graves_kit'), view=(-0.25, -1.6, 0.75), fit=0.62, resolution=(1600, 1000),
               ground_at='origin')
    for obj in models:
        obj.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    for name, view, fit in (('Grave_Ellis', (-0.6, -1.6, 0.9), 0.75), ('Grave_Abel', (-0.5, -1.6, 0.7), 0.75),
                            ('Grave_Keeper', (-0.7, -1.6, 0.6), 0.85), ('Wreath', (-0.15, -1.6, 0.05), 0.8),
                            ('CrepeSwag', (-0.15, -1.6, 0.05), 0.8), ('Coffin_Broken', (-0.8, -1.4, 0.9), 0.8)):
        obj = next(o for o in models if o.name == name)
        lt.preview([obj], lt.preview_path(PREVIEW, name), view=view, fit=fit, ground=name not in ('Wreath', 'CrepeSwag'),
                   ground_at='origin' if name in ('Grave_Ellis', 'Grave_Abel') else 'bottom')
    # The fences chained: iron sections, the gate, a corner turning back; the picket run with its gate and end post.
    by_name = {o.name: o for o in models}
    run = []

    def put(name, at, turn=0.0):
        copy = bpy.data.objects.new('FenceRun', by_name[name].data)   # removed again after the picture
        bpy.context.scene.collection.objects.link(copy)
        copy.location, copy.rotation_euler = at, (0.0, 0.0, math.radians(turn))
        run.append(copy)
    for k, name in enumerate(['Fence_IronSection', 'Fence_IronSection', 'Fence_IronGate', 'Fence_IronSection']):
        put(name, (k * 2.0 - 4.0, 0.0, 0.0))
    put('Fence_IronPost', (4.0, 0.0, 0.0))
    put('Fence_IronSection', (4.0, 0.0, 0.0), 90.0)
    put('Fence_IronPost', (4.0, 2.0, 0.0))
    for k, name in enumerate(['Fence_PicketSection', 'Fence_PicketGate', 'Fence_PicketSection']):
        put(name, (k * 2.0 - 3.0, -3.5, 0.0))
    put('Fence_PicketPost', (3.0, -3.5, 0.0))
    put('Fence_PicketSection', (-3.0, -3.5, 0.0), 90.0)
    bpy.context.view_layer.update()
    lt.preview(run, lt.preview_path(PREVIEW, 'Fences'), view=(-0.5, -1.6, 0.55), fit=0.75)
    for copy in run:
        bpy.data.objects.remove(copy)
    for obj in models:
        print(f'GRAVES: {obj.name}: {lp.tri_count(obj)} triangles', flush=True)
else:
    for obj in models:
        print(f'GRAVES: {obj.name}: {lp.tri_count(obj)} triangles, '
              f'{len([c for c in obj.children if c.name.startswith("UCX_")])} hulls, '
              f'materials {", ".join(m.name for m in obj.data.materials)}', flush=True)
