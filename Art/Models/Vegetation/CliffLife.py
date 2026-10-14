"""Life on Ransom's Rest's sandstone cliffs (the "make it alive" polish, concept B, approved 2026-10-14): plants that cling
to the cliff kit's ledges and cracks, grass and roots spilling over a band's top edge, and the cliff swallows' mud nests
under an overhang. Scripted models (Art/README.md) built with Tools/Blender/looter_plants.py and the scrub's own parts
(Scrub.py, loaded as a module: the same foliage, bark and swatches as the slopes' junipers and sage). The rock pieces
(scree fans, fallen slabs) are Art/Models/Rocks/CliffScree.py; the stain decals Art/Textures/CliffStains/.

Frame (Art/README.md): Blender's -Y (the model's front) becomes the actor's +X; up is +Z. Every piece here is built
against a cliff face: its front (-Y) points OUT of the face, away from the rock. A placer turns each actor so its +X lies
along the face's outward normal (flattened to the horizontal), then stands it on its pivot:

  piece             tris  size (out: along -Y here)  pivot (the origin)               grows / hangs
  CliffJuniper_A    1279  1.75 m tall, 1.3 m out    the crack where the trunk leaves the wall: out level, then up into a
                                                    wall (y 0)                       tiered crown; roots grip and dive in
  CliffJuniper_B    1945  2.3 m tall, 1.35 m out    a ledge's surface (or a band's   a thick trunk leaning out over the
                                                    top) 0.3 m or more from its edge edge, three tiers of pads, a bare
                                                                                     silver stem; roots over the ledge
  CliffJuniper_C     900  1.05 m tall, 0.9 m out    a crack, or the back of a ledge  small and half dead, two clusters
  LedgeSage_A        393  0.65 m tall, 0.8 m wide   a ledge's surface                a sage clump pushed out
  LedgeSage_B        282  0.5 m tall, 0.6 m wide    a ledge's surface                small, two dead twigs
  LedgeGrass_A       417  0.35 m (flowers 0.5)      a ledge's surface                straw tuft, seed heads, 4 yellow daisies
  LedgeGrass_B       407  0.28 m                    a ledge's surface 0.15-0.25 m    half its blades droop 0.2-0.45 m over
                                                    from its edge                    the edge; purple and white flowers
  LipDrape_A        1719  2.4 m along the lip       the lip line: a band's top front grass over the edge, down 0.3-1.0 m
  LipDrape_B        2412  2.0 m                     edge, at the middle of the       grass to 1.4 m, roots to 1.9 m
  LipDrape_C         896  1.2 m                     drape's width                    a narrow tongue of grass and roots
  SwallowNests_A    1080  5 nests, 0.7 m wide       an overhang's inner corner:      nests hang 0.3 m below (-Z), backs on
                                                    the ceiling (z 0) over the back  the wall, 0.2 m out; mouths out and
                                                    wall (y 0)                       down
  SwallowNests_B     648  3 nests in a row, 0.5 m   a ceiling (z 0), the row's back  ceiling only; mouths out and down

Drapes: strands root on the band's top 3-30 cm behind the lip (+Y here, -X in Unreal), arch over it and hang 25-45 cm
proud of the lip line (roots 20-30 cm), so a face below that stands no more than ~20 cm in front of the lip clears them
(the cliff kit's tops round back about 20 cm over their last 30 cm: the lip a downward ray finds is that far behind the
face just under it). Junipers in a crack: the wall is the plane y = 0 (the rock behind, +Y); trunks start 15 cm inside
it, roots hug it and dive in; no spray grows behind it. Ledge pieces reach 0.15-0.25 m below their pivot (stems and
roots), so a ledge's bumps bury them; lean any of them up to ~15 degrees to sit on a sloping ledge.

Materials, all existing instances (one slot each, the junipers two, as the slopes' junipers):
  FoliageJuniper + BarkOak    the junipers (the slopes' pair)
  FoliageSage                 the ledge sage
  FoliagePalette              the ledge grass and the drapes: opaque blades and roots on the palette's swatches, as the
                              dry tufts (no alpha cards: no new texture, no masked overdraw; a strand is 1-2 cm wide)
  GroundDirt                  the nests (the valley's own mud)
Vertex colors are the foliage convention (Scrub.py): R wind weight (0 where a strand meets the rock), G phase per stem, A
occlusion. No Nanite (vegetation); LODs: the junipers and sage the slopes' own (Scrub.py), the grass the tufts', the
drapes 50/25% from screen size 0.12/0.04, the nests 50/25% from 0.06/0.02. Collision: one hull round each juniper's first
0.6 m of trunk (stops bullets and players; Vegetation: its hull is all it collides with); nothing else collides.

    Tools\\artrun.ps1 -Script Art\\Models\\Vegetation\\CliffLife.py [-Preview]
-Preview renders each model in the golden light to Saved/ArtPreviews/RansomsRest/CliffLife/<name>.png; the look sheet
(43, with every piece in place on the cliff kit) is Tools/Blender/cliff_life_sheet.py.
"""
import importlib.util
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Vector, noise

import looter_model as lm
import looter_plants as lp
import looter_textures as lt
from looter_plants import UP, horizontal, lerp, perpendicular, random_unit, smoothstep


def _scrub():
    """Scrub.py as a module (its models are built only when it is run itself)."""
    sys.dont_write_bytecode = True     # no __pycache__ in the art folder
    path = os.path.join(lt.REPO, 'Art', 'Models', 'Vegetation', 'Scrub.py')
    spec = importlib.util.spec_from_file_location('scrub_parts', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


S = _scrub()
DIRT = lt.material('GroundDirt')
OUT = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'CliffLife')
OUTWARD = Vector((0.0, -1.0, 0.0))      # out of the face: the model's front

