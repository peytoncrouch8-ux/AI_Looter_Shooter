"""Preview renders of an area (Eevee) into a folder (Saved/ArtPreviews/Terrain/<Area> by default): the layout's views
(layout.json preview.views) and plan.png (top down, annotated with the zones, preview.labels, the computed points, the
open-ground metric's overlay and, grounded, the playable boundary). A grounded area also gets plan_region.png (the
ring's square: the core, its seam band, the lip, the ridges' feet, the river) and, with region.farTrees, far_trees.png
(the far trees' crowns over that square, and the line they start at), and every area with step 3b's features
gets a clay view of each, feature_<id>.png, with slabs standing in for the cliff kit's pieces (one color per stacked
course). Clay blocks stand in for the buildings, for scale. Everything added is removed afterwards.

A view is an eye and a target, each [X, Y, height above the ground] in cm (an eye or a target outside the map square
takes its height as absolute), and a lens in mm. Optional: "sun" [azimuth from north, elevation] in degrees, the
planned sun: warm, low, its shadows long, its disc in the sky, under a golden sky (else a sun from the view's upper
left); "light" [azimuth, elevation], a plain sun from there (for clay views); "clay": the terrain in clay with the
obstacles and the buildings as stand-ins ("cliffs" adds the cliff kit's courses as see-through bands, and "cliffs":
"solid" stands solid blocks where the level's builder puts the kit's pieces, to see what they hide; "steep": degrees
paints clay ground steeper than that red, so bare steep terrain shows); "obstacles":
the obstacles' stand-ins on the textured terrain (false leaves them out of a clay view, so tree stands don't hide the
ground); "ortho": a width (cm) seen straight down onto the target, north up, under a dim sky so the sun's shadows read
as shapes. preview.features lists the features that get a clay view of their own (all of step 3b's when it's left
out), and preview.labelSize sets the plan's labels (m).
"""
import math
import os

import bpy
import numpy as np
from mathutils import Vector

import area_computed
from area_math import catmull_rom

STAND_INS = {  # kind: length along the actor's forward, width, height (m)
    'Farmhouse': (9.0, 11.0, 6.5), 'Barn': (12.0, 9.0, 8.0), 'Well': (1.8, 1.8, 1.4), 'LogCabin': (6.5, 8.0, 5.0),
    'Cottage': (6.5, 8.0, 5.5), 'Outhouse': (1.3, 1.3, 2.4), 'GunRack': (0.5, 2.0, 1.6), 'Windmill': (2.5, 2.5, 11.0),
    'LookoutTower': (4.0, 4.0, 10.0), 'TargetDummy': (0.5, 0.6, 1.8),
    # Ransom's Rest's buildings and props. A list is several boxes: (length, width, height, ahead, x, up), offset
    # from the pivot along the actor's forward (the model's -Y), the model's own x (as its script describes it) and up
    # (m).
    'Lookout': [(4.4, 4.4, 6.5, 0.0, 0.0, 0.0), (6.6, 6.6, 0.4, 1.1, 0.0, 6.5), (2.4, 3.4, 2.6, -1.0, 0.0, 6.9)],
    'CliffStairs': [(2.9, 8.7, 20.0, 1.45, -2.85, -20.0)], 'MooringPost': (0.4, 0.4, 1.6),
    'Chapel': [(9.6, 6.4, 8.5, -0.8, 0.0, 0.0), (3.2, 3.2, 16.0, 5.6, 0.0, 0.0), (2.6, 2.4, 4.0, -1.5, 4.4, 0.0)],
    'FalseFront_Undertaker': (12.0, 7.0, 8.5), 'FalseFront_Saloon': (14.0, 8.4, 10.5),
    'FalseFront_Store': (13.0, 9.4, 8.0), 'FalseFront_Sheriff': (10.0, 6.6, 8.0),
    'NoticeBoard': (0.6, 2.0, 2.4), 'TownMemorial': (1.0, 1.0, 1.8),
    'Depot': (6.0, 10.0, 6.5), 'WaterTower': (4.0, 4.0, 10.2), 'CoffinShed': (2.4, 3.2, 2.6),
    'DepotPlatform_4m': [(4.0, 4.0, 0.4, 0.0, 2.0, 0.0)], 'DepotPlatform_End': [(4.0, 2.4, 0.3, 0.0, 1.2, 0.0)],
    'Signal': (0.5, 0.5, 6.2), 'BufferStop': (1.5, 3.0, 1.6), 'Track_Straight_12m': [(12.0, 2.6, 0.25, 6.0, 0.0, 0.0)],
    'HearseCar': (10.3, 2.9, 4.2), 'PassengerCar': (12.3, 2.9, 4.2), 'Locomotive_B': (8.8, 2.8, 4.6),
    'BurialDeck': [(25.0, 18.0, 0.4, 0.0, 0.0, 0.0)],
    'Grave_Ellis': (2.2, 1.2, 1.0), 'Grave_Abel': (2.2, 1.2, 1.0), 'Grave_Keeper': (1.6, 2.2, 2.2),
    'Grave_MoundFresh': (2.0, 1.0, 0.9), 'DeadTree_A': (0.8, 0.8, 9.0),
    'Ruin_Chimney': (1.6, 2.2, 6.6), 'Ruin_Derrick': (16.0, 2.0, 1.5), 'Wagon_BurntA': (4.0, 2.0, 2.0),
    'Wagon_BurntB': (4.0, 2.0, 1.4), 'Cairn_A': (0.8, 0.8, 1.0), 'Cairn_B': (0.8, 0.8, 1.0), 'Cairn_C': (0.8, 0.8, 1.2),
}
# The planned light for views with a "sun": golden late afternoon.
GOLDEN_SUN = (1.0, 0.8, 0.58)
# How far (m) the plan's annotations are lifted, so ground higher than they'd otherwise float (a grounded area's
# ridges) never hides them; render_plan sets it.
_plan_lift = 0.0


def _to_b(x, y, z):
    return Vector((-y, -x, z))


def _material(name, color, emission=0.0):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Roughness'].default_value = 0.8
    if emission:
        bsdf.inputs['Emission Color'].default_value = color
        bsdf.inputs['Emission Strength'].default_value = emission
    return mat


def _world(scene, golden=False, strength=1.0):
    """The preview's sky: a gradient from the ground's grey through a pale horizon to blue (golden: a warm horizon,
    the late afternoon's), lighting the scene at strength."""
    world = bpy.data.worlds.new('_PreviewSky')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    ramp = nodes.new('ShaderNodeValToRGB')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    links.new(split.outputs['Z'], ramp.inputs['Fac'])
    ramp.color_ramp.interpolation = 'EASE'
    e = ramp.color_ramp.elements
    e[0].position, e[0].color = 0.0, (0.26, 0.29, 0.33, 1.0) if not golden else (0.3, 0.27, 0.24, 1.0)
    e[1].position, e[1].color = 0.62, (0.16, 0.30, 0.62, 1.0) if not golden else (0.2, 0.33, 0.6, 1.0)
    mid = e.new(0.03)
    mid.color = (0.62, 0.66, 0.66, 1.0) if not golden else (0.86, 0.72, 0.52, 1.0)
    links.new(ramp.outputs['Color'], bg.inputs['Color'])
    bg.inputs['Strength'].default_value = strength
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    world.mist_settings.start = 110.0
    world.mist_settings.depth = 700.0
    world.mist_settings.falloff = 'QUADRATIC'
    scene.world = world
    return world


