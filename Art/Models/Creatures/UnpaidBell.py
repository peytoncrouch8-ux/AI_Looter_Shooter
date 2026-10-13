"""The Bell-Ringer's props (ABellRingerCreature): the chapel's old ringer, one of the Unpaid, still tolling for service.
Both are ghosts of the things he carried, on M_Ghost like Abel's lantern (Abel.py), and both hang from a fist: each
model's pivot is the point the fist closes on, and each hangs straight down from it. The code hangs them plumb from the
posed hand every frame and swings them on pendulums (BellRingerCreaturePose.cpp), so nothing here is fitted to the rig.

Models:
  SM_UnpaidBell      a bronze hand bell with a turned wooden handle, oversized to read at ten meters (18 cm across the
                     mouth, 32 cm from the knob to the lip): the handle's grip at the pivot, the bell below it, its iron
                     clapper hanging inside, two cast bands round it. The middle of its body is BODY_DROP under the pivot
                     (the code's BellBodyDrop: its hit zone's middle). About 2.7k triangles.
  SM_UnpaidBellRope  the chapel bell's pull rope, broken off: a loop over the fist, then the hemp hanging 60 cm with the
                     striped woollen sally a ringer grips partway down and the end frayed into strands that fade. About
                     1.3k triangles.
Both: no collision (the bell's hit zone is a ball the code puts on it), no Nanite, LODs at 50% and 25%.

Vertex colors ('Col', what M_Ghost reads, stored as floats: these props export linear): R the tint zone (0, 1/3, 2/3, 1:
Zone1Color..Zone4Color), G cavity (inside the bell's mouth), B the ember edge (none), A the fade (1 solid; the rope's
frayed end fades out). UV 0 is the Polymer set's world-scale box projection.

Materials (M_Ghost's slots; the importer resets each instance and sets these):
  GhostUnpaidBell  ghost bronze, the clapper's iron, the handle's walnut, the bands' brass
  GhostUnpaidRope  hemp, and the sally's three wools (madder red, bone, a dull blue)

    blender -b --factory-startup --python Art/Models/Creatures/UnpaidBell.py -- --preview
Previews go to Saved/ArtPreviews/Creatures/.
"""
import math

import bpy
import numpy as np

import looter_textures as lt

# The Unpaid's approved ghost look (Unpaid.py's Ghost_A, Abel.py's GHOST_LOOK).
RIM_COLOR = '#DCECEE'
GHOST_LOOK = dict(RimColor=RIM_COLOR, RimPower=4.0, RimStrength=2.2, GlowStrength=0.12, EmberStrength=2.0,
                  NoiseScale=1.4, FadeSoftness=0.07, UVScale=3.0)
BELL_COLORS = ('#C9B489', '#6E7377', '#9C8670', '#D9CB9F')     # ghost bronze, iron, walnut, brass
ROPE_COLORS = ('#B9AE94', '#A2584C', '#D9D1BC', '#5F6E85')     # hemp, the sally's red, bone and blue wools

BODY_DROP = 0.15            # the bell's body middle under the pivot (m): BellRingerCreature.cpp's BellBodyDrop
SEGS = 22                   # round the bell and the handle: smooth enough held at arm's length, ~2.7k triangles
ROPE_SEGS = 10


def log(message):
    print(f'LOOTER: {message}')


# --- Building blocks (numpy: vertices in meters, faces as index lists) ---------------------------------------------

def lathe(profile, segs, loop=False):
    """A surface of revolution about Z from a profile of (z, r) points: a point with r = 0 is a pole. loop joins the
    last point back to the first (a torus's tube). Returns (V, faces), faces a list of index lists."""
    V = []
    rings = []
    for z, r in profile:
        if r < 1e-6:
            rings.append([len(V)])
            V.append((0.0, 0.0, z))
            continue
        ring = []
        for k in range(segs):
            a = 2.0 * math.pi * k / segs
            ring.append(len(V))
            V.append((r * math.cos(a), r * math.sin(a), z))
        rings.append(ring)
    faces = []
    pairs = list(zip(rings[:-1], rings[1:]))
    if loop:
        pairs.append((rings[-1], rings[0]))
    for a, b in pairs:
        if len(a) == 1 and len(b) == 1:
            continue
        if len(a) == 1:
            faces += [[a[0], b[(k + 1) % segs], b[k]] for k in range(segs)]
        elif len(b) == 1:
            faces += [[a[k], a[(k + 1) % segs], b[0]] for k in range(segs)]
        else:
            faces += [[a[k], a[(k + 1) % segs], b[(k + 1) % segs], b[k]] for k in range(segs)]
    return np.array(V, float), faces


