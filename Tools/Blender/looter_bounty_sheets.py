"""The bounty board's paper (Docs/Work/2026-10-22-Next/BOUNTY.md, ABountyBoard): the Rim Rangers' bounty sheets and the
printed names that go on them. Built on looter_posters.py (the same Rye wood type, aged paper, letterpress and nails),
so a bounty sheet is the cousin of Ellis's poster and Calder's note.

Two decal atlases, shown by M_PosterDecal's instances (the board lays a decal per sheet, as AWantedPoster does):

  T_BountySheets_BC  2048 x 2048, RGBA (straight alpha: the paper's cut edge, hard-clipped at 0.5 by the decal master).
                     Four 1024 x 1024 cells, in order Unpaid, Spider, Spitter, DeadMiner (BountyRules::Kinds), two per
                     row. A sheet is 340 x 460 mm drawn at 2 px/mm (680 x 920 px) centred in its cell (172 px in, 52 px
                     down): the decal shows exactly that rectangle. WANTED / BY THE RIM RANGERS in wood type, the mark's
                     ink portrait of its kind in a frame (a hatched sky and ground, a moon; the mark in solid ink with
                     the paper cut through it for eyes and details), a blank band for the NAME decal, two lines about
                     the kind, "SILVER & A STRONGBOX" in red, a ring where the board's wax seal sits (bottom right),
                     four nails.
  T_BountyNames_BC   2048 x 512, RGBA. 32 strips of printed name, 4 per row, 8 rows (kind index * 8 + NameIndex, the
                     order of BountyRulesWords.cpp's NamesOf, read from that file so they never drift): dark ink on
                     alpha only (the paper is the sheet's). A strip is 300 x 37.5 mm drawn at 1.6 px/mm (480 x 60 px)
                     centred in its 512 x 64 cell (16 px in, 2 down), laid over the sheet's blank band.

    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_bounty_sheets.py                     both atlases, installed
    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_bounty_sheets.py -ScriptArgs '--preview-only'   only the previews
    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_bounty_sheets.py -ScriptArgs Spider            one kind's sheet

Writes Art/Textures/Bounty/T_BountySheets_BC.png and T_BountyNames_BC.png (installed, unless --preview-only) and in
Saved/ArtPreviews/Bounty/: the same two atlases, Sheet_<Kind>.png (each sheet cropped, over the board's wood) and
Sheets.png (the four side by side with a name strip laid on each). Its UV rectangles are logged ("bountysheets:") for
the board's C++ (Bounties/BountyBoard.cpp keeps the same numbers). Then Tools/Unreal/build_bounty_sheet_materials.py in
the open editor imports the PNGs and makes MI_BountySheetDecal and MI_BountyNameDecal.
"""
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import looter_textures as lt  # noqa: E402
import looter_posters as lp  # noqa: E402

REPO = lp.REPO
OUT_DIR = os.path.join(lt.PREVIEW_DIR, 'Bounty')
FINAL_DIR = os.path.join(lt.TEXTURE_DIR, 'Bounty')
NAMES_SOURCE = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'Bounties', 'BountyRulesWords.cpp')

rgb = lt.rgb
smooth = lt.smooth
mix = lt.mix

# --- The layout (the board's C++ keeps the same numbers) ---

KINDS = ('Unpaid', 'Spider', 'Spitter', 'DeadMiner')
SHEET_MM = (340.0, 460.0)
SHEET_MM_PX = 2.0                       # px per mm
CELL = 1024
ATLAS = 2048
STRIP_MM = (300.0, 37.5)
STRIP_MM_PX = 1.6
STRIP_CELL = (512, 64)                  # (w, h) px
NAME_ATLAS = (2048, 512)                # (w, h) px
STRIP_COLUMNS = NAME_ATLAS[0] // STRIP_CELL[0]
NAMES_PER_KIND = 8

# The picture's frame on the sheet (mm): a rounded rectangle; local picture units are mm, x right, y up from its middle.
FRAME_CX, FRAME_CY = 170.0, 212.0
FRAME_HW, FRAME_HH = 100.0, 72.0
GROUND_Y = -40.0                        # the horizon, local
FEET_Y = -52.0
# The blank band the name strip goes over, and the seal's ring (sheet mm).
BAND_Y = (296.25, 333.75)
SEAL_AT = (276.0, 403.0)
SEAL_RING = 27.0

NAILS = [(11.0, 11.0), (329.0, 11.0), (11.0, 449.0), (329.0, 449.0)]
INK = (0.17, 0.145, 0.13)
RED = (0.74, 0.3, 0.22)

COPY = {
    'Unpaid': ('RISEN FROM THE BONEYARD,', 'AND KNOCKING AT DOORS'),
    'Spider': ('TAKER OF MULES AND DOGS,', 'WEBBER OF THE TRAIL'),
    'Spitter': ('SPITS FROM THE HIGH GROUND.', 'MIND YOUR HIDE'),
    'DeadMiner': ('STILL WORKING ITS LAST SHIFT.', 'MIND ITS LAMP'),
}