def _haze(scene, on, color=(0.66, 0.72, 0.78, 1.0)):
    """Aerial haze from the mist pass, mixed toward a pale sky color in the compositor."""
    scene.view_layers[0].use_pass_mist = on
    scene.use_nodes = on
    if not on:
        return
    tree = scene.node_tree
    tree.nodes.clear()
    layers = tree.nodes.new('CompositorNodeRLayers')
    mix = tree.nodes.new('CompositorNodeMixRGB')
    mix.inputs[2].default_value = color
    scale = tree.nodes.new('CompositorNodeMath')
    scale.operation = 'MULTIPLY'
    scale.inputs[1].default_value = 0.4
    tree.links.new(layers.outputs['Mist'], scale.inputs[0])
    tree.links.new(scale.outputs[0], mix.inputs[0])
    tree.links.new(layers.outputs['Image'], mix.inputs[1])
    comp = tree.nodes.new('CompositorNodeComposite')
    tree.links.new(mix.outputs[0], comp.inputs['Image'])


def _sun(forward, added, strength=4.2, side=0.85, ahead=0.3, elevation=36.0):
    """A warm sun from the upper left of a view looking along forward (Blender axes)."""
    flat = Vector((forward.x, forward.y, 0.0))
    flat = flat.normalized() if flat.length > 1e-6 else Vector((0.0, 1.0, 0.0))
    left = Vector((0.0, 0.0, 1.0)).cross(flat)
    toward = (flat * ahead + left * side).normalized()
    up = math.radians(elevation)
    toward = (toward * math.cos(up) + Vector((0.0, 0.0, math.sin(up)))).normalized()
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = strength
    sun.data.color = (1.0, 0.9, 0.76)
    sun.data.angle = math.radians(1.5)
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(sun)
    added.append(sun)
    return sun


def _world_sun(azimuth, elevation, added, strength=4.6, color=GOLDEN_SUN, disc=False):
    """A fixed sun in the area's sky: azimuth in degrees from north toward east, elevation over the horizon; warm by
    default, the planned golden late afternoon's. Its shadows are the views' point (a low sun throws long ones). disc
    puts its glowing disc in the sky, 12 km out (inside the cameras' 20 km reach)."""
    a, e = math.radians(azimuth), math.radians(elevation)
    # Toward the sun, in Blender's axes (x = -east, y = -north).
    toward = Vector((-math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e)))
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = strength
    sun.data.color = color
    sun.data.angle = math.radians(0.8)
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(sun)
    added.append(sun)
    if disc:
        mesh = bpy.data.meshes.new('_SunDisc')
        ring = [(math.cos(t) * 160.0, math.sin(t) * 160.0, 0.0) for t in np.linspace(0.0, 2.0 * math.pi, 24,
                                                                                    endpoint=False)]
        mesh.from_pydata(ring, [], [tuple(range(24))])
        mesh.materials.append(_material('_SunDisc', (1.0, 0.86, 0.62, 1.0), emission=40.0))
        obj = bpy.data.objects.new('_SunDisc', mesh)
        obj.location = toward * 12000.0
        obj.rotation_euler = toward.to_track_quat('Z', 'Y').to_euler()
        obj.visible_shadow = False
        bpy.context.scene.collection.objects.link(obj)
        added.append(obj)
    return sun


def _stand_ins(area, added):
    clay = _material('_Clay', (0.55, 0.52, 0.47, 1.0))
    for p in area.layout['placements']:
        if p['kind'] not in STAND_INS:
            continue
        spec = STAND_INS[p['kind']]
        boxes = spec if isinstance(spec, list) else [tuple(spec) + (0.0, 0.0, 0.0)]
        x, y = np.asarray(p['location'][:2]) / 100.0
        z = float(area.height(x, y))
        verts, faces = [], []
        for length, width, height, ahead, model_x, up in boxes:
            # In the stand-in's own axes (a model's Blender axes): the actor's forward is -y, so the box spans its
            # width along x and its length along y.
            hx, hy = width * 0.5, length * 0.5
            cx, cy = model_x, -ahead
            b = len(verts)
            verts += [(cx - hx, cy - hy, up), (cx + hx, cy - hy, up), (cx + hx, cy + hy, up), (cx - hx, cy + hy, up),
                      (cx - hx, cy - hy, up + height), (cx + hx, cy - hy, up + height), (cx + hx, cy + hy, up + height),
                      (cx - hx, cy + hy, up + height)]
            faces += [tuple(b + i for i in f) for f in ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5),
                                                        (2, 3, 7, 6), (3, 0, 4, 7))]
        mesh = bpy.data.meshes.new('_Stand')
        mesh.from_pydata(verts, [], faces)
        mesh.materials.append(clay)
        obj = bpy.data.objects.new('_Stand_' + p['id'], mesh)
        obj.location = _to_b(x, y, z - 0.3)
        obj.rotation_euler = (0.0, 0.0, -math.radians(p.get('yaw', 0.0)))
        bpy.context.scene.collection.objects.link(obj)
        added.append(obj)


def _computed(area):
    """layout_computed.json's values for the previews, worked out once per area."""
    if getattr(area, '_preview_data', None) is None:
        area._preview_data = area_computed.compute(area)
    return area._preview_data


class _Batch:
    """Stand-in solids merged per material (one object each), in layout meters."""

    def __init__(self):
        self.parts = {}

    def _add(self, group, verts, faces):
        vs, fs = self.parts.setdefault(group, ([], []))
        base = len(vs)
        vs += verts
        fs += [tuple(base + i for i in f) for f in faces]

    def prism(self, group, pts, z0, z1):
        """A polygon (N, 2) from per-point foot heights z0 (N,) up to a flat top z1."""
        n = len(pts)
        verts = [tuple(_to_b(x, y, z)) for (x, y), z in zip(pts, z0)] + [tuple(_to_b(x, y, z1)) for x, y in pts]
        faces = [tuple(range(n)), tuple(range(2 * n - 1, n - 1, -1))]
        faces += [(i, (i + 1) % n, n + (i + 1) % n, n + i) for i in range(n)]
        self._add(group, verts, faces)

    def box(self, group, area, c, along, length, width, height, sink=0.3):
        """A box on the ground at c, its length along the unit vector along (layout axes)."""
        side = np.array([-along[1], along[0]])
        pts = np.array([c + along * a * length * 0.5 + side * b * width * 0.5
                        for a, b in ((1, 1), (-1, 1), (-1, -1), (1, -1))])
        z = area.height(pts[:, 0], pts[:, 1]) - sink
        self.prism(group, pts, z, float(np.max(z)) + sink + height)

    def cone(self, group, area, c, radius, height, sides=7):
        z = float(area.height(*c)) - 0.3
        a = np.linspace(0.0, 2.0 * math.pi, sides, endpoint=False)
        verts = [tuple(_to_b(c[0] + radius * math.cos(t), c[1] + radius * math.sin(t), z)) for t in a]
        verts.append(tuple(_to_b(c[0], c[1], z + height + 0.3)))
        faces = [tuple(range(sides))[::-1]] + [(i, (i + 1) % sides, sides) for i in range(sides)]
        self._add(group, verts, faces)

    def finish(self, materials, added):
        for group, (verts, faces) in self.parts.items():
            mesh = bpy.data.meshes.new('_Obstacles_' + group)
            mesh.from_pydata(verts, [], faces)
            mesh.materials.append(materials[group])
            obj = bpy.data.objects.new('_Obstacles_' + group, mesh)
            bpy.context.scene.collection.objects.link(obj)
            added.append(obj)


