"""The Drover's Tripo parts: what every set's fitting shares (Art/Models/Weapons/Tripo/Drover/parts_set<N>.py use it; spec
format in looter_tripo_gun.py). Gun space, cm (Drover.py's layout): u along the gun from the frame's back, v up from the
bore, w to the gun's left.

What each slot keeps from its scripted part, and why:
- Body: the rarity accent strips (GunAccentGlow, crisp, the rarity's colour) and the ember vent core; the cylinder's window
  and the rod's channel are cut again through the Tripo frame (the cylinder turns and swings out through them).
- Barrel: the front blade and its cyan dot (on SIGHT_LINE, where the shots go: the irons' one sight picture); Tripo's own
  blade is trimmed off above the rib.
- Cylinder: the ejector rod, its sleeve and the crane (Tripo was shown the drum alone, drover_views.py); the drum is
  centred on the cylinder's axis, turned so a chamber sits on top (in line with the barrel at rest), and its outer wall
  trued round (it turns a chamber a shot in front of the eye).
- Sights: lenses and reticles; the notch, the peep's aperture, the dot sight's window and the scope's bore are cut open
  (aiming looks through them).
- Grip: nothing; SOCKET_Grip and SOCKET_Foregrip stay the scripted part's (no Drover grip has hand markers yet).
"""
import math

import looter_guns as lg
import looter_tripo_gun as tg

# Drover.py's numbers (kept in step by hand: Drover.py imports this module's user, not the other way round).
CYL_V = -1.2
CYL_U0, CYL_U1 = 2.1, 6.45
STRAP_TOP = 1.8
SIGHT_LINE = 3.0
HALF = 1.3
ROD_END = CYL_U1 + 4.6
BARREL_U = 9.2
CHAMBER_AT = 1.2
WINDOW = [(2.05, -3.4), (7.3, -3.4), (7.3, -0.95), (6.5, -0.95), (6.5, 1.08), (2.05, 1.08)]

GLOWS = ('GunAccentGlow', 'GunEmberGlow', 'GunCyanGlow', 'GunReticle', 'GunAmmoGlow', 'GunHeatGlow', 'GunSoulGlow',
         'GunSaltGlow')
AXES = dict(forward='+X', left='+Y')   # Tripo's multiview output with orientation align_image: the muzzle +X, the left +Y


# --- Colour tests (array-friendly: & and comparisons, so looter_tripo.paint_set runs them on the whole map) ---

def _hi_lo(c):
    r, g, b = c
    hi = r * (r >= g) * (r >= b) + g * (g > r) * (g >= b) + b * (b > r) * (b > g)
    lo = r * (r <= g) * (r <= b) + g * (g < r) * (g <= b) + b * (b < r) * (b < g)
    return hi, lo


def is_ivory(c):
    """Saint Maud's ivory enamel: light, warm, little colour."""
    r, g, b = c
    hi, lo = _hi_lo(c)
    return (hi > 0.55) & ((hi - lo) < 0.3 * hi) & (r >= b) & (g > 0.78 * r)


def is_oxblood(c):
    """Roundhouse's oxblood: a dull red, not the ember's bright orange nor the brass."""
    r, g, b = c
    return (r > 0.28) & (r < 0.78) & (g < 0.62 * r) & (b < 0.6 * r)


def is_teal(c):
    """Windfall's sea-teal paint."""
    r, g, b = c
    return (g > 0.25) & (g > 1.2 * r) & (b > 1.08 * r) & (g > 0.85 * b)


def is_sky(c):
    """Windfall's sky-blue grip."""
    r, g, b = c
    return (b > 0.4) & (b > 1.12 * r) & (b >= 0.98 * g) & (g > 0.9 * r)


def is_white(c):
    """Meridian's satin white (the aluminium is as light but bluer)."""
    r, g, b = c
    hi, lo = _hi_lo(c)
    return (hi > 0.62) & ((hi - lo) < 0.12 * hi) & (b - r < 0.015)


def is_light_grey(c):
    """Meridian's light grey polymer grip."""
    r, g, b = c
    hi, lo = _hi_lo(c)
    return (hi > 0.5) & ((hi - lo) < 0.16 * hi)


