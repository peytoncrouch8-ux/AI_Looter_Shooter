"""Odessa's looks (procedural Cycles materials baked into her texture sets), on looter_looks' node writer and its
shared touches (creases, sewing, wear, grime, dust). Her own: the wine wool tailcoat and its darker silk facings, the gold
brocade waistcoat (gilt thread in swirls and medallions over old-gold silk, the portrait's), black silk, black felt, the
hat's card (ivory board, red and black inks), her skin (alive: olive, the face painted by a 'zones' colour attribute the
model writes: lips, brows, the lid lines), and her combed-back hair read along a 'Strand' UV (u across the strands, v
along them). Colours are sRGB picks; patterns stay under ~300 cycles/m (finer aliases at the bake's density).

Also the costume's tables: LOOKS (key -> builder), the texture sets they bake into, the glows and the game's material
slots (Art/Models/Characters/Odessa.py reads them)."""
import looter_looks as lk
from looter_looks import construction, dust, grime, ridged, stains, wear

import odessa_kit as kit

# ------------------------------------------------------------------------------------------------- her own looks


def wool_coat(name, dark, light, dust_amt=0.15, nap=0.25, thread=None):
    """The tailcoat's wine wool, brushed to a soft nap: colour drifting in large patches, long soft drape creases and a
    few crumples, the nap rubbed lighter on edges and fold crests, a little dust low down, topstitched edges and sewn
    seams, a velvet sheen."""
    mat, g = lk.new_material(name, 'wool_coat')
    base = g.mix(g.ramp(g.noise(4.0, 5.0, 0.6), 0.3, 0.75), dark, light)
    base = g.mix(g.mul(g.ramp(g.noise(160.0, 3.0, 0.6), 0.45, 0.8), nap), base, tuple(min(1.0, c * 1.35) for c in light))
    drape = ridged(g, 8.0, stretch=(1.0, 1.0, 0.24), width=0.11, dist=0.3)
    crumple = ridged(g, 13.0, stretch=(1.0, 1.0, 0.6), width=0.12, detail=2.0, dist=0.3)
    creased = g.math('MAXIMUM', g.mul(drape, 0.9), g.mul(crumple, 0.35))
    base = g.mix(g.mul(creased, 0.35), base, tuple(c * 0.55 for c in dark))
    rubbed = wear(g, 0.55, 0.58)
    base = g.mix(rubbed, base, tuple(min(1.0, c * 1.45 + 0.02) for c in light))
    base = g.mix(grime(g, 0.2, 0.04), base, tuple(c * 0.4 for c in dark))
    base = g.mix(dust(g, (0.46, 0.40, 0.36), dust_amt, height=0.7, up_bias=0.5), base, (0.46, 0.40, 0.36))
    stitches, sewn = construction(g, rows=(0.006,), hem=0.02)
    base = g.mix(g.mul(stitches, 0.6), base, thread or tuple(c * 0.6 for c in dark))
    n = g.bump(g.noise(240.0, 3.0, 0.6), 0.10, 0.0003)
    n = g.bump(g.noise(18.0, 5.0, 0.6), 0.22, 0.003, n)
    n = g.bump(g.add(g.mul(drape, -1.0), g.mul(crumple, -0.4)), 0.38, 0.0022, n)
    n = g.bump(sewn, 0.5, 0.0012, n)
    rough = g.mixf(rubbed, 0.86, 0.7)
    g.finish(base, rough, 0.0, n, sheen=0.6)
    return mat


def silk(name, dark, light, rough=(0.32, 0.5), rib=0.0, dust_amt=0.0):
    """Silk (the cravat, the hat's band, the coat's facings): smooth with a soft lustre, colour shifting in broad
    patches, a few soft creases, a fine rib across if rib (a grosgrain band's), stitched edges."""
    mat, g = lk.new_material(name, 'silk')
    base = g.mix(g.ramp(g.noise(6.0, 4.0, 0.55), 0.3, 0.75), dark, light)
    creases = ridged(g, 16.0, stretch=(1.0, 1.0, 0.5), width=0.09, detail=2.0, dist=0.3)
    base = g.mix(g.mul(creases, 0.3), base, tuple(c * 0.5 for c in dark))
    base = g.mix(wear(g, 0.35, 0.6), base, tuple(min(1.0, c * 1.6 + 0.02) for c in light))
    base = g.mix(grime(g, 0.3, 0.02), base, tuple(c * 0.4 for c in dark))
    if dust_amt:
        base = g.mix(dust(g, (0.45, 0.40, 0.36), dust_amt, height=0.001, up_bias=0.9), base, (0.45, 0.40, 0.36))
    stitches, sewn = construction(g, rows=(0.004,), hem=0.0, width=0.0018, period=0.005)
    base = g.mix(g.mul(stitches, 0.4), base, tuple(min(1.0, c * 1.8) for c in light))
    n = g.bump(g.noise(120.0, 3.0, 0.5), 0.04, 0.0003)
    n = g.bump(g.mul(creases, -1.0), 0.3, 0.0012, n)
    n = g.bump(sewn, 0.3, 0.0008, n)
    if rib:
        ribs = g.wave(rib, direction='Z', profile='SIN')
        n = g.bump(ribs, 0.12, 0.0002, n)
    r = g.mixf(g.mul(creases, 0.5), rough[0], rough[1])
    g.finish(base, r, 0.0, n, sheen=0.35)
    return mat