def oriented(piece, inward=False):
    """The piece's faces turned outward (by the sign of its volume), or inward."""
    V, faces = piece
    vol = 0.0
    for F in faces:
        for k in range(1, len(F) - 1):
            vol += float(np.dot(V[F[0]], np.cross(V[F[k]], V[F[k + 1]])))
    flip = (vol < 0.0) != inward
    return V, [F[::-1] for F in faces] if flip else faces


def moved(piece, offset):
    V, faces = piece
    return V + np.asarray(offset, float), faces


def frame_along(path):
    """Unit tangents and two normals along a polyline (rotation-minimizing, from an up guess)."""
    P = np.asarray(path, float)
    T = np.gradient(P, axis=0)
    T /= np.linalg.norm(T, axis=1, keepdims=True)
    guess = np.array([1.0, 0.0, 0.0]) if abs(T[0][0]) < 0.9 else np.array([0.0, 1.0, 0.0])
    N = [np.cross(T[0], guess)]
    N[0] /= np.linalg.norm(N[0])
    for i in range(1, len(P)):
        n = N[-1] - T[i] * np.dot(N[-1], T[i])
        N.append(n / max(np.linalg.norm(n), 1e-9))
    N = np.array(N)
    B = np.cross(T, N)
    return T, N, B


def tube(path, radii, segs, caps=True):
    """A tube along a polyline with a radius per point, capped with poles at both ends."""
    P = np.asarray(path, float)
    T, N, B = frame_along(P)
    V = []
    rings = []
    for i, p in enumerate(P):
        ring = []
        for k in range(segs):
            a = 2.0 * math.pi * k / segs
            ring.append(len(V))
            V.append(p + radii[i] * (math.cos(a) * N[i] + math.sin(a) * B[i]))
        rings.append(ring)
    faces = []
    for a, b in zip(rings[:-1], rings[1:]):
        faces += [[a[k], a[(k + 1) % segs], b[(k + 1) % segs], b[k]] for k in range(segs)]
    if caps:
        top = len(V)
        V.append(P[0] - T[0] * radii[0] * 0.4)
        bottom = len(V)
        V.append(P[-1] + T[-1] * radii[-1] * 0.4)
        faces += [[top, rings[0][(k + 1) % segs], rings[0][k]] for k in range(segs)]
        faces += [[rings[-1][k], rings[-1][(k + 1) % segs], bottom] for k in range(segs)]
    return np.array(V, float), faces


class Part:
    """A piece of a model with its vertex colors: zone (0-3), cavity and fade per vertex."""

    def __init__(self, piece, zone, cavity=0.0, fade=None):
        self.V, self.faces = piece
        n = len(self.V)
        self.zone = np.full(n, float(zone))
        self.cavity = np.full(n, float(cavity)) if np.isscalar(cavity) else np.asarray(cavity, float)
        self.fade = np.ones(n) if fade is None else np.asarray(fade, float)


def build_object(name, parts, material, location):
    """One static model from its parts: merged, smooth with sharp edges past 60 degrees, the Polymer box projection, the
    ghost's colors as floats, and the props the importer reads (no Nanite, LODs, no collision)."""
    Vs, faces, cols = [], [], []
    base = 0
    for p in parts:
        Vs.append(p.V)
        faces += [[i + base for i in F] for F in p.faces]
        cols.append(np.column_stack([p.zone / 3.0, np.clip(p.cavity, 0, 1), np.zeros(len(p.V)), np.clip(p.fade, 0, 1)]))
        base += len(p.V)
    V = np.concatenate(Vs)
    rgba = np.concatenate(cols).astype(np.float32)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(V.tolist(), [], faces)
    mesh.validate()
    mesh.materials.append(material)
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(60.0))
    col = mesh.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    col.data.foreach_set('color', rgba[loops].ravel())
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = 0
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    lt.box_uv(obj, 'Polymer')
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    obj['LODScreens'] = '0.25,0.1'
    obj['Collision'] = 'None'
    obj.location = location
    log(f'SM_{name}: {sum(len(p.vertices) - 2 for p in mesh.polygons)} triangles')
    return obj


