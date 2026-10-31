"""The game's weapon, ammo and grenade icons in the Inked style, the user's pick on 2026-10-01 (the five concepts are in
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


# --- The revolver: the Drover (Blued frame, service barrel, fluted six-shot cylinder, notch sight, walnut plow handle) ---

def drover():
    rake = lambda pts: [(u + 0.15 * min(v + 3.6, 0.0), v) for u, v in pts]   # the grips lean back as in Drover.py
    frame = [(-0.4, -3.9), (2.0, -3.9), (7.3, -3.85), (8.4, -3.2), (9.0, -1.05), (9.2, -0.95), (9.2, 1.3), (8.7, 1.8),
             (1.2, 1.8), (0.5, 1.5), (-0.1, 0.6), (-0.6, -0.8), (-0.6, -3.0)]
    cylinder = [(2.1, -3.1), (2.32, -3.35), (6.15, -3.35), (6.45, -3.05), (6.45, 0.65), (6.15, 0.95), (2.32, 0.95),
                (2.1, 0.73)]
    hammer = [(-0.55, -1.3), (0.45, -1.3), (0.6, 0.6), (0.45, 1.5), (0.0, 2.0), (-0.8, 2.15), (-1.6, 2.35), (-2.3, 2.55),
              (-2.6, 2.35), (-2.4, 2.05), (-1.6, 1.65), (-1.0, 1.2), (-0.8, 0.3)]
    grip = rake([(2.15, -3.7), (1.95, -4.6), (1.4, -6.6), (0.75, -9.0), (0.35, -10.9), (0.15, -11.5), (-0.3, -12.05),
                 (-1.2, -12.3), (-2.6, -12.3), (-3.7, -11.9), (-4.3, -11.1), (-4.15, -9.2), (-3.5, -6.8), (-2.6, -4.6),
                 (-2.1, -3.7)])
    tang = rake([(-0.3, -3.95), (-2.15, -3.95), (-1.3, -2.6), (-0.75, -2.15), (-0.5, -2.3)])
    trigger = [(3.7, -3.8), (4.6, -3.8), (4.55, -4.7), (4.2, -5.6), (3.7, -6.0), (3.9, -4.9)]
    bow = quad((2.5, -3.75), (2.55, -6.9), (4.6, -6.85)) + quad((4.6, -6.85), (7.0, -6.8), (7.1, -3.8))[1:]
    shapes = [flip(p) for p in (
        frame, cylinder, hammer, grip, tang, trigger,
        rect(9.2, 23.2, -0.9, 0.9), rect(9.2, 10.1, -1.08, 1.08), rect(9.8, 23.1, 0.5, 1.4),          # barrel, collar, rib
        [(21.3, 1.35), (22.9, 1.35), (22.9, 2.25), (22.3, 2.25)],                                   # the front blade
        [(1.0, 1.8), (2.7, 1.8), (2.7, 2.25), (1.3, 2.25)],                                         # the notch sight
        rect(6.45, 10.15, -1.46, -0.94), rect(10.15, 11.05, -1.56, -0.84),                           # the ejector rod
        [(11.0, -0.6), (11.95, -0.6), (11.95, -1.25), (11.45, -1.7), (11.0, -1.7)])]                # its latch
    strokes = [flip(bow)]                                                                            # the trigger guard
    cuts = [flip(p) for p in (
        cylinder + cylinder[:1],                                                     # the cylinder in its window
        [(3.45, 0.38), (5.45, 0.38)], [(3.3, -1.2), (5.6, -1.2)], [(3.45, -2.78), (5.45, -2.78)],   # its flutes
        [(9.2, -0.95), (9.2, 1.3)],                                                  # where the barrel meets the frame
        [(-0.45, -3.85), (2.1, -3.8)])]                                              # the grip under the frame
    return Icon('Revolver', shapes, [], strokes, cuts, shade=1.0, outline=OUTLINE['Revolver'], stroke_width=0.42,
                cut_width=0.16)


# --- The four frontier-tech guns (2026-10-21), traced from their models (Art/Models/Weapons/<Gun>.py) and side previews ---

def rustler():
    """The SMG: the Rustler (Standard upper, standard shroud, cap, the 40-round helix, a small optic, the wire stock).
    The helix's spiral window reads as slanted cuts across the round magazine under the barrel."""
    upper = [(0, 3.4), (29.0, 3.4), (31.1, 1.6), (31.1, -2.6), (0, -2.6)]
    housing = [(1.5, -2.6), (14.8, -2.6), (14.8, -4.0), (2.5, -4.2)]
    grip = [(2.6, -4.0), (9.5, -4.0), (8.4, -8.0), (7.3, -13.0), (5.6, -14.0), (2.0, -13.7), (1.5, -12.5), (3.0, -8.0)]
    trigger = [(11.0, -4.2), (12.0, -4.2), (11.9, -5.2), (11.5, -6.2), (10.9, -6.6), (11.2, -5.4)]
    helix = [(15.1, -2.85), (35.5, -2.85), (36.8, -3.6), (37.3, -5.4), (36.8, -7.2), (35.5, -7.95), (15.1, -7.95)]
    optic = [(12.3, 5.5), (21.0, 5.5), (22.0, 6.2), (22.0, 7.9), (21.0, 8.6), (12.3, 8.6)]
    shapes = [flip(p) for p in (
        upper, housing, grip, trigger, helix, optic,
        rect(31.1, 38.8, -1.75, 1.75), rect(38.8, 40.9, -0.75, 0.75), rect(40.9, 41.8, -0.55, 0.55),   # shroud, cap, tip
        rect(7.2, 25.0, 3.4, 4.4), rect(13.5, 19.6, 4.4, 5.5),                                         # rail, mount
        rect(-20.5, -18.5, -7.7, 2.75))]                                                               # the butt pad
    strokes = [flip(p) for p in (
        [(-18.6, 2.2), (0.2, 2.2)], [(-18.6, -6.6), (0.2, -1.8)],          # the wire stock's struts
        [(9.5, -4.2), (9.8, -6.6), (12.5, -7.4), (14.5, -5.5), (15.0, -4.0)])]   # the trigger guard
    cuts = [flip(p) for p in (
        [(13.3, 0.15), (18.8, 0.15), (18.8, 1.85), (13.3, 1.85), (13.3, 0.15)],   # the ejection port
        [(4.0, -1.25), (24.0, -1.25)],                                            # the accent line
        [(31.1, -1.75), (31.1, 1.6)], [(38.8, -0.75), (38.8, 0.75)],              # shroud joints
        [(19.0, 6.2), (19.0, 7.9)])]                                              # the optic's ring
    cuts += [flip([(u, -2.85), (u + 1.6, -7.95)]) for u in (17.0, 19.6, 22.2, 24.8, 27.4, 30.0, 32.6)]   # the spiral
    return Icon('SMG', shapes, [], strokes, cuts, shade=0.5, outline=OUTLINE['SMG'], stroke_width=0.7, cut_width=0.3)