def _inside_points(poly, spacing, rng):
    """Jittered points on a grid spacing meters apart inside a polygon (layout meters); its middle if none."""
    from area_math import points_in_polygon
    lo, hi = poly.min(axis=0), poly.max(axis=0)
    xs = np.arange(lo[0] + spacing * 0.5, hi[0], spacing)
    ys = np.arange(lo[1] + spacing * 0.5, hi[1], spacing)
    if not len(xs) or not len(ys):
        return poly.mean(axis=0)[None, :]
    x, y = np.meshgrid(xs, ys, indexing='ij')
    pts = np.column_stack([x.ravel(), y.ravel()]) + rng.uniform(-0.3, 0.3, (x.size, 2)) * spacing
    keep = points_in_polygon(pts[:, 0], pts[:, 1], poly)
    return pts[keep] if keep.any() else poly.mean(axis=0)[None, :]


def _along_path(pts, step):
    """Points every step meters along a polyline, with the direction there."""
    out = []
    for a, b in zip(pts[:-1], pts[1:]):
        d = b - a
        length = float(np.linalg.norm(d))
        if length < 1e-6:
            continue
        u = d / length
        for t in np.arange(step * 0.5, length, step):
            out.append((a + u * t, u))
    return out


def _pieces(line, step=2.0):
    """A polyline cut into pieces at most step meters long (center, direction, length), so walls built of them
    follow the ground."""
    out = []
    for a, b in zip(line[:-1], line[1:]):
        d = b - a
        length = float(np.linalg.norm(d))
        if length < 1e-6:
            continue
        count = max(1, int(math.ceil(length / step)))
        for k in range(count):
            out.append((a + d * (k + 0.5) / count, d / length, length / count))
    return out


def _obstacle_stand_ins(area, added, clay=False):
    """layout.json's obstacles as simple solids at their heights: fences and walls along their lines, boulder groups
    as scattered blocks, tree stands as cones, ruins as roofless walls, outcrops and props as blocks; the orchard's
    trees too. One object per material."""
    rng = np.random.default_rng(17)
    colors = {'rock': (0.34, 0.32, 0.3, 1.0), 'tree': (0.27, 0.3, 0.24, 1.0) if clay else (0.13, 0.2, 0.1, 1.0),
              'built': (0.62, 0.55, 0.45, 1.0), 'fence': (0.74, 0.7, 0.62, 1.0)}
    materials = {k: _material('_Ob_' + k + ('_clay' if clay else ''), c) for k, c in colors.items()}
    batch = _Batch()
    for ob in area.layout.get('obstacles', []):
        kind, h = ob.get('kind', ''), ob.get('height', 150) / 100.0
        closed = 'polygon' in ob
        pts = np.asarray(ob['polygon'] if closed else ob['path'], dtype=np.float64) / 100.0
        line = np.vstack([pts, pts[:1]]) if closed else pts
        if kind in ('fence', 'wall'):
            for c, u, length in _pieces(line):
                batch.box('fence', area, c, u, length, 0.15 if kind == 'fence' else 0.6, h)
        elif kind in ('cairns', 'graves', 'biers'):
            step, size = {'cairns': (2.5, (0.9, 0.9)), 'graves': (1.6, (0.15, 0.6)), 'biers': (3.4, (2.2, 0.8))}[kind]
            for c, u in _along_path(line, step):
                batch.box('rock' if kind == 'cairns' else 'built', area, c, u, size[0], size[1], h)
        elif kind == 'ruin' and closed:
            for c, u, length in _pieces(line):
                batch.box('built', area, c, u, length, 0.5, h * 0.75)
        elif kind == 'boulders' and closed:
            for c in _inside_points(pts, 2.8, rng):
                a = rng.uniform(0.0, math.pi)
                batch.box('rock', area, c, np.array([math.cos(a), math.sin(a)]), rng.uniform(1.4, 2.6),
                          rng.uniform(1.2, 2.2), h * rng.uniform(0.45, 1.0))
        elif kind == 'trees' and closed:
            big = h >= 9.0
            for c in _inside_points(pts, 5.5 if big else 4.0, rng):
                batch.cone('tree', area, c, (2.6 if big else 2.0) * rng.uniform(0.85, 1.15), h * rng.uniform(0.8, 1.1))
        elif closed:  # outcrops, props, ruins drawn as areas
            z = area.height(pts[:, 0], pts[:, 1]) - 0.3
            batch.prism('rock' if kind == 'outcrop' else 'built', pts, z, float(np.max(z)) + 0.3 + h)
        else:
            for c, u, length in _pieces(line):
                batch.box('built', area, c, u, length, 1.6, h)
    for row in area_computed.orchard_rows(area):
        for t in row['trees']:
            batch.cone('tree', area, np.array(t[:2]) / 100.0, 1.8, 4.5)
    batch.finish(materials, added)


def _camera(eye, target, lens, added, ortho=None, far=2000.0):
    cam = bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    cam.data.lens = lens
    cam.data.clip_start = 0.2
    cam.data.clip_end = far
    cam.location = eye
    if ortho is not None:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
        cam.rotation_euler = (0.0, 0.0, math.pi)  # looking down, north (-Y) up, east (-X) right
    else:
        cam.rotation_euler = (target - eye).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(cam)
    added.append(cam)
    bpy.context.scene.camera = cam
    return cam


def _render(path, size, samples=24):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = samples
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def _cleanup(added):
    for obj in added:
        data = obj.data
        bpy.data.objects.remove(obj)
        if data is None or data.users:
            continue
        for store in (bpy.data.meshes, bpy.data.lights, bpy.data.cameras, bpy.data.curves):
            if data.name in store and store[data.name] == data:
                store.remove(data)
                break