def log(message):
    lt._log(f'bountysheets: {message}')


def sheet_rect_uv(kind_index):
    """The sheet's rectangle in T_BountySheets_BC (U0, V0, U1, V1; V = 0 at the top)."""
    col, row = kind_index % 2, kind_index // 2
    ox = (CELL - SHEET_MM[0] * SHEET_MM_PX) * 0.5
    oy = (CELL - SHEET_MM[1] * SHEET_MM_PX) * 0.5
    x0, y0 = col * CELL + ox, row * CELL + oy
    return (x0 / ATLAS, y0 / ATLAS, (x0 + SHEET_MM[0] * SHEET_MM_PX) / ATLAS, (y0 + SHEET_MM[1] * SHEET_MM_PX) / ATLAS)


def strip_rect_uv(index):
    """The name strip's rectangle in T_BountyNames_BC."""
    col, row = index % STRIP_COLUMNS, index // STRIP_COLUMNS
    ox = (STRIP_CELL[0] - STRIP_MM[0] * STRIP_MM_PX) * 0.5
    oy = (STRIP_CELL[1] - STRIP_MM[1] * STRIP_MM_PX) * 0.5
    x0, y0 = col * STRIP_CELL[0] + ox, row * STRIP_CELL[1] + oy
    return (x0 / NAME_ATLAS[0], y0 / NAME_ATLAS[1], (x0 + STRIP_MM[0] * STRIP_MM_PX) / NAME_ATLAS[0],
            (y0 + STRIP_MM[1] * STRIP_MM_PX) / NAME_ATLAS[1])


def read_names():
    """The marks' names by kind, from BountyRulesWords.cpp (the game's own list, in its order)."""
    with open(NAMES_SOURCE, encoding='utf-8') as source:
        text = source.read()
    found = {}
    for tag, index, name in re.findall(r'LOCTEXT\("Name(Unpaid|Spider|Spitter|Miner)(\d)",\s*"([^"]+)"\)', text):
        found.setdefault('DeadMiner' if tag == 'Miner' else tag, {})[int(index)] = name
    names = {}
    for kind in KINDS:
        entries = found.get(kind, {})
        if sorted(entries) != list(range(NAMES_PER_KIND)):
            raise SystemExit(f'{NAMES_SOURCE}: expected {NAMES_PER_KIND} names for {kind}, read {sorted(entries)}')
        names[kind] = [entries[i] for i in range(NAMES_PER_KIND)]
    return names


# --- Drawing helpers ---

class Local:
    """The picture's local millimetres (x right, y up from the frame's middle) as sheet pixels."""

    def __init__(self, sheet):
        self.sheet = sheet

    def __call__(self, pts):
        p = np.asarray(pts, np.float64)
        return self.sheet.px(np.stack([FRAME_CX + p[..., 0], FRAME_CY - p[..., 1]], axis=-1))


def ellipse(cx, cy, rx, ry, rot=0.0, n=40):
    a = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    x, y = rx * np.cos(a), ry * np.sin(a)
    r = math.radians(rot)
    return np.stack([cx + x * math.cos(r) - y * math.sin(r), cy + x * math.sin(r) + y * math.cos(r)], axis=1)


def rounded_rect(cx, cy, hw, hh, radius, n=8):
    """A rounded rectangle's outline, closed (first point repeated), in the units given."""
    pts = []
    for sx, sy, a0 in ((1, 1, 0.0), (-1, 1, 90.0), (-1, -1, 180.0), (1, -1, 270.0)):
        ccx, ccy = cx + sx * (hw - radius), cy + sy * (hh - radius)
        for a in np.radians(np.linspace(a0, a0 + 90.0, n)):
            pts.append((ccx + radius * math.cos(a), ccy + radius * math.sin(a)))
    pts.append(pts[0])
    return np.asarray(pts, np.float64)


class Layer:
    """One thing in the picture: solid polygons, thick strokes, and pieces of paper cut through it. halo: the paper is
    cleared round it first (so a figure stands out of the hatching and the backdrop behind it)."""

    def __init__(self, halo=False):
        self.fills, self.holes, self.strokes, self.halo = [], [], [], halo

    def fill(self, pts):
        self.fills.append(np.asarray(pts, np.float64))
        return self

    def hole(self, pts):
        self.holes.append(np.asarray(pts, np.float64))
        return self

    def stroke(self, pts, width, taper=(1.0, 1.0), smooth_pts=True):
        """A stroke through pts (local mm), width mm, pressure running from taper[0] to taper[1] along it."""
        p = np.asarray(pts, np.float64)
        if smooth_pts and len(p) > 2:
            p = lp.spline(p, per=14)
        else:
            p = lp.densify(p, 1.0)
        self.strokes.append((p, width, taper))
        return self


