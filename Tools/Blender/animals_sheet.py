"""Look sheet 44, the small animals (the 2026-10-14 "make it alive" polish): Saved/ArtPreviews/RansomsRest/LookSheets/
44_Small_animals.jpg, about 2000 px wide for a phone, every model labelled with its name and triangles.

It reads the reports and renders the three scripts write when run with --preview (Art/Models/Creatures/Chicken.py, Cat.py,
Swallow.py: Saved/ArtPreviews/RansomsRest/Animals/<Script>_report.json and the PNGs beside them) and builds its banner
itself: the animals at one scale beside a man's boots, from the scripts run in this scene without --preview.

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Chicken.py -Preview      (and Cat.py, Swallow.py)
    Tools\\artrun.ps1 -Script Tools\\Blender\\animals_sheet.py
    Tools\\artrun.ps1 -Script Tools\\Blender\\animals_sheet.py -ScriptArgs --no-banner     (the tiles only, quicker)
"""
import json
import math
import os
import runpy

import bpy
from mathutils import Matrix, Vector

import looter_animals as la
import looter_homes as homes
import looter_textures as lt

SOURCE = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures')
SCRIPTS = ('Chicken', 'Cat', 'Swallow')


def load_reports():
    tris = {}
    for script in SCRIPTS:
        path = os.path.join(la.PREVIEW_DIR, f'{script}_report.json')
        if not os.path.exists(path):
            la.log(f'warning: no {path}: run Art/Models/Creatures/{script}.py with -Preview first')
            continue
        with open(path, encoding='utf-8') as f:
            for name, info in json.load(f).items():
                tris[name] = info.get('triangles', 0)
    return tris


def banner():
    """The three chickens, the cat and a swallow on its nest at one scale, a man's boots behind for size."""
    ns = {s: runpy.run_path(os.path.join(SOURCE, s + '.py'), run_name='__sheet__') for s in SCRIPTS}
    birds = {b.name: b for b in ns['Chicken']['birds']}
    cat = ns['Cat']['cat']
    sw = ns['Swallow']
    for mat in bpy.data.materials:
        if mat.name in ('ChickenAtlas', 'CatAtlas', 'SwallowAtlas'):
            mat.use_backface_culling = True
    studio = la.Studio()
    la.hide([b.rig.arm for b in birds.values()] + [cat.rig.arm] + [sw[k] for k in ('FLYING', 'PERCHED', 'HEAD', 'NESTS_OBJ')]
            + sw['WINGS'])
    objs = [studio.snapshot(birds['Rooster'].rig, birds['Rooster'].obj, birds['Rooster'].poses['rest'], at=(-0.55, 0.15, 0.0),
                            yaw=-35.0, name='R'),
            studio.snapshot(birds['HenBrown'].rig, birds['HenBrown'].obj, birds['HenBrown'].poses['peck'], at=(-0.95, -0.12, 0.0),
                            yaw=-70.0, name='HB'),
            studio.snapshot(birds['HenWhite'].rig, birds['HenWhite'].obj, birds['HenWhite'].poses['alert'], at=(-0.18, -0.05, 0.0),
                            yaw=-20.0, name='HW'),
            studio.snapshot(cat.rig, cat.obj, ns['Cat']['POSES']['sit'], at=(0.32, 0.05, 0.0), yaw=-120.0, name='C')]
    # A swallow clinging at a nest on a post's top, as it would under an overhang (the post is a stand-in).
    post = la.rock_block(studio, (0.32, 0.3, 0.62), (0.86, 0.25, 0.31), seed=3, name='Post')
    at = Vector((0.86, 0.1, 0.62))
    nest = studio.copy(sw['NESTS_OBJ'], name='Nest')
    nest.matrix_world = Matrix.Translation(at)
    socket = next(c for c in sw['NESTS_OBJ'].children if c.name.startswith('SOCKET_Nest2')).matrix_world
    cling = studio.copy(sw['PERCHED'], name='Cling')
    cling.matrix_world = Matrix.Translation(at) @ socket @ Matrix.Rotation(math.radians(180.0), 4, 'Z')
    head_at = next(c for c in sw['PERCHED'].children if c.name.startswith('SOCKET_Head')).matrix_world.translation
    head = studio.copy(sw['HEAD'], name='ClingHead')
    head.matrix_world = cling.matrix_world @ Matrix.Translation(head_at) @ Matrix.Rotation(math.radians(-60.0), 4, 'Z')
    objs += [nest, cling, head]
    human = studio.keep(homes.figure())
    human.location = (-0.2, 0.75, 0.0)
    bpy.context.view_layer.update()
    eye, target = homes.frame_shot(objs, (-0.25, -1.0, 0.28), (2400, 1000), 50.0, margin=1.06, height_bias=0.4)
    path = os.path.join(la.PREVIEW_DIR, 'Animals_row.png')
    homes.shoot(studio.scene, path, eye, target, 50.0, (2400, 1000), -0.1)
    return path


