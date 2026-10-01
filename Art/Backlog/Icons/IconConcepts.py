"""Five icon styles for the user to choose from (2026-10-01): each draws the assault rifle (the bullpup), the shotgun (the
Ranchhand) and the five ammo types. The chosen style becomes the template for every future weapon's icon. Concept art
kept in Art/Backlog (see its README): the game's icons (LoadoutParts' GunIcon/AmmoIcon) are unchanged.

The weapon silhouettes are traced from the models' own outlines (Art/Models/Weapons/Bullpup.py and Ranchhand.py, gun
space in cm), so an icon matches the gun in hand. The ammo types are told apart by shape and count, never by color
(rarity colors are the game's only color code): pistol one short round-nosed round, SMG three short rounds, assault
rifle two bottlenecked rounds, sniper one tall pointed round, shotgun a fat shell.

The game draws icons as LooterUI::FVectorIcon (convex fills and strokes, one tint per layer), so every style here is
one or two flat layers; cut-out details become gaps between convex pieces.

    python Art/Backlog/Icons/IconConcepts.py      (writes HTML sheets to Saved/ArtPreviews/Backlog/Icons)
"""
import math
import os

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Backlog', 'Icons')
FONTS = 'file:///' + REPO.replace('\\', '/') + '/Content/UI/Fonts'

# LooterUI palette.
TEXT, DIM, CYAN, ACCENT, BG, PANEL = '#DCEFFF', '#8FB3CC', '#5ACAFF', '#FF9F1C', '#0B0E12', '#0E1A24'
RARE, UNCOMMON = '#3366FF', '#33CC33'


# --- Geometry: polygons as lists of (x, y) in SVG space (y down) ---

def quad(p0, p1, p2, n=8):
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]) for t in (k / n for k in range(n + 1))]


