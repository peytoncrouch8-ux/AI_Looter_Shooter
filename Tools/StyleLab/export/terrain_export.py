"""Style Lab export, the terrain: from Saved/StyleLab/work/terrain.blend (terrain_build.py: the game's own generator)
writes what the page draws and walks on, in three's axes (x = -Y/100, y = Z/100, z = X/100):

    models/terrain_near.glb   Terrain_Near: the core's tiles inside the region (the game's own mesh: its triangles are
                              what Unreal draws at every quality), material "Terrain"
    models/terrain_far.glb    Terrain_Far (the rest of the core, "Terrain"), Terrain_Water ("Water"), Terrain_Ring
                              ("TerrainRing"), Terrain_CanyonWall ("RockCliff_Ransom") and Terrain_Backdrop_<n>
                              ("Backdrop<n>", unlit silhouettes)
    terrain/heights.bin       the ground's height over the core square, from the same triangles
    tex/Terrain_Macro.webp, Terrain_RingMacro.webp, Terrain_Masks.webp, Terrain_Scatter.webp, Terrain_Scrub.webp
                              the area's maps turned to the lab's convention: row 0 at z0, column 0 at x0

Near and far share their seam's vertices exactly (one triangulation cut in two), and the core meets the ring with no
gap (the generator's seam). TEXCOORD_0 is the macro map's uv over macroRect in the lab's convention (u = (x - x0) /
(x1 - x0), v = (z - z0) / (z1 - z0), for a texture loaded with flipY = false); TEXCOORD_1 is (x, z) in metres for the
detail layers. COLOR_0.a is the generator's baked occlusion.

    /home/user/bpyenv/bin/python Tools/StyleLab/export/terrain_export.py
"""
import os
import sys
import time

sys.dont_write_bytecode = True
import labcommon as lc  # noqa: E402

lc.add_paths()
lc.install_write_guard('terrain')

import bpy  # noqa: E402
import cv2  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Vector  # noqa: E402
from mathutils.bvhtree import BVHTree  # noqa: E402
from PIL import Image  # noqa: E402

OUT = lc.OUT
CORE = [-200.0, -200.0, 200.0, 200.0]      # the core square, three metres [x0, z0, x1, z1]
RING = [-550.0, -550.0, 550.0, 550.0]      # the ring's macro square
HEIGHTS = 1024
MACRO_PX = 2048
MASK_PX = 1024
STARTED = time.time()


def log(message):
    print(f'STYLELAB: [{time.time() - STARTED:5.1f} s] {message}', flush=True)


def gather(objects):
    """Positions (V, 3) in three's axes, triangles (T, 3), and per-corner normals (three), colours and the source UVs,
    of objects (their world transforms applied)."""
    deps = bpy.context.evaluated_depsgraph_get()
    pos, tris, normals, colors, uvs = [], [], [], [], {}
    base = 0
    for obj in objects:
        ev = obj.evaluated_get(deps)
        me = ev.to_mesh()
        me.calc_loop_triangles()
        m = np.array(obj.matrix_world)
        co = np.empty(len(me.vertices) * 3, np.float64)
        me.vertices.foreach_get('co', co)
        co = co.reshape(-1, 3) @ m[:3, :3].T + m[:3, 3]
        nt = len(me.loop_triangles)
        lt_verts = np.empty(nt * 3, np.int64)
        me.loop_triangles.foreach_get('vertices', lt_verts)
        lt_loops = np.empty(nt * 3, np.int64)
        me.loop_triangles.foreach_get('loops', lt_loops)
        cn = np.empty(nt * 9, np.float32)
        me.loop_triangles.foreach_get('split_normals', cn)
        cn = cn.reshape(-1, 3) @ np.linalg.inv(m[:3, :3]).T.astype(np.float32)
        cn /= np.maximum(np.linalg.norm(cn, axis=1, keepdims=True), 1e-9)
        attr = me.color_attributes.get('Col')
        if attr is not None and attr.domain == 'CORNER':
            col = np.empty(len(attr.data) * 4, np.float32)
            attr.data.foreach_get('color', col)
            col = col.reshape(-1, 4)[lt_loops]
        else:
            col = np.ones((nt * 3, 4), np.float32)
        for layer in me.uv_layers:
            uv = np.empty(len(layer.data) * 2, np.float32)
            layer.data.foreach_get('uv', uv)
            uvs.setdefault(layer.name, []).append(uv.reshape(-1, 2)[lt_loops])
        pos.append(np.column_stack([co[:, 0], co[:, 2], -co[:, 1]]))
        tris.append(lt_verts.reshape(-1, 3) + base)
        normals.append(np.column_stack([cn[:, 0], cn[:, 2], -cn[:, 1]]))
        colors.append(col)
        base += len(co)
        ev.to_mesh_clear()
    return (np.vstack(pos), np.vstack(tris), np.vstack(normals), np.vstack(colors),
            {k: np.vstack(v) for k, v in uvs.items()})


