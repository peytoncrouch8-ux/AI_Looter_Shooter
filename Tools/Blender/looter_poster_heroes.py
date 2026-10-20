"""The wanted posters of the heroes but Ellis (2026-10-20, the hero pick): the words and the woodcut bust that
looter_posters.py (--hero=<Id>) sets on the poster in place of Ellis's name, crimes and bandana'd bust. The paper, the
top, ALREADY, Calder's note, the Seven and the cross stay as they are. The words: Saved/Lines VARIANTS painted.WantedPoster
(Tools/Unreal/hero_lines.json); the town's belief, printed by Ruth's Rangers.

The busts are cut as Ellis's is (looter_posters.middle_woodcut, option B): the same frame, the same level lines behind,
the light from the upper left, solid black where the shadow is and white lines cut into the lit side. Each hero's
tell-tale things follow the user's portrait picks (Art/Portraits/<Hero>.py) and the HUD's inked portraits:
  Odessa  a flat-crowned gambler's hat with a playing card in its band, long dark hair, a high collar and a cravat;
  Crane   a battered slouch hat, long white hair, a drooping mustache, a coin over his left eye, the tin star, the toll
          chain over his shoulder;
  Gauge   a squared iron head with one headlamp, a grille mouth and the nameplate GAUGE, a boiler's shoulders, a stack;
  Pike    a bowler with goggles on its band, round spectacles, white mutton chops, a kerchief at the throat.
Picture units are Ellis's: 92 mm, x right and y up from the middle of the frame's foot; a head's own frame (head units,
the chin about 0.82 and the crown 1.44) is Ellis's head's.
"""
import math

import numpy as np

import looter_posters as P

smooth = P.smooth

# Ellis's head frame inside the picture (looter_posters.middle_woodcut).
HX, HY, HZ = 1.14, 1.12, 0.79
LIGHT = np.array((-0.55, 0.5, 0.67))
LIGHT = LIGHT / np.linalg.norm(LIGHT)
CRIME = 'FOR ROBBERY WITH THE DUNNE GANG'


# --- The cut and its frames ---

def S(*p, closed=False, per=8):
    return P.spline(np.array(p, np.float64), per=per, closed=closed)


def H(*p, closed=False):
    """A spline through points in head units, in picture units."""
    q = S(*p, closed=closed)
    return np.stack([q[:, 0] * HX, HZ + (q[:, 1] - 0.85) * HY], axis=1)


def Hp(points):
    """Points (n, 2) in head units, in picture units (no spline)."""
    q = np.asarray(points, np.float64)
    return np.stack([q[:, 0] * HX, HZ + (q[:, 1] - 0.85) * HY], axis=1)


def He(cx, cy, rx, ry, a0=0.0, a1=360.0, step=10.0):
    """An ellipse (or an arc of one) in head units, in picture units."""
    return Hp(P.arc(cx, cy, rx, ry, a0, a1, step=step))


def new_cut(sheet, rng):
    """The cut over Ellis's portrait frame, made exactly as his is."""
    x0, y0, x1, y1 = P.PORTRAIT
    inner = (x0 + 5.5, y0 + 5.5, x1 - 5.5, y1 - 5.5)
    cut = P.Cut(sheet, inner, ((x0 + x1) * 0.5, inner[3]), 92.0, ss=3, seed=int(rng.integers(1 << 30)))
    cut.Xh, cut.Yh = cut.X / HX, 0.85 + (cut.Y - HZ) / HY
    return cut


def sky(cut):
    """The level lines behind, lighter to the left and up top (Ellis's)."""
    X, Y = cut.X, cut.Y
    tone = 0.04 + 0.2 * smooth(-0.3, 1.0, X) + 0.08 * smooth(0.7, 0.0, Y) + 0.04 * P._noise(cut.shape, 7, 12.0)
    cut.paint(np.ones(cut.shape, np.float32), cut.lines(cut.v / 4.0, tone))


def union(cut, *polys):
    return np.max([cut.mask(p) for p in polys], axis=0)


def black(cut, *polys):
    cut.paint(union(cut, *polys), 1.0)


def white(cut, *polys):
    cut.paint(union(cut, *polys), 0.0)


def lambert(cut, cx, cy, rx, ry, head=True):
    """How lit a rounded form is (0..1): an ellipsoid over the ellipse (cx, cy, rx, ry), in head units (or picture
    units, head=False), lit from the upper left."""
    X, Y = (cut.Xh, cut.Yh) if head else (cut.X, cut.Y)
    nx, ny = (X - cx) / rx, (Y - cy) / ry
    nz = np.sqrt(np.clip(1.0 - nx * nx - ny * ny, 0.0, 1.0))
    return np.clip(nx * LIGHT[0] + ny * LIGHT[1] + nz * LIGHT[2], 0.0, 1.0), nz


def engrave(cut, mask, lit, nz, period=0.032, bow=0.05, dark=0.92, light=0.06, angle=0.0):
    """A rounded form engraved: lines across it bowing with its surface, thick where it's in shade and thin where
    it's lit; mask where it is."""
    a = math.radians(angle)
    along = cut.Yh * math.cos(a) - cut.Xh * math.sin(a)
    u = (along + bow * nz) / period
    # Most of a lit face is bare paper; the lines gather only where it turns from the light.
    tone = dark - (dark - light) * smooth(0.12, 0.62, lit)
    cut.paint(mask, cut.lines(u, np.clip(tone, 0.0, 1.0)))


