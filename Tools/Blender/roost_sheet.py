"""The renders and the look sheet of Hob's roost (Art/Models/Props/HobRoost.py runs this with --preview):
Saved/ArtPreviews/RansomsRest/Roost/*.png and Saved/ArtPreviews/RansomsRest/LookSheets/41_Roost.jpg, for the user's phone.

What it shows:
  - the three directions at one scale in Ransom's Rest's golden late afternoon (unlit, as a roost first looks) and at
    dusk with the beacon burning, with a 1.8 m figure and the fauna's crows on their perches;
  - the 60 m test: the three as a 1080p player sees them 60 m off (the game's 90 degree view, at its pixel size,
    magnified on the sheet), by day and at dusk;
  - the recommended direction close up (its head; its foot at eye height);
  - the beacon colour options, none of them a loot colour (white, green, blue, purple, orange, gold) nor the UI's cyan:
    each on the roost near, at dusk, and as a far beacon over a dusk valley 130 m off.

The beacon here is a stand-in for the game's (World/RoostBeacon): an emissive flame in the cresset, a soft halo and a
tall soft column of light fading upward, all additive or emissive (no light casts from it, as in the game on Medium).
The compositor's glare stands in for the game's bloom. The crows are the fauna's own (Intermediate/ArtExport_Fauna's
FBX with the FaunaAtlas texture), or plain black stand-ins if that export isn't there. Nothing here is exported: every
object it adds is named '_...'.

    --quick   only the golden three-quarter view of each direction (for checking a change fast)
"""
import json
import math
import os
import random

import bmesh
import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Matrix, Vector, noise

import looter_homes as homes
import looter_textures as lt

OUT = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Roost')
SHEET = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets', '41_Roost.jpg')
FAUNA = os.path.join(lt.REPO, 'Intermediate', 'ArtExport_Fauna')
UP = Vector((0.0, 0.0, 1.0))
ARGS = homes.args()

# The beacon colours (sRGB): the column's colour and the flame's hot core. None is a loot colour (common white-grey,
# uncommon green, rare blue, epic purple, legendary gold-orange) nor the UI's orange accent or cyan lines.
COLOURS = [
    ('Ember', "1  Hob's Ember (recommended)", 0xd8321c, 0xff8a52,
     "A deep ember red, the coal in Hob's empty socket. No loot is red, so it is never a drop; it holds\n"
     "against the gold afternoon and burns hot over a blue dusk. Red can say danger: the crows and bell say safe."),
    ('Ghost', '2  Ghost Violet', 0xa496e0, 0xe4dcff,
     "A pale violet-grey, a ghost's light. Eerie and calm, but its hue is the epic purple's, and pale\n"
     "enough that in bloom, far off, it reads as a white loot beam."),
    ('Silver', '3  Grave Silver', 0x96e0dc, 0xe8fffc,
     "A cold silver-cyan. Lovely at dusk, but it sits between the uncommon green and the UI's cyan\n"
     "lines, and pales to white in bloom like the ammo beams."),
    ('Bone', '4  Corpse Candle', 0xd2c872, 0xfff4c0,
     "A sickly bone-yellow. Ghostly and warm, but in the golden light it slides toward the legendary\n"
     "gold, the one colour a player must never doubt."),
]
RECOMMENDED = 'Ember'


def out_path(name):
    return os.path.join(OUT, name + '.png')


# --- Crows: the fauna's models (or stand-ins) ---