# LODs (triangle percentages, screen sizes; Unreal's screen size is about 1.8 x bounds radius / distance).
DRAPE_LODS = ('50,25', '0.12,0.04')      # drapes (radius ~1.2 m): LOD1 from ~18 m, LOD2 ~55 m
GRASS_LODS = S.TUFT_LODS
NEST_LODS = ('50,25', '0.06,0.02')       # nests (radius ~0.4 m): LOD1 from ~12 m, LOD2 ~36 m


# --- Strands: blades and roots along any path ---

def strand(plant, mat, points, width, swatch, u_range, wind, occlusion, shade, across=None, end_width=0.0, taper=0.8):
    """A flat strip along points (a hanging blade, a root), width at its root narrowing to end_width, its flat side
    across `across` (default: X, so a strand hanging down a face shows its flat side out of it). wind and occlusion run
    root to tip ((root, tip) each); shade(p) gives its shading normals."""
    across = across or Vector((1.0, 0.0, 0.0))
    count = len(points)
    widths, sides, normals, winds, occl = [], [], [], [], []
    for i, p in enumerate(points):
        t = i / (count - 1)
        d = points[min(i + 1, count - 1)] - points[max(i - 1, 0)]
        d = d.normalized() if d.length > 1e-6 else -UP
        s = across - d * across.dot(d)
        s = s.normalized() if s.length > 1e-4 else perpendicular(d)
        sides.append(s)
        widths.append(width * (1.0 - t) ** taper if i < count - 1 else end_width)
        normals.append(shade(p))
        winds.append(lerp(wind[0], wind[1], t ** 1.5))
        occl.append(lerp(occlusion[0], occlusion[1], t ** 0.7))
    S.ribbon(plant, mat, points, widths, sides, normals, swatch, u_range, winds, occl)


def facing_out(lift=0.25, spread=0.0):
    """Shading normals for strands hanging down a face: out of it, a little up (lit like the face's own light), turned
    sideways by spread per meter off x = 0 so a drape rounds a little."""
    def shade(p):
        return (OUTWARD + UP * lift + Vector((p.x * spread, 0.0, 0.0))).normalized()
    return shade


# --- Junipers that cling to a crack or a ledge ---

