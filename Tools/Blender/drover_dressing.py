"""Concept-only dressing for the Drover's Tripo views (lane TRIPO-PISTOLS, 2026-11-01): each set's maker detail added to
the scripted part before its four views are rendered (Tools/Blender/drover_views.py), so Tripo models it with the rivets,
bands, tape, pinstripes and grip texture the AR's and Ranchhand's Tripo sets have (their concept views were drawn with that
detail). Nothing here reaches the game's scripted parts: the game gets it only through the Tripo model.

The makers' looks (Docs/Work/2026-10-22-Next/MAKERS.md; the AR's and Ranchhand's Tripo sets, Art/Concepts/Weapons/*/README.md):
  1 Saint Maud  plain and careful: ivory enamel, cast iron, gilt pinstripes and screws, fine checkering, a little ember
  2 Roundhouse  railway heavy: oxblood, black iron, big brass rivets, cream stencil chevrons and bands
  3 Windfall    scrap: rust, galvanized patches, black friction tape, hose clamps, copper wire, sea teal paint
  4 Meridian    instrument-precise: satin white, graphite, machined aluminium rings, graduation ticks, a cyan line,
                stippled grips
Everything stays within a few tenths of a mm of the part's skin and inside its outline: the cylinder turns in the
frame's window with 0.13 cm to spare, so nothing on it stands more than 0.1 proud.
"""
import math

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_guns as lg
import looter_props as lp
import looter_textures as lt

CM = lg.CM


def mat(name, set_name, tint):
    return bpy.data.materials.get(name) or lt.material(set_name, name=name, master='Gun', tint=tint)


