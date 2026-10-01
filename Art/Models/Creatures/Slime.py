"""The meadow slime (2026-10-01): the user's pick from Art/Backlog/Creatures/SlimeConcepts.py, as a small rig the game
animates in code. It's a clear green gel dome with a dark core, three pebbles and a few grass blades caught inside.

Built for speed and size: about 1.7k triangles with game LODs, two materials and no new textures.
  SlimeGel   the gel shell, on the Gel master (translucent, lit, a fresnel rim; Unreal's M_Gel). Tint and Opacity
             only, no texture.
  FoliagePalette  everything inside (core, pebbles, grass), mapped onto swatches of the foliage palette the meadow
             already loads, so the slime adds no texture memory. Vertex color R (wind) is 0, so nothing inside sways.

Bones, which the game finds by name:
  body   at the origin (the ground under the slime's middle), pointing up. Everything but the core is weighted to it,
         so scaling it squashes and stretches the slime against the ground: squash before a hop, stretch on the way up,
         a round apex, a splat on landing, and a wobble while idle (the user's reference: a bouncy pixel-art slime).
  core   the nucleus, a child of body at the core's middle. It's the critical spot. It sits inside the gel, so a shot
         first hits the body's hull; the game counts it a crit when the shot's line also passes through the core's
         hull (see the hand-over notes).
Each bone has one convex hit hull (PA_Slime). The origin is on the ground; the front faces Blender -Y (Unreal +X).
A slime looks the same from every side.

    blender -b --factory-startup --python Art/Models/Creatures/Slime.py -- --preview
"""
import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_creatures as lc
import looter_model as lm
import looter_textures as lt
from looter_plants import swatch

RADIUS = 60.0      # cm: the gel's half-width at rest
HEIGHT = 62.0      # cm: its height at rest
SPREAD = 0.3       # how much wider it gets toward its foot
PEAK = 0.1         # a soft pulled-up top
CORE = dict(center=(0.0, 0.0, 30.0), radius=11.0)
GEL_TINT, GEL_OPACITY = 0xb8f088, 0.35


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


# --- Materials ---