def cliff_juniper(name, seed, trunk, radius, top_radius, crowns, sprays, roots, snags=(), dead_stems=(), wall=None,
                  gnarl=0.08, flat_top=None, low=-0.35, spray_size=(0.17, 0.24)):
    """A gnarled juniper growing out of the rock. trunk: four bezier points from inside the rock (its first point) out
    and up. crowns: clusters of foliage pads, (centre, radii, pad count, pad sizes) each; the first is the top one,
    which the trunk runs on into; every other cluster hangs on a branch of its own off the trunk (a tiered, wind-shorn
    crown, as cliff junipers grow). Each pad is a dark core under drooping scale-leaf sprays (the slopes' junipers'
    pads, flatter). roots: (radius, points) each, hugging the rock and diving into it. snags: dead silver limbs (param
    along the trunk, direction, length). dead_stems: bare stems of their own (bezier points, radius). wall: the face's
    plane y (crack junipers): pads stay in front of it, sprays behind it are left out."""
    rng = random.Random(seed)
    plant = S.Shrub(name, seed)
    top_center, top_radii = Vector(crowns[0][0]), crowns[0][1]
    occlusions = [lp.crown_occlusion(Vector(c), radii, floor=0.4) for c, radii, _, _ in crowns]
    shade_center = top_center - UP * top_radii[2] * 1.2

    # The pads: flattened clusters, wind-shorn flat on top, every pad in front of the wall.
    pads = []
    for ci, (center, radii, count, size) in enumerate(crowns):
        for c, r in S.clump_layout(rng, Vector(center), radii, count, size, low, 1 if count > 3 else 0, floor=0.0):
            if flat_top is not None and c.z + r * 0.5 > flat_top:
                c = Vector((c.x, c.y, flat_top - r * 0.5))
            if wall is not None and c.y > wall - r * 0.85 - 0.04:
                c = Vector((c.x, wall - r * 0.85 - 0.04, c.z))
            pads.append((c, r, ci))
    pads.sort(key=lambda pad: (pad[2], pad[0].z, pad[0].x))
    pad_groups = [plant.new_group() for _ in pads]
    leader = max((k for k, pad in enumerate(pads) if pad[2] == 0), key=lambda k: pads[k][0].z)

    # The trunk: out of the crack (or the ledge's soil), twisting, into the top cluster's highest pad.
    p0, p1, p2, p3 = (Vector(p) for p in trunk)
    points = lp.bezier(p0, p1, p2, p3, [0.0, 0.05, 0.1, 0.17, 0.25, 0.34, 0.44, 0.55, 0.66, 0.77, 0.88, 1.0])
    points = lp.gnarl(points, gnarl, 2.2, seed * 3)
    top_c, top_r, _ = pads[leader]
    points.append(top_c + UP * top_r * 0.1)
    limb = lp.Limb(points, radius, 0.0, 7, (0.0, 0.14))
    # Thick where it leaves the rock, swelling and pinching a little along the way (a twisted, ridged stem).
    limb.radii = [lerp(radius, top_radius, smoothstep(0.0, 0.85, t) ** 0.8)
                  * (1.0 + 0.12 * noise.noise(Vector((t * 6.0, seed * 0.3, 0.0)))) for t in limb.params]
    limb.radii[-1] = 0.0
    root_point = Vector((0.0, 0.0, 0.0))

    def flare(p, a):
        # A thickened foot where it leaves the rock, lobed like gripping roots.
        near = (1.0 - smoothstep(0.0, 0.3, (p - root_point).length)) ** 2
        return 1.0 + 0.6 * near * (0.35 + 0.65 * max(0.0, math.cos(4.0 * a + seed)) ** 3)
    plant.group = pad_groups[leader]
    plant.tube(limb, S.BARK, flare, occlusions[0])

    def nearest(branch, c, low_t, high_t):
        """The point of branch (params low_t to high_t) nearest c: where a limb toward c leaves it."""
        best = None
        for k in range(30):
            p, r, w, _ = branch.at(lerp(low_t, high_t, k / 29))
            d = (p - c).length
            if best is None or d < best[0]:
                best = (d, p, r, w)
        return best[1], best[2], best[3]

    def limb_to(start, c, r, start_r, start_wind, index, share=(0.55, 0.7)):
        out = c - start
        out = Vector((out.x, out.y, 0.0)).normalized() + UP * 0.5
        end = c + (c - start).normalized() * r * 0.2
        return lp.curved_limb(start, end, out, 0.15, 4, max(start_r * rng.uniform(*share), 0.018), 0.0,
                              (start_wind, 0.35), rng, 0.06, 0.3, 0.35, seed * 31 + index)

    # A branch out to each lower cluster's middle; a limb from the trunk (top cluster) or the cluster's branch to
    # every pad.
    branches = {}
    for ci, (center, radii, count, size) in enumerate(crowns):
        if ci == 0:
            continue
        center = Vector(center)
        start, trunk_r, trunk_wind = nearest(limb, center, 0.2, 0.8)
        plant.group = pad_groups[[k for k, pad in enumerate(pads) if pad[2] == ci][0]]
        branch = limb_to(start, center - UP * radii[2] * 0.4, 0.0, trunk_r, trunk_wind, 100 + ci, (0.6, 0.7))
        plant.tube(branch, S.BARK, occlusion=occlusions[ci])
        branches[ci] = branch
    for index, (c, r, ci) in enumerate(pads):
        if index == leader:
            continue
        plant.group = pad_groups[index]
        if ci == 0:
            start, start_r, start_wind = nearest(limb, c, 0.45, 0.95)
        else:
            start, start_r, start_wind = nearest(branches[ci], c, 0.4, 0.95)
        plant.tube(limb_to(start, c, r, start_r, start_wind, index), S.BARK, occlusion=occlusions[ci])

    # The pads: flat cushions, dark cores under drooping sprays; none where a neighbour or the wall buries it.
    for index, (c, r, ci) in enumerate(pads):
        plant.group = pad_groups[index]
        others = [(o, orad) for j, (o, orad, _) in enumerate(pads) if j != index]
        occlusion = occlusions[ci]
        size = crowns[ci][3]

        def wind(p, c=c, r=r):
            return lerp(0.35, 0.5, smoothstep(c.z - r, c.z + r, p.z))

        def flow(p, n, c=c):
            return Vector((p.x - c.x, p.y - c.y, 0.0)) * 2.0 - UP * 0.6

        def buried(p, others=others):
            if wall is not None and p.y > wall - 0.02:
                return True
            return any((p - oc).length < orad * 0.72 for oc, orad in others)
        surface, shade = S.lobed(plant, S.JUNIPER, c, (r * 0.85, r * 0.85, r * 0.55), rng, S.JUNIPER_CORE[0],
                                 S.pick(rng, S.JUNIPER_CORE[1]), shade_center, lambda p, o=occlusion: o(p) * 0.6, wind,
                                 lobes=3, levels=1, bottom=0.7, swell=0.3)
        S.cover(plant, S.JUNIPER, rng, surface, shade, c, int(round(sprays * (r / size[1]) ** 2)), -0.65,
                spray_size, 0.4, S.JUNIPER_LEAF[0], S.JUNIPER_LEAF[1], occlusion, wind, flow, lift=(0.08, 0.35),
                leaves=4, fan=0.8, skip=buried)

    # Dead limbs: bare, silvered, snapped, a fork on each.
    for k, (t, direction, length) in enumerate(snags):
        plant.new_group()
        start, trunk_r, trunk_wind, _ = limb.at(t)
        direction = Vector(direction).normalized()
        end = start + direction * length
        dead = lp.curved_limb(start, end, direction + UP * 0.2, 0.05, 4, trunk_r * 0.5, trunk_r * 0.15,
                              (trunk_wind, 0.2), rng, 0.04, 0.2, 0.35, seed * 53 + k)
        plant.tube(dead, S.BARK, occlusion=lambda p: 0.95)
        p, rr, w, d = dead.at(0.55)
        fork = (d + perpendicular(d) * 0.9 + UP * 0.3).normalized()
        plant.tube(lp.Limb([p, p + fork * length * 0.18, p + fork * length * 0.34 + UP * 0.04], rr * 0.6, 0.0, 3,
                           (w, 0.22)), S.BARK, occlusion=lambda p: 0.95)
    for k, (stem, stem_radius) in enumerate(dead_stems):
        plant.new_group()
        q0, q1, q2, q3 = (Vector(p) for p in stem)
        pts = lp.gnarl(lp.bezier(q0, q1, q2, q3, lp.steps(8)), gnarl * 1.2, 2.0, seed * 7 + k)
        plant.tube(lp.Limb(pts, stem_radius, stem_radius * 0.3, 5, (0.0, 0.15)), S.BARK, occlusion=lambda p: 0.95)

    # Roots: gnarled, hugging the rock and diving into it, tapering to nothing inside.
    plant.group = 0
    for k, (root_radius, path) in enumerate(roots):
        pts = [Vector(p) for p in path]
        pts = lp.bezier(pts[0], pts[1], pts[2], pts[3], lp.steps(6)) if len(pts) == 4 else pts
        pts = lp.gnarl(pts, 0.012, 6.0, seed * 11 + k)
        root = lp.Limb(pts, root_radius, 0.0, 5, (0.0, 0.0), taper=0.7)
        plant.tube(root, S.BARK, occlusion=lambda p: 0.7)

    # The hull: round the trunk's first 0.6 m out of the rock (bullets meet the trunk, never the sprays).
    hull_points = []
    for p, r, t in zip(limb.points, limb.radii, limb.params):
        if t * limb.length <= 0.6 and r > 0.0:
            hull_points += lp.cylinder_points(p, p + UP * 0.01, r * 1.05, sides=6)
    return S.finish(plant, hull_points, ao_distance=0.8, ao_blend=0.55, lods=S.JUNIPER_LODS)


