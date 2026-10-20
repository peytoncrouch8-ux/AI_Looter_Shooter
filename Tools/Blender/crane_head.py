"""Hollis Crane's head for his game model (Art/Models/Characters/Crane.py): the visible skin (head, neck, bare hands
and forearms), the eyes (the right burning cyan in its socket, the left a dead ball under the ferryman's coin), the
coin (the portrait's build_coin, its relief baked onto a low disc), the slouch hat (the portrait's build_hat seated on
the head by Tools/Blender/ellis_hat.fit), and the game skin's reduction by region.
"""
import math

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector

import crane_common as cc
import looter_garments as gm
import looter_model as lm
from crane_common import CP, OPT, log, slot


# ------------------------------------------------------------------------------------------------------------ the skin

SLEEVE_INSIDE = 0.45     # the forearm's skin runs this far up from the wrist (share of the forearm) inside the sleeve


def forearm_share(fig, s, P):
    """How far down the forearm on side s points are (0 the elbow, 1 the wrist; past it, more)."""
    e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
    length = np.linalg.norm(w - e)
    return ((P - e) @ ((w - e) / length)) / length


def thicken_hand(fig, verts, used, girth):
    """Hand points pushed away from the bones under them by girth: each point's offset from each of its skinning
    bones, square to that bone, blended by its skin weights (Ellis.py's, for the gloves)."""
    W = fig.Wv[used]
    out = np.zeros_like(verts)
    for j, bone in enumerate(fig.names):
        w = W[:, j]
        if not (w > 1e-4).any():
            continue
        h, t = fig.sk[bone][0], fig.sk[bone][1]
        d = gm.unit(t - h)
        q = verts - h
        perp = q - np.outer(q @ d, d)
        out += w[:, None] * perp
    return verts + (girth - 1.0) * out


def build_skin(fig):
    """The visible skin: the head and the neck down under the shirt's collar band, the bare hands (filled out a
    little, HAND_GIRTH) and the forearms up inside the sleeves; the scalp under the hair takes the hair's colour."""
    z = fig.wv[:, 2]
    neck_band = (z > fig.neck[2] - 0.045) & (fig.w_arm < 0.12) & (np.abs(fig.wv[:, 0]) < 0.10)
    keep = (fig.w_head > 0.3) | neck_band | (fig.w_hand > 0.3)
    for s in 'lr':
        fore = fig.Wv[:, fig.col[f'lowerarm_{s}']] + fig.Wv[:, fig.col[f'hand_{s}']]
        keep |= (fore > 0.5) & (forearm_share(fig, s, fig.wv) > 1.0 - SLEEVE_INSIDE)
    ob = fig.b.mesh('Crane', keep=lambda i, f: bool(keep[f].all()))
    ob.data.materials.append(slot('Skin'))
    ob.data.materials.append(slot('Scalp'))
    idx = np.array([d.value for d in ob.data.attributes['mh_index'].data])
    co = gm._object_coords(ob)
    hand = fig.w_hand[idx] > 1e-3
    co[hand] = thicken_hand(fig, co[hand], idx[hand], cc.HAND_GIRTH)
    ob.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
    ob.data.update()
    attr = ob.data.color_attributes.new('zones', 'FLOAT_COLOR', 'POINT')
    attr.data.foreach_set('color', fig.zones[idx].astype(np.float32).ravel())
    # the scalp under the hat and the long hair: down the back to the nape (head-local, decimetres over the eyes)
    hf = fig.hf
    loc = cc.mh.apply(np.linalg.inv(fig.g_head), fig.b.h['verts'])
    cz = (hf['verts'][:, 2].max() + hf['verts'][:, 2].min()) * 0.5 - 0.1
    az = np.degrees(np.abs(np.arctan2(loc[:, 0], loc[:, 2] - cz)))
    line = np.interp(az, [0, 46, 70, 100, 125, 180], [0.44, 0.40, 0.18, -0.10, -0.95, -1.15]) + hf['eye_y']
    scalp = (loc[:, 1] > line) & (fig.w_head > 0.5)
    for p in ob.data.polygons:
        if scalp[idx[list(p.vertices)]].all():
            p.material_index = 1
    log(f'skin: {len(ob.data.polygons)} faces')
    return ob