def longhorn():
    """The sniper rifle: the Longhorn (Standard chassis with its thumbhole, fluted barrel, brake, 5-round box, the long
    Longhorn scope, a pad on the butt, the bolt's knob)."""
    chassis = [(-15.2, 2.6), (2.0, 2.6), (4.6, 0.3), (6.5, -1.0), (65.0, -1.1), (66.2, -2.2), (66.0, -4.6), (64.8, -5.4),
               (26.6, -5.4), (25.4, -6.2), (22.9, -12.6), (21.6, -13.5), (17.6, -13.6), (16.8, -12.5), (14.0, -11.6),
               (-15.2, -10.1)]
    thumbhole = [(7.0, -4.2), (13.0, -3.6), (16.4, -4.8), (16.4, -7.2), (13.0, -8.0), (7.0, -7.4), (6.0, -5.8)]
    scope = [(13.5, 8.3), (19.5, 8.3), (21.0, 7.4), (38.0, 7.4), (39.5, 8.4), (47.0, 8.4), (47.0, 3.8), (39.5, 3.8),
             (38.0, 4.8), (21.0, 4.8), (19.5, 3.9), (13.5, 3.9)]
    trigger = [(29.4, -5.4), (30.4, -5.4), (30.2, -6.6), (29.8, -7.6), (29.2, -7.8), (29.5, -6.6)]
    shapes = [flip(p) for p in (
        chassis, scope, trigger,
        rect(-17.4, -15.2, -10.2, 2.8),                                       # the butt pad
        rect(13.4, 44.0, -1.1, 1.2), rect(21.0, 42.0, 1.2, 2.3),             # the receiver and its rail
        rect(24.0, 26.0, 2.3, 4.0), rect(36.0, 38.0, 2.3, 4.0),              # the scope's rings
        rect(29.0, 33.0, 7.4, 9.6),                                           # its turret
        rect(44.0, 60.0, -0.9, 0.9), rect(60.0, 101.5, -0.7, 0.7),           # the barrel, stepped
        rect(101.5, 108.5, -1.0, 1.0), rect(108.5, 110.0, -0.8, 0.8),        # the brake and its brass tip
        rect(34.5, 41.2, -8.4, -5.4),                                         # the box
        rect(44.5, 62.0, -6.8, -6.0),                                         # the folded bipod
        circle(17.95, 1.6, 0.95, 12))]                                        # the bolt's knob
    holes = [flip(thumbhole)]
    strokes = [flip([(27.5, -5.4), (27.8, -8.6), (31.5, -8.9), (33.4, -5.4)])]   # the trigger guard
    cuts = [flip(p) for p in (
        [(47.0, -3.6), (63.0, -3.6)],                                         # the accent line
        [(-15.2, -10.1), (-15.2, 2.6)],                                       # the pad's seam
        [(19.5, 3.9), (19.5, 8.3)], [(39.5, 3.8), (39.5, 8.4)],              # the scope's bells
        [(102.8, -1.0), (102.8, 1.0)], [(104.5, -1.0), (104.5, 1.0)], [(106.2, -1.0), (106.2, 1.0)],   # the brake's ports
        [(17.95, -0.6), (17.95, 1.2)])]                                       # the knob's stem
    return Icon('Sniper', shapes, holes, strokes, cuts, shade=0.6, outline=OUTLINE['Sniper'], stroke_width=0.6, cut_width=0.45)


def thresher():
    """The LMG: the Thresher (Standard receiver under its sand top cover, carry handle with a reflex sight, the 100-round
    belt box, standard barrel with its heat rings, bipod down, fixed stock)."""
    stock = [(0, 2.4), (-21.5, 2.4), (-21.5, -7.6), (-14.0, -6.5), (0, -4.6)]
    cover = [(0.4, 2.6), (0.9, 4.4), (2.0, 5.0), (34.6, 5.0), (36.4, 4.0), (36.8, 2.6)]
    handle = [(11.0, 5.0), (13.0, 5.0), (13.0, 7.4), (25.0, 7.4), (25.0, 5.0), (27.0, 5.0), (27.0, 8.6), (11.0, 8.6)]
    grip = [(3.5, -4.6), (9.5, -4.6), (8.6, -9.0), (7.8, -13.6), (6.2, -14.4), (3.0, -14.0), (2.4, -12.6), (4.0, -9.0)]
    trigger = [(11.0, -4.6), (12.0, -4.6), (11.9, -5.6), (11.5, -6.6), (10.9, -7.0), (11.2, -5.8)]
    box = [(15.0, -4.6), (27.0, -4.6), (27.0, -12.4), (26.2, -13.2), (15.8, -13.2), (15.0, -12.4)]
    guard = [(38.0, -0.5), (50.0, -0.5), (50.5, -1.6), (50.0, -4.4), (38.0, -4.4)]
    shapes = [flip(p) for p in (
        stock, rect(-23.2, -21.5, -7.9, 2.7), rect(0, 38.0, -4.6, 3.0), cover, rect(36.8, 39.5, -4.6, 3.4), handle,
        rect(17.0, 21.5, 8.6, 11.0), grip, trigger, box, guard,
        rect(39.5, 72.0, -1.25, 1.25), rect(72.0, 76.0, -1.4, 1.4), rect(76.0, 77.0, -1.0, 1.0))]   # barrel, muzzle, tip
    shapes += [flip(rect(u, u + 1.1, -1.9, 1.9)) for u in (53.0, 56.0, 59.0, 62.0)]                # the heat rings
    strokes = [flip(p) for p in (
        [(10.0, -4.6), (10.2, -7.2), (12.8, -7.8), (14.6, -6.2), (15.0, -4.6)],   # the trigger guard
        [(46.5, -4.4), (50.5, -14.0)], [(47.5, -4.4), (54.0, -13.4)])]           # the bipod's legs
    cuts = [flip(p) for p in (
        [(0.4, 2.6), (36.8, 2.6)],                                              # the cover's seam
        [(2.0, -1.0), (34.0, -1.0)],                                            # the accent line
        [(16.0, -7.5), (26.0, -7.5)],                                           # the box's lid seam
        [(0.0, -4.6), (0.0, 2.4)], [(-21.5, -7.6), (-21.5, 2.4)],             # stock seams
        [(39.5, -1.25), (39.5, 1.25)], [(72.0, -1.4), (72.0, 1.4)])]            # barrel joints
    return Icon('LMG', shapes, [], strokes, cuts, shade=0.5, outline=OUTLINE['LMG'], stroke_width=0.6, cut_width=0.4)


