"""The map of the Reaches on the depot's ticket-office wall (Docs/Story.md, "Seeds in Ransom's Rest"; SM_ReachesMap in
Art/Models/Props/MissionProps.py), painted by numpy like the posters (Tools/Blender/looter_posters.py, whose type,
pen strokes and paper wear it uses), the same bytes on every run.

The sheet is 0.90 x 0.60 m. It's painted at 2048 x 1366 px (2.28 px/mm, square pixels) and stored at 2048 x 1024
(both sides powers of two, so Unreal mips and streams it): the mesh's UVs stretch it back, 2.28 px/mm across and 1.71
down. Writes T_ReachesMap_BC.png (sRGB), _N.png (DirectX: the folds and the paper's grain) and _ORM.png (paper rough,
the ink a little smoother) here; the model's material MI_ReachesMap (master M_World) takes the set, and
Looter.ImportModels imports the three textures with the model.

What's on it, as a railroad's map of its country is drawn: west is left. Down the left edge runs the Rim, the
escarpment hachured toward the empty plains past it (UNSURVEYED: nobody settles the land the dead cross). The lines
(red, black crossties) run from Halfway Junction at the heart: west to a branch for Highwater Landing on the Rim to the
north-west (END OF THE LINE) and on to Ransom's Rest on the Rim, with a branch south to Dustwater Mesa among its mesas;
north to the Needles' spires; north-east to Stoneweight Quarry's terraces; east to Furnace Hollow, with a branch south
to Saint Agnes Mire in the river's delta. Stage roads dotted, a river in blue ink, hills, a compass, a scale in miles,
the title THE REACHES / RAILROAD LINES AND STATIONS, and the surveyor's credit W. PIKE, SURVEYOR (Wendell Pike, who
walked every mile of the line before the gang left him in a Dustwater ravine). Names in Rye (Art/Fonts/Rye-Regular.ttf),
the only face. The pins' rust is painted at the four corners where SM_ReachesMap's brass pins go (14 mm in).

    Tools\\artrun.ps1 -Script Art\\Textures\\ReachesMap\\make_reaches_map.py
also writes Saved/ArtPreviews/Seeds/ReachesMap_flat.png (the colour map at its painted size).
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_posters as lp  # noqa: E402 - needs the path above
import looter_textures as lt  # noqa: E402

SHEET_MM = (900.0, 600.0)
PAINT = (1366, 2048)                    # painted (rows, columns): square pixels
STORE = (1024, 2048)                    # stored
PX = PAINT[1] / SHEET_MM[0]             # px per mm
PREVIEW = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Seeds', 'ReachesMap_flat.png')
PINS = [(14.0, 14.0), (886.0, 14.0), (14.0, 586.0), (886.0, 586.0)]
INSET = 22.0                            # the neatline's inset from the sheet's edge (mm)

rgb, smooth, mix, multiply = lt.rgb, lt.smooth, lt.mix, lp.multiply
INK = (0.19, 0.155, 0.13)               # iron-gall black-brown
RED = (0.66, 0.24, 0.17)                # the lines' red
BLUE = (0.36, 0.47, 0.6)                # the river's ink
SEPIA = (0.42, 0.31, 0.21)              # hills and hachures


class MapSheet:
    """What looter_posters' type needs of a sheet: its shape and mm -> px."""
    shape = PAINT

    @staticmethod
    def px(pts):
        return np.asarray(pts, np.float64) * PX


SHEET = MapSheet()

# --- The network (mm, y down): Halfway Junction at the heart, branches where lines part ---

