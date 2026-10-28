"""AR set 2 (Roundhouse: Armored body, Heavy barrel, BoxBrake muzzle, Drum magazine, Holo sight, Monopod stock) from the
user's Tripo models: how Bullpup.py fits each (the spec format is in Bullpup.py, "Tripo parts"). Not a model:
Tools\\models.ps1 skips parts_*.py.

Boxes are the concept's bounds (render.js, the rh* parts; Art/Concepts/Weapons/AR/README.md "Set 2"), gun space in cm:
u along the gun from the butt, v up from the bore, w across (+ the gun's left). Set 1's interfaces: barrel-nut face
(51.9, 0), butt plate at u 0, well u 10.4-17.6 at v -5.6, rail top v 6.15 over u 14-32, grip hand (22.5, -10).
Every Tripo file here is muzzle -Y, left side +X, up +Z.

Roundhouse's shell is oxblood (maker paint 0x7d2f24): it becomes the paintable GunPolymerSand_<part> slot, by face
(Tripo merged each part into one piece) and as luminance (oxblood's faint green and blue would turn to noise under a
light paint), so another maker's or the roll's shell paint recolours it. Ember slots and vents are GunEmberGlow, the
holo's dot GunReticle (both take the element's colour); the holo's glass is see-through GunLens.
"""

AXES = dict(forward='-Y', left='+X')


# Colour tests take (r, g, b) sRGB 0..1; written with & so they also run on whole texture arrays (looter_tripo.paint_set).

def is_oxblood(rgb):
    """Roundhouse's oxblood paint: a dull dark red, darker than the ember glow."""
    r, g, b = rgb
    return (r > 0.22) & (r < 0.75) & (g < 0.5 * r) & (b < 0.5 * r)


def is_red_dot(rgb):
    """The holo's painted red dot."""
    r, g, b = rgb
    return (r > 0.8) & (g < 0.35) & (b < 0.35)


def is_glass(rgb):
    """The holo's painted blue glass."""
    r, g, b = rgb
    return (b > 0.3) & (b > 1.3 * r) & (b > 1.05 * g)


OXBLOOD = dict(slot='GunPolymerSand', test=is_oxblood, tint=0x7d2f24, by='faces', mode='luma')


def armored_prepare(ctx):
    """Where the receiver's sides are under the vents (for the accent strips)."""
    ctx.sides = {}
    for side in (-1.0, 1.0):
        hit = ctx.tripo.probe(ctx.obj, (33.0, -5.05, side * 10.0), (0.0, 0.0, -side))
        ctx.sides[side] = hit[2] if hit else side * 3.0
    ctx.log(f'receiver sides at w {ctx.sides[-1.0]:.2f}, {ctx.sides[1.0]:.2f}')
    for u, v in ((20.0, -1.0), (33.0, -1.0), (46.0, -1.0), (30.0, 4.7), (51.0, 0.0)):   # its middle against the bore's
        a = ctx.tripo.probe(ctx.obj, (u, v, -10.0), (0.0, 0.0, 1.0))
        b = ctx.tripo.probe(ctx.obj, (u, v, 10.0), (0.0, 0.0, -1.0))
        if a and b:
            ctx.log(f'at u {u} v {v}: w {a[2]:.2f} to {b[2]:.2f}, middle {(a[2] + b[2]) * 0.5:+.2f}')


def armored_finish(ctx):
    """The rarity accent (GunAccentGlow), which every body carries: a strip along each side of the receiver's foot,
    between the grip and the handguard, under the ember vents (Tripo has none)."""
    for side, w in ctx.sides.items():
        ctx.add(ctx.lg.box(27.6, 40.4, -5.2, -4.95, 0.08, w=w + side * 0.03, bevel=0.0, mat=ctx.glow('GunAccentGlow')))


def holo_prepare(ctx):
    """SOCKET_Aim on the dot in the front glass (ADS puts the eye 22 cm behind it, looking along the gun): the painted
    red dot's centre if Tripo painted one, else the front pane's."""
    tripo, obj, bc = ctx.tripo, ctx.obj, ctx.maps['BC']
    # The glass goes see-through (GunLens): faces mostly painted blue, cleaned by their neighbours.
    glass = tripo.majority(obj, tripo.faces_where(obj, bc, is_glass, share=0.4), passes=1)
    ctx.glow_faces['GunLens'] = glass
    front = [i for i in glass if tripo.gun_uvw(obj.data.polygons[i].center)[0] > 23.0]
    pane = tripo.centroid(obj, front) if front else None
    dots = tripo.faces_where(obj, bc, is_red_dot, corners=False)
    dot = tripo.centroid(obj, dots) if dots else None
    v = dot[1] if dot else (pane[1] if pane else 8.95)
    u = pane[0] if pane else 26.9
    ctx.sockets['Aim'] = ((u - 0.15, v), 0.0)
    ctx.aim = (u - 0.15, v)
    ctx.log(f'{len(glass)} glass faces, front pane at {pane and tuple(round(x, 2) for x in pane)}, painted dot '
            f'{dot and tuple(round(x, 2) for x in dot)} ({len(dots)} faces); Aim ({u - 0.15:.2f}, {v:.2f})')


def holo_finish(ctx):
    """A small reticle dot just behind the front glass, on the line of sight (the painted one goes see-through with the
    glass when it shares its faces)."""
    u, v = ctx.aim
    ctx.add(ctx.lg.pin(u, v, -0.11, 0.11, 0.11, mat=ctx.glow('GunReticle'), segments=12))


PARTS = {
    'Body_Armored': dict(glb='BullpupBody_Armored.glb', box=((-0.3, 51.9), (-16.4, 6.15), (-3.64, 3.71)), axes=AXES,
                         glows={'GunEmberGlow': 'ember'}, paint=OXBLOOD, prepare=armored_prepare,
                         finish=armored_finish),
    'Barrel_Heavy': dict(glb='BullpupBarrel_Heavy.glb', box=((51.9, 69.9), (-1.9, 1.9), 3.8), axes=AXES,
                         glows={'GunEmberGlow': 'ember'}, paint=OXBLOOD, sockets={'Muzzle': ((69.9, 0.0), 0.0)}),
    'Muzzle_BoxBrake': dict(glb='BullpupMuzzle_BoxBrake.glb', box=((100.0, 109.9), (-3.2, 3.2), 5.2), axes=AXES,
                            glows={'GunEmberGlow': 'ember'}, paint=OXBLOOD, sockets={'Muzzle': ((109.9, 0.0), 0.0)}),
    'Magazine_Drum': dict(glb='BullpupMagazine_Drum.glb', box=((7.7, 20.3), (-15.6, -5.6), (-7.0, 6.3)), axes=AXES,
                          paint=OXBLOOD),
    'Sight_Holo': dict(glb='BullpupSight_Holo.glb', box=((17.2, 28.8), (6.15, 13.15), 3.9), axes=AXES,
                       glows={'GunReticle': is_red_dot}, paint=OXBLOOD, prepare=holo_prepare,
                       finish=holo_finish),
    'Stock_Monopod': dict(glb='BullpupStock_Monopod.glb', box=((-15.0, 0.3), (-18.6, 3.0), 5.2), axes=AXES,
                          paint=OXBLOOD),
}