def powderkeg():
    """The launcher: the Powderkeg (Standard tube between its sand sleeves, the bell, the brass ring and a frag rocket's
    nose in the mouth, the optic on its rail, the shoulder rest, pistol and front grips)."""
    exhaust = [(0, 6.1), (4.0, 5.4), (10.0, 4.6), (13.9, 4.4), (13.9, -4.4), (10.0, -4.6), (4.0, -5.4), (0, -6.1)]
    bell = [(77.8, 4.4), (85.0, 4.9), (94.4, 6.0), (94.4, -6.0), (85.0, -4.9), (77.8, -4.4)]
    nose = [(95.6, 3.6), (99.5, 2.4), (102.0, 0.9), (103.0, 0), (102.0, -0.9), (99.5, -2.4), (95.6, -3.6)]
    rest = [(18.5, -4.0), (37.8, -4.0), (37.8, -6.6), (36.2, -7.6), (20.4, -7.6), (18.5, -6.0)]
    grip = [(47.0, -4.3), (53.6, -4.3), (52.0, -9.0), (50.6, -14.3), (49.4, -15.1), (46.2, -14.8), (45.6, -13.6), (47.4, -9.0)]
    front = [(68.6, -4.3), (73.6, -4.3), (73.4, -13.6), (72.4, -14.6), (70.0, -14.6), (69.2, -13.6)]
    trigger = [(54.8, -5.0), (55.8, -5.0), (55.6, -6.2), (55.2, -7.2), (54.6, -7.4), (54.9, -6.2)]
    sight = [(38.0, 7.4), (46.6, 7.4), (47.6, 8.0), (47.6, 10.0), (46.6, 10.6), (38.0, 10.6)]
    shapes = [flip(p) for p in (
        exhaust, rect(13.5, 34.7, -5.0, 5.0), rect(34.4, 50.5, -4.3, 4.3), rect(50.5, 77.8, -5.0, 5.0), bell,
        rect(94.4, 95.6, -6.1, 6.1), nose, rest, rect(20.0, 34.0, -8.4, -7.6), grip, front, trigger, sight,
        rect(39.0, 47.0, 4.3, 7.4))]
    strokes = [flip([(53.6, -5.2), (53.9, -7.7), (56.0, -8.6), (58.4, -7.4), (58.6, -5.2)])]   # the trigger guard
    cuts = [flip(p) for p in (
        [(16.0, -5.0), (16.0, 5.0)], [(33.0, -5.0), (33.0, 5.0)],             # the rear sleeve's rings
        [(34.5, -4.3), (34.5, 4.3)], [(50.5, -4.3), (50.5, 4.3)],             # the tube's segment
        [(53.0, -0.8), (72.0, -0.8)],                                          # the ember line
        [(77.8, -4.4), (77.8, 4.4)], [(99.5, -2.4), (99.5, 2.4)],             # the bell's seat, the nose's band
        [(42.0, 7.4), (42.0, 10.6)])]                                          # the optic's ring
    return Icon('Launcher', shapes, [], strokes, cuts, shade=0.5, outline=OUTLINE['Launcher'], stroke_width=0.5, cut_width=0.4)


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


def rocket_body(cx, base, w, nose_base, tip):
    """A rocket standing up (the launcher's, 2026-10-21): the motor's flared nozzle, the body, an ogive warhead to a
    point, as one outline (right half from the nozzle up, then mirrored); its two fins apart (rocket_fins). Returns the
    outline and its cut lines (the warhead's seam, the band under it, the nozzle's joint)."""
    hw = w / 2
    right = [(cx + 3.8, base), (cx + 3.0, base - 2.6), (cx + hw, base - 4.2), (cx + hw, nose_base)]
    n = 10
    for k in range(1, n + 1):
        t = k / n
        right.append((cx + hw * (1 - t) ** 0.7, nose_base - (nose_base - tip) * t))
    left = [(2 * cx - x, y) for x, y in reversed(right)]
    cuts = [[(cx - hw, nose_base), (cx + hw, nose_base)], [(cx - hw, nose_base + 5.0), (cx + hw, nose_base + 5.0)],
            [(cx - hw, base - 4.2), (cx + hw, base - 4.2)]]
    return right + left, cuts


def rocket_fins(cx, base, w):
    """The rocket's two tail fins seen edge-on either side of its body, swept back past the nozzle."""
    hw = w / 2
    right = [(cx + hw - 0.5, base - 15.0), (cx + hw + 7.0, base - 3.5), (cx + hw + 7.0, base + 0.5), (cx + hw - 0.5, base - 5.0)]
    left = [(2 * cx - x, y) for x, y in right]
    return [right, left]


# Each ammo type's rounds in the 64 x 64 box: ('cartridge', its arguments), ('shell', its arguments) or ('rocket', its
# arguments). The 3D ammo pickups (Art/Models/Loot/Ammo.py) turn these same outlines round their axes, so the pickup is
# the icon. The rocket keeps inside the rounds' box, so adding it moved no other icon.
ROUNDS = {
    'AssaultRifle': [('cartridge', (cx, 58, 11.0, 33, 29, 7.0, 25, 10, 'point')) for cx in (24.0, 40.0)],
    'Shotgun': [('shell', (32, 59, 21.0, 10))],
    'Pistol': [('cartridge', (32, 57, 17.0, 37, 37, 17.0, 37, 20, 'round'))],
    'SMG': [('cartridge', (cx, 57, 12.0, 40, 40, 12.0, 40, 28, 'flat')) for cx in (16.5, 32.0, 47.5)],
    'Sniper': [('cartridge', (32, 61, 12.0, 27, 21, 7.2, 15, 2.5, 'point'))],
    'Rocket': [('rocket', (32, 58, 12.0, 21, 5))],
}
SHADE_LINE = {'AssaultRifle': 45, 'Shotgun': 49, 'Pistol': 48, 'SMG': 49, 'Sniper': 48, 'Rocket': 40}


