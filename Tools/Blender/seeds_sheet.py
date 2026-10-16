"""The look sheets of the seeds' art (Saved/Seeds/CONTRACT.md, ART; 2026-10-16), for the user's phone: three JPGs, 1600 px
wide, on a neutral grey, in Saved/Concepts/Seeds/:
    1_PagesLetters.jpg   SM_LedgerPage, SM_DeaconLetter and SM_PostRack (the letter standing at its SOCKET_Slot_3)
    2_ReachesMap.jpg     SM_ReachesMap on a wall, close, and its painted texture flat
    3_SevenPoster.jpg    the Seven cell of T_Posters_BC, on the Rangers' notice board between Ellis's poster and
                         Calder's note, a paid face crossed with the SevenCross cell, and the cell itself
The models come from Art/Models/Props/MissionProps.py's builders (built here, nothing exported), the board from
Boardwalk.py, the decals from the installed atlas (Art/Textures/Posters/T_Posters_BC.png): run looter_posters.py with
--install and make_reaches_map.py first. Neutral light: a white sun from the front left and a grey sky.

    Tools\\artrun.ps1 -Script Tools\\Blender\\seeds_sheet.py
"""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import looter_posters as lp  # noqa: E402
import looter_textures as lt  # noqa: E402

REPO = lt.REPO
OUT = os.path.join(REPO, 'Saved', 'Concepts', 'Seeds')
SHOTS = os.path.join(lt.PREVIEW_DIR, 'Seeds')
ATLAS = os.path.join(REPO, 'Art', 'Textures', 'Posters', 'T_Posters_BC.png')
MAP_FLAT = os.path.join(lt.PREVIEW_DIR, 'Seeds', 'ReachesMap_flat.png')
GREY = 0x8c8c8a                       # the sheets' ground
STUDIO = 0x6e6e6c                     # the renders' floor and wall
SHEET_W, PAD, GAP = 1600, 40, 20


# --- The studio ---

