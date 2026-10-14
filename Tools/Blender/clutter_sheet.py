"""The frontier clutter's look sheet: Saved/ArtPreviews/RansomsRest/LookSheets/42_FrontierClutter.jpg, for the user's phone.

Composes the renders Art/Models/Props/FrontierClutter.py makes with --preview (Saved/ArtPreviews/RansomsRest/Clutter/) and
its report (FrontierClutter.report.json: each model's triangles, materials, size, sockets) into one sheet 2000 px wide:
the three vignettes on the town's own buildings (Main Street, a porch, a yard) across the sheet, the set at one scale,
then every model in a tile labelled with its name, triangles and material slots. Run it after the model script:

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\FrontierClutter.py -Preview
    Tools\\artrun.ps1 -Script Tools\\Blender\\clutter_sheet.py
"""
import json
import os

import bpy

import looter_clutter as lc
import looter_homes as homes
import looter_textures as lt

DIR = lc.PREVIEW_DIR
OUT = lc.SHEET_PATH


def load_report():
    if not os.path.exists(lc.REPORT_PATH):
        lt._log(f'sheet: no {lc.REPORT_PATH}: run FrontierClutter.py with -Preview first')
        return {}
    with open(lc.REPORT_PATH, encoding='utf-8') as f:
        return json.load(f)


REPORT = load_report()


def png(name):
    return os.path.join(DIR, name + '.png')


def label(name, shown=None):
    info = REPORT.get(name, {})
    slots = len(info.get('materials', []))
    return f"{shown or name}  {info.get('triangles', 0):,} tris, {slots} slot{'s' if slots != 1 else ''}"


def caption(name, extra=''):
    info = REPORT.get(name, {})
    w, d, h = info.get('size_cm', (0, 0, 0))
    mats = ', '.join(m for m in info.get('materials', []) if m != 'HouseTrim')
    socks = ', '.join('SOCKET_' + s['name'] for s in info.get('sockets', []))
    tail = f"{w:g} x {d:g} x {h:g} cm" + (f"; {mats}" if mats else '') + (f"; {socks}" if socks else '')
    return (extra or info.get('caption', '')) + '\n' + tail


TILES = [
    ('RockingChair', None, None), ('RockingChair_Shawl', None, None), ('PorchChair', None, None),
    ('Washtub', None, None), ('FlowerBox', None, None), ('PorchLantern', None, None),
    ('BroomBucket', None, None), ('Horseshoe', None, None), ('ChoppingBlock', None, None),
    ('HandPump', None, None), ('MilkCans', None, None), ('KitchenGarden', None, None),
    ('ChickenCoop', None, None), ('Spittoon', None, None), ('BottleCrate', None, None),
    ('FlourSacks', None, None), ('Buckboard', None, None), ('Bunting', None, None),
    ('SignBracket', 'SignBracket + SignBoard_Pruitts', 'The bracket on a storefront, PRUITT\'S hung at its\nSOCKET_Sign.'),
    ('SignBoard_Barber', None, None), ('SignBoard_Hotel', None, None),
]
BLOCKS = [
    ('wide', 'Vignette_Street', "Main Street: bunting on the Gilt Spur's balcony, a crate of empties, the spittoon and a rocker on\n"
                                "its walk; Pruitt's blade sign; the buckboard drawn up, its flour sacks half unloaded."),
    ('pair', [('Vignette_Porch', 'A porch lived in (RR_Home_H)', 'The shawled rocker and a chair, a box gone to\n'
                                                                  'seed, the lantern, the broom, the washtub.'),
              ('Vignette_PorchClose', "Along the porch to the door", 'The rocker and its shawl, the window box,\n'
                                                                     'the lantern, the broom and bucket, the tub.')]),
    ('wide', 'Vignette_Yard', "A yard: the pump by the side wall, the chopping block by the rain barrel, milk cans at the\n"
                              "porch, the coop and its run, the kitchen garden behind its stake fence."),
    ('wide', 'Lineup', 'The ground pieces at one scale with a 1.8 m figure (the wall pieces, signs and bunting are\n'
                       'in their own tiles and the vignettes).'),
]