def ghost_slot(name, colors, **scalars):
    """An M_Ghost slot: its properties are the material instance's parameters (Abel.py's ghost_slot)."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(int(colors[0][1:], 16))
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Ghost'
    mat['TextureSet'] = 'Polymer'
    for k, color in enumerate(colors):
        mat[f'Zone{k + 1}Color'] = color
    for key, value in scalars.items():
        mat[key] = value
    return mat


# --- The bell --------------------------------------------------------------------------------------------------------

def build_bell():
    """SM_UnpaidBell: pivot at the handle's grip, the bell hanging under it, mouth down."""
    # The handle: a turned knob, a neck, the grip (round the pivot, where the fist closes), a collar, the brass ferrule.
    handle = [(0.085, 0.0), (0.083, 0.010), (0.078, 0.017), (0.071, 0.019), (0.064, 0.016), (0.057, 0.011),
              (0.048, 0.012), (0.030, 0.0135), (0.0, 0.0145), (-0.030, 0.0135), (-0.040, 0.0125), (-0.046, 0.016),
              (-0.052, 0.0165), (-0.058, 0.0), ]
    ferrule = [(-0.046, 0.0), (-0.046, 0.0175), (-0.050, 0.019), (-0.058, 0.019), (-0.062, 0.0175), (-0.062, 0.0)]
    # The bell: a cast shell, its outer face down from the crown to the lip, its inner face back up. The sound bow (the
    # thick ring above the lip, where the clapper strikes) flares out; the waist is slim.
    outer = [(-0.060, 0.0), (-0.0605, 0.016), (-0.062, 0.030), (-0.066, 0.040), (-0.074, 0.046), (-0.088, 0.048),
             (-0.105, 0.049), (-0.125, 0.051), (-0.145, 0.055), (-0.165, 0.061), (-0.183, 0.069), (-0.198, 0.078),
             (-0.210, 0.085), (-0.219, 0.090), (-0.226, 0.092), (-0.2295, 0.0905), (-0.231, 0.087)]
    inner = [(-0.230, 0.083), (-0.222, 0.081), (-0.208, 0.075), (-0.190, 0.066), (-0.168, 0.056), (-0.145, 0.049),
             (-0.120, 0.044), (-0.097, 0.041), (-0.080, 0.037), (-0.072, 0.028), (-0.069, 0.014), (-0.068, 0.0)]
    shell = oriented(lathe(outer + inner, SEGS))
    # The inner face is dark: the mouth's hollow, deeper toward the crown.
    Vs = shell[0]
    radius = np.linalg.norm(Vs[:, :2], axis=1)
    outer_r = np.interp(-Vs[:, 2], [-p[0] for p in outer], [p[1] for p in outer])
    inside = radius < outer_r - 0.002
    cavity = np.where(inside, 0.55 + 0.4 * np.clip((Vs[:, 2] + 0.231) / 0.16, 0.0, 1.0), 0.0)
    parts = [Part(oriented(lathe(handle, SEGS)), 2),
             Part(oriented(lathe(ferrule, SEGS)), 3),
             Part(shell, 0, cavity=cavity)]
    # Two cast bands round it: one under the shoulder, one over the sound bow.
    for z, r in ((-0.072, 0.0475), (-0.196, 0.0775)):
        band = [(z + 0.0035 * math.sin(a), r + 0.0035 * math.cos(a)) for a in np.linspace(0.0, 2.0 * math.pi, 8, endpoint=False)]
        parts.append(Part(oriented(lathe(band, SEGS, loop=True)), 3))
    # The clapper: an iron rod from the crown, its ball hanging just inside the mouth (seen from below and from the side).
    rod = [(-0.066, 0.0), (-0.066, 0.0045), (-0.196, 0.0045), (-0.196, 0.0)]
    parts.append(Part(oriented(lathe(rod, 8)), 1, cavity=0.2))
    ball = [(-0.198 + 0.017 * math.cos(a), 0.017 * math.sin(a)) for a in np.linspace(0.0, math.pi, 9)]
    ball[0] = (ball[0][0], 0.0)
    ball[-1] = (ball[-1][0], 0.0)
    parts.append(Part(oriented(lathe(ball, 16)), 1, cavity=0.2))
    material = ghost_slot('GhostUnpaidBell', BELL_COLORS, **GHOST_LOOK)
    return build_object('UnpaidBell', parts, material, (0.0, 0.0, 0.0))