J = (470.0, 318.0)
B_WEST = (390.0, 335.0)                 # the west line parts for Highwater Landing
B_REST = (330.0, 350.0)                 # ... and for Dustwater Mesa
B_EAST = (610.0, 312.0)                 # the east line parts for Saint Agnes Mire
LINES = [
    [J, (430.0, 326.0), B_WEST],
    [B_WEST, (335.0, 285.0), (262.0, 215.0), (228.0, 165.0), (215.0, 118.0)],      # Highwater Landing
    [B_WEST, B_REST, (260.0, 368.0), (178.0, 382.0)],                              # Ransom's Rest
    [B_REST, (318.0, 400.0), (322.0, 450.0), (330.0, 490.0)],                      # Dustwater Mesa
    [J, (466.0, 260.0), (448.0, 190.0), (430.0, 130.0)],                           # the Needles
    [J, (520.0, 270.0), (600.0, 225.0), (680.0, 192.0), (745.0, 175.0)],           # Stoneweight Quarry
    [J, (540.0, 316.0), B_EAST, (690.0, 300.0), (770.0, 305.0)],                   # Furnace Hollow
    [B_EAST, (632.0, 360.0), (650.0, 405.0), (665.0, 445.0)],                      # Saint Agnes Mire
]
# The towns: (name, the station, the label's baseline anchor, alignment, cap height mm, small line under it).
TOWNS = [
    ("RANSOM'S REST", (178.0, 382.0), (166.0, 407.0), 'left', 10.5, None),
    ('HIGHWATER LANDING', (215.0, 118.0), (231.0, 114.0), 'left', 10.0, 'END OF THE LINE'),
    ('DUSTWATER MESA', (330.0, 490.0), (330.0, 514.0), 'center', 10.0, None),
    ('THE NEEDLES', (430.0, 130.0), (443.0, 134.0), 'left', 10.0, None),
    ('STONEWEIGHT QUARRY', (745.0, 175.0), (752.0, 160.0), 'center', 10.0, None),
    ('FURNACE HOLLOW', (770.0, 305.0), (770.0, 330.0), 'center', 10.0, None),
    ('SAINT AGNES MIRE', (665.0, 445.0), (676.0, 470.0), 'center', 10.0, None),
]
STAGE_ROADS = [[(178.0, 382.0), (215.0, 430.0), (262.0, 468.0), (330.0, 490.0)],
               [(770.0, 305.0), (792.0, 250.0), (775.0, 205.0), (745.0, 175.0)]]
RIM = [(154.0, 4.0), (148.0, 60.0), (157.0, 120.0), (144.0, 190.0), (150.0, 255.0), (139.0, 320.0), (147.0, 385.0),
       (136.0, 450.0), (145.0, 515.0), (133.0, 596.0)]
RIVER = [(660.0, 128.0), (643.0, 186.0), (662.0, 238.0), (690.0, 284.0), (706.0, 344.0), (700.0, 404.0), (712.0, 452.0)]
DELTA = [[(712.0, 452.0), (700.0, 492.0), (690.0, 530.0)], [(712.0, 452.0), (730.0, 495.0), (732.0, 528.0)],
         [(712.0, 452.0), (752.0, 480.0), (782.0, 515.0)]]
CREEKS = [[(610.0, 150.0), (628.0, 190.0), (650.0, 222.0), (662.0, 238.0)],
          [(760.0, 360.0), (735.0, 385.0), (704.0, 400.0)]]


def path(points, per=12):
    """A hand-drawn line through points (mm): a smooth curve, as dense polyline in px."""
    return lp.spline(np.asarray(points, np.float64) * PX, per=per)


def strokes_along(points_px, rng, width_mm, wobble=0.004):
    return [lp.hand_stroke(points_px, rng, width_mm * PX * 6.0, wobble=wobble, overshoot=(0.0, 0.0), base=1.0,
                           attack=0.02, release=0.04, lift=0.85, start=0.9)]


def draw(layer, strokes, width_mm, var=0.25, rough=0.05, seed=0):
    cov, _ = lp.draw_strokes(PAINT, strokes, width_mm * PX, var=var, rough=rough, seed=seed)
    np.maximum(layer, cov, out=layer)


def ticks(points_px, every_mm, length_mm):
    """Short strokes across a line every so often (a railroad's crossties): straight segments in px."""
    pts, total = lp._resample(points_px, every_mm * PX)
    out = []
    for i in range(1, len(pts) - 1):
        d = pts[i + 1] - pts[i - 1]
        d /= max(np.hypot(*d), 1e-9)
        n = np.array([-d[1], d[0]]) * length_mm * PX * 0.5
        out.append((np.array([pts[i] - n, pts[i] + n]), np.ones(2)))
    return out


def dashes(points_px, dash_mm, gap_mm):
    pts, _ = lp._resample(points_px, 0.5 * PX)
    period = int(round((dash_mm + gap_mm) / 0.5))
    on = int(round(dash_mm / 0.5))
    return [(pts[i:i + on + 1], np.ones(len(pts[i:i + on + 1]))) for i in range(0, len(pts) - 1, period)
            if len(pts[i:i + on + 1]) > 1]


