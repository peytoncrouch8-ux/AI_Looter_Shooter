"""AR set 3 (Windfall, scrap-built: rust, galvanized tin, sea-teal and sky-blue paint, tape, brass wire, a little ember)
from the user's Tripo models: Salvager body, Shrouded barrel, Prong muzzle, Coupled magazine, Reflex sight, Skeleton
stock. How Bullpup.py fits each (the format is in Bullpup.py, "Tripo parts"); each spec is a function fn(bp, m) that
builds the whole part, with helpers in Tools/Blender/looter_tripo_set3.py. Not a model: Tools\\models.ps1 skips
parts_*.py.

Boxes are the concept's bounds (Art/Concepts/Weapons/AR/render_set3.js PARTS), gun space in cm: u along the gun from the
butt, v up from the bore, w to the gun's left. The Tripo files face muzzle -Y, left +X, up +Z. Joints are measured after
the fit and moved onto set 1's interfaces (barrel-nut face (51.9, 0) r 1.25, butt plate back u -0.3, well top v -5.6 over
u 10.4-17.6 and 2.8 wide, rail top 6.15), so every set 3 part fits every body and the reverse.

Colour: the painted panels are paintable slots on one paint texture (looter_tripo_set3.paint_sets): sea teal takes the
shell paint (GunPolymerSand_<set>), sky blue the fittings paint (GunPolymerGrey_<set>), so a maker's paints recolour them
and the two sides stay mismatched. Glow: the body's ember cell and the shroud's ember glimpses are GunEmberGlow, the
sight's emitter lens GunCyanGlow (both take the gun's element colour), the dot GunReticle; the body's rarity line is
GunAccentGlow on the trigger housing.
"""
import looter_tripo_set3 as t3

AXES = dict(forward='-Y', left='+X')
BARREL_U = 51.9   # the barrel nut's face (Bullpup.BARREL_AT)
TEAL = 0x4f8f8a  # Windfall's sea teal (Docs/Work/2026-10-22-Next/MAKERS.md)
SKY = 0x7d9fb5    # Windfall's sky blue


def load(bp, name, u, v, w):
    """Loads and fits Bullpup<name>.glb onto the box u, v, w ((lo, hi) each; w may be off the bore: moved after)."""
    tripo, obj, maps, mat = bp.tripo_load(name, (u, v, w[1] - w[0]), **AXES)
    t3.shift(obj, dw=(w[0] + w[1]) * 0.5)
    return tripo, obj, maps, mat


def log(bp, m, text):
    bp.lt._log(f'Tripo {m}: {text}')


def fmt(x):
    return 'None' if x is None else (f'{x:.2f}' if isinstance(x, float) else '(' + ', '.join(f'{float(a):.2f}' for a in x) + ')')


def ember(tripo, obj, maps):
    return tripo.faces_where(obj, maps['BC'], tripo.is_ember) or tripo.faces_where(obj, maps['BC'], tripo.is_ember,
                                                                                  corners=False)