def make_object(name, pos, tris, corner_normals, corner_colors, uv0, uv1, material):
    """A Blender object from three-axis data (converted back to Blender's axes: the glTF export turns it again)."""
    used, inverse = np.unique(tris.ravel(), return_inverse=True)
    local = inverse.reshape(-1, 3)
    p = pos[used]
    blender = np.column_stack([p[:, 0], -p[:, 2], p[:, 1]])
    me = bpy.data.meshes.new(name)
    me.from_pydata(blender.tolist(), [], local.tolist())
    n = corner_normals
    me.polygons.foreach_set('use_smooth', [True] * len(me.polygons))
    for layer_name, uv in (('UV0', uv0), ('UV1', uv1)):
        if uv is None:
            continue
        # glTF stores v flipped (1 - v): hand Blender 1 - v so the file holds v as given.
        flipped = np.column_stack([uv[:, 0], 1.0 - uv[:, 1]]).astype(np.float32)
        me.uv_layers.new(name=layer_name).data.foreach_set('uv', flipped.ravel())
    attr = me.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    attr.data.foreach_set('color', np.clip(corner_colors, 0.0, 1.0).astype(np.float32).ravel())
    me.normals_split_custom_set(np.column_stack([n[:, 0], -n[:, 2], n[:, 1]]).tolist())
    me.materials.append(material)
    me.update()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def lab_material(name):
    mat = bpy.data.materials.get(name)
    if mat is not None:
        mat.name = name + '__src'
    return bpy.data.materials.new(name)


def rect_uv(pos, rect):
    return np.column_stack([(pos[:, 0] - rect[0]) / (rect[2] - rect[0]), (pos[:, 2] - rect[1]) / (rect[3] - rect[1])])


def corner(values, tris):
    return values[tris.ravel()]


def export(objs, path):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.gltf(
        filepath=path, export_format='GLB', use_selection=True, export_apply=False, export_yup=True,
        export_materials='EXPORT', export_image_format='NONE', export_texcoords=True, export_normals=True,
        export_tangents=False, export_vertex_color='NAME', export_vertex_color_name='Col',
        export_all_vertex_colors=False, export_attributes=False, export_skins=False, export_animations=False,
        export_extras=False)
    log(f'{os.path.relpath(path, OUT)}: {os.path.getsize(path) / 2 ** 20:.2f} MB')


def heights(core_objs, ring_objs, wall_objs):
    """The ground's height (three y) at the centres of a HEIGHTS x HEIGHTS grid over the core square, from the
    triangles the page draws (the ring's where the core's tiles end, the canyon wall's under the Rim's lip)."""
    trees = []
    for objs in (core_objs, ring_objs, wall_objs):
        pos, tris, _, _, _ = gather(objs)
        blender = [Vector((p[0], -p[2], p[1])) for p in pos]
        trees.append(BVHTree.FromPolygons(blender, tris.tolist(), epsilon=0.0))
    out = np.full((HEIGHTS, HEIGHTS), np.nan, np.float32)
    step = (CORE[2] - CORE[0]) / HEIGHTS
    down = Vector((0.0, 0.0, -1.0))
    for r in range(HEIGHTS):
        z3 = CORE[1] + (r + 0.5) * step
        for c in range(HEIGHTS):
            x3 = CORE[0] + (c + 0.5) * step
            origin = Vector((x3, -z3, 400.0))
            for tree in trees:
                hit = tree.ray_cast(origin, down, 1000.0)
                if hit[0] is not None:
                    out[r, c] = hit[0].z
                    break
    missing = np.isnan(out)
    if missing.any():
        # The nearest cell that found ground (none of these is in the region).
        import cv2 as _cv2
        _, labels = _cv2.distanceTransformWithLabels(missing.astype(np.uint8), _cv2.DIST_L2, 5,
                                                     labelType=_cv2.DIST_LABEL_PIXEL)
        found = ~missing
        lookup = np.zeros(labels.max() + 1, np.float32)
        lookup[labels[found]] = out[found]
        out[missing] = lookup[labels[missing]]
        log(f'heights: {missing.sum()} cells found no ground; each takes its nearest neighbour\'s')
    return out


