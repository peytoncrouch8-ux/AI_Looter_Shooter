"""A story person's Tripo body in M_Ghost's terms (the scripts' docstrings: vertex colour R the tint zone, G cavity,
B the ember edge, A the fade; UV 0 the Polymer box projection and the shroud's strip UVs; UV 1 'Paint' Tripo's own
paint, drawn by M_Ghost with PaintAmount 1, as the Tripo Unpaid's since 2026-10-31). What the script body keeps (its
shroud, its coal, Thaddeus's tape) comes over with its own colours and UVs; the coal's burnt hole is cut into Tripo's
cloth here."""
import os

import bpy
import numpy as np

import looter_rebake as rb
import looter_textures as lt
import looter_tripo as tripo
import looter_tripo_rig as tr
import parts_people_cfg as cfg
import parts_people_rig as pr
import parts_people_src as ps

SIZE = 2048
LAYER = 'Paint'
PACK = 0.97                      # the islands' share of the atlas (lower left); the upper right holds the swatches
# Solid swatches in the atlas's free upper-right corner, a 3 x 3 grid of 0.01 cells: the teeth, mouth and coal, the
# script's shroud, Thaddeus's tape and its case, and one per tint zone for the props that share the body's slot (Abel's
# hat, the Foreman's bowler, the miners' hats: drawn in the slot's own zone colours, as before the paint).
_CELL = 0.01
SWATCH = {key: (0.975 + _CELL * (k % 3), 0.975 + _CELL * (k // 3)) for k, key in
          enumerate(('solid', 'shroud', 'tape', 'case', 'zone0', 'zone1', 'zone2', 'zone3'))}
SWATCH_COLOR = {'solid': 0xa49a86, 'shroud': 0xa9b1b3, 'tape': 0xd6d1c4, 'case': 0x8b7356}
CAGE, REACH = 0.006, 0.016
FILL = 64


# --- What the script body keeps ---

def script_arrays(body):
    """The script body as arrays: V, faces, per-face slot, per-loop UV 0 and colour (the values M_Ghost reads),
    per-vertex weights {bone: w}."""
    me = body.data
    V, F = tr.mesh_arrays(me)
    slots = np.array([p.material_index for p in me.polygons])
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers[0].data.foreach_get('uv', uv)
    col = me.color_attributes['Col']
    rgba = np.empty(len(me.loops) * 4, np.float32)
    col.data.foreach_get('color_srgb', rgba)
    names = {g.index: g.name for g in body.vertex_groups}
    W = {}
    for v in me.vertices:
        for g in v.groups:
            W.setdefault(names[g.group], np.zeros(len(V)))[v.index] = g.weight
    return dict(V=V, F=F, slots=slots, uv=uv.reshape(-1, 2), rgba=rgba.reshape(-1, 4), W=W)


def kept_faces(name, S):
    """Which script faces stay, and as what: {'coal': faces, 'shroud': faces, 'tape': faces, 'case': faces}."""
    F, W, V = S['F'], S['W'], S['V']
    n = len(V)
    roots = ps._components(F, n)
    tail = sum((w for b, w in W.items() if b.startswith('tail')), np.zeros(n))
    hands = sum((w for b, w in W.items() if b.startswith(('hand_', 'thumb', 'index', 'middle', 'ring', 'pinky'))),
                np.zeros(n))
    zone = np.zeros(n)
    loop_v = np.concatenate(F)
    zone[loop_v] = S['rgba'][:, 0] * 3.0
    out = {k: [] for k in ('coal', 'shroud', 'tape', 'case')}
    keep_hands = cfg.PEOPLE[name].get('keep_hand_cloth', False)
    for root in np.unique(roots):
        faces = np.where(roots == root)[0]
        verts = np.unique(np.concatenate([F[i] for i in faces]))
        if (S['slots'][faces] == 1).all():
            out['coal'] += faces.tolist()
        elif tail[verts].mean() > 0.5:
            out['shroud'] += faces.tolist()
        elif keep_hands and hands[verts].mean() > 0.5 and np.round(zone[verts].mean()) >= 1:
            out['case' if np.round(zone[verts].mean()) >= 3 else 'tape'] += faces.tolist()
    ps.log(f'{name}: kept from the script body: ' + ', '.join(f'{k} {len(v)} faces' for k, v in out.items()))
    return out


# --- The coal's burnt hole ---

def crater(name, rig, V, coal_V):
    """Tripo's cloth round the coal pressed back into a bowl behind it (the coal sits in a hole burnt through the
    shirt); returns (V, rim: per vertex 0..1 round the hole's edge, for the ember and cavity)."""
    c = np.array(rig.data.bones['coal'].head_local)
    n = pr.unit(np.array(rig.data.bones['coal'].tail_local) - c)
    R = float(np.max(np.linalg.norm(coal_V - c, axis=1))) if len(coal_V) else 0.045
    d = V - c
    along = d @ n
    perp = np.linalg.norm(d - along[:, None] * n, axis=1)
    hole = cfg.PEOPLE[name].get('hole', 1.25) * R
    bottom = -0.55 * R
    want = bottom * pr.smoothstep(hole * 1.35, hole * 0.6, perp)      # the bowl's surface along n at this radius
    near = (perp < hole * 1.6) & (along > -2.5 * R) & (along < 3.0 * R)
    move = np.where(near & (along > want), want - along, 0.0)
    # Only the front of the chest: the back's cloth never comes near.
    V2 = V + move[:, None] * n
    surface = V[near & (perp < hole)]
    front = float(np.median(surface @ n - c @ n)) if len(surface) else 0.0
    ps.log(f'{name}: the coal ({100 * R:.1f} cm): Tripo\'s chest {100 * front:+.1f} cm from its middle along its facing; '
           f'{int((move < 0).sum())} vertices pressed into its hole')
    rim = pr.smoothstep(hole * 1.5, hole * 0.9, perp) * pr.smoothstep(-1.5 * R, 0.0, along) * near
    return V2, rim


# --- Zones from Tripo's paint ---

def _hex_srgb(h):
    h = int(h.lstrip('#'), 16) if isinstance(h, str) else int(h)
    return np.array([(h >> 16) & 255, (h >> 8) & 255, h & 255], float) / 255.0


def face_zones(name, src_surface, pixels, Vu, faces, W):
    """Each Tripo face's tint zone: 0 bare skin (only where a body shows it: the head, neck, forearms and hands), else
    the nearest of cfg 'zones' (three sRGB colours for zones 1, 2 and 3) to the paint under it."""
    protos = np.array([_hex_srgb(h) for h in cfg.PEOPLE[name]['zones']])
    skin_c = _hex_srgb(cfg.PEOPLE[name].get('skin', '#9AA3B0'))
    centres = np.array([Vu[f].mean(0) for f in faces])
    tri, bary, _, _ = src_surface.nearest(centres)
    srgb = tr.sample_image(pixels, src_surface.uv_at(tri, bary))[:, :3]       # the map's stored (sRGB) values
    bare_bones = ('head', 'jaw', 'neck', 'lowerarm_l', 'lowerarm_r', 'hand_l', 'hand_r') + \
        tuple(f'{f}_01_{s}' for f in pr.FINGERS for s in 'lr')
    bare_v = sum((W[b] for b in bare_bones if b in W), np.zeros(len(Vu)))
    bare = np.array([bare_v[f].mean() for f in faces]) > 0.5
    d_skin = np.linalg.norm(srgb - skin_c, axis=1)
    d = np.linalg.norm(srgb[:, None, :] - protos[None], axis=2)
    zone = 1 + np.argmin(d, axis=1)
    zone = np.where(bare & (d_skin < d.min(1) * 1.15), 0, zone)
    counts = np.bincount(zone, minlength=4)
    ps.log(f'{name}: zones (faces): skin {counts[0]}, 1 {counts[1]}, 2 {counts[2]}, 3 {counts[3]}')
    return zone


# --- Tripo's paint on UV 1 ---

def _unwrap(obj):
    obj.data.uv_layers.new(name=LAYER)
    obj.data.uv_layers.active = obj.data.uv_layers[LAYER]
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.2, island_margin=0.004, area_weight=0.0, correct_aspect=True,
                             scale_to_bounds=False)
    bpy.ops.uv.pack_islands(margin=0.004, rotate=True)
    bpy.ops.object.mode_set(mode='OBJECT')
    uv = np.empty(len(obj.data.loops) * 2)
    obj.data.uv_layers[LAYER].data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2) * PACK
    obj.data.uv_layers[LAYER].data.foreach_set('uv', uv.ravel())
    return uv


