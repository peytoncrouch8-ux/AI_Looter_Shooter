"""The game's weapon and ammo icons in the Inked style, the user's pick on 2026-10-01 (the five concepts are in
Art/Backlog/Icons/IconConcepts.py). An icon is a light top and a shaded underside inside a thick dark ink line, so it
reads over any background: menus, the HUD over the world, an ammo box in the grass.

This file holds every icon's outline; it is where the icons are made. Running it writes
    Source/AI_Looter_Shooter/UI/Style/InkedIconData.inl  every icon as C++ data, one function per icon
                                                         (generated: change the outlines here and run this again)
    Saved/ArtPreviews/Icons/Inked_<Icon>_<px>.png        each icon drawn by the rules below, to check the game against
    Saved/ArtPreviews/Icons/InkedIcons.html              a reference sheet of those pictures

    python Art/Icons/InkedIcons.py
    (or Blender's: blender -b --factory-startup --python Art/Icons/InkedIcons.py; it needs only numpy)

A new weapon: trace its silhouette from its model (Art/Models/Weapons/<Gun>.py, gun space in cm: u along the barrel,
v up) into a function like bullpup(), give it an outline width (OUTLINE), add it to ICONS and run this.

What an icon is (LooterUI::FInkedIcon in the game). Every coordinate is in the icon's own units, x right and y down,
inside a view box that starts at (0, 0) and already leaves room for the ink line:
  Shapes        filled outlines: any simple polygon, either winding, overlapping as they like (their union is filled)
  Holes         polygons cut through the shapes (the trigger guard's opening)
  Strokes       open polylines drawn as part of the shape, StrokeWidth wide (the shotgun's trigger guard)
  Cuts          detail lines drawn in ink across the fill, CutWidth wide (seams, vents, a case mouth)
  ShadeY        the fill is light above this line and shaded below it
  OutlineWidth  the ink line round the outside of the shape and inside its holes; at small sizes it grows to stay
                MIN_OUTLINE_PIXELS wide in the texture, up to MaxOutlineWidth (the room the view box leaves)

How the game draws one (keep LooterUI's rasterizer and draw() below the same): each texture pixel averages
SUPERSAMPLE x SUPERSAMPLE points; at each point p (view units)
  solid   = (p inside any Shape (even-odd test) or within StrokeWidth/2 of a Stroke) and p inside no Hole
  if solid: Ink if p is within CutWidth/2 of a Cut, else Light when p.y < ShadeY, else Shade
  else:     d = distance from p to the nearest Shape edge (or Stroke distance - StrokeWidth/2); inside a Hole, the
            Hole's edges count too. Ink if d <= the outline width, else transparent
The colors (sRGB) are INK, LIGHT and SHADE below; the texture stores them (premultiplied average, then straight
alpha), so the brush tint stays white (a grey tint dims an icon, alpha fades it).
"""
import math
import os
import struct
import zlib

import numpy as np

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DATA_OUT = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'UI', 'Style', 'InkedIconData.inl')
PREVIEW_OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Icons')

INK = (0x0A, 0x12, 0x18)
LIGHT = (0xF4, 0xEF, 0xE6)
SHADE = (0x8E, 0xA3, 0xB4)
SUPERSAMPLE = 4
MIN_OUTLINE_PIXELS = 2.6    # in texture pixels: about 1.3 px on screen with the usual texture at twice the drawn size
PAD = 1.8                   # the view box leaves this many outline widths round the shapes


# --- Geometry helpers (SVG space: x right, y down) ---

def quad(p0, p1, p2, n=8):
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]) for t in (k / n for k in range(n + 1))]