def ammo(kind):
    """EAmmoType's icon: two pointed rounds (assault rifle), a shell, one round-nosed round (pistol), three flat-nosed
    rounds (SMG), one tall pointed round (sniper), a finned rocket (rockets)."""
    shapes, cuts = [], []
    for what, args in ROUNDS[kind]:
        if what == 'rocket':
            outline, lines = rocket_body(*args)
            shapes += [outline] + rocket_fins(args[0], args[1], args[2])
        else:
            outline, lines = cartridge(*args) if what == 'cartridge' else shell(*args)
            shapes.append(outline)
        cuts += lines
    return Icon('Ammo' + kind, shapes, cuts=cuts, shade=SHADE_LINE[kind], outline=OUTLINE['Ammo'], cut_width=1.1)


def star4(cx, cy, r, waist):
    """A four-point star (a spark): tips r from the centre up, right, down and left, its waist points waist off both axes."""
    return [(cx, cy - r), (cx + waist, cy - waist), (cx + r, cy), (cx + waist, cy + waist), (cx, cy + r),
            (cx - waist, cy + waist), (cx - r, cy), (cx - waist, cy - waist)]


def grave_salt_grenade():
    """The grave-salt grenade (Docs/Polish/BorderlandsComparison.md, item 16; the HUD's counter, UHudGrenadeWidget): a
    squat stoppered tin in the ammo icons' box and ink line, with its shoulder band and neck, the cork, and the waxed
    fuse curling up out of it to a spark. The label band has a grave cross cut on it. Told apart from the ammo by being
    a tin, not a round (never by color)."""
    shapes = [
        [(21, 32), (22, 30), (48, 30), (49, 32), (49, 58.5), (47, 61.5), (23, 61.5), (21, 58.5)],   # the body, its feet rounded
        [(22, 30), (25, 24.5), (45, 24.5), (48, 30)],                                              # the shoulder band
        rect(30, 40, 19.5, 24.5),                                                                  # the neck
        [(29.5, 19.5), (28.5, 13.5), (41.5, 13.5), (40.5, 19.5)],                                  # the cork, wider at its top
        star4(49, 5.8, 4.3, 1.2)]                                                                  # the fuse's spark
    strokes = [[(35, 13.5), (35.6, 10.2), (37.6, 7.6), (40.8, 6), (44.6, 5.8)]]                    # the waxed fuse
    cuts = [
        [(22.5, 30), (47.5, 30)],                                                                  # the lid seam
        [(21, 38.5), (49, 38.5)], [(21, 52.5), (49, 52.5)],                                        # the label band
        [(35, 40.8), (35, 50.2)], [(31.2, 43.8), (38.8, 43.8)],                                    # the grave cross on it
        [(29.4, 17.6), (40.6, 17.6)]]                                                              # the cork's wax seal
    return Icon('GraveSaltGrenade', shapes, strokes=strokes, cuts=cuts, shade=46, outline=OUTLINE['Ammo'], stroke_width=2.2,
                cut_width=1.1)


# --- The ward box (SHIELD, 2026-10-22; UShieldDefinition::IconName), in the ammo icons' box ---

def ward_box():
    """The ward box (the player's shield as loot: Art/Models/Shields/WardBox.py), seen from the front in the ammo icons' box
    and ink line: its iron case with the lid's band over it, the damper's wheel on the lid, and the coal window, its frame
    and its four upright bars (a stove grate) cut in ink. Told apart from the ammo and the grenade by being a box with a
    barred window (never by colour)."""
    shapes = [
        rrect(32, 41.5, 36, 31, 2.5),                                                              # the case
        rect(12.5, 51.5, 20, 26),                                                                  # the lid, a little wider
        rect(30.2, 33.8, 14.5, 20),                                                                # the damper's stem
        circle(32, 9.6, 6.2, 24)]                                                                  # its wheel
    holes = [circle(32, 9.6, 2.7, 16)]
    cuts = [
        closed(rect(20.5, 43.5, 31, 51.5)),                                                        # the window's frame
        [(25.1, 31), (25.1, 51.5)], [(29.7, 31), (29.7, 51.5)], [(34.3, 31), (34.3, 51.5)],         # the grate
        [(38.9, 31), (38.9, 51.5)],
        [(12.5, 26), (51.5, 26)]]                                                                  # the lid's seam
    return Icon('WardBox', shapes, holes=holes, cuts=cuts, shade=41, outline=OUTLINE['Ammo'], cut_width=1.1)


# --- The ember powers (Embers/EmberDefinition.h IconName; the HUD's socket and the powers page), in the ammo icons' box ---

def offset_band(centre, widths):
    """A tapered band along a polyline: each point pushed out half its width to the left of travel, then back along the
    right, as one simple polygon (a dust streak, a ray). centre and widths run together."""
    left, right = [], []
    for k, (x, y) in enumerate(centre):
        ax, ay = centre[max(k - 1, 0)]
        bx, by = centre[min(k + 1, len(centre) - 1)]
        dx, dy = bx - ax, by - ay
        length = math.hypot(dx, dy) or 1.0
        nx, ny = -dy / length, dx / length
        half = widths[k] / 2
        left.append((x + nx * half, y + ny * half))
        right.append((x - nx * half, y - ny * half))
    return left + list(reversed(right))


def dust_devil():
    """Dust Devil (Ellis's dash, nature Wind): a tight whirl of three dust streaks round a calm eye. Each streak spirals
    out from near the eye, thickest a third of the way along and thinning to a wisp at its tail, a third of a turn after
    the one before, so the three nest like a whirlwind seen from above."""
    cx, cy = 32.0, 32.0
    shapes = []
    for k in range(3):
        start = math.radians(-90 + 120 * k)
        sweep = math.radians(195)
        centre, widths = [], []
        n = 26
        for i in range(n + 1):
            t = i / n
            angle = start + sweep * t
            radius = 11.0 + 16.5 * t
            centre.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle)))
            # A wisp where it leaves the eye, thickest a third of the way out, a wisp again at its tail.
            u = min(1.0, 0.04 + t * 0.96)
            widths.append(max(0.9, 6.8 * math.sin(math.pi * u) ** 0.8))
        shapes.append(offset_band(centre, widths))
    shapes.append(circle(cx, cy, 5.4, 20))                                                     # the eye
    holes = [circle(cx, cy, 2.0, 12)]                                                          # its calm middle
    return Icon('DustDevil', shapes, holes, shade=36, outline=OUTLINE['Ammo'], cut_width=1.1)