def is_ember(c):
    """The ember glow as Tripo paints it from the views (a strong orange; brighter than Roundhouse's oxblood)."""
    r, g, b = c
    return (r > 0.7) & (g > 0.28 * r) & (g < 0.68 * r) & (b < 0.35 * r)


def is_cyan(c):
    r, g, b = c
    return (b > 0.55) & (g > 0.5) & (r < 0.6 * b)


# Each set's shell paint (the maker's shell colour recolours it in the game; tinted the set's own colour, as the AR's
# sets, so a gun with no maker looks as Tripo made it). By face: a Tripo part is one piece, so the
# pieces rule (a piece mostly shell is shell all over) painted a whole grip's iron frame or none of a frame.
SHELLS = {
    'SaintMaud': dict(slot='GunPolymerSand', test=is_ivory, tint=0xe8e0cc, by='faces'),
    'Roundhouse': dict(slot='GunPolymerSand', test=is_oxblood, tint=0x7d2f24, mode='luma', by='faces'),
    'Windfall': dict(slot='GunPolymerSand', test=is_teal, tint=0x4f8f8a, mode='luma', by='faces'),
    'Meridian': dict(slot='GunPolymerSand', test=is_white, tint=0xe9e7e1, by='faces'),
}


# --- Keep rules ---

def keep_glows(p):
    return p.mat in GLOWS or p.mat == 'GunLens'


def keep_blade(tip):
    """The barrel's front blade (its slab and its dot or bead), standing on the rib's top at tip."""
    def rule(p):
        return keep_glows(p) or (p.lo[0] >= tip - 2.5 and p.hi[0] <= tip - 0.2 and p.hi[1] >= SIGHT_LINE)
    return rule


def keep_rod_crane(p):
    """The cylinder's rod, its head and knurl, the sleeve and crane arm (ahead of the drum) and the crane's hinge tube
    (off the axis): everything that isn't the drum."""
    cu = (p.lo[0] + p.hi[0]) * 0.5
    cv, cw = (p.lo[1] + p.hi[1]) * 0.5 - CYL_V, (p.lo[2] + p.hi[2]) * 0.5
    return keep_glows(p) or cu > CYL_U1 + 0.05 or math.hypot(cv, cw) > 2.0


# --- Hooks ---

def window_cuts(ctx):
    """The cylinder's window and the rod's channel through the front lug (Drover.py frame())."""
    return [lg.cslab(WINDOW, -3.0, 3.0, round=0.15), lg.cbox(7.2, 9.4, -1.62, -0.85, -0.42, 0.42)]


def trim_blade(tip):
    """Drops Tripo's front blade (faces over the rib, in the blade's u span): the scripted blade is kept instead."""
    def prepare(ctx):
        gone = [p.index for p in ctx.obj.data.polygons
                if tip - 2.6 <= ctx.tripo.gun_uvw(p.center)[0] <= tip - 0.1 and ctx.tripo.gun_uvw(p.center)[1] > 1.48]
        if gone:
            import looter_tripo_set4 as t4
            t4.delete_faces(ctx.obj, gone)
        ctx.log(f'{len(gone)} faces of Tripo\'s front blade dropped (the scripted blade kept)')
    return prepare


def drum(chambers, radius):
    """The cylinder: centred on its axis already by the fit (its box is the scripted drum's); turned so a chamber's
    mouth is on top, and its outer wall trued round."""
    def prepare(ctx):
        phase, share = tg.chamber_phase(ctx.obj, CYL_U1, CYL_V, CHAMBER_AT, chambers, image=ctx.maps['BC'])
        if phase is None:
            ctx.log(f'chambers not found on the front face ({share:.2f}): left as fitted')
        else:
            turn = -phase if phase <= 180.0 / chambers else 360.0 / chambers - phase
            tg.rotate_about_u(ctx.obj, turn, v=CYL_V)
            ctx.log(f'chambers {phase:.1f} deg off the top ({share:.2f}): turned {turn:+.1f}')
        moved = tg.true_round(ctx.obj, CYL_U0 + 0.3, CYL_U1 - 0.35, CYL_V, radius, band=0.16)
        ctx.log(f'{moved} outer-wall vertices trued onto r {radius}')
    return prepare


