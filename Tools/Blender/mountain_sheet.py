"""The mountain's look sheet: Saved/ArtPreviews/RansomsRest/LookSheets/37_Mountain.jpg, for the user's phone.

Composes the renders the mountain's scripts made with --preview (Saved/ArtPreviews/RansomsRest/Mountain/, the mine's in
its Mine/ folder) and their triangle counts (the scripts' *.report.json) into one sheet 2000 px wide: wide pictures
across the sheet and pairs of tiles, each labelled with its models and triangles, on looter_homes' look-sheet ground.
Run it after the model scripts:

    Tools/artrun.ps1 -Script Tools/Blender/mountain_sheet.py
"""
import json
import os

import bpy

import looter_homes as homes
import looter_mountain as mtn
import looter_textures as lt

DIR = mtn.PREVIEW_DIR
MINE = os.path.join(DIR, 'Mine')
OUT = os.path.join(mtn.SHEET_DIR, '37_Mountain.jpg')


def reports():
    tris = {}
    for name in os.listdir(DIR):
        if name.endswith('.report.json'):
            with open(os.path.join(DIR, name), encoding='utf-8') as f:
                for model, info in json.load(f).items():
                    if info.get('triangles') and not model.startswith('_'):
                        tris[model] = info['triangles']
    return tris


TRIS = reports()


def t(*names):
    """'Name 1,234' for each model, joined."""
    return '   '.join(f'{n} {TRIS.get("RR_" + n, 0):,}' for n in names)


BLOCKS = [
    ('wide', os.path.join(DIR, 'LiftLine.png'),
     'The ore lift up Larkspur Ridge (a stand-in ridge): the winch house on the quarry floor, the tower\n'
     '(its saddle at +30 m), the head frame on the crest (+47 m), the cage riding east of the cable.'),
    ('grid', [
        (os.path.join(DIR, 'LiftHouse.png'), t('LiftHouse'),
         'The winch house, the bottom gantry and landing\non its north side, the boiler stack.'),
        (os.path.join(DIR, 'LiftHouse_Inside.png'), 'The winch house inside',
         'Boiler, drum and cable, flywheel and engine,\nbrake, coal bin, speaking tube, Hiram\'s stool.'),
        (os.path.join(DIR, 'LiftHead.png'), t('LiftHead', 'LiftCage'),
         'The head frame and return sheave, the\ncounterweight, the cage docked, bell, tool box.'),
        (os.path.join(DIR, 'LiftTower.png'), t('LiftTower', 'LiftCable'),
         'The tower: a trestle west of the cable,\nthe saddle on a cantilever, the cage passing.'),
        (os.path.join(DIR, 'Watchtower.png'), t('Watchtower'),
         "Calder's watchtower: 4 x 4 m, its deck\nat 12 m, an inside stair of six flights."),
        (os.path.join(DIR, 'Watchtower_Deck.png'), 'The watchtower deck',
         'The watch fire basket past the eaves,\nthe heliograph, the spyglass, a lamp.'),
        (os.path.join(DIR, 'RangerCabin.png'), t('RangerCabin'),
         "Ruth's cabin: hewn logs, a shake roof,\nthe woodshed lean-to, the door open."),
        (os.path.join(DIR, 'RangerCabin_Inside.png'), 'The cabin inside',
         'Bunk, cold stove, table with the post\'s log\nand her journal, shelf, the Rim map.'),
        (os.path.join(DIR, 'LeanTo.png'), t('LeanTo'),
         'The empty stable and its corral,\nthe gate standing open.'),
        (os.path.join(DIR, 'MinePortal.png'), t('MinePortal'),
         'The Providence\'s portal: cribbing, brow,\nheader, doors open, the tag board.'),
        (os.path.join(DIR, 'TagBoard.png'), t('TagBoard_TagsIn', 'TagBoard_TagsOut'),
         'IN and OUT: the shift that came up on OUT,\nthe eleven\'s tags on IN (or paid, on OUT).'),
        (os.path.join(DIR, 'OreBin.png'), t('OreBin'),
         'The ore bin: top deck and grizzly at 3.6 m,\nthe trestle north, the chute south.'),
        (os.path.join(DIR, 'PowderHouse.png'), t('PowderHouse'),
         'The powder house: dry fieldstone,\nquoins, the door padlocked.'),
        (os.path.join(DIR, 'Smithy.png'), t('Smithy'),
         'The smithy: a cold forge and its stack,\nanvil, bellows, grindstone, barrel.'),
        (os.path.join(DIR, 'Tramway.png'), t('TramTrack_4m', 'OreCar'),
         'Narrow-gauge track (4 m, 2 m, a curve,\nthe end stop) and the ore car.'),
        (os.path.join(MINE, 'Mine_Fall.png'), t('Mine_Fall'),
         'The fall: the roof come down over the\nstope\'s north end, props snapped in it.'),
    ]),
    ('wide', os.path.join(MINE, 'Mine_Plan.png'),
     'The Providence, in plan (north to the right, cut 2.4 m over each floor): adit, gallery and office\n'
     'at +58 m, the decline down 8 m, the lower drift and the stope at +50 m, the manway back up.'),
    ('grid', [
        (os.path.join(MINE, 'Mine_Adit.png'), t('Mine_Adit', 'Mine_TimberSet'), 'The adit, lit by its lamps only.'),
        (os.path.join(MINE, 'Mine_Gallery.png'), t('Mine_Gallery', 'Mine_Pillar'), 'The upper gallery (fight: MineUpper).'),
        (os.path.join(MINE, 'Mine_Office.png'), t('Mine_Office'), "The Foreman's office."),
        (os.path.join(MINE, 'Mine_Decline.png'), t('Mine_Decline'), 'The decline to the dog-leg landing.'),
        (os.path.join(MINE, 'Mine_Drift.png'), t('Mine_Drift', 'Mine_TimberSetWide'),
         'The lower drift (fight: MineLower).'),
        (os.path.join(MINE, 'Mine_Barricade.png'), t('Mine_Barricade', 'Mine_BarricadeTags'), "The eleven's barricade."),
        (os.path.join(MINE, 'Mine_Stope.png'), t('Mine_Stope', 'Mine_StopeProp'), "The stope, the Foreman's arena."),
        (os.path.join(MINE, 'Mine_Manway.png'), t('Mine_Manway', 'Mine_ManwayGate'), 'The manway stair back up.'),
    ]),
    ('wide', os.path.join(MINE, 'Mine_Kit.png'),
     'The mine kit: timber sets (4 m, 6 m), the pillar, the prop and the broken prop, the barricade and its tags,\n'
     'the manway gate, the winch, the lantern, the ore car. Rock: RockCliffDen and DenFloor (Den Rock\'s).'),
]