def sundown():
    """Sundown (the world slowed, nature Dusk): a half sun sitting on a horizon line with four short rays fanned over it,
    tapering to points, and two bands cut across the sun's lower half."""
    cx, horizon = 32.0, 45.0
    r = 16.5
    sun = [(cx + r * math.cos(math.radians(a)), horizon - r * math.sin(math.radians(a))) for a in range(0, 181, 9)]
    shapes = [sun, rect(4.0, 60.0, horizon, horizon + 4.2)]                                    # the sun, the horizon
    for degrees in (24.0, 67.0, 113.0, 156.0):                                                 # the rays
        a = math.radians(degrees)
        ux, uy = math.cos(a), -math.sin(a)
        centre = [(cx + ux * d, horizon + uy * d) for d in (22.5, 26.0, 29.5)]
        shapes.append(offset_band(centre, [4.4, 3.0, 1.2]))
    cuts = [[(cx - 15.4, horizon - 5.5), (cx + 15.4, horizon - 5.5)],
            [(cx - 13.0, horizon - 10.0), (cx + 13.0, horizon - 10.0)]]                       # the bands on the sun
    return Icon('Sundown', shapes, cuts=cuts, shade=37, outline=OUTLINE['Ammo'], cut_width=1.1)


def borrowed_time():
    """Borrowed Time (the debt, nature Debt): an hourglass between its two end plates, the sand run down into the lower
    bulb (shaded, with its heap and the stream falling on it), with a ledger's ruled line struck through it from corner
    to corner (its edges cut where it crosses the glass, so it reads across it). No posts: at HUD size they only thickened
    the ink."""
    cx = 32.0
    glass_right = quad((42.0, 14.0), (42.5, 26.0), (cx + 1.6, 32.0)) + quad((cx + 1.6, 32.0), (42.5, 38.0), (42.0, 50.0))[1:]
    glass = glass_right + [(2 * cx - x, y) for x, y in reversed(glass_right)]
    shapes = [rect(16.0, 48.0, 8.0, 14.0), rect(16.0, 48.0, 50.0, 56.0),                     # the end plates
              glass]
    # The ledger line: a slanted rule from low left to high right through the waist.
    a, b = (6.0, 47.0), (58.0, 17.0)
    dx, dy = b[0] - a[0], b[1] - a[1]
    length = math.hypot(dx, dy)
    nx, ny = -dy / length * 1.9, dx / length * 1.9
    shapes.append([(a[0] + nx, a[1] + ny), (b[0] + nx, b[1] + ny), (b[0] - nx, b[1] - ny), (a[0] - nx, a[1] - ny)])
    cuts = [[(a[0] + nx, a[1] + ny), (b[0] + nx, b[1] + ny)], [(a[0] - nx, a[1] - ny), (b[0] - nx, b[1] - ny)],
            quad((23.5, 48.0), (cx, 40.0), (40.5, 48.0)),                                      # the sand's heap
            [(cx, 33.5), (cx, 44.0)]]                                                          # its stream
    return Icon('BorrowedTime', shapes, cuts=cuts, shade=32, outline=OUTLINE['Ammo'], cut_width=1.1)


# --- The four new heroes' twelve (2026-10-20; the powers in Embers/Powers), in the same box and ink as Ellis's ---

def rot(points, cx, cy, degrees):
    """Points turned round (cx, cy) by degrees (clockwise on screen: y is down)."""
    a = math.radians(degrees)
    c, s = math.cos(a), math.sin(a)
    return [(cx + (x - cx) * c - (y - cy) * s, cy + (x - cx) * s + (y - cy) * c) for x, y in points]


def rrect(cx, cy, w, h, r, n=4):
    """A rectangle centred on (cx, cy) with its corners rounded to radius r (n + 1 points each), clockwise on screen."""
    hw, hh = w / 2, h / 2
    r = min(r, hw, hh)
    pts = []
    for qx, qy, a0 in ((cx + hw - r, cy - hh + r, -90), (cx + hw - r, cy + hh - r, 0), (cx - hw + r, cy + hh - r, 90),
                       (cx - hw + r, cy - hh + r, 180)):
        for k in range(n + 1):
            a = math.radians(a0 + 90 * k / n)
            pts.append((qx + r * math.cos(a), qy + r * math.sin(a)))
    return pts


def closed(poly):
    """A polygon as a closed cut line (its first point again at its end)."""
    return list(poly) + [poly[0]]


def card(cx, cy, degrees, w=21.0, h=30.0):
    """A playing card centred on (cx, cy) turned by degrees: its outline, and its straight left edge (the edge that lies
    over the card beside it in a fan) as a cut."""
    outline = rot(rrect(cx, cy, w, h, 3.0), cx, cy, degrees)
    edge = rot([(cx - w / 2, cy - h / 2 + 3.0), (cx - w / 2, cy + h / 2 - 3.0)], cx, cy, degrees)
    return outline, edge


def lucky_streak():
    """Lucky Streak (Odessa's Fortune): two playing cards fanned, the front one's diamond pip cut through it, and a spark
    of luck off their top corner (the ricochet's gold, the lucky round)."""
    back, _ = card(25.0, 35.0, -15.0)
    front, front_edge = card(38.0, 37.0, 11.0)
    pip = rot([(38.0, 30.0), (43.0, 37.0), (38.0, 44.0), (33.0, 37.0)], 38.0, 37.0, 11.0)
    # The front card's whole outline cut, so it reads over the back one.
    cuts = [closed(front)]
    shapes = [back, front, star4(51.5, 11.0, 7.5, 2.0)]
    return Icon('LuckyStreak', shapes, holes=[pip], cuts=cuts, shade=36, outline=OUTLINE['Ammo'], cut_width=1.1)


def fifty_two_pickup():
    """Fifty-Two Pickup (Odessa's thrown deck): five cards fanned wide from one corner held low, each lying over the one to
    its left (its left edge cut), the front one's diamond pip cut through it."""
    pivot = (32.0, 55.0)
    shapes, cuts = [], []
    for k, degrees in enumerate((-46.0, -23.0, 0.0, 23.0, 46.0)):
        outline = rot(rrect(32.0, 32.0, 17.0, 27.0, 2.6), pivot[0], pivot[1], degrees)
        edge = rot([(23.5, 21.6), (23.5, 42.4)], pivot[0], pivot[1], degrees)
        shapes.append(outline)
        if k:
            cuts.append(edge)
    # The front card is the right-most: its whole outline cut, and its pip.
    cuts.append(closed(shapes[-1]))
    pip = rot([(32.0, 26.5), (35.6, 32.0), (32.0, 37.5), (28.4, 32.0)], pivot[0], pivot[1], 46.0)
    return Icon('FiftyTwoPickup', shapes, holes=[pip], cuts=cuts, shade=34, outline=OUTLINE['Ammo'], cut_width=1.1)