def coat(cut, w=1.0, lit_tone=0.04, dark_tone=0.55, period=3.6, lapels=True):
    """Ellis's coat: the shoulders, their seams, the lapels folded down to a V (the shirt in it), lit on the left.
    w narrows or widens it. Returns the shoulders' line."""
    pts = [(-1.3, 0.05), (-1.1, 0.4), (-0.88, 0.55), (-0.62, 0.64), (-0.4, 0.71), (-0.22, 0.78), (0.0, 0.8),
           (0.22, 0.78), (0.4, 0.71), (0.62, 0.64), (0.88, 0.55), (1.1, 0.4), (1.3, 0.05)]
    shoulders = S(*[(x * w, y) for x, y in pts])
    mask = cut.mask(np.vstack([shoulders, [(1.3 * w, -0.5), (-1.3 * w, -0.5)]]))
    d = cut.dist(shoulders, 60.0)
    tone = np.where(cut.X < 0.0, lit_tone + 0.026 * d, dark_tone + 0.06 * d)
    cut.paint(mask, cut.lines(d / period + 0.5, np.clip(tone, 0.0, 1.0)))
    for s in (-1.0, 1.0):
        cut.carve(S((s * 0.86 * w, 0.56), (s * 0.8 * w, 0.3), (s * 0.79 * w, 0.0)), 1.0)
    if lapels:
        cut.paint(cut.mask(np.array([(-0.2, 0.86), (0.2, 0.86), (0.0, 0.1)])), cut.lines(cut.u / 6.0, 0.08))
        lapel = S((-0.64, 0.63), (-0.5, 0.72), (-0.36, 0.83), (-0.25, 0.9), (-0.18, 0.84), (-0.13, 0.6), (-0.06, 0.32),
                  (0.0, 0.1), (-0.12, 0.18), (-0.3, 0.4), (-0.48, 0.55), closed=True)
        for poly, base in ((lapel, 0.6), (P._mirror(lapel), 0.8)):
            dd = cut.dist(np.vstack([poly[-30:], poly[:1]]), 50.0)
            cut.paint(cut.mask(poly), cut.lines(dd / 3.0 + 0.5, np.clip(base + 0.05 * dd, 0.0, 1.0)))
            cut.carve(poly, 1.0, closed=True)
    return shoulders


def crown_lines(cut, mask, half, base_tone=0.22, gain=0.7, shine=0.35, top=None):
    """A hat's crown shaded as Ellis's is: vertical lines, lit on the left, a shine down its middle-left."""
    xc = cut.Xh / half
    tone = base_tone + gain * smooth(-0.8, 0.9, xc)
    if top is not None:
        tone = tone + shine * np.exp(-(xc / 0.13) ** 2) * smooth(top - 0.22, top - 0.04, cut.Yh)
    cut.paint(mask, cut.lines(xc * half * HX * cut.unit / 3.5, np.clip(tone, 0.0, 1.0)))


def brim(cut, top, under, base=0.2, gain=0.16):
    """A hat's brim: lines along it, darker toward its underside; its top edge inked, its underside cut."""
    d = cut.dist(top, 30.0)
    cut.paint(cut.mask(np.vstack([top, under])), cut.lines(d / 2.8 + 0.5, np.clip(base + gain * d, 0.0, 1.0)))
    cut.stroke(top, 1.3)
    cut.carve(under, 1.2)


def dots(cut, mask, size=0.074, hole=0.21):
    """The bandana's print: white dots on black in a staggered grid (Ellis's)."""
    q, r = cut.Xh / size, cut.Yh / size
    col = np.round(q)
    rs = r - 0.5 * (col % 2)
    cut.paint(mask, (np.hypot(q - col, rs - np.round(rs)) > hole).astype(np.float32))


def strands(cut, lines, width=0.55, ink=False):
    """Hair: thin lines along each polyline (head units), cut white (or inked black)."""
    for pts in lines:
        poly = H(*pts)
        if ink:
            cut.stroke(poly, width)
        else:
            cut.carve(poly, width)


def finish(cut, sheet, ink, rng):
    cut.print(ink, wood=0.6)
    P.frame(ink, sheet, rng)


# --- Odessa Lark, the Cardsharp ---

