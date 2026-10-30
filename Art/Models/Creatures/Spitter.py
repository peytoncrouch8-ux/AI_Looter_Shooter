"""The Venom Spitter (ASpitterSpider, 2026-10-12): the brown spider's ranged cousin (Docs/Polish/BorderlandsComparison.md,
item 8), a spider that keeps its distance and lobs venom from a swollen sac on its back.

A variant of Art/Models/Creatures/Spider.py, built from it rather than copied: this script runs the spider's own parts code
(everything above its "# --- The rig ---" line) with thinner legs, adds the sac, and builds its own rig the spider's way.
So the bones, the legs' lengths and layout and the standing pose are the spider's, and ASpiderCreature's gait, leg IK,
attack rear and death curl drive it unchanged. A change to the spider's body comes through here on the next export.

What's its own:
  - Thinner legs: LEG_RADIUS 6.2 against the spider's 7.8 (lengths and joints unchanged, so the rig is the same).
  - An olive hide: the spider's own atlas (Art/Textures/SpiderBody, not painted again here) through a tint,
    MI_SpitterBody on the World master. No new textures.
  - The venom sac on the abdomen's back: a swollen, slightly lumpy translucent bulb (MI_SpitterSac on the Gel master: a
    venom-green Tint, denser than the slime's gel, a resting InnerGlow that ASpitterSpider drives brighter as it tells a
    spit), dark veins over it and a fleshy collar of hide where it grows out of the abdomen. The abdomen's bristles under
    it are left out (they'd show through the gel).
  - Bone sac, a child of abdomen at the sac's root: the game swells the sac from there and reads it as a critical spot.
    Its hit hull holds the sac, its veins and its collar (PA_Spitter's sac body); every other bone's hull is the spider's.

The Tripo remake (LOOTER_SPITTER_TRIPO, default on; =0 builds the scripted one above): the user's Tripo model
(Art/Models/Creatures/Tripo/Spitter.glb) made over by Tripo/parts_spitter*.py: levelled (it stood 6 degrees nose-down),
its cephalothorax and its sac (the whole abdomen, lime gel, the sac bone) rebuilt as clean quads on its shape, eight
legs (it had six) on the spider's bones in their IK rest pose wearing one of its legs' profile and paint, its palps and
fangs rebuilt where they were, its paint re-baked into our look (Art/Textures/TripoSpitter, MI_SpitterTripoBody); veins,
a collar of hide tongues and a dark venom core built on the sac. The fangs', palps' and abdomen's bones start where this
body has them; the legs' and the body's are the spider's, so the gait, IK, rear and death curl drive it unchanged.

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Spitter.py -Preview     (Saved/ArtPreviews/Creatures/Spitter.png)
    Tools\\artrun.ps1 -Export Spitter -Family Spitter                      (Intermediate/ArtExport_Spitter, no import)
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

# The user's Tripo remake (2026-10-29) unless LOOTER_SPITTER_TRIPO=0 (the script-built Spitter, exported as before).
TRIPO = os.environ.get('LOOTER_SPITTER_TRIPO', '1') != '0'
TRIPO_DIR = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo')
SPIDER = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Spider.py')
RIG_MARK = '# --- The rig ---'
LEG_LINE = 'LEG_RADIUS = 7.8'
LEG_RADIUS = 6.2

# The hide: the brown spider's atlas, tinted to a dry olive (its browns go mossy, its pale marks khaki). 0x9CC07C until
# the spider's coat went darker and cooler (2026-10-22): raised so the Spitter keeps its own olive (its pale leg
# bands come out khaki).
HIDE_TINT = 0xD8E08A
# The sac's gel: venom green, mostly see-through facing the eye and dense at its edges, glowing a little at rest.
SAC_TINT, SAC_OPACITY, SAC_EDGE, SAC_GLOW = 0x8FE83A, 0.55, 0.93, 0.6
# Creature space as the spider's (cm, x forward, y left), z over the ride height (Spider.py's lifted()).
SAC = dict(center=(-86.0, 0.0, 54.0), radii=(42.0, 36.0, 33.0))
SAC_ROOT = (-86.0, 0.0, 37.0)                      # the sac bone's head: where it grows out of the abdomen
COLLAR = dict(center=(-86.0, 0.0, 39.0), radii=(42.0, 35.0, 6.0))
VEINS = 6
VEIN_RADIUS = (1.5, 0.5)                           # at the sac's foot, at the vein's end near its top


def spider_parts():
    """Runs Spider.py's parts code (above its rig) with thinner legs and returns its names: the parts, bristles and bones
    not yet skinned. The spider's texture sets aren't painted again (lc.paint_atlas is switched off meanwhile, as a review
    stage switches it off), so this never touches Art/Textures."""
    with open(SPIDER, encoding='utf-8') as source:
        code = source.read()
    head, mark, _ = code.partition(RIG_MARK)
    if not mark:
        raise RuntimeError(f'{SPIDER} has no "{RIG_MARK}" line: Spitter.py builds on the spider\'s parts above it')
    if head.count(LEG_LINE) != 1:
        raise RuntimeError(f'{SPIDER} has no single "{LEG_LINE}" line: update Spitter.py\'s LEG_LINE to the spider\'s')
    head = head.replace(LEG_LINE, f'LEG_RADIUS = {LEG_RADIUS}')
    painter = lc.paint_atlas
    lc.paint_atlas = lambda *args, **kwargs: None
    names = {'__name__': 'spider_parts', '__file__': SPIDER}
    try:
        exec(compile(head, SPIDER, 'exec'), names)
    finally:
        lc.paint_atlas = painter
    return names


def gel_material():
    """The sac: a Gel master material with a tint, an opacity, its edge's and a resting glow (no texture). Its nodes only
    preview it in Blender."""
    mat = bpy.data.materials.get('SpitterSac') or bpy.data.materials.new('SpitterSac')
    mat.use_nodes = True
    rgba = lm.hex_color(SAC_TINT)
    bsdf = next(node for node in mat.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = rgba
    bsdf.inputs['Alpha'].default_value = SAC_OPACITY
    bsdf.inputs['Roughness'].default_value = 0.1
    if 'Emission Color' in bsdf.inputs:
        bsdf.inputs['Emission Color'].default_value = rgba
        bsdf.inputs['Emission Strength'].default_value = SAC_GLOW
    mat.diffuse_color = rgba[:3] + [SAC_OPACITY]
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Gel'
    mat['Tint'] = f'#{SAC_TINT:06x}'
    mat['Opacity'] = SAC_OPACITY
    mat['EdgeOpacity'] = SAC_EDGE
    mat['InnerGlow'] = SAC_GLOW
    return mat


def sac_shape(a, s, u):
    """The bulb: a few soft lobes round it and a little unevenness along it, so it reads as swollen flesh rather than a
    ball; its underside flattened where it sits on the abdomen."""
    k = 1.0 + 0.05 * math.sin(3.0 * math.atan2(s, u) + 0.4) * (1.0 - a * a) + 0.03 * math.sin(5.0 * a + 1.0)
    return a * k, s * k, u * k * (0.8 if u < 0.0 else 1.0)


def sac_point(center, theta, phi, out=1.0):
    """A point on the sac's surface (theta from its top, phi round it), pushed out by out."""
    a, s, u = sac_shape(math.sin(theta) * math.cos(phi), math.sin(theta) * math.sin(phi), math.cos(theta))
    rx, ry, rz = SAC['radii']
    return center + Vector((a * rx, s * ry, u * rz)) * out