def rect(x0, x1, y0, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def flip(points):
    """Gun space (u along, v up, cm) to icon space (x right, y down)."""
    return [(u, -v) for u, v in points]


def circle(cx, cy, r, n=20):
    return [(cx + r * math.cos(2 * math.pi * k / n), cy + r * math.sin(2 * math.pi * k / n)) for k in range(n)]


class Icon:
    """One icon (see the module's docstring). shade is the ShadeY line, outline the ink line's width."""

    def __init__(self, name, shapes, holes=(), strokes=(), cuts=(), shade=0.0, outline=1.0, stroke_width=0.6,
                 cut_width=0.35):
        self.name, self.shapes, self.holes = name, [list(s) for s in shapes], [list(h) for h in holes]
        self.strokes, self.cuts = [list(s) for s in strokes], [list(c) for c in cuts]
        self.shade, self.outline, self.stroke_width, self.cut_width = shade, outline, stroke_width, cut_width
        self.view = (0.0, 0.0)
        self.max_outline = outline

    def points(self):
        return [p for s in self.shapes for p in s] + [p for s in self.strokes for p in s]

    def bounds(self):
        pts = self.points()
        grow = self.stroke_width * 0.5 if self.strokes else 0.0
        return (min(x for x, _ in pts) - grow, min(y for _, y in pts) - grow,
                max(x for x, _ in pts) + grow, max(y for _, y in pts) + grow)

    def place(self, box=None):
        """Moves the icon into a view box from (0, 0) that leaves PAD outline widths round box (default: its own
        bounds; a group of icons shares one box so they line up)."""
        x0, y0, x1, y1 = box or self.bounds()
        pad = self.outline * PAD
        dx, dy = pad - x0, pad - y0
        move = lambda poly: [(x + dx, y + dy) for x, y in poly]
        self.shapes, self.holes = [move(s) for s in self.shapes], [move(h) for h in self.holes]
        self.strokes, self.cuts = [move(s) for s in self.strokes], [move(c) for c in self.cuts]
        self.shade += dy
        self.view = (x1 - x0 + 2 * pad, y1 - y0 + 2 * pad)
        self.max_outline = pad - 0.15 * self.outline
        return self


# --- The assault rifle: the bullpup (Standard body, carbine barrel, birdcage, 30 rounds, red dot, butt pad) ---

def bullpup():
    v0 = 6.15
    shell = [(1.2, -9.6), (0, -8.6), (0, 2.0), (2, 3.4), (40, 3.2), (47, 2.4), (51, 0.6), (51.5, -1.8), (49, -3.2),
             (38.5, -3.5), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3, -8.6)]
    loop = [(36.8, -3.8), (37.6, -12.0), (36.4, -14.0), (34, -14.8), (25, -15.2), (22.0, -14.8), (20.6, -12.5),
            (19.6, -7.0), (19.4, -3.8)]
    loop_hole = [(25.0, -5.8), (34.4, -5.8), (35.4, -7.0), (35.6, -11.8), (34.2, -13.0), (27.0, -13.0), (25.4, -11.8),
                 (24.2, -7.2)]
    mount = [(12, 3.0), (34, 3.0), (34, 4.4), (32.5, 5.2), (13.5, 5.2), (12, 4.4)]
    front = quad((17.6, -5.6), (18.3, -15.0), (19.8, -24.0))
    rear = quad((10.4, -5.6), (10.9, -15.0), (12.0, -23.4))
    mag = front + [(20.05, -24.8), (12.25, -24.2)] + list(reversed(rear))
    trigger = [(26.4, -5.8), (27.4, -5.8), (27.2, -7.0), (26.8, -8.0), (26.2, -8.2), (26.5, -7.0)]
    shapes = [flip(p) for p in (
        shell, loop, mount, rect(14, 32, 5.2, 6.15), rect(19.4, 26.6, v0, v0 + 1.3),
        [(19, v0 + 1.35), (20, v0 + 1.35), (20.4, v0 + 1.55), (25.6, v0 + 1.55), (26, v0 + 1.35), (27, v0 + 1.35),
         (27, v0 + 4.25), (26, v0 + 4.25), (25.6, v0 + 4.05), (20.4, v0 + 4.05), (20, v0 + 4.25), (19, v0 + 4.25)],
        mag, rect(-1.5, 0.2, -8.8, 2.0), rect(50.6, 51.9, -1.25, 1.25), rect(51.9, 63.9, -0.8, 0.8),
        rect(54.9, 57.5, -1.0, 1.5), rect(63.9, 69.7, -0.98, 0.98), [(38.8, 2.9), (44.0, 2.6), (44.0, 3.6), (39.6, 3.9)],
        trigger)]
    cuts = [flip(p) for p in (
        [(8, -4.8), (18.2, -4.8), (19.4, 1.6), (9.4, 1.6), (8, -4.8)],      # the side plate
        [(12, 3.0), (34, 3.0)],                                           # the mount's seam
        [(40.6, -1.6), (41.9, 1.2)], [(42.8, -1.6), (44.1, 1.2)], [(45.0, -1.6), (46.3, 1.2)],   # the vents
        [(0.2, -8.6), (0.2, 2.0)],                                        # the pad's seam
        [(65.6, -0.98), (65.6, 0.98)], [(67.6, -0.98), (67.6, 0.98)],     # the birdcage's slots
        [(11.0, -14.0), (18.6, -14.0)],                                   # a rib on the magazine
        [(21.0, -5.2), (36.8, -4.6)])]                                    # where the grip loop meets the shell
    return Icon('Rifle', shapes, [flip(loop_hole)], cuts=cuts, shade=1.8, outline=OUTLINE['Rifle'], cut_width=0.35)


