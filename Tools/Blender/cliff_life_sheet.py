"""Look sheet 43, "Cliff life": Saved/ArtPreviews/RansomsRest/LookSheets/43_CliffLife.jpg, about 2000 px wide for the
user's phone, and its renders in Saved/ArtPreviews/RansomsRest/CliffLife/.

It builds the pieces from their scripts (Art/Models/Vegetation/CliffLife.py, Art/Models/Rocks/CliffScree.py) and stands
them on the cliff kit as Ransom's Rest has it: the kit's own exported faces (Intermediate/ArtExport_RR_CliffPanels:
CliffFace_C as a 6 m band, CliffFace_D as a buttress beside it, CliffFace_A as the band above, a bench of ground between)
in the area's rock tint (MI_RockCliff_Ransom's), its golden late-afternoon light, the slopes' own dry tufts on the
ground. The stains are the atlas (Art/Textures/CliffStains/T_CliffStains_BC.png) laid on the faces as a decal is: the
faces' triangles in each decal's box, copied a hair proud, mapped onto its cell.

Every spot is found on the faces' own meshes the way a placing script can find them in the level (ray casts; see
find_* below): ledges (up-facing rock in the face, room above and in front), the lip line (the front-most point of a
band's top, the face under it no further forward), the wall's foot (the face's front at knee height), crack spots (a
joint: the face locally deepest along a row), an overhang's inner corner (rock overhead within 0.6 m, the back wall
behind). Then the whole kit again on plain stand-in blocks, in order, for the sheet's labels. Nothing here is exported:
every object it makes is named '_...' or is a copy.

It needs the cliff faces' export (Tools\\models.ps1 -Only Cliffs -NoImport -Out Intermediate\\ArtExport_RR_CliffPanels, as
the panels' work left it) and the stains' atlas.

    Tools\\artrun.ps1 -Script Tools\\Blender\\cliff_life_sheet.py            (after make_cliff_stains.py)
    -ScriptArgs --quick                                                      the banner only
"""
import importlib.util
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_homes as homes
import looter_textures as lt

ARGS = homes.args()
OUT = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'CliffLife')
SHEET = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets', '43_CliffLife.jpg')
CLIFFS = os.path.join(lt.REPO, 'Intermediate', 'ArtExport_RR_CliffPanels')
STAINS = os.path.join(lt.REPO, 'Art', 'Textures', 'CliffStains')
STAIN_SHEET = os.path.join(lt.PREVIEW_DIR, 'CliffStains', 'CliffStains_sheet.png')
# MI_RockCliff_Ransom's Tint (0.66, 0.58, 0.5 linear; Art/Levels/RansomsRest/layout.json) as an sRGB hex.
RANSOM_TINT = 0xd3c8bc
SUN = (52.0, 17.0)          # from the front-right, low: the relief rakes
UP = Vector((0.0, 0.0, 1.0))
BENCH_Z = 4.3               # the ground on the bench behind the lower band's top (under its broken top)
SHOT = (1600, 1000)
WIDE = (2400, 1100)
KIT = (2400, 900)


def load(path, name):
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# --- The set ---

def import_cliff(name, location, yaw, rock):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(CLIFFS, f'SM_{name}.fbx'), colors_type='LINEAR')
    new = [o for o in bpy.data.objects if o not in before]
    face = None
    for o in new:
        if o.name.startswith('UCX_'):
            bpy.data.objects.remove(o)
            continue
        face = o
    face.name = '_' + name
    face.location = location
    face.rotation_euler = (0.0, 0.0, math.radians(yaw))
    face.data.materials.clear()
    face.data.materials.append(rock)
    for p in face.data.polygons:
        p.material_index = 0
    return face


def plane(name, x0, x1, y0, y1, z, mat):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)], [], [(0, 1, 2, 3)])
    mesh.uv_layers.new(name='UVMap')
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    lt.assign(obj, mat)
    lt.box_uv(obj, 'GroundDirt', texel_density=160)
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    return obj


class Rocks:
    """Ray casts against the cliff faces in the world."""

    def __init__(self, faces):
        verts, polys = [], []
        for face in faces:
            matrix = face.matrix_world
            base = len(verts)
            verts.extend(matrix @ v.co for v in face.data.vertices)
            polys.extend([base + i for i in p.vertices] for p in face.data.polygons)
        self.tree = BVHTree.FromPolygons(verts, polys, epsilon=0.0)

    def cast(self, origin, direction, distance=50.0):
        hit, normal, _, dist = self.tree.ray_cast(Vector(origin), Vector(direction).normalized(), distance)
        return (hit, normal, dist) if hit is not None else (None, None, None)