def paint_layers(sheet, layers, bg, seed):
    """The picture's ink coverage (sheet-shaped 0..1) from a background and the layers back to front."""
    local = Local(sheet)
    shape = sheet.shape
    cov = bg.copy()
    for layer in layers:
        fills = [local(f) for f in layer.fills]
        strokes_cov = np.zeros(shape, np.float32)
        for k, (pts, width, taper) in enumerate(layer.strokes):
            pr = np.linspace(taper[0], taper[1], len(pts))
            c, _ = lp.draw_strokes(shape, [(local(pts), pr)], width * lp.MM, var=0.7, seed=seed + k)
            strokes_cov = np.maximum(strokes_cov, c)
        fcov = lp.fill(fills, shape, ss=4) if fills else np.zeros(shape, np.float32)
        solid = np.maximum(fcov, strokes_cov)
        if layer.halo:
            halo = np.clip(lt.blur(solid, 3.4 * lp.MM) * 3.2, 0.0, 1.0)
            cov = cov * (1.0 - halo)
        if layer.holes:
            hcov = lp.fill([local(h) for h in layer.holes], shape, ss=4)
            solid = solid * (1.0 - hcov)
            cov = cov * (1.0 - hcov)
        cov = np.maximum(cov, solid)
    return cov


def backdrop(sheet, seed, moon=(-64.0, 44.0, 11.0)):
    """A woodcut sky of horizontal hatching thinning toward the horizon, a harder horizon and a dense ground, a moon
    cut out of it. Returns the coverage (sheet-shaped)."""
    rng = np.random.default_rng(seed)
    local = Local(sheet)
    shape = sheet.shape
    cov = np.zeros(shape, np.float32)
    loops = []
    y = FRAME_HH - 2.0
    while y > GROUND_Y:
        keep = 1.0 - 0.62 * (FRAME_HH - y) / (FRAME_HH - GROUND_Y)
        x = -FRAME_HW
        while x < FRAME_HW:
            run = rng.uniform(14.0, 60.0)
            if rng.random() < keep:
                loops.append(lp.rect(*local([(x, y - 0.3)])[0], *local([(min(x + run, FRAME_HW), y + 0.3)])[0]))
            x += run + rng.uniform(1.0, 7.0)
        y -= 3.0 + rng.uniform(-0.3, 0.3)
    # The horizon and the ground: a heavy line, then close hatching, a little denser toward the front.
    loops.append(lp.rect(*local([(-FRAME_HW, GROUND_Y - 0.55)])[0], *local([(FRAME_HW, GROUND_Y + 0.55)])[0]))
    y = GROUND_Y - 3.0
    gap = 2.4
    while y > -FRAME_HH:
        loops.append(lp.rect(*local([(-FRAME_HW, y - 0.32)])[0], *local([(FRAME_HW, y + 0.32)])[0]))
        y -= gap
        gap = max(1.5, gap - 0.12)
    for loop in loops:
        lp.fill_into(cov, [loop], ss=4)
    # A moon: the hatching gone, a ring of ink round it.
    mx, my, mr = moon
    disc = lp.fill([local(ellipse(mx, my, mr, mr))], shape, ss=4)
    ring, _ = lp.draw_strokes(shape, [(local(np.vstack([ellipse(mx, my, mr, mr), ellipse(mx, my, mr, mr)[:1]])),
                                       np.ones(41))], 0.7 * lp.MM, var=0.0)
    halo = np.clip(lt.blur(disc, 1.6 * lp.MM) * 2.2, 0.0, 1.0)
    return np.maximum(cov * (1.0 - halo), ring)


# --- The marks' portraits (local mm; y up; feet at FEET_Y) ---

