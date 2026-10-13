"""The look sheet of Ransom's Rest's new town buildings (Docs/Areas/RansomsRest_Expansion.md, section 9):
Saved/ArtPreviews/RansomsRest/LookSheets/36_Town.jpg, about 2000 px wide for a phone, every model labelled with its name
and triangles, its rooms shown roof off and from inside.

It reads the reports the four scripts write when run with --preview (Saved/ArtPreviews/RansomsRest/Town/<Script>_report.json:
FrontsRR, DepotRR, WorkshopBright, BarnWhitlock) and their renders; the banner is the whole set at one scale, built here
from the scripts themselves (run in this scene without --preview, so they render nothing of their own).

    Tools\\artrun.ps1 -Script Tools\\Blender\\town_sheet.py              (after the four scripts' -Preview runs)
    Tools\\artrun.ps1 -Script Tools\\Blender\\town_sheet.py -ScriptArgs --no-banner     (the tiles only, quicker)
"""
import json
import os
import runpy
import sys

import bpy
from mathutils import Vector

import looter_homes as homes
import looter_rr_town as rr
import looter_textures as lt

SCRIPTS = ('FrontsRR', 'DepotRR', 'WorkshopBright', 'BarnWhitlock')
SOURCE = os.path.join(lt.REPO, 'Art', 'Models', 'Buildings')
# The tiles in order: (render name, label, caption). A model's first tile carries its triangles.
TILES = [
    ('RR_Front_Assay', None, None),
    ('RR_Front_Hotel', None, None),
    ('RR_Front_Barber', None, None),
    ('RR_CoalStage', None, None),
    ('RR_Depot', None, None),
    ('RR_Depot_Back', 'RR_Depot: the back', 'The yard side: the barred back door, the ticket\noffice window, the stair hall window upstairs.'),
    ('RR_Depot_Plan', 'RR_Depot: roof and upper floor off', 'Waiting room (left), ticket office (back right),\nbaggage room with LOST PROPERTY (front right).'),
    ('RR_Depot_WaitingRoom', 'RR_Depot: the waiting room', 'From the platform doors: benches, the stove, the\nstair up to the agent; panes broken out.'),
    ('RR_Depot_TicketWindow', 'RR_Depot: the ticket window', 'The grille (the talking window), the schedule\nboard, the clock at twenty to nine.'),
    ('RR_Depot_LostProperty', 'RR_Depot: lost property', 'The counter and its hatch under the sign; tagged\nrifles and shotguns, bundles on the shelves.'),
    ('RR_Depot_TicketOffice', 'RR_Depot: the ticket office', 'Behind the grille: the counter and till, the\nticket case, the desk with the telegraph key.'),
    ('RR_Depot_StairHall', 'RR_Depot: up the stair', "The landing and Mr. Sloane's shut door (the\nspeaker point), its lamp."),
    ('RR_Workshop_Bright', None, None),
    ('RR_Workshop_Plan', 'RR_Workshop_Bright: roof off', 'Coffin benches, lumber racks, tool wall, stove,\nthe hook and slate, the stair enclosure.'),
    ('RR_Workshop_Inside', 'RR_Workshop_Bright: inside', 'From the yard doors: the benches, the finished\ncoffin, the slate and hook by the cellar door.'),
    ('RR_Workshop_Stair', 'RR_Workshop_Bright: the cellar stair', 'Through the cellar door: flight one down to the\nhalf landing, stone spine and parapet.'),
    ('RR_Workshop_Cellar', 'RR_Workshop_Bright: the cellar (3.3 m down)', 'Five groin-vaulted bays on two piers, the\nstairwell bay, the cold room at the back.'),
    ('RR_Workshop_CellarView', 'RR_Workshop_Bright: in the cellar', 'Coffins on trestles, lids shoved aside; racks\nalong the walls; lamps on the piers.'),
    ('RR_Workshop_ColdRoom', 'RR_Workshop_Bright: the cold room', "Thaddeus's bench and its lamp; the trestles with\nnothing on them."),
    ('RR_Barn_Whitlock', None, None),
    ('RR_Barn_Whitlock_Bay', 'RR_Barn_Whitlock: the hay bay', "Loose hay, bales, the lantern; Amos's six bales\nstack on the floor in front (BaleStack)."),
    ('RR_Barn_Whitlock_Tack', 'RR_Barn_Whitlock: the tack room', 'Its door open, the hands\' wages chalked inside it;\nharness pegs, the lantern hook.'),
    ('RR_CellarDoor', None, None),
]