# --- The sheet: homes.look_sheet's ground and type, with full-width rows for the pose strips ---

def compose(out_jpg, title, subtitle, rows, width=2000, gap=18, pad=40):
    """rows: lists of one tile (full width) or two (side by side); a tile is (png, label, caption)."""
    scene = bpy.data.scenes.new('_Sheet')
    objs = []
    y = pad
    objs.append(homes._text('title', title, pad, y + 72, 84, 0xf2a64a))
    y += 100
    for k, line in enumerate(subtitle.split('\n')):
        objs.append(homes._text(f'subtitle{k}', line, pad, y + 40, 40, 0xc9c4ba, bold=False))
        y += 52
    y += 18
    inner = width - 2 * pad
    n = 0
    for row in rows:
        cols = len(row)
        tile_w = (inner - gap * (cols - 1)) / cols
        row_h = 0.0
        for c, (png, label, caption) in enumerate(row):
            img = bpy.data.images.load(png, check_existing=True)
            iw, ih = img.size
            th = tile_w * ih / iw
            x = pad + c * (tile_w + gap)
            objs.append(homes._text(f'label{n}', label, x, y + 56, 54 if cols == 1 else 48, 0xf2a64a, z=0.6))
            objs.append(homes._image_plane(f'tile{n}', png, x, y + 74, tile_w, th))
            lines = caption.split('\n')
            for j, line in enumerate(lines):
                objs.append(homes._text(f'cap{n}_{j}', line, x, y + 74 + th + 46 + 46 * j, 36, 0xe8e2d6, bold=False))
            row_h = max(row_h, 74 + th + 30 + 46 * len(lines))
            n += 1
        y += row_h + 30
    height = int(y + pad - 10)
    objs.append(homes._rect('ground', 0, 0, width, height, 0x16171a))
    for o in objs:
        scene.collection.objects.link(o)
    cam = bpy.data.objects.new('_SheetCam', bpy.data.cameras.new('_SheetCam'))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = width
    cam.data.sensor_fit = 'HORIZONTAL'
    cam.location = (width * 0.5, -height * 0.5, 100.0)
    cam.data.clip_start, cam.data.clip_end = 1.0, 500.0
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 16
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'None'
    world = bpy.data.worlds.new('_SheetWorld')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(0x16171a)
    scene.world = world
    scene.render.image_settings.file_format = 'JPEG'
    scene.render.image_settings.quality = 90
    scene.render.image_settings.color_mode = 'RGB'
    os.makedirs(os.path.dirname(out_jpg), exist_ok=True)
    scene.render.filepath = out_jpg
    bpy.ops.render.render(write_still=True, scene=scene.name)
    la.log(f'animals: look sheet {out_jpg} ({width} x {height})')