def cliff_juniper_a():
    """Out of a crack in a sheer face: the trunk leaves the wall level, sags, then turns up into a flat, wind-shorn
    crown 1.1 m out; roots grip the crack above and below; a dead limb out to the side. 1.75 m tall."""
    return cliff_juniper(
        'CliffJuniper_A', 401,
        trunk=[(0.0, 0.16, -0.03), (0.0, -0.45, -0.04), (0.06, -0.88, 0.3), (0.14, -0.76, 1.22)],
        radius=0.1, top_radius=0.05,
        crowns=[((0.12, -0.8, 1.42), (0.62, 0.44, 0.24), 6, (0.27, 0.37)),
                ((-0.44, -0.64, 0.82), (0.32, 0.27, 0.15), 3, (0.21, 0.29))],
        sprays=30, flat_top=1.75, wall=0.0,
        roots=[(0.045, [(0.0, -0.03, 0.0), (-0.12, -0.04, 0.08), (-0.24, -0.03, 0.15), (-0.32, 0.1, 0.2)]),
               (0.04, [(0.02, -0.03, -0.02), (0.07, -0.04, -0.17), (0.05, -0.03, -0.31), (0.08, 0.1, -0.4)]),
               (0.032, [(0.0, -0.03, 0.02), (0.13, -0.04, 0.07), (0.2, -0.025, 0.05), (0.25, 0.08, 0.04)])],
        snags=[(0.42, (0.85, -0.35, 0.45), 0.7)])


def cliff_juniper_b():
    """On a ledge: a thick trunk leaning out over the edge and twisting up into a broad crown 1.2 m out, a bare silver
    stem standing beside it, roots spread over the ledge and down its front edge. 2.45 m tall."""
    return cliff_juniper(
        'CliffJuniper_B', 413,
        trunk=[(0.0, 0.05, -0.25), (0.0, -0.05, 0.45), (0.16, -0.62, 0.95), (0.3, -0.86, 1.8)],
        radius=0.14, top_radius=0.07,
        crowns=[((0.28, -0.82, 1.95), (0.78, 0.58, 0.32), 7, (0.32, 0.45)),
                ((-0.38, -0.52, 1.32), (0.44, 0.36, 0.2), 4, (0.27, 0.37)),
                ((0.47, -0.36, 0.8), (0.32, 0.28, 0.15), 3, (0.22, 0.3))],
        sprays=32, flat_top=2.45, gnarl=0.09,
        roots=[(0.05,[(0.0, 0.0, 0.05), (0.22, 0.06, 0.04), (0.38, 0.12, -0.02), (0.48, 0.16, -0.16)]),
               (0.04, [(0.0, 0.0, 0.05), (-0.2, 0.0, 0.04), (-0.36, -0.06, -0.01), (-0.46, -0.05, -0.16)]),
               (0.034, [(0.0, -0.03, 0.04), (0.05, -0.22, 0.03), (0.08, -0.36, -0.06), (0.1, -0.42, -0.26)]),
               (0.03, [(0.0, 0.03, 0.04), (-0.05, 0.2, 0.03), (-0.08, 0.32, -0.04), (-0.1, 0.36, -0.2)])],
        dead_stems=[([(0.05, 0.06, -0.2), (0.03, 0.08, 0.55), (-0.2, 0.05, 1.0), (-0.36, 0.0, 1.45)], 0.055)],
        snags=[(0.62, (-0.6, -0.5, 0.6), 0.55)])


def cliff_juniper_c():
    """Small and half dead, out of a crack or the back of a narrow ledge: a short twisted trunk out and up, four pads
    on its living side, a bare stub on the other. 1.05 m tall."""
    return cliff_juniper(
        'CliffJuniper_C', 427,
        trunk=[(0.0, 0.12, 0.0), (0.02, -0.25, 0.04), (0.0, -0.42, 0.36), (-0.08, -0.46, 0.72)],
        radius=0.075, top_radius=0.035,
        crowns=[((-0.08, -0.52, 0.86), (0.36, 0.28, 0.17), 3, (0.2, 0.27)),
                ((0.25, -0.36, 0.48), (0.17, 0.15, 0.09), 2, (0.16, 0.21))],
        sprays=26, flat_top=1.05, wall=0.0, low=-0.2, spray_size=(0.15, 0.21),
        roots=[(0.034, [(0.0, -0.03, 0.0), (0.1, -0.035, -0.06), (0.17, -0.025, -0.14), (0.22, 0.08, -0.18)]),
               (0.03, [(0.0, -0.03, 0.01), (-0.1, -0.035, 0.05), (-0.16, -0.02, 0.12), (-0.2, 0.07, 0.15)])],
        snags=[(0.38, (0.75, -0.4, 0.25), 0.38)])


# --- Ledge sage and grass ---

