"""The gun makers' marks (Docs/Work/2026-10-22-Next/MAKERS.md; concept Saved/ArtPreviews/Weapons/Concepts/Makers.png).
One outline per maker serves twice:
  - as an Inked icon for the UI (cards, labels): InkedIcons.py's icons() adds maker_marks() to its list, so they are
    generated into UI/Style/InkedIconData.inl as MakerRoundhouse, MakerSaintMaud, MakerWindfall, MakerMeridian
    (WeaponMakers' MarkIcon names them);
  - as the mask the gun master paints on a gun's body: running this file writes the 2x2 atlas
    Art/Textures/GunMakerMarks/T_GunMakerMarks.png (white where the mark is, cell 1 top left, 2 top right, 3 bottom left,
    4 bottom right: FWeaponMaker::MarkCell), which Tools/Unreal/create_manufacturers.py imports. The mask is the icon's
    solid fill with its cuts left open (no ink line: the material colours it).

    python Art/Icons/MakerMarks.py        (or Blender's python; numpy only)

Icons are in the 64-unit square box the ember powers use (x right, y down).
"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import InkedIcons as ink   # noqa: E402  (the shared Icon type, geometry helpers and rasterizer rules)

REPO = ink.REPO
ATLAS_OUT = os.path.join(REPO, 'Art', 'Textures', 'GunMakerMarks', 'T_GunMakerMarks.png')
ATLAS_CELL = 256             # pixels per cell: the mark is about 4-6 cm on a gun, so this is plenty at first-person range
MARK_ORDER = ('MakerRoundhouse', 'MakerSaintMaud', 'MakerWindfall', 'MakerMeridian')   # cells 1-4
CUT = 3.0                    # the cuts' width in icon units (wider than the ember icons': the mark is drawn small)


def annulus_sector(cx, cy, r0, r1, a0, a1, n=12):
    """A ring's slice between radii r0 and r1 from angle a0 to a1 (degrees, clockwise on screen from +x)."""
    outer = [(cx + r1 * math.cos(math.radians(a0 + (a1 - a0) * k / n)), cy + r1 * math.sin(math.radians(a0 + (a1 - a0) * k / n)))
             for k in range(n + 1)]
    inner = [(cx + r0 * math.cos(math.radians(a1 - (a1 - a0) * k / n)), cy + r0 * math.sin(math.radians(a1 - (a1 - a0) * k / n)))
             for k in range(n + 1)]
    return outer + inner


def roundhouse():
    """Roundhouse Rail Works: a locomotive's drive wheel, its rim, eight spokes, the hub and the counterweight."""
    cx, cy = 32.0, 32.0
    ring = ink.circle(cx, cy, 25.0, 40)
    shapes = [ink.rot(ink.rect(cx - 1.7, cx + 1.7, cy - 24.0, cy + 24.0), cx, cy, a) for a in (0, 45, 90, 135)]
    shapes += [ink.circle(cx, cy, 7.5, 18), annulus_sector(cx, cy, 6.5, 23.5, 120, 240)]
    cuts = [ink.closed(ink.circle(cx, cy, 2.6, 10))]
    return ink.Icon('MakerRoundhouse', shapes, strokes=[ring + ring[:1]], cuts=cuts, shade=34,
                    outline=ink.OUTLINE['Ammo'], stroke_width=5.0, cut_width=CUT)


def saint_maud():
    """Saint Maud's Foundry: an anvil (horn to the right) under a saint's halo, a spark between."""
    anvil = [(8.0, 30.0), (46.0, 30.0)] + ink.quad((46.0, 30.0), (58.0, 30.0), (60.0, 24.0))[1:] \
        + [(60.0, 28.0)] + ink.quad((60.0, 28.0), (59.0, 39.0), (46.0, 40.0))[1:] \
        + [(41.0, 40.0), (38.0, 47.0), (44.0, 52.0), (44.0, 58.0), (14.0, 58.0), (14.0, 52.0), (20.0, 47.0), (17.0, 40.0),
           (14.0, 40.0)] + ink.quad((14.0, 40.0), (8.0, 39.0), (8.0, 30.0))[1:-1]
    halo = [(30.0 + 15.0 * math.cos(2 * math.pi * k / 32), 10.0 + 4.5 * math.sin(2 * math.pi * k / 32)) for k in range(32)]
    shapes = [anvil, ink.star4(30.0, 22.0, 4.5, 1.3)]
    cuts = [[(14.0, 52.0), (44.0, 52.0)]]
    return ink.Icon('MakerSaintMaud', shapes, strokes=[halo + halo[:1]], cuts=cuts, shade=44,
                    outline=ink.OUTLINE['Ammo'], stroke_width=3.4, cut_width=CUT)