def build_eyes(fig):
    """The eyeballs (looter_human's MakeHuman helpers): the right set 3.5 mm deeper and smaller, burning (the portrait's
    socket lamp is the glow itself in the game), the left dead under the coin. Returns (left, right)."""
    out = {}
    fwd = np.array(fig.axes[0])
    for side in ('l', 'r'):
        c, r = fig.balls[side]
        c = fig.head_point(c)
        rad = r * 0.1 * fig.b.scale
        if side == 'r':
            c = c - fwd * 0.0035
            rad *= 0.9
        bm = bmesh.new()
        lm.uv_sphere(bm, 20, 12, rad)      # fixed face order: the bake must fit the next run
        me = bpy.data.meshes.new(f'Eye_{side}')
        bm.to_mesh(me)
        bm.free()
        me.shade_smooth()
        eo = bpy.data.objects.new(f'Eye_{side}', me)
        bpy.context.scene.collection.objects.link(eo)
        eo.location = Vector(c)
        eo.data.materials.append(slot('EyeGlow' if side == 'r' else 'DeadEye'))
        out[side] = eo
    return out['l'], out['r']


COIN_R, COIN_T = 0.0172, 0.0024      # the portrait's coin (build_coin's R and T)


def build_coin(fig, skin, eye_l):
    """The ferryman's coin over the left eye (the portrait's build_coin: a bevelled disc resting on brow and cheekbone,
    its worn face a robed figure poling a boat under a lantern in a ring of beads; no real money's), the detailed bake
    source; and its game disc (coin_low)."""
    bvh = gm.bvh_objects([skin])
    coin = CP.build_coin(eye_l, fig.axes, bvh, 1.0)
    coin.data.materials.clear()
    coin.data.materials.append(slot('Coin'))
    fig.coin_matrix = coin.matrix_world.copy()
    cc.bake_transform(coin)
    # the portrait's coin faces inward (its bust renders both sides, so it never showed): turned out, or the bake casts
    # its relief inside out and the coin reads as a black hole over the eye
    bm = bmesh.new()
    bm.from_mesh(coin.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(coin.data)
    bm.free()
    coin.data.update()
    return coin


def coin_low(fig, n=28):
    """The coin's game disc: a rim and two faces, its relief from the bake's normal map."""
    R, T = COIN_R, COIN_T
    ang = np.linspace(0.0, 2 * math.pi, n, endpoint=False)
    rings = [(R * 0.86, 0.00055), (R, 0.0003), (R, -T), (R * 0.9, -T)]
    verts = [(0.0, 0.0, 0.0004)]
    for r, zz in rings:
        verts += [(r * math.cos(a), r * math.sin(a), zz) for a in ang]
    verts.append((0.0, 0.0, -T))
    faces = [[0, 1 + (j + 1) % n, 1 + j] for j in range(n)]
    for k in range(len(rings) - 1):
        o = 1 + k * n
        faces += [[o + j, o + (j + 1) % n, o + n + (j + 1) % n, o + n + j] for j in range(n)]
    last, back = 1 + (len(rings) - 1) * n, len(verts) - 1
    faces += [[back, last + j, last + (j + 1) % n] for j in range(n)]
    me = bpy.data.meshes.new('Coin_low')
    me.from_pydata(verts, [], faces)
    me.update()
    ob = bpy.data.objects.new('Coin_low', me)
    bpy.context.scene.collection.objects.link(ob)
    ob.matrix_world = fig.coin_matrix
    bpy.context.view_layer.update()
    cc.bake_transform(ob)
    # faces out, in place (built ring by ring they faced in: the bake cast the relief from inside the disc, its normal
    # map came out inverted and its occlusion black, and the coin read as a black hole over the eye)
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(me)
    bm.free()
    me.update()
    fwd = np.array(fig.axes[0])
    front = [np.array(p.normal) @ fwd for p in me.polygons if np.array(p.normal) @ fwd > 0.5]
    log(f'coin: {len(front)} of {len(me.polygons)} faces look out of the face (the front fan and its ring)')
    ob.data.materials.append(slot('Coin'))
    me.shade_smooth()
    return ob


# ------------------------------------------------------------------------------------------- the game skin reduced

EYE_KEEP = 0.018         # round each eye the lids keep their detail (the right's glowing slit, the coin's rim)
SKIN_LOW = {'eyes': 1.0, 'skull': 0.22, 'face': 0.32, 'neck': 0.18, 'hands': 0.30, 'fore': 0.12}


def decimate_skin(fig, low):
    """The game's skin (low: a copy of the detailed one) reduced region by region, each to its SKIN_LOW share (one
    Decimate per region with the rest spared: from one ratio over all of it the skull fell to a dozen triangles,
    Ellis round 6): the lids kept whole, the skull under the crown and the long hair, the face (his is bare: nose,
    cheekbones, mouth and jaw under the mustache), the neck, the hands, the forearms inside the sleeves."""
    import ellis_hat
    s = ellis_hat.seat(fig)

    def regions(co):
        d = np.minimum(np.linalg.norm(co - fig.eyes[0], axis=1), np.linalg.norm(co - fig.eyes[1], axis=1))
        eyes = d < EYE_KEEP
        head = co[:, 2] > fig.neck[2] + 0.05
        skull = ((co - s['O']) @ s['u'] > 0.012) & ~eyes & head
        hands = np.zeros(len(co), dtype=bool)
        fore = np.zeros(len(co), dtype=bool)
        for side in 'lr':
            t = forearm_share(fig, side, co)
            near = np.linalg.norm(np.cross(co - fig.sk[f'lowerarm_{side}'][0],
                                           gm.unit(fig.sk[f'lowerarm_{side}'][1] - fig.sk[f'lowerarm_{side}'][0])),
                                  axis=1) < 0.09
            hands |= near & (t > 0.97)
            fore |= near & (t <= 0.97) & (t > 0.2)
        face = head & ~eyes & ~skull & ~hands & ~fore
        neck = ~(eyes | skull | face | hands | fore)
        return {'eyes': eyes, 'skull': skull, 'face': face, 'neck': neck, 'hands': hands, 'fore': fore}

    before = cc.tris_of(low)
    for name in ('fore', 'skull', 'neck', 'face', 'hands'):
        share = SKIN_LOW[name]
        co = gm._object_coords(low)
        reg = regions(co)[name]
        tris = cc.tris_of(low)
        mine = sum(len(p.vertices) - 2 for p in low.data.polygons if reg[list(p.vertices)].all())
        if not mine:
            continue
        m = low.modifiers.new('decimate', 'DECIMATE')
        m.ratio = (tris - mine * (1.0 - share)) / tris
        m.use_symmetry = True
        m.symmetry_axis = 'X'
        g = low.vertex_groups.new(name='keep')
        g.add([int(i) for i in np.nonzero(~reg)[0]], 1.0, 'REPLACE')
        m.vertex_group = 'keep'
        m.invert_vertex_group = True
        m.vertex_group_factor = 1.0
        gm.apply_modifiers(low, keep=())
        low.vertex_groups.remove(low.vertex_groups['keep'])
    return before, cc.tris_of(low)


# ----------------------------------------------------------------------------------------------------- the hat

BRIM_REACH = 0.92        # the brim's reach past the crown, of the portrait's (a little less: the game tips it to aim)
BROW_UP = 0.032          # the band's foot over the brows at the front (Ellis's cattleman sits 2 cm over them; the
                         # portrait's slouch sat higher, and seated as low its pushed-in top came down onto the skull)
BRIM_SAG = (0.006, 0.022, 0.012)   # the soft brim's extra sag at its edge (m): all round, more on his left (the side
                                   # with the bite; the portrait's brim dips there), more at the back


def seat(fig):
    """ellis_hat.seat with the slouch hat's own height over the brows (BROW_UP), cached where ellis_hat looks for it
    (fig.hat_seat), so its fit, pivot and band ring all use it."""
    import ellis_hat
    if getattr(fig, 'hat_seat', None):
        return fig.hat_seat
    keep = ellis_hat.BROW_UP
    ellis_hat.BROW_UP = BROW_UP
    try:
        return ellis_hat.seat(fig)
    finally:
        ellis_hat.BROW_UP = keep


def sag_brim(fig, brim):
    """The brim's soft sag (BRIM_SAG) after its fit: each ring of 96 down by its share of the way out, squared."""
    s = fig.hat_seat
    co = gm._object_coords(brim).reshape(-1, 96, 3)
    t = (np.arange(co.shape[0]) / (co.shape[0] - 1))[:, None]
    rel = co - s['O']
    ang = np.arctan2(rel @ s['x'], rel @ s['f'])
    left = np.maximum(np.sin(ang), 0.0) ** 1.5
    back = np.maximum(-np.cos(ang), 0.0) ** 2
    dz = (BRIM_SAG[0] + BRIM_SAG[1] * left + BRIM_SAG[2] * back) * t ** 1.8
    co[..., 2] -= dz
    brim.data.vertices.foreach_set('co', co.reshape(-1).astype(np.float32))
    brim.data.update()


def build_hat(fig):
    """The portrait's slouch hat (Art/Portraits/Crane.py build_hat "slouch": a low round crown pushed in on top, off
    centre as if sat on; a wide soft brim drooping at the sides and back, battered, a bite torn out of it on the left),
    moved into the figure's space and seated on the head (ellis_hat.fit: the band's foot on a plane over the brows and
    the occipital bump, the crown fitted round the head with the pressed hair's room); the crown's felt and the brim get
    their thickness after the bake (THICK). A sweatband lines the crown (HatBandInner)."""
    import ellis_hat
    parts = CP.build_hat(fig.b.h, fig.hf, OPT)
    for ob in parts:
        for m in list(ob.modifiers):
            ob.modifiers.remove(m)
        fig.to_portrait(ob)
        cc.bake_transform(ob)
    brim = next(o for o in parts if o.name.startswith('HatBrim'))
    co = gm._object_coords(brim)
    assert len(co) % 96 == 0, "the portrait's brim: rings of 96 points expected"
    co = co.reshape(-1, 96, 3)
    # the bite: the portrait deleted the brim's faces there (a staircase of quads); here every face is kept and the
    # spokes through the bite are shortened instead (its outline the portrait's: 0.55 + 2.5 |phi - 1.25| of the
    # brim's width, his left front), a little ragged
    nseg, rings = 96, co.shape[0]
    phi = 2 * math.pi * np.arange(nseg) / nseg
    bite = np.clip(0.55 + 2.5 * np.abs(phi - 1.25) + 0.05 * np.sin(np.arange(nseg) * 2.7), 0.0, 1.0)
    co = co[:1] + (co - co[:1]) * (BRIM_REACH * bite)[None, :, None]
    faces = [[i * nseg + j, (i + 1) * nseg + j, (i + 1) * nseg + (j + 1) % nseg, i * nseg + (j + 1) % nseg]
             for i in range(rings - 1) for j in range(nseg)]
    me = bpy.data.meshes.new('HatBrim')
    me.from_pydata([tuple(p) for p in co.reshape(-1, 3)], [], faces)
    me.update()
    me.shade_smooth()
    old = brim.data
    brim.data = me
    bpy.data.meshes.remove(old)
    seat(fig)
    parts = ellis_hat.fit(fig, parts)
    sag_brim(fig, brim)
    cc.HAT['pivot'] = ellis_hat.pivot(fig, parts)
    band = next(o for o in parts if o.name.startswith('HatBand'))
    cc.HAT['band_z'] = float(gm._object_coords(band)[:, 2].min())
    for ob in parts:
        ob.data.materials.clear()
        ob.data.materials.append(slot('HatBand' if ob.name.startswith('HatBand') else 'Felt'))
        cc.uv_cylinder(ob)
    log(f'hat: seated (crown raised {fig.hat_fit["raise_by"] * 100:.0f}%, its least air over the skull '
        f'{fig.hat_fit["air"] * 1000:.1f} mm), the band at {cc.HAT["band_z"]:.3f} m')
    return parts