def ledge_sage_a():
    """A sage clump sized for a ledge: lopsided, its crown pushed out over the edge (-Y here), about 0.55 m tall and
    0.85 m across."""
    return S.sagebrush('LedgeSage_A', 441, (0.0, -0.12, 0.27), (0.4, 0.3, 0.27), 4, 85, stalks=5, dead=1, stems=3,
                       stem_radius=(0.02, 0.03), leaf_size=(0.11, 0.15), lods=S.SHRUB_LODS)


def ledge_sage_b():
    """A small, wind-starved one, about 0.42 m tall: three lobes pushed out, two dead twigs."""
    return S.sagebrush('LedgeSage_B', 449, (0.05, -0.15, 0.2), (0.3, 0.24, 0.21), 3, 52, stalks=2, dead=2, stems=2,
                       stem_radius=(0.015, 0.022), leaf_size=(0.09, 0.13), lods=S.SMALL_SHRUB_LODS)


def droop_path(root, heading, rise, out, hang, segments, rng):
    """A blade that arches up out of root, over a ledge's edge (out along heading) and hangs down past it."""
    heading = Vector((heading.x, heading.y, 0.0)).normalized()
    p1 = root + UP * rise * 1.3 + heading * out * 0.25
    p2 = root + UP * rise * 0.9 + heading * out * 0.85
    p3 = root + heading * out * 1.15 - UP * hang
    points = lp.bezier(root, p1, p2, p3, [k / segments for k in range(segments + 1)])
    return [p + Vector((0.0, 0.0, 0.0)) for p in points]


def flower(plant, mat, rng, root, height, lean, swatch, u, center_swatch, petals, shade, stem_swatch=('DryTan', 0.35)):
    """A wildflower: a slender stem from root bending toward lean (a vector), a small domed middle and a star of petals
    (one triangle each) round it, facing up and out."""
    plant.new_group()
    tip = root + UP * height + lean * height
    mid = root + UP * height * 0.55 + lean * height * 0.3
    points = lp.bezier(root, mid, tip - (UP * 0.4 + lean).normalized() * height * 0.2, tip, lp.steps(3))
    strand(plant, mat, points, 0.007, stem_swatch[0], (stem_swatch[1], stem_swatch[1] + 0.1), (0.0, 0.95),
           (0.6, 0.95), shade, across=perpendicular(UP + lean), end_width=0.004, taper=0.3)
    up = (UP + lean * 0.8).normalized()
    radius = rng.uniform(0.012, 0.016)
    S.flower_head(plant, mat, tip, radius, up, rng, center_swatch[0], center_swatch[1], 0.95, shade, rim_count=5)
    side = perpendicular(up)
    other = up.cross(side)
    turn = rng.uniform(0.0, 2.0 * math.pi)
    for k in range(petals):
        a = turn + 2.0 * math.pi * k / petals + rng.uniform(-0.15, 0.15)
        direction = (side * math.cos(a) + other * math.sin(a) + up * 0.15).normalized()
        S.tuft_leaf(plant, mat, tip - up * 0.004, direction, up, rng.uniform(0.024, 0.032), rng.uniform(0.016, 0.02),
                    swatch, u + rng.uniform(-0.05, 0.05), shade, lambda p: 1.0, (0.95, 1.0))


def ledge_grass(name, seed, blades, height, spread, colors, droop_share, droop_hang, stalks, flowers, lods=GRASS_LODS):
    """A tuft of dry grass for a ledge, leaning out (-Y here): blades rooted in a tight patch slightly back from the
    pivot, the front ones arching out and a share (droop_share) drooping over the edge (hanging droop_hang below the
    pivot); seed stalks; flowers: (count, petal swatch and U, middle swatch and U, petals)."""
    rng = random.Random(seed)
    plant = S.Shrub(name, seed)
    mat = S.PALETTE
    pool = [(s, u) for s, u, w in colors for _ in range(w)]
    shade = lp.up_normal(Vector((0.0, 0.0, 0.0)), spread + height * 0.6, 0.5)

    def shade_out(p):
        return (UP * 0.7 + OUTWARD * 0.45 + Vector((p.x, 0.0, 0.0))).normalized()
    for i in range(blades):
        plant.new_group()
        r = spread * math.sqrt((i + 0.5) / blades) * rng.uniform(0.7, 1.1)
        a = S.GOLDEN_ANGLE * i + rng.uniform(-0.4, 0.4)
        root = horizontal(a, r) + Vector((0.0, 0.03, -0.025))
        # Most lean out over the edge (-Y); the back ones stand up against the wall.
        out_angle = -math.pi * 0.5 + rng.uniform(-1.1, 1.1)
        heading = horizontal(out_angle)
        swatch, u = rng.choice(pool)
        u0 = rng.uniform(*u)
        width = 0.017 * rng.uniform(0.8, 1.2)
        if rng.random() < droop_share:
            hang = droop_hang * rng.uniform(0.5, 1.0)
            points = droop_path(root, heading, height * rng.uniform(0.35, 0.55), rng.uniform(0.18, 0.3), hang, 5, rng)
            strand(plant, mat, points, width, swatch, (u0 - 0.12, u0), (0.0, 1.0), (0.55, 1.0), shade_out,
                   across=UP.cross(heading))
        else:
            back = r > spread * 0.5 and math.sin(a) > 0.3
            h = height * rng.uniform(0.7, 1.1) * (0.8 if back else 1.0)
            lean = rng.uniform(0.25, 0.5) if back else rng.uniform(0.5, 0.9)
            S.blade(plant, mat, root, heading, h, width, lean, swatch, (u0 - 0.12, u0), rng, 3, shade, twist=0.4,
                    occlusion=(0.5, 1.0))
    for k in range(stalks):
        plant.new_group()
        a = rng.uniform(math.pi * 0.9, math.pi * 2.1)
        root = horizontal(a, spread * 0.5 * math.sqrt(rng.random())) - UP * 0.02
        S.spike(plant, mat, root, horizontal(-math.pi * 0.5 + rng.uniform(-0.8, 0.8)), height * rng.uniform(1.25, 1.5),
                rng.uniform(0.15, 0.4), 'Straw', rng.uniform(0.85, 0.95), shade, wind=(0.0, 0.95))
    for count, (petal_swatch, petal_u), middle, petals in flowers:
        for k in range(count):
            a = rng.uniform(0.0, 2.0 * math.pi)
            root = horizontal(a, spread * 0.6 * math.sqrt(rng.random())) - UP * 0.02
            lean = horizontal(-math.pi * 0.5 + rng.uniform(-0.9, 0.9), rng.uniform(0.15, 0.4))
            flower(plant, mat, rng, root, height * rng.uniform(1.15, 1.55), lean, petal_swatch, petal_u, middle,
                   petals, shade)
    return S.finish(plant, None, ao_blend=1.0, lods=lods)