def bust_odessa(sheet, ink, rng):
    cut = new_cut(sheet, rng)
    sky(cut)
    # Her hair falls behind her shoulders to the coat's line, darker than the coat.
    hair_back = H((-0.33, 1.42), (-0.4, 1.22), (-0.44, 1.0), (-0.5, 0.78), (-0.6, 0.6), (-0.52, 0.5), (0.0, 0.5),
                  (0.52, 0.5), (0.6, 0.6), (0.5, 0.78), (0.44, 1.0), (0.4, 1.22), (0.33, 1.42), closed=True)
    black(cut, hair_back)
    coat(cut, w=0.92, lit_tone=0.1, dark_tone=0.62)
    # The neck, in the hair's shadow, lit on its left.
    neck = H((-0.12, 0.98), (-0.12, 0.7), (0.12, 0.7), (0.12, 0.98), closed=True)
    lit, nz = lambert(cut, 0.0, 0.84, 0.13, 0.4)
    engrave(cut, cut.mask(neck), lit * 0.7, nz, dark=0.97, light=0.2)
    # The high collar's white wings up under the jaw, and the cravat puffed at the throat with its pin.
    for s in (-1.0, 1.0):
        white(cut, H((s * 0.04, 0.66), (s * 0.15, 0.68), (s * 0.19, 0.86), (s * 0.13, 0.86), closed=True))
        cut.stroke(H((s * 0.04, 0.66), (s * 0.15, 0.68), (s * 0.19, 0.86)), 0.6)
    cravat = H((-0.16, 0.66), (-0.11, 0.73), (0.0, 0.7), (0.11, 0.73), (0.16, 0.66), (0.12, 0.5), (0.06, 0.38),
               (0.0, 0.33), (-0.06, 0.38), (-0.12, 0.5), closed=True)
    black(cut, cravat)
    for fold in (((-0.1, 0.66), (-0.05, 0.52), (-0.02, 0.4)), ((0.08, 0.66), (0.05, 0.5)), ((-0.13, 0.6), (-0.1, 0.48))):
        strands(cut, [fold], 0.6)
    white(cut, He(0.0, 0.6, 0.022, 0.022))
    # Her face: an oval to a pointed chin, engraved, lit on the left.
    face = H((-0.27, 1.02), (-0.28, 1.2), (-0.25, 1.36), (0.0, 1.44), (0.25, 1.36), (0.28, 1.2), (0.27, 1.02),
             (0.16, 0.88), (0.0, 0.82), (-0.16, 0.88), closed=True)
    face_m = cut.mask(face)
    lit, nz = lambert(cut, 0.0, 1.12, 0.28, 0.33)
    engrave(cut, face_m, lit, nz, dark=0.95, light=0.0)
    # The brim's shadow over her brow, its edge falling across just above the eyes.
    shade = H((-0.4, 1.5), (-0.3, 1.27), (0.0, 1.25), (0.3, 1.28), (0.4, 1.5), closed=True)
    cut.paint(face_m * cut.mask(shade), 1.0)
    # The eyes under it: dark lids, a glint in each; the brows lost in the shade.
    for s in (-1.0, 1.0):
        eye = H((s * 0.04, 1.2), (s * 0.1, 1.225), (s * 0.17, 1.205), (s * 0.1, 1.185), closed=True)
        black(cut, eye)
        white(cut, He(s * 0.1 - 0.012, 1.207, 0.012, 0.012))
    # The nose's shadow on its right, the nostrils, the lips (the lower one lit), the jaw's line on the dark side.
    black(cut, H((0.02, 1.2), (0.045, 1.1), (0.05, 1.05), (0.02, 1.04), (0.035, 1.1), closed=True))
    black(cut, He(-0.025, 1.045, 0.016, 0.01), He(0.035, 1.045, 0.016, 0.01))
    black(cut, H((-0.085, 0.965), (-0.04, 0.985), (0.0, 0.977), (0.04, 0.985), (0.085, 0.965), (0.0, 0.952), closed=True))
    cut.stroke(H((-0.06, 0.935), (0.0, 0.925), (0.06, 0.935)), 0.7)
    cut.stroke(H((0.27, 1.05), (0.2, 0.92), (0.1, 0.85), (0.0, 0.82)), 0.8)
    # Her hair over her brow and down either side of her face, strands cut into it on the lit side.
    for s in (-1.0, 1.0):
        lock = H((s * 0.02, 1.43), (s * 0.18, 1.4), (s * 0.29, 1.3), (s * 0.31, 1.12), (s * 0.34, 0.92),
                 (s * 0.42, 0.72), (s * 0.5, 0.56), (s * 0.4, 0.62), (s * 0.3, 0.8), (s * 0.26, 0.98),
                 (s * 0.25, 1.18), (s * 0.2, 1.32), (s * 0.1, 1.38), closed=True)
        black(cut, lock)
    strands(cut, [((-0.25, 1.36), (-0.3, 1.15), (-0.33, 0.95), (-0.42, 0.72)),
                  ((-0.29, 1.3), (-0.34, 1.1), (-0.39, 0.9), (-0.47, 0.66)),
                  ((-0.36, 1.2), (-0.41, 1.0), (-0.5, 0.78), (-0.56, 0.62)),
                  ((0.31, 1.25), (0.36, 1.05), (0.42, 0.85))], 0.5)
    # The hat: a flat crown, a little narrower at its top, a band with a playing card tucked in it, a wide flat brim.
    crown = H((-0.34, 1.42), (-0.335, 1.56), (-0.32, 1.71), (-0.16, 1.735), (0.0, 1.74), (0.16, 1.735), (0.32, 1.71),
              (0.335, 1.56), (0.34, 1.42))
    hat = cut.mask(crown)
    crown_lines(cut, hat, 0.34, base_tone=0.3, top=1.74)
    cut.stroke(crown, 1.3)
    cut.carve(H((-0.31, 1.705), (0.0, 1.725), (0.31, 1.705)), 0.7)               # the crown's flat top catching light
    band = H((-0.345, 1.43), (0.345, 1.43), (0.34, 1.53), (-0.34, 1.53), closed=True)
    black(cut, band)
    cut.carve(H((-0.34, 1.535), (0.0, 1.535), (0.34, 1.535)), 0.6)
    a = math.radians(-14.0)
    cx, cy, hw, hh = 0.2, 1.55, 0.055, 0.085
    corners = [(cx + x * math.cos(a) - y * math.sin(a), cy + x * math.sin(a) + y * math.cos(a))
               for x, y in ((-hw, -hh), (hw, -hh), (hw, hh), (-hw, hh))]
    card = Hp(corners)
    white(cut, card)
    cut.stroke(np.vstack([card, card[:1]]), 0.7)
    pip = Hp([(cx + x * math.cos(a) - y * math.sin(a), cy + 0.01 + x * math.sin(a) + y * math.cos(a))
              for x, y in ((0.0, 0.035), (0.026, 0.0), (0.0, -0.03), (-0.026, 0.0))])
    black(cut, pip)                                                               # its pip
    top = H((-0.86, 1.47), (-0.76, 1.44), (-0.56, 1.43), (-0.3, 1.425), (0.0, 1.42), (0.3, 1.425), (0.56, 1.43),
            (0.76, 1.44), (0.86, 1.47))
    under = H((0.86, 1.47), (0.8, 1.42), (0.62, 1.385), (0.36, 1.36), (0.0, 1.35), (-0.36, 1.36), (-0.62, 1.385),
              (-0.8, 1.42), (-0.86, 1.47))
    brim(cut, top, under)
    finish(cut, sheet, ink, rng)