def compose():
    width, pad, gap = 2000, 40, 18
    scene = bpy.data.scenes.new('_Sheet')
    objs = []
    y = pad
    objs.append(homes._text('title', "Ransom's Rest: frontier clutter", pad, y + 72, 84, 0xf2a64a))
    y += 100
    total = sum(info.get('triangles', 0) for info in REPORT.values())
    one = sum(1 for info in REPORT.values() if len(info.get('materials', [])) == 1)
    subtitle = ("Look sheet 42 (concept A, a lived-in town): the small props for porches, yards and Main Street.\n"
                "Blender previews in the area's late-afternoon light on the town's own buildings; nothing imported.\n"
                f"{len(REPORT)} models, {total:,} triangles in all; {one} on one material slot (the house trim sheet),\n"
                "the rest on two. Every piece placed instanced: one draw per mesh and slot.")
    for k, line in enumerate(subtitle.split('\n')):
        objs.append(homes._text(f'sub{k}', line, pad, y + 40, 40, 0xc9c4ba, bold=False))
        y += 52
    y += 20
    inner = width - 2 * pad
    n = [0]

    def wide(name, text):
        nonlocal y
        path = png(name)
        if not os.path.exists(path):
            lt._log(f'sheet: missing {path}')
            return
        img = bpy.data.images.load(path, check_existing=True)
        h = inner * img.size[1] / img.size[0]
        objs.append(homes._image_plane(f'wide{n[0]}', path, pad, y, inner, h))
        y += h + 14
        for k, line in enumerate(text.split('\n')):
            objs.append(homes._text(f'wcap{n[0]}_{k}', line, pad, y + 40, 40, 0xe8e2d6, bold=False))
            y += 52
        y += 30
        n[0] += 1

    def grid(tiles, columns, label_size, caption_size):
        nonlocal y
        tile_w = (inner - gap * (columns - 1)) / columns
        tiles = [t for t in tiles if os.path.exists(t[0])]
        for r in range(0, len(tiles), columns):
            row_h = 0.0
            for c, (path, head, text) in enumerate(tiles[r:r + columns]):
                img = bpy.data.images.load(path, check_existing=True)
                th = tile_w * img.size[1] / img.size[0]
                x = pad + c * (tile_w + gap)
                objs.append(homes._text(f'label{n[0]}', head, x, y + label_size, label_size, 0xf2a64a, z=0.6))
                top = y + label_size + 16
                objs.append(homes._image_plane(f'tile{n[0]}', path, x, top, tile_w, th))
                lines = text.split('\n')
                for j, line in enumerate(lines):
                    objs.append(homes._text(f'cap{n[0]}_{j}', line, x, top + th + caption_size + 10 + (caption_size + 8) * j,
                                            caption_size, 0xe8e2d6, bold=False))
                row_h = max(row_h, label_size + 16 + th + 20 + (caption_size + 8) * len(lines))
                n[0] += 1
            y += row_h + 30

    for block in BLOCKS:
        if block[0] == 'wide':
            wide(block[1], block[2])
        else:
            grid([(png(name), head, text) for name, head, text in block[1]], 2, 46, 36)
    y += 10
    objs.append(homes._text('models', 'The models', pad, y + 60, 60, 0xf2a64a))
    y += 90
    grid([(png(name), label(name, shown), caption(name, extra)) for name, shown, extra in TILES], 3, 38, 30)
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
    scene.render.image_settings.quality = 84
    scene.render.image_settings.color_mode = 'RGB'
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    scene.render.filepath = OUT
    bpy.ops.render.render(write_still=True, scene=scene.name)
    lt._log(f'sheet: {OUT} ({width} x {height})')


compose()