def body(bp, m):
    """The Salvager: butt plate -0.3 to the barrel nut's face 51.9, grip foot -16.2 to the rail top 6.15; the ember cell
    on the right sets w -3.6, the charging knob on the left 3.4. The ember cell's window glows; a rarity line runs along
    each side of the trigger housing (no accent was modeled)."""
    lg = bp.lg
    tripo, obj, maps, mat = load(bp, 'Body_Salvager', (-0.3, 51.9), (-16.2, 6.15), (-3.6, 3.4))
    nut = tripo.probe_u(obj, [(0.6, 0.0), (-0.6, 0.0), (0.0, 0.6), (0.0, -0.6)], reach=80.0)
    rail = t3.extent(obj, u=(15.0, 31.0), w=(-0.5, 0.5))
    well = t3.extent(obj, u=(10.0, 18.0), v=(-7.0, -4.8))
    butt = t3.probe_u_back(obj, [(-7.0, 0.0), (1.0, 0.0), (-3.3, 1.5), (-3.3, -1.5)])
    behind = t3.extent(obj, u=(-1.0, (butt or 0.0) - 0.02))
    log(bp, m, f'nut face u {fmt(nut)}; rail top {fmt(rail and rail[1][1])}; well lo/hi {fmt(well and well[0])} '
               f'{fmt(well and well[1])}; butt back u {fmt(butt)}; behind it {fmt(behind and behind[0])} '
               f'{fmt(behind and behind[1])}')
    if butt is not None and nut is not None and behind is None:
        # The butt plate's back on u -0.3 (where every stock seats) and the nut's face on 51.9: a stretch under 1%.
        k = t3.remap_u(obj, butt, nut, -0.3, BARREL_U)
        log(bp, m, f'butt plate and nut face put on -0.3 and {BARREL_U} (u x {k:.4f})')
    # Tripo's rail is a coarse saw-tooth: the scripted Picatinny rail (the sights' seat, top 6.15) takes its place on
    # the risers.
    top = t3.faces_in(obj, u=(13.2, 32.8), v=(5.12, 7.0), w=(-1.4, 1.4))
    log(bp, m, f'{len(top)} rail faces replaced by the scripted rail')
    cell = t3.faces_in(obj, u=(30.0, 42.5), w=(-4.0, -1.2))
    hot = set(cell)
    glows = {bp.EMBER: [f for f in tripo.faces_where(obj, maps['BC'], tripo.is_ember, share=0.4) if f in hot]}
    log(bp, m, f'{len(glows[bp.EMBER])} ember faces in the cell')
    # Glow that reads at a distance (the element's colour on element guns): the cell's window as one bright pane on
    # its outer face, and the recoil spring's guide rod glowing inside the open coil (seen from both sides).
    box = t3.extent(obj, u=(32.0, 40.5), w=(-4.0, -1.6), v=(-2.0, 0.8))
    if box:
        (_, cv0, cw0), (_, cv1, _) = box
        cv = (cv0 + cv1) * 0.5
        w = t3.probe_w(obj, 36.0, cv, -1.0)
        w = w if w is not None else cw0
        bp.g.add(m, lg.box(33.3, 38.9, cv - 0.42, cv + 0.42, 0.05, w=w - 0.01, bevel=0.0, mat=bp.EMBER))
        log(bp, m, f'cell window pane at v {cv:.2f} w {w:.2f}')
    coil = t3.extent(obj, u=(34.0, 40.0), v=(-1.6, 1.6), w=(-1.0, 1.0))
    log(bp, m, f"coil region {fmt(coil and coil[0])} {fmt(coil and coil[1])}")
    if coil:
        (_, sv0, sw0), (_, sv1, sw1) = coil
        sv, sw = (sv0 + sv1) * 0.5, (sw0 + sw1) * 0.5
        bp.g.add(m, lg.tube(30.8, 45.8, 0.24, v=sv, w=sw, segments=12, mat=bp.EMBER))
        log(bp, m, f'spring guide glow at v {sv:.2f} w {sw:.2f}')
    # The rarity line: a strip on each side of the trigger housing, on its measured surface.
    for side in (-1.0, 1.0):
        ws = [t3.probe_w(obj, u, -3.0, side) for u in (23.5, 25.0, 26.5, 28.0, 29.0)]
        log(bp, m, f'trigger housing side {side:+.0f}: w {[fmt(w) for w in ws]}')
        ws = sorted(w for w in ws if w is not None and abs(w) < 3.0)
        if ws:
            w = ws[len(ws) // 2]
            bp.g.add(m, lg.box(23.0, 29.4, -3.15, -2.9, 0.06, w=w + side * 0.03, bevel=0.0, mat=bp.ACCENT))
            log(bp, m, f'accent at w {w:.2f}')
    t3.add(bp, m, obj, maps, mat, glows, drop=top,
           paints=[('GunPolymerSand', t3.is_teal, TEAL, None), ('GunPolymerGrey', t3.is_sky, SKY, None)])
    bp.g.add(m, lg.rail(14, 32, 5.2, mat=lg.BLACK))
    bp.body_sockets(m)


def barrel(bp, m):
    """The Shrouded barrel: 16.5 from the barrel nut; the shroud (r 1.8) is centred on the bore and sized from its
    middle (the tape's loose end and the front-sight tab make the bounds lopsided); the rear adapter sits on the nut."""
    u0 = bp.BARREL_AT[0]
    tripo, obj, maps, mat = load(bp, 'Barrel_Shrouded', (u0, u0 + 16.5), (-1.9, 2.75), (-2.35, 1.9))
    # The shroud's section between the hose clamp and the tape (u 3.3-5.9 from the nut).
    sec = t3.extent(obj, u=(u0 + 3.3, u0 + 5.9))
    if sec:
        (_, v0, w0), (_, v1, w1) = sec
        centre = ((v0 + v1) * 0.5, (w0 + w1) * 0.5)
        t3.rescale(obj, centre, 3.6 / (v1 - v0), 3.6 / (w1 - w0))
        t3.shift(obj, dv=-centre[0], dw=-centre[1])
        log(bp, m, f'shroud section v {v0:.2f}..{v1:.2f} w {w0:.2f}..{w1:.2f}: centred, sized to r 1.8')
    rear = t3.extent(obj, u=(u0, u0 + 0.5))
    tip = t3.extent(obj, u=(u0 + 16.0, u0 + 16.6))
    back = t3.probe_u_back(obj, [(0.0, 0.6), (0.0, -0.6), (0.6, 0.0), (-0.6, 0.0)])
    log(bp, m, f'rear face u {fmt(back)}, rear r v {fmt(rear and rear[0][1])}..{fmt(rear and rear[1][1])}; tip r '
               f'{fmt(tip and tip[1][1])}')
    glows = {bp.EMBER: tripo.faces_where(obj, maps["BC"], tripo.is_ember, share=0.4)}   # the middle row's glimpses
    log(bp, m, f'{len(glows[bp.EMBER])} ember faces')
    # Heat bands that read at a distance: glowing rings round the shroud between the clamp and the tape, and one ahead
    # of the brass wire (the element's colour on element guns).
    for a in (3.1, 4.3, 5.5, 13.55):
        bp.g.add(m, bp.lg.tube(u0 + a, u0 + a + 0.38, 1.93, segments=28, mat=bp.EMBER))
    t3.add(bp, m, obj, maps, mat, glows)
    bp.g.socket(m, 'Muzzle', (u0 + 16.5, 0.0))


def muzzle(bp, m):
    """The Prong flash hider: 6.2 long from its back face (on the barrel's tip); centred and sized on its hex nut
    (flats v +-1.1, corners w +-1.27)."""
    u0 = bp.MUZZLE_AT[0]
    tripo, obj, maps, mat = load(bp, 'Muzzle_Prong', (u0, u0 + 6.2), (-1.15, 1.45), (-1.3, 1.3))
    nut = t3.extent(obj, u=(u0 + 0.15, u0 + 1.1))
    if nut:
        (_, v0, w0), (_, v1, w1) = nut
        centre = ((v0 + v1) * 0.5, (w0 + w1) * 0.5)
        t3.rescale(obj, centre, 2.2 / (v1 - v0), 2.54 / (w1 - w0))
        t3.shift(obj, dv=-centre[0], dw=-centre[1])
        log(bp, m, f'nut v {v0:.2f}..{v1:.2f} w {w0:.2f}..{w1:.2f}: centred on the bore')
    # The bore sleeve between the prongs glows hot at its mouth (GunEmberGlow: the element's colour on element guns).
    p = t3.points(obj)
    near = p[(p[:, 0] > u0 + 1.4) & (p[:, 0] < u0 + 3.6) & (p[:, 1] ** 2 + p[:, 2] ** 2 < 0.45 ** 2)]   # inside the prongs
    if len(near):
        su1 = float(near[:, 0].max())
        log(bp, m, f'bore sleeve mouth at u {su1:.2f} ({len(near)} vertices inside r 0.45)')
        bp.g.add(m, bp.lg.tube(su1 - 0.03, su1 + 0.01, 0.34, segments=16, mat=bp.EMBER))
        # The whole sleeve glows between the prongs, so the heat reads from the side too.
        bp.g.add(m, bp.lg.tube(u0 + 1.3, su1 + 0.005, 0.52, segments=16, mat=bp.EMBER))
    t3.add(bp, m, obj, maps, mat)
    bp.g.socket(m, 'Muzzle', (u0 + 6.2, 0.0))


def magazine(bp, m):
    """The Coupled pair: the seated rusty box's top on the well (v -5.6, u 10.4-17.6, w -1.4..1.4), the galvanized box
    taped upside down on the gun's left; bounds u 10.3-19.6, v -25.9..-5.6, w -1.65..4.75. The seated box is measured
    and moved onto the well's middle. Its teal band takes the shell paint."""
    tripo, obj, maps, mat = load(bp, 'Magazine_Coupled', (10.3, 19.6), (-25.9, -5.6), (-1.65, 4.75))
    top = t3.extent(obj, v=(-6.4, -5.6))
    if top:
        (u0, _, w0), (u1, _, w1) = top
        dw = -(w0 + w1) * 0.5
        t3.shift(obj, dw=dw)
        log(bp, m, f'seated box top u {u0:.2f}..{u1:.2f} w {w0:.2f}..{w1:.2f} ({w1 - w0:.2f} wide): moved {dw:+.2f} '
                   f'across')
    t3.add(bp, m, obj, maps, mat, paints=[('GunPolymerSand', t3.is_teal, TEAL, None)])


def sight(bp, m):
    """The Reflex: base on the rail top (6.15), u 18.9-26.4, w -2.45..2.3, 5.6 tall. Tripo modeled the glass as an
    opaque dark pane with the dot painted on, which would blind the sight: its faces go, a clear pane (GunLens) takes
    their place on the same lean, the dot is a GunReticle pin on it, and SOCKET_Aim sits on the dot. The emitter's
    front lens glows GunCyanGlow; the teal cell takes the shell paint, the sky-blue gusset the fittings paint."""
    lg = bp.lg
    v0 = bp.SIGHT_AT[1]
    tripo, obj, maps, mat = load(bp, 'Sight_Reflex', (18.9, 26.4), (v0, v0 + 5.6), (-2.45, 2.3))
    base = t3.extent(obj, u=(24.2, 26.0), v=(v0, v0 + 0.5))   # ahead of the wing nut and bolt
    if base:
        dw = -(base[0][2] + base[1][2]) * 0.5
        t3.shift(obj, dw=dw)
        log(bp, m, f'clamp w {base[0][2]:.2f}..{base[1][2]:.2f}: moved {dw:+.2f} across')
    # The pane: every face a ray from the eye meets in the frame's window, ahead of the emitter; its lean from those
    # facing along the gun.
    glass = sorted(set(t3.faces_seen(obj, 22.8, 26.6, (v0 + 1.2, v0 + 2.3), (-0.9, 0.9)))      # over the emitter
                   | set(t3.faces_seen(obj, 18.0, 26.6, (v0 + 2.3, v0 + 5.02), (-0.9, 0.9))))  # above it, from the eye
    facing = set(t3.faces_in(obj, normal=(1.0, 0.0, 0.0), min_dot=0.5))
    pts = [tripo.gun_uvw(obj.data.polygons[i].center) for i in glass if i in facing]
    if len(pts) >= 6:
        import numpy as np
        p = np.array(pts)
        b, a = np.polyfit(p[:, 1], p[:, 0], 1)           # u = a + b v: the pane's lean
        win = (p[:, 1].min(), p[:, 1].max(), p[:, 2].min(), p[:, 2].max())
        log(bp, m, f'{len(glass)} glass faces; u = {a:.2f} + {b:.3f} v; window v {win[0]:.2f}..{win[1]:.2f} w '
                   f'{win[2]:.2f}..{win[3]:.2f}')
    else:
        b, a = -0.24, 25.0 + 0.24 * (v0 + 3.1)
        win = (v0 + 1.15, v0 + 5.05, -0.98, 0.98)
        log(bp, m, f'only {len(glass)} glass faces found: the concept pane used')
    pane_u = lambda v: a + b * v
    lv0, lv1 = win[0] - 0.12, win[1] + 0.12
    hw = max(abs(win[2]), abs(win[3])) + 0.12
    bp.g.add(m, lg.slab([(pane_u(lv0) - 0.03, lv0), (pane_u(lv0) + 0.03, lv0), (pane_u(lv1) + 0.03, lv1),
                         (pane_u(lv1) - 0.03, lv1)], 2.0 * hw, bevel=0.0, mat=lg.LENS))
    dot_v = v0 + 3.2
    dot_u = pane_u(dot_v) - 0.06
    bp.g.add(m, lg.pin(dot_u, dot_v, -0.06, 0.06, 0.06, mat=bp.RETICLE))
    # The emitter's lens: its front face, measured, under the window.
    tube = t3.extent(obj, u=(18.8, 22.8), v=(v0 + 1.0, v0 + 2.1), w=(-0.6, 0.6))
    if tube:
        (eu0, ev0, ew0), (eu1, ev1, ew1) = tube
        ev, ew, er = (ev0 + ev1) * 0.5, (ew0 + ew1) * 0.5, min(ev1 - ev0, ew1 - ew0) * 0.5
        log(bp, m, f'emitter u {eu0:.2f}..{eu1:.2f}, centre v {ev:.2f} w {ew:.2f}, r {er:.2f}')
        bp.g.add(m, lg.tube(eu1, eu1 + 0.04, er * 0.85, v=ev, w=ew, segments=16, mat=bp.CYAN))
        # A glowing band round the emitter behind its lens, seen from the sides and from the eye.
        bp.g.add(m, lg.tube(eu1 - 0.75, eu1 - 0.3, er + 0.03, v=ev, w=ew, segments=16, mat=bp.CYAN))
    right = lambda f: tripo.gun_uvw(obj.data.polygons[f].center)[2] < -1.0   # the cell and the sky gusset
    t3.add(bp, m, obj, maps, mat, drop=glass,
           paints=[('GunPolymerSand', t3.is_cell, TEAL, right), ('GunPolymerGrey', t3.is_sky, SKY, right)])
    bp.g.socket(m, 'Aim', (dot_u, dot_v))


def stock(bp, m):
    """The Skeleton stock: rust mount plate, two tubes, brace and tyre pad, -11.6..0.3 long, -9.2..2.45 tall, 5.1
    across; slid so the mount plate's face sits on the body's butt plate (u -0.3), as set 1's pad."""
    tripo, obj, maps, mat = load(bp, 'Stock_Skeleton', (-11.6, 0.3), (-9.2, 2.45), (-2.55, 2.55))
    face = tripo.probe_u(obj, [(-7.6, 0.0), (0.8, 0.0), (-3.3, 1.6), (-3.3, -1.6)])
    if face is not None:
        t3.shift(obj, du=-0.3 - face)
        log(bp, m, f'mount face at u {face:.2f}, moved {-0.3 - face:+.2f} onto the butt plate')
    t3.add(bp, m, obj, maps, mat)


PARTS = {
    'Body_Salvager': body,
    'Barrel_Shrouded': barrel,
    'Muzzle_Prong': muzzle,
    'Magazine_Coupled': magazine,
    'Sight_Reflex': sight,
    'Stock_Skeleton': stock,
}