# --- Hollis Crane, the Unpaid ---

def bust_crane(sheet, ink, rng):
    cut = new_cut(sheet, rng)
    sky(cut)
    # His long white hair falls behind him to the shoulders: white, outlined, a few strands inked on the shadow side.
    hair = H((-0.32, 1.4), (-0.4, 1.2), (-0.45, 0.98), (-0.52, 0.76), (-0.62, 0.58), (-0.4, 0.55), (0.0, 0.6),
             (0.4, 0.55), (0.62, 0.58), (0.52, 0.76), (0.45, 0.98), (0.4, 1.2), (0.32, 1.4), closed=True)
    white(cut, hair)
    cut.stroke(np.vstack([hair, hair[:1]]), 0.9)
    # A marshal's long coat, dark; the toll chain over his right shoulder; the tin star on his left breast.
    coat(cut, w=1.02, lit_tone=0.18, dark_tone=0.68)
    links = []
    p0, p1 = np.array((-0.62, 0.66)), np.array((0.12, -0.05))
    n = 9
    for k in range(n):
        c = p0 + (p1 - p0) * (k + 0.5) / n + np.array((0.0, -0.05 * math.sin(math.pi * (k + 0.5) / n)))
        ang = math.degrees(math.atan2(*(p1 - p0)[::-1]))
        if k % 2 == 0:
            loop = P.arc(0.0, 0.0, 0.07, 0.035, 0.0, 360.0, step=15.0)
        else:
            loop = P.arc(0.0, 0.0, 0.07, 0.012, 0.0, 360.0, step=15.0)
        r = math.radians(ang)
        rot = np.array([[math.cos(r), -math.sin(r)], [math.sin(r), math.cos(r)]])
        links.append(loop @ rot.T + c)
    for k, loop in enumerate(links):
        black(cut, loop)
        if k % 2 == 0:
            cut.carve(np.vstack([loop, loop[:1]]), 0.8)
        else:
            cut.carve(np.vstack([loop, loop[:1]]), 0.6)
    star = P.star(0.44, 0.38, 0.085, 5, 0.45)
    star[:, 1] = 2 * 0.38 - star[:, 1]              # point up (P.star is drawn y down)
    white(cut, star)
    cut.stroke(np.vstack([star, star[:1]]), 0.8)
    black(cut, P.arc(0.44, 0.38, 0.014, 0.014, 0.0, 360.0, step=30.0))
    # His face: long and gaunt, the hollows of the cheeks dark, under the brim's shadow to the cheekbones.
    face = H((-0.26, 1.0), (-0.275, 1.2), (-0.25, 1.38), (0.0, 1.44), (0.25, 1.38), (0.275, 1.2), (0.26, 1.0),
             (0.17, 0.85), (0.0, 0.79), (-0.17, 0.85), closed=True)
    face_m = cut.mask(face)
    lit, nz = lambert(cut, 0.0, 1.1, 0.27, 0.35)
    hollow = np.exp(-(((np.abs(cut.Xh) - 0.17) / 0.06) ** 2 + ((cut.Yh - 1.04) / 0.07) ** 2))
    engrave(cut, face_m, np.clip(lit - 0.45 * hollow, 0.0, 1.0), nz, dark=0.97, light=0.02)
    shade = H((-0.4, 1.5), (-0.3, 1.18), (0.0, 1.15), (0.3, 1.17), (0.4, 1.5), closed=True)
    cut.paint(face_m * cut.mask(shade), 1.0)
    # The ferryman's coin over his left eye, white in the shade with its square hole; a glint in the other.
    white(cut, He(0.115, 1.24, 0.075, 0.07))
    cut.stroke(np.vstack([He(0.115, 1.24, 0.075, 0.07)]), 0.7)
    cut.stroke(np.vstack([He(0.115, 1.24, 0.055, 0.051)]), 0.45)
    black(cut, Hp([(0.098, 1.223), (0.132, 1.223), (0.132, 1.257), (0.098, 1.257)]))
    white(cut, He(-0.11, 1.235, 0.017, 0.012))
    # The nose's ridge into the light; a drooping white mustache with its hairs inked; the chin under it.
    cut.carve(H((-0.01, 1.17), (-0.02, 1.08)), 0.6)
    must = H((0.0, 1.06), (0.09, 1.07), (0.17, 1.03), (0.2, 0.95), (0.19, 0.84), (0.15, 0.86), (0.12, 0.95),
             (0.06, 0.985), (0.0, 0.98), (-0.06, 0.985), (-0.12, 0.95), (-0.15, 0.86), (-0.19, 0.84), (-0.2, 0.95),
             (-0.17, 1.03), (-0.09, 1.07), closed=True)
    white(cut, must)
    cut.stroke(np.vstack([must, must[:1]]), 0.6)
    for s in (-1.0, 1.0):
        strands(cut, [((s * 0.03, 1.04), (s * 0.1, 1.02), (s * 0.16, 0.95), (s * 0.17, 0.87)),
                      ((s * 0.05, 1.01), (s * 0.12, 0.98), (s * 0.15, 0.92))], 0.45, ink=True)
    # Strands down his hair on the shadow side.
    strands(cut, [((0.3, 1.3), (0.37, 1.08), (0.44, 0.86), (0.54, 0.66)), ((0.35, 1.2), (0.42, 1.0), (0.5, 0.8)),
                  ((-0.33, 1.25), (-0.4, 1.0), (-0.48, 0.78))], 0.45, ink=True)
    # A battered slouch hat: the crown dented down its middle and knocked out of round, the brim drooping at both
    # sides, a bite torn out of it on the right.
    crown = H((-0.34, 1.42), (-0.35, 1.6), (-0.29, 1.73), (-0.15, 1.78), (-0.03, 1.71), (0.07, 1.76), (0.22, 1.76),
              (0.32, 1.64), (0.34, 1.42))
    hat = cut.mask(crown)
    crown_lines(cut, hat, 0.35, base_tone=0.26, top=1.76)
    cut.carve(H((-0.03, 1.7), (-0.05, 1.6)), 0.7)                               # the dent
    cut.stroke(crown, 1.3)
    black(cut, H((-0.345, 1.43), (0.345, 1.43), (0.34, 1.5), (-0.34, 1.5), closed=True))
    cut.carve(H((-0.34, 1.505), (0.34, 1.505)), 0.6)
    top = H((-0.94, 1.3), (-0.84, 1.38), (-0.62, 1.44), (-0.3, 1.455), (0.0, 1.45), (0.3, 1.46), (0.6, 1.44),
            (0.82, 1.37), (0.95, 1.28))
    under = H((0.95, 1.28), (0.86, 1.3), (0.7, 1.36), (0.4, 1.385), (0.0, 1.38), (-0.4, 1.385), (-0.68, 1.36),
              (-0.86, 1.31), (-0.94, 1.3))
    brim(cut, top, under, base=0.25)
    white(cut, H((0.6, 1.47), (0.66, 1.4), (0.72, 1.46), closed=True))          # the bite out of the brim
    finish(cut, sheet, ink, rng)