def figure_unpaid():
    """A gaunt figure risen from the ground, hatless, coat in tatters, arms out, coal-cold eyes; a leaning headstone."""
    stone = Layer()
    stone.fill(np.vstack([[(58, -46), (58, -6)], lp.arc(68, -6, 10, 11, 180, 0, step=12)[::1], [(78, -46)]]))
    stone.hole(np.array([(66.8, -10), (69.2, -10), (69.2, -18), (74, -18), (74, -20.4), (69.2, -20.4), (69.2, -30), (66.8, -30),
                         (66.8, -20.4), (62, -20.4), (62, -18), (66.8, -18)]))
    cross = Layer()
    cross.fill(np.array([(-76, -46), (-72, -46), (-71, -20), (-66, -19.5), (-66.2, -16), (-71.2, -16.5), (-71.4, -8), (-75.4, -8.3),
                         (-75.4, -16.6), (-80, -17), (-80, -20.4), (-75.6, -20.2)]))
    man = Layer(halo=True)
    man.fill(ellipse(2, 50, 9.0, 11.5, rot=-8))
    man.fill([(-3, 41), (7, 41), (9, 33), (-5, 33)])
    man.fill([(-8, 38), (8, 38), (22, 33), (27, 20), (25, 2), (29, -20), (24, -32), (17, -28), (12, -37), (5, -30), (-1, -40),
              (-8, -31), (-14, -38), (-19, -29), (-27, -34), (-28, -12), (-24, 6), (-27, 22), (-22, 33)])
    man.stroke([(-9, -30), (-11, -42), (-12, -51)], 7.0)
    man.stroke([(9, -30), (11, -42), (14, -51)], 7.0)
    man.fill([(-21, -53), (-9, -53), (-8, -49), (-14, -46), (-21, -48)])
    man.fill([(9, -53), (22, -53), (22, -49), (15, -46), (9, -48)])
    man.stroke([(-22, 30), (-37, 22), (-52, 27)], 6.5)
    man.stroke([(22, 30), (40, 32), (57, 41)], 6.5)
    man.fill(ellipse(-54, 28, 3.6, 3.0))
    man.fill(ellipse(59, 42, 3.6, 3.0))
    # Hair in a few wisps, the eyes and mouth cut through, buttons and the collar's line.
    for dx in (-9, -5, 0, 5, 9):
        man.stroke([(2 + dx * 0.8, 58), (2 + dx * 1.3, 66), (2 + dx * 1.6 + 2, 68)], 0.9, smooth_pts=False)
    man.hole(ellipse(-1.8, 52.5, 2.1, 1.5))
    man.hole(ellipse(5.8, 52.5, 2.1, 1.5))
    man.hole([(0, 45.4), (5.4, 45.6), (5.4, 44.2), (0, 44.0)])
    for y in (24, 12, 0, -12):
        man.hole(ellipse(3, y, 1.2, 1.2, n=14))
    man.hole(lp.rect(-0.4, 8, 0.4, 36))
    return [stone, cross, man]


def spider_legs(layer, raised=0.0, width=4.4):
    legs = [[(8, 22), (30, 52 + raised), (52, 40 + raised * 0.4), (66, 18)],
            [(10, 16), (40, 44), (66, 26), (78, -14)],
            [(11, 8), (44, 26), (70, 6), (80, -38)],
            [(11, 0), (40, 6), (62, -14), (66, -50)]]
    for leg in legs:
        for side in (1, -1):
            layer.stroke([(side * x, y) for x, y in leg], width, taper=(1.0, 0.5))


def spider_body(layer, dy=0.0, tilt=0.0):
    layer.fill(ellipse(0, -14 + dy, 25, 22))
    layer.fill(ellipse(-tilt * 0.3, 16 + dy, 13, 11, rot=tilt))
    # The hourglass the larder queens wear, and the cluster of eyes, cut through.
    layer.hole([(-6, -4 + dy), (6, -4 + dy), (0, -14 + dy)])
    layer.hole([(0, -14 + dy), (6, -24 + dy), (-6, -24 + dy)])
    for x, y, r in ((-6, 19, 1.8), (-2.4, 22, 1.5), (2.4, 22, 1.5), (6, 19, 1.8), (-3.6, 16, 1.2), (3.6, 16, 1.2)):
        layer.hole(ellipse(x - tilt * 0.3, y + dy, r, r, n=14))


def figure_spider():
    web = Layer()
    # A strand of web across the corner, in thin ink.
    web.stroke([(-98, 68), (-60, 52), (-20, 62)], 0.5)
    web.stroke([(-98, 68), (-70, 40), (-60, 16)], 0.5)
    web.stroke([(-82, 62), (-70, 52), (-62, 46)], 0.4, smooth_pts=False)
    spider = Layer(halo=True)
    spider_legs(spider)
    spider_body(spider)
    return [web, spider]


def figure_spitter():
    spit = Layer(halo=True)
    # The venom: a spray of drops thrown over the ground from the mouth.
    for k in range(9):
        t = k / 8.0
        x = -14.0 - 84.0 * t
        y = 14.0 + 50.0 * t * (1.0 - t * 0.9) * 2.0 - 30.0 * t * t
        r = 5.4 - 3.4 * t
        spit.fill(np.vstack([ellipse(x, y, r, r * 1.25, n=14), [(x + r * 1.4 + 1.2, y + r * 1.5)]]))
    sp = Layer(halo=True)
    spider_legs(sp, raised=8.0)
    spider_body(sp, dy=-6.0, tilt=14.0)
    # The fangs, wide apart, a bead of venom at each tip.
    sp.fill([(-12, 4), (-8.5, 6), (-9.4, -4)])
    sp.fill([(-3.5, 6.4), (0, 4.8), (-1.6, -4)])
    return [sp, spit]