def double_or_nothing():
    """Double or Nothing (Odessa's Risk): a struck coin in front, its rim ringed and a star pip cut through it, and behind
    it an empty ring where the other coin was: something, or nothing."""
    front = circle(39.0, 37.0, 15.5, 28)
    ring = circle(19.0, 21.5, 12.0, 24)
    ring_hole = circle(19.0, 21.5, 7.4, 20)
    pip = star4(39.0, 37.0, 6.5, 1.9)
    cuts = [closed(front), closed(circle(39.0, 37.0, 11.6, 28))]
    return Icon('DoubleOrNothing', [ring, front], holes=[ring_hole, pip], cuts=cuts, shade=34, outline=OUTLINE['Ammo'], cut_width=1.1)


def toll_chain():
    """Toll Chain (Crane's Debt): three iron links running down from the top left, face, edge, face, and the hook hanging
    off the last, its barb turned up."""
    shapes, holes, cuts = [], [], []
    for k, (cx, cy) in enumerate(((13.0, 11.0), (21.5, 19.5), (30.0, 28.0))):
        if k % 2 == 0:
            # A link face on: a stadium with its slot through it.
            outline = rot(rrect(cx, cy, 15.0, 8.6, 4.3, 5), cx, cy, 45.0)
            shapes.append(outline)
            holes.append(rot(rrect(cx, cy, 8.0, 3.0, 1.5, 4), cx, cy, 45.0))
            cuts.append(closed(outline))
        else:
            # A link edge on: a short thick bar.
            shapes.append(rot(rrect(cx, cy, 15.0, 4.2, 2.1, 4), cx, cy, 45.0))
    # The hook: a thick J from under the last link, round to its barb.
    hook = [(36.0, 34.0), (39.5, 40.0), (41.5, 47.5), (44.5, 54.0), (49.5, 57.0), (54.5, 55.5), (57.0, 50.5), (56.5, 45.5)]
    shapes.append([(53.0, 46.5), (56.8, 39.5), (60.0, 46.6)])                                  # the barb
    return Icon('TollChain', shapes, holes=holes, strokes=[hook], cuts=cuts, shade=33, outline=OUTLINE['Ammo'], stroke_width=5.2,
                cut_width=1.1)


def grave_ward():
    """Grave Ward (Crane's Mercy): the lantern he plants, its bail, cap, glass in two panes with the flame in it and its
    foot, and four short rays of its light either side."""
    cx = 32.0
    shapes = [
        [(23.5, 20.0), (27.5, 14.0), (36.5, 14.0), (40.5, 20.0)],                                # the cap
        rect(21.5, 42.5, 20.0, 23.0),                                                             # its band
        rect(23.5, 40.5, 23.0, 45.5),                                                             # the glass
        rect(20.5, 43.5, 45.5, 50.5)]                                                             # the foot
    strokes = [[(25.5, 15.5), (26.5, 9.0), (cx, 6.0), (37.5, 9.0), (38.5, 15.5)]]               # the bail
    for degrees in (8.0, 38.0, 142.0, 172.0):
        a = math.radians(degrees)
        ux, uy = math.cos(a), -math.sin(a)
        centre = [(cx + ux * d, 33.0 + uy * d) for d in (17.5, 21.5, 25.5)]
        shapes.append(offset_band(centre, [4.2, 2.8, 1.1]))
    flame = [(cx, 26.5), (34.6, 32.5), (34.0, 37.5), (cx, 39.5), (30.0, 37.5), (29.4, 32.5), (cx, 26.5)]
    cuts = [flame, [(23.5, 23.0), (40.5, 23.0)], [(21.5, 45.5), (42.5, 45.5)]]
    return Icon('GraveWard', shapes, strokes=strokes, cuts=cuts, shade=36, outline=OUTLINE['Ammo'], stroke_width=2.4, cut_width=1.1)


def last_rites():
    """Last Rites (Crane's Judgment): a ringed grave cross on its plinth, the ring cut where it crosses the arms so both
    read."""
    cx, cy = 32.0, 25.0
    shapes = [rect(28.0, 36.0, 4.0, 52.0), rect(16.0, 48.0, 21.0, 29.0),                    # the cross
              rect(22.0, 42.0, 52.0, 59.0)]                                                     # its plinth
    ring = [p for p in circle(cx, cy, 13.0, 32)]
    cuts = [[(28.0, 52.0), (36.0, 52.0)]]                                                       # the cross on its plinth
    return Icon('LastRites', shapes, strokes=[ring + ring[:1]], cuts=cuts, shade=30, outline=OUTLINE['Ammo'], stroke_width=4.4,
                cut_width=1.1)


def slag_bomb():
    """Slag Bomb (Gauge's Forge): a ball of molten slag lobbed down to the right, its crust cracked across it, three
    tapered streaks of fire trailing back up the way it came, and a drop flung off ahead of it."""
    cx, cy, r = 38.0, 39.0, 15.0
    shapes = [circle(cx, cy, r, 30)]
    # The trail: three streaks leaving the ball's upper left, thick at the ball, thinning to points up the arc.
    for side, length, w in ((-7.5, 27.0, 7.0), (0.0, 33.0, 8.4), (7.5, 24.0, 6.2)):
        ux, uy = -0.70, -0.71                                                                  # back up the arc
        nx, ny = -uy, ux                                                                       # across it
        start = (cx + ux * (r - 4.0) + nx * side, cy + uy * (r - 4.0) + ny * side)
        centre = [(start[0] + ux * d, start[1] + uy * d) for d in (0.0, length * 0.45, length)]
        shapes.append(offset_band(centre, [w, w * 0.62, 0.9]))
    shapes += [circle(56.5, 54.5, 2.8, 12), circle(58.5, 45.0, 2.0, 10)]                       # drops flung ahead
    # The crust cracked open: a jagged crack across the ball and a branch off it.
    cuts = [[(26.0, 36.0), (31.5, 39.5), (35.0, 36.5), (40.5, 41.5), (45.0, 39.0), (51.5, 43.5)],
            [(40.5, 41.5), (39.0, 47.5), (42.5, 51.5)]]
    return Icon('SlagBomb', shapes, cuts=cuts, shade=38, outline=OUTLINE['Ammo'], cut_width=1.3)