def lab_image(path, size):
    """A repository map (north up, U east) turned to the lab's convention (row 0 at z0 = south, column 0 at x0 =
    east: a half turn), as an RGBA uint8 array. RGB and alpha are resized apart: Pillow resizes RGBA premultiplied,
    which wipes the colour wherever alpha is 0 (the macro map's grass, whose alpha is the detail selector)."""
    im = Image.open(path).convert('RGBA')
    rgb = im.convert('RGB')
    alpha = im.getchannel('A')
    if im.size[0] != size:
        rgb = rgb.resize((size, size), Image.LANCZOS)
        alpha = alpha.resize((size, size), Image.LANCZOS)
    a = np.dstack([np.asarray(rgb), np.asarray(alpha)])[::-1, ::-1]
    return np.ascontiguousarray(a)


def save_split(a, rel, quality=90, lossless=False, alpha=True):
    """An RGBA array as an RGB webp (rel) and, with alpha, its alpha as a grayscale webp beside it (<rel>_A.webp): a
    browser loses the colour under alpha 0 (premultiplied decoding), and these maps' alpha is data, not coverage."""
    save_webp(Image.fromarray(np.ascontiguousarray(a[..., :3]), 'RGB'), rel, quality=quality, lossless=lossless)
    if alpha:
        save_webp(Image.fromarray(np.ascontiguousarray(a[..., 3]), 'L'), rel.replace('.webp', '_A.webp'),
                  quality=quality, lossless=lossless)


def save_webp(im, rel, quality=88, lossless=False):
    path = os.path.join(OUT, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path, 'WEBP', quality=quality, method=6, lossless=lossless, exact=True)
    log(f'{rel}: {im.size[0]} px, {os.path.getsize(path) / 1024:.0f} KB')


def maps(pos_core, tris_core, normals_core):
    tex = os.path.join(lc.REPO, 'Art', 'Textures')
    # The macro maps as RGB (Terrain_Macro.webp) and their alpha, the detail selector, apart (Terrain_Macro_A.webp).
    save_split(lab_image(os.path.join(tex, 'RansomsRestMacro', 'T_RansomsRestMacro_BC.png'), MACRO_PX),
               'tex/Terrain_Macro.webp', quality=90)
    save_split(lab_image(os.path.join(tex, 'RansomsRestRingMacro', 'T_RansomsRestRingMacro_BC.png'), 1024),
               'tex/Terrain_RingMacro.webp', quality=88)
    # The scatter and scrub masks: data in all four channels, lossless, RGB and A apart.
    scatter = lab_image(os.path.join(tex, 'RansomsRestMacro', 'T_RansomsRestScatter_BC.png'), MASK_PX)
    scrub = lab_image(os.path.join(tex, 'RansomsRestMacro', 'T_RansomsRestScrub_BC.png'), MASK_PX)
    save_split(scatter, 'tex/Terrain_Scatter.webp', lossless=True)
    save_split(scrub, 'tex/Terrain_Scrub.webp', lossless=True)
    macro = lab_image(os.path.join(tex, 'RansomsRestMacro', 'T_RansomsRestMacro_BC.png'), MASK_PX).astype(np.float32) \
        / 255.0
    sc = scatter.astype(np.float32) / 255.0
    sb = scrub.astype(np.float32) / 255.0
    # Steepness on the mask grid, from the drawn triangles' normals (the slope M_Terrain reads).
    up = np.full((MASK_PX, MASK_PX), 1.0, np.float32)
    cn = normals_core.reshape(-1, 3, 3).mean(1)
    centre = pos_core[tris_core].mean(1)
    col = np.clip(((centre[:, 0] - CORE[0]) / (CORE[2] - CORE[0]) * MASK_PX).astype(int), 0, MASK_PX - 1)
    row = np.clip(((centre[:, 2] - CORE[1]) / (CORE[3] - CORE[1]) * MASK_PX).astype(int), 0, MASK_PX - 1)
    np.minimum.at(up, (row, col), cn[:, 1].astype(np.float32))
    up = cv2.erode(up, np.ones((3, 3), np.uint8))   # cells no triangle centre fell in take their neighbours' slope
    degrees = np.degrees(np.arccos(np.clip(up, -1.0, 1.0)))
    steep = np.clip((degrees - 40.0) / 15.0, 0.0, 1.0)
    select = macro[..., 3]
    rock = np.maximum(np.clip((select - 0.5) / 0.3, 0.0, 1.0), steep)
    road = np.clip(1.0 - np.abs(select - 0.33) / 0.17, 0.0, 1.0) * (select < 0.55)
    bare = np.clip(1.0 - sc[..., 1] * 1.5, 0.0, 1.0) * (1.0 - rock)
    masks = np.stack([np.maximum(road, bare), rock, sc[..., 1], sb.max(axis=2)], axis=-1)
    save_split((np.clip(masks, 0, 1) * 255 + 0.5).astype(np.uint8), 'tex/Terrain_Masks.webp', lossless=True)