def load_reports():
    reports = {}
    for script in SCRIPTS:
        path = os.path.join(rr.PREVIEW_DIR, f'{script}_report.json')
        if not os.path.exists(path):
            print(f'LOOTER: warning: no {path}: run Art/Models/Buildings/{script}.py with -Preview first', flush=True)
            continue
        with open(path, encoding='utf-8') as file:
            reports.update(json.load(file))
    return reports


def banner(reports):
    """Every model at one scale in a row, fronts on a line, the big ones behind: built here from the scripts."""
    built = []
    for script in SCRIPTS:
        before = set(bpy.context.scene.objects)
        runpy.run_path(os.path.join(SOURCE, script + '.py'), run_name='__sheet__')
        built += [o for o in bpy.context.scene.objects if o not in before and o.parent is None and o.type == 'MESH'
                  and not o.name.startswith(('_', 'UCX_'))]
    by_name = {o.name: o for o in built}
    rows = [['RR_Workshop_Bright', 'RR_Depot', 'RR_Barn_Whitlock'],
            ['RR_Front_Assay', 'RR_Front_Hotel', 'RR_Front_Barber', 'RR_CoalStage', 'RR_CellarDoor']]
    scene, sun, human = rr.stage()
    homes.light(scene, sun, homes.golden_sun(*homes.SUN))
    for o in scene.objects:
        if o.name.startswith('UCX_'):
            o.hide_render = True
    # Every pivot is on its model's front line, so a row's fronts line up at the row's y.
    placed = []
    for k, row in enumerate(rows):
        objs = [by_name[n] for n in row if n in by_name]
        boxes = [homes._bounds(o, local=True) for o in objs]
        y = 0.0
        if k:
            y = min(o.location.y + homes._bounds(o, local=True)[0].y for o in placed) - max(hi.y for lo, hi in boxes) - 4.0
        x = -(sum(hi.x - lo.x for lo, hi in boxes) + 3.0 * (len(objs) - 1)) * 0.5
        for obj, (lo, hi) in zip(objs, boxes):
            obj.location = (x - lo.x, y, 0.0)
            x += hi.x - lo.x + 3.0
            placed.append(obj)
    # The workshop's cellar and the depot's are underground: the ground hides them, as the valley's will.
    human.location = (x - 1.0, placed[-1].location.y - 1.5, 0.0)
    human.hide_render = False
    bpy.context.view_layer.update()
    eye, target = homes.frame_shot(placed + [human], (-0.2, -1.0, 0.55), homes.ROW_SHOT, 38.0, margin=1.02,
                                   height_bias=0.3)
    return homes.shoot(scene, os.path.join(rr.PREVIEW_DIR, 'Town_new_row.png'), eye, target, 38.0, homes.ROW_SHOT, -0.1)


def main():
    reports = load_reports()
    tris_of = {name: info.get('triangles', 0) for name, info in reports.items()}
    captions = {name: '\n'.join(info.get('captions', [])) for name, info in reports.items()}
    tiles = []
    for shot, label, caption in TILES:
        png = os.path.join(rr.PREVIEW_DIR, shot + '.png')
        if not os.path.exists(png):
            print(f'LOOTER: warning: no {png}', flush=True)
            continue
        if label is None:
            label = f'{shot}  {tris_of.get(shot, 0):,} tris'
            caption = captions.get(shot, '')
        tiles.append((png, label, caption))
    total = sum(tris_of.values())
    row = None
    if '--no-banner' not in homes.args():
        row_png = banner(reports)
        row = (row_png, f'The set at one scale (the figure is 1.8 m): {len(tris_of)} models, {total:,} triangles in all.\n'
                        f'The depot\'s and the workshop\'s rooms are inside; the cellar lies under the ground.')
    homes.look_sheet(rr.SHEET_PATH, "Ransom's Rest: the town's new buildings",
                     'Three Main Street fronts, the depot rebuilt with its rooms, the coal stage, Bright &\n'
                     "Daughter's workshop over its vaulted cellar, and Amos's hay barn. Blender previews\n"
                     "in the area's late-afternoon sun, lamps lit inside; nothing imported yet.",
                     row, tiles)


main()