def ledge_grass_a():
    """A straw-golden tuft about 35 cm tall leaning out, seed stalks, and four small yellow daisies to 0.5 m."""
    return ledge_grass('LedgeGrass_A', 461, 56, 0.34, 0.08,
                       [('Straw', (0.6, 0.85), 5), ('DryTan', (0.55, 0.85), 3), ('GrassDry', (0.75, 0.95), 2)],
                       0.1, 0.14, 5, [(4, ('FlowerYellow', 0.55), ('FlowerCenter', 0.3), 7)])


def ledge_grass_b():
    """A low tuft at a ledge's edge, about 28 cm: nearly half its blades droop out and down over the edge (to 0.45 m
    below it); three pale purple flowers and two white."""
    return ledge_grass('LedgeGrass_B', 467, 40, 0.27, 0.075,
                       [('Straw', (0.45, 0.8), 3), ('DryTan', (0.35, 0.75), 3), ('DryTan', (0.12, 0.25), 1),
                        ('GrassDry', (0.65, 0.9), 2)],
                       0.45, 0.45, 3, [(3, ('FlowerPurple', 0.75), ('FlowerYellow', 0.4), 6),
                                       (2, ('FlowerWhite', 0.7), ('FlowerYellow', 0.5), 7)])


# --- Drapes over a band's lip ---

def drape(name, seed, width, tufts, blades_per_tuft, hang, roots, root_hang, fringe, envelope=0.6):
    """Dry grass and fine roots spilling over a cliff band's top edge: tufts rooted on the band's top along the lip
    (pivot: the lip line's middle; the top runs back into +Y), their blades arching over the edge and hanging hang
    (low, high) metres, longest mid-drape (envelope shapes the ragged lower edge); roots (count) curling over and
    hanging root_hang (low, high), thin and dark; fringe: short upright blades along the top."""
    rng = random.Random(seed)
    plant = S.Shrub(name, seed)
    mat = S.PALETTE
    shade = facing_out(0.25, 0.25)
    half = width * 0.5
    colors = [('Straw', (0.55, 0.85)), ('Straw', (0.4, 0.7)), ('DryTan', (0.45, 0.85)), ('GrassDry', (0.75, 0.95)),
              ('DryTan', (0.15, 0.3))]

    def reach(x):
        """How far the drape hangs at x along the lip: longest mid-drape, ragged."""
        edge = max(0.0, 1.0 - abs(x / half) ** 2) ** envelope
        return edge * (0.75 + 0.25 * noise.noise(Vector((x * 2.3, seed * 0.37, 0.5))) * 2.0)

    xs = sorted(rng.uniform(-half * 0.95, half * 0.95) for _ in range(tufts))
    for tx in xs:
        tuft_hang = lerp(hang[0], hang[1], max(0.05, reach(tx))) * rng.uniform(0.75, 1.15)
        y_root = rng.uniform(0.04, 0.3)
        drift = rng.uniform(-0.12, 0.12)
        count = blades_per_tuft + rng.randint(-2, 3)
        for b in range(count):
            plant.new_group()
            x = tx + rng.uniform(-0.06, 0.06)
            swatch, u = rng.choice(colors)
            u0 = rng.uniform(*u)
            length = tuft_hang * rng.uniform(0.45, 1.05)
            out = rng.uniform(0.26, 0.4)
            root = Vector((x, y_root + rng.uniform(-0.03, 0.03), -0.04))
            rise = rng.uniform(0.04, 0.12)
            # Up out of the soil, over the rounded lip and out past it, then straight down, bowing out a little and
            # back toward the face at the tip, drifting sideways.
            arch = lp.bezier(root, root + Vector((drift * 0.1, -0.05, rise + 0.04)),
                             Vector((x + drift * 0.2, -out * 0.55, rise * 0.8)),
                             Vector((x + drift * 0.3, -out, -0.08)), [0.0, 0.35, 0.7, 1.0])
            steps = max(2, int(math.ceil(length / 0.2)))
            points = list(arch)
            sway = rng.uniform(0.0, 100.0)
            for k in range(1, steps + 1):
                t = k / steps
                z = -0.08 - length * t
                y = -out - 0.06 * math.sin(math.pi * min(t * 1.4, 1.0)) + 0.04 * t * t
                xx = x + drift * (0.3 + t) + 0.04 * noise.noise(Vector((sway, t * 2.5, 0.0)))
                points.append(Vector((xx, y, z)))
            strand(plant, mat, points, rng.uniform(0.012, 0.019), swatch, (u0 - 0.15, u0), (0.0, 0.65), (0.5, 1.0),
                   shade, end_width=0.0, taper=0.6)

    # Fine roots: curling over with the grass, hanging closer to the face, thin, dark and long, some forked.
    for k in range(roots):
        plant.new_group()
        x = rng.uniform(-half * 0.8, half * 0.8)
        length = lerp(root_hang[0], root_hang[1], max(0.1, reach(x))) * rng.uniform(0.6, 1.0)
        out = rng.uniform(0.22, 0.3)
        root = Vector((x, rng.uniform(0.03, 0.12), -0.05))
        arch = lp.bezier(root, root + Vector((0.0, -0.06, 0.05)), Vector((x, -out * 0.6, 0.03)),
                         Vector((x, -out, -0.1)), [0.0, 0.4, 0.75, 1.0])
        steps = max(3, int(math.ceil(length / 0.18)))
        points = list(arch)
        wander = rng.uniform(0.0, 100.0)
        for s in range(1, steps + 1):
            t = s / steps
            points.append(Vector((x + 0.07 * noise.noise(Vector((wander, t * 3.0, 1.0))),
                                  -out + 0.03 * t + 0.015 * noise.noise(Vector((wander, t * 5.0, 2.0))),
                                  -0.1 - length * t)))
        swatch = ('CattailBrown', rng.uniform(0.35, 0.8)) if rng.random() < 0.65 else ('DryTan', rng.uniform(0.12, 0.3))
        strand(plant, mat, points, rng.uniform(0.007, 0.011), swatch[0], (swatch[1] - 0.1, swatch[1]), (0.0, 0.35),
               (0.45, 0.85), shade, end_width=0.002, taper=0.4)
        if rng.random() < 0.5:
            i = len(arch) + steps // 3
            p = points[i]
            side = rng.choice((-1.0, 1.0))
            branch = [p + Vector((side * 0.06 * t + side * 0.04 * t * t, 0.01 * t, -length * 0.35 * t))
                      for t in (0.0, 0.33, 0.66, 1.0)]
            strand(plant, mat, branch, 0.005, swatch[0], (swatch[1] - 0.05, swatch[1]), (0.12, 0.35), (0.6, 0.85),
                   shade, end_width=0.0, taper=0.5)

    # A short fringe standing along the top, so the lip itself reads grassy from below.
    top_shade = lp.up_normal(Vector((0.0, 0.1, 0.0)), width, 0.3)
    for k in range(fringe):
        plant.new_group()
        x = rng.uniform(-half, half)
        root = Vector((x, rng.uniform(0.02, 0.28), -0.03))
        swatch, u = rng.choice(colors[:4])
        u0 = rng.uniform(*u)
        S.blade(plant, mat, root, horizontal(-math.pi * 0.5 + rng.uniform(-0.7, 0.7)), rng.uniform(0.1, 0.24),
                0.016 * rng.uniform(0.8, 1.2), rng.uniform(0.45, 0.9), swatch, (u0 - 0.12, u0), rng, 3, top_shade,
                twist=0.4, occlusion=(0.55, 1.0))
    return S.finish(plant, None, ao_blend=1.0, lods=DRAPE_LODS)


