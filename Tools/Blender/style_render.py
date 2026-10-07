"""Renders a scripted model for the art-style exploration (Docs/Art/StyleExploration/README.md): builds the model
headlessly in an empty scene, lights it with one sun and a soft sky over a grass plane, and renders it with Cycles on
the CPU into the passes style_compose.py composes the styles from (linear EXR), plus the plain beauty PNG (the current
look, AgX view transform).

    blender -b --factory-startup --python Tools/Blender/style_render.py -- <model.py> <outdir> [--test] [--no-ao]
    python Tools/Blender/style_render.py <model.py> <outdir> [--test] [--no-ao]       (with the pip bpy module)

Pass 1 ("full"): sun + sky: Image, Alpha, Depth, Mist, Normal (world space), DiffCol, DiffDir, DiffInd, GlossDir, AO,
IndexMA (material pass index) and Freestyle (clean ink lines as their own pass). Pass 2 ("sun"): the sun alone, few
samples: Image, DiffCol, DiffDir (hard sun shading and shadows) and Freestyle again with a sketchy line style.
scene.json records the sun and camera directions for the compositor. --test renders small and fast (no Freestyle) to
check the framing. The full render takes about two and a half minutes on four cores at 1600x900.
"""
import json
import math
import os
import runpy
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))

import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import looter_textures as lt  # noqa: E402

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
model_path, outdir = argv[0], argv[1]
TEST = '--test' in argv
if not os.path.isabs(model_path):
    model_path = os.path.join(REPO, model_path)
if not os.path.isabs(outdir):
    outdir = os.path.join(REPO, outdir)
os.makedirs(outdir, exist_ok=True)

# Build the model in an empty scene, as models.ps1 does. The kit reads its own flags after '--' in sys.argv.
bpy.ops.wm.read_factory_settings(use_empty=True)
sys.argv = ['blender'] + (['--', '--no-ao'] if '--no-ao' in argv else [])
t = time.time()
runpy.run_path(model_path, run_name='__main__')
print(f'RENDER: built {os.path.basename(model_path)} in {time.time() - t:.1f} s', flush=True)
scene = bpy.context.scene
view_layer = bpy.context.view_layer

WIDTH, HEIGHT = (480, 270) if TEST else (1600, 900)
SAMPLES_FULL = 8 if TEST else 160
SAMPLES_SUN = 4 if TEST else 32
VIEW = Vector((-1.0, -1.45, 0.40))      # camera direction from the model: front-left, a little above eye level
LENS = 32.0
FIT = 0.78

# --- The model: its render meshes, hulls hidden ---
roots = [o for o in scene.objects if o.parent is None and o.type == 'MESH' and not o.name.startswith(('_', 'UCX_'))]
shown = []
for root in roots:
    for o in [root] + list(root.children_recursive):
        if o.type == 'MESH':
            if o.name.startswith(('UCX_', 'USP_', 'UCP_', '_')):
                o.hide_render = True
            else:
                shown.append(o)
lo, hi = lt._bounds(shown)
center = (lo + hi) * 0.5
radius = max((hi - lo).length * 0.5, 0.05)
print(f'RENDER: model bounds {tuple(round(v, 2) for v in lo)} .. {tuple(round(v, 2) for v in hi)}', flush=True)

# Material pass indices (for flat fills per material in the stylizer) and their average colors.
materials = {}
for i, mat in enumerate(m for m in bpy.data.materials if m.users):
    mat.pass_index = i + 1
    set_name = mat.get('TextureSet')
    info = lt.SETS.get(set_name, {}) if set_name else {}
    base = list(mat.diffuse_color)[:3]
    materials[i + 1] = dict(name=mat.name, set=set_name, color_linear=base, master=mat.get('Master'),
                            kind=mat.get('Kind'), glow=float(mat.get('Glow', 0.0)))
with open(os.path.join(outdir, 'materials.json'), 'w') as f:
    json.dump(materials, f, indent=1)

# --- Ground: a big grass plane under the model, textured with the game's GroundGrass set ---
ground_mat = lt.material('GroundGrass')
ground_mat.pass_index = 100
materials[100] = dict(name='GroundGrass', set='GroundGrass', color_linear=list(ground_mat.diffuse_color)[:3])
with open(os.path.join(outdir, 'materials.json'), 'w') as f:
    json.dump(materials, f, indent=1)