def rect(x0, x1, y0, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def flip(points):
    """Gun space (u along, v up, cm) to SVG (x right, y down)."""
    return [(u, -v) for u, v in points]


def circle(cx, cy, r, n=20):
    return [(cx + r * math.cos(2 * math.pi * k / n), cy + r * math.sin(2 * math.pi * k / n)) for k in range(n)]


class Icon:
    """An icon: fills (solid shapes), holes (cut through every fill), cuts (detail lines cut into the fills), strokes
    (thin solid lines drawn in the icon color), and a shade line: below it the duotone style darkens the fill."""

    def __init__(self, view, fills, holes=(), cuts=(), strokes=(), shade=None, cut_width=0.35, stroke_width=0.6):
        self.view, self.fills, self.holes, self.cuts, self.strokes = view, fills, list(holes), list(cuts), list(strokes)
        self.shade, self.cut_width, self.stroke_width = shade, cut_width, stroke_width


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
    fills = [flip(p) for p in (
        shell, loop, mount, rect(14, 32, 5.2, 6.15), rect(19.4, 26.6, v0, v0 + 1.3),
        [(19, v0 + 1.35), (20, v0 + 1.35), (20.4, v0 + 1.55), (25.6, v0 + 1.55), (26, v0 + 1.35), (27, v0 + 1.35),
         (27, v0 + 4.25), (26, v0 + 4.25), (25.6, v0 + 4.05), (20.4, v0 + 4.05), (20, v0 + 4.25), (19, v0 + 4.25)],
        mag, rect(-1.5, 0.2, -8.8, 2.0), rect(50.6, 51.9, -1.25, 1.25), rect(51.9, 63.9, -0.8, 0.8),
        rect(54.9, 57.5, -1.0, 1.5), rect(63.9, 69.7, -0.98, 0.98), [(38.8, 2.9), (44.0, 2.6), (44.0, 3.6), (39.6, 3.9)],
        trigger)]
    holes = [flip(loop_hole)]
    cuts = [flip(p) for p in (
        [(8, -4.8), (18.2, -4.8), (19.4, 1.6), (9.4, 1.6), (8, -4.8)],      # the side plate
        [(12, 3.0), (34, 3.0)],                                           # the mount's seam
        [(40.6, -1.6), (41.9, 1.2)], [(42.8, -1.6), (44.1, 1.2)], [(45.0, -1.6), (46.3, 1.2)],   # the vents
        [(0.2, -8.6), (0.2, 2.0)],                                        # the pad's seam
        [(65.6, -0.98), (65.6, 0.98)], [(67.6, -0.98), (67.6, 0.98)],     # the birdcage's slots
        [(11.0, -14.0), (18.6, -14.0)],                                   # a rib on the magazine
        [(21.0, -5.2), (36.8, -4.6)])]                                    # where the grip loop meets the shell
    return Icon((-2.6, -11.6, 73.4, 37.6), fills, holes, cuts, shade=1.8)


# --- The shotgun: the Ranchhand (Standard body, field barrel, crown, 6-shell tube, flip sight, skeleton stock) ---

def ranchhand():
    v0 = 3.15
    stock = [(0, 1.8), (0, -3.0), (-2.5, -4.4), (-5.5, -7.4), (-9, -8.0), (-12, -7.4), (-37, -12.6), (-38.5, -12.4),
             (-38.5, 0.2), (-36, 0.6), (-14, 1.0), (-4, 1.8)]
    stock_hole = [(-16.5, -8.0), (-33.0, -10.9), (-34.2, -10.0), (-34.0, -6.4), (-32.5, -5.6), (-18.0, -4.9)]
    pump = [(28, -1.4), (46, -1.4), (47, -2.4), (46.5, -4.0), (45, -4.4), (29, -4.4), (27.5, -3.6), (27.5, -2.0)]
    fills = [flip(p) for p in (
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
    return Icon((-41.4, -6.2, 109.4, 19.6), fills, holes, cuts, strokes, shade=-0.6, cut_width=0.3, stroke_width=0.45)


# --- Ammo, in a 64 x 64 box ---

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


def ammo_icon(kind):
    view = (0, 0, 64, 64)
    if kind == 'AssaultRifle':
        parts = [cartridge(cx, 58, 11.0, 33, 29, 7.0, 25, 10, 'point') for cx in (24.0, 40.0)]
        shade = 45
    elif kind == 'Sniper':
        parts = [cartridge(32, 61, 12.0, 27, 21, 7.2, 15, 2.5, 'point')]
        shade = 48
    elif kind == 'Pistol':
        parts = [cartridge(32, 57, 17.0, 37, 37, 17.0, 37, 20, 'round')]
        shade = 48
    elif kind == 'SMG':
        parts = [cartridge(cx, 57, 12.0, 40, 40, 12.0, 40, 28, 'flat') for cx in (16.5, 32.0, 47.5)]
        shade = 49
    else:
        parts = [shell(32, 59, 21.0, 10)]
        shade = 49
    fills = [p[0] for p in parts]
    cuts = [c for p in parts for c in p[1]]
    return Icon(view, fills, (), cuts, shade=shade, cut_width=1.1)


AMMO = [('AssaultRifle', 'Assault rifle'), ('Shotgun', 'Shotgun'), ('Pistol', 'Pistol'), ('SMG', 'SMG'),
        ('Sniper', 'Sniper')]


# --- Drawing an icon in a style ---

def path(points, close=True):
    d = 'M' + ' L'.join(f'{x:.2f},{y:.2f}' for x, y in points)
    return d + (' Z' if close else '')


def body(icon):
    return ' '.join(path(p) for p in icon.fills)


def svg(icon, style, width, height, ident, color=TEXT):
    """The icon as an inline SVG drawn in a style. Holes cut through by evenodd masks; cuts drawn in the background."""
    x0, y0, w, h = icon.view
    pad = max(w, h) * (0.06 if style != 'badge' else 0.0)
    vb = f'{x0 - pad:.2f} {y0 - pad:.2f} {w + 2 * pad:.2f} {h + 2 * pad:.2f}'
    unit = max(w, h) / 64.0                 # scale strokes with the icon
    mask_id = f'm{ident}'
    hole_paths = ''.join(f'<path d="{path(p)}" fill="black"/>' for p in icon.holes)
    cut_paths = ''.join(f'<path d="{path(p, False)}" stroke="black" stroke-width="{icon.cut_width}" fill="none" '
                        f'stroke-linecap="butt"/>' for p in icon.cuts)
    stroke_paths = lambda c, wdt: ''.join(f'<path d="{path(p, False)}" stroke="{c}" stroke-width="{wdt}" fill="none" '
                                          f'stroke-linejoin="round" stroke-linecap="round"/>' for p in icon.strokes)
    defs = (f'<mask id="{mask_id}" maskUnits="userSpaceOnUse" x="{x0 - pad - 5}" y="{y0 - pad - 5}" width="{w + 2 * pad + 10}" '
            f'height="{h + 2 * pad + 10}"><rect x="{x0 - pad - 5}" y="{y0 - pad - 5}" width="{w + 2 * pad + 10}" '
            f'height="{h + 2 * pad + 10}" fill="white"/>{hole_paths}CUTS</mask>')
    inner = ''
    if style == 'stencil':
        # Spray-painted stencil: solid, with stencil bridges (the gaps that hold a real stencil together) and overspray.
        bw = 0.9 * unit if max(w, h) > 64 else 2.4
        if max(w, h) > 64:
            bridges = [[(x0 + w * f, y0 - 5), (x0 + w * f, y0 + h + 5)] for f in (0.22, 0.47, 0.71)]
        else:
            bridges = [[(-5, icon.shade - 8), (70, icon.shade - 8)]]
        bridge_paths = ''.join(f'<path d="{path(p, False)}" stroke="black" stroke-width="{bw}" fill="none"/>'
                               for p in bridges)
        defs = defs.replace('CUTS', bridge_paths + cut_paths.replace(f'stroke-width="{icon.cut_width}"',
                                                                     f'stroke-width="{icon.cut_width * 1.5}"'))
        defs += (f'<filter id="spray{ident}" x="-5%" y="-5%" width="110%" height="110%"><feTurbulence type="fractalNoise" '
                 f'baseFrequency="{1.6 / unit:.3f}" numOctaves="2" seed="4"/><feDisplacementMap in="SourceGraphic" scale="{0.5 * unit:.2f}"/></filter>')
        inner = (f'<g filter="url(#spray{ident})" opacity="0.95"><path d="{body(icon)}" fill="{color}" mask="url(#{mask_id})"/>'
                 f'{stroke_paths(color, icon.stroke_width)}</g>')
    elif style == 'line':
        defs = defs.replace('CUTS', '')
        lw = 0.9 * unit if max(w, h) > 64 else 1.6
        inner = (f'<path d="{body(icon)}" fill="{CYAN}" fill-opacity="0.10" stroke="{CYAN}" stroke-width="{lw}" '
                 f'stroke-linejoin="round"/>'
                 + ''.join(f'<path d="{path(p)}" fill="{BG}" stroke="{CYAN}" stroke-width="{lw}"/>' for p in icon.holes)
                 + ''.join(f'<path d="{path(p, False)}" stroke="{CYAN}" stroke-width="{lw * 0.6}" fill="none" opacity="0.8"/>'
                           for p in icon.cuts) + stroke_paths(CYAN, lw))
    elif style == 'solid':
        defs = defs.replace('CUTS', cut_paths)
        inner = f'<path d="{body(icon)}" fill="{color}" mask="url(#{mask_id})"/>' + stroke_paths(color, icon.stroke_width)
    elif style == 'duotone':
        defs = defs.replace('CUTS', cut_paths)
        ow = (1.5 * unit) if max(w, h) > 64 else 3.2
        clip = f'c{ident}'
        defs += (f'<clipPath id="{clip}"><rect x="{x0 - 50}" y="{icon.shade}" width="{w + 100}" height="{h + 100}"/></clipPath>')
        inner = (f'<path d="{body(icon)}" fill="none" stroke="#0A1218" stroke-width="{ow * 2}" stroke-linejoin="round"/>'
                 + stroke_paths('#0A1218', icon.stroke_width + ow * 2)
                 + f'<path d="{body(icon)}" fill="#F4EFE6" mask="url(#{mask_id})"/>'
                 + f'<path d="{body(icon)}" fill="#8EA3B4" mask="url(#{mask_id})" clip-path="url(#{clip})"/>'
                 + stroke_paths('#F4EFE6', icon.stroke_width))
    elif style == 'badge':
        defs = defs.replace('CUTS', cut_paths)
        inner = f'<path d="{body(icon)}" fill="{color}" mask="url(#{mask_id})"/>' + stroke_paths(color, icon.stroke_width)
    return (f'<svg viewBox="{vb}" width="{width}" height="{height}" preserveAspectRatio="xMidYMid meet">'
            f'<defs>{defs}</defs>{inner}</svg>')


def badge_wrap(inner_svg, w, h, label=None):
    """The badge style: a chamfered hexagon plate around the icon (matching the HUD's weapon slots)."""
    pts = f'2,{h / 2} {w * 0.18},2 {w * 0.82},2 {w - 2},{h / 2} {w * 0.82},{h - 2} {w * 0.18},{h - 2}'
    lab = (f'<div style="position:absolute;left:0;right:0;bottom:-30px;text-align:center;font:700 18px Chakra;'
           f'letter-spacing:3px;color:{DIM}">{label}</div>' if label else '')
    return (f'<div style="position:relative;width:{w}px;height:{h}px">'
            f'<svg width="{w}" height="{h}" style="position:absolute;inset:0"><polygon points="{pts}" fill="rgba(14,44,66,0.85)" '
            f'stroke="{CYAN}" stroke-width="2"/><polygon points="{pts}" fill="none" stroke="rgba(90,200,255,0.25)" '
            f'stroke-width="8" transform="translate({w * 0.04} {h * 0.04}) scale(0.92)"/></svg>'
            f'<div style="position:absolute;inset:{h * 0.16}px {w * 0.14}px;display:flex;align-items:center;justify-content:center">'
            f'{inner_svg}</div>{lab}</div>')


STYLES = [
    ('stencil', 'Stencil', 'Spray-painted crate stencils',
     'Solid silhouettes with stencil bridges and a soft overspray edge, like the marks painted on supply crates. '
     'Rustic and military; the same marks could be printed on the ammo boxes.'),
    ('line', 'Blueprint', 'Technical line art',
     'Thin cyan outlines with the parts drawn in, matching the menus’ Concept C screens. Elegant up close; '
     'reads lighter at small sizes.'),
    ('solid', 'Cutout', 'Bold solid silhouettes',
     'Solid shapes with the details cut out as thin gaps, the classic modern-shooter icon. The crispest at the small '
     'sizes of the HUD and inventory.'),
    ('duotone', 'Inked', 'Two-tone with a dark outline',
     'A light top and a shaded underside inside a thick dark outline, like the comic-inked Borderlands look. '
     'Reads over any background; two layers in the game.'),
    ('badge', 'Badge', 'Icons in hex plates',
     'The solid icon framed in a chamfered hexagon, the shape of the new HUD’s weapon slots, so every icon looks '
     'like a part of the same kit; ammo badges carry their short code.'),
]
# The backdrop each style is shown on: what it's meant to sit on, or what shows its edges.
STAGE = {
    'stencil': 'repeating-linear-gradient(90deg, rgba(0,0,0,0.06) 0 3px, transparent 3px 9px), #4A5636',
    'line': 'linear-gradient(rgba(90,200,255,0.08) 1px, transparent 1px) 0 0 / 24px 24px, '
            'linear-gradient(90deg, rgba(90,200,255,0.08) 1px, transparent 1px) 0 0 / 24px 24px, #08202F',
    'solid': 'transparent',
    'duotone': 'linear-gradient(180deg, #56636F, #3A4651)',
    'badge': 'transparent',
}
CODES = {'AssaultRifle': 'AR', 'Shotgun': 'SG', 'Pistol': 'PS', 'SMG': 'SMG', 'Sniper': 'SN'}


def hex_slot(content, active, rarity, w=96, h=72):
    """One of the approved HUD's weapon slots (UPlayerHUDWidget), with an icon in it."""
    rx, ry = w / 2 - 6, h / 2 - 6
    cx, cy = w / 2, h / 2
    pts = f'{cx - rx},{cy} {cx - rx * 0.55},{cy - ry} {cx + rx * 0.55},{cy - ry} {cx + rx},{cy} {cx + rx * 0.55},{cy + ry} {cx - rx * 0.55},{cy + ry}'
    foot = f'{cx - rx + 4},{cy + 2} {cx - rx * 0.55 + 3},{cy + ry - 3} {cx + rx * 0.55 - 3},{cy + ry - 3} {cx + rx - 4},{cy + 2}'
    fill = f'{rarity}55' if active else 'rgba(7,26,40,0.5)'
    line = ACCENT if active else rarity
    return (f'<div style="position:relative;width:{w}px;height:{h}px">'
            f'<svg width="{w}" height="{h}" style="position:absolute;inset:0"><polygon points="{pts}" fill="{fill}" '
            f'stroke="{line}" stroke-width="{3 if active else 2}"/><polyline points="{foot}" fill="none" stroke="{rarity}" '
            f'stroke-width="5" stroke-linejoin="round"/></svg>'
            f'<div style="position:absolute;inset:16px 14px;display:flex;align-items:center;justify-content:center">{content}</div></div>')


def sheet(index, style, name, kind, desc):
    rifle, shotgun = bullpup(), ranchhand()
    ammo = [(key, label, ammo_icon(key)) for key, label in AMMO]
    big_ar = svg(rifle, style, 620, 300, f'ar{style}')
    big_sg = svg(shotgun, style, 760, 160, f'sg{style}')
    if style == 'badge':
        big_ar = badge_wrap(svg(rifle, style, 470, 230, f'ar{style}'), 620, 300)
        big_sg = badge_wrap(svg(shotgun, style, 600, 120, f'sg{style}'), 760, 190)
    tiles = []
    for key, label, icon in ammo:
        if style == 'badge':
            tile = badge_wrap(svg(icon, style, 110, 110, f'a{key}{style}'), 180, 150, CODES[key])
        else:
            tile = svg(icon, style, 150, 150, f'a{key}{style}')
        lab = '&nbsp;' if (style == 'badge' and CODES[key] == label) else label
        tiles.append(f'<div class="tile">{tile}<div class="lab">{lab}</div></div>')
    # In use: HUD slots with rarity-tinted weapon icons, the ammo readout, an inventory chip row.
    tint = lambda col: col if style in ('solid', 'stencil', 'badge') else TEXT
    slot_ar = svg(rifle, 'duotone' if style == 'duotone' else ('line' if style == 'line' else 'solid'), 66, 40, f'sar{style}',
                  color=TEXT)
    slot_sg = svg(shotgun, 'duotone' if style == 'duotone' else ('line' if style == 'line' else 'solid'), 68, 30, f'ssg{style}',
                  color=UNCOMMON)
    if style == 'stencil':
        slot_ar = svg(rifle, 'stencil', 66, 40, f'sar{style}', color=TEXT)
        slot_sg = svg(shotgun, 'stencil', 68, 30, f'ssg{style}', color=UNCOMMON)
    readout_icon = svg(ammo[0][2], style if style != 'badge' else 'solid', 40, 40, f'ro{style}')
    chips = ''.join(f'<div class="chip">{svg(icon, style if style != "badge" else "solid", 30, 30, f"ch{key}{style}")}'
                    f'<span>{n}</span></div>' for (key, label, icon), n in zip(ammo, (186, 42, 96, 240, 18)))
    return f"""<!doctype html><html><head><meta charset="utf-8"><style>
@font-face {{ font-family: Chakra; src: url('{FONTS}/ChakraPetch-Bold.ttf'); font-weight: 700; }}
@font-face {{ font-family: Chakra; src: url('{FONTS}/ChakraPetch-Regular.ttf'); font-weight: 400; }}
html, body {{ margin: 0; width: 1600px; height: 1070px; background: {BG}; font-family: Chakra; color: {TEXT}; overflow: hidden; }}
.head {{ padding: 30px 48px 10px; display: flex; align-items: baseline; gap: 24px; border-bottom: 2px solid {ACCENT}; }}
.kick {{ color: {ACCENT}; font-weight: 700; font-size: 22px; letter-spacing: 6px; }}
.name {{ font-weight: 700; font-size: 52px; }}
.kind {{ color: {CYAN}; font-weight: 700; font-size: 22px; letter-spacing: 2px; text-transform: uppercase; }}
.desc {{ padding: 14px 48px 0; color: #C7CED6; font-size: 23px; line-height: 1.4; }}
.row {{ display: flex; align-items: center; justify-content: space-around; padding: 10px 40px; margin: 6px 48px; background: {STAGE[style]}; }}
.sec {{ padding: 8px 48px 0; color: {DIM}; font-weight: 700; font-size: 18px; letter-spacing: 4px; }}
.tile {{ display: flex; flex-direction: column; align-items: center; gap: 8px; }}
.lab {{ color: {DIM}; font-size: 20px; font-weight: 700; letter-spacing: 2px; text-transform: uppercase; margin-top: 18px; }}
.use {{ display: flex; gap: 40px; padding: 10px 48px; align-items: stretch; }}
.hud {{ width: 640px; height: 200px; background: url('file:///{REPO.replace(chr(92), '/')}/Saved/Screenshots/Tour/Slimes_Medium.png') -1100px -760px;
  position: relative; border: 1px solid #1a2a36; }}
.inv {{ flex: 1; background: rgba(7,26,40,0.95); border: 1px solid rgba(90,200,255,0.4); padding: 18px 22px; }}
.chips {{ display: flex; gap: 14px; margin-top: 14px; }}
.chip {{ display: flex; align-items: center; gap: 8px; background: rgba(18,64,94,0.55); border: 1px solid rgba(90,200,255,0.3);
  padding: 6px 12px; font-weight: 700; font-size: 20px; }}
.o {{ text-shadow: 0 0 3px #001824, 0 0 3px #001824, 0 0 3px #001824; }}
</style></head><body>
<div class="head"><span class="kick">ICON STYLE {index} / 5</span><span class="name">{name}</span><span class="kind">{kind}</span></div>
<div class="desc">{desc}</div>
<div class="sec">WEAPONS</div>
<div class="row">{big_ar}{big_sg}</div>
<div class="sec">AMMO</div>
<div class="row">{''.join(tiles)}</div>
<div class="sec">IN USE (ACTUAL SIZE)</div>
<div class="use">
 <div class="hud">
  <div style="position:absolute;left:24px;top:22px;display:flex;gap:10px">{hex_slot(slot_ar, True, RARE)}{hex_slot(slot_sg, False, UNCOMMON)}</div>
  <div style="position:absolute;right:28px;bottom:38px;display:flex;align-items:center;gap:12px">{readout_icon}
   <span class="o" style="font-weight:700;font-size:48px">24</span><span class="o" style="font-weight:700;font-size:22px;color:{DIM}">186</span></div>
 </div>
 <div class="inv"><div style="display:flex;align-items:center;gap:18px">
  <div style="width:150px">{svg(rifle, style if style != 'badge' else 'solid', 150, 70, f'inv{style}', color=RARE if style in ('solid', 'stencil', 'badge') else TEXT)}</div>
  <div><div style="font-weight:700;font-size:24px;color:#6E93FF">SILENT ASSAULT RIFLE</div>
   <div style="color:{DIM};font-size:18px;margin-top:4px">LV 4 &middot; RARE &middot; FULL-AUTO</div></div></div>
  <div class="chips">{chips}</div></div>
</div></body></html>"""


def main():
    os.makedirs(OUT, exist_ok=True)
    for i, (style, name, kind, desc) in enumerate(STYLES, 1):
        with open(os.path.join(OUT, f'icons_{i}_{name}.html'), 'w', encoding='utf-8') as f:
            f.write(sheet(i, style, name, kind, desc))
    print('wrote', OUT)


if __name__ == '__main__':
    main()