def studio():
    scene = lp._render_setup()
    scene.eevee.taa_render_samples = 64
    scene.view_settings.look = 'AgX - Base Contrast'
    world = bpy.data.worlds.new('_Studio')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(0x9a9a98)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.9
    scene.world = world
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy, sun.data.color, sun.data.angle = 3.2, (1.0, 0.98, 0.95), math.radians(3.0)
    sun.rotation_euler = (-Vector((-0.5, -0.8, 0.75)).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    grey = bpy.data.materials.new('_StudioGrey')
    grey.use_nodes = True
    bsdf = grey.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = lt.hex_color(STUDIO)
    bsdf.inputs['Roughness'].default_value = 0.9
    floor = lp._mesh_object('_Floor', [(-20, -20, 0), (20, -20, 0), (20, 20, 0), (-20, 20, 0)], [(0, 1, 2, 3)])
    floor.data.materials.append(grey)
    wall = lp._mesh_object('_Wall', [(-20, 0.0005, 0), (20, 0.0005, 0), (20, 0.0005, 8), (-20, 0.0005, 8)],
                           [(0, 1, 2, 3)])
    wall.data.materials.append(grey)
    return scene, wall


def hide_helpers():
    for o in bpy.data.objects:
        if o.name.startswith(('UCX_', 'SOCKET_')):
            o.hide_render = True


def solo(objs):
    keep = set(objs) | {bpy.data.objects.get('_Floor'), bpy.data.objects.get('_Wall')}
    for o in bpy.data.objects:
        if o.type == 'MESH' and not o.name.startswith('UCX_'):
            o.hide_render = o not in keep


def shot(scene, name, eye, target, lens, size=(1500, 1000)):
    cam = lp._camera('_Cam_' + name, eye, target, lens, 'HORIZONTAL')
    return lp._render(scene, cam, os.path.join(SHOTS, name + '.png'), size)


def build_models():
    path = os.path.join(REPO, 'Art', 'Models', 'Props', 'MissionProps.py')
    ns = {'__name__': 'seeds_sheet', '__file__': path}
    saved = sys.argv[:]
    sys.argv = [sys.argv[0], '--']
    try:
        with open(path, encoding='utf-8') as f:
            exec(compile(f.read(), path, 'exec'), ns)
        made = {name: ns[fn]() for name, fn in (('LedgerPage', 'ledger_page'), ('DeaconLetter', 'deacon_letter'),
                                                  ('PostRack', 'post_rack'), ('ReachesMap', 'reaches_map'))}
        # A second letter for the rack's slot 3.
        made['SlotLetter'] = ns['deacon_letter']()
    finally:
        sys.argv = saved
    return made, ns


# --- Compositing the sheets ---

def load(path):
    return lp._pixels(path)


def resize(img, w):
    h = int(round(img.shape[0] * w / img.shape[1]))
    k = img.shape[1] / w
    if k > 1.0:
        img = lt.blur(img, 0.45 * k, 0.45 * k)
    ys = (np.arange(h) + 0.5) * img.shape[0] / h - 0.5
    xs = (np.arange(w) + 0.5) * img.shape[1] / w - 0.5
    gx, gy = np.meshgrid(xs, ys)
    return np.clip(lt.sample(img, gx, gy), 0.0, 1.0)


def compose(out_jpg, title, subtitle, tiles, columns=2):
    """tiles: (image (h, w, 3) sRGB 0..1, caption, span 1 or columns)."""
    inner = SHEET_W - 2 * PAD
    tile_w = (inner - GAP * (columns - 1)) // columns
    blocks, y = [], PAD + 70 + 44 * len(subtitle) + 20
    col, row_h = 0, 0
    for img, caption, span in tiles:
        w = inner if span > 1 else tile_w
        if span > 1 and col:
            y += row_h + 30
            col, row_h = 0, 0
        img = resize(img, w)
        x = PAD + col * (tile_w + GAP)
        lines = caption.split('\n')
        blocks.append((img, x, y, lines))
        row_h = max(row_h, img.shape[0] + 16 + 34 * len(lines))
        col += span
        if col >= columns:
            y += row_h + 30
            col, row_h = 0, 0
    if col:
        y += row_h + 30
    canvas = np.ones((int(y + PAD - 30), SHEET_W, 3), np.float32) * lt.rgb(GREY)
    lp.sheet_label(canvas, title, PAD, PAD + 52, 46, color=(0.1, 0.1, 0.1))
    for k, line in enumerate(subtitle):
        lp.sheet_label(canvas, line, PAD, PAD + 104 + 44 * k, 22, color=(0.16, 0.16, 0.16))
    for img, x, y0, lines in blocks:
        canvas[y0:y0 + img.shape[0], x:x + img.shape[1]] = img
        for k, line in enumerate(lines):
            lp.sheet_label(canvas, line, x, y0 + img.shape[0] + 34 + 34 * k, 20, color=(0.08, 0.08, 0.08))
    write_jpg(out_jpg, canvas)


def write_jpg(path, rgb):
    h, w = rgb.shape[:2]
    img = bpy.data.images.new('_sheet', w, h, alpha=False)
    rgba = np.concatenate([rgb[::-1], np.ones((h, w, 1), np.float32)], axis=-1)
    img.pixels.foreach_set(rgba.ravel())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.filepath_raw = path
    img.file_format = 'JPEG'
    bpy.context.scene.render.image_settings.quality = 90
    img.save()
    bpy.data.images.remove(img)
    lt._log(f'seeds sheet: {path} ({w} x {h})')


def atlas_cell(name, scale=3):
    rgba = lt_rgba(ATLAS)
    x0, y0, x1, y1 = lp.CELLS[name]
    cell = rgba[y0:y1, x0:x1]
    alpha = cell[..., 3:]
    img = cell[..., :3] * alpha + lt.rgb(GREY) * (1.0 - alpha)
    img = np.repeat(np.repeat(img, scale, axis=0), scale, axis=1)
    if name == 'SevenCross':                    # small on a grey card the width of a tile
        h, w = img.shape[:2]
        card = np.ones((int(h * 1.6), int(w * 2.0), 3), np.float32) * lt.rgb(GREY)
        y0, x0 = (card.shape[0] - h) // 2, (card.shape[1] - w) // 2
        card[y0:y0 + h, x0:x0 + w] = img
        img = card
    return img


def lt_rgba(path):
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = 'Non-Color'
    w, h = img.size
    px = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)[::-1].copy()


# --- The three sheets ---