def lip_drape_a():
    """2.4 m along the lip: dry grass curling over and hanging 0.3-1.0 m, a few roots, a fringe on top."""
    return drape('LipDrape_A', 501, 2.4, 18, 7, (0.3, 1.0), 6, (0.6, 1.1), 26)


def lip_drape_b():
    """2.0 m: a heavy drape, grass to 1.4 m and a curtain of fine roots to 2.0 m."""
    return drape('LipDrape_B', 509, 2.0, 16, 8, (0.45, 1.4), 16, (1.1, 2.0), 18, envelope=0.45)


def lip_drape_c():
    """1.2 m: a narrow tongue of grass to 0.8 m and roots to 1.5 m, for a notch or a gully's lip."""
    return drape('LipDrape_C', 517, 1.2, 9, 7, (0.25, 0.8), 9, (0.8, 1.5), 10, envelope=0.8)


# --- Cliff swallows' mud nests ---

def pellets(p, seed):
    """The lumpy mud-pellet skin of a nest: bumps about 2 cm across, a few millimetres high."""
    v = noise.voronoi(p * 38.0 + Vector((seed * 1.7, seed * 0.9, 0.0)), distance_metric='DISTANCE', exponent=2.5)
    return 0.004 * (1.0 - min(v[0][0] * 1.6, 1.0)) + 0.002 * noise.noise(p * 70.0 + Vector((seed, 0.0, 0.0)))


# A nest's outline along its axis, from the back of the bulb (on the rock) to the lip of its mouth: (share of the
# length, radius as a share of the nest's size). Then the inside of the neck, back from the lip, closed deep in.
NEST_PROFILE = [(0.06, 0.5), (0.16, 0.82), (0.3, 1.0), (0.45, 0.96), (0.6, 0.78), (0.73, 0.54), (0.85, 0.4),
                (0.95, 0.36), (1.0, 0.37)]
NEST_INSIDE = [(0.985, 0.24), (0.88, 0.21), (0.76, 0.18)]
NEST_SIDES = 9