def compose():
    width, pad, gap, columns = 2000, 40, 18, 2
    scene = bpy.data.scenes.new('_Sheet')
    objs = []
    y = pad
    objs.append(homes._text('title', "Ransom's Rest: the mountain", pad, y + 72, 84, 0xf2a64a))
    y += 100
    total = sum(v for k, v in TRIS.items() if k.startswith('RR_'))
    subtitle = ("The ore lift, Calder's Post, the Providence's Diggings and the mine's kit. Blender previews\n"
                "(the area's late-afternoon light; the mine lit by its own lamps). Nothing imported yet.\n"
                f"{len([k for k in TRIS if k.startswith('RR_')])} models, {total:,} triangles in all (each counted once).")
    for k, line in enumerate(subtitle.split('\n')):
        objs.append(homes._text(f'sub{k}', line, pad, y + 40, 40, 0xc9c4ba, bold=False))
        y += 52
    y += 20
    inner = width - 2 * pad
    tile_w = (inner - gap * (columns - 1)) / columns
    n = 0
    for block in BLOCKS:
        if block[0] == 'wide':
            _, png, caption = block
            if not os.path.exists(png):
                lt._log(f'sheet: missing {png}')
                continue
            img = bpy.data.images.load(png, check_existing=True)
            h = inner * img.size[1] / img.size[0]
            objs.append(homes._image_plane(f'wide{n}', png, pad, y, inner, h))
            y += h + 14
            for k, line in enumerate(caption.split('\n')):
                objs.append(homes._text(f'wcap{n}_{k}', line, pad, y + 40, 40, 0xe8e2d6, bold=False))
                y += 52
            y += 30
            n += 1
            continue
        tiles = [tile for tile in block[1] if os.path.exists(tile[0])]
        for r in range(0, len(tiles), columns):
            row_h = 0.0
            for c, (png, label, caption) in enumerate(tiles[r:r + columns]):
                img = bpy.data.images.load(png, check_existing=True)
                th = tile_w * img.size[1] / img.size[0]
                x = pad + c * (tile_w + gap)
                objs.append(homes._text(f'label{n}', label, x, y + 52, 50, 0xf2a64a, z=0.6))
                objs.append(homes._image_plane(f'tile{n}', png, x, y + 70, tile_w, th))
                lines = caption.split('\n')
                for j, line in enumerate(lines):
                    objs.append(homes._text(f'cap{n}_{j}', line, x, y + 70 + th + 44 + 46 * j, 36, 0xe8e2d6, bold=False))
                row_h = max(row_h, 70 + th + 28 + 46 * len(lines))
                n += 1
            y += row_h + 30
    height = int(y + pad - 10)
    objs.append(homes._rect('ground', 0, 0, width, height, 0x16171a))
    for o in objs:
        scene.collection.objects.link(o)
    cam = bpy.data.objects.new('_SheetCam', bpy.data.cameras.new('_SheetCam'))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = max(width, height)
    cam.data.sensor_fit = 'AUTO'
    cam.location = (width * 0.5, -height * 0.5, 100.0)
    cam.data.clip_start, cam.data.clip_end = 1.0, 500.0
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 8
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'None'
    world = bpy.data.worlds.new('_SheetWorld')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(0x16171a)
    scene.world = world
    scene.render.image_settings.file_format = 'JPEG'
    scene.render.image_settings.quality = 82
    scene.render.image_settings.color_mode = 'RGB'
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    scene.render.filepath = OUT
    bpy.ops.render.render(write_still=True, scene=scene.name)
    lt._log(f'sheet: {OUT} ({width} x {height})')


compose()