def main():
    bpy.ops.wm.open_mainfile(filepath=os.path.join(lc.WORK, 'terrain.blend'))
    scene = bpy.context.scene
    objs = sorted(scene.objects, key=lambda o: o.name)
    tiles = [o for o in objs if o.type == 'MESH' and '_Tile_' in o.name]
    ring = [o for o in objs if o.type == 'MESH' and '_Ring_' in o.name]
    canyon = [o for o in objs if o.type == 'MESH' and '_CanyonWall_' in o.name]
    backdrop = [o for o in objs if o.type == 'MESH' and '_Backdrop_' in o.name]
    water = [o for o in objs if o.type == 'MESH' and o.name.endswith('_Water')]
    log(f'tiles {len(tiles)}, ring {len(ring)}, canyon wall {len(canyon)}, backdrop {len(backdrop)}, water {len(water)}')
    # The pieces as the level build places them (SM_<name> at the origin), in Unreal cm, for mock_unreal.py's traces.
    pieces = {}
    for obj in tiles + ring + canyon + backdrop + water:
        p, t, _, _, _ = gather([obj])
        tri = p[t]                                       # three metres
        pieces[obj.name] = (np.stack([tri[..., 2], -tri[..., 0], tri[..., 1]], axis=-1) * 100.0).astype(np.float32)
    np.savez(os.path.join(lc.WORK, 'terrain_pieces.npz'), **pieces)
    log(f'terrain_pieces.npz: {len(pieces)} pieces for the level build\'s traces')
    os.makedirs(os.path.join(OUT, 'models'), exist_ok=True)
    os.makedirs(os.path.join(OUT, 'terrain'), exist_ok=True)

    pos, tris, cn, colors, _ = gather(tiles)
    centre = pos[tris].mean(axis=1)
    x0, z0, x1, z1 = lc.REGION_THREE
    near = (centre[:, 0] >= x0) & (centre[:, 0] <= x1) & (centre[:, 2] >= z0) & (centre[:, 2] <= z1)
    ncn = cn.reshape(-1, 3, 3)
    ncol = colors.reshape(-1, 3, 4)
    uv0 = rect_uv(pos, CORE)
    uv1 = np.column_stack([pos[:, 0], pos[:, 2]])
    terrain = lab_material('Terrain')
    made = {}
    for name, keep in (('Terrain_Near', near), ('Terrain_Far', ~near)):
        t = tris[keep]
        made[name] = make_object(name, pos, t, ncn[keep].reshape(-1, 3), ncol[keep].reshape(-1, 4),
                                 corner(uv0, t), corner(uv1, t), terrain)
        log(f'{name}: {len(t)} triangles')

    wpos, wtris, wcn, wcol, _ = gather(water)
    made['Terrain_Water'] = make_object('Terrain_Water', wpos, wtris, wcn, wcol, corner(rect_uv(wpos, CORE), wtris),
                                        corner(np.column_stack([wpos[:, 0], wpos[:, 2]]), wtris), lab_material('Water'))
    rpos, rtris, rcn, rcol, _ = gather(ring)
    made['Terrain_Ring'] = make_object('Terrain_Ring', rpos, rtris, rcn, rcol, corner(rect_uv(rpos, RING), rtris),
                                       corner(np.column_stack([rpos[:, 0], rpos[:, 2]]), rtris),
                                       lab_material('TerrainRing'))
    log(f'Terrain_Ring: {len(rtris)} triangles')
    cpos, ctris, ccn, ccol, cuv = gather(canyon)
    made['Terrain_CanyonWall'] = make_object('Terrain_CanyonWall', cpos, ctris, ccn, ccol, cuv.get('UVMap'),
                                             None, lab_material('RockCliff_Ransom'))
    for k, obj in enumerate(backdrop):
        bpos, btris, bcn, bcol, buv = gather([obj])
        layer = int(obj.data.materials[0].name.replace('Backdrop', '')[:1] or 0)
        made[f'Terrain_Backdrop_{k}'] = make_object(f'Terrain_Backdrop_{k}', bpos, btris, bcn, bcol,
                                                    buv.get('UVMap'), buv.get('Depth'),
                                                    lab_material(f'Backdrop{layer}'))
    export([made['Terrain_Near']], os.path.join(OUT, 'models', 'terrain_near.glb'))
    export([o for n, o in made.items() if n != 'Terrain_Near'], os.path.join(OUT, 'models', 'terrain_far.glb'))

    h = heights(tiles, ring, canyon)
    h.astype('<f4').tofile(os.path.join(OUT, 'terrain', 'heights.bin'))
    log(f'heights.bin: {HEIGHTS} x {HEIGHTS}, {h.min():.1f} .. {h.max():.1f} m')
    np.save(os.path.join(lc.WORK, 'heights_three.npy'), h)
    maps(pos, tris, cn)
    log('terrain done')


if __name__ == '__main__':
    main()
