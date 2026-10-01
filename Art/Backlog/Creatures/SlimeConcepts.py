"""Eight slime concepts for the user to choose from (2026-10-01), in the stylized-realism style: wet, glossy gel with
real light passing through it, and things caught inside. Concept art kept in Art/Backlog (see its README): nothing in
the game uses it.

  MeadowSlime   pale green dewy gel with pebbles, grass blades and seeds suspended in it and a dark green core.
  HoneySlime    deep amber, thick and dripping, a chunk of honeycomb and a trapped bee inside.
  BoulderSlime  grey-green ooze carrying mossy granite plates on its back, the gel only showing between the stones.
  MagmaSlime    a black basalt crust cracked over a glowing orange body, the cracks brightest on top.
  CrystalSlime  clear ice-blue jelly with pale crystals growing out of its top and floating inside.
  BogSlime      murky purple-green, cloudy, bubbles rising through it past a sunken skull and bones.
  MushroomSlime milky cream jelly with red-brown and tan mushrooms sprouting from its crown.
  ScrapSlime    oily black ooze with a chrome sheen, gears, bolts, shell casings and a coin half swallowed: it drops
                what it ate.

Most have a core (a darker nucleus): the natural critical spot if a slime joins the game. Each slime is a few mesh
objects (the body, its drips, what's inside); the gel is a transmissive material, so the renders use Eevee's ray
traced refraction.

    blender -b --factory-startup --python Art/Backlog/Creatures/SlimeConcepts.py -- --preview [MeadowSlime ...]
"""
import math
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_textures as lt
from looter_plants import swatch


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


# --- Materials ---