def notch_cuts(depth=0.52):
    """The rear's U notch through the Tripo plate (Drover.py sights(): 0.84 across, its bottom depth under the line)."""
    def cuts(ctx):
        L = SIGHT_LINE
        cut = [(0.42, L + 1.0), (0.42, L - depth + 0.2), (0.3, L - depth + 0.05), (0.0, L - depth),
               (-0.3, L - depth + 0.05), (-0.42, L - depth + 0.2), (-0.42, L + 1.0)]
        return [lg.face(cut, -1.0, 5.0, bevel=0.0)]
    return cuts


def peep_cuts(u=1.8, r=0.68):
    def cuts(ctx):
        return [lg.tube(u - 1.0, u + 1.0, r, v=SIGHT_LINE, segments=24)]
    return cuts


def pip_cuts(ctx):
    v0 = STRAP_TOP
    return [lg.cbox(3.0, 7.2, v0 + 0.8, v0 + 2.5, -0.65, 0.65)]


def scope_cuts(u_eye, profile, v, wall=0.13):
    """The scope's bore: its tube's inside (Drover.py's profile less the wall), open past both ends."""
    def cuts(ctx):
        inner = [(-1.0, profile[0][1] - wall)] + [(du, r - wall) for du, r in profile] + \
            [(profile[-1][0] + 1.0, profile[-1][1] - wall)]
        prof = [(du + 1.0, r) for du, r in inner]
        return [lg.turned(prof, u_eye - 1.0, v=v, segments=28)]
    return cuts


def vent_cuts(ctx):
    """The window, the rod's channel and the two slanted vents through the front lug (Drover.py vent_cuts()): the
    scripted ember core kept inside shows through them as on the scripted frame (Tripo painted the vents, didn't cut)."""
    vents = [lg.cslab([(7.46 + d, -2.92), (7.74 + d, -2.92), (8.04 + d, -1.98), (7.76 + d, -1.98)], -3.0, 3.0,
                      round=0.08) for d in (0.0, 0.42)]
    return window_cuts(ctx) + vents


def slit_cuts(ctx):
    """The window, the rod's channel and the Ironclad's armour slits (Drover.py: v -2.15..-1.85 at u 0.1..1.5 and
    7.6..8.7), cut from just inside the armour outward, so the scripted ember strips behind them glow through."""
    slits = []
    for u0, u1 in ((0.1, 1.5), (7.6, 8.7)):
        for side in (-1.0, 1.0):
            w0, w1 = sorted((side * (HALF + 0.1), side * (HALF + 1.2)))
            slits.append(lg.cbox(u0, u1, -2.15, -1.85, w0, w1))
    return window_cuts(ctx) + slits


def skin_line(points, r, glow_name, lift=0.015, depth=0.35):
    """A glowing line along side-view points (u, v) on both sides' skin (the views' painted cyan line, too thin in
    Tripo's atlas to become glow faces): rays from each side find the skin; the line breaks where the skin is more
    than depth cm inside the outermost hit (the window, a gap)."""
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree

    def prepare(ctx):
        me = ctx.obj.data
        tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])
        mat = ctx.glow(glow_name)
        pieces = 0
        for side in (-1.0, 1.0):
            hits = []
            for u, v in _resample(points, 0.25):
                hit = tree.ray_cast(lg.at(u, v, side * 10.0), Vector((-side, 0.0, 0.0)), 20.0 * lg.CM)[0]
                hits.append((u, v, hit.x / lg.CM) if hit is not None else None)
            outer = max((abs(h[2]) for h in hits if h), default=0.0)
            run = []
            for h in hits + [None]:
                if h is not None and abs(h[2]) > outer - depth:
                    run.append((h[0], h[1], h[2] + side * lift))
                    continue
                if len(run) > 2:
                    ctx.add(lg.pipe(run, r, mat=mat, sides=4))
                    pieces += 1
                run = []
        ctx.log(f'{pieces} {glow_name} lines laid on the skin')
    return prepare


def _resample(points, step):
    out = []
    for (u0, v0), (u1, v1) in zip(points, points[1:]):
        n = max(1, int(math.hypot(u1 - u0, v1 - v0) / step))
        out += [(u0 + (u1 - u0) * k / n, v0 + (v1 - v0) * k / n) for k in range(n)]
    return out + [points[-1]]


def both(*hooks):
    """Several prepare hooks in turn."""
    def prepare(ctx):
        for hook in hooks:
            hook(ctx)
    return prepare