def sheet_pages(scene, made, ns):
    page, letter, rack, slot_letter = made['LedgerPage'], made['DeaconLetter'], made['PostRack'], made['SlotLetter']
    for o in (page, letter, slot_letter):
        o.location = (0.0, 0.0, 0.0)
    rack.location = (0.0, 0.0, 1.4)
    page.location = (-1.5, -1.0, 0.0)
    letter.location = (-1.1, -1.0, 0.0)
    bpy.context.view_layer.update()
    # The letter at the rack's slot 3: its bottom edge on the socket, standing up the socket's +Z, its address out.
    slot_letter.matrix_world = ns['socket_matrix'](rack, 'Slot_3') @ Matrix.Rotation(math.radians(90.0), 4, 'X') @ \
        Matrix.Translation((0.0, 0.09, 0.0))
    bpy.context.view_layer.update()
    tiles = []
    solo([page])
    p = Vector((-1.5, -1.0, 0.0))
    tiles.append((load(shot(scene, 'LedgerPage', p + Vector((-0.17, -0.36, 0.3)), p + Vector((0.0, 0.0, 0.01)), 50)),
                  'SM_LedgerPage: 20 x 32 cm, one corner lifted\nblue rules, red columns, his copperplate', 1))
    tiles.append((load(shot(scene, 'LedgerPage_Top', p + Vector((0.0, -0.04, 0.56)), p, 50)),
                  'From above: heading, ten entries, sums,\nthe slantwise margin note', 1))
    solo([letter])
    q = Vector((-1.1, -1.0, 0.0))
    tiles.append((load(shot(scene, 'DeaconLetter', q + Vector((-0.1, -0.2, 0.17)), q + Vector((0.0, 0.0, 0.004)), 50)),
                  "SM_DeaconLetter: 11 x 18 cm, folded in three,\nsealed in red wax, addressed in his hand", 1))
    solo([page, letter])
    m = (p + q) * 0.5
    tiles.append((load(shot(scene, 'PageAndLetter', m + Vector((-0.06, -0.66, 0.6)), m + Vector((0.0, 0.0, 0.0)), 50)),
                  'Both at one scale (pivots: the middle of\nthe footprint, lying flat)', 1))
    solo([rack, slot_letter])
    r = Vector((0.0, 0.0, 1.4))
    tiles.append((load(shot(scene, 'PostRack', r + Vector((-0.45, -1.3, 0.05)), r, 50, size=(1520, 1000))),
                  "SM_PostRack: 60 x 42 cm, eight pigeonholes; the Deacon's letter at SOCKET_Slot_3,\n"
                  'two old letters baked into slots 6 and 8. Pivot: the middle of its back face', 2))
    tiles.append((load(shot(scene, 'PostRack_Slot3', r + Vector((0.2, -0.45, 0.2)), r + Vector((0.07, 0.0, 0.08)), 50)),
                  'Slot 3: the letter leans back 8 degrees,\nits address over the slat', 1))
    tiles.append((load(shot(scene, 'PostRack_Side', r + Vector((-0.9, -0.55, 0.15)), r, 50)),
                  'From the side: 7.5 cm deep, nailed\nthrough its back to the wall', 1))
    compose(os.path.join(OUT, '1_PagesLetters.jpg'), "Seeds: the page, the letter, the rack",
            ['Blender renders in neutral light; nothing imported yet. Paper: the slip\'s SextonLedgerPages; ink and',
             'rules: PaintBlack, ClothRed, ClothBlue; the rack: WoodPlanks pine. No new textures.'], tiles)


def sheet_map(scene, made):
    mp = made['ReachesMap']
    mp.location = (0.0, 0.0, 1.55)
    bpy.context.view_layer.update()
    solo([mp])
    c = Vector((0.0, 0.0, 1.55))
    tiles = [(load(shot(scene, 'ReachesMap', c + Vector((0.0, -1.25, 0.0)), c, 45, size=(1520, 1000))),
              'SM_ReachesMap on a wall: 90 x 60 cm, its own texture (MI_ReachesMap, M_World), four brass pins.\n'
              'Pivot: the middle of its back. North up; the Rim and the empty west on the left', 2),
             (load(shot(scene, 'ReachesMap_Close', c + Vector((-0.3, -0.38, -0.12)), c + Vector((-0.12, 0.0, -0.1)), 45)),
              "Close: the Rest on the Rim, the lines' red\nand crossties, Rye names", 1),
             (load(shot(scene, 'ReachesMap_Side', c + Vector((-1.05, -0.5, -0.3)), c + Vector((-0.1, 0.0, -0.1)), 45)),
              'Raking: proud of the wall between the pins,\nthe bottom edge curling out', 1)]
    flat = load(MAP_FLAT)
    tiles.append((flat, 'T_ReachesMap_BC as painted (2048 x 1366; stored 2048 x 1024): Halfway Junction at the heart,\n'
                        'Highwater Landing at the end of the line, seven towns, W. Pike, Surveyor', 2))
    compose(os.path.join(OUT, '2_ReachesMap.jpg'), 'Seeds: the map of the Reaches',
            ['Painted by Art/Textures/ReachesMap/make_reaches_map.py (numpy, Rye, the posters\' paper and pens);',
             'BC, N (folds, grain) and ORM in Art/Textures/ReachesMap, imported with the model.'], tiles)