# --- Gauge, the Iron Hand ---

def bust_gauge(sheet, ink, rng):
    cut = new_cut(sheet, rng)
    sky(cut)
    X, Y = cut.X, cut.Y
    # The stack behind his right shoulder: a narrow iron stack with a flared cap, smoke-dark.
    stack = np.array([(-0.8, 0.6), (-0.8, 1.22), (-0.86, 1.3), (-0.86, 1.38), (-0.6, 1.38), (-0.6, 1.3),
                      (-0.66, 1.22), (-0.66, 0.6)])
    lit, nz = lambert(cut, -0.73, 1.0, 0.09, 3.0, head=False)
    cut.paint(cut.mask(stack), cut.lines(X / 0.022, np.clip(0.95 - 0.75 * lit, 0.0, 1.0)))
    cut.stroke(np.vstack([stack, stack[:1]]), 1.1)
    cut.carve(np.array([(-0.84, 1.3), (-0.62, 1.3)]), 0.6)
    black(cut, np.array([(-0.84, 1.36), (-0.62, 1.36), (-0.62, 1.38), (-0.84, 1.38)]))
    # The boiler shoulders: two iron domes, riveted along their seams, a plate between them over the chest.
    body = np.vstack([[(-1.3, -0.5), (-1.3, 0.3)], S((-1.25, 0.42), (-1.05, 0.68), (-0.75, 0.82), (-0.45, 0.8),
                                                     (-0.28, 0.72), (0.0, 0.7), (0.28, 0.72), (0.45, 0.8),
                                                     (0.75, 0.82), (1.05, 0.68), (1.25, 0.42)), [(1.3, 0.3), (1.3, -0.5)]])
    body_m = cut.mask(body)
    cut.paint(body_m, cut.lines(Y / 0.034, np.where(X < 0.0, 0.45, 0.75)))
    for s in (-1.0, 1.0):
        dome = np.vstack([S((s * 0.3, 0.2), (s * 0.3, 0.66), (s * 0.45, 0.8), (s * 0.75, 0.83), (s * 1.05, 0.69),
                            (s * 1.27, 0.4), (s * 1.3, 0.2))])
        dm = cut.mask(np.vstack([dome, [(s * 1.3, 0.2), (s * 0.3, 0.2)]]))
        # Iron: dark but for a sheen on each dome's upper left, the one on his left darker for being away from the
        # light.
        lit, nz = lambert(cut, s * 0.8, 0.3, 0.55, 0.6, head=False)
        tone = 0.98 - 0.78 * smooth(0.62, 0.97, lit) + (0.0 if s > 0 else -0.08)
        cut.paint(dm, cut.lines((Y + 0.08 * nz) / 0.034, np.clip(tone, 0.0, 1.0)))
        cut.stroke(dome, 1.4)
        for k in range(7):
            t = (k + 0.5) / 7
            p = dome[int(t * (len(dome) - 1))]
            q = p + np.array((-s * 0.0, -0.05))
            white(cut, P.arc(q[0], q[1], 0.018, 0.018, 0.0, 360.0, step=30.0))
        cut.carve(np.array([(s * 0.32, 0.2), (s * 1.3, 0.2)]), 0.8)
    # The chest plate: a shield and his gauge, its needle near the red.
    plate = np.array([(-0.3, -0.5), (-0.3, 0.68), (0.3, 0.68), (0.3, -0.5)])
    cut.paint(cut.mask(plate), cut.lines(X / 0.03, np.where(X < 0.0, 0.5, 0.78)))
    cut.stroke(plate[1:3], 1.0)
    dial = P.arc(0.14, 0.38, 0.11, 0.11, 0.0, 360.0, step=10.0)
    white(cut, dial)
    cut.stroke(np.vstack([dial, dial[:1]]), 1.0)
    for k in range(9):
        a = math.radians(210.0 - k * 30.0)
        cut.stroke(np.array([(0.14 + 0.085 * math.cos(a), 0.38 + 0.085 * math.sin(a)),
                             (0.14 + 0.105 * math.cos(a), 0.38 + 0.105 * math.sin(a))]), 0.45)
    cut.stroke(np.array([(0.14, 0.38), (0.14 + 0.08 * math.cos(math.radians(-15.0)), 0.38 + 0.08 * math.sin(math.radians(-15.0)))]), 0.9)
    shield = np.array([(-0.24, 0.52), (-0.06, 0.52), (-0.06, 0.38), (-0.15, 0.27), (-0.24, 0.38)])
    white(cut, shield)
    cut.stroke(np.vstack([shield, shield[:1]]), 0.8)
    black(cut, P.star(-0.15, 0.415, 0.05, 6, 0.5))
    # The neck: a ringed iron collar.
    collar = Hp([(-0.15, 0.7), (-0.15, 0.88), (0.15, 0.88), (0.15, 0.7)])
    lit, nz = lambert(cut, 0.0, 0.8, 0.16, 1.0)
    cut.paint(cut.mask(collar), cut.lines(cut.Yh / 0.035, np.clip(0.95 - 0.8 * lit, 0.0, 1.0)))
    # The head: a squared iron box, bevelled at its corners, its face plate riveted round; lit on the left.
    head = Hp([(-0.31, 0.86), (-0.33, 0.89), (-0.33, 1.49), (-0.3, 1.52), (0.3, 1.52), (0.33, 1.49), (0.33, 0.89),
               (0.31, 0.86)])
    hm = cut.mask(head)
    xn = cut.Xh / 0.33
    cut.paint(hm, cut.lines(cut.Xh * HX * cut.unit / 3.2, np.clip(0.3 + 0.55 * smooth(-0.9, 0.8, xn), 0.0, 1.0)))
    cut.stroke(np.vstack([head, head[:1]]), 1.2)
    cut.carve(Hp([(-0.3, 0.9), (-0.3, 1.49)]), 0.8)                               # the lit edge
    cut.carve(Hp([(-0.29, 1.495), (0.29, 1.495)]), 0.7)
    for y in np.linspace(0.92, 1.46, 7):
        for x in (-0.27, 0.27):
            white(cut, He(x, y, 0.014, 0.0125, step=30.0))
    # The headlamp: a heavy bezel, the lens bright with a glint, short rays cut out round it.
    lamp_c, lamp_r = (0.0, 1.24), 0.15
    for k in range(12):
        a = math.radians(k * 30.0 + 15.0)
        r0, r1 = lamp_r + 0.03, lamp_r + (0.1 if k % 2 == 0 else 0.065)
        cut.carve(Hp([(lamp_c[0] + r0 * math.cos(a), lamp_c[1] + r0 * math.sin(a)),
                      (lamp_c[0] + r1 * math.cos(a), lamp_c[1] + r1 * math.sin(a))]), 0.7)
    black(cut, He(*lamp_c, lamp_r + 0.03, lamp_r + 0.03))
    lens = He(*lamp_c, lamp_r - 0.01, lamp_r - 0.01)
    lm = cut.mask(lens)
    rr = np.hypot(cut.Xh - lamp_c[0] + 0.04, cut.Yh - lamp_c[1] - 0.04) / lamp_r
    cut.paint(lm, cut.lines(rr / 0.16, np.clip(0.35 * smooth(0.35, 1.2, rr), 0.0, 1.0)))
    white(cut, He(lamp_c[0] - 0.045, lamp_c[1] + 0.045, 0.035, 0.035))
    cut.carve(Hp([(lamp_c[0] - 0.045, lamp_c[1] - 0.02), (lamp_c[0] - 0.045, lamp_c[1] + 0.11)]), 0.6)
    cut.carve(Hp([(lamp_c[0] - 0.11, lamp_c[1] + 0.045), (lamp_c[0] + 0.02, lamp_c[1] + 0.045)]), 0.6)
    # The grille mouth: dark slots in a frame.
    grille = Hp([(-0.15, 0.93), (0.15, 0.93), (0.15, 1.03), (-0.15, 1.03)])
    black(cut, grille)
    for k in range(6):
        x = -0.115 + k * 0.046
        white(cut, Hp([(x - 0.01, 0.945), (x + 0.01, 0.945), (x + 0.01, 1.015), (x - 0.01, 1.015)]))
    # The nameplate on its two posts over the head (its name is printed after, NAMEPLATE).
    for x in (-0.13, 0.13):
        black(cut, Hp([(x - 0.02, 1.52), (x + 0.02, 1.52), (x + 0.02, 1.57), (x - 0.02, 1.57)]))
    plate = Hp([(-0.24, 1.57), (0.24, 1.57), (0.24, 1.72), (-0.24, 1.72)])
    white(cut, plate)
    cut.stroke(np.vstack([plate, plate[:1]]), 1.1)
    for x in (-0.215, 0.215):
        black(cut, He(x, 1.645, 0.012, 0.012, step=30.0))
    finish(cut, sheet, ink, rng)
    # GAUGE on the plate, in the poster's own type, centred on it.
    (x0, y0), (x1, y1) = cut.mm(Hp([(-0.24, 1.57), (0.24, 1.72)]))
    P.type_line(ink, sheet, rng, 'GAUGE', 9.0, (x0 + x1) * 0.5, (y0 + y1) * 0.5 + 4.5, width=(x1 - x0) * 0.72,
                wood=True)