class Dresser:
    """Adds pieces (built in gun space, cm) to a built part object (its mesh is in the part's own space: gun space less
    its origin), and finds its skin by rays."""

    def __init__(self, obj, origin, mod, looks):
        self.obj, self.origin, self.mod, self.looks = obj, lg.at(*origin), mod, looks
        me = obj.data
        self.tree = BVHTree.FromPolygons([v.co + self.origin for v in me.vertices], [list(p.vertices) for p in me.polygons])
        self.pieces = []

    def m(self, base):
        """The maker's colour of a scripted material (base: GunBrass, GunPolymerSand, ...), a dressing material, or a
        glow as it is (GunCyanGlow)."""
        if base not in self.looks:
            return bpy.data.materials[base]
        set_name, tint = self.looks[base]
        return mat(f'{base}_{self.looks["_maker"]}', set_name, tint)

    def add(self, *objs):
        self.pieces += [o for o in objs if o is not None]

    def side_w(self, u, v, side, reach=8.0):
        """The skin's w at (u, v) seen from the gun's left (side +1) or right (-1), or None."""
        hit = self.tree.ray_cast(lg.at(u, v, side * reach), Vector((-side, 0.0, 0.0)), 2 * reach * CM)[0]
        return hit.x / CM if hit is not None else None

    def top_v(self, u, w, reach=8.0):
        hit = self.tree.ray_cast(lg.at(u, reach, w), Vector((0.0, 0.0, -1.0)), 2 * reach * CM)[0]
        return hit.z / CM if hit is not None else None

    def rivets(self, points, r, base, sides=(-1.0, 1.0), sink=0.04):
        for side in sides:
            for u, v in points:
                w = self.side_w(u, v, side)
                if w is not None:
                    self.add(lg.rivet(u, v, w - side * sink, side, r=r, mat=self.m(base)))

    def side_line(self, pts, r, base, sides=(-1.0, 1.0), lift=0.0, flat=True):
        """A thin inlaid line along side-view points (u, v), on the skin of each side."""
        for side in sides:
            run = []
            for u, v in pts:
                w = self.side_w(u, v, side)
                if w is not None:
                    run.append((u, v, w + side * lift))
                elif len(run) > 1:
                    self.add(self._line(run, r, base, flat))
                    run = []
                else:
                    run = []
            if len(run) > 1:
                self.add(self._line(run, r, base, flat))

    def _line(self, pts, r, base, flat):
        # Four sides lie flatter on the skin (an inlay); six read as a wire.
        return lg.pipe(pts, r, mat=self.m(base), sides=4 if flat else 6)

    def side_patch(self, outline, base, sides=(-1.0, 1.0), thick=0.06, lift=0.0):
        """A thin plate (a stencil, a painted chevron, a galvanized patch) on each side's skin, at the skin's w in the
        outline's middle."""
        cu = sum(p[0] for p in outline) / len(outline)
        cv = sum(p[1] for p in outline) / len(outline)
        for side in sides:
            w = self.side_w(cu, cv, side)
            if w is not None:
                self.add(lg.slab(outline, thick, w=w + side * (lift + thick * 0.5 - 0.03), bevel=0.0, mat=self.m(base)))

    def band(self, u0, u1, r, base, v=0.0, segments=32):
        self.add(lg.tube(u0, u1, r, v=v, segments=segments, mat=self.m(base)))

    def helix(self, u0, u1, r, turns, wire, base, v=0.0, n=120):
        pts = []
        for k in range(n + 1):
            t = k / n
            a = 2.0 * math.pi * turns * t
            pts.append((u0 + (u1 - u0) * t, v + r * math.cos(a), r * math.sin(a)))
        self.add(lg.pipe(pts, wire, mat=self.m(base), sides=6))

    def ticks(self, u0, u1, step, v0, v1, base, sides=(-1.0, 1.0), long_every=5, width=0.04):
        """Graduation ticks on a side face (every long_every-th one longer)."""
        for side in sides:
            k = 0
            u = u0
            while u <= u1 + 1e-6:
                top = v1 if k % long_every == 0 else v0 + (v1 - v0) * 0.55
                w = self.side_w(u, (v0 + top) * 0.5, side)
                if w is not None:
                    self.add(lg.box(u - width * 0.5, u + width * 0.5, v0, top, 0.05, w=w + side * 0.0,
                                    bevel=0.0, mat=self.m(base)))
                u += step
                k += 1

    def round_ticks(self, u0, u1, r, count, base, v=0.0, wide=0.05):
        """Knurl-like ticks round a drum or barrel."""
        for k in range(count):
            a = 2.0 * math.pi * k / count
            box = lg.box(u0, u1, v + r - 0.04, v + r + 0.03, wide, bevel=0.0, mat=self.m(base))
            box.data.transform(_about_u(a, v))
            self.add(box)

    def wrap_band(self, outline, width, a, b, base, grow=0.06):
        """A band wrapped round a grip: the grip's side outline clipped between two lines across it (a, b: two points
        each, the band's lower and upper edges), grown a little, extruded across the grip a little wider than it."""
        poly = _clip(_clip(_grow(outline, grow), a, keep_left=True), b, keep_left=False)
        if len(poly) >= 3:
            self.add(lg.slab(poly, width + 2 * grow, bevel=0.03, mat=self.m(base)))

    def stipple(self, outline, step, base, inset=0.5, r=0.05, sides=(-1.0, 1.0)):
        """A grid of fine raised dots on a grip panel's sides, inside its outline (inset cm from its edge)."""
        lo_u, hi_u = min(p[0] for p in outline), max(p[0] for p in outline)
        lo_v, hi_v = min(p[1] for p in outline), max(p[1] for p in outline)
        inner = _grow(outline, -inset)
        v, row = lo_v, 0
        while v < hi_v:
            u = lo_u + (step * 0.5 if row % 2 else 0.0)
            while u < hi_u:
                if _inside(inner, u, v):
                    for side in sides:
                        w = self.side_w(u, v, side)
                        if w is not None:
                            self.add(lg.rivet(u, v, w - side * 0.02, side, r=r, mat=self.m(base)))
                u += step
            v += step * 0.866
            row += 1

    def checker(self, outline, step, base, inset=0.45, angle=35.0, r=0.022, sides=(-1.0, 1.0)):
        """Diamond checkering lines on a grip panel's sides, inside its outline."""
        inner = _grow(outline, -inset)
        lo_u, hi_u = min(p[0] for p in inner), max(p[0] for p in inner)
        lo_v, hi_v = min(p[1] for p in inner), max(p[1] for p in inner)
        span = math.hypot(hi_u - lo_u, hi_v - lo_v)
        cu, cv = (lo_u + hi_u) * 0.5, (lo_v + hi_v) * 0.5
        for sign in (-1.0, 1.0):
            a = math.radians(sign * angle + 90.0)
            d = (math.cos(a), math.sin(a))
            n = (-d[1], d[0])
            k = -span
            while k <= span:
                pts = []
                for j in range(81):
                    t = -span + 2 * span * j / 80
                    u, v = cu + n[0] * k + d[0] * t, cv + n[1] * k + d[1] * t
                    pts.append((u, v) if _inside(inner, u, v) else None)
                run = []
                for p in pts + [None]:
                    if p is not None:
                        run.append(p)
                    else:
                        if len(run) > 1:
                            self.side_line(run, r, base, sides=sides)
                        run = []
                k += step

    def finish(self):
        """Joins the pieces into the part object (moved into its own space)."""
        if not self.pieces:
            return 0
        from mathutils import Matrix
        for p in self.pieces:
            p.data.transform(Matrix.Translation(-self.origin))
        objs = [self.obj] + self.pieces
        with bpy.context.temp_override(active_object=self.obj, object=self.obj, selected_objects=objs,
                                       selected_editable_objects=objs):
            bpy.ops.object.join()
        return len(self.pieces)