def _emitting(src):
    mat = bpy.data.materials.new('_PeoplePaintSource')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    em = nodes.new('ShaderNodeEmission')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = bpy.data.images[src['BC']]
    links.new(tex.outputs['Color'], em.inputs['Color'])
    links.new(em.outputs['Emission'], out.inputs['Surface'])
    src.data.materials.clear()
    src.data.materials.append(mat)


def _bary_free(surface, tri, P):
    """Barycentric weights of points on their triangles' planes, unclamped (a corner past its centroid's triangle)."""
    A, B, C = (surface.V[surface.T[tri, k]] for k in range(3))
    v0, v1, v2 = B - A, C - A, P - A
    d00, d01, d11 = (v0 * v0).sum(1), (v0 * v1).sum(1), (v1 * v1).sum(1)
    d20, d21 = (v2 * v0).sum(1), (v2 * v1).sum(1)
    den = np.where(np.abs(d00 * d11 - d01 * d01) < 1e-20, 1e-20, d00 * d11 - d01 * d01)
    b = (d11 * d20 - d01 * d21) / den
    c = (d00 * d21 - d01 * d20) / den
    return np.column_stack([1.0 - b - c, b, c])


def grade(name, rgb):
    """The game's muted palette on Tripo's paint (the user, 2026-11-01: everything from Tripo in the game's look):
    saturation over cfg 'sat_knee' (0.45) compressed by 'sat_keep' (0.6), and cfg 'grade' hue bands (degrees lo, hi,
    saturation kept) pulled further: a ghost's blue skin toward ashen grey, a loud orange apron toward worn leather."""
    c = cfg.PEOPLE[name]
    mx, mn = rgb.max(-1), rgb.min(-1)
    sat = np.where(mx > 1e-6, (mx - mn) / np.maximum(mx, 1e-6), 0.0)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    d = np.maximum(mx - mn, 1e-6)
    hue = np.where(mx == r, (g - b) / d % 6.0, np.where(mx == g, (b - r) / d + 2.0, (r - g) / d + 4.0)) * 60.0
    knee, keep = c.get('sat_knee', 0.45), c.get('sat_keep', 0.6)
    want = np.where(sat > knee, knee + (sat - knee) * keep, sat)
    for lo, hi, kept in c.get('grade', ()):
        band = np.clip(np.minimum(hue - lo, hi - hue) / 12.0, 0.0, 1.0) * (sat > 0.08)
        want = want * (1.0 - band * (1.0 - kept))
    scale = np.where(sat > 1e-6, want / np.maximum(sat, 1e-6), 1.0)
    out = mx[..., None] - (mx[..., None] - rgb) * scale[..., None]
    ps.log(f'{name}: paint graded: mean saturation {sat.mean():.3f} -> {want.mean():.3f}')
    return np.clip(out, 0.0, 1.0).astype(np.float32)