# --- Wendell Pike, the Surveyor ---

def bust_pike(sheet, ink, rng):
    cut = new_cut(sheet, rng)
    sky(cut)
    # A surveyor's light canvas coat, its pocket flaps; a kerchief knotted at the throat.
    coat(cut, w=1.0, lit_tone=0.03, dark_tone=0.4)
    for s in (-1.0, 1.0):
        flap = np.array([(s * 0.3, 0.42), (s * 0.62, 0.44), (s * 0.6, 0.34), (s * 0.31, 0.32)])
        cut.paint(cut.mask(flap), cut.lines(cut.v / 2.2, 0.6 if s > 0 else 0.3))
        cut.stroke(np.vstack([flap, flap[:1]]), 0.7)
    kerchief = H((-0.24, 0.78), (0.0, 0.72), (0.24, 0.78), (0.1, 0.55), (0.0, 0.42), (-0.1, 0.55), closed=True)
    dots(cut, cut.mask(kerchief), size=0.06, hole=0.27)
    cut.carve(kerchief, 0.9, closed=True)
    black(cut, He(0.0, 0.72, 0.05, 0.04))
    # His face, lit; white hair at his temples.
    face = H((-0.29, 1.02), (-0.3, 1.2), (-0.27, 1.37), (0.0, 1.45), (0.27, 1.37), (0.3, 1.2), (0.29, 1.02),
             (0.18, 0.86), (0.0, 0.8), (-0.18, 0.86), closed=True)
    face_m = cut.mask(face)
    lit, nz = lambert(cut, 0.0, 1.12, 0.3, 0.34)
    engrave(cut, face_m, lit, nz, dark=0.9, light=0.0)
    for s in (-1.0, 1.0):
        ear = He(s * 0.31, 1.18, 0.045, 0.075)
        black(cut, ear)
    # The bowler's shade on his brow, short (its brim is narrow).
    shade = H((-0.4, 1.5), (-0.3, 1.36), (0.0, 1.34), (0.3, 1.36), (0.4, 1.5), closed=True)
    cut.paint(face_m * cut.mask(shade), 1.0)
    # White mutton chops down his cheeks into a mustache, his chin shaved under it; their hairs inked.
    for s in (-1.0, 1.0):
        chop = H((s * 0.27, 1.3), (s * 0.31, 1.15), (s * 0.31, 1.0), (s * 0.26, 0.88), (s * 0.15, 0.86),
                 (s * 0.08, 0.95), (s * 0.12, 1.03), (s * 0.2, 1.1), (s * 0.21, 1.22), (s * 0.23, 1.3), closed=True)
        white(cut, chop)
        cut.stroke(np.vstack([chop, chop[:1]]), 0.55)
        strands(cut, [((s * 0.27, 1.22), (s * 0.27, 1.08), (s * 0.22, 0.94)),
                      ((s * 0.24, 1.15), (s * 0.24, 1.02), (s * 0.18, 0.93)),
                      ((s * 0.29, 1.08), (s * 0.27, 0.96))], 0.4, ink=True)
    must = H((0.0, 1.05), (0.1, 1.06), (0.16, 1.0), (0.12, 0.96), (0.05, 0.985), (0.0, 0.98), (-0.05, 0.985),
             (-0.12, 0.96), (-0.16, 1.0), (-0.1, 1.06), closed=True)
    white(cut, must)
    cut.stroke(np.vstack([must, must[:1]]), 0.55)
    cut.stroke(H((-0.05, 0.94), (0.0, 0.935), (0.05, 0.94)), 0.6)              # the mouth under it
    # The nose: its shadow to the right; round spectacles, white lenses, an eye dark in each.
    black(cut, H((0.02, 1.2), (0.05, 1.11), (0.055, 1.07), (0.02, 1.065), (0.035, 1.11), closed=True))
    for s in (-1.0, 1.0):
        rim = He(s * 0.11, 1.205, 0.072, 0.068)
        white(cut, rim)
        cut.stroke(np.vstack([rim, rim[:1]]), 0.8)
        black(cut, He(s * 0.11 + 0.006, 1.2, 0.02, 0.018))
        cut.stroke(Hp([(s * 0.182, 1.215), (s * 0.29, 1.24)]), 0.55)            # the bows to the ears
    cut.stroke(H((-0.04, 1.215), (0.0, 1.235), (0.04, 1.215)), 0.6)             # the bridge
    # His brows, white and bushy over the rims.
    for s in (-1.0, 1.0):
        brow = H((s * 0.04, 1.29), (s * 0.12, 1.32), (s * 0.2, 1.3), (s * 0.18, 1.285), (s * 0.1, 1.295), closed=True)
        white(cut, brow)
    # The bowler: a round crown, a narrow brim rolled up at the sides, a band with his goggles on it.
    crown = H((-0.32, 1.44), (-0.33, 1.56), (-0.28, 1.69), (-0.15, 1.77), (0.0, 1.79), (0.15, 1.77), (0.28, 1.69),
              (0.33, 1.56), (0.32, 1.44))
    hat = cut.mask(crown)
    lit, nz = lambert(cut, 0.0, 1.5, 0.33, 0.3)
    engrave(cut, hat, lit, nz, period=0.03, bow=0.08, dark=0.92, light=0.18, angle=90.0)
    cut.stroke(crown, 1.3)
    black(cut, H((-0.325, 1.44), (0.325, 1.44), (0.325, 1.52), (-0.325, 1.52), closed=True))
    for s in (-1.0, 1.0):
        g = He(s * 0.11, 1.52, 0.075, 0.07)
        black(cut, g)
        lensm = He(s * 0.11, 1.52, 0.05, 0.047)
        white(cut, lensm)
        cut.carve(Hp([(s * 0.11 - 0.025, 1.5), (s * 0.11 + 0.005, 1.55)]), 0.5)
        black(cut, He(s * 0.11 + 0.012, 1.512, 0.022, 0.02))
    black(cut, Hp([(-0.04, 1.51), (0.04, 1.51), (0.04, 1.53), (-0.04, 1.53)]))
    top = H((-0.56, 1.52), (-0.5, 1.47), (-0.36, 1.44), (0.0, 1.43), (0.36, 1.44), (0.5, 1.47), (0.56, 1.52))
    under = H((0.56, 1.52), (0.48, 1.42), (0.3, 1.39), (0.0, 1.385), (-0.3, 1.39), (-0.48, 1.42), (-0.56, 1.52))
    brim(cut, top, under, base=0.3)
    finish(cut, sheet, ink, rng)