# --- The shotgun: the Ranchhand (Standard body, field barrel, crown, 6-shell tube, flip sight, skeleton stock) ---

def ranchhand():
    v0 = 3.15
    stock = [(0, 1.8), (0, -3.0), (-2.5, -4.4), (-5.5, -7.4), (-9, -8.0), (-12, -7.4), (-37, -12.6), (-38.5, -12.4),
             (-38.5, 0.2), (-36, 0.6), (-14, 1.0), (-4, 1.8)]
    stock_hole = [(-16.5, -8.0), (-33.0, -10.9), (-34.2, -10.0), (-34.0, -6.4), (-32.5, -5.6), (-18.0, -4.9)]
    pump = [(28, -1.4), (46, -1.4), (47, -2.4), (46.5, -4.0), (45, -4.4), (29, -4.4), (27.5, -3.6), (27.5, -2.0)]
    shapes = [flip(p) for p in (
        [(0, -3.0), (20, -3.0), (20, 1.4), (18.5, 2.2), (1.5, 2.2), (0, 1.6)], rect(1, 19.5, 2.2, 3.15),
        [(3.4, v0), (6.4, v0), (6.0, v0 + 2.05), (4.4, v0 + 2.05)],
        [(3, -3.0), (13, -3.0), (12.4, -4.6), (4, -4.4)],
        [(7.4, -4.4), (8.4, -4.4), (8.2, -5.6), (7.8, -6.6), (7.2, -6.8), (7.5, -5.6)],
        rect(20, 66, -1.15, 1.15), rect(20, 66, 1.3, 1.85), rect(20, 54, -3.55, -1.45),
        [(54, -3.7), (55.6, -3.7), (56, -3.4), (56, -1.6), (55.6, -1.3), (54, -1.3)],
        [(52.5, -3.4), (54.5, -3.4), (54.5, 0.9), (52.5, 0.9)], pump, stock,
        [(-38.4, -12.7), (-40.2, -12.6), (-40.2, 0.4), (-38.4, 0.3)])] + [circle(65.2, -2.15, 0.35, 10)]
    holes = [flip(stock_hole), circle(5.2, -(v0 + 1.6), 0.3, 10)]
    strokes = [flip([(4.6, -4.4), (5.0, -6.8), (7.0, -7.6), (11.0, -7.2), (12.4, -4.6)])]   # the trigger guard
    cuts = [flip(p) for p in (
        [(31, -2.2), (44, -2.2)], [(31, -2.9), (44, -2.9)], [(31, -3.6), (44, -3.6)],       # the pump's grooves
        [(2, -2.4), (17, -2.4), (18, 1.2), (3, 1.2), (2, -2.4)],                             # the side plate
        [(-0.4, -3.0), (-0.4, 1.8)], [(-38.4, -12.5), (-38.4, 0.3)])]                          # stock seams
    cuts += [flip([(u, 1.15), (u, 1.3)]) for u in range(23, 65, 3)]                           # the rib's vents
    return Icon('Shotgun', shapes, holes, strokes, cuts, shade=-0.6, outline=OUTLINE['Shotgun'], stroke_width=0.45,
                cut_width=0.3)


