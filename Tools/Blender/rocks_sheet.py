"""Look sheet 45, "Rocks reworked" (the cliff review, 2026-10-14): the outcrop kit (Art/Models/Rocks/Outcrops.py) before
and after its rework into weathered sandstone forms, for the user's phone:
Saved/ArtPreviews/RansomsRest/LookSheets/45_Rocks_reworked.jpg, about 2000 px wide, every model labelled with its name
and triangles.

It reads the renders Outcrops.py writes with -Preview (Saved/ArtPreviews/RansomsRest/Rocks/Outcrop_<piece>.png and
_B.png, the kit at one scale in Rocks_kit.png) and the first kit's, kept in Saved/ArtPreviews/RansomsRest/RocksBefore
(copied there before the rework); each tile is the piece's render cropped to the rock, before beside after (a new piece:
its front beside its back). The triangles come from the renders' report (Outcrops_report.json, beside them).

    Tools\\artrun.ps1 -Script Art\\Models\\Rocks\\Outcrops.py -Preview -ScriptArgs TorA,TorB,...   (the renders)
    Tools\\artrun.ps1 -Script Tools\\Blender\\rocks_sheet.py
"""
import json
import os

import bpy
import numpy as np

import looter_homes as homes
import looter_textures as lt

AFTER = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Rocks')
BEFORE = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'RocksBefore')
TILES_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'RocksSheet')
SHEET = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets', '45_Rocks_reworked.jpg')
REPORT = os.path.join(AFTER, 'Outcrops_report.json')
TILE_H = 470
# (piece, where the level stands it, what changed). The pieces the level places keep their names.
PIECES = [
    ('TorA', 'Rimrock, bench tors, the bluff top\'s 3 m cover', 'Three worn pillows of the beds, undercut at the\n'
     'soft ones, one cleft: no box stack, no slits.'),
    ('TorB', 'Keeper\'s Gate north, Hearse Rock\'s head, bench', 'Two pillars parted by one natural cleft, a cap rock\n'
     'over the taller; no grid of joints.'),
    ('TorD', 'Keeper\'s Gate south, Rimrock north, bench cover', 'A tor split along one joint, its outer half\n'
     'slumped away: replaces the leaning dominoes.'),
    ('Spine', 'Rimrock middle, Hearse Rock, bench', 'A whaleback whose crest rises and falls, a soft\nbed undercut '
     'along its foot, hollows.'),
    ('Slab', 'The Three Widows, bench', 'One tapering monolith, waisted, a rounded hooded\nhead; still leans 14 degrees.'),
    ('Fin', 'Stone Teeth', 'A thin fin with a stepped crest, rain flutes and\nhollows instead of stacked blocks.'),
    ('TorC', 'Larkspur Bench (one knob, by the remap)','A leaning stack, its ledges rounded, the top\noverhanging.'),
    ('FinB', 'NEW: a Stone Tooth', 'A thicker fin, two summits and a window worn\nthrough it.'),
    ('HoodooA', 'NEW: a Stone Tooth, bench', 'A hoodoo on a flared foot, pinched at the soft\nbeds, under a cap rock.'),
    ('HoodooB', 'NEW: a Stone Tooth, bench', 'Twin hoodoos from one foot, each under its own\ncap rock.'),
]


def load(path):
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = 'Non-Color'
    w, h = img.size
    a = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)       # rows bottom first


def crop(a, margin=24):
    """The render cut to what isn't its plain backdrop (the rock, its shadow, the figure), a little square-ish."""
    h, w = a.shape[:2]
    # The backdrop row by row (its ground darkens toward the horizon, which a view from low crosses): each row's own
    # colour at its two ends.
    ground = np.median(np.concatenate([a[:, :24, :3], a[:, -24:, :3]], axis=1), axis=1)
    differs = np.abs(a[..., :3] - ground[:, None, :]).max(axis=-1) > 0.06
    rows, cols = np.nonzero(differs.any(axis=1))[0], np.nonzero(differs.any(axis=0))[0]
    if not len(rows):
        return a
    r0, r1 = max(rows[0] - margin, 0), min(rows[-1] + margin, h - 1)
    c0, c1 = max(cols[0] - margin, 0), min(cols[-1] + margin, w - 1)
    # No narrower than three quarters of its height (a tall hoodoo keeps some ground either side).
    want = int((r1 - r0) * 0.75)
    if c1 - c0 < want:
        grow = (want - (c1 - c0)) // 2
        c0, c1 = max(c0 - grow, 0), min(c1 + grow, w - 1)
    return a[r0:r1 + 1, c0:c1 + 1]