def paint_from_tripo(name, surface, image, Vu, faces, zone_colors=()):
    """Tripo's own colour map as the paint: the map shrunk into the atlas's lower left (PACK), the swatches in its
    upper right, and each face's corners on Tripo's UVs where they lay on Tripo's surface (upright space, before the
    warp). A face across one of Tripo's UV seams takes its corners from the island under its middle (extrapolated on
    that triangle's plane), so it never spans the atlas. (A fresh unwrap and a cast, the Unpaid's way, left a collapsed
    mesh in thousands of specks: 6.7% of the atlas hit, 2026-11-01.) Returns (per-loop UVs in face order, linear colour
    rows top first); writes Art/Textures/<Name>Paint/T_<Name>Paint_BC.png."""
    inner = int(round(SIZE * PACK))
    tex = grade(name, tripo.pixels(image, inner)[..., :3])          # stored sRGB, rows top first
    atlas = np.zeros((SIZE, SIZE, 3), np.float32)
    atlas[SIZE - inner:, :inner] = tex[:inner, :inner]
    atlas[:SIZE - inner, :inner] = tex[0]               # the strip above the map: its top row, stretched up
    atlas[:, inner:] = atlas[:, inner - 1:inner]        # the strip right of it: its right column
    swatches = dict(SWATCH_COLOR)
    for k, hex_ in enumerate(zone_colors):
        swatches[f'zone{k}'] = int(hex_.lstrip('#'), 16)
    for key, (u, v) in SWATCH.items():
        if key not in swatches:
            continue
        r0, r1 = int((1.0 - v - _CELL / 2) * SIZE), int((1.0 - v + _CELL / 2) * SIZE)
        c0, c1 = int((u - _CELL / 2) * SIZE), int((u + _CELL / 2) * SIZE)
        h = swatches[key]
        atlas[r0:r1, c0:c1] = np.array([(h >> 16) & 255, (h >> 8) & 255, h & 255], np.float32) / 255.0
    path = lt.texture_path(f'{name}Paint', 'BC')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    lt.write_png(path, lt.to8(atlas))
    ps.log(f'{name}: wrote {path} ({SIZE} px; the Tripo map at {inner} px)')

    flat = np.concatenate(faces)
    starts = np.cumsum([0] + [len(f) for f in faces])[:-1]
    tri, bary, _, _ = surface.nearest(Vu[flat])
    uv = surface.uv_at(tri, bary)
    centres = np.array([Vu[f].mean(0) for f in faces])
    ctri, cbary, _, _ = surface.nearest(centres)
    cuv = surface.uv_at(ctri, cbary)
    owner = np.repeat(np.arange(len(faces)), [len(f) for f in faces])
    jump = np.linalg.norm(uv - cuv[owner], axis=1)
    span = np.median(jump) * 6.0 + 1e-4
    far = jump > span
    if far.any():
        b = _bary_free(surface, ctri[owner[far]], Vu[flat[far]])
        uv[far] = sum(b[:, k:k + 1] * surface.uv[ctri[owner[far]], k] for k in range(3))
    ps.log(f'{name}: {int(far.sum())} of {len(flat)} corners across a Tripo seam taken from the island under the face')
    uv = np.clip(np.mod(uv, 1.0), 0.0, 1.0) * PACK
    srgb = atlas
    linear = np.where(srgb <= 0.04045, srgb / 12.92, ((srgb + 0.055) / 1.055) ** 2.4)
    return uv, linear


