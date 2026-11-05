"""The dredge barge's house and casing walls rebaked in our texture sets (Docs/Work/2026-11-04-Art/CONTRACT.md, "Round 4",
the REBAKE lane; the user's pick "Blurry house walls"): looter_tripo_zones' rebake with the barge's zone table
(looter_tripo_zones.BARGE: where Tripo painted cream, oxide red, slate, black and rust; the shared PaintCream,
PaintOxide, PaintBlack, PaintWorn in Tripo's slate, MetalRust in its rusts; the glass kept). The stacks, the tanks, the
rails, the fittings, the hull and the decks keep Tripo's paint.

paint() is SPECS['DredgeBarge']['paint'] (looter_tripo_wardens.build runs it after the rework and any repack, before
write_maps). walls() is the face rule (looter_tripo_barge.slots takes the same faces out of Super for the Walls slot):
faces of the rework's Super slot (barge_slot) inside the house and casing's box (WALL_X, |y| < WALL_Y inboard of the
walks, over WALL_Z), not on the stacks (STACK_R round their squeezed axes, over STACK_Z) or the tanks (the rework's
TANKS box), and not thin (a ray in from the face meets the far side within THIN: rails, pipes, horns, brackets). It works
on face rules and texels, never on face counts, so it re-runs unchanged on a reworked mesh. LOOTER_BARGE_PAINT=0 (or
LOOTER_REBAKE=0) skips it.
The walls' own set: when maps['Walls'] is there (REPACK's TripoDredgeBargeWalls: {'BC', 'RM', 'N'} on the wall faces'
own UV square, the TripoDredgeBarge_Walls slot) the step paints that set, and its colour map's alpha mask gets the
paint-chip weight (MI_TripoDredgeBarge_Walls' DetailMaskAlpha: 1 on the painted zones, 0 on the rusts and the glass);
write_maps adds it to the PNG.
The limit (measured 2026-11-04): in the shared atlas the walls get 14 px/m at 2048 (a texel ~7 cm); in a walls-only 2K
set 22 px/m (REPACK's pack of round 4). Close-up crispness is the material's detail layer's: the previews emulate
today's Super detail (MetalWorn 0.4) and the Walls instance's (PaintWorn 0.65 at 1.6 m, masked by the chip alpha).
Previews (Saved/ArtPreviews/BargeRebake/: before/after at the same cameras, the zone map):
    $env:LOOTER_TRIPO_ONLY='DredgeBarge'; Tools\\artrun.ps1 -Script Tools\\Blender\\looter_tripo_barge_paint.py -Preview
(builds the barge twice: LOOTER_BARGE_PAINT=0 for the before views, then with the paint).
"""
import os

import bpy
import numpy as np
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo_barge as barge
import looter_tripo_zones as tz
import looter_tripo_zones_look as zl
import looter_tripo_zones_preview as zp

# --- The walls (cm, the barge's frame) ---
WALL_X = (-3420.0, 2290.0)             # the casing's aft end .. the house's fore face (Tripo's 2005 + the midship jumbo)
WALL_Y = 840.0                         # inboard of the walks: the house's sides |y| 550-700, the casing's 650-800, the
                                       # bulwark's inner face from 875
WALL_Z = 2258.0                        # over the walk floors (2255)
STACK_R, STACK_Z = 290.0, 2700.0       # round the stacks' squeezed axes (their foot band at 2900 starts below)
TANK_MARGIN, TANK_DROP = 80.0, 130.0   # the rework's TANKS box widened in x, from its blend's start down
THIN = 0.35                            # m: a face whose inside is thinner is a rail, pipe or fitting
ZONES = tz.BARGE['zones']
KEEP = len(ZONES)
LAST = tz.LAST


def log(message):
    print(f'LOOTER: DredgeBarge paint: {message}', flush=True)