size = 600.0
mesh = bpy.data.meshes.new('_Ground')
mesh.from_pydata([(-size, -size, 0.0), (size, -size, 0.0), (size, size, 0.0), (-size, size, 0.0)], [], [(0, 1, 2, 3)])
uv = mesh.uv_layers.new(name='UVMap')
tile = 4.0  # 1024 px at 256 px/m
for loop in mesh.loops:
    co = mesh.vertices[loop.vertex_index].co
    uv.data[loop.index].uv = (co.x / tile, co.y / tile)
col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
for c in col.data:
    c.color = (1.0, 1.0, 1.0, 1.0)
mesh.materials.append(ground_mat)
ground = bpy.data.objects.new('_Ground', mesh)
ground.location = (0.0, 0.0, min(0.0, roots[0].matrix_world.translation.z))  # the pivot is ground level; the plinth dips below it
scene.collection.objects.link(ground)

# --- Camera: a player's-eye three-quarter view from the front left, standing a little up-slope ---
CAM_DIST = max(4.0, radius * 2.38)   # the cottage (radius 6.1 m) sits 14.5 m away, seen from 2.4 m up
CAM_Z = max(1.2, radius * 0.39)
AIM_Z = radius * 0.56
LENS = 30.0
camera = bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
camera.data.lens = LENS
camera.data.sensor_fit = 'HORIZONTAL'
scene.collection.objects.link(camera)
direction = VIEW.normalized()
flat = Vector((VIEW.x, VIEW.y, 0.0)).normalized()
camera.location = Vector((center.x, center.y, 0.0)) + flat * CAM_DIST + Vector((0.0, 0.0, CAM_Z))
aim = Vector((center.x, center.y, AIM_Z))
camera.rotation_euler = (aim - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera.data.clip_start = 0.05
camera.data.clip_end = 5000.0
scene.camera = camera
distance = CAM_DIST

# --- Sun: a key light about 55 degrees left of the camera's axis, 33 degrees up: the long side is bright, the
# front gable half lit, and the shadow falls across the grass to the right of the house where the camera sees it ---
toward_sun = Vector((-0.92, -0.38, 0.0)).normalized()
elev = math.radians(33.0)
toward_sun = (toward_sun * math.cos(elev) + Vector((0.0, 0.0, math.sin(elev)))).normalized()
sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
sun.data.energy = 4.5
sun.data.color = (1.0, 0.92, 0.80)
sun.data.angle = math.radians(1.2)
sun.rotation_euler = (-toward_sun).to_track_quat('-Z', 'Y').to_euler()
scene.collection.objects.link(sun)

with open(os.path.join(outdir, 'scene.json'), 'w') as f:
    json.dump(dict(sun=list(toward_sun), cam_forward=list((aim - camera.location).normalized()), horizon_hint=None), f)

# --- World: the kit's soft afternoon sky (what the current previews use) ---
world = lt._preview_world(scene)
scene.world = world
world.mist_settings.start = distance * 0.5
world.mist_settings.depth = distance * 6.0
world.mist_settings.falloff = 'LINEAR'
bg_strength = next(n for n in world.node_tree.nodes if n.type == 'BACKGROUND').inputs['Strength']

# --- Cycles ---
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.use_adaptive_sampling = True
scene.cycles.adaptive_threshold = 0.015
scene.cycles.use_denoising = False
scene.cycles.max_bounces = 4
scene.cycles.diffuse_bounces = 2
scene.cycles.glossy_bounces = 2
scene.cycles.transparent_max_bounces = 8
scene.cycles.caustics_reflective = False
scene.cycles.caustics_refractive = False
scene.cycles.blur_glossy = 1.0
scene.cycles.sample_clamp_indirect = 8.0
scene.cycles.pixel_filter_type = 'BLACKMAN_HARRIS'
scene.cycles.filter_width = 1.2
scene.render.resolution_x, scene.render.resolution_y = WIDTH, HEIGHT
scene.render.resolution_percentage = 100
scene.render.film_transparent = True
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.view_settings.view_transform = 'AgX'
scene.view_settings.look = 'AgX - Medium High Contrast'
scene.view_settings.exposure = 0.0

# Passes.
vl = view_layer
vl.use_pass_combined = True
vl.use_pass_z = True
vl.use_pass_mist = True
vl.use_pass_normal = True
vl.use_pass_diffuse_color = True
vl.use_pass_diffuse_direct = True
vl.use_pass_diffuse_indirect = True
vl.use_pass_glossy_direct = True
vl.use_pass_ambient_occlusion = True
vl.use_pass_material_index = True
vl.cycles.use_pass_shadow_catcher = False
scene.cycles.ao_distance = 1.5 if hasattr(scene.cycles, 'ao_distance') else None
world.light_settings.distance = 1.5

# Freestyle as its own pass.
scene.render.use_freestyle = not TEST
scene.render.line_thickness_mode = 'ABSOLUTE'
scene.render.line_thickness = 1.0
vl.use_freestyle = not TEST
fs = vl.freestyle_settings
fs.as_render_pass = True
fs.crease_angle = math.radians(140.0)
fs.use_culling = True
fs.use_smoothness = False
for ls in list(fs.linesets):
    fs.linesets.remove(ls)
ls = fs.linesets.new('Ink')
ls.select_silhouette = True
ls.select_border = True
ls.select_crease = True
ls.select_material_boundary = True
ls.select_contour = False
ls.select_external_contour = False
ls.select_edge_mark = False
ls.select_by_visibility = True
ls.visibility = 'VISIBLE'
style = ls.linestyle
style.name = 'InkClean'
style.color = (0.0, 0.0, 0.0)
style.alpha = 1.0
style.thickness = 2.2
style.thickness_position = 'CENTER'
style.use_chaining = True
style.chaining = 'PLAIN'
style.use_same_object = False
style.use_length_min = True
style.length_min = 6.0
style.caps = 'ROUND'


def file_output(prefix, passes):
    """A compositor File Output node writing each pass as a 32-bit linear EXR named <prefix><pass>0001.exr."""
    scene.use_nodes = True
    scene.render.use_compositing = True
    tree = scene.node_tree
    tree.nodes.clear()
    rl = tree.nodes.new('CompositorNodeRLayers')
    rl.layer = vl.name
    composite = tree.nodes.new('CompositorNodeComposite')
    tree.links.new(rl.outputs['Image'], composite.inputs['Image'])
    fo = tree.nodes.new('CompositorNodeOutputFile')
    fo.base_path = outdir
    fo.format.file_format = 'OPEN_EXR'
    fo.format.color_depth = '32'
    fo.format.exr_codec = 'ZIP'
    fo.format.color_mode = 'RGBA'
    fo.file_slots.clear()
    available = {s.name for s in rl.outputs if s.enabled}
    for name in passes:
        if name not in available:
            print(f'RENDER: pass {name} not available (have {sorted(available)})', flush=True)
            continue
        fo.file_slots.new(prefix + name)
        tree.links.new(rl.outputs[name], fo.inputs[prefix + name])
    return fo


import time  # noqa: E402

# ---- Pass 1: full lighting ----
scene.cycles.samples = SAMPLES_FULL
bg_strength.default_value = 1.0
passes = ['Image', 'Alpha', 'Depth', 'Mist', 'Normal', 'DiffCol', 'DiffDir', 'DiffInd', 'GlossDir', 'AO', 'IndexMA']
if not TEST:
    passes.append('Freestyle')
file_output('full_', passes)
scene.render.filepath = os.path.join(outdir, 'beauty_current.png')
t = time.time()
bpy.ops.render.render(write_still=True)
print(f'RENDER: full pass in {time.time() - t:.0f} s -> {scene.render.filepath}', flush=True)

# ---- Pass 2: sun only, sketchy lines ----
scene.cycles.samples = SAMPLES_SUN
scene.cycles.use_adaptive_sampling = False
bg_strength.default_value = 0.0
sun.data.angle = math.radians(0.6)
style.name = 'InkSketch'
style.thickness = 1.6
style.use_length_min = True
style.length_min = 4.0
mod = style.geometry_modifiers.new('Backbone', 'BACKBONE_STRETCHER')
mod.backbone_length = 6.0
noise = style.geometry_modifiers.new('Perlin', 'PERLIN_NOISE_2D')
noise.frequency = 10.0
noise.amplitude = 1.6
noise.octaves = 2
noise.seed = 7
along = style.thickness_modifiers.new('Along', 'ALONG_STROKE')
along.mapping = 'CURVE'
along.value_min = 0.5
along.value_max = 1.4
along.blend = 'MULTIPLY'
file_output('sun_', ['Image', 'DiffCol', 'DiffDir', 'Freestyle'] if not TEST else ['Image', 'DiffCol', 'DiffDir'])
scene.render.filepath = os.path.join(outdir, 'beauty_sunonly.png')
t = time.time()
bpy.ops.render.render(write_still=True)
print(f'RENDER: sun pass in {time.time() - t:.0f} s', flush=True)
print('RENDER: done', flush=True)