def figure_miner():
    timber = Layer()
    for x0, x1 in ((-82, -70), (70, 82)):
        timber.fill(lp.rect(x0, -46, x1, 64))
        timber.hole(lp.rect(x0 + 1.5, -46, x1 - 1.5, 62.5))
    timber.fill(lp.rect(-86, 52, 86, 64))
    timber.hole(lp.rect(-84.5, 53.5, 84.5, 62.5))
    timber.hole(lp.rect(-14, 52.0, 14, 64.5))
    miner = Layer(halo=True)
    miner.fill(ellipse(4, 42, 9.0, 10.0))
    miner.fill(np.vstack([lp.arc(4, 46, 12.5, 14.0, 0, 180, step=12), [(-9, 45), (-12, 43), (20, 43), (17, 45)]]))
    miner.fill([(-8, 38), (10, 40), (22, 36), (24, 12), (18, -8), (16, -26), (-12, -26), (-14, -8), (-19, 10), (-17, 34)])
    miner.stroke([(-6, -24), (-8, -40), (-14, -51)], 8.0)
    miner.stroke([(8, -24), (11, -40), (9, -51)], 8.0)
    miner.fill([(-24, -53), (-9, -53), (-8, -48), (-15, -45), (-24, -47)])
    miner.fill([(2, -53), (18, -53), (18, -49), (11, -45), (2, -47)])
    # The pick, held across the body; the arms to it.
    miner.stroke([(-14, -6), (14, 20), (46, 58)], 2.8)
    miner.stroke([(34, 66), (48, 62), (60, 50)], 5.5, taper=(1.0, 0.25))
    miner.stroke([(48, 62), (54, 71)], 3.0, taper=(1.0, 0.3), smooth_pts=False)
    miner.stroke([(-17, 30), (-26, 14), (-14, -4)], 6.5)
    miner.stroke([(22, 34), (32, 26), (20, 18)], 6.5)
    miner.fill(ellipse(-13, -5, 3.8, 3.4))
    miner.fill(ellipse(19, 19, 3.8, 3.4))
    # The lamp on the helmet's brow, the vest's braces and the helmet's rim, cut through.
    miner.hole(ellipse(4, 49.5, 2.8, 2.8, n=16))
    miner.hole(lp.rect(-13, 44.2, 21, 44.9))
    miner.hole(lp.rect(-3.4, 12, -2.4, 38))
    miner.hole(lp.rect(10.6, 12, 11.6, 38))
    miner.hole(ellipse(-2.2, 41.6, 1.7, 1.2, n=12))
    miner.hole(ellipse(10.4, 41.6, 1.7, 1.2, n=12))
    return [timber, miner]


FIGURES = {'Unpaid': figure_unpaid, 'Spider': figure_spider, 'Spitter': figure_spitter, 'DeadMiner': figure_miner}


# --- The sheet ---

def register_cells():
    lp.CELLS['BountySheet'] = (0, 0, CELL, CELL)
    lp.CELLS['BountyName'] = (0, 0, STRIP_CELL[0], STRIP_CELL[1])


def bounty_outline(sheet, seed):
    """The sheet's outline (px): four machine-cut edges, a hair uneven with a few nicks."""
    rng = np.random.default_rng(seed)
    W, H = sheet.size
    top = lp._edge(sheet, (0.0, 0.0), (W, 0.0), rng, lp.CUT, bumps=[(0.33, 1.6, 4.0), (0.74, 2.2, 5.0)])
    right = lp._edge(sheet, (W, 0.0), (W, H), rng, lp.CUT, bumps=[(0.5, 1.8, 4.0)])
    bottom = lp._edge(sheet, (W, H), (0.0, H), rng, lp.CUT, bumps=[(0.6, 2.0, 5.0)])
    left = lp._edge(sheet, (0.0, H), (0.0, 0.0), rng, lp.CUT, bumps=[(0.38, 3.0, 6.0), (0.8, 1.6, 3.0)])
    return np.concatenate([top[:-1], right[:-1], bottom[:-1], left[:-1]])


def check_text(text):
    font = lp.font()
    missing = sorted({ch for ch in text if ch != ' ' and font.glyph(ch) == 0})
    if missing:
        raise SystemExit(f'Rye has no glyph for {missing} in "{text}"')


def sheet_print(sheet, kind, seed):
    """The sheet's type and picture: (black ink, red ink)."""
    rng = np.random.default_rng(seed)
    black, red = lp.Ink(sheet.shape), lp.Ink(sheet.shape)
    W, H = sheet.size
    # The border: a heavy rule and a hairline inside it.
    for inset, thick in ((10.5, 2.4), (14.8, 1.0)):
        lp.rule(black, sheet, inset - thick * 0.5, W - inset + thick * 0.5, inset, thick, rng)
        lp.rule(black, sheet, inset - thick * 0.5, W - inset + thick * 0.5, H - inset, thick, rng)
        for x in (inset, W - inset):
            lp.type_shape(black, sheet, [lp.rect(x - thick * 0.5, inset, x + thick * 0.5, H - inset)])
    copy = COPY[kind]
    for text in (copy[0], copy[1], "RIM RANGERS OF RANSOM'S REST", 'WANTED', 'BY THE RIM RANGERS', 'SILVER & A STRONGBOX',
                 'PAID AT THE BOARD, ON PROOF'):
        check_text(text)
    lp.type_line(black, sheet, rng, "RIM RANGERS OF RANSOM'S REST", 7.2, W * 0.5, 31.0, width=226.0)
    for x in (44.0, W - 44.0):
        lp.type_shape(black, sheet, [lp.star(x, 28.0, 5.2)])
    lp.type_line(black, sheet, rng, 'WANTED', 58.0, W * 0.5, 94.0, width=268.0, wood=True)
    lp.rule(black, sheet, 38.0, W - 38.0, 103.0, 2.0, rng)
    lp.rule(black, sheet, 38.0, W - 38.0, 107.4, 0.9, rng)
    lp.type_line(black, sheet, rng, 'BY THE RIM RANGERS', 14.5, W * 0.5, 129.0, width=252.0)
    # The picture, in its frame.
    return black, red, rng


