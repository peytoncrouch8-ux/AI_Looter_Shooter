"""Pike's own procedural looks (Art/Models/Characters/Pike.py), on looter_looks' machinery: each ends in the four OUT_
reroutes the bake reads. Patterns stay under ~300 cycles/m (finer ones alias at the bake's ~1000 px/m), creases few,
broad and soft (a dense fine network read as cracked stone; looter_looks' notes).
"""
import looter_looks as lk
from looter_looks import construction, dust, grime, ridged, splatter, stains, wear


def drill(name, dark, light, dust_amt=0.5, thread=(0.70, 0.64, 0.48)):
    """Khaki cotton drill, sun-faded and field-worn: blotched lighter where the sun took it, sweat and water stains,
    long soft drape creases and crumpled ones at the elbows and the waist, edges rubbed pale, grime in the folds, dust
    from the ground up and on the shoulders, topstitched edges and sewn seams."""
    mat, g = lk.new_material(name, 'drill')
    big = g.noise(3.0, 5.0, 0.6)
    base = g.mix(g.ramp(big, 0.25, 0.8), dark, light)
    fade = g.noise(1.2, 3.0, 0.5, vec=g.mapping((1.0, 1.0, 0.5)))
    base = g.mix(g.mul(g.ramp(fade, 0.5, 0.8), 0.45), base, tuple(min(1.0, c * 1.12 + 0.03) for c in light))
    drape = ridged(g, 8.0, stretch=(1.0, 1.0, 0.3), width=0.10, dist=0.3)
    crumple = ridged(g, 13.0, stretch=(1.0, 1.0, 0.7), width=0.11, detail=2.0, dist=0.35)
    creased = g.math('MAXIMUM', g.mul(drape, 0.8), g.mul(crumple, 0.45))
    base = g.mix(g.mul(creased, 0.30), base, tuple(c * 0.62 for c in dark))
    base = g.mix(wear(g, 0.45, 0.58), base, tuple(min(1.0, c * 1.25 + 0.06) for c in light))
    base = g.mix(grime(g, 0.6, 0.04), base, tuple(c * 0.35 for c in dark))
    base = g.mix(stains(g, 0.30, 5.0), base, tuple(c * 0.72 for c in dark))
    base = g.mix(splatter(g, 0.35, top=0.9, scale=45.0), base, (0.30, 0.25, 0.18))
    base = g.mix(dust(g, (0.62, 0.56, 0.46), dust_amt, height=0.7, up_bias=0.8), base, (0.62, 0.56, 0.46))
    stitches, sewn = construction(g, rows=(0.006, 0.012), hem=0.022)
    base = g.mix(g.mul(stitches, 0.75), base, thread)
    # a fine weave bump well under the bake's limit, the creases and seams
    n = g.bump(g.noise(240.0, 3.0, 0.6), 0.08, 0.0003)
    n = g.bump(g.noise(18.0, 6.0, 0.62), 0.25, 0.003, n)
    n = g.bump(g.add(g.mul(drape, -1.0), g.mul(crumple, -0.5)), 0.42, 0.0022, n)
    n = g.bump(sewn, 0.6, 0.0014, n)
    rough = g.mixf(g.mul(creased, 0.4), 0.84, 0.92)
    g.finish(base, rough, 0.0, n, sheen=0.3)
    return mat


def tweed(name, dark, light):
    """The brown wool waistcoat: a soft flecked tweed (warm flecks, under the bake's limit), a few creases across the
    stomach, its edges stitched."""
    mat, g = lk.new_material(name, 'tweed')
    base = g.mix(g.ramp(g.noise(9.0, 5.0, 0.6), 0.3, 0.7), dark, light)
    fleck = g.ramp(g.noise(160.0, 2.0, 0.5), 0.66, 0.74)
    base = g.mix(g.mul(fleck, 0.45), base, (0.52, 0.36, 0.20))
    fleck2 = g.ramp(g.noise(140.0, 2.0, 0.5, vec=g.mapping((1.0, 1.0, 1.0), rot=(0.7, 0.3, 0.0))), 0.70, 0.76)
    base = g.mix(g.mul(fleck2, 0.35), base, (0.09, 0.06, 0.04))
    creases = ridged(g, 10.0, stretch=(1.0, 1.0, 2.5), width=0.08)
    base = g.mix(grime(g, 0.5, 0.03), base, tuple(c * 0.4 for c in dark))
    base = g.mix(wear(g, 0.5), base, tuple(min(1.0, c * 1.5 + 0.04) for c in light))
    stitches, sewn = construction(g, rows=(0.005,), hem=0.0)
    base = g.mix(g.mul(stitches, 0.5), base, tuple(min(1.0, c * 1.6) for c in light))
    n = g.bump(g.noise(220.0, 3.0, 0.6), 0.12, 0.0003)
    n = g.bump(g.mul(creases, -1.0), 0.18, 0.0009, n)
    n = g.bump(sewn, 0.4, 0.001, n)
    g.finish(base, 0.9, 0.0, n, sheen=0.45)
    return mat