def render_views(area, out_dir, log=print, only=None):
    scene = bpy.context.scene
    for name, view in area.layout.get('preview', {}).get('views', {}).items():
        if only and name not in only:
            continue
        golden = 'sun' in view
        # Seen straight down, the sky lights shadowed ground almost as much as the low sun lights the rest: dimmed,
        # the shadows show.
        world = _world(scene, golden=golden, strength=0.35 if view.get('ortho') else 1.0)
        _haze(scene, True, (0.82, 0.75, 0.64, 1.0) if golden else (0.66, 0.72, 0.78, 1.0))
        (ex, ey, eh), (tx, ty, th) = (np.asarray(view[k], dtype=np.float64) / 100.0 for k in ('eye', 'target'))
        added = []
        _stand_ins(area, added)
        restore = None
        if view.get('clay'):
            restore = _clay_terrain(area, view.get('steep'))
            if view.get('cliffs') == 'solid':
                _piece_stand_ins(area, _computed(area), added)
            elif view.get('cliffs'):
                _cliff_stand_ins(area, _computed(area), added)
        if view.get('obstacles', bool(view.get('clay'))):
            _obstacle_stand_ins(area, added, clay=bool(view.get('clay')))
        inside = abs(ex) < area.half and abs(ey) < area.half
        eye = _to_b(ex, ey, float(area.height(ex, ey)) + eh if inside else eh)
        aimed = abs(tx) < area.half and abs(ty) < area.half
        target = _to_b(tx, ty, float(area.height(tx, ty)) + th if aimed else th)
        if view.get('ortho'):
            # Straight down onto the target, north up, the given width (cm) across: shadows read as shapes.
            _camera(Vector((target.x, target.y, target.z + 300.0)), None, view['lens'], added,
                    ortho=view['ortho'] / 100.0, far=2000.0)
        else:
            # A grounded area's backdrop stands out to 12 km.
            _camera(eye, target, view['lens'], added, far=20000.0 if area.setting == 'grounded' else 2000.0)
        if golden:
            _world_sun(view['sun'][0], view['sun'][1], added, disc=True)
        elif 'light' in view:
            _world_sun(view['light'][0], view['light'][1], added, strength=4.2, color=(1.0, 0.95, 0.88))
        else:
            _sun((target - eye).normalized(), added)
        path = os.path.join(out_dir, name + '.png')
        _render(path, (1600, 900))
        _cleanup(added)
        if restore:
            restore()
        _haze(scene, False)
        bpy.data.worlds.remove(world)
        log(f'terrain: preview {path}')


def _text(body, x, y, size, color, added, z=40.0):
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = body
    curve.size = size
    curve.align_x = 'CENTER'
    curve.align_y = 'CENTER'
    obj = bpy.data.objects.new('_Label', curve)
    obj.visible_shadow = False  # annotations float above the terrain: no shadows on it
    obj.location = _to_b(x, y, z + _plan_lift)
    obj.rotation_euler = (0.0, 0.0, math.pi)
    obj.data.materials.append(_material('_Label' + str(color), color, emission=3.0))
    bpy.context.scene.collection.objects.link(obj)
    added.append(obj)
    return obj


def _line(points, width, color, added, z=30.0, closed=False):
    curve = bpy.data.curves.new('_Line', 'CURVE')
    curve.dimensions = '3D'
    curve.bevel_depth = width
    spline = curve.splines.new('POLY')
    spline.points.add(len(points) - 1)
    for p, (x, y) in zip(spline.points, points):
        v = _to_b(x, y, z + _plan_lift)
        p.co = (v.x, v.y, v.z, 1.0)
    spline.use_cyclic_u = closed
    obj = bpy.data.objects.new('_Line', curve)
    obj.visible_shadow = False
    obj.data.materials.append(_material('_Line' + str(color), color, emission=2.0))
    bpy.context.scene.collection.objects.link(obj)
    added.append(obj)


def render_plan(area, out_dir, log=print):
    """Top down over the map square: the area in its macro colors, zones outlined, labels, and the computed points
    (cliff dressing, hanging rim cliffs, the bridge, the waterfall)."""
    global _plan_lift
    scene = bpy.context.scene
    world = _world(scene)
    added = []
    _stand_ins(area, added)
    # Annotations float over the highest ground in the square (a grounded area's ridges reach well over them).
    _plan_lift = max(0.0, float(np.max(area.h)) - 25.0)
    _camera(Vector((0.0, 0.0, 300.0 + _plan_lift)), None, 50.0, added, ortho=2.0 * area.half + 1.2)
    _sun(Vector((0.0, -1.0, -0.3)), added, strength=3.6, side=0.8, ahead=-0.6, elevation=40.0)
    white, dark = (1.0, 1.0, 1.0, 1.0), (0.02, 0.02, 0.03, 1.0)
    zone_color = (1.0, 0.45, 0.9, 1.0)
    for zone in area.layout['zones']:
        pts = np.asarray(zone['polygon']) / 100.0
        _line([tuple(p) for p in pts], 0.12, zone_color, added, closed=True)
    size = area.layout.get('preview', {}).get('labelSize', 3.4)
    for text, (x, y) in area.layout.get('preview', {}).get('labels', {}).items():
        x, y = x / 100.0, y / 100.0
        _text(text, x - 0.5 * size / 3.4, y + 0.5 * size / 3.4, size, dark, added, z=39.0)
        _text(text, x, y, size, white, added)
    data = _computed(area)
    orange, cyan, red = (1.0, 0.5, 0.1, 1.0), (0.2, 0.9, 1.0, 1.0), (1.0, 0.1, 0.1, 1.0)
    for points in data['cliffs'].values():
        for point in points:
            x, y = point['location'][0] / 100.0, point['location'][1] / 100.0
            a = math.radians(point['yaw'])
            # Hanging cliffs (the rim, with its drop below) in cyan, standing ones in orange.
            hanging = 'drop' in point
            length = 2.5 if hanging else 3.0
            _line([(x, y), (x + length * math.cos(a), y + length * math.sin(a))], 0.25 if hanging else 0.35,
                  cyan if hanging else orange, added)
    _plan_marks(area, data, added)
    b = data['bridge']
    if b:
        bx, by = b['location'][0] / 100.0, b['location'][1] / 100.0
        a = math.radians(b['yaw'])
        half = b['span'] / 200.0
        _line([(bx - half * math.cos(a), by - half * math.sin(a)), (bx + half * math.cos(a), by + half * math.sin(a))],
              0.9, red, added)
        _text('Bridge', bx + 4.0, by + 8.0, 2.6, white, added)
    w = data['waterfall']
    if w:
        _text('Waterfall', w['location'][0] / 100.0 - 4.0, w['location'][1] / 100.0 - 6.0, 2.6, white, added)
    path = os.path.join(out_dir, 'plan.png')
    _render(path, (2048, 2048), samples=16)
    _cleanup(added)
    _plan_lift = 0.0
    bpy.data.worlds.remove(world)
    log(f'terrain: preview {path}')