# --- Ammo, drawn in a 64 x 64 box, told apart by shape and count (never color) ---

def cartridge(cx, base, w, case_top, shoulder_top, neck_w, neck_top, tip, nose):
    """A cartridge standing up: rim and extractor groove, case, shoulder and neck (none when neck_w = w), the bullet
    (nose 'point', 'round' or 'flat'). Returns its outline and its cut lines (the case mouth, the groove)."""
    hw, nw = w / 2, neck_w / 2
    bw = nw - 0.25
    right = [(cx + hw, base), (cx + hw, base - 2.0), (cx + hw - 0.9, base - 2.6), (cx + hw - 0.9, base - 3.4),
             (cx + hw, base - 4.2), (cx + hw, case_top)]
    if nw < hw:
        right += [(cx + nw, shoulder_top)]
    right += [(cx + nw, neck_top), (cx + bw, neck_top)]
    n = 10
    for k in range(1, n + 1):
        t = k / n
        y = neck_top - (neck_top - tip) * t
        if nose == 'point':
            x = bw * (1 - t) ** 0.62
        elif nose == 'round':
            x = bw * math.sqrt(max(0.0, 1 - t ** 2))
        else:
            x = bw * (1 - 0.45 * t ** 2)
        right.append((cx + x, y))
    left = [(2 * cx - x, y) for x, y in reversed(right)]
    outline = right + left
    cuts = [[(cx - bw, neck_top), (cx + bw, neck_top)], [(cx - hw, base - 2.0), (cx + hw, base - 2.0)]]
    return outline, cuts


def shell(cx, base, w, top):
    """A shotgun shell: brass head with its rim, the hull, a crimped top."""
    hw = w / 2
    outline = [(cx - hw - 0.6, base), (cx + hw + 0.6, base), (cx + hw + 0.6, base - 1.8), (cx + hw, base - 2.4),
               (cx + hw, top + 2.5), (cx + hw - 1.2, top + 0.4), (cx + 2.0, top), (cx - 2.0, top), (cx - hw + 1.2, top + 0.4),
               (cx - hw, top + 2.5), (cx - hw, base - 2.4), (cx - hw - 0.6, base - 1.8)]
    head = base - 10
    cuts = [[(cx - hw, head), (cx + hw, head)], [(cx - hw, base - 2.4), (cx + hw, base - 2.4)],
            [(cx - 3.2, top + 1.2), (cx - 1.0, top + 4.0)], [(cx + 3.2, top + 1.2), (cx + 1.0, top + 4.0)],
            [(cx, top + 0.6), (cx, top + 3.8)]]
    return outline, cuts


# Each ammo type's rounds in the 64 x 64 box: ('cartridge', its arguments) or ('shell', its arguments). The 3D ammo
# pickups (Art/Models/Loot/Ammo.py) turn these same outlines round their axes, so the pickup is the icon.
ROUNDS = {
    'AssaultRifle': [('cartridge', (cx, 58, 11.0, 33, 29, 7.0, 25, 10, 'point')) for cx in (24.0, 40.0)],
    'Shotgun': [('shell', (32, 59, 21.0, 10))],
    'Pistol': [('cartridge', (32, 57, 17.0, 37, 37, 17.0, 37, 20, 'round'))],
    'SMG': [('cartridge', (cx, 57, 12.0, 40, 40, 12.0, 40, 28, 'flat')) for cx in (16.5, 32.0, 47.5)],
    'Sniper': [('cartridge', (32, 61, 12.0, 27, 21, 7.2, 15, 2.5, 'point'))],
}
SHADE_LINE = {'AssaultRifle': 45, 'Shotgun': 49, 'Pistol': 48, 'SMG': 49, 'Sniper': 48}


def ammo(kind):
    """EAmmoType's icon: two pointed rounds (assault rifle), a shell, one round-nosed round (pistol), three flat-nosed
    rounds (SMG), one tall pointed round (sniper)."""
    parts = [cartridge(*args) if what == 'cartridge' else shell(*args) for what, args in ROUNDS[kind]]
    return Icon('Ammo' + kind, [p[0] for p in parts], cuts=[c for p in parts for c in p[1]], shade=SHADE_LINE[kind],
                outline=OUTLINE['Ammo'], cut_width=1.1)