def brocade_gold(name):
    """The waistcoat's gold brocade (the portrait's): gilt thread in swirling rings and medallions over darker
    old-gold silk, the thread metallic and brighter, the silk's own soft creases across the stomach, its edges stitched."""
    mat, g = lk.new_material(name, 'brocade_gold')
    # vines: thin lines where a distorted ring pattern crosses its middle (thresholded as bands it read as stripes)
    rings = g.wave(32.0, kind='RINGS', distortion=7.0, detail=2.0)
    swirl = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', rings, 0.5)), 0.06, 0.02)
    vor = g.voronoi(55.0, out='Distance')
    ring = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', vor, 0.26)), 0.045, 0.0)
    dot = g.ramp(vor, 0.11, 0.07)
    thread = g.math('MAXIMUM', g.mul(swirl, 0.7), g.math('MAXIMUM', ring, dot))
    silk_col = g.mix(g.ramp(g.noise(5.0, 4.0, 0.55), 0.3, 0.75), (0.30, 0.19, 0.06), (0.42, 0.28, 0.09))
    base = g.mix(thread, silk_col, (0.93, 0.76, 0.38))
    creases = ridged(g, 9.0, stretch=(1.0, 1.0, 2.5), width=0.08)
    base = g.mix(g.mul(creases, 0.3), base, (0.16, 0.10, 0.03))
    base = g.mix(grime(g, 0.4, 0.03), base, (0.12, 0.08, 0.03))
    rubbed = wear(g, 0.4)
    base = g.mix(rubbed, base, (0.98, 0.86, 0.52))
    stitches, sewn = construction(g, rows=(0.005,), hem=0.0)
    base = g.mix(g.mul(stitches, 0.5), base, (0.95, 0.80, 0.45))
    n = g.bump(thread, 0.3, 0.0006)
    n = g.bump(g.mul(creases, -1.0), 0.14, 0.0008, n)
    n = g.bump(sewn, 0.4, 0.001, n)
    metal = g.mul(thread, 0.85)
    r = g.mixf(thread, 0.6, 0.32)
    g.finish(base, r, metal, n, sheen=0.2)
    return mat


def felt_black(name):
    """Her black fur felt: kept brushed, but a gambler's hat on the road: a faint grey bloom of dust on the brim's top
    and in the crown's top edge, edges rubbed a shade lighter, fibres and fuzz, a few dents."""
    mat, g = lk.new_material(name, 'felt_black')
    dark, light = (0.022, 0.020, 0.024), (0.065, 0.060, 0.068)
    base = g.mix(g.ramp(g.noise(7.0, 6.0, 0.6), 0.3, 0.75), dark, light)
    base = g.mix(grime(g, 0.4, 0.04), base, (0.01, 0.01, 0.012))
    base = g.mix(dust(g, (0.30, 0.28, 0.27), 0.25, height=0.001, up_bias=0.9), base, (0.30, 0.28, 0.27))
    base = g.mix(wear(g, 0.6), base, (0.16, 0.15, 0.16))
    fuzz = g.noise(280.0, 4.0, 0.75)
    dents = ridged(g, 14.0, width=0.12, detail=3.0)
    base = g.mix(g.mul(g.ramp(fuzz, 0.4, 0.8), 0.2), base, (0.12, 0.11, 0.12))
    n = g.bump(fuzz, 0.12, 0.0004)
    n = g.bump(g.noise(25.0, 5.0, 0.6), 0.12, 0.003, n)
    n = g.bump(g.mul(dents, -1.0), 0.1, 0.002, n)
    g.finish(base, g.mixf(fuzz, 0.8, 0.92), 0.0, n, sheen=0.7)
    return mat