def _plan_marks(area, data, added):
    """The plan's annotations from step 3b on: stacked cliff courses (a second, purple tick), the knobs' rocks, the
    obstacles, ground left open on purpose, the playable boundary (closed edges white, open ones cyan) and the
    open-ground overlay with its worst spots."""
    purple, brown, blue = (0.75, 0.3, 1.0, 1.0), (0.55, 0.35, 0.2, 1.0), (0.25, 0.45, 1.0, 1.0)
    white, cyan, dark = (1.0, 1.0, 1.0, 1.0), (0.2, 0.9, 1.0, 1.0), (0.05, 0.05, 0.06, 1.0)
    for points in data['cliffs'].values():
        for point in points:
            x, y = point['location'][0] / 100.0, point['location'][1] / 100.0
            if point.get('kind') == 'outcrop':
                _ring_line(x, y, max(point['radius'] / 100.0, 1.5), 0.25, brown, added)
            elif point.get('courses'):
                a = math.radians(point['yaw'])
                side = np.array([-math.sin(a), math.cos(a)]) * 0.8
                _line([(x + side[0], y + side[1]), (x + side[0] + 3.0 * math.cos(a), y + side[1] + 3.0 * math.sin(a))],
                      0.3, purple, added)
    for ob in area.layout.get('obstacles', []):
        pts = np.asarray(ob.get('polygon', ob.get('path')), dtype=np.float64) / 100.0
        _line([tuple(p) for p in pts], 0.2, dark, added, z=31.0, closed='polygon' in ob)
    for spot in area.layout.get('openGround', {}).get('open', []):
        pts = np.asarray(spot['polygon'], dtype=np.float64) / 100.0
        _line([tuple(p) for p in pts], 0.18, blue, added, z=31.5, closed=True)
        c = pts.mean(axis=0)
        _text('open on purpose', c[0], c[1], 1.6, blue, added, z=39.0)
    boundary = data.get('boundary')
    if boundary:
        corners = np.array([c[:2] for c in boundary['corners']]) / 100.0
        for i, is_open in enumerate(boundary['openEdges']):
            a, b = corners[i], corners[(i + 1) % len(corners)]
            _line([tuple(a), tuple(b)], 0.32 if is_open else 0.4, cyan if is_open else white, added, z=32.0)
    _overlay(area, data, added)


def _ring_line(x, y, radius, width, color, added, z=31.0):
    a = np.linspace(0.0, 2.0 * math.pi, 33)
    _line([(x + radius * math.cos(t), y + radius * math.sin(t)) for t in a], width, color, added, z=z)