class Crows:
    """Templates for perched and flying crows, placed as linked copies."""

    def __init__(self):
        self.perched = self.head = self.flying = None
        self.head_at = Vector((0.08, 0.0, 0.205))
        self.wings = {}
        self.sockets = {}
        manifest = os.path.join(FAUNA, 'Art_Models_Creatures_AmbientFauna.json')
        try:
            with open(manifest, encoding='utf-8') as f:
                info = {m['name']: m for m in json.load(f)['models']}
            mat = self._atlas()
            parts = {}
            for key in ('Crow_Perched', 'Crow_Head', 'Crow_Flying', 'Crow_WingInnerL', 'Crow_WingInnerR',
                        'Crow_WingOuterL', 'Crow_WingOuterR'):
                parts[key] = self._import(os.path.join(FAUNA, f'SM_{key}.fbx'), '_T' + key, mat)
                # The manifest's sockets are in Unreal's axes and centimetres.
                self.sockets[key] = {s['name']: Vector((-s['location'][1] / 100.0, -s['location'][0] / 100.0,
                                                        s['location'][2] / 100.0)) for s in info['SM_' + key]['sockets']}
            self.perched, self.head, self.flying = parts['Crow_Perched'], parts['Crow_Head'], parts['Crow_Flying']
            self.wings = {k: v for k, v in parts.items() if 'Wing' in k}
            self.head_at = self.sockets['Crow_Perched']['Head']
            lt._log('roost: the fauna crows are in the renders')
        except Exception as error:      # no fauna export here: plain stand-ins
            lt._log(f'roost: stand-in crows ({error})')
            self.perched, self.head, self.flying = self._standin(), None, None

    @staticmethod
    def _atlas():
        mat = bpy.data.materials.new('_FaunaAtlas')
        mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
        tex = nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(os.path.join(lt.REPO, 'Art', 'Textures', 'FaunaAtlas', 'T_FaunaAtlas_BC.png'))
        links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
        bsdf.inputs['Roughness'].default_value = 0.62
        return mat

    @staticmethod
    def _import(path, name, mat):
        before = set(o.name for o in bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=path)
        new = [o for o in bpy.data.objects if o.name not in before and o.type == 'MESH']
        obj = new[0]
        for o in bpy.data.objects:
            if o.name not in before and o is not obj:
                bpy.data.objects.remove(o, do_unlink=True)
        obj.name = name
        obj.parent = None
        obj.matrix_world = Matrix.Identity(4)
        obj.data.materials.clear()
        obj.data.materials.append(mat)
        obj.hide_render = True
        return obj

    @staticmethod
    def _standin():
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=10, v_segments=6, radius=1.0,
                                  matrix=Matrix.Translation((0.0, -0.02, 0.13)) @ Matrix.Diagonal((0.065, 0.13, 0.075, 1.0)))
        bmesh.ops.create_uvsphere(bm, u_segments=8, v_segments=5, radius=0.045, matrix=Matrix.Translation((0.0, -0.11, 0.22)))
        bmesh.ops.create_cone(bm, segments=6, radius1=0.016, radius2=0.0, depth=0.06, cap_ends=True,
                              matrix=Matrix.Translation((0.0, -0.17, 0.215)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
        bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation((0.0, 0.17, 0.09)) @ Matrix.Rotation(math.radians(-25.0), 4, 'X')
                              @ Matrix.Diagonal((0.07, 0.16, 0.012, 1.0)))
        mesh = bpy.data.meshes.new('_TCrow')
        bm.to_mesh(mesh)
        bm.free()
        obj = bpy.data.objects.new('_TCrow', mesh)
        bpy.context.scene.collection.objects.link(obj)
        mat = bpy.data.materials.new('_CrowBlack')
        mat.use_nodes = True
        bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Base Color'].default_value = (0.012, 0.012, 0.016, 1.0)
        bsdf.inputs['Roughness'].default_value = 0.4
        mesh.materials.append(mat)
        obj.hide_render = True
        return obj

    @staticmethod
    def _copy(template, matrix):
        obj = template.copy()
        obj.name = '_Shot' + template.name
        bpy.context.scene.collection.objects.link(obj)
        obj.matrix_world = matrix
        obj.hide_render = False
        return obj

    def perch(self, matrix, rnd):
        """A perched crow on a socket's matrix, its head turned a little."""
        out = [self._copy(self.perched, matrix)]
        if self.head is not None:
            turn = Matrix.Rotation(math.radians(rnd.uniform(-55.0, 55.0)), 4, 'Z') @ Matrix.Rotation(math.radians(rnd.uniform(-10, 15)), 4, 'X')
            out.append(self._copy(self.head, matrix @ Matrix.Translation(self.head_at) @ turn))
        return out

    def fly(self, matrix, up=14.0, bend=8.0):
        """A gliding crow (the fauna's flying body and wings) at matrix."""
        if self.flying is None:
            return []
        out = [self._copy(self.flying, matrix)]
        for side, sock in ((1, 'L'), (-1, 'R')):
            at = self.sockets['Crow_Flying']['Wing' + sock]
            arm = matrix @ Matrix.Translation(at) @ Matrix.Rotation(-math.radians(up) * side, 4, 'Y')
            out.append(self._copy(self.wings['Crow_WingInner' + sock], arm))
            wrist = self.sockets['Crow_WingInner' + sock]['Wrist']
            out.append(self._copy(self.wings['Crow_WingOuter' + sock], arm @ Matrix.Translation(wrist)
                                  @ Matrix.Rotation(-math.radians(bend) * side, 4, 'Y')))
        return out


# --- The beacon stand-in ---

def _glow_material(name, colour, strength, fade=None, edge=2.0):
    """Additive light: a transparent surface plus emission, brightest face-on and fading at its edges (and upward over
    fade metres in the object's own Z, for the column)."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    add = nodes.new('ShaderNodeAddShader')
    clear = nodes.new('ShaderNodeBsdfTransparent')
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs['Color'].default_value = lt.hex_color(colour)
    weight = nodes.new('ShaderNodeLayerWeight')
    weight.inputs['Blend'].default_value = 0.5
    face = nodes.new('ShaderNodeMath')
    face.operation = 'SUBTRACT'
    face.inputs[0].default_value = 1.0
    links.new(weight.outputs['Facing'], face.inputs[1])
    soft = nodes.new('ShaderNodeMath')
    soft.operation = 'POWER'
    links.new(face.outputs['Value'], soft.inputs[0])
    soft.inputs[1].default_value = edge
    level = soft.outputs['Value']
    if fade:
        coords = nodes.new('ShaderNodeTexCoord')
        split = nodes.new('ShaderNodeSeparateXYZ')
        links.new(coords.outputs['Object'], split.inputs['Vector'])
        span = nodes.new('ShaderNodeMapRange')
        span.inputs['From Min'].default_value = 0.0
        span.inputs['From Max'].default_value = fade
        span.inputs['To Min'].default_value = 1.0
        span.inputs['To Max'].default_value = 0.0
        links.new(split.outputs['Z'], span.inputs['Value'])
        curve = nodes.new('ShaderNodeMath')
        curve.operation = 'POWER'
        links.new(span.outputs['Result'], curve.inputs[0])
        curve.inputs[1].default_value = 1.6
        # Fading in over its first metre too, so it rises out of the flame rather than starting at a rim.
        rise = nodes.new('ShaderNodeMapRange')
        rise.inputs['From Min'].default_value = 0.0
        rise.inputs['From Max'].default_value = 1.2
        links.new(split.outputs['Z'], rise.inputs['Value'])
        both = nodes.new('ShaderNodeMath')
        both.operation = 'MULTIPLY'
        links.new(level, both.inputs[0])
        links.new(curve.outputs['Value'], both.inputs[1])
        three = nodes.new('ShaderNodeMath')
        three.operation = 'MULTIPLY'
        links.new(both.outputs['Value'], three.inputs[0])
        links.new(rise.outputs['Result'], three.inputs[1])
        level = three.outputs['Value']
    scale = nodes.new('ShaderNodeMath')
    scale.operation = 'MULTIPLY'
    links.new(level, scale.inputs[0])
    scale.inputs[1].default_value = strength
    links.new(scale.outputs['Value'], emit.inputs['Strength'])
    links.new(clear.outputs['BSDF'], add.inputs[0])
    links.new(emit.outputs['Emission'], add.inputs[1])
    links.new(add.outputs['Shader'], out.inputs['Surface'])
    return mat


def _flame_material(name, colour, strength):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(colour)
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(colour)
    bsdf.inputs['Emission Strength'].default_value = strength
    return mat


def _mesh(name, bm, mat):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for p in mesh.polygons:
        p.use_smooth = True
    mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def beacon(at, colour, core, height=34.0, scale=1.0, column=True):
    """The beacon stand-in burning at `at` (a roost's SOCKET_Beacon): flames, a halo and the column."""
    objs = []
    at = Vector(at)
    rnd = random.Random(int(at.x * 100 + at.y * 10))
    # Flames: a few teardrops leaning together, the core's colour inside the column's.
    for k, (dx, dy, h, r, hot) in enumerate(((0.0, 0.0, 0.42, 0.1, True), (0.06, 0.04, 0.3, 0.07, False), (-0.06, 0.03, 0.32, 0.07, False),
                                             (0.02, -0.07, 0.28, 0.065, False), (-0.03, -0.04, 0.22, 0.06, True))):
        bm = bmesh.new()
        prof = [(0.0, 0.0), (r * 0.7, h * 0.08), (r, h * 0.25), (r * 0.75, h * 0.5), (r * 0.35, h * 0.78), (0.0, h)]
        verts = []
        for j, (pr, pz) in enumerate(prof):
            ring = []
            for s in range(10):
                a = 2.0 * math.pi * s / 10
                ring.append(bm.verts.new((math.cos(a) * pr, math.sin(a) * pr, pz)) if pr > 0 else None)
            verts.append(ring if prof[j][0] > 0 else bm.verts.new((0.0, 0.0, pz)))
        for j in range(len(prof) - 1):
            a, b = verts[j], verts[j + 1]
            for s in range(10):
                s1 = (s + 1) % 10
                if isinstance(a, list) and isinstance(b, list):
                    bm.faces.new((a[s], a[s1], b[s1], b[s]))
                elif isinstance(a, list):
                    bm.faces.new((a[s], a[s1], b))
                else:
                    bm.faces.new((a, b[s1], b[s]))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        mat = _flame_material(f'_Flame{k}', core if hot else colour, 12.0 if hot else 8.0)
        obj = _mesh(f'_Flame{k}', bm, mat)
        obj.matrix_world = (Matrix.Translation(at + Vector((dx, dy, -0.16)) * scale) @ Matrix.Rotation(math.radians(rnd.uniform(-8, 8)), 4, 'X')
                            @ Matrix.Diagonal((scale, scale, scale, 1.0)))
        objs.append(obj)
    # The halo round the basket.
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=24, v_segments=14, radius=0.9 * scale)
    halo = _mesh('_Halo', bm, _glow_material('_HaloGlow', colour, 1.4, edge=3.0))
    halo.scale = (0.6, 0.6, 0.6)
    halo.location = at + Vector((0.0, 0.0, 0.05))
    objs.append(halo)
    if column:
        for k, (r0, r1, strength, edge) in enumerate(((0.55, 1.05, 1.8, 3.0), (0.16, 0.3, 2.6, 2.0))):
            bm = bmesh.new()
            bmesh.ops.create_cone(bm, segments=32, radius1=r0 * scale, radius2=r1 * scale, depth=height, cap_ends=False,
                                  matrix=Matrix.Translation((0.0, 0.0, height * 0.5)))
            obj = _mesh(f'_Column{k}', bm, _glow_material(f'_ColumnGlow{k}', colour, strength, fade=height, edge=edge))
            obj.location = at - Vector((0.0, 0.0, 0.1))
            objs.append(obj)
    return objs


# --- Light ---

DUSK_SKY = [(0.0, 0x0b0c10), (0.485, 0x1e1a20), (0.5, 0x9a5a44), (0.512, 0x6e4a52), (0.55, 0x3e3e5c), (0.68, 0x232a48),
            (1.0, 0x0c1024)]
DUSK_GLOWS = [(0xff8a4a, 6.0, 0.4), (0xffc890, 60.0, 1.0)]
# Off to the west of the view, so the afterglow sits at the frame's edge as it does over Gravewind Canyon.
DUSK_SUN = Vector((-1.0, 0.4, 0.0))


def golden(scene, sun, toward):
    homes.light(scene, sun, toward)
    _compositor(scene, False)


def dusk(scene, sun, toward):
    """Dusk: the sun just down behind the far side, a warm band on the horizon, a deep blue overhead."""
    t = Vector(toward).normalized()
    sun.data.energy, sun.data.color, sun.data.angle = 0.9, (1.0, 0.5, 0.32), math.radians(4.0)
    sun.rotation_euler = (-t).to_track_quat('-Z', 'Y').to_euler()
    scene.world = homes.sky('_Dusk', DUSK_SKY, Vector((t.x, t.y, 0.02)), DUSK_GLOWS)
    _compositor(scene, True)


def _compositor(scene, glare):
    """The compositor's glare stands in for the game's bloom at dusk."""
    scene.use_nodes = True
    tree = scene.node_tree
    tree.nodes.clear()
    layers = tree.nodes.new('CompositorNodeRLayers')
    comp = tree.nodes.new('CompositorNodeComposite')
    if glare:
        g = tree.nodes.new('CompositorNodeGlare')
        g.glare_type = 'FOG_GLOW'
        g.quality = 'HIGH'
        g.threshold = 0.9
        g.size = 8
        tree.links.new(layers.outputs['Image'], g.inputs['Image'])
        tree.links.new(g.outputs['Image'], comp.inputs['Image'])
    else:
        tree.links.new(layers.outputs['Image'], comp.inputs['Image'])


# --- Placing a roost for a shot ---

def socket_matrices(obj):
    return {c.name[7:].split('.')[0]: c.matrix_local.copy() for c in obj.children if c.name.startswith('SOCKET_')}


class Shot:
    """Objects added for one shot (removed with clear())."""

    def __init__(self):
        self.objs = []

    def roost(self, obj, bell, at, yaw, flock=None, perches=(1, 2, 4, 5, 6), lit=None, column=True, seed=0,
              beacon_scale=1.0, height=34.0):
        """A copy of a roost at `at`, turned yaw degrees, its bell on its socket, the flock's crows on some perches, and
        (lit: (colour, core)) its beacon burning."""
        world = Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
        copy = obj.copy()
        copy.name = '_Shot' + obj.name
        bpy.context.scene.collection.objects.link(copy)
        copy.matrix_world = world
        copy.hide_render = False
        self.objs.append(copy)
        sockets = socket_matrices(obj)
        b = bell.copy()
        b.name = '_ShotBell'
        bpy.context.scene.collection.objects.link(b)
        b.matrix_world = world @ Matrix.Translation(sockets['Bell'].translation) @ Matrix.Rotation(math.radians(6.0), 4, 'Y')
        b.hide_render = False
        self.objs.append(b)
        if flock is not None:
            rnd = random.Random(seed)
            for k in perches:
                self.objs += flock.perch(world @ sockets[f'Perch_{k}'], rnd)
        if lit:
            self.objs += beacon(world @ sockets['Beacon'].translation, lit[0], lit[1], height=height, column=column,
                                scale=beacon_scale)
        return copy, world, sockets

    def add(self, objs):
        self.objs += list(objs)

    def clear(self):
        for o in self.objs:
            bpy.data.objects.remove(o, do_unlink=True)
        self.objs = []


def figure_at(human, at, yaw=0.0):
    human.location = Vector(at)
    human.rotation_euler = (0.0, 0.0, math.radians(yaw))
    human.hide_render = False


# --- The valley (for the far beacons) ---

def valley():
    """A dusk valley 1 km across: the floor running north from the camera, ridges east and west with pines on them,
    a knoll at (0, 130) for the roost. Returns the ground, the pines, and a height function."""
    def height(x, y):
        ridge = 70.0 * max(0.0, min(1.0, (abs(x) - 90.0) / 260.0)) ** 1.4
        far = 40.0 * max(0.0, min(1.0, (y - 420.0) / 420.0)) ** 1.2
        roll = 3.5 * noise.noise(Vector((x * 0.012, y * 0.012, 0.3))) + 1.2 * noise.noise(Vector((x * 0.05, y * 0.05, 2.0)))
        knoll = 4.5 * math.exp(-((x / 22.0) ** 2 + ((y - 130.0) / 22.0) ** 2))
        return ridge + far + roll + knoll
    size, n = 1100.0, 140
    bm = bmesh.new()
    verts = []
    for j in range(n + 1):
        row = []
        for i in range(n + 1):
            x = -size * 0.5 + size * i / n
            y = -150.0 + size * j / n
            row.append(bm.verts.new((x, y, height(x, y))))
        verts.append(row)
    for j in range(n):
        for i in range(n):
            bm.faces.new((verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i]))
    mat = bpy.data.materials.new('_ValleyGround')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(nd for nd in nodes if nd.type == 'BSDF_PRINCIPLED')
    coords = nodes.new('ShaderNodeTexCoord')
    tex = nodes.new('ShaderNodeTexNoise')
    tex.inputs['Scale'].default_value = 0.02
    tex.inputs['Detail'].default_value = 4.0
    links.new(coords.outputs['Object'], tex.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position, ramp.color_ramp.elements[0].color = 0.35, lt.hex_color(0x5d5a34)
    ramp.color_ramp.elements[1].position, ramp.color_ramp.elements[1].color = 0.65, lt.hex_color(0x8a7448)
    links.new(tex.outputs['Fac'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.95
    ground = _mesh('_Valley', bm, mat)
    # Pines on the ridges' flanks.
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, segments=7, radius1=3.2, radius2=0.0, depth=14.0, cap_ends=True, matrix=Matrix.Translation((0.0, 0.0, 9.0)))
    bmesh.ops.create_cone(bm, segments=5, radius1=0.35, radius2=0.25, depth=3.0, cap_ends=True, matrix=Matrix.Translation((0.0, 0.0, 1.5)))
    pmat = bpy.data.materials.new('_Pine')
    pmat.use_nodes = True
    pb = next(nd for nd in pmat.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
    pb.inputs['Base Color'].default_value = lt.hex_color(0x26341f)
    pine = _mesh('_PineT', bm, pmat)
    pine.hide_render = True
    pines = []
    rnd = random.Random(7)
    for k in range(150):
        side = -1.0 if k % 2 else 1.0
        x = side * rnd.uniform(110.0, 420.0)
        y = rnd.uniform(60.0, 900.0)
        p = pine.copy()
        p.name = '_Pine'
        bpy.context.scene.collection.objects.link(p)
        s = rnd.uniform(0.7, 1.3)
        p.matrix_world = Matrix.Translation((x, y, height(x, y) - 0.5)) @ Matrix.Diagonal((s, s, s * rnd.uniform(0.9, 1.2), 1.0))
        p.hide_render = False
        pines.append(p)
    for k in range(18):
        # A few on the valley floor near the knoll, for scale.
        x, y = rnd.uniform(-70.0, 70.0), rnd.uniform(70.0, 300.0)
        if abs(x) < 14.0 and abs(y - 130.0) < 20.0:
            continue
        p = pine.copy()
        p.name = '_Pine'
        bpy.context.scene.collection.objects.link(p)
        s = rnd.uniform(0.6, 1.0)
        p.matrix_world = Matrix.Translation((x, y, height(x, y) - 0.5)) @ Matrix.Diagonal((s, s, s, 1.0))
        p.hide_render = False
        pines.append(p)
    return ground, pines, height


# --- The renders ---

LINEUP = (2400, 1000)
TILE = (1600, 900)
FAR60 = (384, 160)          # 1080p's pixels at the game's 90 degree view: lens 18 mm x 1920 / 384
FAR60_LENS = 18.0 * 1920.0 / FAR60[0]
SPACING = 4.4               # between the directions in the lineups (m)


def circling(center, radius, heights, seed):
    """Matrices for crows gliding round a point: facing along their circle, banked into it."""
    rnd = random.Random(seed)
    out = []
    for k, z in enumerate(heights):
        a = 2.0 * math.pi * (k / len(heights)) + rnd.uniform(-0.4, 0.4)
        p = Vector(center) + Vector((math.cos(a) * radius, math.sin(a) * radius, z))
        tangent = Vector((-math.sin(a), math.cos(a), 0.0))
        yaw = math.atan2(tangent.x, -tangent.y)
        out.append(Matrix.Translation(p) @ Matrix.Rotation(yaw, 4, 'Z') @ Matrix.Rotation(math.radians(-18.0), 4, 'Y'))
    return out


def screen_x(scene, point):
    return world_to_camera_view(scene, scene.camera, Vector(point)).x


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def render(built, bell, titles, recommended):
    """Every render and the look sheet. built: {direction: (roost object, Roost)}."""
    os.makedirs(OUT, exist_ok=True)
    scene, sun = homes.stage(samples=48)
    ground = bpy.data.objects['_Ground']
    fill = bpy.data.objects.new('_Fill', bpy.data.lights.new('_Fill', 'SUN'))
    scene.collection.objects.link(fill)
    fill.data.energy, fill.data.color = 0.0, (0.55, 0.65, 1.0)
    fill.rotation_euler = (math.radians(50.0), 0.0, math.radians(200.0))
    human = homes.figure()
    human.hide_render = True
    for key, (obj, _) in built.items():
        obj.hide_render = True
    bell.hide_render = True
    crows = Crows()
    shot = Shot()
    keys = sorted(built)
    pngs, labels = {}, {}
    view = Vector((-0.55, -1.0, 0.22))
    ember = next(c for c in COLOURS if c[0] == RECOMMENDED)
    rec_obj = built[recommended][0]

    def light_golden():
        fill.data.energy = 0.0
        golden(scene, sun, homes.golden_sun(38.0, 18.0))

    def light_dusk(toward):
        fill.data.energy = 0.25
        dusk(scene, sun, toward)

    # Each direction alone, golden, three-quarter.
    for key in keys:
        shot.roost(built[key][0], bell, (0.0, 0.0, 0.0), 0.0, flock=crows, seed=3)
        figure_at(human, (1.1, -1.4, 0.0), 20.0)
        light_golden()
        eye, target = homes.frame_shot([o for o in shot.objs if o.type == 'MESH'] + [human], view, TILE, 35.0, margin=1.06,
                                       height_bias=0.45)
        pngs[f'Dir_{key}'] = homes.shoot(scene, out_path(f'Roost_{key}_golden'), eye, target, 35.0, TILE, -0.1)
        shot.clear()
    human.hide_render = True
    if '--quick' in ARGS:
        return pngs

    # The lineups: the three at one scale, golden (not yet found) and at dusk (found, burning).
    places = {key: (SPACING * (k - 1), 0.0, 0.0) for k, key in enumerate(keys)}
    for state in ('golden', 'dusk'):
        for k, key in enumerate(keys):
            shot.roost(built[key][0], bell, places[key], 0.0, flock=crows, seed=11 + k, perches=(1, 2, 3, 5, 6),
                       lit=(ember[2], ember[3]) if state == 'dusk' else None, height=40.0)
        figure_at(human, (SPACING * 0.5, -1.3, 0.0), 10.0)
        framed = [o for o in shot.objs if o.type == 'MESH' and not o.name.startswith(('_Column', '_Halo'))] + [human]
        if state == 'golden':
            light_golden()
            exposure = -0.1
        else:
            light_dusk(DUSK_SUN)
            exposure = 0.3
        eye, target = homes.frame_shot(framed, (-0.18, -1.0, 0.12), LINEUP, 32.0, margin=1.05, height_bias=0.5)
        pngs[f'Lineup_{state}'] = homes.shoot(scene, out_path(f'Lineup_{state}'), eye, target, 32.0, LINEUP, exposure)
        if state == 'golden':
            labels['lineup'] = {key: screen_x(scene, places[key]) for key in keys}
        shot.clear()
    human.hide_render = True

    # The 60 m test: the three 60 m off, as a 1080p player sees them.
    places60 = {key: (7.0 * (k - 1), 60.0, 0.0) for k, key in enumerate(keys)}
    for state in ('golden', 'dusk'):
        for k, key in enumerate(keys):
            shot.roost(built[key][0], bell, places60[key], 0.0, flock=crows, seed=21 + k, perches=(1, 2, 3, 5, 6),
                       lit=(ember[2], ember[3]) if state == 'dusk' else None, height=40.0)
        if state == 'golden':
            light_golden()
            exposure = -0.1
        else:
            light_dusk(DUSK_SUN)
            exposure = 0.3
        pngs[f'Far60_{state}'] = homes.shoot(scene, out_path(f'Far60_{state}'), (0.0, 0.0, 1.6), (0.0, 60.0, 3.3),
                                             FAR60_LENS, FAR60, exposure)
        if state == 'golden':
            labels['far60'] = {key: screen_x(scene, places60[key]) for key in keys}
        shot.clear()

    # The recommendation close up: its head, and its foot from where Ellis wakes.
    _, world, sockets = shot.roost(rec_obj, bell, (0.0, 0.0, 0.0), 0.0, flock=crows, seed=31, perches=(3, 4, 5, 6))
    light_golden()
    pngs['Detail_head'] = homes.shoot(scene, out_path('Detail_head'), (-2.2, -3.4, 3.1), (0.1, 0.1, 3.75), 45.0, TILE, -0.1)
    shot.clear()
    shot.roost(rec_obj, bell, (0.0, 0.0, 0.0), 0.0, flock=crows, seed=32, perches=(1, 2))
    pngs['Detail_foot'] = homes.shoot(scene, out_path('Detail_foot'), (1.05, -3.0, 1.5), (0.14, 0.0, 1.78), 32.0, TILE, -0.1)
    shot.clear()

    # The beacon colours on the recommendation: near at dusk, and far over a dusk valley.
    for name, label, colour, core, note in COLOURS:
        _, world, sockets = shot.roost(rec_obj, bell, (0.0, 0.0, 0.0), 0.0, flock=crows, seed=41, perches=(1, 2, 3, 5),
                                       lit=(colour, core), height=40.0)
        beacon_at = world @ sockets['Beacon'].translation
        for m in circling(beacon_at, 1.9, (1.2, 2.0, 2.8), 5):
            shot.add(crows.fly(m))
        figure_at(human, (0.3, -1.7, 0.0), 0.0)
        light_dusk(DUSK_SUN)
        pngs[f'Near_{name}'] = homes.shoot(scene, out_path(f'Near_{name}'), (-4.6, -9.6, 2.0), (0.0, 0.0, 3.4), 26.0, TILE, 0.35)
        shot.clear()
    human.hide_render = True
    ground.hide_render = True
    valley_ground, pines, height = valley()
    knoll = Vector((0.0, 130.0, height(0.0, 130.0) - 0.05))
    eye = Vector((-10.0, 0.0, height(-10.0, 0.0) + 1.7))
    for name, label, colour, core, note in COLOURS:
        shot.roost(rec_obj, bell, tuple(knoll), 20.0, flock=crows, seed=51, perches=(1, 2, 5), lit=(colour, core), height=45.0)
        light_dusk(DUSK_SUN)
        pngs[f'Far_{name}'] = homes.shoot(scene, out_path(f'Far_{name}'), tuple(eye), tuple(knoll + Vector((0.0, 0.0, 16.0))),
                                          30.0, TILE, 0.45)
        shot.clear()
    valley_ground.hide_render = True
    for p in pines:
        p.hide_render = True
    ground.hide_render = False

    tris = {key: triangles(built[key][0]) for key in keys}
    compose(pngs, labels, titles, tris, triangles(bell), recommended, built[recommended][1])
    return pngs


# --- The look sheet ---

LOOT = [('Common', 0xe7e7e7), ('Uncommon', 0x7ce77c), ('Rare', 0x7caaff), ('Epic', 0xcb7cf3), ('Legendary', 0xffcb00),
        ('UI accent', 0xff9f1c), ('UI lines', 0x5ac8ff)]


def compose(pngs, labels, titles, tris, bell_tris, recommended, rec_roost):
    """Saved/ArtPreviews/RansomsRest/LookSheets/41_Roost.jpg: 2000 px wide, every label large enough for a phone, on the
    look sheets' dark ground (looter_homes' sheet parts)."""
    width, pad, gap = 2000, 40, 18
    inner = width - 2 * pad
    half = (inner - gap) / 2
    scene = bpy.data.scenes.new('_Sheet')
    objs = []
    y = pad
    ORANGE, TEXT, DIM = 0xf2a64a, 0xe8e2d6, 0xc9c4ba

    def text(body, x, yy, size, color, bold=True, align='LEFT'):
        objs.append(homes._text(f't{len(objs)}', body, x, yy, size, color, bold=bold, align=align))

    def image(key, x, yy, w):
        img = bpy.data.images.load(pngs[key], check_existing=True)
        h = w * img.size[1] / img.size[0]
        objs.append(homes._image_plane(f'i{len(objs)}', pngs[key], x, yy, w, h))
        return h

    def lines(body, x, yy, size=38, step=48, color=TEXT):
        for k, line in enumerate(body.split('\n')):
            text(line, x, yy + size + step * k, size, color, bold=False)
        return step * len(body.split('\n'))

    def header(body):
        nonlocal y
        text(body, pad, y + 56, 56, ORANGE)
        y += 80

    text("Hob's roost: save, respawn and travel, on every map", pad, y + 72, 78, ORANGE)
    y += 100
    y += lines("The one point the player learns: found, it burns a beacon you see across the map, and Hob's crows\n"
               "carry Ellis between roosts and back to the nearest after a death. Blender previews; nothing imported.",
               pad, y, 38, 50, DIM) + 16

    header('Three directions at one scale: the golden afternoon, not yet found')
    h = image('Lineup_golden', pad, y, inner)
    for key, fx in labels['lineup'].items():
        mark = '  (recommended)' if key == recommended else ''
        text(f'{key}  {titles[key]}{mark}', pad + inner * fx, y + h + 54, 44, ORANGE, align='CENTER')
        text(f'{tris[key]:,} triangles', pad + inner * fx, y + h + 100, 36, DIM, bold=False, align='CENTER')
    y += h + 124
    y += lines("A: a hewn post, a forged crook, the fire basket hung from it (the keepers' lantern crook, grown).\n"
               "B: a mast, its yard lashed cockbilled (tilted, as ships mourn), a stork's nest, a fire basket on top.\n"
               "C: a dead oak cut back to perches, the nest in its fork, the basket spiked on the tallest leader.",
               pad, y) + 26

    header("At dusk, found: the beacon burning (Hob's Ember here)")
    h = image('Lineup_dusk', pad, y, inner)
    y += h + 12
    y += lines("The beacon stands in for the game's: a flame in the basket, a halo, a soft column fading upward (all\n"
               "emissive or additive; it lights nothing around it, as in the game on Medium). Crows on their perches.",
               pad, y) + 26

    header('From 60 m, as a 1080p player sees it (the 90 degree view, its pixels shown 2.5x)')
    h = image('Far60_golden', pad, y, half)
    image('Far60_dusk', pad + half + gap, y, half)
    for key, fx in labels['far60'].items():
        for x0 in (pad, pad + half + gap):
            text(key, x0 + half * fx, y + h - 14, 40, 0xffffff, align='CENTER')
    y += h + 12
    y += lines('By day the tilted yard and the nest set B apart (A reads as a lamp, C as a dead tree); at dusk\n'
               'only the beacons carry, so its colour and the column are what the player learns from afar.',
               pad, y) + 26

    header(f'{recommended}  {titles[recommended]}: close up')
    h = image('Detail_head', pad, y, half)
    image('Detail_foot', pad + half + gap, y, half)
    y += h + 12
    lines('The head: the cresset (the code lights it), the\ncrosstree, the nest on its bed round the topmast,\n'
          'black feathers in the sticks and lashings.', pad, y)
    lines('The foot, from where Ellis wakes: the crow\'s-foot\nmark burnt in a board (the same on every roost, and\n'
          f'a map icon), the horseshoe, the bell ({bell_tris} tris).', pad + half + gap, y)
    y += 3 * 48 + 26

    header('The beacon colour: pick one')
    # The colours already taken, as swatches.
    text('Taken:', pad, y + 40, 38, DIM, bold=False)
    x = pad + 140
    for name, colour in LOOT:
        objs.append(homes._rect(f'sw{len(objs)}', x, y + 6, 40, 40, colour, z=0.2))
        text(name, x + 50, y + 40, 32, DIM, bold=False)
        x += 50 + 18 * len(name) + 46
    y += 70
    for name, label, colour, core, note in COLOURS:
        objs.append(homes._rect(f'sw{len(objs)}', pad, y + 10, 50, 50, colour, z=0.2))
        text(label, pad + 66, y + 54, 50, ORANGE)
        y += 74
        h = image(f'Near_{name}', pad, y, half)
        image(f'Far_{name}', pad + half + gap, y, half)
        y += h + 10
        y += lines(note, pad, y, 36, 44) + 22

    height = int(y + pad - 10)
    objs.append(homes._rect('ground', 0, 0, width, height, 0x16171a))
    for o in objs:
        scene.collection.objects.link(o)
    cam = bpy.data.objects.new('_SheetCam', bpy.data.cameras.new('_SheetCam'))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = width
    cam.data.sensor_fit = 'HORIZONTAL'
    cam.location = (width * 0.5, -height * 0.5, 100.0)
    cam.data.clip_start, cam.data.clip_end = 1.0, 500.0
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 16
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'None'
    world = bpy.data.worlds.new('_SheetWorld')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(0x16171a)
    scene.world = world
    scene.render.image_settings.file_format = 'JPEG'
    scene.render.image_settings.quality = 90
    scene.render.image_settings.color_mode = 'RGB'
    os.makedirs(os.path.dirname(SHEET), exist_ok=True)
    scene.render.filepath = SHEET
    bpy.ops.render.render(write_still=True, scene=scene.name)
    lt._log(f'roost: look sheet {SHEET} ({width} x {height})')
    return SHEET