def windfall():
    """Windfall Salvage: a salvager's grapple (eye, shank, two tines) with a gust of wind blowing through it."""
    shank = ink.rect(29.5, 34.5, 10.0, 44.0)
    left = ink.quad((32.0, 42.0), (32.0, 58.0), (16.0, 55.0)) + ink.quad((16.0, 55.0), (9.0, 53.0), (10.0, 45.0))[1:]
    right = [(64.0 - x, y) for x, y in left]
    eye = ink.circle(32.0, 7.0, 4.2, 16)
    gust = ink.quad((4.0, 27.0), (16.0, 19.0), (27.0, 25.0)) + [(37.0, 25.0)] + ink.quad((37.0, 25.0), (50.0, 31.0), (60.0, 20.0))[1:]
    shapes = [shank, ink.offset_band(gust, [0.8] + [3.4] * (len(gust) - 2) + [0.8])]
    return ink.Icon('MakerWindfall', shapes, strokes=[left, right, eye + eye[:1]], shade=34,
                    outline=ink.OUTLINE['Ammo'], stroke_width=5.0, cut_width=CUT)


def meridian():
    """Meridian Instrument Co.: a surveyor's transit reticle (ring, four ticks, a dot) under a north star."""
    cx, cy = 32.0, 36.0
    ring = ink.circle(cx, cy, 20.0, 36)
    shapes = [ink.rect(cx - 1.7, cx + 1.7, cy - 17.0, cy - 6.0), ink.rect(cx - 1.7, cx + 1.7, cy + 6.0, cy + 17.0),
              ink.rect(cx - 17.0, cx - 6.0, cy - 1.7, cy + 1.7), ink.rect(cx + 6.0, cx + 17.0, cy - 1.7, cy + 1.7),
              ink.circle(cx, cy, 2.6, 12), [(32.0, 2.0), (36.5, 12.0), (32.0, 10.0), (27.5, 12.0)]]
    return ink.Icon('MakerMeridian', shapes, strokes=[ring + ring[:1]], shade=36,
                    outline=ink.OUTLINE['Ammo'], stroke_width=4.4, cut_width=CUT)


def maker_marks(square=(0.0, 0.0, 64.0, 64.0)):
    """The four marks, each placed in the shared 64-unit box (InkedIcons.icons() passes the ember powers' box)."""
    return [m.place(square) for m in (roundhouse(), saint_maud(), windfall(), meridian())]


def mask(icon, size):
    """The mark's fill (no ink line, cuts left open) at size x size pixels, 0-255."""
    ppu = size / max(icon.view)
    s = ink.SUPERSAMPLE
    X, Y = np.meshgrid((np.arange(size * s) + 0.5) / (s * ppu), (np.arange(size * s) + 0.5) / (s * ppu))
    shape = np.zeros(X.shape, bool)
    for poly in icon.shapes:
        shape |= ink.inside(X, Y, poly)
    for line in icon.strokes:
        shape |= ink.line_distance(X, Y, line, False) <= icon.stroke_width * 0.5
    for poly in icon.holes:
        shape &= ~ink.inside(X, Y, poly)
    for line in icon.cuts:
        shape &= ~(ink.line_distance(X, Y, line, False) <= icon.cut_width * 0.5)
    return (shape.astype(float).reshape(size, s, size, s).mean(axis=(1, 3)) * 255 + 0.5).astype(np.uint8)


def write_atlas():
    marks = {m.name: m for m in maker_marks()}
    atlas = np.zeros((ATLAS_CELL * 2, ATLAS_CELL * 2), np.uint8)
    for index, name in enumerate(MARK_ORDER):
        row, col = divmod(index, 2)
        atlas[row * ATLAS_CELL:(row + 1) * ATLAS_CELL, col * ATLAS_CELL:(col + 1) * ATLAS_CELL] = mask(marks[name], ATLAS_CELL)
    rgba = np.dstack([atlas, atlas, atlas, np.full(atlas.shape, 255, np.uint8)])
    os.makedirs(os.path.dirname(ATLAS_OUT), exist_ok=True)
    ink.write_png(ATLAS_OUT, rgba)
    return ATLAS_OUT


if __name__ == '__main__':
    print('wrote', write_atlas())