# --- The words under the bust ---

def hero_print(sheet, black_ink, red, rng, hero):
    """Everything under the poster's top for a hero (looter_posters.poster_print): the bust, the name in wood type,
    what for, the reward in red (and who pays it), Calder's printed name."""
    W = sheet.size[0]
    hero['bust'](sheet, black_ink, rng)
    base = P.NAME_BASELINE
    cap = hero.get('cap', 42.0)
    natural = P.set_line(hero['name'], cap)[1]
    tracking = hero.get('tracking', 0.0)
    width = min(372.0, natural * hero.get('stretch', 1.0) + tracking * (len(hero['name']) - 1))
    P.type_line(black_ink, sheet, rng, hero['name'], cap, W * 0.5, base, width=width, tracking=tracking, wood=True)
    epithet, crime = hero['what']
    P.type_line(black_ink, sheet, rng, epithet, 15.0, W * 0.5, base + 26.0,
                width=min(340.0, P.set_line(epithet, 15.0)[1] * 1.04))
    P.type_line(black_ink, sheet, rng, crime, 12.5, W * 0.5, base + 46.0, width=min(350.0, P.set_line(crime, 12.5)[1]))
    P.rule(black_ink, sheet, 150.0, W - 150.0, base + 60.0, 0.8, rng)
    for x in (W * 0.5 - 158.0, W * 0.5 + 158.0):
        P.type_shape(black_ink, sheet, [P.diamond(x, base + 60.0, 4.0, 2.4)])
    if hero.get('paid'):
        P.type_line(red, sheet, rng, P.REWARD, 42.0, W * 0.5, 584.0, width=330.0, wood=True)
        P.type_line(red, sheet, rng, hero['paid'], 10.0, W * 0.5, 603.0, width=min(250.0, P.set_line(hero['paid'], 10.0)[1]))
    else:
        P.type_line(red, sheet, rng, P.REWARD, 46.0, W * 0.5, 591.0, width=360.0, wood=True)
    P.type_line(black_ink, sheet, rng, P.SIGN_OFF, 11.5, W * 0.5 + 40.0, 648.0, width=250.0)


# The heroes, by UHeroDefinition id: the name as the poster prints it (DisplayName, upper case), what for (an epithet,
# then the crime), who pays the reward when it isn't the Rangers, the bust. Wood type for a short name is spaced out.
HERO_POSTERS = {
    'Odessa': dict(name='ODESSA LARK', what=('CARDSHARP.', CRIME), bust=bust_odessa, print=hero_print),
    'Crane': dict(name='HOLLIS CRANE', what=('CALLS HIMSELF A MARSHAL.', CRIME), bust=bust_crane, print=hero_print),
    'Gauge': dict(name='GAUGE', what=('RAILROAD PROPERTY.', CRIME), paid='PAID BY THE RAILROAD', bust=bust_gauge,
                  print=hero_print, cap=44.0, stretch=1.25, tracking=9.0),
    'Pike': dict(name='WENDELL PIKE', what=('SURVEYOR.', CRIME), bust=bust_pike, print=hero_print),
}
