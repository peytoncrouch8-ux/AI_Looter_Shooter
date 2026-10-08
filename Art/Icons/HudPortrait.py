"""The HUD's portrait of the player character (Art/Icons/HudPortrait.svg, the user's pick on 2026-10-03) turned into
C++ data: painted pictures the game draws into textures once (LooterUI::FPaintedIcon, UI/Style/LooterUIPaintedIcons.cpp)
and the soft glows behind the eyes (LooterUI::GlowBrush). UHudPortraitWidget stacks them in the player frame's diamond
window, moves them and fades them (UI/HUD/HudPortraitWidget.cpp).

The SVG is where the portrait is drawn; this file only reads it. Running it writes
    Source/AI_Looter_Shooter/UI/Style/HudPortraitData.inl   the portrait as C++ data (generated: change the SVG and run
                                                            this again)
    Saved/ArtPreviews/Icons/HudPortrait_<State>.png         the window drawn from that data, big and at its HUD size
                                                            (calm, mid-blink, hurt, flaring), to check against the SVG

    python Art/Icons/HudPortrait.py
    The data needs any Python 3; the previews need numpy (Blender's Python has it:
    "C:/Program Files/Blender Foundation/Blender 4.4/4.4/python/bin/python.exe" Art/Icons/HudPortrait.py).

How the SVG becomes pictures. Each picture is one texture, so the widget can move or fade it on its own:
  Glass      the glass group: its radial gradient as one radial layer per stretch between two stops (so the colour runs
             exactly as the SVG's), and its scanline pattern as one layer of thin rectangles
  Bust       the colour group (shifted by its misregistration offset), the rim light and the ink: all that breathes
  EyesOpen   the open eyes' slits: they squash for the blink
  BrowsCalm  the open eyes' brows: they stay put while the eyes blink (the mockup blinks the eyes only)
  EyesHurt   the squint: slits and pulled-down brows
  EyesFlare  the level-up flare's slits
The eye groups' ellipses filled with the eye gradient become glows instead: a body in the gradient's middle colour and
a white-hot core in its first, drawn by GlowBrush (white falling as (1 - r^2)^2, tinted) under the group's picture.

The window: the SVG clips everything to the diamond inscribed in its square (the clipPath "window"). The game clips to
that diamond in the widget, not in the pictures, because the content shakes and breathes behind a window that stays
put; so each picture keeps its art up to MARGIN units past the square. Pictures start and end on whole HUD pixels
(SNAP units) so that at rest their textures (drawn at twice the HUD size) land 2:1 on the screen's pixels.

Supported SVG: groups (fill, stroke, stroke-width, opacity, transform translate), paths (M L H V C Q Z, absolute or
relative), rects, polygons, and ellipses filled with a radial gradient (the glows). Anything else stops the script, so a
change to the SVG is never dropped quietly.
"""
import math
import os
import re
import struct
import xml.etree.ElementTree as ET
import zlib

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SVG_IN = os.path.join(REPO, 'Art', 'Icons', 'HudPortrait.svg')
DATA_OUT = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'UI', 'Style', 'HudPortraitData.inl')
PREVIEW_OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Icons')

HUD_SCALE = 0.64        # px a unit on a 1080p screen: the 200-unit window is 128 px tip to tip (the spec)
SNAP = 1.0 / HUD_SCALE  # one HUD pixel, in units
MARGIN = 10.0           # art kept past the square: the shake (4 px) and the breath (1.4 px) reach about 8.5 units
PAD = 1.5               # room round a picture's art for its anti-aliased edge and the texture filter
TOLERANCE = 0.04        # curves are flattened until they stray less than this (0.05 px in the 2x texture)
GLOW_BODY_PEAK = 0.9    # the glow body's opacity in the middle: 0.9 (1 - r^2)^2 follows the gradient's 0.7 at r 0.3,
                        # 0.5 at r 0.5 and 0.3 at r 0.7
GLOW_CORE_SIZE = 0.45   # the white-hot core spans this share of the ellipse (the gradient's first stretch, out to 0.3)
GLOW_CORE_PEAK = 0.7

# Each picture: its name, the SVG groups it takes, and which of their layers ('all', 'fills' or 'strokes').
PICTURES = (
    ('Glass', ('glass',), 'all'),
    ('Bust', ('colour', 'rim', 'ink'), 'all'),
    ('EyesOpen', ('eyes-open',), 'fills'),
    ('BrowsCalm', ('eyes-open',), 'strokes'),
    ('EyesHurt', ('eyes-hurt',), 'all'),
    ('EyesFlare', ('eyes-flare',), 'all'),
)
# The glows of each eye group, by the name of their C++ function.
GLOWS = (('EyesOpenGlows', 'eyes-open'), ('EyesHurtGlows', 'eyes-hurt'), ('EyesFlareGlows', 'eyes-flare'))
GROUPS = {group for _, groups, _ in PICTURES for group in groups}