def main():
    tris = load_reports()
    P = lambda name: os.path.join(la.PREVIEW_DIR, name + '.png')
    t = lambda name: f'{tris.get(name, 0):,}'
    rows = []
    if '--no-banner' not in homes.args():
        rows.append([(banner(), 'All at one scale',
                      'The rooster (55 cm to his comb), the hens (43 cm), the barn cat sitting (25 cm at the shoulder) and a\n'
                      'cliff swallow clinging at its mud nest. The figure behind is a 1.8 m man.')])
    rows += [
        [(P('Chickens'), f'SK_HenBrown, SK_HenWhite  {t("HenBrown")} tris each;  SK_Rooster  {t("Rooster")} tris',
          'A speckled brown hen, a black-breasted red rooster and the white hen pecking. Painted feathers on one smooth\n'
          'skin, shaped feathers where the outline needs them (hackles, wings, tail, the rooster\'s saddle and sickles).\n'
          'One material for all three (2048 px atlas), 24 bones each, no hit zones (shots pass through, as through Hob).')],
        [(P('Hen_Poses'), 'Hen: alert, peck, scratch, walk, settled',
          'The extremes the game\'s code blends between: stretched up watching, the beak to the ground, a foot raking\n'
          'back, a walking step (the head held still, then snapped forward), sat down on her belly.')],
        [(P('Hen_Flutter'), 'Flutter: wings up, wings down',
          'A gunshot or a running player sends them off in a flapping run: wings hinge open from the shoulder and the\n'
          'primaries fan out (two bones); flapped between these two at about 8 beats a second.'),
         (P('Rooster_Poses'), 'Rooster: crowing, walking',
          'He crows now and then (beak open, chest out), struts\nand keeps an eye on the hens.')],
        [(P('Chicken_Heads'), 'Chickens up close',
          'Single combs (his tall, hers small), wattles\non their own bone, earlobes, orange eyes,\na beak in two halves, '
          'scaled legs, clawed toes.'),
         (P('Cat_Head'), 'The cat, watching you go by',
          'The forehead\'s M, dark-rimmed eyes with slit\npupils, a pale muzzle and chin, whiskers,\ncupped ears.')],
        [(P('Cat'), f'SK_BarnCat  {t("BarnCat")} tris',
          'A lean brown mackerel tabby for a porch, loafing (its idle) and sitting up. 29 bones: a spine, a tail chain,\n'
          'four legs, ears. One material (2048 x 1024 px atlas); no hit zones.')],
        [(P('Cat_Poses'), 'Cat: walking (two steps), stretching, slinking',
          'The walk\'s two contacts, the stretch on waking (front legs out, hips up), and the low quick slink under the porch\n'
          'when a gun goes off, ears back.')],
        [(P('Swallows'), f'SwallowNests  {t("SwallowNests")} tris;  CliffSwallow pieces',
          'Gourds of dried mud pellets under an overhang, each with a dark\nentrance spout; swallows clinging, peeking out,\n'
          'darting in and out. One material, 1024 px.'),
         (P('Swallow_Close'), f'CliffSwallow_Perched  {t("CliffSwallow_Perched")};  _Head  {t("CliffSwallow_Head")}',
          'A cliff swallow, not a barn swallow: square tail, pale rump,\npale forehead, chestnut face, steel-blue back.')],
        [(P('Swallow_Flight'), f'CliffSwallow_Flying  {t("CliffSwallow_Flying")} tris;  wings  '
                               f'{t("CliffSwallow_WingInnerL")} + {t("CliffSwallow_WingOuterL")} a side',
          'Seen from above: wings up, wings down, gliding, and the hands swept back for a dive. Pieces like the crows\':\n'
          'a body, an arm and a hand each side, turned by the fauna code; the whole flock is one draw per piece.')],
    ]
    rows = [[tile for tile in row if os.path.exists(tile[0])] for row in rows]
    compose(la.SHEET_PATH, "Ransom's Rest: small animals",
            'Chickens for a yard or two, a barn cat for a porch, cliff swallows for the cliffs: life about the town while\n'
            'its people stay unseen. Blender previews in the area\'s late-afternoon sun; nothing imported yet.',
            [row for row in rows if row])


main()
