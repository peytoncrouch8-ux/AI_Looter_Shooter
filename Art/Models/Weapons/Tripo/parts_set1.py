"""AR set 1 (the defaults: Standard body, Carbine barrel, Cap muzzle, 30 magazine, Iron sight, Pad stock) from the user's
Tripo models: how Bullpup.py fits each (the spec format is in Bullpup.py, "Tripo parts"). Not a model: Tools\\models.ps1
skips parts_*.py.

Boxes are the concept's size table (Art/Concepts/Weapons/AR/README.md), gun space in cm: u along the gun from the butt,
v up from the bore, w across. The body's rail, nut, well and loop land on the attach points (rail top 6.15, nut face
51.9); the sight's SOCKET_Aim is measured: the rear aperture's plane at the front post's cyan dot (the sight-picture
rule), 2026-10-27: aperture centre v 8.70, dot v 8.90, ring 1.86 across.
"""

# The Tripo files' axes: the body was exported muzzle -X, left side -Y; the rest muzzle -Y, left +X. Up is +Z.
BODY_AXES = dict(forward='-X', left='-Y')
AXES = dict(forward='-Y', left='+X')


def body_accent(isl):
    """The body's modeled accent inlay, a thin strip along the left side: the rarity glow."""
    (u0, v0, w0), (u1, v1, w1) = isl.lo, isl.hi
    return u0 >= 20.5 and u1 <= 37.5 and v1 - v0 < 0.7 and min(abs(w0), abs(w1)) > 2.2


def body_finish(ctx):
    """The accent strip on the right (Tripo modeled the left one only) and ember glow inserts over the painted nose vents
    (the shell there is flat, at w 2.54)."""
    lg = ctx.lg
    ctx.add(lg.box(21.0, 36.2, -3.1, -2.75, 0.2, w=-2.6, bevel=0.0, mat=ctx.glow('GunAccentGlow')))
    for side in (-1.0, 1.0):
        for u in ctx.at['VENTS']:
            ctx.add(lg.slab([(u, -1.6), (u + 0.9, -1.6), (u + 2.2, 1.2), (u + 1.3, 1.2)], 0.04, w=side * 2.57,
                            bevel=0.0, mat=ctx.glow('GunEmberGlow'), round=0.3))


def stock_prepare(ctx):
    """Slides the pad so its backing plate's face sits on the body's butt plate (u -0.3); the locating boss in front of
    it sinks into the plate."""
    face = ctx.tripo.probe_u(ctx.obj, [(-7.6, 0.0), (0.8, 0.0), (-3.3, 1.8), (-3.3, -1.8)])   # off the boss
    if face is not None:
        from mathutils import Matrix
        shift = -0.3 - face
        ctx.obj.data.transform(Matrix.Translation(ctx.lg.at(shift, 0.0, 0.0)))
        ctx.log(f'backing plate face at u {face:.2f}, moved {shift:+.2f} cm onto the butt plate')


def iron_prepare(ctx):
    """SOCKET_Aim in the rear aperture's plane at the front post's dot (the dot on the crosshair); logs the aperture's
    centre against it."""
    tripo, obj, v0 = ctx.tripo, ctx.obj, ctx.at['SIGHT_AT'][1]
    cyan = tripo.faces_where(obj, ctx.maps['BC'], tripo.is_cyan) or tripo.faces_where(obj, ctx.maps['BC'], tripo.is_cyan,
                                                                                      corners=False)
    dot = tripo.centroid(obj, [i for i in cyan if tripo.gun_uvw(obj.data.polygons[i].center)[0] > 24.0])
    leaf = [p for p in (tripo.gun_uvw(v.co) for v in obj.data.vertices) if p[0] < 24.0 and p[1] > v0 + 2.2]
    ur = sum(p[0] for p in leaf) / len(leaf)
    ring = tripo.hole_centre(obj, ur, (v0 + 0.8, v0 + 4.2), (-1.6, 1.6))
    ctx.log(f'rear leaf u {ur:.2f}; aperture {ring and tuple(round(x, 2) for x in ring)} (v, w, width); front dot '
            f'{dot and tuple(round(x, 2) for x in dot)} (u, v, w); {len(cyan)} cyan faces')
    aim = (dot[1], dot[2]) if dot else ((ring[0], ring[1]) if ring else (v0 + 2.6, 0.0))
    ctx.sockets['Aim'] = ((ur, aim[0]), aim[1])


SAND = dict(slot='GunPolymerSand', test='sand', tint=0xe6d8b6)

PARTS = {
    'Body_Standard': dict(glb='BullpupBody_Standard.glb', box=((-0.3, 51.9), (-15.25, 6.15), 5.7), axes=BODY_AXES,
                          island_glows={'GunAccentGlow': body_accent}, paint=SAND, finish=body_finish),
    'Barrel_Carbine': dict(glb='BullpupBarrel_Carbine.glb', box=((51.9, 63.9), (-1.65, 1.65), 3.3), axes=AXES,
                           glows={'GunEmberGlow': 'ember'}),
    'Muzzle_Cap': dict(glb='BullpupMuzzle_Cap.glb', box=((100.0, 101.8), (-0.92, 0.92), 1.84), axes=AXES),
    'Magazine_30': dict(glb='BullpupMagazine_30.glb', box=((10.4, 20.1), (-24.8, -5.6), 2.8), axes=AXES),
    'Sight_Iron': dict(glb='BullpupSight_Iron.glb', box=((15.6, 32.0), (6.15, 10.1), 2.9), axes=AXES,
                       glows={'GunCyanGlow': 'cyan'}, paint=SAND, prepare=iron_prepare),
    'Stock_Pad': dict(glb='BullpupStock_Pad.glb', box=((-1.9, -0.3), (-8.8, 2.0), 5.2), axes=AXES,
                      prepare=stock_prepare),
}
