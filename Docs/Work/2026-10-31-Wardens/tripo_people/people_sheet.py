"""One sheet of the six fitted story people (the three-quarter panel of each <Name>_rest.png) beside a crop of the game
(Saved/Screenshots/HudShots/01_calm.png) and the Tripo Unpaid, for judging the look side by side.
Blender: artrun.ps1 -Script <this>  ->  Saved/ArtPreviews/TripoPeople/people_sheet.png"""
import os

import bpy
import numpy as np

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'TripoPeople')
W, H = 560, 900


def load(path):
    im = bpy.data.images.load(path)
    w, h = im.size
    a = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(a)
    return a.reshape(h, w, 4)          # rows bottom first


tiles = []
for name in ('Abel', 'Foreman', 'Amos', 'Hiram', 'Thaddeus', 'DeadMiner'):
    a = load(os.path.join(OUT, f'{name}_rest.png'))
    tiles.append(a[:, 3 * W:4 * W])
game = load(os.path.join(REPO, 'Saved', 'Screenshots', 'HudShots', '01_calm.png'))   # 1920 x 1080
h0 = (1080 - H) // 2
tiles.append(game[h0:h0 + H, 560:560 + W])
unpaid = load(os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Unpaid', 'UnpaidTripo_vs_concept.png'))
uh, uw = unpaid.shape[:2]
y0, x0 = max(0, (uh - H) // 2), 480
crop = unpaid[y0:y0 + H, x0:x0 + W]
pad = np.ones((H, W, 4), np.float32)
pad[:crop.shape[0], :crop.shape[1]] = crop
tiles.append(pad)
sheet = np.concatenate(tiles, axis=1)
img = bpy.data.images.new('sheet', sheet.shape[1], sheet.shape[0])
img.pixels.foreach_set(sheet.ravel())
img.filepath_raw = os.path.join(OUT, 'people_sheet.png')
img.file_format = 'PNG'
img.save()
print('SHEET wrote', img.filepath_raw)