# --- Reading the SVG ---

def local(tag):
    return tag.split('}')[-1]


def colour(text):
    """'#rrggbb' or '#rgb' as an (r, g, b) byte triple."""
    text = text.strip()
    if not re.fullmatch(r'#[0-9a-fA-F]{6}|#[0-9a-fA-F]{3}', text):
        raise ValueError(f'unsupported colour {text!r}')
    if len(text) == 4:
        text = '#' + ''.join(c * 2 for c in text[1:])
    return tuple(int(text[i:i + 2], 16) for i in (1, 3, 5))


def translate(text):
    """A transform attribute, which may only translate, as (dx, dy)."""
    match = re.fullmatch(r'\s*translate\(\s*([-+.\deE]+)(?:[\s,]+([-+.\deE]+))?\s*\)\s*', text)
    if not match:
        raise ValueError(f'unsupported transform {text!r} (only translate)')
    return float(match.group(1)), float(match.group(2) or 0.0)


def point_chord_distance(p, a, b):
    dx, dy = b[0] - a[0], b[1] - a[1]
    length = math.hypot(dx, dy)
    if length < 1e-12:
        return math.hypot(p[0] - a[0], p[1] - a[1])
    return abs((p[0] - a[0]) * dy - (p[1] - a[1]) * dx) / length