def walls(obj):
    """The faces painted: a bool per face (None when the mesh has no barge_slot: not the reworked barge)."""
    me = obj.data
    attr = me.attributes.get('barge_slot')
    if attr is None:
        return None
    n = len(me.polygons)
    slot = np.empty(n, np.int64)
    attr.data.foreach_get('value', slot)
    c, nn = np.empty(n * 3), np.empty(n * 3)
    me.polygons.foreach_get('center', c)
    me.polygons.foreach_get('normal', nn)
    c, nn = c.reshape(-1, 3), nn.reshape(-1, 3)
    x, y, z = -c[:, 1] * 100.0, -c[:, 0] * 100.0, c[:, 2] * 100.0
    stacks = [(sx, sy * barge.SQUEEZE_CASING) for sx, sy in barge.STACKS]
    r_stack = np.min([np.hypot(x - sx, y - sy) for sx, sy in stacks], axis=0)
    t0, t1 = barge.final_x(barge.TANKS[0]) - TANK_MARGIN, barge.final_x(barge.TANKS[1]) + TANK_MARGIN
    tank = (x > t0) & (x < t1) & (np.abs(y) < barge.TANKS[2]) & (z > barge.TANKS[3] - TANK_DROP)
    stack = (r_stack < STACK_R) & (z > STACK_Z)
    box = (x > WALL_X[0]) & (x < WALL_X[1]) & (np.abs(y) < WALL_Y) & (z > WALL_Z)
    sup = slot == barge.SLOTS.index('Super')
    cand = sup & box & ~stack & ~tank
    # Thin pieces: a ray in from the face's middle meets the far side soon (Tripo's skin is closed).
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    tree = BVHTree.FromPolygons([tuple(v) for v in co.reshape(-1, 3)], [tuple(p.vertices) for p in me.polygons])
    thin = np.zeros(n, bool)
    for f in np.nonzero(cand)[0]:
        d = -nn[f]
        thin[f] = tree.ray_cast(tuple(c[f] + d * 0.003), tuple(d), THIN)[0] is not None
    wall = cand & ~thin
    log(f'walls: {wall.sum()} faces of {sup.sum()} Super ({(sup & stack).sum()} on the stacks, {(sup & tank).sum()} on '
        f'the tanks, {(cand & thin).sum()} thin left out)')
    return wall


def paint(obj, report, maps):
    """SPECS['DredgeBarge']['paint']: the walls' texels rebaked in our sets (into the walls' own set when there is one)."""
    if os.environ.get('LOOTER_BARGE_PAINT', '1') == '0':
        log('skipped (LOOTER_BARGE_PAINT=0)')
        return
    import looter_tripo_wardens as tw
    wall = walls(obj)
    if wall is None or not wall.any():
        log('no walls found (no barge_slot attribute): nothing painted')
        return
    own = maps.get('Walls')                  # the walls' own set (REPACK), on their own UV square
    sub = {}
    tz.rebake(obj, 'DredgeBarge', own or maps, faces=wall, own=bool(own), grade=tw.GRADE['ship'], report=sub,
              alpha='chips' if own else None)
    report['paint'] = sub.get('rebake')


# --- Previews ---

ZONE_COLOURS = np.array([[0.95, 0.88, 0.62], [0.9, 0.2, 0.12], [0.3, 0.45, 0.75], [0.75, 0.42, 0.12], [0.4, 0.2, 0.08],
                         [0.03, 0.03, 0.03], [0.1, 0.9, 0.3]], np.float32)   # ZONES in order, then Tripo kept (green)
# Cameras (the barge's frame, cm; a direction from the target to the camera; metres away; lens mm): the -Y walk beside the
# house (its side at y -640) and the casing (-570..-720), at eye height, up close and along them (the base swell); the
# house's fore face from the well deck.
CLOSE = {'house_1m': ((900.0, -650.0, 2560.0), (0.0, -1.0, 0.05), 1.0, 24.0),
         'house_3m': ((900.0, -650.0, 2600.0), (0.1, -1.0, 0.05), 2.3, 24.0),
         'house_walk': ((900.0, -660.0, 2500.0), (-0.988, -0.153, -0.027), 11.1, 24.0),
         'casing_walk': ((-2000.0, -720.0, 2450.0), (0.995, -0.1, -0.027), 11.0, 24.0),
         'casing_3m': ((-1800.0, -700.0, 2550.0), (0.2, -1.0, 0.05), 2.0, 24.0),
         'house_front': ((2245.0, 0.0, 2800.0), (1.0, 0.0, 0.05), 9.0, 30.0)}