# The ink line's width in each icon's units: guns scale it with their length so every gun's line looks the same
# when they're drawn the same width; the ammo icons share one box and one width.
OUTLINE = {'Rifle': 1.72, 'Shotgun': 2.56, 'Ammo': 3.2}
AMMO_TYPES = ('AssaultRifle', 'Shotgun', 'Pistol', 'SMG', 'Sniper')   # EAmmoType order


def icons():
    """Every icon, placed in its view box: each gun in its own, the ammo types all in one shared box."""
    guns = [bullpup().place(), ranchhand().place()]
    rounds = [ammo(kind) for kind in AMMO_TYPES]
    boxes = [r.bounds() for r in rounds]
    shared = (min(b[0] for b in boxes), min(b[1] for b in boxes), max(b[2] for b in boxes), max(b[3] for b in boxes))
    side = max(shared[2] - shared[0], shared[3] - shared[1])
    cx, cy = (shared[0] + shared[2]) / 2, (shared[1] + shared[3]) / 2
    square = (cx - side / 2, cy - side / 2, cx + side / 2, cy + side / 2)
    return guns + [r.place(square) for r in rounds]


# --- The rules the game draws by (see the module's docstring), for the reference pictures ---

def inside(X, Y, poly):
    """Even-odd point in polygon."""
    result = np.zeros(X.shape, bool)
    for (ax, ay), (bx, by) in zip(poly, poly[1:] + poly[:1]):
        if ay == by:
            continue
        crosses = (ay > Y) != (by > Y)
        result ^= crosses & (X < ax + (Y - ay) * (bx - ax) / (by - ay))
    return result


def segment_distance(X, Y, a, b):
    (ax, ay), (bx, by) = a, b
    dx, dy = bx - ax, by - ay
    t = np.clip(((X - ax) * dx + (Y - ay) * dy) / max(dx * dx + dy * dy, 1e-12), 0.0, 1.0)
    return np.hypot(X - (ax + t * dx), Y - (ay + t * dy))


def line_distance(X, Y, points, closed):
    d = np.full(X.shape, np.inf)
    pts = points + points[:1] if closed else points
    for a, b in zip(pts, pts[1:]):
        d = np.minimum(d, segment_distance(X, Y, a, b))
    return d


def draw(icon, ppu):
    """The icon's texture at ppu pixels per unit, as an H x W x 4 uint8 array (straight alpha)."""
    w, h = math.ceil(icon.view[0] * ppu), math.ceil(icon.view[1] * ppu)
    s = SUPERSAMPLE
    X, Y = np.meshgrid((np.arange(w * s) + 0.5) / (s * ppu), (np.arange(h * s) + 0.5) / (s * ppu))
    outline = min(max(icon.outline, MIN_OUTLINE_PIXELS / ppu), icon.max_outline)
    shape = np.zeros(X.shape, bool)
    d = np.full(X.shape, np.inf)
    for poly in icon.shapes:
        shape |= inside(X, Y, poly)
        d = np.minimum(d, line_distance(X, Y, poly, True))
    for line in icon.strokes:
        sd = line_distance(X, Y, line, False) - icon.stroke_width * 0.5
        shape |= sd <= 0.0
        d = np.minimum(d, sd)
    hole = np.zeros(X.shape, bool)
    for poly in icon.holes:
        here = inside(X, Y, poly)
        hole |= here
        d = np.where(here, np.minimum(d, line_distance(X, Y, poly, True)), d)
    solid = shape & ~hole
    cut = np.zeros(X.shape, bool)
    for line in icon.cuts:
        cut |= line_distance(X, Y, line, False) <= icon.cut_width * 0.5
    ink = (solid & cut) | (~solid & (d <= outline))
    light = solid & ~cut & (Y < icon.shade)
    shade = solid & ~cut & (Y >= icon.shade)
    rgb = np.zeros(X.shape + (3,))
    for mask, color in ((ink, INK), (light, LIGHT), (shade, SHADE)):
        rgb[mask] = color
    alpha = (ink | light | shade).astype(float)
    # Average each pixel's points: color premultiplied by coverage, then back to straight alpha.
    pre = (rgb * alpha[..., None]).reshape(h, s, w, s, 3).mean(axis=(1, 3))
    a = alpha.reshape(h, s, w, s).mean(axis=(1, 3))
    color = np.where(a[..., None] > 0, pre / np.maximum(a[..., None], 1e-6), 0.0)
    return np.dstack([np.clip(color + 0.5, 0, 255), np.clip(a * 255 + 0.5, 0, 255)]).astype(np.uint8)