def hull_of_points(rig, bone, points):
    """A convex hit zone on bone round points (Blender meters): the sac's and the abdomen's halves of one bulb."""
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    hull = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in hull['geom_interior'] + hull['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    return lm._hit_shape('UCX_', rig, bone, bm, Matrix.Identity(4))


spider = spider_parts()
ATLAS = spider['ATLAS']
lifted = spider['lifted']
parts, hairs, bones = spider['parts'], spider['hairs'], spider['bones']


def scripted():
    """The script-built Spitter (LOOTER_SPITTER_TRIPO=0): the spider's parts, olive, with the scripted sac. Returns
    (rig, mesh)."""
    hide = lt.material('SpiderBody', name='SpitterBody', tint=HIDE_TINT)
    gel = gel_material()

    # --- The sac ---
    sac_center = lifted(SAC['center'])
    sac = lc.Mesh(ATLAS)
    lc.ellipsoid(sac, sac_center, SAC['radii'], 'misc', fwd=(1, 0, 0), up=(0, 0, 1), segs=32, rings=18, deform=sac_shape,
                 patch=True)
    collar = lc.Mesh(ATLAS)
    lc.ellipsoid(collar, lifted(COLLAR['center']), COLLAR['radii'], 'abdomen', fwd=(1, 0, 0), up=(0, 0, 1), segs=28, rings=8)
    # Veins over the bulb from its foot toward its top, each wandering a little, dark on the hide's darkest patch.
    veins = lc.Mesh(ATLAS)
    for k in range(VEINS):
        phi0 = 2.0 * math.pi * k / VEINS + 0.3
        reach = 0.35 + 0.12 * ((k * 7) % 3)
        steps = [i / 8 for i in range(9)]
        pts = [sac_point(sac_center, math.radians(112.0) - t * (math.radians(112.0) - reach),
                         phi0 + 0.18 * math.sin(t * 5.0 + k), 1.015) for t in steps]
        radii = [VEIN_RADIUS[0] + (VEIN_RADIUS[1] - VEIN_RADIUS[0]) * t for t in steps]
        lc.tube(veins, pts, radii, 'misc', sides=6, patch=True)
    bones.append(('sac', lifted(SAC_ROOT), lifted(SAC_ROOT) + Vector((0.0, 0.0, 30.0)), 'abdomen'))

    # The abdomen's bristles under the sac would show through the gel: those inside it (and its collar) go.
    under = hairs['abdomen'].bm
    rx, ry, rz = SAC['radii']
    inside = [v for v in under.verts
              if ((v.co.x - sac_center.x) / (rx * 1.1)) ** 2 + ((v.co.y - sac_center.y) / (ry * 1.1)) ** 2
              + ((v.co.z - sac_center.z) / (rz * 1.25)) ** 2 < 1.0]
    bmesh.ops.delete(under, geom=inside, context='VERTS')

    # --- The rig (the spider's, with the sac) ---
    rig = lm.armature()
    lm.bones(rig, [(name, lc.to_blender(head), lc.to_blender(tail), parent) for name, head, tail, parent in bones])
    skin = {}
    for bone, mesh in parts.items():
        skin[bone] = [mesh.finish(bone.title().replace('_', ''), hide)]
        if bone in hairs:
            skin[bone].append(hairs.pop(bone).finish(bone.title().replace('_', '') + 'Hair', hide))
        lm.hit_hull(rig, bone, skin[bone])
    skin['sac'] = [sac.finish('Sac', gel, sharp=80.0), collar.finish('SacCollar', hide), veins.finish('SacVeins', hide)]
    lm.hit_hull(rig, 'sac', skin['sac'])
    return rig, lm.skin(rig, 'Spitter', skin)


def tripo():
    """The user's Tripo remake (Art/Models/Creatures/Tripo/parts_spitter*.py): its cephalothorax, sac, palps and fangs
    rebuilt in clean quads, eight legs on the spider's bones in its IK rest pose, its paint baked in our look. The legs'
    bones are the spider's; the fangs' and palps' start at their roots on this head, the abdomen's at the waist (the
    pedicel's body end: ASpiderCreature reads every pivot from the skeleton), the sac's there too (it swells from the
    waist, and dead it shrinks onto the body rather than hanging on a bare stalk). The sac's
    hit zone is its upper part (PA_Spitter's sac body, critical), the abdomen's its lower. Returns (rig, mesh)."""
    sys.path.insert(0, TRIPO_DIR)
    import parts_spitter as shape
    import parts_spitter_limbs as limbs
    import parts_spitter_look as look
    import parts_spitter_sac as sacs
    for mesh in list(parts.values()) + list(hairs.values()):
        mesh.bm.free()
    gel = gel_material()
    m = limbs.measure()
    roots = limbs.roots(m)
    waist = Vector(m['place'].root_c)
    moved = []
    for name, head, tail, parent in bones:
        if name in roots:
            head, tail = Vector(roots[name]), Vector(roots[name]) + (tail - head)
        elif name == 'abdomen':
            head, tail = waist, waist + (tail - head)
        moved.append((name, head, tail, parent))
    moved.append(('sac', waist, waist + Vector((0.0, 0.0, 30.0)), 'abdomen'))
    bones[:] = moved
    pieces = {}
    for group in (limbs.legs(m, bones), limbs.palps_fangs(m), sacs.body_parts(m), sacs.sac_parts(m),
                  {'head': limbs.eyes_parts(m)}):
        for bone, plist in group.items():
            pieces.setdefault(bone, []).extend(plist)
    sources = [shape.GLB] + [os.path.join(TRIPO_DIR, f'parts_spitter{s}.py')
                             for s in ('', '_limbs', '_sac', '_look', '_paint')]
    body = look.texture(m, pieces, sources)

    rig = lm.armature()
    lm.bones(rig, [(name, lc.to_blender(head), lc.to_blender(tail), parent) for name, head, tail, parent in bones])
    skin = {}
    for bone, plist in pieces.items():
        skin[bone] = [look.to_object(p, 'V', gel if p.look == 'gel' else body) for p in plist]
        if bone != 'sac':
            lm.hit_hull(rig, bone, skin[bone])
    # The bulb's hit zones: the sac above its middle (with a little overlap), the abdomen below.
    pts = [Vector(v.co) for o in skin['sac'] for v in o.data.vertices]
    middle = sum(p.z for p in pts) / len(pts)
    span = max(p.z for p in pts) - min(p.z for p in pts)
    hull_of_points(rig, 'sac', [p for p in pts if p.z > middle - span * 0.1])
    hull_of_points(rig, 'abdomen', [p for p in pts if p.z < middle + span * 0.05])
    for o in [o for o in bpy.data.objects if o.name.startswith('_')]:
        bpy.data.objects.remove(o)
    return rig, lm.skin(rig, 'Spitter', skin)


rig, spitter = tripo() if TRIPO else scripted()
# As the spider: no baked vertex occlusion (the parts move against each other), vertex alpha 1.
colors = spitter.data.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
colors.data.foreach_set('color', [1.0] * (4 * len(spitter.data.loops)))
rig['LODs'] = '50,25'
rig['LODScreens'] = '0.3,0.12'
lt._log(f'Spitter{" (Tripo)" if TRIPO else ""}: {sum(len(p.vertices) - 2 for p in spitter.data.polygons)} triangles, '
        f'{len(bones)} bones, materials {", ".join(m.name for m in spitter.data.materials)}')

if lt.want_preview():
    lt.preview([spitter], lt.preview_path('Creatures', 'Spitter'), view=(0.7, -1.0, 0.55))