def gel(name, color, rough=0.08, ior=1.34, glow=0.25, murk=0.0, emit=None, emit_strength=0.0, coat=0.35, **_):
    """A wet translucent gel: bright tinted transmission (what's inside shows through), a faint glow of its own color
    standing in for the light it scatters, and murk (0..1) clouding it toward an opaque jelly."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']
    out = nodes['Material Output']
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Transmission Weight'].default_value = 1.0 - 0.7 * murk
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['IOR'].default_value = ior
    bsdf.inputs['Coat Weight'].default_value = coat
    bsdf.inputs['Coat Roughness'].default_value = 0.03
    bsdf.inputs['Subsurface Weight'].default_value = murk
    bsdf.inputs['Subsurface Radius'].default_value = (0.3, 0.25, 0.2)
    bsdf.inputs['Subsurface Scale'].default_value = 0.15
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(emit if emit is not None else color)
    bsdf.inputs['Emission Strength'].default_value = emit_strength if emit is not None else glow
    # Shadow rays pass the gel tinted by its color (a light path trick): the sun lights what floats inside, as the
    # scattered light in a real gel would, instead of leaving it in the dark.
    light_path = nodes.new('ShaderNodeLightPath')
    clear = nodes.new('ShaderNodeBsdfTransparent')
    clear.inputs['Color'].default_value = lt.hex_color(color)
    mix = nodes.new('ShaderNodeMixShader')
    links.new(light_path.outputs['Is Shadow Ray'], mix.inputs['Fac'])
    links.new(bsdf.outputs['BSDF'], mix.inputs[1])
    links.new(clear.outputs['BSDF'], mix.inputs[2])
    links.new(mix.outputs['Shader'], out.inputs['Surface'])
    mat.surface_render_method = 'DITHERED'
    mat.use_raytrace_refraction = True
    # A thin slab: light crosses the gel with only a small offset, so what floats inside stays visible.
    mat.thickness_mode = 'SLAB'
    thick = nodes.new('ShaderNodeValue')
    thick.outputs[0].default_value = 0.04
    links.new(thick.outputs[0], out.inputs['Thickness'])
    return mat


def solid(name, color, rough=0.6, metal=0.0, coat=0.0, emit=None, emit_strength=0.0, bump=0.0, scale=40.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Coat Weight'].default_value = coat
    if emit is not None:
        bsdf.inputs['Emission Color'].default_value = lt.hex_color(emit)
        bsdf.inputs['Emission Strength'].default_value = emit_strength
    if bump:
        tex = nodes.new('ShaderNodeTexNoise')
        tex.inputs['Scale'].default_value = scale
        tex.inputs['Detail'].default_value = 6.0
        b = nodes.new('ShaderNodeBump')
        b.inputs['Strength'].default_value = bump
        b.inputs['Distance'].default_value = 0.01
        links.new(tex.outputs['Fac'], b.inputs['Height'])
        links.new(b.outputs['Normal'], bsdf.inputs['Normal'])
    return mat


def magma_crust(name):
    """Black basalt plates over a glowing body: a noise pattern splits crust from crack, cracks glow orange."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']
    coords = nodes.new('ShaderNodeTexCoord')
    vor = nodes.new('ShaderNodeTexVoronoi')
    vor.feature = 'DISTANCE_TO_EDGE'
    vor.inputs['Scale'].default_value = 7.0
    warp = nodes.new('ShaderNodeTexNoise')
    warp.inputs['Scale'].default_value = 3.0
    mixv = nodes.new('ShaderNodeMix')
    mixv.data_type = 'VECTOR'
    mixv.inputs['Factor'].default_value = 0.12
    links.new(coords.outputs['Object'], mixv.inputs['A'])
    links.new(warp.outputs['Color'], mixv.inputs['B'])
    links.new(coords.outputs['Object'], warp.inputs['Vector'])
    links.new(mixv.outputs['Result'], vor.inputs['Vector'])
    crack = nodes.new('ShaderNodeMapRange')
    crack.inputs['From Min'].default_value = 0.0
    crack.inputs['From Max'].default_value = 0.06
    crack.inputs['To Min'].default_value = 1.0
    crack.inputs['To Max'].default_value = 0.0
    links.new(vor.outputs['Distance'], crack.inputs['Value'])
    up = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Object'], up.inputs['Vector'])
    heat = nodes.new('ShaderNodeMapRange')       # the cracks glow more toward the top
    heat.inputs['From Min'].default_value = 0.0
    heat.inputs['From Max'].default_value = 0.9
    heat.inputs['To Min'].default_value = 3.0
    heat.inputs['To Max'].default_value = 9.0
    links.new(up.outputs['Z'], heat.inputs['Value'])
    glow = nodes.new('ShaderNodeMath')
    glow.operation = 'MULTIPLY'
    links.new(crack.outputs['Result'], glow.inputs[0])
    links.new(heat.outputs['Result'], glow.inputs[1])
    color = nodes.new('ShaderNodeMix')
    color.data_type = 'RGBA'
    color.inputs['A'].default_value = lt.hex_color(0x1e1714)
    color.inputs['B'].default_value = lt.hex_color(0xff6a14)
    links.new(crack.outputs['Result'], color.inputs['Factor'])
    links.new(color.outputs['Result'], bsdf.inputs['Base Color'])
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(0xff5a10)
    links.new(glow.outputs['Value'], bsdf.inputs['Emission Strength'])
    rough = nodes.new('ShaderNodeMapRange')
    rough.inputs['To Min'].default_value = 0.75
    rough.inputs['To Max'].default_value = 0.35
    links.new(crack.outputs['Result'], rough.inputs['Value'])
    links.new(rough.outputs['Result'], bsdf.inputs['Roughness'])
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.8
    bump.inputs['Distance'].default_value = 0.03
    links.new(vor.outputs['Distance'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    return mat


def oil(name):
    """Black oil with a thin-film rainbow sheen and a chrome glint."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x0e0f12)
    bsdf.inputs['Metallic'].default_value = 0.55
    bsdf.inputs['Roughness'].default_value = 0.12
    bsdf.inputs['Coat Weight'].default_value = 1.0
    bsdf.inputs['Coat Roughness'].default_value = 0.02
    if 'Thin Film Thickness' in bsdf.inputs:
        bsdf.inputs['Thin Film Thickness'].default_value = 420.0
        bsdf.inputs['Thin Film IOR'].default_value = 1.45
    return mat


# --- Geometry ---

def link(name, bm, mat):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    if mat is not None:
        mesh.materials.append(mat)
    for p in mesh.polygons:
        p.use_smooth = True
    # The textured materials darken by the vertex color's alpha (baked occlusion): white, or they'd render black.
    colors = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    colors.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    return obj


def blob(name, mat, radius, height, seed, spread=0.22, peak=0.0, wobble=0.04, lean=(0.0, 0.0), segs=72, rings=48,
         center=(0.0, 0.0), flat=0.06, sag=0.0):
    """A slime body: a sphere squashed to height, widened toward its foot, a flat contact patch on the ground, an
    optional pulled-up peak and a lean, and a slow wobble over its skin. Returns the object and a function giving the
    surface height over a ground point (to sit things on its back)."""
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=rings, radius=1.0)
    off = Vector((seed * 1.7, seed * 0.9, seed * 2.3))
    for v in bm.verts:
        x, y, z = v.co
        r = 1.0 + wobble * noise.noise(Vector((x, y, z)) * 1.6 + off) + 0.5 * wobble * noise.noise(Vector((x, y, z)) * 4.0 + off)
        s = 1.0 + spread * smoothstep(0.35, -1.0, z) - sag * smoothstep(-0.2, 0.6, z) * 0.3
        px, py = x * radius * s * r, y * radius * s * r
        pz = (z * 0.5 + 0.5) * height * r
        pz += peak * height * smoothstep(0.6, 1.0, z) ** 2
        pz = max(pz, flat * height * smoothstep(-1.0, -0.6, z) * 0.0)
        # Lean: the top shifts sideways the higher it is.
        px += lean[0] * pz
        py += lean[1] * pz
        if z < -0.55:   # the foot spreads flat on the ground
            pz *= smoothstep(-1.0, -0.55, z) ** 1.5
        v.co = Vector((px + center[0], py + center[1], max(pz, 0.0)))
    obj = link(name, bm, mat)

    def top(x, y):
        # The highest point of the body over (x, y): ray cast down on the mesh.
        from mathutils.bvhtree import BVHTree
        tree = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
        hit = tree.ray_cast(Vector((x, y, height * 3.0)), Vector((0, 0, -1)))
        return hit[0].z if hit[0] is not None else 0.0
    return obj, top


def drips(name, mat, radius, count, seed, height=0.06):
    """Small flattened droplets around the slime's foot, as if it just moved."""
    rng = random.Random(seed)
    bm = bmesh.new()
    for k in range(count):
        a = rng.uniform(0, 2 * math.pi)
        d = radius * rng.uniform(1.15, 1.6)
        r = rng.uniform(0.03, 0.09)
        bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=10, radius=1.0,
                                  matrix=Matrix.LocRotScale(Vector((math.cos(a) * d, math.sin(a) * d, r * 0.25)), None,
                                                            Vector((r * rng.uniform(1.0, 1.8), r, r * 0.45))))
    return link(name, bm, mat)


def puddle(name, mat, radius):
    """A thin wet film on the ground around the foot."""
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=True, segments=48, radius=radius,
                            matrix=Matrix.Translation((0, 0, 0.004)))
    for v in bm.verts:
        a = math.atan2(v.co.y, v.co.x)
        k = 1.0 + 0.12 * math.sin(a * 5.0) + 0.06 * math.sin(a * 11.0 + 1.0)
        v.co.x *= k
        v.co.y *= k
    return link(name, bm, mat)