def sheet_poster(scene):
    path = os.path.join(REPO, 'Art', 'Models', 'Props', 'Boardwalk.py')
    board = lp._exec_models(path, [('NoticeBoard', 'notice_board', ('NoticeBoard', 509))])['NoticeBoard']
    board.location = (3.0, -0.4, 0.0)
    bpy.context.view_layer.update()
    image = bpy.data.images.load(ATLAS, check_existing=False)
    image.colorspace_settings.name = 'sRGB'
    image.alpha_mode = 'STRAIGHT'
    mat = lp._decal_material('_PostersSheet', image)
    y, z = -0.4 - 0.081, 1.36
    decals = [lp._decal('_EllisPoster', 'Poster', mat, (3.0 - 0.36, y, z), turn=-0.8),
              lp._decal('_Seven', 'Seven', mat, (3.0 + 0.12, y, z + 0.02), lift=0.0014, turn=0.6),
              lp._decal('_Note', 'Note', mat, (3.0 + 0.42, y, z + 0.09), lift=0.0016, turn=1.5)]
    # A paid face: the SevenCross cell, scaled to the oval, over the Deacon.
    x0, y0, x1, y1 = lp.CELLS['Seven']
    sheet = lp.Sheet('Seven', lp.SEVEN_MM)
    _, (cx, cy), _ = lp.SEVEN[0]
    px, py = sheet.px((cx, cy))
    dx, dz = (px - (x1 - x0) * 0.5) / lp.PX_PER_M, -(py - (y1 - y0) * 0.5) / lp.PX_PER_M
    cross = lp._decal('_Cross', 'SevenCross', mat, (0.0, 0.0, 0.0), lift=0.0)
    cross.scale = (0.42, 1.0, 0.42)
    cross.location = (3.0 + 0.12 + dx, y - 0.0018, z + 0.02 + dz)
    bpy.context.view_layer.update()
    solo([board] + decals + [cross])
    b = Vector((3.0, -0.4, 1.36))
    tiles = [(load(shot(scene, 'NoticeBoard_Seven', b + Vector((-0.5, -2.6, 0.25)), b + Vector((0.0, 0.0, 0.05)), 35,
                        size=(1520, 1000))),
              "The Rangers' board: Ellis's poster, THE SEVEN in the free strip, Calder's note (decals, one atlas).\n"
              "The Deacon's face crossed out with the SevenCross cell, as when his ember is paid", 2),
             (load(shot(scene, 'Seven_Close', b + Vector((0.12, -0.75, 0.04)), b + Vector((0.12, 0.0, 0.02)), 50,
                        size=(1000, 1100))),
              'At reading distance: the busts tell\nthe seven apart by hat and outline', 1),
             (atlas_cell('Seven', 2), 'The cell, U 0.75-1.0, V 0.703-1.0:\n256 x 304 px, a 23 x 28 cm sheet', 1),
             (atlas_cell('SevenCross', 4), 'SevenCross, U 0.25-0.359, V 0.891-1.0:\nan ink X on nothing (alpha)', 1)]
    compose(os.path.join(OUT, '3_SevenPoster.jpg'), 'Seeds: THE SEVEN, on the Rangers\' notice board',
            ['The new cells of T_Posters_BC (Tools/Blender/looter_posters.py, option B as installed; every other',
             'cell unchanged): preacher hat, bowler, tall hat and whistle, veil, flat cap and apron, spectacles, bulk.'],
            tiles)


def main():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    made, ns = build_models()
    hide_helpers()
    scene, _ = studio()
    os.makedirs(SHOTS, exist_ok=True)
    sheet_pages(scene, made, ns)
    sheet_map(scene, made)
    sheet_poster(scene)
    hide_helpers()


if __name__ == '__main__':
    main()
