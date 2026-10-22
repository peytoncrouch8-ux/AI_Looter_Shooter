"""Hollis Crane's own procedural looks (Cycles node trees on looter_looks' G writer, ending in its OUT_ reroutes so
looter_bake reads them), for what looter_looks has no look for: the slate coat (old wool broadcloth gone grey-blue,
faded on the crests, swamp mud and mildew up from the hem), the ferryman's coin (its relief rubbed bright, the field dull,
verdigris in the recesses), the dead eye under it, and the rusted iron of the toll chain. Patterns stay under ~300/m
(finer ones alias at the bake's ~1000 px/m)."""
import looter_looks as lk
from looter_looks import construction, dust, grime, ridged, splatter, stains, wear


def slate(name, dark, light, hem=0.30, thread=(0.36, 0.38, 0.38)):
    """Wool broadcloth forty years in the weather: a dark slate blue blotched paler, long streaks where rain ran, the
    nap worn grey on fold crests and edges, grime in the folds, water stains, black swamp mud and a green mildew bloom
    climbing from the hem, dust on the shoulders; long drape creases and fewer crumples, a fine twill-ish grain under
    them; hems, topstitching and seams from 'Seams'."""
    mat, g = lk.new_material(name, 'slate')
    big = g.noise(2.4, 5.0, 0.62)
    base = g.mix(g.ramp(big, 0.3, 0.75), dark, light)
    streak = g.noise(1.0, 3.0, 0.5, vec=g.mapping((5.0, 5.0, 0.6)))
    base = g.mix(g.mul(g.ramp(streak, 0.45, 0.75), 0.3), base, light)
    drape = ridged(g, 7.0, stretch=(1.0, 1.0, 0.22), width=0.11, dist=0.3)
    crumple = ridged(g, 12.0, stretch=(1.0, 1.0, 0.6), width=0.12, detail=2.0, dist=0.3)
    creased = g.math('MAXIMUM', g.mul(drape, 0.9), g.mul(crumple, 0.35))
    base = g.mix(g.mul(creased, 0.35), base, tuple(c * 0.55 for c in dark))
    worn = wear(g, 0.5, 0.6)
    base = g.mix(worn, base, (0.40, 0.43, 0.45))                       # the nap rubbed off: grey, not chalk
    base = g.mix(grime(g, 0.7, 0.05), base, (0.025, 0.028, 0.03))
    base = g.mix(stains(g, 0.4), base, tuple(c * 0.55 for c in dark))
    z = g.xyz()[2]
    mud = g.mul(g.ramp(z, hem + 0.24, hem - 0.02), g.ramp(g.noise(14.0, 6.0, 0.7, dist=0.4), 0.46, 0.60), clamp=True)
    base = g.mix(g.mul(mud, 0.85), base, (0.075, 0.07, 0.055))
    mildew = g.mul(g.ramp(z, hem + 0.40, hem + 0.05), g.ramp(g.noise(22.0, 6.0, 0.7, dist=0.5), 0.55, 0.68), clamp=True)
    base = g.mix(g.mul(mildew, 0.45), base, (0.22, 0.27, 0.19))
    base = g.mix(splatter(g, 0.6, top=hem + 0.25), base, (0.12, 0.11, 0.09))
    base = g.mix(dust(g, (0.46, 0.47, 0.46), 0.5, height=hem + 0.4, up_bias=0.8), base, (0.46, 0.47, 0.46))
    stitches, sewn = construction(g)
    base = g.mix(g.mul(stitches, 0.7), base, thread)
    h = g.add(g.mul(drape, -1.0), g.mul(crumple, -0.4))
    n = g.bump(g.noise(240.0, 3.0, 0.6), 0.10, 0.0004)
    n = g.bump(g.noise(16.0, 6.0, 0.62), 0.30, 0.004, n)
    n = g.bump(h, 0.4, 0.0025, n)
    n = g.bump(sewn, 0.6, 0.0015, n)
    rough = g.mixf(worn, 0.78, 0.9)
    rough = g.mixf(g.mul(mud, 0.8), rough, 0.6)
    g.finish(base, rough, 0.0, n, sheen=0.35)
    return mat


def coin(name, tarnish=1.0):
    """The coin's gold: the field dull and dark, the design and the beaded rim rubbed bright (the portrait's 'relief'
    colour attribute, kept on the detailed coin), verdigris in what's low, fine scratches."""
    mat, g = lk.new_material(name, 'coin')
    a = g.node('ShaderNodeAttribute', attribute_name='relief')
    relief = a.outputs['Fac']
    gold = g.mix(g.noise(260.0, 4.0, 0.6), (0.70, 0.50, 0.20), (0.95, 0.76, 0.36))
    field = g.mix(g.ramp(relief, 0.0, 0.55), (0.30, 0.23, 0.10), gold)
    base = g.mix(g.mul(g.ramp(relief, 0.25, 0.0), 0.5 * tarnish), field, (0.20, 0.26, 0.14))
    base = g.mix(grime(g, 0.6, 0.01), base, (0.16, 0.20, 0.10))
    # rough enough to catch the light (smooth, it only mirrored a dim sky and read black over the eye)
    rough = g.mixf(g.ramp(relief, 0.0, 0.6), 0.68, 0.48)
    n = g.bump(g.noise(220.0, 3.0, 0.6), 0.08, 0.0001)
    g.finish(base, rough, 0.9, n)
    return mat