def sphere(bm, center, radii, segs=16, rings=10, rot=None):
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=rings, radius=1.0,
                              matrix=Matrix.LocRotScale(Vector(center), rot, Vector(radii)))


def pebbles(name, mat, spots, seed):
    rng = random.Random(seed)
    bm = bmesh.new()
    for c, r in spots:
        before = set(bm.verts)
        sphere(bm, c, (r * rng.uniform(1.0, 1.4), r, r * rng.uniform(0.6, 0.9)), 12, 8,
               rot=Matrix.Rotation(rng.uniform(0, 3), 3, 'Z').to_quaternion())
        off = Vector((rng.uniform(0, 9), rng.uniform(0, 9), 0))
        for v in set(bm.verts) - before:
            v.co += (v.co - Vector(c)).normalized() * r * 0.18 * noise.noise(v.co * 18.0 + off)
    obj = link(name, bm, mat)
    lt.box_uv(obj, 'RockGranite') if mat.get('TextureSet') else None
    return obj


def blades(name, mat, center, count, seed, length=(0.12, 0.3), spread=0.15, colors=('GrassFresh', 'GrassDeep', 'GrassOlive'),
           tilt=0.9):
    """Loose grass blades tumbling in the gel (or standing out of something), on the foliage palette."""
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    for _ in range(count):
        base = Vector(center) + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.6, 0.6))) * spread
        d = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1) * tilt + (1 - tilt))).normalized()
        side = d.orthogonal().normalized()
        h, w = rng.uniform(*length), rng.uniform(0.008, 0.014)
        bend = side.cross(d) * h * 0.25
        pts = [base, base + d * h * 0.5 + bend * 0.5, base + d * h + bend]
        col = rng.choice(colors)
        v = [bm.verts.new(p) for p in (pts[0] - side * w, pts[0] + side * w, pts[1] + side * w * 0.7, pts[1] - side * w * 0.7, pts[2])]
        f1 = bm.faces.new((v[0], v[1], v[2], v[3]))
        f2 = bm.faces.new((v[3], v[2], v[4]))
        for f, uvs in ((f1, [swatch(col, 0, 0), swatch(col, 0, 1), swatch(col, 0.5, 1), swatch(col, 0.5, 0)]),
                       (f2, [swatch(col, 0.5, 0), swatch(col, 0.5, 1), swatch(col, 1.0, 0.5)])):
            for loop, t in zip(f.loops, uvs):
                loop[uv].uv = t
    return link(name, bm, mat)


