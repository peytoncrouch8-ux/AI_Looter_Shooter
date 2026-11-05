"""The zone tables of looter_tripo_zones (one per model; the barge's walls are looter_tripo_zones.BARGE): each Tripo
model's paint (prototype colours, sRGB hex, measured as OKLab clusters of its own atlas; the comments give each
cluster's mean and share) -> the look that repaints it. See looter_tripo_zones' docstring for the keys; 'up' limits a
zone to texels whose normal's z lies in (lo, hi) (a deck's planks against the struts of the same grey)."""

# Planks along the model's length (Blender Y): on the sides (normal +-X) and on top (decks); WoodPlanks runs its boards
# along U, 20 cm wide.
ALONG_Y = [((0.0, 1.0, 0.0), (0.0, 0.0, -1.0), (0.37, 0.11)), ((1.0, 0.0, 0.0), (0.0, 0.0, -1.0), (0.71, 0.53)),
           ((0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.19, 0.83))]

# The Lamplighter (Art/Tripo/Lamplighter, 31 m; her atlas 50% used, 32 px/m at 2048 (p10-p90 27-39); measured
# 2026-11-04): a teal-painted planked hull with gold strakes, red-brown rails, grey-green deck planks and struts, brown
# wood fittings (cabin, wheel), a canvas bag in a rope net, brass and black iron fittings.
LAMPLIGHTER = dict(
    zones=[
        dict(name='HullTeal', set='WoodPlanks', colour='tripo', hue=0.0, pattern=1.0, detail=0.5, saturate=1.35,
             axes=ALONG_Y,
             protos=(0x16282a, 0x283d3e)),                       # dark teal 16282a 11.7%, teal 283d3e 9.1%
        dict(name='Deck', set='WoodPlanks', colour='tripo', hue=0.3, pattern=0.75, detail=0.5, axes=ALONG_Y,
             up=(0.6, 1.01), protos=(0x565b50,)),                # grey-green 565b50 6.4% (the deck's up faces)
        dict(name='IronGrey', set='MetalWorn', colour='tripo', detail=0.3, up=(-1.01, 0.6),
             protos=(0x565b50,)),                                # the same grey on the struts and fittings
        dict(name='Wood', set='WoodPlanks', colour='tripo', hue=0.5, detail=0.5, axes=ALONG_Y,
             protos=(0x3d200f, 0x594024)),                       # dark brown 3d200f 7.0%, brown 594024 4.1%
        dict(name='RailOxide', set='PaintWorn', tint=0x8c3e2c, chips=True, detail=0.4,
             protos=(0x79320f,)),                                # red-brown 79320f 4.2%: the shared PaintOxide
        # The bag and its net: the net's darker tans are rope, the bag's shaded underside (a68e6f) canvas still (the
        # net's ropes lie on the bag's skin, so a thickness rule cannot tell them apart).
        dict(name='Canvas', set='Canvas', colour='tripo', detail=0.7,
             protos=(0xaa9d88, 0xbcb19e, 0xa68e6f)),             # the bag aa9d88 18.3%, its shade a68e6f 13.4%
        dict(name='Rope', set='Canvas', colour='tripo', pattern=0.3, detail=0.8,
             protos=(0x947a57, 0x7c613d)),                       # the net 947a57 9.2%, 7c613d 5.4%
        dict(name='Brass', set='MetalWorn', tint=0xc49c56, detail=0.3,
             protos=(0xc09b57,)),                                # gold c09b57 3.4% (metal 0.64): the shared BrassWorn
        dict(name='Iron', set='MetalWorn', tint=0x2e2c2a, detail=0.3,
             protos=(0x0e0e0d,)),                                # black 0e0e0d 7.9%: the shared IronBlack
    ],
    keep=(),
    params=dict(work=2048, mip_bias=0.5, value_keep=0.5, detail_px=1.5, match_value=True))   # 2048: her map's size