def scaled(a, height):
    """a resized to height px (Blender's own scaling)."""
    h, w = a.shape[:2]
    img = bpy.data.images.new('_scale', w, h, alpha=True)
    img.colorspace_settings.name = 'Non-Color'
    img.pixels.foreach_set(np.ascontiguousarray(a).ravel())
    width = max(int(round(w * height / h)), 1)
    img.scale(width, height)
    out = np.empty(width * height * 4, np.float32)
    img.pixels.foreach_get(out)
    bpy.data.images.remove(img)
    return out.reshape(height, width, 4)


def save(a, path):
    img = bpy.data.images.new('_tile', a.shape[1], a.shape[0], alpha=True)
    img.colorspace_settings.name = 'Non-Color'
    img.pixels.foreach_set(np.ascontiguousarray(a).ravel())
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)


def pair(left, right):
    """Two renders cropped, scaled to TILE_H and set side by side on the sheets' dark ground."""
    parts = [scaled(crop(load(p)), TILE_H) for p in (left, right)]
    gap = np.zeros((TILE_H, 14, 4), np.float32)
    gap[..., :3] = np.array([0x16, 0x17, 0x1a], np.float32) / 255.0
    gap[..., 3] = 1.0
    return np.concatenate([parts[0], gap, parts[1]], axis=1)


def main():
    os.makedirs(TILES_DIR, exist_ok=True)
    tris = {}
    if os.path.exists(REPORT):
        with open(REPORT, encoding='utf-8') as f:
            tris = {name: info['triangles'] for name, info in json.load(f).items()}
    tiles = []
    for piece, where, caption in PIECES:
        name = f'Outcrop_{piece}'
        after = os.path.join(AFTER, name + '.png')
        before = os.path.join(BEFORE, name + '.png')
        if not os.path.exists(after):
            print(f'LOOTER: warning: no {after}: run Outcrops.py with -Preview first', flush=True)
            continue
        if os.path.exists(before):
            image, view = pair(before, after), 'before | after'
        else:
            image, view = pair(after, os.path.join(AFTER, name + '_B.png')), 'front | back'
        path = os.path.join(TILES_DIR, name + '.png')
        save(image, path)
        count = tris.get(name)
        label = f'{name}  ({view})' + (f'  {count:,} tris' if count else '')
        tiles.append((path, label, f'{where}.\n{caption}'))
    kit = os.path.join(AFTER, 'Rocks_kit.png')
    if os.path.exists(kit):
        cropped = os.path.join(TILES_DIR, 'Rocks_kit.png')
        save(crop(load(kit), margin=30), cropped)
        kit = cropped
    banner = (kit,'The reworked kit at one scale (the blue post is 1.8 m). Seen from every side; each sinks 0.9 m\n'
                   'below its pivot to sit on a slope. One material slot (RockCliff), 7-11k triangles, Nanite, a\n'
                   'fallback of about 2.5k.') if os.path.exists(kit) else None
    homes.look_sheet(SHEET, "Ransom's Rest: rocks reworked",
                     'The outcrop kit as weathered sandstone: rounded beds, undercut soft layers, caps,\n'
                     'clefts and wind hollows instead of stacked blocks with regular slits. Before the rework\n'
                     '(left of each pair) and after (right); three new pieces vary the Stone Teeth and the bench.',
                     banner, tiles, columns=1 if len(tiles) < 4 else 2)


main()