def gem(bm, base, direction, length, radius, sides=6):
    """A hexagonal crystal: a prism with a pointed tip."""
    d = Vector(direction).normalized()
    a = d.orthogonal().normalized()
    b = d.cross(a)
    ring0 = [bm.verts.new(base + (a * math.cos(t) + b * math.sin(t)) * radius) for t in (2 * math.pi * k / sides for k in range(sides))]
    ring1 = [bm.verts.new(v.co + d * length * 0.75) for v in ring0]
    tip = bm.verts.new(base + d * length)
    for k in range(sides):
        k1 = (k + 1) % sides
        bm.faces.new((ring0[k], ring0[k1], ring1[k1], ring1[k]))
        bm.faces.new((ring1[k], ring1[k1], tip))
    bm.faces.new(list(reversed(ring0)))


def gear(bm, center, normal, radius, teeth, thick):
    """A spur gear: a disc with teeth and a hole-like hub."""
    n = Vector(normal).normalized()
    a = n.orthogonal().normalized()
    b = n.cross(a)
    pts = []
    for k in range(teeth * 4):
        t = 2 * math.pi * k / (teeth * 4)
        r = radius * (1.0 if (k % 4) in (1, 2) else 0.82)
        pts.append(a * math.cos(t) * r + b * math.sin(t) * r)
    c = Vector(center)
    bottom = [bm.verts.new(c + p - n * thick / 2) for p in pts]
    topv = [bm.verts.new(c + p + n * thick / 2) for p in pts]
    bm.faces.new(list(reversed(bottom)))
    bm.faces.new(topv)
    for k in range(len(pts)):
        k1 = (k + 1) % len(pts)
        bm.faces.new((bottom[k], bottom[k1], topv[k1], topv[k]))


def cylinder(bm, center, direction, radius, length, segs=12, r1=None):
    d = Vector(direction).normalized()
    m = Matrix.Translation(Vector(center)) @ d.to_track_quat('Z', 'Y').to_matrix().to_4x4()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=segs, radius1=radius, radius2=r1 if r1 is not None else radius,
                          depth=length, matrix=m)


# --- The eight slimes ---

def meadow():
    green = gel('MeadowGel', 0xb8f088, glow=0.18)
    body, top = blob('MeadowSlime', green, 0.6, 0.62, 1, spread=0.3, peak=0.1, lean=(0.04, 0.0))
    parts = [body, drips('MeadowDrips', green, 0.6, 7, 3), puddle('MeadowPuddle', green, 0.78)]
    parts.append(pebbles('MeadowPebbles', lt.material('RockGranite', name='SlimePebble'),
                         [((0.12, -0.1, 0.18), 0.07), ((-0.18, 0.08, 0.12), 0.05), ((0.05, 0.2, 0.28), 0.04)], 5))
    parts.append(blades('MeadowBlades', lt.material('FoliagePalette'), (0, 0, 0.28), 14, 7, spread=0.2, length=(0.1, 0.22)))
    bm = bmesh.new()
    sphere(bm, (0.02, -0.04, 0.3), (0.11, 0.11, 0.1), 20, 14)
    for k in range(9):
        r = random.Random(k)
        sphere(bm, (r.uniform(-0.28, 0.28), r.uniform(-0.28, 0.28), r.uniform(0.12, 0.42)), (0.012, 0.008, 0.008), 8, 6)
    parts.append(link('MeadowCore', bm, solid('MeadowCore', 0x2f5a1c, rough=0.3, coat=0.5)))
    return parts