def _about_u(a, v):
    from mathutils import Matrix
    pivot = lg.at(0.0, v, 0.0)
    return Matrix.Translation(pivot) @ Matrix.Rotation(a, 4, 'Y') @ Matrix.Translation(-pivot)


def _inside(poly, u, v):
    hit = False
    n = len(poly)
    for i in range(n):
        (u0, v0), (u1, v1) = poly[i][:2], poly[(i + 1) % n][:2]
        if (v0 > v) != (v1 > v) and u < u0 + (v - v0) * (u1 - u0) / (v1 - v0):
            hit = not hit
    return hit


def _grow(poly, d):
    """The outline pushed out (d > 0) or in by d cm along its corners' bisectors (good enough for convex-ish grips)."""
    n = len(poly)
    area = sum(poly[i][0] * poly[(i + 1) % n][1] - poly[(i + 1) % n][0] * poly[i][1] for i in range(n))
    s = 1.0 if area > 0 else -1.0   # counterclockwise: outward normal is (dv, -du)
    out = []
    for i in range(n):
        p0, p1, p2 = poly[i - 1][:2], poly[i][:2], poly[(i + 1) % n][:2]
        e1 = Vector((p1[0] - p0[0], p1[1] - p0[1])).normalized()
        e2 = Vector((p2[0] - p1[0], p2[1] - p1[1])).normalized()
        n1, n2 = Vector((e1.y, -e1.x)) * s, Vector((e2.y, -e2.x)) * s
        bis = (n1 + n2)
        if bis.length < 1e-6:
            bis = n1
        bis.normalize()
        k = 1.0 / max(0.35, bis.dot(n1))
        out.append((p1[0] + bis.x * d * k, p1[1] + bis.y * d * k))
    return out


def _clip(poly, line, keep_left):
    """Sutherland-Hodgman: the polygon's part on one side of the line through two points."""
    (ax, ay), (bx, by) = line
    side = lambda p: ((bx - ax) * (p[1] - ay) - (by - ay) * (p[0] - ax)) * (1.0 if keep_left else -1.0)
    out = []
    n = len(poly)
    for i in range(n):
        p, q = poly[i], poly[(i + 1) % n]
        sp, sq = side(p), side(q)
        if sp >= 0:
            out.append(p)
        if (sp >= 0) != (sq >= 0):
            t = sp / (sp - sq)
            out.append((p[0] + (q[0] - p[0]) * t, p[1] + (q[1] - p[1]) * t))
    return out


# --- Each part's dressing: fn(d, mod) ---

def saint_body(d, mod):
    # Gilt pinstripes along the top strap's sides and under the window bar, gilt screws, a gilt rosette on the shield.
    d.side_line([(1.3, 1.42), (5.0, 1.36), (8.6, 1.42)], 0.04, 'GunBrass', lift=0.0)
    d.side_line([(0.0, -3.62), (1.7, -3.62)], 0.04, 'GunBrass')
    d.rivets([(0.75, -2.65), (8.0, -3.1)], 0.2, 'GunBrass')


def saint_barrel(d, mod):
    u0 = mod.BARREL_AT[0]
    d.band(u0 + 1.25, u0 + 1.55, 0.95, 'GunBrass')
    d.band(u0 + 12.6, u0 + 12.85, 0.95, 'GunBrass')
    d.side_line([(mod.ROD_END + 1.3, -1.05), (u0 + 12.9, -1.05)], 0.035, 'GunBrass')


def saint_cylinder(d, mod):
    v = mod.CYL_V
    d.band(mod.CYL_U1 - 0.42, mod.CYL_U1 - 0.3, 2.19, 'GunBrass', v=v)