def _overlay(area, data, added):
    """The open-ground metric as a translucent heat map over the plan: nothing under 40% of the target, yellow to
    orange to red at the target and past it; a circle and the distance at each of the worst spots."""
    import area_open
    result, grid, shown = area_open.measure(area)
    target = result['target'] / 100.0
    t = np.nan_to_num(shown, nan=0.0) / target
    rgba = np.zeros(shown.shape + (4,), dtype=np.float32)
    warm = np.clip((t - 0.4) / 0.4, 0.0, 1.0)
    hot = np.clip((t - 0.8) / 0.2, 0.0, 1.0)
    rgba[..., 0] = 1.0
    rgba[..., 1] = 0.85 - 0.45 * warm - 0.3 * hot
    rgba[..., 2] = 0.2 - 0.1 * hot
    rgba[..., 3] = np.where(np.isfinite(shown), 0.22 * warm + 0.4 * hot, 0.0)
    image = bpy.data.images.new('_OpenGround', grid.n, grid.n, alpha=True)
    image.pixels.foreach_set(np.ascontiguousarray(rgba).ravel())
    mat = bpy.data.materials.new('_OpenGround')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = image
    tex.interpolation = 'Linear'
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs['Strength'].default_value = 1.6
    clear = nodes.new('ShaderNodeBsdfTransparent')
    mix = nodes.new('ShaderNodeMixShader')
    links.new(tex.outputs['Color'], emit.inputs['Color'])
    links.new(tex.outputs['Alpha'], mix.inputs['Fac'])
    links.new(clear.outputs['BSDF'], mix.inputs[1])
    links.new(emit.outputs['Emission'], mix.inputs[2])
    links.new(mix.outputs['Shader'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    h = area.half
    mesh = bpy.data.meshes.new('_OpenGround')
    corners = [(-h, -h), (-h, h), (h, h), (h, -h)]
    mesh.from_pydata([tuple(_to_b(x, y, 34.0 + _plan_lift)) for x, y in corners], [], [(0, 1, 2, 3)])
    uv = mesh.uv_layers.new(name='UVMap')
    uv.data.foreach_set('uv', [0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0])
    mesh.materials.append(mat)
    obj = bpy.data.objects.new('_OpenGround', mesh)
    obj.visible_shadow = False
    bpy.context.scene.collection.objects.link(obj)
    added.append(obj)
    red = (1.0, 0.15, 0.1, 1.0)
    for spot in result.get('worst', []):
        x, y = spot['location'][0] / 100.0, spot['location'][1] / 100.0
        r = spot['distance'] / 100.0
        _ring_line(x, y, r, 0.15, red, added, z=35.0)
        _text(f'{r:.1f} m', x, y, 1.8, red, added, z=39.5)
    _text(f"open ground: largest {result.get('largest', 0.0) / 100.0:.1f} m from a break (target {target:g} m)",
          -h + 6.0, 0.0, 2.4, (1.0, 1.0, 1.0, 1.0), added, z=39.5)


def render_region(area, out_dir, log=print):
    """A grounded area from above, over the ring's whole square: the core square and its seam band, the lip, the
    ridges' feet, the canyon's river and the playable boundary."""
    region = area.region
    scene = bpy.context.scene
    world = _world(scene)
    added = []
    _camera(Vector((0.0, 0.0, 800.0)), None, 50.0, added, ortho=2.0 * region.half + 4.0)
    _sun(Vector((0.0, -1.0, -0.3)), added, strength=3.6, side=0.8, ahead=-0.6, elevation=40.0)
    white, yellow, cyan, orange, blue = ((1.0, 1.0, 1.0, 1.0), (1.0, 0.85, 0.2, 1.0), (0.2, 0.9, 1.0, 1.0),
                                         (1.0, 0.5, 0.1, 1.0), (0.25, 0.45, 1.0, 1.0))
    top = 120.0
    h, inner = area.half, area.half - region.seam
    for size, color in ((h, white), (inner, yellow)):
        _line([(-size, -size), (size, -size), (size, size), (-size, size)], 0.9, color, added, z=top, closed=True)
    if region.lip is not None:
        lip = region.lip[(np.abs(region.lip[:, 0]) < region.half) & (np.abs(region.lip[:, 1]) < region.half)]
        _line([tuple(p) for p in lip[::4]], 0.8, cyan, added, z=top)
        river = region.river[(np.abs(region.river[:, 0]) < region.half) & (np.abs(region.river[:, 1]) < region.half)]
        _line([tuple(p) for p in river], 1.0, blue, added, z=top)
    for r in region.ridges:
        foot = r['foot'][(np.abs(r['foot'][:, 0]) < region.half) & (np.abs(r['foot'][:, 1]) < region.half)]
        _line([tuple(p) for p in foot], 0.8, orange, added, z=top)
        k = len(foot) // 2
        _text(r['id'], foot[k][0] + 12.0 * np.sign(foot[k][0] or 1.0), foot[k][1], 9.0, orange, added, z=top + 5.0)
    if area.boundary is not None:
        for i, is_open in enumerate(area.open_edges):
            a, b = area.boundary[i], area.boundary[(i + 1) % len(area.boundary)]
            _line([tuple(a), tuple(b)], 0.7, cyan if is_open else white, added, z=top + 1.0)
    _text('core (seam band in yellow)', h + 30.0, 0.0, 8.0, white, added, z=top + 5.0)
    if region.lip is not None:
        _text('canyon', 0.0, -region.half * 0.55, 14.0, white, added, z=top + 5.0)
    path = os.path.join(out_dir, 'plan_region.png')
    _render(path, (2048, 2048), samples=16)
    _cleanup(added)
    bpy.data.worlds.remove(world)
    log(f'terrain: preview {path}')


# The far trees' crowns on far_trees.png: color and radius (m) by mesh (Art/Models/Vegetation/FarTrees.py's reach).
FAR_TREE_LOOK = {'SM_FarPine_A': ((0.04, 0.3, 0.1, 1.0), 2.65, 'dark green'),
                 'SM_FarPine_B': ((0.3, 0.6, 0.12, 1.0), 3.4, 'green'),
                 'SM_FarBroadleaf': ((0.95, 0.8, 0.1, 1.0), 4.85, 'yellow')}


def render_far_trees(area, out_dir, log=print):
    """far_trees.png: a grounded area's far trees (region.farTrees) from above over the ring's whole square, each a
    crown to scale (dark green spires, green old pines, yellow cottonwoods), with the playable boundary (white), the
    line the trees start at (orange, near past it) and the corridors they keep off (blue), to judge their density."""
    data = _computed(area)
    trees = data.get('farTrees')
    spec = area.layout.get('region', {}).get('farTrees')
    if not trees or not spec:
        return
    import area_fartrees
    region = area.region
    scene = bpy.context.scene
    world = _world(scene)
    added = []
    _camera(Vector((0.0, 0.0, 800.0)), None, 50.0, added, ortho=2.0 * region.half + 4.0)
    _sun(Vector((0.0, -1.0, -0.3)), added, strength=3.6, side=0.8, ahead=-0.6, elevation=40.0)
    top = 130.0
    for name, items in trees['meshes'].items():
        if not items:
            continue
        color, reach, _ = FAR_TREE_LOOK.get(name, ((1.0, 0.0, 1.0, 1.0), 3.0, 'magenta'))
        verts, faces = [], []
        for x, y, _, _, scale in items:
            cx, cy, r = x / 100.0, y / 100.0, reach * scale
            base = len(verts)
            verts += [tuple(_to_b(cx + r * math.cos(t), cy + r * math.sin(t), top)) for t in np.linspace(
                0.0, 2.0 * math.pi, 8, endpoint=False)]
            faces.append(tuple(range(base, base + 8)))
        mesh = bpy.data.meshes.new('_FarTrees')
        mesh.from_pydata(verts, [], faces)
        mesh.materials.append(_material('_FarTree' + name, color, emission=0.5))
        obj = bpy.data.objects.new('_FarTrees_' + name, mesh)
        obj.visible_shadow = False
        bpy.context.scene.collection.objects.link(obj)
        added.append(obj)
    # The line they start at: where the distance to the boundary crosses near, every 2 m, as dots.
    near = spec.get('near', 15000) / 100.0
    c = np.arange(-region.half + 1.0, region.half, 2.0)
    gx, gy = np.meshgrid(c, c, indexing='ij')
    d = area_fartrees._segments_distance(np.column_stack([gx.ravel(), gy.ravel()]), area.boundary,
                                         closed=True).reshape(gx.shape)
    across = (d[:-1, :] - near) * (d[1:, :] - near) <= 0.0
    along = (d[:, :-1] - near) * (d[:, 1:] - near) <= 0.0
    edge = across[:, :-1] | along[:-1, :]
    dots, faces = [], []
    for i, j in zip(*np.nonzero(edge)):
        base = len(dots)
        dots += [tuple(_to_b(gx[i, j] + dx, gy[i, j] + dy, top)) for dx, dy in ((-0.9, -0.9), (0.9, -0.9), (0.9, 0.9),
                                                                               (-0.9, 0.9))]
        faces.append((base, base + 1, base + 2, base + 3))
    mesh = bpy.data.meshes.new('_NearLine')
    mesh.from_pydata(dots, [], faces)
    mesh.materials.append(_material('_NearLine', (1.0, 0.3, 0.02, 1.0), emission=0.8))
    obj = bpy.data.objects.new('_NearLine', mesh)
    obj.visible_shadow = False
    bpy.context.scene.collection.objects.link(obj)
    added.append(obj)
    for i in range(len(area.boundary)):
        a, b = area.boundary[i], area.boundary[(i + 1) % len(area.boundary)]
        _line([tuple(a), tuple(b)], 0.8, (1.0, 1.0, 1.0, 1.0), added, z=top + 1.0)
    for corridor in spec.get('clear', []):
        path = np.asarray(corridor['path'], dtype=np.float64) / 100.0
        _line([tuple(p) for p in path], corridor['width'] / 200.0, (0.25, 0.55, 1.0, 1.0), added, z=top - 1.0)
    # The legend in the empty band south of the boundary, where no tree stands.
    counts = ', '.join(f'{len(v)} {k[3:]} ({FAR_TREE_LOOK.get(k, (None, None, "magenta"))[2]})'
                       for k, v in trees['meshes'].items())
    south = float(area.boundary[:, 0].min())
    _text(f'far trees: {counts}', south - near * 0.4, 0.0, 10.0, (1.0, 1.0, 1.0, 1.0), added, z=top + 5.0)
    _text(f"from {near:g} m past the boundary (orange) to the ring's edge", south - near * 0.4 - 16.0, 0.0, 8.0,
          (1.0, 0.3, 0.02, 1.0), added, z=top + 5.0)
    path = os.path.join(out_dir, 'far_trees.png')
    _render(path, (2048, 2048), samples=16)
    _cleanup(added)
    bpy.data.worlds.remove(world)
    log(f'terrain: preview {path}')


# --- Clay views of the features ---

CLAY = (0.2, 0.19, 0.175, 1.0)
COURSE_COLORS = ((1.0, 0.42, 0.12, 1.0), (1.0, 0.85, 0.2, 1.0), (0.45, 0.9, 0.3, 1.0))  # bottom course first
ONE_COURSE = (0.85, 0.88, 0.95, 1.0)
FEATURE_TYPES = ('ridge', 'scarp', 'pit', 'mesa', 'plateau', 'knob', 'gully', 'creek')


def _band_material(color, alpha=0.5):
    """A see-through tint for a cliff course's band, faintly glowing so it reads in shade."""
    name = '_Band%.2f_%.2f_%.2f_%.2f' % (color[0], color[1], color[2], alpha)
    mat = bpy.data.materials.get(name)
    if mat:
        return mat
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Alpha'].default_value = alpha
    bsdf.inputs['Emission Color'].default_value = color
    bsdf.inputs['Emission Strength'].default_value = 0.35
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    return mat


def _box(name, x, y, z, yaw, length, width, height, material, added):
    """A box standing on (x, y, z) in layout meters: length along yaw, width across, height up."""
    mesh = bpy.data.meshes.new(name)
    hx, hy = width * 0.5, length * 0.5
    mesh.from_pydata([(-hx, -hy, 0), (hx, -hy, 0), (hx, hy, 0), (-hx, hy, 0),
                      (-hx, -hy, height), (hx, -hy, height), (hx, hy, height), (-hx, hy, height)], [],
                     [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)])
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    obj.location = _to_b(x, y, z)
    obj.rotation_euler = (0.0, 0.0, -math.radians(yaw))
    obj.visible_shadow = False
    bpy.context.scene.collection.objects.link(obj)
    added.append(obj)
    return obj


def _cliff_stand_ins(area, data, added, groups=None):
    """See-through bands where the cliff kit's pieces go (only the named groups' when given), standing just in front
    of the wall: one per course, as wide as the gap to the neighbors plus the overlap, each on its own course's foot,
    colored by course (bottom first: orange, yellow, green; a single piece faint blue) so stacked courses show. A
    knob's rock is a grey block."""
    rock = _material('_RockBlock', (0.3, 0.28, 0.27, 1.0))
    for group, points in data['cliffs'].items():
        if groups is not None and group not in groups:
            continue
        for i, point in enumerate(points):
            if point.get('kind') == 'bank':
                continue
            x, y, z = (v / 100.0 for v in point['location'])
            if point.get('kind') == 'outcrop':
                size = max(point['radius'] / 100.0, 2.0)
                _box('_Rock', x, y, z - 0.5, point['yaw'], size * 1.3, size, point['height'] / 100.0 + 0.5, rock,
                     added)
                continue
            gaps = [math.dist(point['location'][:2], points[j]['location'][:2]) / 100.0 for j in (i - 1, i + 1)
                    if 0 <= j < len(points)]
            width = min(min(gaps) if gaps else 10.0, 12.0) + 1.5
            if 'drop' in point and not point.get('courses'):
                pieces = [{'location': [x * 100.0, y * 100.0, (z - point['drop'] / 100.0) * 100.0],
                           'height': point['drop']}]
            else:
                pieces = point.get('courses') or [{'location': point['location'], 'height': point['height']}]
            a = math.radians(point['yaw'])
            for k, course in enumerate(pieces):
                cx, cy, cz = (v / 100.0 for v in course['location'])
                stacked = len(pieces) > 1
                color = COURSE_COLORS[k % len(COURSE_COLORS)] if stacked else ONE_COURSE
                _box('_Course', cx + 0.5 * math.cos(a), cy + 0.5 * math.sin(a), cz, point['yaw'], 0.2, width,
                     course['height'] / 100.0, _band_material(color, 0.5 if stacked else 0.28), added)


# The cliff kit's pieces (Art/Models/Rocks/Cliffs.py: height, width, depth in m, the pivot on the ground in the middle
# of the width and depth, the bottom sunk CLIFF_SINK below it) and how Tools/Unreal/build_area.py places them.
CLIFF_KIT = ((10.0, 12.0, 4.0), (8.0, 10.0, 3.6), (6.0, 15.0, 3.2), (12.0, 8.0, 4.2))
CLIFF_SINK = 1.3
CLIFF_INSET, CLIFF_OVERTOP, CLIFF_OVERLAP, CLIFF_COURSE_OVERLAP = 1.2, 0.2, 2.5, 0.3


def _piece_stand_ins(area, data, added):
    """Solid blocks where Tools/Unreal/build_area.py stands the cliff kit's pieces: per course, the piece nearest its
    height (the builder adds a little chance), set CLIFF_INSET into the wall from the course's foot, reaching the
    course's height plus the overtop, widened to overlap its neighbors, its bottom sunk below the foot. A knob's rock is
    a grey block. They cast shadows, as the pieces do."""
    rock = _material('_CliffPiece', (0.6, 0.55, 0.48, 1.0))
    knob = _material('_RockBlock', (0.3, 0.28, 0.27, 1.0))
    for group, points in data['cliffs'].items():
        for i, point in enumerate(points):
            kind = point.get('kind')
            if kind == 'bank':
                continue
            x, y, z = (v / 100.0 for v in point['location'])
            if kind == 'outcrop':
                size = max(point['radius'] / 100.0, 2.0)
                _box('_Rock', x, y, z - 0.5, point['yaw'], size * 1.3, size, point['height'] / 100.0 + 0.5, knob,
                     added).visible_shadow = True
                continue
            if 'drop' in point:
                bottom, height = z - point['drop'] / 100.0, point['drop'] / 100.0
            else:
                bottom, height = z, point.get('height', point.get('top', z * 100.0) - z * 100.0) / 100.0
            courses = point.get('courses') or [{'location': [x * 100.0, y * 100.0, bottom * 100.0],
                                                'height': height * 100.0}]
            gaps = [math.dist(point['location'][:2], points[j]['location'][:2]) / 100.0 for j in (i - 1, i + 1)
                    if 0 <= j < len(points)]
            gap = min(gaps) if gaps else 10.0
            a = math.radians(point['yaw'])
            for k, course in enumerate(courses):
                cx, cy, cz = (v / 100.0 for v in course['location'])
                reach = course['height'] / 100.0 + (CLIFF_OVERTOP if k == len(courses) - 1 else CLIFF_COURSE_OVERLAP)
                piece_height, piece_width, depth = min(CLIFF_KIT, key=lambda p: abs(math.log(max(reach, 0.1) / p[0])))
                width = min(max((gap + CLIFF_OVERLAP) / piece_width, 0.75), 1.6) * piece_width
                _box('_Piece', cx - math.cos(a) * CLIFF_INSET, cy - math.sin(a) * CLIFF_INSET, cz - CLIFF_SINK,
                     point['yaw'], depth, width, reach + CLIFF_SINK, rock, added).visible_shadow = True


def _steep_clay(steep):
    """Clay that turns red where the ground is steeper than steep degrees (its face's normal, in the world's Z up)."""
    mat = bpy.data.materials.new('_ClaySteep')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Roughness'].default_value = 0.8
    geometry = nodes.new('ShaderNodeNewGeometry')
    split = nodes.new('ShaderNodeSeparateXYZ')
    below = nodes.new('ShaderNodeMath')
    below.operation = 'LESS_THAN'
    below.inputs[1].default_value = math.cos(math.radians(steep))
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    sockets = {sk.identifier: sk for sk in mix.inputs}
    sockets['A_Color'].default_value = CLAY
    sockets['B_Color'].default_value = (0.75, 0.05, 0.03, 1.0)
    links.new(geometry.outputs['Normal'], split.inputs['Vector'])
    links.new(split.outputs['Z'], below.inputs[0])
    links.new(below.outputs['Value'], sockets['Factor_Float'])
    links.new(next(sk for sk in mix.outputs if sk.identifier == 'Result_Color'), bsdf.inputs['Base Color'])
    return mat


def _clay_terrain(area, steep=None):
    """Gives the terrain's own meshes (not the water or the backdrop) a clay material (red where steeper than steep
    degrees, when given); returns what puts theirs back."""
    clay = _steep_clay(steep) if steep else _material('_ClayTerrain', CLAY)
    saved = []
    for obj in bpy.data.objects:
        if obj.type != 'MESH' or not obj.name.startswith(area.name + '_') or obj.name.endswith('_Water') \
                or '_Backdrop_' in obj.name:
            continue
        saved.append((obj, [slot.material for slot in obj.material_slots]))
        for slot in obj.material_slots:
            slot.material = clay

    def restore():
        for obj, materials in saved:
            for slot, material in zip(obj.material_slots, materials):
                slot.material = material
    return restore


def _unit(v):
    v = np.asarray(v, dtype=np.float64)
    return v / max(float(np.linalg.norm(v)), 1e-9)


def _feature_frames(area):
    """(id, the eye (x, y, z), the point looked at (x, y, z), its cliff groups) for every step 3b feature and every
    falls: each seen from the side its cliff faces (a pit from above its ramp's start, a plateau or mesa from the
    valley's middle), back far enough to frame it, the view centered on its height range."""
    frames = []
    for f in area.layout['features']:
        kind = f['type']
        if kind not in FEATURE_TYPES or (kind == 'creek' and not f.get('falls')):
            continue
        if kind == 'creek':
            gorge = np.asarray(f['falls']['gorge']['path'], dtype=np.float64) / 100.0
            lip = gorge[0]
            away = _unit(gorge[min(2, len(gorge) - 1)] - lip)
            side = np.array([-away[1], away[0]])
            eye = lip + away * 24.0 + side * 9.0
            zl = float(area.height(*lip))
            group = f['falls']['gorge'].get('cliffGroup', f['falls']['gorge'].get('id', f['id'] + 'Gorge'))
            frames.append((f['id'], (eye[0], eye[1], zl + 9.0), (lip[0], lip[1], zl - 4.0), {group}))
            continue
        if 'polygon' in f:
            pts = np.asarray(f['polygon'], dtype=np.float64) / 100.0
        elif 'path' in f:
            pts = np.asarray(f['path'], dtype=np.float64) / 100.0
            if kind == 'gully':
                # A long gully: its middle stretch.
                curve = catmull_rom(pts, step=1.0)
                pts = curve[len(curve) // 3:2 * len(curve) // 3 + 1]
        else:
            pts = np.asarray([f['center']], dtype=np.float64) / 100.0
        whole = np.vstack([pts, np.asarray(f['ramp']['path'], dtype=np.float64) / 100.0]) if f.get('ramp') else pts
        lo, hi = whole.min(axis=0), whole.max(axis=0)
        center = (lo + hi) * 0.5
        size = float(max(np.max(hi - lo), 8.0))
        toward_middle = _unit(-center) if np.linalg.norm(center) > 5.0 else np.array([-0.7, 0.7])
        elevation = 28.0
        if kind in ('ridge', 'scarp', 'gully'):
            mid = len(pts) // 2
            t = _unit(pts[min(mid + 1, len(pts) - 1)] - pts[max(mid - 1, 0)])
            right = np.array([-t[1], t[0]])
            if kind == 'ridge' and f.get('cliffs', 'both') in ('left', 'right'):
                out = right if f['cliffs'] == 'right' else -right
            elif kind == 'scarp':
                out = -right if f.get('side', 'left') == 'right' else right  # the face looks to the low side
            else:
                out = right if right @ toward_middle > 0.0 else -right
            if kind == 'gully':
                elevation = 24.0
        elif kind == 'pit':
            ramp = np.asarray(f['ramp']['path'], dtype=np.float64) / 100.0 if f.get('ramp') else None
            out = _unit(ramp[0] - center) if ramp is not None else toward_middle
            elevation = 40.0
        else:
            out = toward_middle
        distance = min(max(size * 0.8 + 10.0, 18.0), 60.0)
        eye = center + out * distance
        if kind == 'gully':
            # Down its middle stretch from just past its upper end, low over the channel.
            along = _unit(pts[-1] - pts[0])
            eye, center, distance, elevation = pts[0] - along * 10.0, pts[0] + along * 18.0, 12.0, 24.0
        around = np.vstack([whole, whole + out * 6.0])
        heights = area.height(around[:, 0], around[:, 1])
        zt = 0.5 * (float(np.max(heights)) + float(np.min(heights)))
        ze = zt + distance * math.tan(math.radians(elevation))
        groups = {f.get('cliffGroup', f['id'])}
        if f.get('ramp'):
            groups.add(f['ramp'].get('cliffGroup', f['ramp'].get('id', f['id'] + '_ramp')))
        frames.append((f['id'], (eye[0], eye[1], ze), (center[0], center[1], zt), groups))
    return frames


def render_features(area, out_dir, log=print):
    """feature_<id>.png: a clay view of each step 3b feature (those preview.features lists, when it does), with the
    cliff kit's courses as see-through bands."""
    frames = _feature_frames(area)
    wanted = area.layout.get('preview', {}).get('features')
    if wanted is not None:
        frames = [f for f in frames if f[0] in wanted]
    if not frames:
        return
    scene = bpy.context.scene
    world = _world(scene)
    data = _computed(area)
    restore = _clay_terrain(area)
    try:
        for fid, eye, target, groups in frames:
            added = []
            _cliff_stand_ins(area, data, added, groups)
            cam_eye, aim = _to_b(*eye), _to_b(*target)
            _camera(cam_eye, aim, 28.0, added, far=20000.0)
            _sun((aim - cam_eye).normalized(), added, strength=4.6, elevation=40.0)
            path = os.path.join(out_dir, f'feature_{fid}.png')
            _render(path, (1400, 900), samples=32)
            _cleanup(added)
            log(f'terrain: preview {path}')
    finally:
        restore()
        bpy.data.worlds.remove(world)


def render_all(area, out_dir, log=print):
    os.makedirs(out_dir, exist_ok=True)
    render_plan(area, out_dir, log)
    if area.setting == 'grounded':
        render_region(area, out_dir, log)
        render_far_trees(area, out_dir, log)
    render_features(area, out_dir, log)
    render_views(area, out_dir, log)