def honey():
    amber = gel('HoneyGel', 0xffb438, ior=1.48, glow=0.22, coat=0.5, murk=0.1)
    body, top = blob('HoneySlime', amber, 0.62, 0.6, 2, spread=0.34, peak=0.05, sag=0.6)
    parts = [body, drips('HoneyDrips', amber, 0.66, 10, 4), puddle('HoneyPuddle', amber, 0.88)]
    # Drips running down its sides.
    bm = bmesh.new()
    rng = random.Random(8)
    for k in range(8):
        a = rng.uniform(0, 2 * math.pi)
        z0 = rng.uniform(0.3, 0.6)
        x, y = math.cos(a), math.sin(a)
        z0 *= 0.75
        r = 0.62 * (1 + 0.34 * smoothstep(0.35, -1, (z0 / 0.6) * 2 - 1))
        sphere(bm, (x * r * 0.78, y * r * 0.78, z0 - 0.12), (0.035, 0.035, 0.1), 12, 8)
        sphere(bm, (x * r * 0.8, y * r * 0.8, z0 - 0.24), (0.045, 0.045, 0.05), 12, 8)
    parts.append(link('HoneyRuns', bm, amber))
    # Honeycomb chunk and a bee.
    comb = solid('Honeycomb', 0xe8b04a, rough=0.45, bump=0.3)
    bm = bmesh.new()
    for i in range(4):
        for j in range(3):
            cx = -0.12 + i * 0.052 + (0.026 if j % 2 else 0.0)
            cy = 0.05 + j * 0.045
            cylinder(bm, (cx, cy, 0.24), (0.3, 0.2, 1.0), 0.028, 0.08, segs=6)
    parts.append(link('HoneyComb', bm, comb))
    bm = bmesh.new()
    sphere(bm, (0.14, -0.12, 0.36), (0.045, 0.028, 0.028), 14, 10)
    sphere(bm, (0.19, -0.12, 0.37), (0.022, 0.022, 0.022), 12, 8)
    parts.append(link('Bee', bm, solid('BeeBody', 0x1a140c, rough=0.4)))
    bm = bmesh.new()
    for dx in (0.125, 0.145):
        cylinder(bm, (dx, -0.12, 0.36), (1, 0, 0), 0.03, 0.008, segs=14)
    parts.append(link('BeeStripes', bm, solid('BeeStripe', 0xf2c230, rough=0.4)))
    bm = bmesh.new()
    for side in (-1, 1):
        sphere(bm, (0.15, -0.12 + side * 0.04, 0.4), (0.03, 0.04, 0.004), 10, 6)
    parts.append(link('BeeWings', bm, solid('BeeWing', 0xe8eef0, rough=0.1, coat=1.0)))
    return parts