def saint_sight(d, mod):
    L = mod.SIGHT_LINE
    # A gilt bar under the notch on the eye's face and a gilt screw each side.
    d.add(lg.box(0.96, 1.0, L - 0.85, L - 0.75, 0.5, bevel=0.0, mat=d.m('GunBrass')))
    d.rivets([(1.8, mod.STRAP_TOP + 0.4)], 0.1, 'GunBrass')


def saint_grip(d, mod):
    d.checker(mod.PLOW, 0.42, 'GunBlackMetal', inset=0.75)
    d.rivets([mod.rk(-1.0, -7.4)], 0.32, 'GunBrass')


def round_body(d, mod):
    # Big brass rivets round the plates, cream chevrons on the front lug.
    d.rivets([(u, 1.3) for u in (0.6, 2.4, 4.2, 6.0, 7.8)], 0.2, 'GunBrass')
    d.rivets([(-0.2, -3.5), (1.6, -3.5), (-0.2, -1.4)], 0.22, 'GunBrass')
    for k in range(3):
        u = 7.45 + k * 0.42
        d.side_patch([(u, -3.3), (u + 0.18, -3.3), (u + 0.5, -2.2), (u + 0.32, -2.2)], 'GunCream')


def round_barrel(d, mod):
    u0 = mod.BARREL_AT[0]
    tip = u0 + 15.0
    d.rivets([(u, -1.75) for u in (u0 + 1.2, u0 + 4.0, u0 + 6.8, u0 + 9.6, u0 + 12.4)], 0.17, 'GunBrass')
    d.band(tip - 1.9, tip - 1.4, 0.95, 'GunCream')
    d.band(u0 + 1.15, u0 + 1.7, 1.0, 'GunBlackMetal')


def round_cylinder(d, mod):
    v = mod.CYL_V
    d.band(mod.CYL_U0 + 1.95, mod.CYL_U0 + 2.4, 2.33, 'GunCream', v=v)


def round_sight(d, mod):
    v0 = mod.STRAP_TOP
    v = v0 + 2.5
    d.band(1.0, 1.6, 0.9, 'GunCream', v=v)
    d.band(7.6, 8.2, 0.9, 'GunCream', v=v)
    d.rivets([(2.0, v0 + 0.9), (5.6, v0 + 0.9)], 0.13, 'GunBrass')


def round_grip(d, mod):
    rk = mod.rk
    d.rivets([rk(-1.6, -4.4), rk(1.0, -4.4), rk(-2.4, -11.4), rk(-0.2, -11.4)], 0.22, 'GunBrass')
    for k in range(3):
        v = -9.0 - k * 0.7
        d.side_patch([rk(-3.7, v), rk(0.6, v + 0.9), rk(0.6, v + 1.2), rk(-3.7, v + 0.3)], 'GunCream')


def wind_body(d, mod):
    # A hazard-yellow tag riveted on the right, black friction tape round the strap's front.
    d.side_patch([(3.0, 1.15), (4.4, 1.15), (4.4, 1.7), (3.0, 1.7)], 'GunHazard', sides=(-1.0,))
    d.add(lg.box(7.9, 8.7, 1.05, 1.95, 2 * mod.HALF + 0.12, bevel=0.06, mat=d.m('GunTape')))


def wind_barrel(d, mod):
    u0 = mod.BARREL_AT[0]
    d.band(u0 + 2.0, u0 + 3.6, 0.97, 'GunTape', segments=24)
    # A hose clamp: a steel band and its screw housing on the right.
    d.band(u0 + 6.0, u0 + 6.45, 0.98, 'GunSteelBare')
    d.add(lg.box(u0 + 5.85, u0 + 6.6, -0.35, 0.35, 0.45, w=-1.1, bevel=0.06, mat=d.m('GunSteelBare')))
    d.add(lg.pin(u0 + 6.22, 0.0, -1.5, -1.25, 0.16, mat=d.m('GunSteelBare'), segments=8))
    d.helix(u0 + 8.0, u0 + 9.0, 0.95, 5, 0.05, 'GunBrass')


def wind_cylinder(d, mod):
    v = mod.CYL_V
    d.band(mod.CYL_U0 + 1.6, mod.CYL_U0 + 2.3, 2.17, 'GunPolymerSand', v=v)


def wind_sight(d, mod):
    v0 = mod.STRAP_TOP
    d.helix(1.6, 2.0, 0.38, 4, 0.04, 'GunBrass', v=v0 + 0.55)
    d.add(lg.box(2.4, 3.2, v0 - 0.02, v0 + 0.3, 1.06, bevel=0.04, mat=d.m('GunTape')))