def print_lower(sheet, kind, black, red, rng):
    W, H = sheet.size
    copy = COPY[kind]
    # Rules that hold the name band, and the lines about the mark under it.
    lp.rule(black, sheet, 24.0, W - 24.0, BAND_Y[0] - 4.0, 1.2, rng)
    lp.rule(black, sheet, 24.0, W - 24.0, BAND_Y[1] + 4.0, 1.2, rng)
    for x in (W * 0.5 - 8.0, W * 0.5, W * 0.5 + 8.0):
        lp.type_shape(black, sheet, [lp.diamond(x, BAND_Y[1] + 4.0, 2.2, 1.6)])
    lp.type_line(black, sheet, rng, copy[0], 10.5, W * 0.5, 354.0, width=min(286.0, lp.set_line(copy[0], 10.5)[1]))
    lp.type_line(black, sheet, rng, copy[1], 10.5, W * 0.5, 369.0, width=min(286.0, lp.set_line(copy[1], 10.5)[1]))
    # The pay in red, left of the seal's ring.
    lp.type_line(red, sheet, rng, 'SILVER & A STRONGBOX', 17.0, 28.0, 408.0, width=205.0, wood=True, align='left')
    lp.type_line(black, sheet, rng, 'PAID AT THE BOARD, ON PROOF', 6.8, 28.0, 425.0, width=176.0, align='left')


def picture_cov(sheet, kind, seed):
    local = Local(sheet)
    layers = FIGURES[kind]()
    moon = (64.0, 46.0, 11.0) if kind == "Spitter" else (-64.0, 44.0, 11.0)
    cov = paint_layers(sheet, layers, backdrop(sheet, seed + 1, moon), seed + 2)
    # Clipped to the frame; the frame itself, a heavy line and a hairline.
    inner = rounded_rect(FRAME_CX, FRAME_CY, FRAME_HW, FRAME_HH, 9.0)
    mask = lp.fill([sheet.px(inner[:-1])], sheet.shape, ss=4)
    cov = cov * mask
    for grow, width in ((0.0, 1.8), (3.2, 0.7)):
        path = rounded_rect(FRAME_CX, FRAME_CY, FRAME_HW + grow, FRAME_HH + grow, 9.0 + grow)
        c, _ = lp.draw_strokes(sheet.shape, [(sheet.px(path), np.ones(len(path)))], width * lp.MM, var=0.0)
        cov = np.maximum(cov, c)
    # Brass corner studs on the frame.
    for sx in (-1, 1):
        for sy in (-1, 1):
            cov = np.maximum(cov, lp.fill([sheet.px(ellipse(FRAME_CX + sx * (FRAME_HW + 1.6), FRAME_CY + sy * (FRAME_HH + 1.6), 1.8, 1.8,
                                                           n=16))], sheet.shape, ss=4))
    return cov


def seal_ring(sheet):
    """Where the wax seal goes: a thin printed ring and a fainter inner one, the seal covering most of it."""
    cov = np.zeros(sheet.shape, np.float32)
    for r, width in ((SEAL_RING, 0.9), (SEAL_RING - 3.0, 0.45)):
        pts = ellipse(SEAL_AT[0], SEAL_AT[1], r, r, n=72)
        pts = np.vstack([pts, pts[:1]])
        c, _ = lp.draw_strokes(sheet.shape, [(sheet.px(pts), np.ones(len(pts)))], width * lp.MM, var=0.0)
        cov = np.maximum(cov, c)
    return cov