def flatten_cubic(p0, p1, p2, p3, depth=0):
    """The cubic's points after p0, split in halves until each half's control points lie within TOLERANCE of its chord
    (the curve then strays less than that)."""
    if depth >= 14 or max(point_chord_distance(p1, p0, p3), point_chord_distance(p2, p0, p3)) <= TOLERANCE:
        return [p3]
    mid = lambda a, b: ((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5)
    p01, p12, p23 = mid(p0, p1), mid(p1, p2), mid(p2, p3)
    p012, p123 = mid(p01, p12), mid(p12, p23)
    p0123 = mid(p012, p123)
    return flatten_cubic(p0, p01, p012, p0123, depth + 1) + flatten_cubic(p0123, p123, p23, p3, depth + 1)


PATH_TOKEN = re.compile(r'[A-Za-z]|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?')


def parse_path(d):
    """A path's subpaths as (points, closed), its curves flattened."""
    tokens = PATH_TOKEN.findall(d)
    unknown = {t for t in tokens if t.isalpha()} - set('MmLlHhVvCcQqZz')
    if unknown:
        raise ValueError(f'unsupported path commands {sorted(unknown)} in {d[:40]!r}')
    subpaths, points = [], None
    cur = start = (0.0, 0.0)
    cmd, i = None, 0

    def numbers(count):
        nonlocal i
        values = [float(t) for t in tokens[i:i + count]]
        if len(values) < count or any(t.isalpha() for t in tokens[i:i + count]):
            raise ValueError(f'path ends early: {d[:40]!r}')
        i += count
        return values

    def finish(closed):
        nonlocal points
        if points and len(points) > 1:
            subpaths.append((points, closed))
        points = None

    while i < len(tokens):
        if tokens[i].isalpha():
            cmd = tokens[i]
            i += 1
            if cmd in 'Zz':
                finish(True)
                cur = start
                continue
        elif cmd is None:
            raise ValueError(f'path starts without a command: {d[:40]!r}')
        rel = cmd.islower()
        ox, oy = cur if rel else (0.0, 0.0)
        kind = cmd.upper()
        if kind == 'M':
            x, y = numbers(2)
            finish(False)
            cur = start = (x + ox, y + oy)
            points = [cur]
            cmd = 'l' if rel else 'L'   # pairs after a moveto are linetos
            continue
        if points is None:              # drawing on after a Z starts a new subpath where the last one started
            points = [cur]
        if kind == 'L':
            x, y = numbers(2)
            cur = (x + ox, y + oy)
            points.append(cur)
        elif kind == 'H':
            (x,) = numbers(1)
            cur = (x + ox, cur[1])
            points.append(cur)
        elif kind == 'V':
            (y,) = numbers(1)
            cur = (cur[0], y + oy)
            points.append(cur)
        elif kind == 'C':
            x1, y1, x2, y2, x, y = numbers(6)
            p1, p2, p3 = (x1 + ox, y1 + oy), (x2 + ox, y2 + oy), (x + ox, y + oy)
            points += flatten_cubic(cur, p1, p2, p3)
            cur = p3
        elif kind == 'Q':
            x1, y1, x, y = numbers(4)
            q, p3 = (x1 + ox, y1 + oy), (x + ox, y + oy)
            # The same curve as a cubic: its control points two thirds of the way to the quadratic's.
            p1 = (cur[0] + (q[0] - cur[0]) * 2 / 3, cur[1] + (q[1] - cur[1]) * 2 / 3)
            p2 = (p3[0] + (q[0] - p3[0]) * 2 / 3, p3[1] + (q[1] - p3[1]) * 2 / 3)
            points += flatten_cubic(cur, p1, p2, p3)
            cur = p3
    finish(False)
    # Drop repeated points (a curve ending where a line starts).
    cleaned = []
    for pts, closed in subpaths:
        kept = [pts[0]]
        for p in pts[1:]:
            if math.hypot(p[0] - kept[-1][0], p[1] - kept[-1][1]) > 1e-6:
                kept.append(p)
        if closed and len(kept) > 2 and math.hypot(kept[0][0] - kept[-1][0], kept[0][1] - kept[-1][1]) < 1e-6:
            kept.pop()
        cleaned.append((kept, closed))
    return cleaned


class Shape:
    """One drawn element with the style it inherits: geometry in the SVG's units, translation applied."""

    def __init__(self, element, style):
        self.tag = local(element.tag)
        self.id = element.get('id') or ''
        self.group = style['group']
        self.fill, self.stroke = style['fill'], style['stroke']
        self.stroke_width = float(style['stroke-width'])
        self.opacity = style['opacity']
        dx, dy = style['dx'], style['dy']
        get = lambda name, default=0.0: float(element.get(name, default))
        self.subpaths, self.rect, self.ellipse = [], None, None
        if self.tag == 'path':
            self.subpaths = [([(x + dx, y + dy) for x, y in pts], closed) for pts, closed in parse_path(element.get('d'))]
        elif self.tag == 'rect':
            x, y, w, h = get('x') + dx, get('y') + dy, get('width'), get('height')
            self.rect = (x, y, w, h)
            self.subpaths = [([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], True)]
        elif self.tag == 'polygon':
            values = [float(v) for v in re.split(r'[\s,]+', element.get('points').strip())]
            self.subpaths = [([(values[k] + dx, values[k + 1] + dy) for k in range(0, len(values), 2)], True)]
        elif self.tag == 'ellipse':
            self.ellipse = (get('cx') + dx, get('cy') + dy, get('rx'), get('ry'))
        else:
            raise ValueError(f'unsupported element <{self.tag}>')


def inherit(style, element):
    style = dict(style)
    for name in ('fill', 'stroke', 'stroke-width'):
        if element.get(name) is not None:
            style[name] = element.get(name)
    if element.get('opacity') is not None:
        style['opacity'] *= float(element.get('opacity'))
    if element.get('transform') is not None:
        dx, dy = translate(element.get('transform'))
        style['dx'] += dx
        style['dy'] += dy
    if local(element.tag) == 'g' and element.get('id') in GROUPS:
        style['group'] = element.get('id')
    return style


def read_svg(path):
    """The SVG's shapes, gradients, patterns and window."""
    root = ET.parse(path).getroot()
    gradients, patterns, window = {}, {}, None
    for element in root.iter():
        tag = local(element.tag)
        if tag == 'radialGradient':
            if element.get('gradientUnits', 'objectBoundingBox') != 'objectBoundingBox' or element.get('fx') or element.get('fy'):
                raise ValueError('radial gradients must use the bounding box, with no focus')
            fraction = lambda text: float(text[:-1]) / 100 if text.endswith('%') else float(text)
            stops = []
            for stop in element:
                offset = stop.get('offset')
                stops.append((fraction(offset), colour(stop.get('stop-color')), float(stop.get('stop-opacity', 1.0))))
            gradients[element.get('id')] = dict(cx=fraction(element.get('cx', '50%')), cy=fraction(element.get('cy', '50%')),
                                                r=fraction(element.get('r', '50%')), stops=sorted(stops))
        elif tag == 'pattern':
            if element.get('patternUnits') != 'userSpaceOnUse' or element.get('patternTransform'):
                raise ValueError('patterns must be in user space, untransformed')
            rects = [(float(r.get('x', 0)), float(r.get('y', 0)), float(r.get('width')), float(r.get('height')),
                      colour(r.get('fill')), float(r.get('opacity', 1.0))) for r in element if local(r.tag) == 'rect']
            patterns[element.get('id')] = dict(x=float(element.get('x', 0)), y=float(element.get('y', 0)),
                                               w=float(element.get('width')), h=float(element.get('height')), rects=rects)
        elif tag == 'clipPath' and element.get('id') == 'window':
            polygon = [c for c in element if local(c.tag) == 'polygon'][0]
            values = [float(v) for v in re.split(r'[\s,]+', polygon.get('points').strip())]
            window = [(values[k], values[k + 1]) for k in range(0, len(values), 2)]
    if not window:
        raise ValueError('the SVG has no clipPath "window"')

    shapes = []

    def walk(element, style):
        if not isinstance(element.tag, str):
            return
        tag = local(element.tag)
        if tag in ('defs', 'clipPath', 'radialGradient', 'linearGradient', 'pattern', 'title', 'desc'):
            return
        style = inherit(style, element)
        if tag in ('svg', 'g'):
            for child in element:
                walk(child, style)
        elif style['group']:
            shapes.append(Shape(element, style))
        else:
            raise ValueError(f'<{tag} id={element.get("id")}> sits outside the portrait\'s groups')

    walk(root, {'fill': '#000000', 'stroke': 'none', 'stroke-width': '1', 'opacity': 1.0, 'dx': 0.0, 'dy': 0.0,
                'group': None})
    return shapes, gradients, patterns, window


# --- Pictures and glows ---

class Layer:
    """LooterUI::FPaintLayer: fills (even-odd together) and strokes (round), one colour or a two-colour gradient."""

    def __init__(self, note, rgb, fills=(), strokes=(), width=1.0, opacity=1.0, gradient=None, kind='fills'):
        self.note, self.rgb, self.width, self.opacity, self.gradient = note, rgb, width, opacity, gradient
        self.fills, self.strokes = [list(f) for f in fills], [list(s) for s in strokes]
        self.kind = kind   # where it came from: a shape's fill or its stroke (pictures can take one or the other)

    def bounds(self):
        reach = self.width * 0.5 if self.strokes else 0.0
        pts = [(p, 0.0) for f in self.fills for p in f] + [(p, reach) for s in self.strokes for p in s]
        return (min(p[0] - r for p, r in pts), min(p[1] - r for p, r in pts),
                max(p[0] + r for p, r in pts), max(p[1] + r for p, r in pts))


def circle(cx, cy, r, sides=128):
    return [(cx + r * math.cos(2 * math.pi * k / sides), cy + r * math.sin(2 * math.pi * k / sides)) for k in range(sides)]


def radial_layers(shape, gradient):
    """A rect filled with a radial gradient, as one radial layer per stretch between stops, outermost first: each runs
    straight from the centre colour that meets both of its stops' colours, and covers the disc out to its outer stop
    (the outermost the whole rect, the gradient's last colour holding past it), so the stretches join seamlessly."""
    x, y, w, h = shape.rect
    if abs(w - h) > 1e-6:
        raise ValueError('a radial gradient on a non-square rect would be an ellipse')
    cx, cy, radius = x + gradient['cx'] * w, y + gradient['cy'] * h, gradient['r'] * w
    stops = gradient['stops']
    if any(a != 1.0 for _, _, a in stops):
        raise ValueError('the glass gradient must be opaque')
    layers = []
    for k in range(len(stops) - 2, -1, -1):
        (o0, c0, _), (o1, c1, _) = stops[k], stops[k + 1]
        if o1 - o0 < 1e-9:
            continue
        centre = tuple(c0[i] - (c1[i] - c0[i]) * o0 / (o1 - o0) for i in range(3))
        if any(v < -0.5 or v > 255.5 for v in centre):
            raise ValueError(f'stretch {k} of the glass gradient can\'t be drawn from its centre')
        centre = tuple(int(round(min(max(v, 0.0), 255.0))) for v in centre)
        outer = k == len(stops) - 2
        area = shape.subpaths[0][0] if outer else circle(cx, cy, radius * o1)
        layers.append(Layer(f'{shape.id or "glass"}: the gradient from {o0:g} to {o1:g}', centre, fills=[area],
                            opacity=shape.opacity,
                            gradient=dict(to=c1, start=(cx, cy), end=(cx + radius * o1, cy), radial=True)))
    return layers


def pattern_layer(shape, pattern):
    """A rect filled with a pattern of full-width rows (the scanlines), as one layer of thin rectangles."""
    x, y, w, h = shape.rect
    if len(pattern['rects']) != 1:
        raise ValueError('the scanline pattern must hold one rect')
    px, py, pw, ph, rgb, opacity = pattern['rects'][0]
    if px > 0 or px + pw < pattern['w']:
        raise ValueError('the scanline pattern\'s rect must span its tile (rows, not dashes)')
    rows = []
    first, last = math.floor((y - pattern['y']) / pattern['h']) - 1, math.ceil((y + h - pattern['y']) / pattern['h']) + 1
    for k in range(first, last + 1):
        top = max(pattern['y'] + k * pattern['h'] + py, y)
        bottom = min(pattern['y'] + k * pattern['h'] + py + ph, y + h)
        if bottom > top:
            rows.append([(x, top), (x + w, top), (x + w, bottom), (x, bottom)])
    return Layer(f'{shape.id or "glass"}: the scanlines', rgb, fills=rows, opacity=opacity * shape.opacity)


def shape_layers(shape):
    """A path or polygon's fill layer and stroke layer (each when it has one)."""
    layers = []
    if shape.fill not in ('none', None):
        if shape.fill.startswith('url('):
            raise ValueError(f'{shape.id}: gradient fills only on the glass and the eye glows')
        layers.append(Layer(shape.id or shape.group, colour(shape.fill), fills=[pts for pts, _ in shape.subpaths],
                            opacity=shape.opacity, kind='fills'))
    if shape.stroke not in ('none', None):
        lines = [pts + [pts[0]] if closed else pts for pts, closed in shape.subpaths]
        layers.append(Layer(f'{shape.id or shape.group}, {shape.stroke_width:g} wide', colour(shape.stroke), strokes=lines,
                            width=shape.stroke_width, opacity=shape.opacity, kind='strokes'))
    return layers


def merge_ink(layers):
    """Joins stroke layers of the same width within each run of opaque stroke-only layers of one colour: drawn in one
    opaque colour, their order can't show, and fewer layers draw faster."""
    merged, run = [], []

    def flush():
        by_width = {}
        for layer in run:
            if layer.width in by_width:
                by_width[layer.width].strokes += layer.strokes
                if layer.note not in by_width[layer.width].note.split('; '):
                    by_width[layer.width].note += '; ' + layer.note
            else:
                by_width[layer.width] = layer
                merged.append(layer)
        run.clear()

    for layer in layers:
        joins = (layer.strokes and not layer.fills and layer.opacity == 1.0 and layer.gradient is None
                 and (not run or run[0].rgb == layer.rgb))
        if not joins:
            flush()
        if layer.strokes and not layer.fills and layer.opacity == 1.0 and layer.gradient is None:
            run.append(layer)
        else:
            merged.append(layer)
    flush()
    return merged


class Picture:
    def __init__(self, name, layers):
        self.name, self.layers = name, layers
        bounds = [layer.bounds() for layer in layers]
        x0, y0 = min(b[0] for b in bounds) - PAD, min(b[1] for b in bounds) - PAD
        x1, y1 = max(b[2] for b in bounds) + PAD, max(b[3] for b in bounds) + PAD
        # Clamped to what can ever show through the window, then out to whole HUD pixels.
        x0, y0 = max(x0, -MARGIN), max(y0, -MARGIN)
        x1, y1 = min(x1, 200.0 + MARGIN), min(y1, 200.0 + MARGIN)
        self.origin = (math.floor(x0 / SNAP + 1e-9) * SNAP, math.floor(y0 / SNAP + 1e-9) * SNAP)
        self.view = (math.ceil(x1 / SNAP - 1e-9) * SNAP - self.origin[0], math.ceil(y1 / SNAP - 1e-9) * SNAP - self.origin[1])


def glows(shape, gradient):
    """An eye's glowing ellipse as GlowBrush glows: the body in the gradient's middle colour, and a white-hot core."""
    cx, cy, rx, ry = shape.ellipse
    stops = gradient['stops']
    body = dict(center=(cx, cy), size=(2 * rx, 2 * ry), rgb=stops[1][1], opacity=GLOW_BODY_PEAK * shape.opacity)
    core = dict(center=(cx, cy), size=(2 * rx * GLOW_CORE_SIZE, 2 * ry * GLOW_CORE_SIZE), rgb=stops[0][1],
                opacity=GLOW_CORE_PEAK * stops[0][2] * shape.opacity)
    return body, core


def build(shapes, gradients, patterns, window):
    xs, ys = [p[0] for p in window], [p[1] for p in window]
    side = max(xs) - min(xs)
    if (abs(side - 200.0) > 1e-6 or abs(max(ys) - min(ys) - 200.0) > 1e-6
            or sorted(window) != sorted([(100.0, 0.0), (200.0, 100.0), (100.0, 200.0), (0.0, 100.0)])):
        raise ValueError('the window must be the diamond inscribed in the 200-unit square')

    layers_by_group, glows_by_group = {}, {}
    for shape in shapes:
        out = layers_by_group.setdefault(shape.group, [])
        if shape.tag == 'ellipse':
            gradient = gradients[re.fullmatch(r'url\(#(.+)\)', shape.fill).group(1)]
            body, core = glows(shape, gradient)
            glows_by_group.setdefault(shape.group, []).append((body, core))
        elif shape.tag == 'rect' and shape.fill.startswith('url('):
            ref = re.fullmatch(r'url\(#(.+)\)', shape.fill).group(1)
            out += radial_layers(shape, gradients[ref]) if ref in gradients else [pattern_layer(shape, patterns[ref])]
        else:
            out += shape_layers(shape)

    pictures = []
    for name, groups, which in PICTURES:
        layers = [layer for group in groups for layer in layers_by_group.get(group, [])
                  if which == 'all' or layer.kind == which]
        if not layers:
            raise ValueError(f'picture {name} came out empty')
        pictures.append(Picture(name, merge_ink(layers)))

    glow_sets = []
    for name, group in GLOWS:
        pairs = glows_by_group.get(group, [])
        # Every body first, then every core, so one eye's body never covers the other's core where they overlap.
        glow_sets.append((name, [b for b, _ in pairs] + [c for _, c in pairs]))

    # The blink squashes the open eyes toward the middle of their box (glows and slits), as the mockup's does.
    eyes = [s for s in shapes if s.group == 'eyes-open']
    top = min([s.ellipse[1] - s.ellipse[3] for s in eyes if s.ellipse]
              + [p[1] for s in eyes if s.fill not in ('none', None) and not s.ellipse for pts, _ in s.subpaths for p in pts])
    bottom = max([s.ellipse[1] + s.ellipse[3] for s in eyes if s.ellipse]
                 + [p[1] for s in eyes if s.fill not in ('none', None) and not s.ellipse for pts, _ in s.subpaths for p in pts])
    return pictures, glow_sets, (top + bottom) * 0.5


# --- The C++ data ---

def cpp_points(points, x0, y0):
    return '{ ' + ', '.join(f'{{ {x - x0:.2f}, {y - y0:.2f} }}' for x, y in points) + ' }'


def cpp_polys(polys, x0, y0):
    return '{\n' + ',\n'.join('\t\t\t\t' + cpp_points(p, x0, y0) for p in polys) + '\n\t\t\t}'


def cpp_hex(rgb):
    return 'LooterUI::Hex(0x{:02X}, 0x{:02X}, 0x{:02X})'.format(*rgb)


def cpp_layer(layer, x0, y0):
    lines = [f'\t\t{{\n\t\t\t// {layer.note}',
             '\t\t\tLooterUI::FPaintLayer& Layer = Picture.Icon.Layers.AddDefaulted_GetRef();',
             f'\t\t\tLayer.Color = {cpp_hex(layer.rgb)};']
    if layer.opacity != 1.0:
        lines.append(f'\t\t\tLayer.Opacity = {layer.opacity:.3f}f;')
    if layer.gradient:
        g = layer.gradient
        lines += [f'\t\t\tLayer.GradientTo = {cpp_hex(g["to"])};',
                  f'\t\t\tLayer.GradientStart = FVector2D({g["start"][0] - x0:.2f}, {g["start"][1] - y0:.2f});',
                  f'\t\t\tLayer.GradientEnd = FVector2D({g["end"][0] - x0:.2f}, {g["end"][1] - y0:.2f});']
        if g['radial']:
            lines.append('\t\t\tLayer.bRadialGradient = true;')
    if layer.fills:
        lines.append(f'\t\t\tLayer.Fills = {cpp_polys(layer.fills, x0, y0)};')
    if layer.strokes:
        lines.append(f'\t\t\tLayer.StrokeWidth = {layer.width:.2f}f;')
        lines.append(f'\t\t\tLayer.Strokes = {cpp_polys(layer.strokes, x0, y0)};')
    return '\n'.join(lines) + '\n\t\t}\n'


def cpp_picture(picture):
    x0, y0 = picture.origin
    return (f'\tinline FPicture {picture.name}()\n\t{{\n\t\tFPicture Picture;\n'
            f'\t\tPicture.Origin = FVector2D({x0:.4f}, {y0:.4f});\n'
            f'\t\tPicture.Icon.ViewBox = FVector2D({picture.view[0]:.4f}, {picture.view[1]:.4f});\n'
            + ''.join(cpp_layer(layer, x0, y0) for layer in picture.layers)
            + '\t\treturn Picture;\n\t}\n')


def cpp_glows(name, glow_list):
    rows = ',\n'.join(f'\t\t\t{{ FVector2D({g["center"][0]:.2f}, {g["center"][1]:.2f}), '
                      f'FVector2D({g["size"][0]:.2f}, {g["size"][1]:.2f}), {cpp_hex(g["rgb"])}, {g["opacity"]:.3f}f }}'
                      for g in glow_list)
    return f'\tinline TArray<FGlow> {name}()\n\t{{\n\t\treturn {{\n{rows}\n\t\t}};\n\t}}\n'


def write_cpp(pictures, glow_sets, blink_y):
    header = (
        '// Generated by Art/Icons/HudPortrait.py from Art/Icons/HudPortrait.svg: change the art there and run it again;\n'
        '// don\'t edit this file. The HUD portrait as painted pictures (LooterUI::FPaintedIcon) and eye glows\n'
        '// (LooterUI::GlowBrush) for UHudPortraitWidget; how the SVG becomes these is in that script\'s docstring.\n'
        '// Units are the SVG\'s (x right, y down); a picture\'s points and gradients count from its Origin.\n'
        '#pragma once\n\n'
        '// Include after LooterUI::FPaintedIcon is declared (UI/Style/LooterUIStyle.h).\n'
        'namespace HudPortraitData\n{\n'
        '\t/** The SVG\'s square: the diamond window is inscribed in it, its tips at the middles of the sides. */\n'
        '\tconstexpr double WindowUnits = 200.0;\n\n'
        '\t/** The open eyes squash toward this height for the blink: the middle of their glows and slits. */\n'
        f'\tconstexpr double BlinkCenterY = {blink_y:.2f};\n\n'
        '\t/** One picture: its painting, and where its view box\'s top-left corner sits in the SVG. */\n'
        '\tstruct FPicture\n\t{\n\t\tLooterUI::FPaintedIcon Icon;\n\t\tFVector2D Origin = FVector2D::ZeroVector;\n\t};\n\n'
        '\t/** A soft round glow (GlowBrush): its middle and full size in the SVG, its color and its opacity in the middle. */\n'
        '\tstruct FGlow\n\t{\n\t\tFVector2D Center;\n\t\tFVector2D Size;\n\t\tFLinearColor Color;\n\t\tfloat Opacity;\n\t};\n\n')
    body = '\n'.join([cpp_picture(p) for p in pictures] + [cpp_glows(n, g) for n, g in glow_sets])
    with open(DATA_OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write(header + body + '}\n')
    return DATA_OUT


# --- The previews: the window drawn from the data (the game's rules, supersampled), to check against the SVG ---

def render(pictures, glow_sets, state, scale, ss):
    """The window in one state at scale px a unit, as an RGB image over the HUD's dark backdrop. Layers composite
    over each other in display (sRGB) space, as the game's rasterizer and Slate do."""
    import numpy as np
    n = int(round(200 * scale))
    grid = (np.arange(n * ss) + 0.5) / (ss * scale)
    X, Y = np.meshgrid(grid, grid)
    rgb = np.zeros(X.shape + (3,))
    alpha = np.zeros(X.shape)
    by_name = {p.name: p for p in pictures}
    glows_by_name = dict(glow_sets)

    def over(mask_alpha, colour_rgb):
        nonlocal rgb, alpha
        a = mask_alpha[..., None]
        rgb = colour_rgb * a + rgb * (1 - a)
        alpha = mask_alpha + alpha * (1 - mask_alpha)

    def window_of(points):
        xs, ys = [p[0] for p in points], [p[1] for p in points]
        return (max(int((min(xs)) * scale * ss) - 2, 0), min(int(max(xs) * scale * ss) + 3, n * ss),
                max(int((min(ys)) * scale * ss) - 2, 0), min(int(max(ys) * scale * ss) + 3, n * ss))

    def paint(picture, dy=0.0, squash=None, opacity=1.0):
        # The pictures' points are kept in the SVG's units here; only the C++ data counts them from each Origin.
        def move(p):
            x, y = p[0], p[1] + dy
            if squash:
                y = squash[0] + (y - squash[0]) * squash[1]
            return x, y
        for layer in picture.layers:
            cover = np.zeros(X.shape, bool)
            for poly in layer.fills:
                pts = [move(p) for p in poly]
                x0, x1, y0, y1 = window_of(pts)
                if x1 <= x0 or y1 <= y0:
                    continue
                sx, sy = X[y0:y1, x0:x1], Y[y0:y1, x0:x1]
                inside = np.zeros(sx.shape, bool)
                for (ax, ay), (bx, by) in zip(pts, pts[1:] + pts[:1]):
                    if ay == by:
                        continue
                    inside ^= ((ay > sy) != (by > sy)) & (sx < ax + (sy - ay) * (bx - ax) / (by - ay))
                cover[y0:y1, x0:x1] ^= inside
            half = layer.width * 0.5
            for line in layer.strokes:
                pts = [move(p) for p in line]
                for a, b in zip(pts, pts[1:] if len(pts) > 1 else pts):
                    x0, x1, y0, y1 = window_of([(a[0] - half, a[1] - half), (b[0] + half, b[1] + half),
                                                (a[0] + half, a[1] + half), (b[0] - half, b[1] - half)])
                    if x1 <= x0 or y1 <= y0:
                        continue
                    sx, sy = X[y0:y1, x0:x1], Y[y0:y1, x0:x1]
                    dx, dy2 = b[0] - a[0], b[1] - a[1]
                    t = np.clip(((sx - a[0]) * dx + (sy - a[1]) * dy2) / max(dx * dx + dy2 * dy2, 1e-12), 0, 1)
                    cover[y0:y1, x0:x1] |= np.hypot(sx - a[0] - t * dx, sy - a[1] - t * dy2) <= half
            colour_rgb = np.broadcast_to(np.array(layer.rgb, float) / 255.0, X.shape + (3,))
            if layer.gradient:
                g = layer.gradient
                gx, gy = g['start'][0], g['start'][1] + dy
                ex, ey = g['end'][0], g['end'][1] + dy
                if g['radial']:
                    t = np.clip(np.hypot(X - gx, Y - gy) / math.hypot(ex - gx, ey - gy), 0, 1)
                else:
                    t = np.clip(((X - gx) * (ex - gx) + (Y - gy) * (ey - gy)) / ((ex - gx) ** 2 + (ey - gy) ** 2), 0, 1)
                c0, c1 = np.array(layer.rgb, float) / 255.0, np.array(g['to'], float) / 255.0
                colour_rgb = c0 + (c1 - c0) * t[..., None]
            over(cover * layer.opacity * opacity, colour_rgb)

    def glow(group, dy=0.0, squash=None, opacity=1.0):
        for g in glows_by_name[group]:
            cx, cy = g['center'][0], g['center'][1] + dy
            w, h = g['size']
            if squash:
                cy = squash[0] + (cy - squash[0]) * squash[1]
                h *= squash[1]
            r2 = ((X - cx) / (w / 2)) ** 2 + ((Y - cy) / (max(h, 1e-6) / 2)) ** 2
            over(np.where(r2 < 1, (1 - r2) ** 2, 0.0) * g['opacity'] * opacity,
                 np.broadcast_to(np.array(g['rgb'], float) / 255.0, X.shape + (3,)))

    paint(by_name['Glass'])
    paint(by_name['Bust'])
    if state in ('Calm', 'Blink', 'Flare'):
        squash = (BLINK_Y, 0.08) if state == 'Blink' else None
        glow('EyesOpenGlows', 0.0, squash)
        paint(by_name['EyesOpen'], 0.0, squash)
        paint(by_name['BrowsCalm'])
    if state == 'Hurt':
        glow('EyesHurtGlows')
        paint(by_name['EyesHurt'])
    if state == 'Flare':
        glow('EyesFlareGlows')
        paint(by_name['EyesFlare'])

    # The window, and the HUD's dark backdrop round it.
    inside = (np.abs(X - 100) + np.abs(Y - 100)) <= 100
    backdrop = np.array([0x0a, 0x0f, 0x14], float) / 255.0
    out = np.where(inside[..., None], rgb + backdrop * (1 - alpha[..., None]), backdrop)
    out = out.reshape(n, ss, n, ss, 3).mean(axis=(1, 3))
    return np.clip(out * 255 + 0.5, 0, 255).astype(np.uint8)


def write_png(path, rgb):
    h, w, _ = rgb.shape
    raw = b''.join(b'\x00' + rgb[y].tobytes() for y in range(h))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def write_previews(pictures, glow_sets):
    try:
        import numpy  # noqa: F401
    except ImportError:
        print('no numpy: skipped the previews')
        return []
    os.makedirs(PREVIEW_OUT, exist_ok=True)
    written = []
    for state in ('Calm', 'Blink', 'Hurt', 'Flare'):
        for suffix, scale, ss in (('', 2.56, 2), ('_Hud', HUD_SCALE, 6)):
            path = os.path.join(PREVIEW_OUT, f'HudPortrait_{state}{suffix}.png')
            write_png(path, render(pictures, glow_sets, state, scale, ss))
            written.append(path)
    return written


BLINK_Y = 90.0


def main():
    global BLINK_Y
    shapes, gradients, patterns, window = read_svg(SVG_IN)
    pictures, glow_sets, BLINK_Y = build(shapes, gradients, patterns, window)
    print('wrote', write_cpp(pictures, glow_sets, BLINK_Y))
    for p in pictures:
        px = (math.ceil(p.view[0] * HUD_SCALE * 2 - 1e-3), math.ceil(p.view[1] * HUD_SCALE * 2 - 1e-3))
        print(f'{p.name}: origin {p.origin[0]:.2f}, {p.origin[1]:.2f}, view {p.view[0]:.2f} x {p.view[1]:.2f} '
              f'({px[0]} x {px[1]} texture px), {len(p.layers)} layers, '
              f'{sum(len(q) for l in p.layers for q in l.fills + l.strokes)} points')
    for name, g in glow_sets:
        print(f'{name}: {len(g)} glows')
    print(f'blink centre y {BLINK_Y:.2f}')
    for path in write_previews(pictures, glow_sets):
        print('wrote', path)


if __name__ == '__main__':
    main()