def nest(bm, center, size, mouth, seed, wall=True):
    """One cliff swallow's nest: a gourd of mud pellets, its bulb pressed flat against the ceiling (z 0) and, with
    wall, the back wall (y 0), its neck narrowing and drooping toward mouth (out and down) to an open, slightly flared
    lip; inside, the neck runs back dark and closes. Built into bm (about 215 triangles)."""
    rng = random.Random(seed)
    m = Vector(mouth).normalized()
    side = perpendicular(m)
    other = m.cross(side)
    length = size * 2.4
    back = center - m * size
    turn = rng.uniform(0.0, 2.0 * math.pi)

    def point(t, share, s, outside=True):
        a = turn + 2.0 * math.pi * s / NEST_SIDES
        radial = side * math.cos(a) + other * math.sin(a)
        wobble = 1.0 + 0.08 * noise.noise(Vector((t * 4.0, s * 0.7, seed * 0.37)))
        p = back + m * (length * t) + radial * (share * size * wobble) - UP * size * 0.25 * smoothstep(0.5, 1.0, t) ** 2
        if outside:
            p += radial * pellets(p, seed)
        # Pressed flat where it was built on the rock (a little spread there).
        if p.z > -0.004:
            p.z = -0.004 + (p.z + 0.004) * 0.1
        if wall and p.y > -0.004:
            p.y = -0.004 + (p.y + 0.004) * 0.1
        return bm.verts.new(p)

    pole = bm.verts.new(back + m * (length * 0.01))
    outer = [[point(t, share, s) for s in range(NEST_SIDES)] for t, share in NEST_PROFILE]
    inner = [[point(t, share, s, False) for s in range(NEST_SIDES)] for t, share in NEST_INSIDE]
    end = bm.verts.new(back + m * (length * 0.7) - UP * size * 0.25 * smoothstep(0.5, 1.0, 0.7) ** 2)
    for s in range(NEST_SIDES):
        s1 = (s + 1) % NEST_SIDES
        bm.faces.new((pole, outer[0][s1], outer[0][s]))
        for ring, nxt in zip(outer + inner, outer[1:] + inner):
            bm.faces.new((ring[s], ring[s1], nxt[s1], nxt[s]))
        bm.faces.new((inner[-1][s], inner[-1][s1], end))
    return back + m * length


def swallow_nests(name, seed, spots, wall=True):
    """A cluster of mud nests (spots: (x, y, z of the bulb's middle, size, mouth heading in degrees off -Y)), merged,
    on the valley's own mud (GroundDirt). The pivot is where the cluster meets the ceiling, on its back line."""
    bm = bmesh.new()
    for k, (x, y, z, size, turn) in enumerate(spots):
        a = math.radians(turn)
        mouth = Vector((math.sin(a), -math.cos(a), -0.7))
        nest(bm, Vector((x, y, z)), size, mouth, seed + k * 13, wall)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    lm.smooth(obj, 70.0)
    lt.assign(obj, DIRT)
    # The valley's dirt at four times its ground's fineness: its pebbles become the grit in dried mud.
    lt.box_uv(obj, 'GroundDirt', texel_density=1000.0, seed=seed)
    # The ceiling (the "ground" plane at the pivot) shades their tops; the tubes' insides go dark.
    lt.bake_vertex_ao(obj, samples=32, distance=0.12)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    obj['LODs'] = NEST_LODS[0]
    obj['LODScreens'] = NEST_LODS[1]
    return obj


def swallow_nests_a():
    """Five nests in the inner corner of an overhang, crowded together on the ceiling and the back wall, mouths out
    and down in a fan, one old nest low and half the size."""
    return swallow_nests('SwallowNests_A', 541, [
        (-0.27, -0.085, -0.085, 0.085, -25.0), (-0.1, -0.08, -0.09, 0.09, -8.0), (0.08, -0.085, -0.085, 0.088, 6.0),
        (0.26, -0.08, -0.08, 0.08, 22.0), (-0.02, -0.07, -0.2, 0.065, 0.0)])


def swallow_nests_b():
    """Three nests in a short row along a ceiling with no wall behind (flat on top only), mouths out."""
    return swallow_nests('SwallowNests_B', 547, [
        (-0.17, -0.02, -0.085, 0.085, -15.0), (0.0, 0.0, -0.09, 0.09, 0.0), (0.17, -0.03, -0.08, 0.08, 18.0)],
        wall=False)


BUILDERS = {
    'CliffJuniper_A': cliff_juniper_a, 'CliffJuniper_B': cliff_juniper_b, 'CliffJuniper_C': cliff_juniper_c,
    'LedgeSage_A': ledge_sage_a, 'LedgeSage_B': ledge_sage_b,
    'LedgeGrass_A': ledge_grass_a, 'LedgeGrass_B': ledge_grass_b,
    'LipDrape_A': lip_drape_a, 'LipDrape_B': lip_drape_b, 'LipDrape_C': lip_drape_c,
    'SwallowNests_A': swallow_nests_a, 'SwallowNests_B': swallow_nests_b,
}


def build(names=None):
    return [maker() for name, maker in BUILDERS.items() if not names or name in names]


def previews(models):
    """Each model in the area's golden late-afternoon light (the sun at bearing 247.5, 15 degrees up), from the
    front-left three-quarter, on a plain ground."""
    os.makedirs(OUT, exist_ok=True)
    ground = S.ground_plane(30.0)
    for model in models:
        lo = Vector((min(c[0] for c in model.bound_box), min(c[1] for c in model.bound_box),
                     min(c[2] for c in model.bound_box)))
        hi = Vector((max(c[0] for c in model.bound_box), max(c[1] for c in model.bound_box),
                     max(c[2] for c in model.bound_box)))
        size = (hi - lo).length
        copy = S.place(model, (0.0, 0.0, -lo.z))
        target = (lo + hi) * 0.5 + Vector((0.0, 0.0, -lo.z))
        view = Vector((-0.45, -1.0, 0.3)).normalized()
        S.render([copy, ground], os.path.join(OUT, model.name + '.png'), target + view * max(size * 1.35, 0.9), target,
                 lens=35.0, resolution=(1280, 900), samples=48)
        bpy.data.objects.remove(copy)
    bpy.data.objects.remove(ground)


if __name__ == '__main__':
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    # Model names after '--' build only those (the exporter's own arguments are never model names).
    wanted = [a for a in argv if a in BUILDERS]
    models = build(wanted)
    lp.report(models)
    if lt.want_preview():
        previews(models)