def bounty_sheet(kind, seed):
    """One kind's sheet: (colour (h, w, 3), alpha) over its 1024 cell."""
    sheet = lp.Sheet('BountySheet', SHEET_MM)
    loop = bounty_outline(sheet, seed + 100)
    mask = lp.fill([loop], sheet.shape, ss=4)
    color, maps = lp.poster_paper(sheet, mask, seed + 200)
    black, red, rng = sheet_print(sheet, kind, seed + 300)
    print_lower(sheet, kind, black, red, rng)
    black.cov = np.maximum(black.cov, picture_cov(sheet, kind, seed + 350) * 0.97)
    black.cov = np.maximum(black.cov, seal_ring(sheet) * 0.7)
    a = lp.letterpress(black, maps['fine'], maps['fibers'], seed + 400) * (1.0 - 0.75 * maps['cracks'])
    color = lp.multiply(color, a, INK)
    shift = np.roll(np.roll(red.cov, 1, axis=1), -1, axis=0)
    red.cov, red.wood = shift, np.roll(np.roll(red.wood, 1, axis=1), -1, axis=0)
    a = lp.letterpress(red, maps['fine'], maps['fibers'], seed + 410, fade=0.85) * (1.0 - 0.75 * maps['cracks'])
    color = lp.multiply(color, a, RED)
    heads = np.zeros(sheet.shape, np.float32)
    for k, (nx, ny) in enumerate(NAILS):
        cx, cy = sheet.px((nx, ny))
        lp.paint_nail(color, mask, heads, cx, cy, 4.0 * lp.MM, seed + 700 + k, rust=0.7)
    return np.clip(color, 0.0, 1.0), np.maximum(mask, heads)


def name_strip(text, seed):
    """One printed name: (colour (h, w, 3), alpha) over its 512 x 64 cell: dark ink, alpha only where the type is."""
    sheet = lp.Sheet('BountyName', STRIP_MM)
    rng = np.random.default_rng(seed)
    ink = lp.Ink(sheet.shape)
    upper = text.upper()
    check_text(upper)
    cap = 21.0
    natural = lp.set_line(upper, cap)[1]
    lp.type_line(ink, sheet, rng, upper, cap, STRIP_MM[0] * 0.5, 29.0, width=min(natural, 290.0), wood=True)
    alpha = np.clip(lt.blur(ink.cov, 0.35), 0.0, 1.0)
    color = np.ones(sheet.shape + (3,), np.float32) * np.asarray(INK, np.float32)
    return color, alpha


# --- Atlases ---

def build_sheets(kinds):
    color = np.zeros((ATLAS, ATLAS, 3), np.float32)
    alpha = np.zeros((ATLAS, ATLAS), np.float32)
    done = {}
    lp.MM = SHEET_MM_PX
    for index, kind in enumerate(KINDS):
        if kind not in kinds:
            continue
        c, a = bounty_sheet(kind, 11 + 37 * index)
        done[kind] = (c, a)
        col, row = index % 2, index // 2
        color[row * CELL:(row + 1) * CELL, col * CELL:(col + 1) * CELL] = c
        alpha[row * CELL:(row + 1) * CELL, col * CELL:(col + 1) * CELL] = a
        log(f'{kind} sheet drawn; UV {tuple(round(v, 6) for v in sheet_rect_uv(index))}')
    color = lt.fill_transparent(color, alpha)
    return np.concatenate([lt.to8(color), lt.to8(alpha)[..., None]], axis=-1), done


def build_names(names):
    color = np.zeros((NAME_ATLAS[1], NAME_ATLAS[0], 3), np.float32) + np.asarray(INK, np.float32)
    alpha = np.zeros((NAME_ATLAS[1], NAME_ATLAS[0]), np.float32)
    lp.MM = STRIP_MM_PX
    strips = []
    for k, kind in enumerate(KINDS):
        for n, name in enumerate(names[kind]):
            index = k * NAMES_PER_KIND + n
            c, a = name_strip(name, 500 + index)
            col, row = index % STRIP_COLUMNS, index // STRIP_COLUMNS
            x0, y0 = col * STRIP_CELL[0], row * STRIP_CELL[1]
            color[y0:y0 + STRIP_CELL[1], x0:x0 + STRIP_CELL[0]] = c
            alpha[y0:y0 + STRIP_CELL[1], x0:x0 + STRIP_CELL[0]] = a
            strips.append((kind, name, c, a))
    rgba = np.concatenate([lt.to8(color), lt.to8(alpha)[..., None]], axis=-1)
    return rgba, strips


# --- Previews ---

def wood_plank(shape):
    """The board's weathered wood, for the previews to lay the sheets on."""
    base = np.ones(shape + (3,), np.float32) * rgb(0x6b5642)
    grain = 0.5 * lt.noise(shape, 5, 90.0, 2.0, octaves=2) + 0.3 * lt.noise(shape, 6, 30.0, 1.2)
    return np.clip(base * (1.0 + 0.16 * grain)[..., None], 0.0, 1.0)