def puttee(name, color):
    """Wool puttees wound from the knee to the boot: the wool's twill running along each wrap (the wraps are in the
    geometry; the bake brings their edges), mud and dust thick toward the boot."""
    mat, g = lk.new_material(name, 'puttee')
    base = g.mix(g.ramp(g.noise(7.0, 5.0, 0.6), 0.3, 0.7), tuple(c * 0.78 for c in color), color)
    twill = ridged(g, 60.0, stretch=(1.0, 1.0, 3.0), width=0.25, detail=1.0)
    base = g.mix(g.mul(twill, 0.18), base, tuple(c * 0.7 for c in color))
    base = g.mix(grime(g, 0.75, 0.03), base, tuple(c * 0.35 for c in color))
    base = g.mix(wear(g, 0.5), base, tuple(min(1.0, c * 1.3) for c in color))
    base = g.mix(splatter(g, 0.8, top=0.32, scale=50.0), base, (0.24, 0.20, 0.15))
    base = g.mix(dust(g, (0.55, 0.50, 0.42), 0.8, height=0.45, up_bias=0.5), base, (0.55, 0.50, 0.42))
    n = g.bump(twill, 0.12, 0.0004)
    n = g.bump(g.noise(22.0, 5.0, 0.6), 0.2, 0.002, n)
    g.finish(base, 0.9, 0.0, n, sheen=0.4)
    return mat


def blanket(name, color, stripe):
    """The bedroll's wool blanket: a trade blanket's pale stripe near each end of the roll (across x), felted and
    pilled, dusty."""
    mat, g = lk.new_material(name, 'blanket')
    x = g.xyz()[0]
    band = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', g.math('ABSOLUTE', x), 0.15)), 0.018, 0.010)
    base = g.mix(g.ramp(g.noise(8.0, 5.0, 0.6), 0.3, 0.7), tuple(c * 0.75 for c in color), color)
    base = g.mix(band, base, stripe)
    base = g.mix(grime(g, 0.6, 0.03), base, tuple(c * 0.4 for c in color))
    base = g.mix(dust(g, (0.55, 0.50, 0.42), 0.5, height=1.2, up_bias=0.8), base, (0.55, 0.50, 0.42))
    n = g.bump(g.noise(200.0, 3.0, 0.6), 0.15, 0.0004)
    n = g.bump(g.mul(ridged(g, 16.0, width=0.1), -1.0), 0.3, 0.002, n)
    g.finish(base, 0.95, 0.0, n, sheen=0.5)
    return mat


def grosgrain(name, color):
    """The bowler's ribbed silk band: fine ribs running round it (vertical lines in object space, under the limit)."""
    mat, g = lk.new_material(name, 'grosgrain')
    rib = g.wave(260.0, direction='Z', profile='SIN')
    base = g.mix(g.mul(rib, 0.25), color, tuple(min(1.0, c * 1.8) for c in color))
    base = g.mix(dust(g, (0.5, 0.45, 0.4), 0.3, height=0.001, up_bias=0.9), base, (0.5, 0.45, 0.4))
    n = g.bump(rib, 0.15, 0.0003)
    g.finish(base, g.mixf(rib, 0.45, 0.6), 0.0, n, sheen=0.4)
    return mat


def wood(name, dark, light):
    """The tripod's ash legs: straight grain along them (the legs stand near upright: grain drawn out along z), oiled,
    the edges worn pale, nicks."""
    mat, g = lk.new_material(name, 'wood')
    grain = g.noise(1.0, 4.0, 0.6, vec=g.mapping((90.0, 90.0, 3.0)))
    lines = g.ramp(grain, 0.42, 0.58)
    base = g.mix(lines, dark, light)
    base = g.mix(wear(g, 0.6), base, tuple(min(1.0, c * 1.3) for c in light))
    base = g.mix(grime(g, 0.5, 0.02), base, tuple(c * 0.5 for c in dark))
    nicks = ridged(g, 30.0, width=0.05)
    base = g.mix(g.mul(nicks, 0.3), base, tuple(c * 0.6 for c in dark))
    n = g.bump(grain, 0.15, 0.0004)
    n = g.bump(g.mul(nicks, -1.0), 0.3, 0.0006, n)
    g.finish(base, g.mixf(lines, 0.42, 0.6), 0.0, n)
    return mat


def glass(name, color):
    """The goggles' smoked glass: dark, glossy, a little dust settled on it."""
    mat, g = lk.new_material(name, 'glass')
    base = g.mix(dust(g, (0.5, 0.48, 0.44), 0.25, height=0.001, up_bias=0.9), color, (0.5, 0.48, 0.44))
    g.finish(base, g.mixf(g.noise(30.0, 3.0, 0.5), 0.06, 0.18), 0.0, None)
    return mat