def write_png(path, rgba):
    h, w, _ = rgba.shape
    raw = b''.join(b'\x00' + rgba[y].tobytes() for y in range(h))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


# --- The C++ data ---

def cpp_points(poly):
    return '{ ' + ', '.join(f'{{ {x:.2f}, {y:.2f} }}' for x, y in poly) + ' }'


def cpp_list(name, polys):
    if not polys:
        return ''
    rows = ',\n'.join('\t\t\t' + cpp_points(p) for p in polys)
    return f'\t\tIcon.{name} = {{\n{rows}\n\t\t}};\n'


def cpp_icon(icon):
    return (f'\tinline LooterUI::FInkedIcon {icon.name}()\n\t{{\n'
            f'\t\tLooterUI::FInkedIcon Icon;\n'
            f'\t\tIcon.ViewBox = FVector2D({icon.view[0]:.2f}, {icon.view[1]:.2f});\n'
            f'\t\tIcon.ShadeY = {icon.shade:.2f}f;\n'
            f'\t\tIcon.OutlineWidth = {icon.outline:.2f}f;\n'
            f'\t\tIcon.MaxOutlineWidth = {icon.max_outline:.2f}f;\n'
            f'\t\tIcon.StrokeWidth = {icon.stroke_width:.2f}f;\n'
            f'\t\tIcon.CutWidth = {icon.cut_width:.2f}f;\n'
            + cpp_list('Shapes', icon.shapes) + cpp_list('Holes', icon.holes) + cpp_list('Strokes', icon.strokes)
            + cpp_list('Cuts', icon.cuts) + '\t\treturn Icon;\n\t}\n')