def iron_hide():
    """Iron Hide (Gauge's plates locked): a heater shield of riveted iron plate, its centre seam and a band across it,
    rivets along them."""
    right = [(32.0, 7.0), (52.0, 7.0), (52.0, 30.0)] + quad((52.0, 30.0), (51.0, 47.0), (32.0, 59.0))[1:]
    shield = right + [(2 * 32.0 - x, y) for x, y in reversed(right[1:-1])]
    cuts = [[(32.0, 7.0), (32.0, 57.0)], [(12.0, 22.0), (52.0, 22.0)]]
    for x, y in ((17.0, 12.0), (47.0, 12.0), (17.0, 27.0), (47.0, 27.0), (27.0, 37.0), (37.0, 37.0), (32.0, 48.0)):
        cuts.append(closed(circle(x, y, 1.0, 8)))
    return Icon('IronHide', [shield], cuts=cuts, shade=30, outline=OUTLINE['Ammo'], cut_width=1.3)


def rail_spike():
    """Rail Spike (Gauge's Steam): a railway spike driven up to the right, its chisel point leading and its hooked head
    behind, two speed streaks beside it and a cloud of steam where it left."""
    # The spike along +x in its own frame, then turned to rise to the right.
    spike = [(-3.0, -4.5), (2.0, -4.5), (2.0, -3.0), (34.0, -3.0), (42.0, 0.0), (34.0, 3.0), (2.0, 3.0), (2.0, 9.5),
             (-3.0, 9.5)]
    cx, cy, degrees = 30.0, 32.0, -42.0
    place = lambda pts: rot([(cx - 17.0 + x, cy + y) for x, y in pts], cx, cy, degrees)
    shapes = [place(spike)]
    for offset in (-8.5, 8.5):
        centre = place([(8.0, offset), (18.0, offset), (28.0, offset)])
        shapes.append(offset_band(centre, [0.8, 2.6, 0.8]))
    shapes += [circle(12.5, 52.5, 6.0, 16), circle(7.5, 45.5, 4.4, 14), circle(19.5, 58.0, 3.6, 12)]   # the steam
    cuts = [place([(2.0, -3.0), (2.0, 3.0)])]
    return Icon('RailSpike', shapes, cuts=cuts, shade=34, outline=OUTLINE['Ammo'], cut_width=1.1)


def spyglass():
    """Spyglass (Pike's Sight): a brass spyglass drawn out, three sections rising to the right, their joints banded, and a
    glint off its lens."""
    sections = [rect(0.0, 12.5, -3.4, 3.4), rect(11.5, 25.0, -4.7, 4.7), rect(24.0, 38.0, -6.0, 6.0), rect(37.0, 41.5, -7.0, 7.0)]
    cx, cy, degrees = 30.0, 35.0, -36.0
    place = lambda pts: rot([(cx - 21.0 + x, cy + y) for x, y in pts], cx, cy, degrees)
    shapes = [place(s) for s in sections] + [star4(53.0, 11.0, 7.0, 1.9)]
    cuts = [place([(x, -h), (x, h)]) for x, h in ((12.0, 3.4), (24.5, 4.7), (31.0, 6.0), (37.5, 6.0))]
    return Icon('Spyglass', shapes, cuts=cuts, shade=33, outline=OUTLINE['Ammo'], cut_width=1.1)


def tripwire():
    """Tripwire (Pike's Craft): two stakes driven into the ground line, the wire strung between them plucked in the middle
    where something has caught it."""
    stake = [(-3.2, 20.0), (0.0, 16.5), (3.2, 20.0), (3.2, 46.0), (0.0, 51.5), (-3.2, 46.0)]
    shapes = [[(10.5 + x, y) for x, y in stake], [(53.5 + x, y) for x, y in stake],
              rect(4.0, 60.0, 48.0, 52.0)]                                                           # the ground
    wire = [(13.0, 28.0), (25.5, 28.5), (29.0, 24.5), (32.0, 33.5), (35.0, 24.5), (38.5, 28.5), (51.0, 28.0)]
    cuts = [[(7.3, 48.0), (13.7, 48.0)], [(50.3, 48.0), (56.7, 48.0)]]                                # where the stakes go in
    return Icon('Tripwire', shapes, strokes=[wire], cuts=cuts, shade=34, outline=OUTLINE['Ammo'], stroke_width=2.2, cut_width=1.1)


def long_shot():
    """Long Shot (Pike's aimed shot): a scope's reticle, its ring with four ticks and a dot, and a bullet's long streak
    coming in from the bottom left to its heart."""
    cx, cy = 37.0, 27.0
    ring = circle(cx, cy, 14.5, 32)
    shapes = [rect(cx - 1.6, cx + 1.6, cy - 21.5, cy - 9.0), rect(cx - 1.6, cx + 1.6, cy + 9.0, cy + 21.5),
              rect(cx - 21.5, cx - 9.0, cy - 1.6, cy + 1.6), rect(cx + 9.0, cx + 21.5, cy - 1.6, cy + 1.6),
              circle(cx, cy, 2.8, 12)]
    centre = [(4.0, 57.0), (12.0, 49.5), (20.0, 42.0), (26.5, 36.0)]
    shapes.append(offset_band(centre, [0.8, 2.4, 4.2, 5.2]))
    return Icon('LongShot', shapes, strokes=[ring + ring[:1]], shade=30, outline=OUTLINE['Ammo'], stroke_width=3.4, cut_width=1.1)


# --- The soul flask (2026-10-30, Docs/Work/2026-10-30-SoulFlask; the HUD's flask row, UHudFlaskWidget), in the ammo
# icons' box ---