def card_board(name, color=(0.90, 0.86, 0.76)):
    """The playing card's board: ivory, a fine paper grain, its edges and corners rubbed grey from shuffling."""
    mat, g = lk.new_material(name, 'card')
    base = g.mix(g.ramp(g.noise(30.0, 4.0, 0.6), 0.3, 0.8), tuple(c * 0.92 for c in color), color)
    base = g.mix(wear(g, 0.6, 0.55), base, (0.55, 0.52, 0.47))
    base = g.mix(grime(g, 0.3, 0.01), base, (0.45, 0.42, 0.38))
    n = g.bump(g.noise(260.0, 3.0, 0.6), 0.05, 0.0002)
    g.finish(base, 0.5, 0.0, n)
    return mat


def ink(name, color):
    mat, g = lk.new_material(name, 'ink')
    base = g.mix(g.mul(g.ramp(g.noise(80.0, 3.0, 0.6), 0.5, 0.9), 0.15), color, (0.9, 0.86, 0.78))
    g.finish(base, 0.42, 0.0, None)
    return mat


def skin_alive(name, color):
    """Her skin: olive, alive (an ember's revenant who still passes for living), the colour times painted zones (the
    'zones' attribute Odessa.py writes: the eyes' shadow, the lips, the brows and the lines of her lashes), mottled, a
    faint cool blush of veins at the temples, pores, fine wrinkles, a little powder-dull sheen."""
    mat, g = lk.new_material(name, 'skin')
    zones = g.node('ShaderNodeAttribute', attribute_name='zones')
    mott = g.noise_col(38.0, 6.0, 0.6)
    base = g.mix(0.16, color, mott, 'OVERLAY')
    base = g.mix(1.0, base, zones.outputs['Color'], 'MULTIPLY')
    veins = g.ramp(g.noise(55.0, 8.0, 0.75, dist=0.8), 0.48, 0.52)
    base = g.mix(g.mul(veins, 0.05), base, (0.42, 0.46, 0.55))
    wrinkles = ridged(g, 60.0, stretch=(1.0, 1.0, 3.0), width=0.05, detail=3.0)
    base = g.mix(g.mul(wrinkles, 0.12), base, tuple(c * 0.7 for c in color))
    base = g.mix(grime(g, 0.1, 0.02), base, (0.36, 0.26, 0.22))
    pores = g.noise(400.0, 2.0, 0.5)
    n = g.bump(pores, 0.10, 0.0005)
    n = g.bump(g.mul(wrinkles, -1.0), 0.18, 0.0008, n)
    n = g.bump(g.noise(70.0, 4.0, 0.6), 0.08, 0.002, n)
    g.finish(base, g.mixf(pores, 0.46, 0.62), 0.0, n, sss=0.25)
    return mat


def hair_combed(name, dark=(0.035, 0.025, 0.024), light=(0.12, 0.085, 0.07), across=260.0, along=7.0):
    """Dark hair combed back and pinned: strands along the 'Strand' UV's v (u across them, in metres or so), separated
    into fine combed lines with darker partings between clumps, a lighter sheen on the strands, a little lift and
    stray here and there."""
    mat, g = lk.new_material(name, 'hair_combed')
    u, v, _ = g.xyz(g.uv('Strand'))
    st = g.node('ShaderNodeCombineXYZ')
    g.link(g.mul(u, across), st.inputs[0])
    g.link(g.mul(v, along), st.inputs[1])
    strands = g.noise(1.0, 3.0, 0.6, vec=st.outputs[0], dist=0.25)
    clumps = g.node('ShaderNodeCombineXYZ')
    g.link(g.mul(u, across * 0.12), clumps.inputs[0])
    g.link(g.mul(v, along * 0.5), clumps.inputs[1])
    clump = g.noise(1.0, 2.0, 0.5, vec=clumps.outputs[0])
    lines = g.ramp(strands, 0.32, 0.68)
    base = g.mix(lines, dark, light)
    parting = g.ramp(clump, 0.38, 0.30)
    base = g.mix(g.mul(parting, 0.7), base, tuple(c * 0.35 for c in dark))
    base = g.mix(g.mul(g.noise(10.0, 4.0, 0.6), 0.2), base, (0.16, 0.12, 0.10))
    n = g.bump(strands, 0.55, 0.0005)
    n = g.bump(g.mul(parting, -1.0), 0.35, 0.001, n)
    g.finish(base, g.mixf(lines, 0.52, 0.36), 0.0, n, sheen=0.35)
    return mat


# ------------------------------------------------------------------------------------------------- the costume

COAT = ((0.115, 0.012, 0.045), (0.27, 0.035, 0.10))           # wine wool, dark and light (the portrait's Wine velvet)
FACING = ((0.15, 0.018, 0.05), (0.30, 0.045, 0.11))           # its silk facings, a deeper wine: lapels, collar, cuffs