# The Unpaid (parts_unpaid_paint: Tripo's paint cast onto SK_Unpaid's and SM_UnpaidHat's Paint UVs, 2048, 32% of it
# painted; measured 2026-11-04): blue-grey dead skin, a cream shirt, a dark brown waistcoat and tie, a brown tattered robe,
# a brown hat. The colour picks the garment, the mesh says where each may be (unpaid() writes each face's M_Ghost tint
# zone, 0 skin and shroud .. 3 tie and band, and whether it is the hat): Tripo's browns run into each other. Our character
# looks (looter_looks) in Tripo's own colours, with Tripo's painted detail kept (detail: buttons' shading, seams,
# folds); M_Ghost reads only the colour (no ORM, no normal), multiplying its own weave, folds, top light and cavities.
BODY, HAT = ('unpaid_part', 0, -0.5, 0.5), ('unpaid_part', 0, 0.5, 1.5)
# Off unless LOOTER_REBAKE names it (=Unpaid or =1): at 554 px/m Tripo's paint is not soft, and the rebake (2026-11-04
# previews, Saved/ArtPreviews/Rebake2/unpaid_*) softened the face's wrinkles, lost the eyes' ember glint and the lapels'
# crisp lines at 1 m; it reads the same at 3 m. Kept for when the looks or M_Ghost's paint normals make it worth it.
UNPAID = dict(
    on=False,
    zones=[
        dict(name='Skin', look=('skin', {'color': 'tripo'}), pattern=0.6, detail=0.9, faces=[('unpaid_zone', 0, -0.5, 0.5), BODY],
             protos=(0x4e5052, 0x656464, 0x3e4244, 0x7a7a78)),              # blue-grey 4e5052 7.5%, 656464 6.2%
        dict(name='Shirt', look=('linen', {'color': 'tripo'}), pattern=0.5, detail=0.85, faces=[('unpaid_zone', 0, 0.5, 2.5), BODY],
             protos=(0x9b9384, 0x847c70, 0xb4ab99, 0x6f685d)),              # cream 9b9384 4.9%, 847c70 5.4%
        dict(name='Vest', look=('wool', {'color': 'tripo'}), pattern=0.8, faces=[('unpaid_zone', 0, 0.5, 3.5), BODY],
             protos=(0x2a231d, 0x1a1613, 0x342c24, 0x3c342b)),              # dark brown 2a231d 9.8%, the tie
        dict(name='Robe', look=('oilskin', {'dark': 'tripo*0.7', 'light': 'tripo*1.2', 'hem': 0.3}), pattern=0.8,
             faces=[('unpaid_zone', 0, -0.5, 0.5), BODY],
             protos=(0x342c24, 0x3c342b, 0x443d34, 0x544a40, 0x2a231d, 0x1a1613)),   # browns 342c24 20.9%, 3c342b 19%
        dict(name='Hat', look=('felt', {'dark': 'tripo*0.6', 'light': 'tripo'}), pattern=0.8, faces=[HAT],
             protos=(0x544a40, 0x443d34, 0x3c342b, 0x342c24, 0x2a231d, 0x1a1613)),
    ],
    keep=(0x8a2a1a, 0xb83a22, 0xd05a30),    # the eyes' red glow; past keep_dist (gold buttons, a sleeve's grey shadow)
                                            # Tripo's paint stays too
    params=dict(work=2048, keep_dist=0.14, value_keep=0.4, detail=0.7, detail_px=3.0, look_samples=4, match_value=True))

MODELS = {'Lamplighter': LAMPLIGHTER, 'Unpaid': UNPAID}


def unpaid(target, colour, body, hat, solid):
    """parts_unpaid_paint's hook: Tripo's cast paint (colour, linear, the target's Paint UVs) repainted per garment.
    target: the joined copy of body and hat (their faces in that order) the paint was cast on; solid: per body face, the
    teeth, the mouth's pocket and the coal (they keep the solid corner). Returns the new colour."""
    import numpy as np
    import looter_tripo_zones as tz
    me = target.data
    zone = []
    for obj in (body, hat):
        col = obj.data.color_attributes['Col']
        n = len(obj.data.loops)
        rgba = np.empty(n * 4, np.float32)
        col.data.foreach_get('color_srgb' if col.data_type == 'BYTE_COLOR' else 'color', rgba)
        first = np.array([p.loop_start for p in obj.data.polygons])
        zone.append(np.clip(np.round(rgba.reshape(-1, 4)[first, 0] * 3.0), 0, 3))
    nb = len(body.data.polygons)
    for name, vals in (('unpaid_zone', np.concatenate(zone)), ('unpaid_part', np.r_[np.zeros(nb), np.ones(len(zone[1]))])):
        a = me.attributes.get(name) or me.attributes.new(name, 'FLOAT', 'FACE')
        a.data.foreach_set('value', np.asarray(vals, np.float32))
    faces = np.ones(len(me.polygons), bool)
    faces[:nb][np.asarray(solid, bool)] = False
    out = tz.rebake(target, 'Unpaid', {'colour': colour}, faces=faces)
    for name in ('unpaid_zone', 'unpaid_part'):
        me.attributes.remove(me.attributes[name])
    return out