def soul_flask():
    """The soul flask (Art/Models/Items/SoulFlask.py: a worn glass flask in a leather and tin harness, with a cork): a
    round-bellied flask on a flat foot, its short neck a little crooked (tipped 5 degrees, as a hand-blown one sits), the
    tin collar at its base, the lip, and the cork wider at its top with its wax ring. The harness is a leather band round
    the belly's widest, wider than the glass so it reads as wrapped on. The soul's level is a cut across the belly with
    the shade under it, so the lower glass reads as filled. No straps up the glass: at HUD size they crossed the level
    and read as cracks. Told apart from the grenade (a squat tin with a fuse) by its round belly and its band (never by
    colour)."""
    cx, cy, r = 32.0, 43.5, 18.0
    base = cy + r * 0.86                                                                       # the flat foot
    belly = [(x, min(y, base)) for x, y in circle(cx, cy, r, 40)]
    # The neck and what sits on it, tipped round the collar's middle: hand-blown glass, not a chemist's.
    tip = lambda poly: rot(poly, cx, 25.5, 5.0)
    neck = tip(rect(28.6, 35.4, 17.0, 28.0))
    collar = tip(rect(25.4, 38.6, 23.2, 27.4))                                                 # the tin collar
    lip = tip(rect(27.0, 37.0, 14.6, 17.8))
    cork = tip([(28.4, 14.6), (27.5, 7.4), (36.5, 7.4), (35.6, 14.6)])                        # wider at its top
    band = rect(cx - r - 1.6, cx + r + 1.6, cy - 3.0, cy + 3.0)                                # the leather band
    level = cy - 9.0
    half = math.sqrt(r * r - 9.0 * 9.0) - 0.9
    cuts = [
        [(cx - r - 1.6, cy - 3.0), (cx + r + 1.6, cy - 3.0)],                                  # the band's edges
        [(cx - r - 1.6, cy + 3.0), (cx + r + 1.6, cy + 3.0)],
        quad((cx - half, level), (cx, level + 1.8), (cx + half, level)),                       # the soul's level
        tip([(28.2, 11.6), (35.8, 11.6)])]                                                     # the cork's wax ring
    return Icon('SoulFlask', [belly, neck, collar, lip, cork, band], cuts=cuts, shade=level, outline=OUTLINE['Ammo'],
                cut_width=1.1)


# The ink line's width in each icon's units: guns scale it with their length so every gun's line looks the same
# when they're drawn the same width; the ammo icons, the grenade and the ember powers share one box and one width.
OUTLINE = {'Rifle': 1.72, 'Shotgun': 2.56, 'Revolver': 0.72, 'SMG': 1.51, 'Sniper': 3.08, 'LMG': 2.42, 'Launcher': 2.49,
           'Ammo': 3.2}
AMMO_TYPES = ('AssaultRifle', 'Shotgun', 'Pistol', 'SMG', 'Sniper', 'Rocket')   # EAmmoType order
# The ember powers' icons by UEmberDefinition::IconName (the contract's names: the generated functions are named so).
EMBER_ICONS = ('DustDevil', 'Sundown', 'BorrowedTime', 'LuckyStreak', 'FiftyTwoPickup', 'DoubleOrNothing', 'TollChain',
               'GraveWard', 'LastRites', 'SlagBomb', 'IronHide', 'RailSpike', 'Spyglass', 'Tripwire', 'LongShot')


def is_square(icon):
    """Drawn in the ammo icons' square box (the ammo, the grenade, the ember powers, the flask), as against a gun's own
    wide one."""
    return (icon.name.startswith('Ammo') or icon.name in ('GraveSaltGrenade', 'WardBox', 'SoulFlask') or icon.name in EMBER_ICONS
            or icon.name.startswith('Maker'))


def icons():
    """Every icon, placed in its view box: each gun in its own, the ammo types, the grenade and the ember powers all in
    one shared box (fitted to the ammo, so the others can't move the ammo icons)."""
    guns = [bullpup().place(), ranchhand().place(), drover().place(), rustler().place(), longhorn().place(), thresher().place(),
            powderkeg().place()]
    rounds = [ammo(kind) for kind in AMMO_TYPES]
    boxes = [r.bounds() for r in rounds]
    shared = (min(b[0] for b in boxes), min(b[1] for b in boxes), max(b[2] for b in boxes), max(b[3] for b in boxes))
    side = max(shared[2] - shared[0], shared[3] - shared[1])
    cx, cy = (shared[0] + shared[2]) / 2, (shared[1] + shared[3]) / 2
    square = (cx - side / 2, cy - side / 2, cx + side / 2, cy + side / 2)
    embers = [dust_devil(), sundown(), borrowed_time(), lucky_streak(), fifty_two_pickup(), double_or_nothing(), toll_chain(),
              grave_ward(), last_rites(), slag_bomb(), iron_hide(), rail_spike(), spyglass(), tripwire(), long_shot()]
    # The gun makers' marks (Art/Icons/MakerMarks.py: the same outlines make the guns' mark atlas), in the shared box.
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))   # Blender's python doesn't add the script's folder
    import MakerMarks
    return (guns + [r.place(square) for r in rounds] + [grave_salt_grenade().place(square)] + [e.place(square) for e in embers]
            + [ward_box().place(square)] + MakerMarks.maker_marks(square) + [soul_flask().place(square)])


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


def cpp_find(all_icons):
    """Every icon by its name, for data that names its icon (UEmberDefinition::IconName; LooterUI::FindInkedIcon)."""
    rows = ''.join(f'\t\t\tAll.Add(FName(TEXT("{i.name}")), {i.name}());\n' for i in all_icons)
    return ('\t/** Every icon above by its function\'s name ("DustDevil"), or null: made once, kept for the session. */\n'
            '\tinline const LooterUI::FInkedIcon* Find(FName Name)\n\t{\n'
            '\t\tstatic const TMap<FName, LooterUI::FInkedIcon> Icons = []()\n\t\t{\n'
            '\t\t\tTMap<FName, LooterUI::FInkedIcon> All;\n' + rows +
            '\t\t\treturn All;\n\t\t}();\n'
            '\t\treturn Name.IsNone() ? nullptr : Icons.Find(Name);\n\t}\n')


def write_cpp(all_icons):
    body = '\n'.join(cpp_icon(i) for i in all_icons) + '\n' + cpp_find(all_icons)
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
        sizes = AMMO_SIZES if is_square(icon) else GUN_SIZES
        for px in sizes:
            image = picture(icon, px)
            name = f'Inked_{icon.name}_{px[0]}.png'
            write_png(os.path.join(PREVIEW_OUT, name), image)
            files[(icon.name, px)] = (name, image.shape[1] // 2, image.shape[0] // 2)
        big = picture(icon, (150, 150) if is_square(icon) else (440, 200))
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
    guns = [i for i in all_icons if not is_square(i)]
    rounds = [i for i in all_icons if is_square(i)]
    big = ''.join(f'<div class="cell">{img((i.name, "big"))}<span>{i.name}</span></div>' for i in guns)
    big_ammo = ''.join(f'<div class="cell">{img((i.name, "big"))}<span>{i.name.removeprefix("Ammo")}</span></div>' for i in rounds)
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