def write_cpp(all_icons):
    body = '\n'.join(cpp_icon(i) for i in all_icons)
    text = ('// Generated by Art/Icons/InkedIcons.py: change the outlines there and run it again; don\'t edit this file.\n'
            '// The game\'s Inked icons (the user\'s pick, 2026-10-01). What the fields mean and how an icon is drawn are in\n'
            '// that file\'s docstring. Colors: ink #{:02X}{:02X}{:02X}, light #{:02X}{:02X}{:02X}, shade #{:02X}{:02X}{:02X}; '
            'supersample {}x{}, minimum ink line {} texture pixels.\n'.format(*INK, *LIGHT, *SHADE, SUPERSAMPLE,
                                                                               SUPERSAMPLE, MIN_OUTLINE_PIXELS)
            + '#pragma once\n\n// Include after LooterUI::FInkedIcon is declared (UI/Style/LooterUIStyle.h).\nnamespace InkedIconData\n{\n'
            + body + '}\n')
    with open(DATA_OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write(text)
    return DATA_OUT


# --- The reference pictures ---

# Boxes to check, in screen pixels (each icon fitted inside, its texture drawn at twice that as the game does): the
# HUD slot, the inventory's list, drag ghost and card; the ammo gauge, the readout and the icon over an ammo box.
GUN_SIZES = ((52, 26), (84, 28), (120, 40), (186, 62))
AMMO_SIZES = ((18, 18), (26, 26), (34, 34), (48, 48))


def picture(icon, box):
    """The icon fitted inside box (screen pixels), drawn at twice that like the game's textures."""
    return draw(icon, 2.0 * min(box[0] / icon.view[0], box[1] / icon.view[1]))


def write_previews(all_icons):
    os.makedirs(PREVIEW_OUT, exist_ok=True)
    files = {}
    for icon in all_icons:
        sizes = AMMO_SIZES if icon.name.startswith('Ammo') else GUN_SIZES
        for px in sizes:
            image = picture(icon, px)
            name = f'Inked_{icon.name}_{px[0]}.png'
            write_png(os.path.join(PREVIEW_OUT, name), image)
            files[(icon.name, px)] = (name, image.shape[1] // 2, image.shape[0] // 2)
        big = picture(icon, (150, 150) if icon.name.startswith('Ammo') else (440, 200))
        name = f'Inked_{icon.name}_big.png'
        write_png(os.path.join(PREVIEW_OUT, name), big)
        files[(icon.name, 'big')] = (name, big.shape[1] // 2, big.shape[0] // 2)
    return files


def write_sheet(all_icons, files):
    shot = 'file:///' + REPO.replace('\\', '/') + '/Saved/Screenshots/Tour/Slimes_Medium.png'
    fonts = 'file:///' + REPO.replace('\\', '/') + '/Content/UI/Fonts'

    def img(key):
        name, w, h = files[key]
        return f'<img src="{name}" width="{w}" height="{h}">'
    guns = [i for i in all_icons if not i.name.startswith('Ammo')]
    rounds = [i for i in all_icons if i.name.startswith('Ammo')]
    big = ''.join(f'<div class="cell">{img((i.name, "big"))}<span>{i.name}</span></div>' for i in guns)
    big_ammo = ''.join(f'<div class="cell">{img((i.name, "big"))}<span>{i.name[4:]}</span></div>' for i in rounds)
    rows = ''
    for bg_name, bg in (('PANEL', '#0E1A24'), ('WORLD', f'url({shot}) -980px -520px')):
        cells = ''.join(f'<div class="s">{img((i.name, px))}</div>' for i in guns for px in GUN_SIZES)
        cells += ''.join(f'<div class="s">{img((i.name, px))}</div>' for i in rounds for px in AMMO_SIZES)
        rows += f'<div class="sec">AT GAME SIZES ON {bg_name}</div><div class="strip" style="background:{bg}">{cells}</div>'
    html = f"""<!doctype html><html><head><meta charset="utf-8"><style>
@font-face {{ font-family: Chakra; src: url('{fonts}/ChakraPetch-Bold.ttf'); font-weight: 700; }}
html, body {{ margin: 0; width: 1600px; background: #0A0E13; font-family: Chakra; color: #DCEFFF; }}
.head {{ padding: 26px 48px 10px; border-bottom: 2px solid #FF9F1C; font-size: 40px; font-weight: 700; }}
.head small {{ color: #5ACAFF; font-size: 20px; letter-spacing: 2px; margin-left: 18px; }}
.row {{ display: flex; flex-wrap: wrap; gap: 26px; padding: 18px 48px; align-items: flex-end;
  background: linear-gradient(180deg, #56636F, #3A4651); margin: 10px 48px; }}
.cell {{ display: flex; flex-direction: column; align-items: center; gap: 8px; }}
.cell span {{ color: #DCEFFF; font-size: 18px; font-weight: 700; letter-spacing: 2px; }}
.sec {{ padding: 14px 48px 4px; color: #8FB3CC; font-weight: 700; font-size: 18px; letter-spacing: 4px; }}
.strip {{ display: flex; flex-wrap: wrap; align-items: center; gap: 18px; padding: 16px 20px; margin: 0 48px; }}
</style></head><body>
<div class="head">Inked icons<small>REFERENCE FOR THE GAME'S RASTERIZER</small></div>
<div class="row">{big}</div><div class="row">{big_ammo}</div>{rows}
</body></html>"""
    path = os.path.join(PREVIEW_OUT, 'InkedIcons.html')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(html)
    return path


def main():
    all_icons = icons()
    print('wrote', write_cpp(all_icons))
    files = write_previews(all_icons)
    print('wrote', write_sheet(all_icons, files))
    for i in all_icons:
        print(f'{i.name}: view {i.view[0]:.2f} x {i.view[1]:.2f}, {len(i.shapes)} shapes, {len(i.holes)} holes, '
              f'{len(i.strokes)} strokes, {len(i.cuts)} cuts, {sum(len(p) for p in i.shapes)} points')


if __name__ == '__main__':
    main()