def boulder():
    ooze = gel('BoulderGel', 0x9ab890, rough=0.12, glow=0.18, murk=0.45)
    body, top = blob('BoulderSlime', ooze, 0.66, 0.62, 3, spread=0.3, peak=0.0, wobble=0.05)
    parts = [body, drips('BoulderDrips', ooze, 0.68, 6, 5), puddle('BoulderPuddle', ooze, 0.86)]
    # Granite plates riding on its back, mossy on top.
    granite = lt.material('RockGranite', name='SlimeGranite', MossAmount=0.7)
    moss = solid('SlimeMoss', 0x5a7a2a, rough=0.95, bump=0.9, scale=80.0)
    rng = random.Random(12)
    bm = bmesh.new()
    mbm = bmesh.new()
    for k, (a, d, r) in enumerate([(0.0, 0.0, 0.2), (1.0, 0.3, 0.16), (2.4, 0.33, 0.17), (3.9, 0.3, 0.15), (5.2, 0.32, 0.14),
                                    (0.4, 0.5, 0.1), (3.0, 0.52, 0.1)]):
        x, y = math.cos(a) * d, math.sin(a) * d
        z = top(x, y)
        n = Vector((x * 1.4, y * 1.4, 1.0)).normalized()
        rot = n.to_track_quat('Z', 'Y')
        before = set(bm.verts)
        sphere(bm, (x, y, z - r * 0.12), (r, r * 0.85, r * 0.42), 12, 8, rot=rot)
        for v in set(bm.verts) - before:
            v.co += (v.co - Vector((x, y, z))).normalized() * r * 0.15 * noise.noise(v.co * 12.0 + Vector((k, 0, 0)))
        sphere(mbm, (x, y, z + r * 0.22), (r * 0.7, r * 0.6, r * 0.14), 10, 6, rot=rot)
    stones = link('BoulderStones', bm, granite)
    lt.box_uv(stones, 'RockGranite')
    parts += [stones, link('BoulderMoss', mbm, moss)]
    parts.append(blades('BoulderGrass', lt.material('FoliagePalette'), (0, 0, top(0, 0) + 0.06), 18, 13, spread=0.12,
                        length=(0.08, 0.2), tilt=0.15))
    bm = bmesh.new()
    sphere(bm, (0.05, -0.05, 0.24), (0.12, 0.12, 0.1), 20, 14)
    parts.append(link('BoulderCore', bm, solid('BoulderCore', 0x2c3a28, rough=0.4)))
    return parts


def magma():
    crust = magma_crust('MagmaCrust')
    body, top = blob('MagmaSlime', crust, 0.62, 0.64, 4, spread=0.3, peak=0.08, wobble=0.06)
    glowing = solid('MagmaPool', 0x401006, rough=0.3, emit=0xff5010, emit_strength=6.0)
    parts = [body, drips('MagmaDrips', glowing, 0.62, 8, 6), puddle('MagmaScorch', solid('Scorch', 0x16110e, rough=0.95), 0.85)]
    # Embers floating up off it.
    bm = bmesh.new()
    rng = random.Random(4)
    for _ in range(14):
        sphere(bm, (rng.uniform(-0.5, 0.5), rng.uniform(-0.5, 0.5), rng.uniform(0.7, 1.4)), (0.008,) * 3, 6, 4)
    parts.append(link('Embers', bm, solid('Ember', 0x401006, emit=0xffa040, emit_strength=20.0)))
    return parts


def crystal():
    ice = gel('CrystalGel', 0xd4f0ff, rough=0.03, ior=1.4, glow=0.08, coat=0.6)
    body, top = blob('CrystalSlime', ice, 0.58, 0.64, 5, spread=0.28, peak=0.14, wobble=0.03)
    parts = [body, drips('CrystalDrips', ice, 0.56, 6, 7), puddle('CrystalPuddle', ice, 0.72)]
    shard = solid('Crystal', 0xd8f0ff, rough=0.05, coat=1.0, emit=0x9fd8ff, emit_strength=0.6)
    bm = bmesh.new()
    rng = random.Random(21)
    for k in range(7):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0.0, 0.28)
        x, y = math.cos(a) * d, math.sin(a) * d
        z = top(x, y)
        n = Vector((x * 2.0, y * 2.0, 1.0)).normalized()
        gem(bm, Vector((x, y, z - 0.06)), n + Vector((rng.uniform(-0.2, 0.2), rng.uniform(-0.2, 0.2), 0)),
            rng.uniform(0.14, 0.3), rng.uniform(0.025, 0.05))
    for k in range(6):
        gem(bm, Vector((rng.uniform(-0.22, 0.22), rng.uniform(-0.22, 0.22), rng.uniform(0.12, 0.4))),
            Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))), rng.uniform(0.06, 0.12), rng.uniform(0.012, 0.025))
    parts.append(link('Crystals', bm, shard))
    bm = bmesh.new()
    sphere(bm, (0.0, -0.03, 0.3), (0.09, 0.09, 0.09), 20, 14)
    parts.append(link('CrystalCore', bm, solid('CrystalCore', 0x3a7ab0, rough=0.1, coat=1.0, emit=0x60b0ff, emit_strength=1.5)))
    return parts