# --- Map symbols (strokes in px) ---

def caret(cx, cy, w, h, rng, shade=3):
    """A hill: a peak and a few strokes down its shaded east side."""
    j = lambda: rng.normal(0.0, 0.25)
    peak = (cx + j(), cy - h)
    out = [lp.spline(np.array([(cx - w + j(), cy), (cx - w * 0.4, cy - h * 0.6 + j()), peak, (cx + w * 0.45, cy - h * 0.55),
                               (cx + w + j(), cy)]) * PX, per=5)]
    for k in range(shade):
        t = (k + 1) / (shade + 1)
        a = (peak[0] + w * 0.45 * t, peak[1] + h * 0.5 * t)
        out.append(np.array([a, (a[0] + w * 0.35 * (1.0 - t * 0.5), cy - 0.4)]) * PX)
    return [(p, np.ones(len(p))) for p in out]


def spire(cx, cy, w, h, rng):
    j = lambda: rng.normal(0.0, 0.2)
    out = [np.array([(cx - w + j(), cy), (cx - w * 0.3, cy - h * 0.7), (cx + j() * 0.3, cy - h),
                     (cx + w * 0.3, cy - h * 0.7), (cx + w + j(), cy)]) * PX,
           np.array([(cx + w * 0.15, cy - h * 0.6), (cx + w * 0.55, cy - 0.5)]) * PX]
    return [(p, np.ones(len(p))) for p in out]


def mesa(cx, cy, w, h, rng):
    j = lambda: rng.normal(0.0, 0.25)
    top = w * 0.62
    out = [np.array([(cx - w + j(), cy), (cx - top, cy - h + j() * 0.3), (cx + top, cy - h + j() * 0.3),
                     (cx + w + j(), cy)]) * PX]
    for k in range(4):                                  # its face hatched
        x = cx - top * 0.6 + top * 1.4 * k / 3.0
        out.append(np.array([(x, cy - h + 1.2), (x + 0.6, cy - 0.6)]) * PX)
    return [(p, np.ones(len(p))) for p in out]


def tuft(cx, cy, s, rng):
    """A marsh tuft: a short base and three blades fanning up."""
    out = [np.array([(cx - s, cy), (cx + s, cy)]) * PX]
    for a in (-0.5, 0.0, 0.5):
        a = a + rng.normal(0.0, 0.08)
        out.append(np.array([(cx + a * s * 0.6, cy - 0.2), (cx + math.sin(a) * s * 1.4, cy - s * 1.3 * math.cos(a))]) * PX)
    return [(p, np.ones(len(p))) for p in out]


def terraces(cx, cy, rng):
    """A quarry: stepped terraces cut into a hillside."""
    out = []
    for k in range(4):
        y = cy + k * 3.2
        x0, x1 = cx - 4.0 - k * 2.5, cx + 6.0 + k * 2.5
        out.append(np.array([(x0, y), (x1, y), (x1 + 1.2, y + 2.2)]) * PX)
    return [(p, np.ones(len(p))) for p in out]


def ring(cx, cy, r):
    return lp.arc(cx * PX, cy * PX, r * PX, r * PX, 0.0, 360.0, step=10.0)


# --- Type ---

def label(ink, rng, text, cap, x, y, align='center', max_w=None, tracking=0.0):
    _, natural = lp.set_line(text, cap, tracking=tracking)
    width = min(natural, max_w) if max_w else None
    return lp.type_line(ink, SHEET, rng, text, cap, x, y, width=width, tracking=tracking, align=align, turn=0.12,
                        wobble=0.1)


def stacked(ink, rng, text, cap, x, y0, step):
    """Letters set one under another down a feature (THE RIM)."""
    for k, ch in enumerate(text):
        if ch != ' ':
            lp.type_line(ink, SHEET, rng, ch, cap, x, y0 + step * k, turn=0.3, wobble=0.1)


# --- The paper ---