# The walls' detail layer in the previews (M_World DetailOn emulated on the Walls slot, else Super): today's Super
# instance (MetalWorn 0.4, 1 m tiles) and MI_TripoDredgeBarge_Walls' (PaintWorn 0.65 at 1.6 m). (set, strength, m).
DETAIL_TODAY, DETAIL_PROPOSED = ('MetalWorn', 0.4, 1.0), ('PaintWorn', 0.65, 1.6)


def preview_views(obj, folder, suffix, details=(('', DETAIL_TODAY),)):
    """The whole ship from Tripo's preview angle and the spyglass side, and the close views (CLOSE) with each of details
    ((name suffix, (set, strength, m)) on the walls' slot)."""
    import looter_train as tk
    from looter_tripo_rework import B, D
    bpy.context.view_layer.update()
    for sl in obj.material_slots:
        zp.straight_colour(sl.material)       # a colour map's alpha is a mask: never let it darken the preview
    tk.render([obj], f'ship_tripo_angle_{suffix}', view=(1.05, 1.0, 0.75), lens=50.0, fit=0.95, folder=folder)
    tk.render([obj], f'ship_side_{suffix}', view=(1.0, -0.08, 0.02), lens=70.0, fit=0.62, resolution=(1920, 1080),
              folder=folder)
    slot = next((sl for sl in obj.material_slots if sl.material and sl.material.name.endswith('_Walls')), None) or \
        next((sl for sl in obj.material_slots if sl.material and sl.material.name.endswith('_Super')), None)
    kept = slot.material if slot else None
    for tag, (set_name, strength, scale) in details:
        if slot:
            mean = 0.5076 if set_name == 'MetalWorn' else float(zl.Set(set_name).mean @ zl.LUMA)
            slot.material = zp.detail_copy(kept, set_name, strength, scale, mean, f'_detail_{set_name}')
        for name, (target, way, dist, lens) in CLOSE.items():
            tk.closeup([obj], f'{name}_{suffix}{tag}', B(target), tuple(D(way)), dist, lens=lens, folder=folder)
        if slot:
            done = slot.material
            slot.material = kept
            bpy.data.materials.remove(done)


def zone_preview(obj, folder):
    """The zone map: false colours (ZONE_COLOURS; grey: not painted) over the atlas and on the ship."""
    from looter_tripo_rework import B, D
    target, way, dist, lens = CLOSE['house_3m']
    zp.zone_preview(obj, folder, LAST['zone'], LAST['core'], ZONE_COLOURS, KEEP, (B(target), tuple(D(way)), dist, lens),
                    LAST.get('own', False))


if __name__ == '__main__':
    # Through the imported module: build()'s hook fills its LAST, not this script's (__main__ is another module object).
    import looter_tripo_barge_paint as module
    import looter_tripo_wardens as tw
    folder = os.path.join(lt.PREVIEW_DIR, 'BargeRebake')
    os.makedirs(folder, exist_ok=True)
    if lt.want_preview():
        os.environ['LOOTER_BARGE_PAINT'] = '0'
        before, _ = tw.build('DredgeBarge')
        module.preview_views(before, folder, 'before')
        mesh = before.data
        bpy.data.objects.remove(before)
        bpy.data.meshes.remove(mesh)
        os.environ['LOOTER_BARGE_PAINT'] = '1'
    obj, report = tw.build('DredgeBarge')
    for img in bpy.data.images:
        if img.filepath and 'TripoDredgeBarge' in img.filepath:
            img.reload()            # lt.material reuses a loaded image by its path: the before run's maps
    log(f'report: {report.get("paint")}')
    if lt.want_preview():
        module.preview_views(obj, folder, 'after', (('', DETAIL_TODAY), ('_paintdetail', DETAIL_PROPOSED)))
        module.zone_preview(obj, folder)