def prop_paint_uvs(body, slot):
    """Every other mesh drawn with the body's slot (a hat) gets a 'Paint' UV per face onto its zone's swatch (its own
    zone colour, as before the paint), so the slot's PaintAmount 1 draws it as it was."""
    done = []
    for o in bpy.data.objects:
        if o.type != 'MESH' or o is body or slot.name not in [m.name for m in o.data.materials if m]:
            continue
        me = o.data
        col = me.color_attributes.get('Col')
        n = len(me.loops)
        rgba = np.zeros(n * 4, np.float32)
        if col is not None:
            col.data.foreach_get('color_srgb' if col.data_type == 'BYTE_COLOR' else 'color', rgba)
        zone = np.clip(np.round(rgba.reshape(-1, 4)[:, 0] * 3.0), 0, 3).astype(int)
        uv = np.zeros((n, 2), np.float32)
        for p in me.polygons:
            z = zone[p.loop_start]
            uv[p.loop_start:p.loop_start + p.loop_total] = SWATCH[f'zone{z}']
        layer = me.uv_layers.get(LAYER) or me.uv_layers.new(name=LAYER)
        layer.data.foreach_set('uv', uv.ravel())
        me.uv_layers.active_index = 0
        done.append(o.name)
    return done


def zone_means(colour, uv_faces, zones, areas):
    """Each zone's mean paint colour (linear), area-weighted over the Tripo faces."""
    acc, area = np.zeros((4, 3)), np.zeros(4)
    for uvf, z, a in zip(uv_faces, zones, areas):
        c = uvf.mean(0)
        row = min(SIZE - 1, max(0, int((1.0 - c[1]) * SIZE)))
        col = min(SIZE - 1, max(0, int(c[0] * SIZE)))
        acc[z] += colour[row, col] * a
        area[z] += a
    return acc / np.maximum(area[:, None], 1e-9), area


def to_hex(linear):
    s = np.clip(np.where(linear <= 0.0031308, linear * 12.92, 1.055 * np.power(np.clip(linear, 0, 1), 1 / 2.4) - 0.055),
                0.0, 1.0)
    return '#' + ''.join(f'{int(round(c * 255)):02X}' for c in s)


def paint_look(name, ghost, means):
    """The body's ghost slot set to draw the paint (parts_unpaid_look.paint_look's values): TextureSet <Name>Paint,
    PaintAmount 1, PaintZone<N> each zone's mean paint colour and Zone<N>Color the same (the model's own colours) or
    cfg 'zone_tint' (a zone pulled toward a colour: M_Ghost scales the paint by Zone / PaintZone)."""
    import parts_unpaid_look as ul
    ghost['TextureSet'] = f'{name}Paint'
    tints = cfg.PEOPLE[name].get('zone_tint', {})
    for k, mean in enumerate(means):
        ghost[f'PaintZone{k + 1}'] = to_hex(mean)
        if k in tints:
            target, share = tints[k]
            t = np.array(lt.hex_color(int(target.lstrip('#'), 16))[:3])
            ghost[f'Zone{k + 1}Color'] = to_hex(mean * (1.0 - share) + t * share)
        else:
            ghost[f'Zone{k + 1}Color'] = to_hex(mean)
    for key, value in ul.PAINT_LOOK.items():
        ghost[key] = value
    for key, value in cfg.PEOPLE[name].get('look', {}).items():
        ghost[key] = value
    bsdf = next(n for n in ghost.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(int(to_hex(means[0])[1:], 16))
    ps.log(f'{name}: {ghost.name}: ' + ', '.join(f'zone {k} paint {ghost[f"PaintZone{k + 1}"]} -> '
                                                 f'{ghost[f"Zone{k + 1}Color"]}' for k in range(4)))