def paper(seed):
    shape = PAINT
    rng = np.random.default_rng(seed)
    x, y = lt.coords(shape)
    u, v = (x + 0.5) / PX, (y + 0.5) / PX
    W, H = SHEET_MM
    edge = np.minimum(np.minimum(u, W - u), np.minimum(v, H - v))
    big = lp._noise(shape, seed + 1, 160.0, octaves=2)
    mid = lp._noise(shape, seed + 2, 30.0)
    fine = lp._noise(shape, seed + 3, 0.7)
    fibers = 0.55 * lp._noise(shape, seed + 4, 6.0, 0.6) + 0.55 * lp._noise(shape, seed + 5, 0.6, 6.0)
    color = np.ones(shape + (3,), np.float32) * rgb(0xdccfae)
    # Faded where the window's light falls (the upper left), toned and grubby toward the edges.
    color = mix(color, rgb(0xe6dcc2), smooth(-0.8, 1.4, 0.6 * big - 0.002 * (u + v - 500.0)) * smooth(0.0, 40.0, edge) * 0.8)
    toned = (1.0 - smooth(0.0, 14.0, edge + 5.0 * mid)) * 0.7 + (1.0 - smooth(0.0, 70.0, edge)) * 0.22
    color = mix(color, rgb(0xb99a6a), np.clip(toned, 0.0, 1.0))
    color = color * (1.0 + 0.02 * big + 0.012 * mid + 0.02 * fine + 0.012 * fibers)[..., None]
    # A dried stain where the stove's kettle once spat, and a run from a pin.
    inside, tide = lp.tide_stain(u, v, (826.0, 420.0), 28.0, 1.3, seed + 21, shape)
    color = multiply(color, inside * 0.4, (0.95, 0.9, 0.78))
    color = mix(color, rgb(0x9a7b52), np.clip(tide, 0.0, 1.0) * 0.3)
    inside, tide = lp.drip(u, v, 14.0, 20.0, 110.0, seed + 23, width=2.0)
    color = multiply(color, inside * 0.4, (0.93, 0.88, 0.76))
    color = mix(color, rgb(0x9a7b52), np.clip(tide, 0.0, 1.0) * 0.25)
    # Foxing and fly specks.
    h, w = shape
    chance = ((1.0 - smooth(0.0, 90.0, edge)) * 0.6 + 0.3 * smooth(0.6, 1.8, mid) + 0.1).ravel()
    picks = rng.choice(h * w, size=260, replace=False, p=chance / chance.sum())
    pts = [(p % w + 0.5, p // w + 0.5) for p in picks]
    sizes = [float(rng.choice([0.7, 1.0, 1.6, 2.6], p=[0.45, 0.3, 0.17, 0.08])) for _ in picks]
    strengths = [float(rng.uniform(0.25, 0.75)) for _ in picks]
    cores, halos = lp.spots(shape, rng, pts, sizes, strengths)
    color = multiply(color, halos * 0.3, rgb(0xc9a070))
    color = multiply(color, cores * 0.75, rgb(0xa06a3e))
    # Folded in six to come out on the train: the creases' shading, and their height for the normal map.
    shade = np.zeros(shape, np.float32)
    cracks = np.zeros(shape, np.float32)
    height = np.zeros(shape, np.float32)
    for k, (p0, p1, s) in enumerate((((-10.0, 301.0), (910.0, 297.0), 1.0), ((301.0, -10.0), (298.0, 610.0), 0.8),
                                     ((601.0, -10.0), (603.0, 610.0), 0.8))):
        sh, core = lp.crease(u, v, p0, p1, seed + 41 + 2 * k, s)
        shade += sh
        cracks = np.maximum(cracks, core)
        height -= 0.00018 * core
    color = color * (1.0 + 0.08 * np.clip(shade, -1.5, 1.5))[..., None]
    color = multiply(color, cracks * 0.1, (0.6, 0.52, 0.42))
    # Rust round the pins.
    for k, (px_, py_) in enumerate(PINS):
        d = np.hypot(u - px_, v - py_)
        halo = np.exp(-((d - 3.2) / 2.2) ** 2) * (0.6 + 0.4 * smooth(-1.0, 1.0, lp._noise(shape, seed + 60 + k, 2.0)))
        run = smooth(1.6, 0.6, np.abs(u - px_ - 0.4 * np.sin(v * 0.2))) * smooth(py_, py_ + 2.0, v) * \
            smooth(py_ + 16.0, py_ + 4.0, v)
        color = multiply(color, np.clip(0.45 * halo + 0.35 * run, 0.0, 1.0), rgb(0x9a5a30))
    height += 0.00002 * fibers + 0.00001 * fine
    maps = dict(fibers=fibers, fine=fine, cracks=cracks, tooth=lp.tooth(shape, seed + 70))
    return np.clip(color, 0.0, 1.0), height, maps


# --- The map ---

def paint(seed=7):
    rng = np.random.default_rng(seed)
    color, height, maps = paper(seed)
    shape = PAINT
    black, red = lp.Ink(shape), lp.Ink(shape)
    rails = np.zeros(shape, np.float32)          # the lines' red
    ties = np.zeros(shape, np.float32)           # crossties, town rings: black pen
    sepia = np.zeros(shape, np.float32)          # hachures, hills, mesas, the quarry, marsh
    blue = np.zeros(shape, np.float32)           # water
    roads = np.zeros(shape, np.float32)
    # The Rim: its edge drawn firm, hachures falling west from it.
    rim = path(RIM, per=10)
    draw(sepia, strokes_along(rim, rng, 0.9), 0.9, seed=seed + 1)
    pts, _ = lp._resample(rim, 3.2 * PX)
    hach = []
    for i in range(1, len(pts) - 1):
        d = pts[i + 1] - pts[i - 1]
        d /= max(np.hypot(*d), 1e-9)
        n = np.array([d[1], -d[0]])              # toward the west (left of a line drawn down the page)
        if n[0] > 0.0:
            n = -n
        length = (4.0 + 5.0 * abs(lp.noise1d(1, rng, 1.0)[0]) + 2.0 * (i % 3 == 0)) * PX
        a = pts[i] + n * 0.6 * PX
        hach.append((np.array([a, a + n * length + d * rng.normal(0.0, 0.4) * PX]), np.array([1.0, 0.5])))
    draw(sepia, hach, 0.55, var=0.5, seed=seed + 2)
    # Water: the river from the hills to its delta in the south-east, two creeks into it.
    for k, (line, width) in enumerate([(RIVER, 1.2)] + [(d, 0.9) for d in DELTA] + [(c, 0.6) for c in CREEKS]):
        draw(blue, strokes_along(path(line), rng, width, wobble=0.01), width, var=0.3, seed=seed + 10 + k)
    # Terrain.
    terrain = []
    for k in range(12):                          # the Needles' spires
        terrain += spire(380.0 + 4.0 * k + rng.normal(0.0, 1.2), 146.0 - 4.0 * (k % 3) + rng.normal(0.0, 2.0),
                         2.0, rng.uniform(12.0, 22.0), rng)
    for cx, cy in ((520, 150), (545, 132), (572, 152), (600, 128), (628, 150), (560, 175), (505, 128), (842, 220),
                   (862, 248), (826, 398), (852, 380), (800, 420)):
        terrain += caret(float(cx), float(cy), rng.uniform(7.0, 10.0), rng.uniform(6.0, 9.0), rng)
    for cx, cy, w, h in ((268, 450, 10, 6), (282, 548, 13, 7), (384, 548, 9, 6), (398, 470, 11, 6), (250, 515, 6, 5),
                         (362, 455, 5, 4), (420, 515, 7, 5)):
        terrain += mesa(float(cx), float(cy), float(w), float(h), rng)
    for k in range(30):                          # the Mire's marsh round the delta
        cx = 632.0 + rng.uniform(0.0, 150.0)
        cy = 488.0 + rng.uniform(0.0, 44.0)
        terrain += tuft(cx, cy, rng.uniform(1.8, 2.6), rng)
    terrain += terraces(808.0, 180.0, rng)
    draw(sepia, terrain, 0.5, var=0.3, seed=seed + 30)
    # Stage roads, dotted.
    for k, road in enumerate(STAGE_ROADS):
        draw(roads, dashes(path(road), 2.4, 2.2), 0.55, var=0.0, seed=seed + 40 + k)
    # The lines: red, a firm hand, crossties every 6 mm.
    for k, line in enumerate(LINES):
        p = path(line, per=16)
        draw(rails, strokes_along(p, rng, 2.0, wobble=0.002), 2.0, var=0.15, rough=0.02, seed=seed + 50 + k)
        draw(ties, ticks(p, 6.0, 4.6), 0.55, var=0.0, seed=seed + 60 + k)
    # Stations: a ring with a dot; the Junction a double ring filled red.
    rings = []
    for name, (sx, sy), *_ in TOWNS:
        rings.append((ring(sx, sy, 3.2), np.ones(37)))
    rings.append((ring(*J, 6.2), np.ones(37)))
    rings.append((ring(*J, 8.4), np.ones(37)))
    draw(ties, rings, 0.9, var=0.0, seed=seed + 70)
    dots = np.zeros(shape, np.float32)
    for name, (sx, sy), *_ in TOWNS:
        lp.fill_into(dots, [ring(sx, sy, 1.7)])
    jfill = np.zeros(shape, np.float32)
    lp.fill_into(jfill, [ring(*J, 5.6)])
    lp.fill_into(dots, [lp.star(J[0] * PX, J[1] * PX, 3.6 * PX, 5, 0.45)])
    # The names: printed in Rye.
    for name, (sx, sy), (lx, ly), align, cap, sub in TOWNS:
        label(black, rng, name, cap, lx, ly, align, max_w=170.0)
        if sub:
            label(black, rng, sub, 5.5, lx + 1.0, ly + 12.0, align, tracking=0.6)
    label(black, rng, 'HALFWAY', 12.0, J[0], 352.0)
    label(black, rng, 'JUNCTION', 12.0, J[0], 368.0)
    stacked(black, rng, 'THE RIM', 7.0, 120.0, 214.0, 11.0)
    label(black, rng, 'UNSURVEYED', 5.0, 78.0, 158.0, tracking=1.6)
    # The title in its cartouche, top right.
    bx0, by0, bx1, by1 = 562.0, 32.0, 866.0, 104.0
    for inset, thick in ((0.0, 1.6), (2.8, 0.6)):
        a0, b0, a1, b1 = bx0 + inset, by0 + inset, bx1 - inset, by1 - inset
        lp.type_shape(black, SHEET, [lp.rect(a0, b0, a1, b0 + thick), lp.rect(a0, b1 - thick, a1, b1),
                                     lp.rect(a0, b0, a0 + thick, b1), lp.rect(a1 - thick, b0, a1, b1)])
    label(black, rng, 'THE REACHES', 28.0, (bx0 + bx1) * 0.5, 74.0, max_w=262.0)
    lp.rule(black, SHEET, bx0 + 40.0, bx1 - 40.0, 81.0, 0.7, rng)
    label(black, rng, 'RAILROAD LINES AND STATIONS', 7.0, (bx0 + bx1) * 0.5, 95.0, max_w=236.0)
    # The neatline: a heavy rule and a hairline, a scale of miles and the compass.
    W, H = SHEET_MM
    for inset, thick in ((INSET, 2.2), (INSET + 3.6, 0.7)):
        lp.rule(black, SHEET, inset - thick * 0.5, W - inset + thick * 0.5, inset, thick, rng)
        lp.rule(black, SHEET, inset - thick * 0.5, W - inset + thick * 0.5, H - inset, thick, rng)
        for x in (inset, W - inset):
            lp.type_shape(black, SHEET, [lp.rect(x - thick * 0.5, inset, x + thick * 0.5, H - inset)])
    sx0, sy = 712.0, 548.0
    for k in range(4):
        box = lp.rect(sx0 + 34.0 * k, sy - 1.4, sx0 + 34.0 * (k + 1), sy + 1.4)
        if k % 2 == 0:
            lp.type_shape(black, SHEET, [box])
        else:
            for yy in (sy - 1.4, sy + 1.4):
                lp.rule(black, SHEET, sx0 + 34.0 * k, sx0 + 34.0 * (k + 1), yy, 0.5, rng)
        label(black, rng, str(25 * k), 4.6, sx0 + 34.0 * k, sy - 4.0)
    label(black, rng, '100', 4.6, sx0 + 136.0, sy - 4.0)
    label(black, rng, 'MILES', 5.0, sx0 + 68.0, sy + 10.5, tracking=1.0)
    label(black, rng, 'W. PIKE, SURVEYOR', 5.0, W - INSET - 6.0, H - INSET - 6.5, align='right', tracking=0.5)
    cx, cy, r = 78.0, 500.0, 24.0
    lp.type_shape(black, SHEET, [lp.star(cx, cy, r, 4, 0.22)])
    lp.type_shape(red, SHEET, [lp.star(cx, cy, r * 0.62, 4, 0.3)])
    rose = np.zeros(shape, np.float32)
    draw(rose, [(ring(cx, cy, r * 0.8), np.ones(37)), (ring(cx, cy, r * 0.86), np.ones(37))], 0.5, var=0.0)
    label(black, rng, 'N', 9.0, cx, cy - r - 4.0)
    # Labels sit on clean paper: the line work stops short round every name (black ink's coverage, spread a little).
    knock = 1.0 - np.clip(lt.blur(black.cov, 2.2) * 6.0, 0.0, 1.0)
    for layer in (sepia, blue, roads, ties):
        layer *= knock
    rails *= 1.0 - np.clip(lt.blur(black.cov, 1.4) * 4.0, 0.0, 1.0)
    # Nothing drawn runs past the neatline.
    x, y = lt.coords(shape)
    u, v = (x + 0.5) / PX, (y + 0.5) / PX
    edge = INSET + 3.6 + 1.2
    inner = smooth(edge, edge + 0.6, np.minimum(np.minimum(u, W - u), np.minimum(v, H - v)))
    for layer in (sepia, blue, roads, ties, rails):
        layer *= inner
    # Lay it down: water and terrain, roads, the red lines, the black pen, the stations' red, then the press's black.
    grain = maps['tooth']
    color = multiply(color, blue * 0.85 * (0.85 + 0.15 * grain), BLUE)
    color = multiply(color, sepia * 0.85, SEPIA)
    color = multiply(color, roads * 0.8, INK)
    color = multiply(color, rails * 0.92 * (1.0 - 0.6 * maps['cracks']), RED)
    color = multiply(color, np.maximum(ties, rose) * 0.92, INK)
    color = multiply(color, jfill * 0.9, RED)
    color = multiply(color, dots * 0.95, INK)
    a = lp.letterpress(black, maps['fine'], maps['fibers'], seed + 400) * (1.0 - 0.7 * maps['cracks'])
    color = multiply(color, a, (0.17, 0.145, 0.13))
    a_red = lp.letterpress(red, maps['fine'], maps['fibers'], seed + 410, fade=0.85)
    color = multiply(color, a_red, RED)
    ink = np.clip(np.maximum.reduce([a, a_red, rails, ties, sepia * 0.8, blue * 0.8, dots, jfill]), 0.0, 1.0)
    return np.clip(color, 0.0, 1.0), height, ink


def down(a):
    """From the painted rows to the stored rows: a little blur down the page, then sampled (2048 x 1366 -> 2048 x 1024)."""
    a = np.asarray(a, np.float32)
    soft = lt.blur(a, 0.0, 0.55) if a.ndim == 2 else np.stack([lt.blur(a[..., c], 0.0, 0.55) for c in range(a.shape[2])],
                                                              axis=-1)
    src = (np.arange(STORE[0]) + 0.5) * PAINT[0] / STORE[0] - 0.5
    i0 = np.clip(np.floor(src).astype(np.int64), 0, PAINT[0] - 1)
    i1 = np.clip(i0 + 1, 0, PAINT[0] - 1)
    t = (src - i0).astype(np.float32)
    t = t[:, None] if a.ndim == 2 else t[:, None, None]
    return soft[i0] * (1.0 - t) + soft[i1] * t


def main():
    color, height, ink = paint()
    lt.write_png(PREVIEW, lt.to8(color))
    # The normal from the painted (square) pixels, then stored: renormalized after the resample.
    n = lt.normal_map(height, 0.001 / PX, 1.0).astype(np.float32) / 255.0 * 2.0 - 1.0
    n = down(n)
    n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
    rough = 0.86 - 0.12 * ink + 0.03 * lp._noise(PAINT, 91, 3.0)
    ao = 1.0 - 0.25 * np.clip(-height / 0.00018, 0.0, 1.0)
    lt.save_set('ReachesMap', down(color), None, 0.001 / PX, np.clip(down(rough), 0.0, 1.0),
                normal=lt.to8(n * 0.5 + 0.5), ao=np.clip(down(ao), 0.0, 1.0), look=False)
    lt._log(f'ReachesMap: preview {PREVIEW}')


if __name__ == '__main__':
    main()