def bog():
    murk = gel('BogGel', 0xa496b8, rough=0.16, glow=0.1, murk=0.12)
    body, top = blob('BogSlime', murk, 0.68, 0.56, 6, spread=0.34, sag=0.4, wobble=0.07)
    parts = [body, drips('BogDrips', murk, 0.68, 9, 8), puddle('BogPuddle', murk, 0.9)]
    bone = solid('Bone', 0xd8ceb0, rough=0.6, bump=0.2)
    bm = bmesh.new()
    sphere(bm, (0.06, 0.02, 0.24), (0.11, 0.09, 0.09), 20, 14)                # skull
    sphere(bm, (0.13, 0.02, 0.19), (0.06, 0.06, 0.05), 16, 10)                # jaw
    cylinder(bm, (-0.15, -0.12, 0.2), (1, 0.4, 0.3), 0.02, 0.3, segs=10)        # bones
    cylinder(bm, (-0.05, 0.18, 0.15), (0.3, 1, -0.2), 0.018, 0.24, segs=10)
    for end in ((-0.29, -0.18, 0.15), (-0.01, -0.06, 0.25)):
        sphere(bm, end, (0.03, 0.03, 0.03), 10, 8)
    parts.append(link('BogBones', bm, bone))
    bm = bmesh.new()
    for side in (-1, 1):
        sphere(bm, (0.16, 0.02 + side * 0.04, 0.26), (0.025, 0.025, 0.025), 10, 8)
    parts.append(link('SkullEyes', bm, solid('SkullHole', 0x0a0806, rough=0.9)))
    bm = bmesh.new()
    rng = random.Random(30)
    for _ in range(18):
        r = rng.uniform(0.012, 0.035)
        sphere(bm, (rng.uniform(-0.32, 0.32), rng.uniform(-0.32, 0.32), rng.uniform(0.08, 0.44)), (r, r, r), 12, 8)
    parts.append(link('BogBubbles', bm, gel('Bubble', 0xe4ffd0, rough=0.0, ior=1.0, emit=0x9cff60, emit_strength=0.8)))
    return parts


def mushroom():
    milk = gel('MushroomGel', 0xf6ead2, rough=0.14, glow=0.12, murk=0.6)
    body, top = blob('MushroomSlime', milk, 0.62, 0.6, 7, spread=0.3, peak=0.05)
    parts = [body, drips('MushroomDrips', milk, 0.6, 6, 9), puddle('MushroomPuddle', milk, 0.78)]
    caps = [solid('CapRed', 0x9a3a22, rough=0.45, coat=0.4, bump=0.2), solid('CapTan', 0xb8905a, rough=0.6, bump=0.2)]
    stem = solid('Stem', 0xe8dcc0, rough=0.7, bump=0.15)
    rng = random.Random(40)
    cap_bm = [bmesh.new(), bmesh.new()]
    stem_bm = bmesh.new()
    spots_bm = bmesh.new()
    for k, (a, d, h, r) in enumerate([(0.3, 0.05, 0.28, 0.16), (2.0, 0.24, 0.18, 0.1), (3.6, 0.22, 0.2, 0.11),
                                       (5.0, 0.2, 0.14, 0.07), (1.2, 0.3, 0.1, 0.05)]):
        x, y = math.cos(a) * d, math.sin(a) * d
        z = top(x, y) - 0.04
        lean = Vector((x * 1.5, y * 1.5, 1.0)).normalized()
        cylinder(stem_bm, Vector((x, y, z)) + lean * h * 0.5, lean, r * 0.28, h, segs=12, r1=r * 0.22)
        tipc = Vector((x, y, z)) + lean * h
        sphere(cap_bm[k % 2], tipc, (r, r, r * 0.55), 20, 12, rot=lean.to_track_quat('Z', 'Y'))
        if k % 2 == 0:
            for _ in range(6):
                t = rng.uniform(0, 2 * math.pi)
                sphere(spots_bm, tipc + Vector((math.cos(t) * r * 0.6, math.sin(t) * r * 0.6, r * 0.4)), (r * 0.12,) * 3, 8, 6)
    parts += [link('Caps0', cap_bm[0], caps[0]), link('Caps1', cap_bm[1], caps[1]), link('Stems', stem_bm, stem),
              link('CapSpots', spots_bm, solid('CapSpot', 0xf4ecd8, rough=0.6))]
    bm = bmesh.new()
    sphere(bm, (0.0, -0.05, 0.26), (0.1, 0.1, 0.09), 20, 14)
    parts.append(link('MushroomCore', bm, solid('MushroomCore', 0x6a4a2a, rough=0.5)))
    return parts