def gel_material():
    """The gel: a Gel master material with a tint and an opacity (no texture). Its nodes only preview it in Blender."""
    mat = bpy.data.materials.get('SlimeGel') or bpy.data.materials.new('SlimeGel')
    mat.use_nodes = True
    rgba = lm.hex_color(GEL_TINT)
    bsdf = next(node for node in mat.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = rgba
    bsdf.inputs['Alpha'].default_value = GEL_OPACITY
    bsdf.inputs['Roughness'].default_value = 0.08
    mat.diffuse_color = rgba[:3] + [GEL_OPACITY]
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Gel'
    mat['Tint'] = f'#{GEL_TINT:06x}'
    mat['Opacity'] = GEL_OPACITY
    return mat


GEL = gel_material()
INNER = lt.material('FoliagePalette')

# The palette's swatches as atlas regions, so looter_creatures' parts can map onto them (patch=True: a solid color).
rows = len(lt.PALETTE)


def swatch_region(name):
    i = lt.PALETTE.index(name)
    return (0.3, 0.7, (i + 0.25) / rows, (i + 0.75) / rows)


ATLAS = {'gel': (0.0, 1.0, 0.0, 1.0), 'core': swatch_region('CloverDark'), 'pebble': swatch_region('CattailBrown'),
         'seed': swatch_region('DryTan')}


# --- The parts (creature space: cm, x forward, y left, z up) ---

def gel_body():
    """The gel dome: a sphere squashed to HEIGHT, wider toward its flat foot, a soft peak, and a slow wobble baked
    into its skin so it never looks like a perfect primitive."""
    m = lc.Mesh(ATLAS)
    bm = m.bm
    segs, rings = 32, 18
    off = Vector((3.1, 1.7, 4.3))

    def point(theta, phi):
        z = -math.cos(theta)
        r = math.sin(theta)
        x, y = r * math.cos(phi), r * math.sin(phi)
        p = Vector((x, y, z))
        wob = 1.0 + 0.035 * noise.noise(p * 1.6 + off) + 0.015 * noise.noise(p * 4.0 + off)
        s = 1.0 + SPREAD * smoothstep(0.35, -1.0, z)
        pz = (z * 0.5 + 0.5) * HEIGHT * wob + PEAK * HEIGHT * smoothstep(0.6, 1.0, z) ** 2
        if z < -0.55:      # the foot spreads flat on the ground
            pz *= smoothstep(-1.0, -0.55, z) ** 1.5
        return Vector((x * RADIUS * s * wob, y * RADIUS * s * wob, max(pz, 0.0)))

    bottom = bm.verts.new(point(0.0, 0.0))
    top = bm.verts.new(point(math.pi, 0.0))
    ring = [[bm.verts.new(point(math.pi * i / rings, 2.0 * math.pi * j / segs)) for j in range(segs)] for i in range(1, rings)]
    uv = lambda u, v: m.at('gel', u, v)
    for j in range(segs):
        j1 = (j + 1) % segs
        ua, ub = j / segs, (j + 1) / segs
        m.face((bottom, ring[0][j1], ring[0][j]), [uv(ua, 0), uv(ub, 1 / rings), uv(ua, 1 / rings)])
        for i in range(rings - 2):
            va, vb = (i + 1) / rings, (i + 2) / rings
            m.face((ring[i][j], ring[i][j1], ring[i + 1][j1], ring[i + 1][j]), [uv(ua, va), uv(ub, va), uv(ub, vb), uv(ua, vb)])
        m.face((ring[-1][j], ring[-1][j1], top), [uv(ua, 1 - 1 / rings), uv(ub, 1 - 1 / rings), uv(ua, 1)])
    return m


def core_part():
    m = lc.Mesh(ATLAS)
    r = CORE['radius']
    lc.ellipsoid(m, CORE['center'], (r, r, r * 0.9), 'core', segs=14, rings=9, patch=True)
    return m


def inner_bits():
    """Three pebbles and a few seeds caught in the gel, and grass blades tumbling in it."""
    m = lc.Mesh(ATLAS)
    rng = random.Random(5)
    for (x, y, z), r in (((12.0, -10.0, 18.0), 7.0), ((-18.0, 8.0, 12.0), 5.0), ((5.0, 20.0, 28.0), 4.0)):
        before = set(m.bm.verts)
        lc.ellipsoid(m, (x, y, z), (r * rng.uniform(1.0, 1.4), r, r * rng.uniform(0.6, 0.9)), 'pebble', segs=8, rings=5,
                     patch=True)
        jitter = Vector((rng.uniform(0, 9), rng.uniform(0, 9), 0))
        for v in set(m.bm.verts) - before:
            v.co += (v.co - Vector((x, y, z))).normalized() * r * 0.18 * noise.noise(v.co * 0.18 + jitter)
    for _ in range(7):
        c = (rng.uniform(-28, 28), rng.uniform(-28, 28), rng.uniform(12, 42))
        lc.ellipsoid(m, c, (1.2, 0.8, 0.8), 'seed', fwd=(rng.uniform(-1, 1), rng.uniform(-1, 1), 0.3), segs=6, rings=4,
                     patch=True)
    # Grass blades: thin bent quads and a tip, on the palette's grass swatches.
    for _ in range(10):
        base = Vector((rng.uniform(-20, 20), rng.uniform(-20, 20), rng.uniform(14, 40)))
        d = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.6, 1.0))).normalized()
        side = d.orthogonal().normalized()
        h, w = rng.uniform(10, 22), rng.uniform(0.8, 1.3)
        bend = side.cross(d) * h * 0.25
        mid, tip = base + d * h * 0.5 + bend * 0.5, base + d * h + bend
        col = rng.choice(('GrassFresh', 'GrassDeep', 'GrassOlive'))
        v = [m.bm.verts.new(p) for p in (base - side * w, base + side * w, mid + side * w * 0.7, mid - side * w * 0.7, tip)]
        m.face((v[0], v[1], v[2], v[3]), [swatch(col, 0, 0), swatch(col, 0, 1), swatch(col, 0.5, 1), swatch(col, 0.5, 0)])
        m.face((v[3], v[2], v[4]), [swatch(col, 0.5, 0), swatch(col, 0.5, 1), swatch(col, 1.0, 0.5)])
    return m   # one-sided blades are fine: the foliage master draws both sides


def vertex_colors(obj, wind_free):
    """'Col' (linear, face corners): white for the gel; for the inside, R = 0 (no wind sway), G a value per part
    (the foliage master's hue variation), A = 1 (no baked occlusion: the gel moves)."""
    mesh = obj.data
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    values = [1.0, 1.0, 1.0, 1.0] if not wind_free else [0.0, 0.5, 0.0, 1.0]
    col.data.foreach_set('color', values * len(mesh.loops))


# --- The rig ---

gel = gel_body().finish('SlimeGel', GEL, sharp=80.0)
core = core_part().finish('SlimeCore', INNER, sharp=80.0)
bits = inner_bits().finish('SlimeBits', INNER, sharp=80.0)
vertex_colors(gel, False)
vertex_colors(core, True)
vertex_colors(bits, True)

rig = lm.armature()
core_center = Vector(CORE['center'])
lm.bones(rig, [('body', lc.to_blender((0.0, 0.0, 0.0)), lc.to_blender((0.0, 0.0, 40.0)), None),
               ('core', lc.to_blender(core_center), lc.to_blender(core_center + Vector((0.0, 0.0, 10.0))), 'body')])
lm.hit_hull(rig, 'body', [gel])
lm.hit_hull(rig, 'core', [core])
slime = lm.skin(rig, 'Slime', {'body': [gel, bits], 'core': [core]})
rig['LODs'] = '50,25'
rig['LODScreens'] = '0.25,0.1'
lt._log(f'Slime: {sum(len(p.vertices) - 2 for p in slime.data.polygons)} triangles, 2 bones, '
        f'materials {", ".join(m.name for m in slime.data.materials)}')

if lt.want_preview():
    lt.preview([slime], lt.preview_path('Creatures', 'Slime'), view=(0.6, -1.0, 0.45))
