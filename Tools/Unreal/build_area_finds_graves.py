"""The two dug finds for build_area_finds.py (Docs/Work/2026-10-28-Core/FINDS.md section 2): Pike's cache by the Nose's
north foot (N4: the field-book scrap's bearing from the bench mark, a rust-red stone and a survey cairn by it) and the
miser's grave on boot hill (N5: a headboard to read, coins on the mound, the MiserPlot ambush round it is the FIGHTS
lane's). Each is an AChest of kind Grave (held to dig). Helpers are build_area_finds_sites.py's; not run on its own.
"""
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402
from build_area_finds_sites import (GRAVE_HALF, GRAVE_HEIGHT, OXIDE_FALLBACK, blocked, core, footprint_ok, frame,  # noqa: E402
                                    ground_at, log, ring_search, yaw_of)


# ---------------------------------------------------------------------------
# N4, Pike's cache; N5, the miser's grave
# ---------------------------------------------------------------------------

def place_pikes_cache(build, entry, found):
    spec, chest = entry['cache'], entry['chest']
    fid = entry['id']
    mark = found.get(spec['clue']['from'])
    x0, y0 = spec['at']
    toward = yaw_of((mark.get_actor_location().x - x0, mark.get_actor_location().y - y0)) if mark else 0.0

    def test(x, y, yaw):
        z = ground_at(x, y)
        if z is None or not footprint_ok(x, y, z, GRAVE_HALF, yaw, GRAVE_HEIGHT):
            return None
        for along, across in (spec['stone']['at'], spec['cairn']['at']):
            px, py = frame(x, y, yaw, along, across)
            g = ground_at(px, py, z + 300.0, z - 300.0)
            if g is None or abs(g - z) > 80.0 or blocked((px, py, g + 60.0), (40.0, 40.0, 40.0)):
                return None
        return z
    spot = ring_search(x0, y0, spec['ring'], (toward, toward + 30.0, toward - 30.0), test)
    if spot is None:
        build.warn(f'{fid}: no level, clear grave by ({x0:.0f}, {y0:.0f}): left out')
        return None
    x, y, z, yaw = spot
    box = core().place_chest(build, chest, x, y, z, yaw, fid)
    if box is None:
        return None
    oxide = core().area_material(build, spec['stone']['material'])
    if oxide is None and unreal.EditorAssetLibrary.does_asset_exist(OXIDE_FALLBACK):
        oxide = unreal.load_asset(OXIDE_FALLBACK)
    for name, part in (('Stone', spec['stone']), ('Cairn', spec['cairn'])):
        px, py = frame(x, y, yaw, *part['at'])
        g = ground_at(px, py, z + 300.0, z - 300.0)
        s = part['scale']
        piece = core().place_prop(build, part['mesh'], (px, py, g), yaw + part.get('turn', 0.0), fid, f'{fid}_{name}',
                                  scale=(s, s, s))
        if piece is not None and name == 'Stone' and oxide is not None:
            piece.static_mesh_component.set_material(0, oxide)
    if mark is not None:
        at = mark.get_actor_location()
        bearing = math.degrees(math.atan2(y - at.y, -(x - at.x)))
        yards = math.hypot(x - at.x, y - at.y) / 91.44
        clue = spec['clue']
        said = f'{yards:.1f} yards S. {bearing:.0f} E. from the bench mark'
        if abs(bearing - clue['eastOfSouth']) <= 12.0 and abs(yards - clue['yards']) <= 5.0:
            log(build, fid, said)
        else:
            build.warn(f'FINDS {fid}: {said}, the scrap says {clue["yards"]} yards S. {clue["eastOfSouth"]} E.')
    log(build, fid, f'grave at ({x:.0f}, {y:.0f}, {z:.0f}) facing {yaw:.0f}')
    return core().place_spot(build, entry, (x, y, z), box), (x, y, z)


def place_miser_grave(build, entry, found):
    spec, chest = entry['grave'], entry['chest']
    fid = entry['id']
    back = spec['board']['back']

    def test(x, y, yaw):
        z = ground_at(x, y)
        if z is None or not footprint_ok(x, y, z, GRAVE_HALF, yaw, GRAVE_HEIGHT):
            return None
        bx, by = frame(x, y, yaw, -back, 0.0)
        g = ground_at(bx, by, z + 300.0, z - 300.0)
        if g is None or abs(g - z) > 40.0 or blocked((bx, by, g + 60.0), (25.0, 45.0, 50.0)):
            return None
        return z
    spot = ring_search(spec['at'][0], spec['at'][1], spec['ring'], spec['yaws'], test)
    if spot is None:
        build.warn(f'{fid}: no level, clear grave by ({spec["at"][0]}, {spec["at"][1]}): left out')
        return None
    x, y, z, yaw = spot
    box = core().place_chest(build, chest, x, y, z, yaw, fid)
    if box is None:
        return None
    bx, by = frame(x, y, yaw, -back, 0.0)
    bz = ground_at(bx, by, z + 300.0, z - 300.0)
    core().place_readable(build, spec['readable'], bx, by, z if bz is None else bz, yaw, fid, spec['board']['mesh'])
    # The coins on the mound (a tell: hidden once found), and the same coins on the coffin in the open grave after.
    hit = story.onto_host(box, unreal.Vector(x, y, z + 200.0), unreal.Vector(x, y, z - 50.0))
    top = hit.z if hit is not None else z + spec['coins']['up']
    coins = core().place_prop(build, spec['coins']['mesh'], (x, y, top + 0.5), yaw + 20.0, fid, f'{fid}_Coins', solid=False)
    cls = story.actor_class('StoryProp')
    mesh = core().find_mesh(spec['coins']['mesh'])
    if cls is not None and mesh is not None:
        kept = build.place(cls, (x, y, z - spec['openCoins']['drop']), yaw - 15.0, label=f'{fid}_CoinsOpen', folder=core().FOLDER,
                           tags=(core().PIECE_TAG, fid))
        kept.get_editor_property('mesh').set_static_mesh(mesh)
        kept.set_editor_property('shown_when', [core().texts().condition({'with_flags': [f'Find.{fid}']})])
    log(build, fid, f'grave at ({x:.0f}, {y:.0f}, {z:.0f}) facing {yaw:.0f}, {math.hypot(x - spec["at"][0], y - spec["at"][1]):.0f} '
        'cm from the ambush\'s middle')
    return core().place_spot(build, entry, (x, y, z), box, tells=[coins]), (x, y, z)