def wind_grip(d, mod):
    rk = mod.rk
    width = 3.5
    for k, (va, vb) in enumerate(((-5.0, -6.3), (-7.4, -8.8), (-9.9, -11.1))):
        a = (rk(-6.0, va - 0.5), rk(4.0, va + 0.3))
        b = (rk(-6.0, vb - 0.5), rk(4.0, vb + 0.3))
        d.wrap_band(mod.WRAP, width, b, a, 'GunTape')
    d.wrap_band(mod.WRAP, width, (rk(-6.0, -11.75), rk(4.0, -11.75)), (rk(-6.0, -11.5), rk(4.0, -11.5)), 'GunBrass',
                grow=0.09)


def meri_body(d, mod):
    # A cyan line along the sides under the strap, graduation ticks on the strap cap.
    d.side_line([(0.9, 0.9), (4.0, 0.85), (8.9, 0.9)], 0.04, 'GunCyanGlow')
    d.ticks(1.5, 8.3, 0.34, 1.2, 1.62, 'GunBlackMetal')


def meri_barrel(d, mod):
    u0 = mod.BARREL_AT[0]
    tip = u0 + 15.0
    d.band(u0 + 1.15, u0 + 1.5, 1.0, 'GunSteelBare')
    d.band(tip - 0.9, tip - 0.6, 0.95, 'GunSteelBare')
    d.side_line([(mod.ROD_END + 1.3, -1.2), (tip - 1.2, -1.2)], 0.035, 'GunCyanGlow')
    d.ticks(mod.ROD_END + 1.6, tip - 1.6, 0.5, -1.0, -0.65, 'GunBlackMetal')


def meri_cylinder(d, mod):
    v = mod.CYL_V
    d.band(mod.CYL_U1 - 0.36, mod.CYL_U1 - 0.22, 2.19, 'GunSteelBare', v=v)
    d.round_ticks(mod.CYL_U0 + 0.05, mod.CYL_U0 + 0.3, 2.17, 48, 'GunBlackMetal', v=v)


def meri_sight(d, mod):
    v0 = mod.STRAP_TOP
    d.add(lg.box(3.72, 3.84, v0 + 0.5, v0 + 2.55, 1.86, bevel=0.0, mat=d.m('GunSteelBare')))   # a rear bezel
    d.add(lg.upright(5.0, v0 + 2.9, v0 + 3.2, 0.32, mat=d.m('GunBrass'), segments=16))         # an adjuster cap
    d.side_line([(1.2, v0 + 0.18), (6.4, v0 + 0.18)], 0.03, 'GunCyanGlow')


def meri_grip(d, mod):
    d.stipple(mod.TARGET, 0.3, 'GunPolymerBlack', inset=0.8)
    d.rivets([mod.rk(-1.4, -5.4)], 0.36, 'GunSteelBare')


DRESS = {
    'Body_Blued': saint_body, 'Barrel_Service': saint_barrel, 'Cylinder_Fluted6': saint_cylinder,
    'Sight_Notch': saint_sight, 'Grip_Plowhandle': saint_grip,
    'Body_Ironclad': round_body, 'Barrel_Lugged': round_barrel, 'Cylinder_Bigbore5': round_cylinder,
    'Sight_Glass': round_sight, 'Grip_Coffin': round_grip,
    'Body_Scrapper': wind_body, 'Barrel_Comped': wind_barrel, 'Cylinder_Plain6': wind_cylinder,
    'Sight_Peep': wind_sight, 'Grip_Wrapped': wind_grip,
    'Body_Ranger': meri_body, 'Barrel_Vented': meri_barrel, 'Cylinder_Moon6': meri_cylinder,
    'Sight_Pip': meri_sight, 'Grip_Target': meri_grip,
}
EXTRA = {   # dressing materials beyond the scripted ones, per maker
    'SaintMaud': {'GunCream': ('Polymer', 0xefe3c4)},
    'Roundhouse': {'GunCream': ('Polymer', 0xefe3c4)},
    'Windfall': {'GunHazard': ('Polymer', 0xd8a832), 'GunTape': ('Polymer', 0x232323)},
    'Meridian': {},
}


def dress(key, obj, origin, mod, maker, looks):
    fn = DRESS.get(key)
    if fn is None:
        return 0
    table = dict(looks[maker])
    table.update(EXTRA.get(maker, {}))
    table['_maker'] = maker
    d = Dresser(obj, origin, mod, table)
    fn(d, mod)
    return d.finish()