def scrap():
    slick = oil('ScrapOil')
    body, top = blob('ScrapSlime', slick, 0.64, 0.6, 8, spread=0.32, peak=0.06, wobble=0.06)
    parts = [body, drips('ScrapDrips', slick, 0.66, 10, 10), puddle('ScrapPuddle', slick, 0.9)]
    steel = lt.material('MetalWorn', name='ScrapSteel', tint=0x8a8d90)
    brass = lt.material('MetalWorn', name='ScrapBrass', tint=0xf0bd5c)
    rust = lt.material('MetalRust', name='ScrapRust', uv_scale=2.0)
    rng = random.Random(50)
    steel_bm, brass_bm, rust_bm = bmesh.new(), bmesh.new(), bmesh.new()
    # Half-swallowed junk poking out of its skin.
    for k in range(5):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0.1, 0.42)
        x, y = math.cos(a) * d, math.sin(a) * d
        z = top(x, y)
        n = Vector((x * 1.8, y * 1.8, 1.0)).normalized() + Vector((rng.uniform(-0.4, 0.4), rng.uniform(-0.4, 0.4), 0))
        gear(rust_bm if k % 2 else steel_bm, Vector((x, y, z)) - n.normalized() * 0.02, n.orthogonal(), rng.uniform(0.06, 0.11),
             rng.randint(8, 12), 0.02)
    for k in range(7):   # shell casings and bolts
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0.05, 0.45)
        x, y = math.cos(a) * d, math.sin(a) * d
        z = top(x, y)
        n = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.2, 1))).normalized()
        if k % 2:
            cylinder(brass_bm, Vector((x, y, z)), n, 0.012, 0.06, segs=10)
        else:
            cylinder(steel_bm, Vector((x, y, z)), n, 0.01, 0.08, segs=6)
            cylinder(steel_bm, Vector((x, y, z)) + n * 0.04, n, 0.02, 0.014, segs=6)
    cylinder(brass_bm, (0.15, -0.3, top(0.15, -0.3) - 0.005), (0.3, -0.5, 1.0), 0.04, 0.008, segs=24)   # a coin
    for k in range(4):   # junk sunk deeper inside
        gear(rust_bm, Vector((rng.uniform(-0.2, 0.2), rng.uniform(-0.2, 0.2), rng.uniform(0.15, 0.38))),
             Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))), rng.uniform(0.05, 0.08), 10, 0.016)
    for bm, mat, set_name, label in ((steel_bm, steel, 'MetalWorn', 'ScrapSteelBits'), (brass_bm, brass, 'MetalWorn', 'ScrapBrassBits'),
                                     (rust_bm, rust, 'MetalRust', 'ScrapRustBits')):
        obj = link(label, bm, mat)
        lt.box_uv(obj, set_name, texel_density=1024.0)
        parts.append(obj)
    return parts


SLIMES = {'MeadowSlime': meadow, 'HoneySlime': honey, 'BoulderSlime': boulder, 'MagmaSlime': magma,
          'CrystalSlime': crystal, 'BogSlime': bog, 'MushroomSlime': mushroom, 'ScrapSlime': scrap}

built = {}
for k, (name, make) in enumerate(SLIMES.items()):
    objs = make()
    for o in objs:
        o.location.x += k * 3.0      # side by side in the scene
    built[name] = objs
    lt._log(f'{name}: {sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in objs)} triangles')

if lt.want_preview():
    argv = sys.argv[sys.argv.index('--') + 1:]
    for name in [a for a in argv if a in SLIMES] or list(SLIMES):
        lt.preview(built[name], lt.preview_path('Backlog', name), view=(0.6, -1.0, 0.45))