# --- The rope --------------------------------------------------------------------------------------------------------

def build_rope():
    """SM_UnpaidBellRope: pivot at the fist's grip; a loop over the fist, the rope hanging 60 cm, the sally partway."""
    rng = np.random.default_rng(1871)
    parts = []
    # The loop over the fist: the rope comes up out of the fist, turns over and goes back down through it.
    loop = [(0.0, -0.012 + 0.012 * math.cos(a), 0.035 + 0.03 * math.sin(a)) for a in np.linspace(-0.5 * math.pi, 1.5 * math.pi, 14)]
    parts.append(Part(tube(loop, np.full(len(loop), 0.0085), ROPE_SEGS), 0))
    # The hanging rope: a gentle sway in it, so it isn't ruler-straight; the sally swells round it partway down.
    n = 34
    z = np.linspace(0.02, -0.56, n)
    s = (0.02 - z) / 0.58
    path = np.column_stack([0.012 * np.sin(s * 5.0), -0.01 * np.sin(s * 3.1 + 0.4), z])
    sally_top, sally_bottom = -0.19, -0.36
    in_sally = (z <= sally_top) & (z >= sally_bottom)
    taper = np.clip(np.minimum(sally_top - z, z - sally_bottom) / 0.025, 0.0, 1.0)
    radii = np.where(in_sally, 0.0085 + 0.016 * np.sqrt(taper), 0.0085)
    rope = tube(path, radii, ROPE_SEGS)
    # The sally is wool in stripes (red, bone, blue, bone...); the rope is hemp. Each vertex takes its ring's height.
    Vr = rope[0]
    zones = np.zeros(len(Vr))
    stripe = ((sally_top - Vr[:, 2]) / 0.028).astype(int)
    wool = (Vr[:, 2] <= sally_top + 0.002) & (Vr[:, 2] >= sally_bottom - 0.002)
    zones[wool] = np.array([1, 2, 3, 2])[stripe[wool] % 4]
    rope_part = Part(rope, 0)
    rope_part.zone = zones
    parts.append(rope_part)
    # The broken end: strands splaying out of the last few centimeters, fading to nothing.
    for k in range(7):
        a = 2.0 * math.pi * k / 7 + rng.uniform(-0.3, 0.3)
        out = rng.uniform(0.012, 0.03)
        length = rng.uniform(0.05, 0.09)
        start = path[-2]
        strand = [start + np.array([math.cos(a) * out * u, math.sin(a) * out * u, -length * u]) for u in np.linspace(0.0, 1.0, 5)]
        piece = tube(strand, np.linspace(0.0032, 0.0012, 5), 5)
        fade = np.clip((piece[0][:, 2] - (start[2] - length)) / length, 0.0, 1.0) * 0.8 + 0.1
        parts.append(Part(piece, 0, fade=fade))
    material = ghost_slot('GhostUnpaidRope', ROPE_COLORS, **GHOST_LOOK)
    return build_object('UnpaidBellRope', parts, material, (0.35, 0.0, 0.0))


bell = build_bell()
rope = build_rope()
log(f'SM_UnpaidBell: {bell.dimensions.z * 100.0:.1f} cm tall, {bell.dimensions.x * 100.0:.1f} cm across; body middle '
    f'{BODY_DROP * 100.0:.0f} cm under the grip')

if lt.want_preview():
    path = lt.preview([bell, rope], lt.preview_path('Creatures', 'UnpaidBell'), resolution=(1280, 900), view=(-0.6, -1.6, 0.35))
    log(f'preview: {path}')