def find_lips(rocks, x0, x1, top_low, top_high, y_max, step=0.1):
    """The lip line along a band: at each x the front-most point of the band's top (a ray down from above, walking
    back from in front no further than y_max, first lands on the top between top_low and top_high)."""
    lips = []
    x = x0
    while x <= x1:
        for k in range(int((y_max + 4.0) / 0.05)):
            y = -4.0 + k * 0.05
            hit, normal, _ = rocks.cast((x, y, top_high + 0.5), (0.0, 0.0, -1.0))
            if hit is not None and hit.z > top_low and normal.z > 0.5:
                lips.append(hit)
                break
        x += step
    return lips


def face_below_clear(rocks, lip, drop=(0.4, 1.0, 1.6), proud=0.22):
    """True when the face under a lip stands no further forward than proud in front of it (a drape hangs clear)."""
    for d in drop:
        hit, _, _ = rocks.cast((lip.x, lip.y - 5.0, lip.z - d), (0.0, 1.0, 0.0), 10.0)
        if hit is not None and hit.y < lip.y - proud:
            return False
    return True


def straight_run(lips, start, width, tolerance=0.25):
    """Where a drape width wide fits the lip round start: the lip points over its width, most of them (80%) within
    tolerance of their median in y and 0.35 m in z, give its pivot (start's x, the medians); None on a step or a notch.
    (A drape a little proud of a lip point behind the median hangs clear; one behind it would sink in.)"""
    run = [p for p in lips if start.x - width * 0.5 <= p.x <= start.x + width * 0.5]
    if len(run) < width / 0.1 * 0.8:
        return None
    ys = sorted(p.y for p in run)
    zs = sorted(p.z for p in run)
    y, z = ys[len(ys) // 2], zs[len(zs) // 2]
    near = [p for p in run if abs(p.y - y) <= tolerance and abs(p.z - z) <= 0.35]
    if len(near) < len(run) * 0.8 or min(p.y for p in run) < y - tolerance - 0.1:
        return None
    return Vector((start.x, y, z))


def find_ledges(rocks, x0, x1, z0, z1, room):
    """Up-facing rock in a face (rays from in front, 50 degrees down), with room above (room m) and in front."""
    spots = []
    x = x0
    while x <= x1:
        z = z0
        while z <= z1:
            origin = Vector((x, -6.0, z + 6.0 * 1.19))
            hit, normal, _ = rocks.cast(origin, (0.0, 0.64, -0.77), 20.0)
            if hit is not None and normal.z > 0.8 and z0 - 0.5 < hit.z < z1 + 0.5:
                above, _, _ = rocks.cast(hit + UP * 0.05, UP, room)
                front, _, _ = rocks.cast(hit + UP * 0.25, (0.0, -1.0, 0.0), 1.0)
                if above is None and front is None:
                    spots.append((hit, normal))
            z += 0.2
        x += 0.2
    return spots


def find_cracks(rocks, x0, x1, z, step=0.05):
    """Joints along a row at height z: where the face (rays straight in) is deepest against its neighbours either
    side within 0.3 m, by at least 8 cm. Returns (point, outward normal) each."""
    row = []
    x = x0
    while x <= x1:
        hit, normal, _ = rocks.cast((x, -6.0, z), (0.0, 1.0, 0.0), 12.0)
        row.append((x, hit, normal))
        x += step
    spots = []
    reach = int(round(0.3 / step))
    for i in range(reach, len(row) - reach):
        x, hit, normal = row[i]
        if hit is None:
            continue
        neighbours = [row[j][1] for j in range(i - reach, i + reach + 1) if j != i and row[j][1] is not None]
        if len(neighbours) < reach:
            continue
        if all(hit.y >= n.y for n in neighbours) and hit.y - min(n.y for n in neighbours) > 0.08:
            spots.append((hit, normal))
    return spots


def find_foot(rocks, x, z=0.3):
    hit, normal, _ = rocks.cast((x, -8.0, z), (0.0, 1.0, 0.0), 16.0)
    return hit, normal


def find_overhangs(rocks, x0, x1, z0, z1, depth=0.25):
    """An overhang's inner corner: rock overhead within 0.6 m of a point just off the face, facing down, deep enough
    (still overhead depth m further out), and the back wall behind. Returns (corner, outward normal) each."""
    found = []
    x = x0
    while x <= x1:
        z = z0
        while z <= z1:
            wall, wn, _ = rocks.cast((x, -6.0, z), (0.0, 1.0, 0.0), 12.0)
            if wall is not None and wn.y < -0.5:
                at = wall - Vector((0.0, 0.06, 0.0))
                ceiling, cn, cd = rocks.cast(at, UP, 0.6)
                if ceiling is not None and cn.z < -0.6:
                    out, _, _ = rocks.cast(at - Vector((0.0, depth, 0.0)), UP, 0.8)
                    if out is not None:
                        corner, _, _ = rocks.cast(Vector((x, wall.y - 0.3, ceiling.z - 0.03)), (0.0, 1.0, 0.0), 1.0)
                        if corner is not None:
                            found.append((Vector((x, corner.y, ceiling.z)), wn))
            z += 0.1
        x += 0.1
    return found


def yaw_from(normal):
    """The turn that points a model's front (-Y) along normal, flattened."""
    return math.atan2(normal.x, -normal.y)


def place(model, location, yaw=0.0, tilt=None, scale=1.0):
    copy = model.copy()
    copy.data = model.data
    bpy.context.scene.collection.objects.link(copy)
    matrix = Matrix.Translation(location) @ Matrix.Rotation(yaw, 4, 'Z')
    if tilt is not None:
        matrix = matrix @ tilt
    copy.matrix_world = matrix @ Matrix.Scale(scale, 4)
    copy.hide_render = False
    return copy


# --- Stains: the atlas laid on the faces as a decal ---

def stain_material(cell):
    name = f'_Stain{cell}'
    mat = bpy.data.materials.get(name)
    if mat:
        return mat
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    uv = nodes.new('ShaderNodeUVMap')
    uv.uv_map = 'UVMap'
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(uv.outputs['UV'], split.inputs['Vector'])

    def math_node(op, a, b=None, clamp=False):
        node = nodes.new('ShaderNodeMath')
        node.operation = op
        node.use_clamp = clamp
        for index, value in enumerate((a, b)):
            if value is None:
                continue
            if isinstance(value, (int, float)):
                node.inputs[index].default_value = value
            else:
                links.new(value, node.inputs[index])
        return node.outputs['Value']
    u, v = split.outputs['X'], split.outputs['Y']
    cu = math_node('ADD', u, 0.0, clamp=True)
    cv = math_node('ADD', v, 0.0, clamp=True)
    atlas_u = math_node('DIVIDE', math_node('ADD', cu, float(cell)), 8.0)
    atlas_v = math_node('SUBTRACT', 1.0, cv)
    join = nodes.new('ShaderNodeCombineXYZ')
    links.new(atlas_u, join.inputs['X'])
    links.new(atlas_v, join.inputs['Y'])
    inside = math_node('MULTIPLY', math_node('MULTIPLY', math_node('GREATER_THAN', u, 0.0),
                                             math_node('LESS_THAN', u, 1.0)),
                       math_node('MULTIPLY', math_node('GREATER_THAN', v, 0.0), math_node('LESS_THAN', v, 1.0)))
    bc = nodes.new('ShaderNodeTexImage')
    bc.image = bpy.data.images.load(os.path.join(STAINS, 'T_CliffStains_BC.png'), check_existing=True)
    bc.extension = 'EXTEND'
    links.new(join.outputs['Vector'], bc.inputs['Vector'])
    orm = nodes.new('ShaderNodeTexImage')
    orm.image = bpy.data.images.load(os.path.join(STAINS, 'T_CliffStains_ORM.png'), check_existing=True)
    orm.image.colorspace_settings.name = 'Non-Color'
    orm.extension = 'EXTEND'
    links.new(join.outputs['Vector'], orm.inputs['Vector'])
    rough = nodes.new('ShaderNodeSeparateColor')
    links.new(orm.outputs['Color'], rough.inputs['Color'])
    links.new(rough.outputs['Green'], bsdf.inputs['Roughness'])
    # The rock's baked occlusion darkens the stain as it darkens the rock.
    vertex = nodes.new('ShaderNodeVertexColor')
    vertex.layer_name = 'Col'
    shade = nodes.new('ShaderNodeMix')
    shade.data_type = 'RGBA'
    shade.blend_type = 'MULTIPLY'
    lt._socket(shade.inputs, 'Factor_Float').default_value = 1.0
    links.new(bc.outputs['Color'], lt._socket(shade.inputs, 'A_Color'))
    links.new(vertex.outputs['Alpha'], lt._socket(shade.inputs, 'B_Color'))
    links.new(lt._socket(shade.outputs, 'Result_Color'), bsdf.inputs['Base Color'])
    links.new(math_node('MULTIPLY', bc.outputs['Alpha'], inside), bsdf.inputs['Alpha'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = True
    return mat


def stain(face, x, top, width, height, cell):
    """The faces of face (in the world) inside a decal's box (x across, top down height), facing out, copied 4 mm proud
    and mapped onto the atlas's cell (U across the box, V down it)."""
    matrix = face.matrix_world
    mesh = face.data
    col = mesh.color_attributes.get('Col')
    ao = [1.0] * len(mesh.loops)
    if col is not None and col.domain == 'CORNER':
        ao = [c.color[3] for c in col.data]
    x0, x1, z1, z0 = x - width * 0.5, x + width * 0.5, top, top - height
    verts, polys, uvs, alphas, index = [], [], [], [], {}
    normal_matrix = matrix.to_3x3().inverted_safe().transposed()
    for p in mesh.polygons:
        n = (normal_matrix @ p.normal).normalized()
        if n.y > -0.2:
            continue
        corners = [matrix @ mesh.vertices[i].co for i in p.vertices]
        if max(c.x for c in corners) < x0 - 0.05 or min(c.x for c in corners) > x1 + 0.05:
            continue
        if max(c.z for c in corners) < z0 - 0.05 or min(c.z for c in corners) > z1 + 0.05:
            continue
        face_verts = []
        for vi, c in zip(p.vertices, corners):
            if vi not in index:
                index[vi] = len(verts)
                verts.append(c + n * 0.004)
            face_verts.append(index[vi])
        polys.append(face_verts)
        for loop, c in zip(p.loop_indices, corners):
            uvs.append(((c.x - x0) / width, (z1 - c.z) / height))
            alphas.append(ao[loop])
    data = bpy.data.meshes.new(f'_Decal{cell}')
    data.from_pydata(verts, [], polys)
    uv = data.uv_layers.new(name='UVMap')
    for loop, value in zip(uv.data, uvs):
        loop.uv = value
    color = data.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    for loop, a in zip(color.data, alphas):
        loop.color = (1.0, 1.0, 1.0, a)
    for poly in data.polygons:
        poly.use_smooth = True
    data.materials.append(stain_material(cell))
    obj = bpy.data.objects.new(f'_Decal{cell}', data)
    bpy.context.scene.collection.objects.link(obj)
    obj.visible_shadow = False
    return obj


# --- The scene ---

def build_scene():
    life = load(os.path.join(lt.REPO, 'Art', 'Models', 'Vegetation', 'CliffLife.py'), 'cliff_life_models')
    scree = load(os.path.join(lt.REPO, 'Art', 'Models', 'Rocks', 'CliffScree.py'), 'cliff_scree_models')
    models = life.build() + scree.build()
    by = {m.name: m for m in models}
    tris = {m.name: triangles(m) for m in models}
    slots = {m.name: [mat.name for mat in m.data.materials] for m in models}
    rock = lt.material('RockCliff', name='_RockRansom', tint=RANSOM_TINT)
    for m in models:
        for k, mat in enumerate(m.data.materials):
            if mat.name == 'RockCliff':
                m.data.materials[k] = rock
    tufts = [life.S.dry_tuft_a(), life.S.dry_tuft_b(), life.S.sagebrush_c()]

    scene, sun = homes.stage(samples=64)
    homes.light(scene, sun, homes.golden_sun(*SUN))
    for o in list(scene.objects):
        if o.name.startswith('UCX_') or o in models or o in tufts or (o.parent in models):
            o.hide_render = True

    band = import_cliff('CliffFace_C', (0.0, 0.0, 0.0), 0.0, rock)
    buttress = import_cliff('CliffFace_D', (10.6, 0.9, 0.0), 14.0, rock)
    upper = import_cliff('CliffFace_A', (3.0, 11.0, BENCH_Z - 0.45), 0.0, rock)
    dirt = bpy.data.materials.get('_PreviewDirt')
    plane('_Bench', -4.0, 16.0, 1.6, 40.0, BENCH_Z, dirt)
    rocks = Rocks([band, buttress, upper])
    placed = {}

    def put(name, location, yaw=0.0, tilt=None):
        copy = place(by[name], location, yaw, tilt)
        placed.setdefault(name, []).append(copy)
        return copy

    # The lip line of the lower band: drapes where it runs straight and the face under it stands back.
    lips = find_lips(rocks, -7.0, 7.0, 4.0, 6.0, 1.3)
    used = []
    # (This band's top is broken into steps and notches: the 2.4 m drape finds no straight stretch on it.)
    for name, width in (('LipDrape_B', 2.0), ('LipDrape_A', 2.4), ('LipDrape_C', 1.2), ('LipDrape_C', 1.2)):
        best = None
        for p in lips:
            if any(abs(p.x - u) < (width + w) * 0.5 + 0.4 for u, w in used) or abs(p.x) > 7.2 - width * 0.5:
                continue
            pivot = straight_run(lips, p, width)
            if pivot is None or not face_below_clear(rocks, pivot):
                continue
            score = -abs(p.x + 1.0)
            if best is None or score > best[0]:
                best = (score, pivot)
        if best is None:
            print(f'LOOTER: sheet: no lip for {name}', flush=True)
            continue
        used.append((best[1].x, width))
        put(name, best[1])
    print(f'LOOTER: sheet: {len(lips)} lip points, drapes at {[round(u, 1) for u, _ in used]}', flush=True)

    # Ledges in the lower band's face: sage and grass; the ledge juniper where there is room for it.
    ledges = find_ledges(rocks, -7.0, 7.0, 1.0, 4.4, 1.0)
    print(f'LOOTER: sheet: {len(ledges)} ledge points', flush=True)
    taken = []
    rng = random.Random(43)
    plan = ['CliffJuniper_B', 'LedgeSage_A', 'LedgeGrass_A', 'LedgeSage_B', 'LedgeGrass_B', 'LedgeGrass_A',
            'LedgeSage_A', 'LedgeGrass_B']
    order = sorted(ledges, key=lambda s: (round(s[0].x * 3.0) * 7 + round(s[0].z * 5.0)) % 11)
    for name in plan:
        room = 2.5 if name.startswith('Cliff') else 0.8
        for hit, normal in order:
            if any((hit - t).length < 1.6 for t in taken):
                continue
            above, _, _ = rocks.cast(hit + UP * 0.05, UP, room)
            if above is not None:
                continue
            taken.append(hit)
            put(name, hit - UP * 0.02, rng.uniform(-0.3, 0.3))
            break

    # Cracks: the crack junipers on joints, one high, one low.
    for name, z, x0, x1 in (('CliffJuniper_A', 3.0, -4.8, -1.5), ('CliffJuniper_C', 2.3, 2.5, 6.0)):
        cracks = [c for c in find_cracks(rocks, x0, x1, z) if all((c[0] - t).length > 1.5 for t in taken)]
        if cracks:
            hit, normal = cracks[len(cracks) // 2]
            taken.append(hit)
            put(name, hit, yaw_from(normal))
        else:
            print(f'LOOTER: sheet: no crack for {name}', flush=True)

    # The foot: fans where the face meets the ground, a slab or two off it.
    for name, x in (('ScreeFan_A', -3.6), ('ScreeFan_C', 3.1), ('ScreeFan_B', 8.3)):
        hit, normal = find_foot(rocks, x)
        if hit is not None:
            put(name, Vector((hit.x, hit.y + 0.15, 0.0)), yaw_from(normal) * 0.5)
    put('FallenSlab_A', Vector((0.6, -4.2, 0.0)), math.radians(20.0))
    put('FallenSlab_B', Vector((6.3, -4.6, 0.0)), math.radians(-35.0))

    # Nests under an overhang, high on the band.
    overhangs = find_overhangs(rocks, -6.5, 6.5, 2.4, 5.0)
    print(f'LOOTER: sheet: {len(overhangs)} overhang corners', flush=True)
    nest_spots = []
    for corner, normal in sorted(overhangs, key=lambda o: -o[0].z):
        if len(nest_spots) == 2:
            break
        if any(abs(corner.x - n.x) < 2.0 for n in nest_spots) or any((corner - t).length < 1.0 for t in taken):
            continue
        if any(abs(corner.x - u) < w * 0.5 + 0.6 for u, w in used):     # not behind a drape
            continue
        nest_spots.append(corner)
        put('SwallowNests_A' if not nest_spots[1:] else 'SwallowNests_B', corner, yaw_from(normal))

    # Stains: dark varnish under the lips, a seep's bloom, whitewash under the nests.
    decals = []
    for x, width, height, cell in ((0.9, 2.0, 4.6, 0), (-2.0, 0.9, 4.4, 1), (-5.0, 1.2, 4.4, 2), (5.3, 1.4, 3.2, 3),
                                   (3.3, 2.0, 4.0, 4), (-3.6, 1.2, 3.4, 5), (6.6, 1.4, 4.0, 6)):
        lip = min((p for p in lips if abs(p.x - x) < 0.3), key=lambda p: abs(p.x - x), default=None)
        top = (lip.z if lip is not None else 5.0) - 0.05
        decals.append(stain(band, x, top, width, height, cell))
    for corner in nest_spots:
        decals.append(stain(band, corner.x, corner.z - 0.15, 0.6, 2.2, 7))

    # The slopes' own tufts and sage on the ground and the bench, off the fans and slabs.
    keep_off = [(o.matrix_world.translation, max(o.dimensions.x, o.dimensions.y) * 0.55)
                for name, copies in placed.items() if name.startswith(('Scree', 'Fallen')) for o in copies]
    ground = []
    for k in range(140):
        x, y = rng.uniform(-10.0, 14.0), rng.uniform(-14.0, -1.0)
        foot, _ = find_foot(rocks, x, 0.2)
        if foot is None or y > foot.y - 0.6 or any((Vector((x, y, 0.0)) - c).length < r for c, r in keep_off):
            continue
        ground.append(place(rng.choice(tufts[:2] * 3 + tufts[2:]), Vector((x, y, 0.0)), rng.uniform(0.0, 6.28),
                            scale=rng.uniform(0.85, 1.2)))
    for k in range(40):
        x, y = rng.uniform(-5.5, 14.0), rng.uniform(1.8, 9.0)
        hit, _, _ = rocks.cast((x, y, 30.0), (0.0, 0.0, -1.0), 40.0)
        if hit is not None and hit.z > BENCH_Z + 0.05:
            continue
        ground.append(place(rng.choice(tufts), Vector((x, y, BENCH_Z)), rng.uniform(0.0, 6.28)))
    human = homes.figure()
    human.location = (-1.2, -5.2, 0.0)
    human.rotation_euler = (0.0, 0.0, math.radians(200.0))
    return scene, rocks, placed, decals, ground, human, tris, slots, by, rock


# --- The shots ---

def shoot(scene, name, eye, target, lens=35.0, size=SHOT):
    return homes.shoot(scene, os.path.join(OUT, name + '.png'), eye, target, lens, size, -0.1)


def first(placed, name):
    return placed[name][0].matrix_world.translation if placed.get(name) else Vector((0.0, -1.0, 3.0))


def shots(scene, placed, decals, ground, human):
    out = {}
    out['banner'] = shoot(scene, 'CliffLife_Band', (-1.0, -18.5, 2.0), (0.6, 0.0, 4.2), 30.0, WIDE)
    if '--quick' in ARGS:
        return out
    life = [o for copies in placed.values() for o in copies] + decals
    for o in life:
        o.hide_render = True
    out['before'] = shoot(scene, 'CliffLife_Band_Before', (-1.0, -18.5, 2.0), (0.6, 0.0, 4.2), 30.0, WIDE)
    for o in life:
        o.hide_render = False
    d = first(placed, 'LipDrape_B')
    out['lip'] = shoot(scene, 'CliffLife_Lip', Vector((d.x - 1.8, -6.0, 1.7)), d + Vector((0.4, 0.0, -1.0)), 32.0)
    ledge = first(placed, 'LedgeSage_A')
    out['ledges'] = shoot(scene, 'CliffLife_Ledges', ledge + Vector((-1.4, -4.2, 0.6)), ledge + Vector((0.4, 0.0, 0.1)))
    if placed.get('CliffJuniper_A'):
        eye, target = homes.frame_shot(placed['CliffJuniper_A'][:1], (-0.35, -1.0, 0.0), SHOT, 35.0, margin=1.9,
                                       height_bias=0.4)
        out['juniper'] = shoot(scene, 'CliffLife_Juniper', eye, target)
    out['foot'] = shoot(scene, 'CliffLife_Foot', (-1.5, -12.5, 2.4), (1.5, -1.5, 0.3), 32.0)
    if placed.get('SwallowNests_A'):
        eye, target = homes.frame_shot(placed['SwallowNests_A'][:1], (0.05, -1.0, -0.35), SHOT, 45.0, margin=3.6,
                                       height_bias=0.5)
        out['nests'] = shoot(scene, 'CliffLife_Nests', eye, target, 45.0)
    out['stains'] = shoot(scene, 'CliffLife_Stains', (2.2, -9.5, 2.3), (2.2, 0.0, 2.9), 32.0)
    return out


KIT_X = 70.0        # the kit's stand-ins stand well off the band, out of its shots


def box(name, x0, x1, y0, y1, z0, z1, mat):
    """A stand-in block of rock (the cliffs' material at the faces' 180 px/m), its occlusion baked."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector((x0 + (v.co.x + 0.5) * (x1 - x0), y0 + (v.co.y + 0.5) * (y1 - y0), z0 + (v.co.z + 0.5) * (z1 - z0)))
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    lt.assign(obj, mat)
    lt.box_uv(obj, 'RockCliff', texel_density=180.0)
    lt.bake_vertex_ao(obj, samples=8, distance=0.3)
    return obj


def kit(scene, by, rock):
    """Every piece on plain stand-ins, in order, for the sheet's labels: a wall (drapes over its top, nests under a
    shelf, crack junipers in its face, fans at its foot), and a ledge (sage, grass, the ledge juniper) with the slabs
    on the ground in front."""
    x = KIT_X
    wall = [box('_KitWall', x - 1.0, x + 18.0, 0.0, 1.6, -0.3, 3.2, rock),
            box('_KitShelf', x + 8.6, x + 12.2, -0.6, 0.3, 1.7, 2.0, rock)]
    pieces = []
    for name, at in (('LipDrape_A', (1.6, 0.0, 3.2)), ('LipDrape_B', (4.4, 0.0, 3.2)), ('LipDrape_C', (6.6, 0.0, 3.2)),
                     ('SwallowNests_A', (9.4, 0.0, 1.7)), ('SwallowNests_B', (11.3, -0.14, 1.7)),
                     ('CliffJuniper_A', (13.6, 0.0, 1.4)), ('CliffJuniper_C', (16.4, 0.0, 1.2)),
                     ('ScreeFan_A', (3.4, 0.1, 0.0)), ('ScreeFan_C', (10.4, 0.1, 0.0)), ('ScreeFan_B', (16.0, 0.1, 0.0))):
        pieces.append(place(by[name], Vector((x + at[0], at[1], at[2]))))
    human = homes.figure()
    human.location = (x + 7.4, -2.6, 0.0)
    # Eye height about the shelf's underside, so the nests under it show.
    eye, target = homes.frame_shot(wall + pieces, (-0.12, -1.0, 0.0), KIT, 32.0, margin=1.02, height_bias=0.42)
    shot_wall = shoot(scene, 'CliffLife_KitWall', eye, target, 32.0, KIT)
    ledge = [box('_KitLedge', x + 20.0, x + 29.5, -0.2, 1.6, -0.3, 0.8, rock),
             box('_KitLedgeBack', x + 20.0, x + 29.5, 1.6, 2.6, -0.3, 2.4, rock)]
    rest = []
    for name, at in (('LedgeSage_A', (21.2, 0.05, 0.8)), ('LedgeSage_B', (22.6, 0.05, 0.8)),
                     ('LedgeGrass_A', (23.8, 0.05, 0.8)), ('LedgeGrass_B', (24.9, 0.0, 0.8)),
                     ('CliffJuniper_B', (27.5, 0.3, 0.8)), ('FallenSlab_A', (22.8, -3.6, 0.0)),
                     ('FallenSlab_B', (27.4, -3.7, 0.0))):
        rest.append(place(by[name], Vector((x + at[0], at[1], at[2]))))
    human2 = homes.figure()
    human2.location = (x + 25.2, -2.0, 0.0)
    eye, target = homes.frame_shot(ledge + rest, (-0.12, -1.0, 0.28), KIT, 32.0, margin=1.03, height_bias=0.45)
    shot_ledge = shoot(scene, 'CliffLife_KitLedge', eye, target, 32.0, KIT)
    return shot_wall, shot_ledge


def compose(out, tris, slots):
    """The sheet: wide pictures across it and pairs of tiles, each labelled with its models and triangles, on the look
    sheets' dark ground (mountain_sheet.py's layout), 2000 px wide."""
    def t(*names):
        return '   '.join(f'{n} {tris.get(n, 0):,}' for n in names)

    def n(name):
        return f'{name} {tris.get(name, 0):,}'
    total = sum(tris.values())
    blocks = [
        ('wide', out.get('banner'), 'With life (below, the same view today). '
         f'{len(tris)} models, {total:,} triangles in all; the stains are decals.\n'
         'One material slot each but the junipers (FoliageJuniper, BarkOak: the slopes\' pair) and the fans\n'
         '(RockCliff chips on a DenFloor bed). Every spot found by rays on the faces\' own meshes.'),
        ('grid', [
            (out.get('before'), 'Today: the same band, bare',
             'Clean stacked slabs, nothing growing, nothing\nliving; a hard line where the foot meets the ground.'),
            (out.get('lip'), t('LipDrape_B', 'LipDrape_C'),
             'Dry grass and fine roots spilling over a band\'s\nlip, hanging 0.3-2 m, clear of the face under it.'),
            (out.get('juniper'), t('CliffJuniper_A', 'LedgeGrass_A'),
             'Out of a joint in the face: the trunk leaves the\ncrack level and turns up; roots grip; tiered pads.'),
            (out.get('ledges'), t('LedgeSage_A', 'CliffJuniper_B'),
             'On ledges in the face and on the band\'s top:\nsage leaning out, the ledge juniper.'),
            (out.get('foot'), t('ScreeFan_A', 'FallenSlab_A'),
             'Fans of broken stone on a bed of dust, tucked\nunder the face\'s foot; a fallen slab of two beds.'),
            (out.get('nests'), t('SwallowNests_A', 'SwallowNests_B'),
             'Cliff swallows\' mud nests in an overhang\'s\ninner corner, mouths out and down.'),
            (STAIN_SHEET, 'CliffStains: 8 decal cells',
             'Varnish: curtain, streak, twin, tongue, ribbons.\nMineral seep, wash; whitewash. Bare rock at right.'),
            (out.get('stains'), 'The stains on the band (decals)',
             'Varnish under the lips, a seep\'s bloom, whitewash\nunder the nests: colour and roughness only.'),
        ]),
        ('wide', out.get('kit_wall'),
         f'The kit on stand-ins (1 of 2), left to right. Over the top: {n("LipDrape_A")}, {n("LipDrape_B")}, '
         f'{n("LipDrape_C")}.\nUnder the shelf: {n("SwallowNests_A")}, {n("SwallowNests_B")}. In cracks: '
         f'{n("CliffJuniper_A")}, {n("CliffJuniper_C")}.\nAt the foot: {n("ScreeFan_A")}, {n("ScreeFan_C")}, '
         f'{n("ScreeFan_B")}. The figure is 1.8 m.'),
        ('wide', out.get('kit_ledge'),
         f'The kit on stand-ins (2 of 2), left to right. On the ledge: {n("LedgeSage_A")}, {n("LedgeSage_B")},\n'
         f'{n("LedgeGrass_A")}, {n("LedgeGrass_B")}, {n("CliffJuniper_B")}. In front: {n("FallenSlab_A")}, '
         f'{n("FallenSlab_B")}.'),
    ]
    width, pad, gap, columns = 2000, 40, 18, 2
    scene = bpy.data.scenes.new('_Sheet')
    objs = []
    y = pad
    objs.append(homes._text('title', "Ransom's Rest: cliff life (look sheet 43)", pad, y + 72, 84, 0xf2a64a))
    y += 100
    subtitle = ('Plants that cling, lips that spill over, scree at the feet, stains down the faces, swallows\' nests:\n'
                'on the cliff kit\'s own faces (CliffFace_C as a 6 m band, _D beside it, _A above) in the area\'s rock '
                'tint\nand late sun. Blender previews; nothing imported yet.')
    for k, line in enumerate(subtitle.split('\n')):
        objs.append(homes._text(f'sub{k}', line, pad, y + 40, 40, 0xc9c4ba, bold=False))
        y += 52
    y += 20
    inner = width - 2 * pad
    tile_w = (inner - gap * (columns - 1)) / columns
    count = 0
    for block in blocks:
        if block[0] == 'wide':
            _, png, caption = block
            if not png or not os.path.exists(png):
                continue
            img = bpy.data.images.load(png, check_existing=True)
            h = inner * img.size[1] / img.size[0]
            objs.append(homes._image_plane(f'wide{count}', png, pad, y, inner, h))
            y += h + 14
            for k, line in enumerate(caption.split('\n')):
                objs.append(homes._text(f'wcap{count}_{k}', line, pad, y + 40, 38, 0xe8e2d6, bold=False))
                y += 50
            y += 30
            count += 1
            continue
        tiles = [tile for tile in block[1] if tile[0] and os.path.exists(tile[0])]
        for r in range(0, len(tiles), columns):
            row_h = 0.0
            for c, (png, label, caption) in enumerate(tiles[r:r + columns]):
                img = bpy.data.images.load(png, check_existing=True)
                th = tile_w * img.size[1] / img.size[0]
                x = pad + c * (tile_w + gap)
                objs.append(homes._text(f'label{count}', label, x, y + 52, 48, 0xf2a64a, z=0.6))
                objs.append(homes._image_plane(f'tile{count}', png, x, y + 70, tile_w, th))
                lines = caption.split('\n')
                for j, line in enumerate(lines):
                    objs.append(homes._text(f'cap{count}_{j}', line, x, y + 70 + th + 44 + 46 * j, 36, 0xe8e2d6,
                                            bold=False))
                row_h = max(row_h, 70 + th + 28 + 46 * len(lines))
                count += 1
            y += row_h + 30
    height = int(y + pad - 10)
    objs.append(homes._rect('ground', 0, 0, width, height, 0x16171a))
    for o in objs:
        scene.collection.objects.link(o)
    cam = bpy.data.objects.new('_SheetCam', bpy.data.cameras.new('_SheetCam'))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = max(width, height)
    cam.data.sensor_fit = 'AUTO'
    cam.location = (width * 0.5, -height * 0.5, 100.0)
    cam.data.clip_start, cam.data.clip_end = 1.0, 500.0
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 8
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'None'
    world = bpy.data.worlds.new('_SheetWorld')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(0x16171a)
    scene.world = world
    scene.render.image_settings.file_format = 'JPEG'
    scene.render.image_settings.quality = 85
    scene.render.image_settings.color_mode = 'RGB'
    os.makedirs(os.path.dirname(SHEET), exist_ok=True)
    scene.render.filepath = SHEET
    bpy.ops.render.render(write_still=True, scene=scene.name)
    lt._log(f'sheet: {SHEET} ({width} x {height})')


def main():
    scene, rocks, placed, decals, ground, human, tris, slots, by, rock = build_scene()
    for name in sorted(tris):
        print(f'LOOTER: sheet: {name}: {tris[name]} triangles, {", ".join(slots[name])}, placed '
              f'{len(placed.get(name, []))}', flush=True)
    out = shots(scene, placed, decals, ground, human)
    if '--quick' not in ARGS:
        out['kit_wall'], out['kit_ledge'] = kit(scene, by, rock)
        compose(out, tris, slots)


main()