def dead_eye(name):
    """The left eye under the coin: a dull dark ball (only its edge shows round the coin)."""
    mat, g = lk.new_material(name, 'deadeye')
    base = g.mix(g.noise(60.0, 3.0, 0.6), (0.06, 0.065, 0.06), (0.14, 0.15, 0.13))
    g.finish(base, 0.35, 0.0, None)
    return mat


def painted_light(g, base, top=(0.78, 1.10), front=(0.90, 1.04)):
    """Light painted into skin (CHAR-ART item 3, the user's OK 2026-10-22: "less flat faces"): by the detailed head's own
    normal, faces toward the sky a step lighter and those turned down (under the brow, the nose, the cheekbones, the
    jaw) deeper; planes turning away from the front a little darker. So the face keeps its cheek and brow structure in
    flat light (Medium, no Lumen), as a painted face does. top, front: the multipliers at each end."""
    x, y, z = g.xyz(g.geo().outputs['Normal'])
    base = g.mix(1.0, base, g.ramp(z, -0.75, 0.85, top[0], top[1]), 'MULTIPLY')
    return g.mix(1.0, base, g.ramp(g.mul(y, -1.0), 0.05, 0.9, front[0], front[1]), 'MULTIPLY')


def dead_skin(name, color, warm=(0.92, 0.84, 0.80)):
    """Crane's skin (CHAR-ART item 3): looter_looks.skin's dead skin (zones, mottle, veins, wrinkles, grave dirt, pores)
    with light painted in (painted_light) and his zones turned warm: bust.zones darkens the sockets, the lips and the
    nose's flush coolest in blue (bloodless); here each zone's darkening is mirrored (its red as its blue), so the
    sockets read deep red-brown, the lips and nose a dead man's warm pink-grey (warm: the lips' tone over it) instead of
    one flat green. color: the grey-green, with the daylight's yellow cut (Crane.py's SKIN)."""
    mat, g = lk.new_material(name, 'skin')
    zones = g.node('ShaderNodeAttribute', attribute_name='zones')
    sep = g.node('ShaderNodeSeparateColor')
    g.link(zones.outputs['Color'], sep.inputs['Color'])
    mirrored = g.node('ShaderNodeCombineColor')
    g.link(sep.outputs['Blue'], mirrored.inputs['Red'])
    g.link(sep.outputs['Green'], mirrored.inputs['Green'])
    g.link(sep.outputs['Red'], mirrored.inputs['Blue'])
    mott = g.noise_col(38.0, 6.0, 0.6)
    base = g.mix(0.22, color, mott, 'OVERLAY')
    base = g.mix(1.0, base, mirrored.outputs['Color'], 'MULTIPLY')
    # the lips and the nose's tip: where the zones darken least in blue and most in green (their flush), a warm tone
    flush = g.ramp(g.math('SUBTRACT', sep.outputs['Blue'], sep.outputs['Green']), 0.02, 0.10)
    base = g.mix(g.mul(flush, 0.45), base, warm, 'MULTIPLY')
    veins = g.ramp(g.noise(55.0, 8.0, 0.75, dist=0.8), 0.48, 0.52)
    base = g.mix(g.mul(veins, 0.10), base, (0.35, 0.42, 0.52))
    wrinkles = ridged(g, 60.0, stretch=(1.0, 1.0, 3.0), width=0.06, detail=3.0)
    base = g.mix(g.mul(wrinkles, 0.2), base, tuple(c * 0.6 for c in color))
    base = g.mix(grime(g, 0.5, 0.02), base, (0.22, 0.16, 0.15))
    base = g.mix(splatter(g, 0.25, top=3.0, scale=90.0), base, (0.30, 0.25, 0.21))
    base = painted_light(g, base)
    pores = g.noise(400.0, 2.0, 0.5)
    n = g.bump(pores, 0.12, 0.0005)
    n = g.bump(g.mul(wrinkles, -1.0), 0.25, 0.0008, n)
    n = g.bump(g.noise(70.0, 4.0, 0.6), 0.1, 0.002, n)
    g.finish(base, g.mixf(pores, 0.42, 0.6), 0.0, n, sss=0.25)
    return mat


def rusted_iron(name):
    """Wrought iron gone to rust: dark iron bright where links rub, orange-brown rust in the recesses and in patches,
    pitted."""
    mat, g = lk.new_material(name, 'iron')
    nz = g.noise(120.0, 6.0, 0.6)
    base = g.mix(nz, (0.07, 0.07, 0.07), (0.30, 0.29, 0.27))
    rust = g.ramp(g.noise(30.0, 5.0, 0.65, dist=0.4), 0.48, 0.62)
    base = g.mix(g.mul(rust, 0.85), base, (0.34, 0.15, 0.06))
    base = g.mix(grime(g, 0.8, 0.01), base, (0.22, 0.09, 0.04))
    shine = wear(g, 0.8)
    base = g.mix(shine, base, (0.52, 0.50, 0.47))
    metal = g.mixf(g.mul(rust, 0.9), 0.95, 0.15)
    r = g.mixf(g.mul(rust, 0.9), 0.45, 0.85)
    r = g.mixf(shine, r, 0.3)
    n = g.bump(g.noise(200.0, 4.0, 0.7), 0.15, 0.0003)
    n = g.bump(g.mul(rust, 1.0), 0.2, 0.0006, n)
    g.finish(base, r, metal, n)
    return mat