LOOKS = {
    'Skin': lambda n: skin_alive(n, (0.76, 0.66, 0.58)),
    'Hair': lambda n: hair_combed(n),
    'EyeGlow': lambda n: lk.glow(n, (0.35, 0.85, 1.0), 6.0),
    'Felt': lambda n: felt_black(n),
    'HatBand': lambda n: silk(n, (0.24, 0.025, 0.06), (0.40, 0.06, 0.11), rough=(0.38, 0.5), rib=180.0),
    'SweatBand': lambda n: lk.leather(n, (0.14, 0.08, 0.05), grain=280.0, dust_amt=0.1, dust_height=0.001),
    'Card': lambda n: card_board(n),
    'InkRed': lambda n: ink(n, (0.66, 0.05, 0.07)),
    'InkBlack': lambda n: ink(n, (0.02, 0.02, 0.025)),
    'Coat': lambda n: wool_coat(n, *COAT),
    'CoatTrim': lambda n: silk(n, *FACING, rough=(0.42, 0.6), dust_amt=0.15),
    'Brocade': lambda n: brocade_gold(n),
    'Shirt': lambda n: lk.linen(n, (0.90, 0.88, 0.84)),
    'Silk': lambda n: silk(n, (0.018, 0.016, 0.02), (0.06, 0.055, 0.065)),
    'Trousers': lambda n: lk.wool(n, (0.16, 0.15, 0.175), dust_amt=0.35),
    # leather grain under the bake's limit (280/m, Ellis's lesson: finer read as suede); her riding boots black and
    # kept polished, the toe cap stitched 5 cm behind the toe, the welt 5 mm over the sole
    'Boots': lambda n: lk.leather(n, (0.055, 0.05, 0.055), grain=280.0, dust_amt=0.2, dust_height=0.12,
                                  scuff=(0.24, 0.22, 0.22), rough=(0.24, 0.55), boot=kit.BOOT_CAP),
    'Soles': lambda n: lk.stacked(n, (0.10, 0.07, 0.05)),
    'Gloves': lambda n: lk.leather(n, (0.045, 0.04, 0.045), grain=280.0, dust_amt=0.08, dust_height=0.001,
                                   scuff=(0.20, 0.19, 0.20), rough=(0.35, 0.6)),
    'Belt': lambda n: lk.leather(n, (0.07, 0.05, 0.045), grain=280.0, dust_amt=0.15, dust_height=0.001),
    'Case': lambda n: lk.leather(n, (0.30, 0.08, 0.07), grain=280.0, dust_amt=0.1, dust_height=0.001,
                                 scuff=(0.50, 0.30, 0.26)),
    'Gold': lambda n: lk.metal(n, (0.62, 0.45, 0.16), (0.96, 0.79, 0.42), rough=(0.16, 0.4), tarnish=(0.25, 0.17, 0.06)),
}
kit.LOOKS.update(LOOKS)

# The texture sets the looks bake into (Art/Textures/<Set>/, 2048), and the glowing looks the game draws as Glow.
TEXTURE_SETS = {
    'OdessaSkin': ('Skin', 'Hair'),
    'OdessaCoat': ('Coat', 'CoatTrim'),
    'OdessaGarb': ('Brocade', 'Shirt', 'Silk', 'Trousers', 'Felt', 'HatBand'),
    'OdessaGear': ('Boots', 'Soles', 'Gloves', 'Belt', 'Case', 'Gold', 'SweatBand', 'Card', 'InkRed', 'InkBlack'),
}
GLOWS = {'EyeGlow': (0x5AD8FF, 4.0)}

# The game's material slots: each kind of surface on the character master made for it, several slots sharing a set
# (as Ellis's). Slot: (master, its looks, the master's parameters).
GAME_MATERIALS = {
    'OdessaSkin': ('CharacterSkin', ('Skin',), {}),
    # combed hair on a shell (not cards): default lit, a little sheen (the Hair shading needs the cards' strand UVs)
    'OdessaHair': ('Character', ('Hair',), {'Specular': 0.45}),
    'OdessaCoat': ('CharacterCloth', ('Coat', 'CoatTrim'), {'FuzzStrength': 0.35, 'FuzzLift': 0.12}),
    'OdessaGarbCloth': ('CharacterCloth', ('Shirt', 'Trousers', 'Felt', 'HatBand'), {'FuzzStrength': 0.3, 'FuzzLift': 0.12}),
    # the gilt thread is metal: default lit, not the cloth shading's sheen
    'OdessaGarbSilk': ('Character', ('Brocade', 'Silk'), {'Specular': 0.5}),
    'OdessaGear': ('Character', ('Boots', 'Soles', 'Gloves', 'Belt', 'Case', 'Gold', 'SweatBand', 'Card', 'InkRed',
                                 'InkBlack'), {}),
}