def preview_sheet(color, alpha, strip=None):
    """A sheet cropped from its cell, over wood, a shadow under it, the name strip laid on its band."""
    ox = int((CELL - SHEET_MM[0] * SHEET_MM_PX) * 0.5)
    oy = int((CELL - SHEET_MM[1] * SHEET_MM_PX) * 0.5)
    w, h = int(SHEET_MM[0] * SHEET_MM_PX), int(SHEET_MM[1] * SHEET_MM_PX)
    pad = 40
    out = wood_plank((h + 2 * pad, w + 2 * pad))
    c = color[oy:oy + h, ox:ox + w]
    a = alpha[oy:oy + h, ox:ox + w]
    shadow = np.zeros(out.shape[:2], np.float32)
    shadow[pad + 6:pad + 6 + h, pad + 5:pad + 5 + w] = a
    shadow = lt.blur(shadow, 5.0) * 0.55
    out = out * (1.0 - shadow[..., None])
    region = out[pad:pad + h, pad:pad + w]
    if strip is not None:
        sc, sa = strip
        sx = int((STRIP_CELL[0] - STRIP_MM[0] * STRIP_MM_PX) * 0.5)
        sy = int((STRIP_CELL[1] - STRIP_MM[1] * STRIP_MM_PX) * 0.5)
        sw, sh = int(STRIP_MM[0] * STRIP_MM_PX), int(STRIP_MM[1] * STRIP_MM_PX)
        piece = sc[sy:sy + sh, sx:sx + sw]
        pa = sa[sy:sy + sh, sx:sx + sw]
        # The strip is 300 mm at 1.6 px/mm and the sheet 2 px/mm: scaled up to the sheet's pixels.
        scale = SHEET_MM_PX / STRIP_MM_PX
        ny, nx = int(round(sh * scale)), int(round(sw * scale))
        yy = (np.arange(ny) / scale).astype(int).clip(0, sh - 1)
        xx = (np.arange(nx) / scale).astype(int).clip(0, sw - 1)
        piece, pa = piece[yy][:, xx], pa[yy][:, xx]
        # The strip's hard cut at 0.5, as the decal master does.
        pa = (pa >= 0.5).astype(np.float32)
        top = int(round((BAND_Y[0]) * SHEET_MM_PX))
        left = int(round(20.0 * SHEET_MM_PX))
        c = c.copy()
        sub = c[top:top + ny, left:left + nx]
        sub[...] = mix(sub, piece, pa)
    # The decal master clips alpha hard at 0.5.
    clip = (a >= 0.5).astype(np.float32)[..., None]
    region[...] = mix(region, c, clip[..., 0])
    return out


def build_previews(sheet_rgba, done, name_rgba, strips, names):
    os.makedirs(OUT_DIR, exist_ok=True)
    lt.write_png(os.path.join(OUT_DIR, 'T_BountySheets_BC.png'), sheet_rgba)
    lt.write_png(os.path.join(OUT_DIR, 'T_BountyNames_BC.png'), name_rgba)
    tiles = []
    for index, kind in enumerate(KINDS):
        if kind not in done:
            continue
        c, a = done[kind]
        strip = next(((sc, sa) for k, n, sc, sa in strips if k == kind and n == names[kind][0]), None)
        # The strip alpha comes from the whole name atlas cell; rebuild a cell-shaped pair for the preview.
        tile = preview_sheet(c, a, strip)
        lt.write_png(os.path.join(OUT_DIR, f'Sheet_{kind}.png'), lt.to8(tile))
        tiles.append(tile)
    if len(tiles) > 1:
        height = max(t.shape[0] for t in tiles)
        row = np.concatenate([np.pad(t, ((0, height - t.shape[0]), (0, 0), (0, 0))) for t in tiles], axis=1)
        lt.write_png(os.path.join(OUT_DIR, 'Sheets.png'), lt.to8(row))
    log('previews written to ' + OUT_DIR)


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    kinds = [k for k in KINDS if k in argv] or list(KINDS)
    preview_only = '--preview-only' in argv
    register_cells()
    names = read_names()
    saved_mm = lp.MM
    try:
        sheet_rgba, done = build_sheets(kinds)
        name_rgba, strips = build_names(names)
    finally:
        lp.MM = saved_mm
    for index in range(len(KINDS) * NAMES_PER_KIND):
        if index in (0, 8, 16, 24, 31):
            log(f'name strip {index}: UV {tuple(round(v, 6) for v in strip_rect_uv(index))}')
    # The name strips for the previews are cell-shaped: rebuild them cell by cell from the atlas.
    cells = []
    for k, kind in enumerate(KINDS):
        for n, name in enumerate(names[kind]):
            index = k * NAMES_PER_KIND + n
            col, row = index % STRIP_COLUMNS, index // STRIP_COLUMNS
            x0, y0 = col * STRIP_CELL[0], row * STRIP_CELL[1]
            cell = name_rgba[y0:y0 + STRIP_CELL[1], x0:x0 + STRIP_CELL[0]]
            cells.append((kind, name, cell[..., :3].astype(np.float32) / 255.0, cell[..., 3].astype(np.float32) / 255.0))
    build_previews(sheet_rgba, done, name_rgba, cells, names)
    if not preview_only and len(kinds) == len(KINDS):
        os.makedirs(FINAL_DIR, exist_ok=True)
        lt.write_png(os.path.join(FINAL_DIR, 'T_BountySheets_BC.png'), sheet_rgba)
        lt.write_png(os.path.join(FINAL_DIR, 'T_BountyNames_BC.png'), name_rgba)
        log(f'installed in {FINAL_DIR}')


if __name__ == '__main__':
    main()
