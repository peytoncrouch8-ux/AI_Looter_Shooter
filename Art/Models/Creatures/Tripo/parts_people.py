"""The story people's Tripo bodies (the user's call 2026-11-01: "the story people", bodies only) on their script rigs:
SK_Abel, SK_Foreman, SK_Amos, SK_Hiram, SK_Thaddeus, SK_DeadMiner. Each model script builds its script body, rig,
props, pose table and hit zones as before; at its end swap(name, globals()) puts the Tripo body in the script body's
place (LOOTER_PEOPLE_TRIPO=0 keeps the script body). Kept exactly: every bone, its place and the rest pose (so the pose
tables and the code are unchanged), the props (hats, lanterns, picks...), the slots (the person's ghost slot and
GhostCoal), the script's coal and shroud (and Thaddeus's tape) with their own colours and UVs, the hit zones rebuilt by
the script's own hit_zones() on the new skin.

How (recipe 2 of Docs/Work/2026-10-31-Wardens/TRIPO_REDO_INVENTORY.md, the Tripo Unpaid's, parts_unpaid*.py):
Tripo's model (Art/Tripo/<Name>/<Name>.glb, a T- or A-pose) turned to face -Y and scaled into the scripts' upright
frame by its shoulders and hips (parts_people_src); the legs below the hems cut off (the script's shroud is the ghost's
lower body); even quads by QuadriFlow (hands and face smaller); the lips parted with a pocket and teeth behind; a
skeleton laid on its measured joints (parts_people_cfg) and skin weights made in its own pose (bone heat for the arms,
the torso by height, the head, jaw and the lower body by rule); each bone's map from there onto the rig's rest carries
the mesh into the rest pose (the head scaled so the script's hat fits it, the hands to the rig's knuckles); the coal's
hole pressed into the chest; tint zones from Tripo's paint; Tripo's paint baked to a 2048 atlas on UV 1
(Art/Textures/<Name>Paint) and the ghost slot set to draw it (PaintAmount 1), as the Unpaid's."""
import math
import os

import bpy
import numpy as np

import looter_textures as lt
import looter_tripo as tripo
import looter_tripo_rig as tr
import parts_people_cfg as cfg
import parts_people_look as pl
import parts_people_rig as pr
import parts_people_src as ps

ENABLED = os.environ.get('LOOTER_PEOPLE_TRIPO', '1') != '0'
HULL_PAD = 0.005          # m: each hit hull made this much proud of the skin it wraps


def _flat(faces):
    return [np.asarray(f) for F in faces for f in np.atleast_2d(F)]


def _script_head(rig, S):
    """The script head's chin and crown along the rig's head bone (its jaw's lowest point and its head's highest)."""
    h, t = (np.array(rig.data.bones['head'].head_local), np.array(rig.data.bones['head'].tail_local))
    axis = pr.unit(t - h)
    owner = {}
    for b, w in S['W'].items():
        owner[b] = w
    names = sorted(owner)
    Wm = np.column_stack([owner[b] for b in names])
    top = np.array(names)[np.argmax(Wm, axis=1)]
    proj = (S['V'] - h) @ axis
    crown = proj[top == 'head'].max()
    chin = proj[np.isin(top, ('jaw', 'head'))].min()
    return chin, crown


def swap(name, G):
    """Replaces the script body G['BODY'] (and its hit zones) by the Tripo body on the same rig. Returns the body."""
    c = cfg.PEOPLE[name]
    rig, old = G['RIG'], G['BODY']
    mats = list(old.data.materials)
    S = pl.script_arrays(old)
    keep = pl.kept_faces(name, S)
    chin, crown = _script_head(rig, S)

    # The source: raw for the cut, upright for everything after.
    src, Vraw, Fraw = ps.source_object(name, 'raw')
    Vc, Fc = ps.cut_legs(name, Vraw, Fraw)
    J = ps.joints(name)
    s, t = ps.upright_fit(name)
    ps.log(f'{name}: upright = raw x {s:.4f} + {np.round(t, 3)}')
    Vsrc = ps.to_upright(name, Vraw)
    src.data.vertices.foreach_set('co', Vsrc.astype(np.float32).ravel())
    src.data.update()
    m = c['mouth']
    mz = ps.to_upright(name, np.array([m['x'], m['y'], m['z']]))
    c['mouth_u'] = dict(x=mz[0], y=mz[1], z=mz[2], half_width=m['half_width'] * s)
    J_head = J['crown'][2] - J['chin'][2]
    c['head_scale'] = c.get('head_scale_fixed') or (crown - chin) / J_head
    Vq, Fq = ps.remesh(name, ps.to_upright(name, Vc), Fc)
    ps.log(f'{name}: quads: {len(Fq)} faces ({sum(len(f) == 4 for f in Fq)} quads)')
    Vq, Fq, lip, pocket = pr.cut_mouth(name, Vq, Fq)
    n_main = len(Fq)

    # Skeleton, maps, weights in upright space; the warp to the rig's rest.
    Sk = pr.skeleton(name, rig, J)
    M = pr.bone_maps(name, rig, Sk, J)
    W = pr.skin_weights(name, rig, Sk, J, Vq, Fq, lip)
    Vr = tr.warp(Vq, W, M)
    # The script's hat must sit on the head, not have it poke through its crown: the head shrunk a little at a time.
    hats = _hats(name, G)
    first = c['head_scale']
    for _ in range(8):
        poke = _poking(Vr, W, hats, rig)
        ps.log(f'{name}: head scale {c["head_scale"]:.3f}: {poke} head vertices through the crown of the hat')
        if poke <= c.get('poke_ok', 2) or not hats or c['head_scale'] < first * 0.92:
            break
        c['head_scale'] *= 0.97
        M = pr.bone_maps(name, rig, Sk, J)
        Vr = tr.warp(Vq, W, M)
    W = pr.finger_weights(name, rig, Vr, W)
    Vr = _grips(name, rig, Vr, W)

    # The teeth (head and jaw), warped as the head carries them.
    U = G.get('U') or _unpaid_library()
    teeth_V, teeth_F, teeth_W = [], [], []
    for Vt, Ft, bone in pr.teeth(name, U):
        teeth_V.append(tr.warp(Vt, {'head': np.ones(len(Vt))}, M))
        teeth_F.append(_flat(Ft))
        teeth_W.append(bone)

    # The coal's hole, with the script's coal.
    coal_faces = keep['coal']
    coal_v = np.unique(np.concatenate([S['F'][i] for i in coal_faces])) if coal_faces else np.zeros(0, int)
    Vr, rim = pl.crater(name, rig, Vr, S['V'][coal_v])

    # Zones and the paint (on Tripo's own surface: the upright positions before the warp).
    src_uv = tr.corner_uvs(src.data, tr.mesh_arrays(src.data)[1])
    surface = tr.Surface(Vsrc, tr.mesh_arrays(src.data)[1], src_uv)
    pixels = tripo.pixels(bpy.data.images[src['BC']], 2048)[::-1].copy()
    zones = pl.face_zones(name, surface, pixels, Vq, Fq, W)
    script_zones = [mats[0].get(f'Zone{k + 1}Color', '#BAC4C6') for k in range(4)]
    paint_uv, colour = pl.paint_from_tripo(name, surface, bpy.data.images[src['BC']], Vq, Fq, script_zones)

    # --- One mesh: Tripo's skin and pocket, the teeth, then what the script keeps ---
    Vs, Fs, slots, uv0, uv1, cols, kinds = [Vr], list(Fq), [], [], [], [], []
    blocks = [W]                    # each block of vertices' weights, {bone: w}
    count = len(Vr)
    occ = tr.ao(Vr, Fq, rays=20, reach=0.045)
    cavity = np.maximum(np.clip(pocket, 0.0, 1.0), pr.smoothstep(0.9, 0.45, occ) * 0.45)
    cavity = np.maximum(cavity, rim * 0.5)
    ember = rim * c.get('ember', 0.8)
    at = 0
    for k, f in enumerate(Fq):
        is_pocket = pocket[f].mean() > 0.5
        slots.append(0)
        uv0.append(None)
        uv1.append(np.tile(pl.SWATCH['solid'], (len(f), 1)) if is_pocket else paint_uv[at:at + len(f)])
        z = 0 if is_pocket else zones[k]
        cols.append(np.column_stack([np.full(len(f), z / 3.0), cavity[f], ember[f], np.ones(len(f))]))
        kinds.append('tripo')
        at += len(f)
    for Vt, Ft, bone in zip(teeth_V, teeth_F, teeth_W):
        Vs.append(Vt)
        for f in Ft:
            Fs.append(f + count)
            slots.append(0)
            uv0.append(None)
            uv1.append(np.tile(pl.SWATCH['solid'], (len(f), 1)))
            cols.append(np.tile([0.0, 0.15, 0.0, 1.0], (len(f), 1)))
            kinds.append('teeth')
        blocks.append({bone: np.ones(len(Vt))})
        count += len(Vt)
    loops_of = np.cumsum([0] + [len(f) for f in S['F']])
    kept = [(i, kind) for kind in ('coal', 'shroud', 'tape', 'case') for i in keep[kind]]
    kv = np.unique(np.concatenate([S['F'][i] for i, _ in kept])) if kept else np.zeros(0, int)
    remap = -np.ones(len(S['V']), int)
    remap[kv] = np.arange(len(kv)) + count
    Vs.append(S['V'][kv])
    for i, kind in kept:
        f = S['F'][i]
        Fs.append(remap[f])
        slots.append(int(S['slots'][i]))
        lo, hi = loops_of[i], loops_of[i + 1]
        uv0.append(S['uv'][lo:hi])
        uv1.append(np.tile(pl.SWATCH['solid' if kind == 'coal' else kind], (len(f), 1)))
        cols.append(S['rgba'][lo:hi])
        kinds.append(kind)
    blocks.append({b: w[kv] for b, w in S['W'].items()})
    count += len(kv)
    V = np.vstack(Vs)
    every = set().union(*blocks)
    Wt = {b: np.concatenate([np.asarray(blk.get(b, np.zeros(len(v))), float) for blk, v in zip(blocks, Vs)])
          for b in every}
    Wt = {b: w for b, w in Wt.items() if b in rig.data.bones and w.any()}
    Wt = tr.normalize(Wt, count)
    names = sorted(Wt)
    Wm = np.column_stack([Wt[b] for b in names])
    # Faces of no area (a contour cut grazing a corner) go: Unreal warns of their tangents.
    area = np.array([np.linalg.norm(sum(np.cross(V[f[k]] - V[f[0]], V[f[k + 1]] - V[f[0]])
                                        for k in range(1, len(f) - 1))) for f in Fs]) * 0.5
    good = area > 1e-9
    pick = lambda seq: [x for x, g in zip(seq, good) if g]      # noqa: E731
    Fs, slots, uv0, uv1, cols, kinds = map(pick, (Fs, slots, uv0, uv1, cols, kinds))
    ps.log(f'{name}: {int((~good).sum())} faces of no area dropped')

    # --- The object in the script body's place ---
    body_name = old.name
    for o in list(rig.children):
        if o.name.startswith(('UCX_', 'USP_', 'UCP_')) or o is old:
            data = o.data
            bpy.data.objects.remove(o, do_unlink=True)
            if isinstance(data, bpy.types.Mesh) and not data.users:
                bpy.data.meshes.remove(data)
    mesh = tr.new_mesh(body_name, V, Fs)
    if len(mesh.polygons) != len(Fs) or len(mesh.vertices) != len(V):
        raise RuntimeError(f'{name}: validate() changed the Tripo body')
    for mat in mats:
        mesh.materials.append(mat)
    mesh.polygons.foreach_set('material_index', np.array(slots, np.int32))
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    body = bpy.data.objects.new(body_name, mesh)
    bpy.context.scene.collection.objects.link(body)
    lt.box_uv(body, 'Polymer')
    layer = mesh.uv_layers[0].data
    coords = np.empty(len(mesh.loops) * 2)
    layer.foreach_get('uv', coords)
    coords = coords.reshape(-1, 2)
    for p, u in zip(mesh.polygons, uv0):
        if u is not None:
            coords[p.loop_start:p.loop_start + p.loop_total] = u
    layer.foreach_set('uv', coords.ravel())
    paint = mesh.uv_layers.new(name=pl.LAYER)
    paint.data.foreach_set('uv', np.vstack(uv1).astype(np.float32).ravel())
    mesh.uv_layers.active_index = 0
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color_srgb', np.clip(np.vstack(cols), 0, 1).astype(np.float32).ravel())
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = 0
    for b, bone in enumerate(names):
        group = body.vertex_groups.new(name=bone)
        w = Wm[:, b]
        for value in np.unique(np.round(w[w > 0.0], 3)):
            group.add(np.where(np.abs(np.round(w, 3) - value) < 1e-9)[0].tolist(), float(value), 'REPLACE')
    body.parent = rig
    body.modifiers.new('Armature', 'ARMATURE').object = rig

    # The hit zones, by the script's own rules on the new skin.
    by_size = tr.by_size(Fs)
    # The hulls' planes without the slivers of a dense skin's hull (parts_unpaid._supporting_planes, as the Tripo Unpaid).
    import looter_ghosts as lg
    import parts_unpaid as pu
    import looter_model as lm
    kept_planes, kept_hull = lg.HitZones.convex_planes, lm.hit_hull
    lg.HitZones.convex_planes = staticmethod(pu._supporting_planes)

    def padded_hull(arm, bone, objects):
        # Each hull 5 mm proud of its points: bmesh's hull of a dense skin leaves a few of them 4-6 mm outside it.
        for o in objects:
            P = np.array([v.co[:] for v in o.data.vertices])
            if len(P) > 3:
                c = P.mean(0)
                d = P - c
                P = P + d / np.maximum(np.linalg.norm(d, axis=1, keepdims=True), 1e-9) * pad[0]
                o.data.vertices.foreach_set('co', P.astype(np.float32).ravel())
        return kept_hull(arm, bone, objects)
    lm.hit_hull = padded_hull
    spaces = [ns for ns in (G, G.get('U')) if isinstance(ns, dict) and 'convex_planes' in ns]
    old_planes = [ns['convex_planes'] for ns in spaces]
    for ns in spaces:
        ns['convex_planes'] = pu._supporting_planes
    hit_zones = G['hit_zones'] if 'hit_zones' in G else G['U']['hit_zones']
    pad = [HULL_PAD]
    try:
        for attempt in range(3):
            try:
                if name == 'Abel':
                    hit_zones(rig, V, by_size, Wm, names, G['LANTERN'])
                else:
                    hit_zones(rig, V, by_size, Wm, names)
                break
            except RuntimeError as error:
                if 'outside every zone' not in str(error) or attempt == 2:
                    raise
                for o in [o for o in rig.children if o.name.startswith(('UCX_', 'USP_', 'UCP_'))]:
                    bpy.data.objects.remove(o, do_unlink=True)
                pad[0] += 0.005
                ps.log(f'{name}: {error}; the hulls again {1000 * pad[0]:.0f} mm proud')
    finally:
        lg.HitZones.convex_planes = kept_planes
        lm.hit_hull = kept_hull
        for ns, old in zip(spaces, old_planes):
            ns['convex_planes'] = old

    # The ghost slot draws the paint.
    tri_area = np.array([a for a, g in zip(area, good) if g])
    tripo_faces = [k for k, kind in enumerate(kinds) if kind == 'tripo']
    zl = [int(round(cols[k][0, 0] * 3)) for k in tripo_faces]
    means, zarea = pl.zone_means(colour, [uv1[k] for k in tripo_faces], zl, tri_area[tripo_faces])
    for z in range(4):        # a zone no Tripo face took keeps a neighbour's colour
        if zarea[z] <= 0:
            means[z] = means[int(np.argmax(zarea))]
    pl.paint_look(name, mats[0], means)
    shared = pl.prop_paint_uvs(body, mats[0])
    if shared:
        ps.log(f'{name}: props on {mats[0].name} given Paint UVs onto their zone swatches: {shared}')

    # Props worn on the body made a little wider to sit on Tripo's fuller clothes (the Foreman's satchel and strap):
    # scaled about their bone's axis across the body, never along it.
    for prop, (sx, sy) in c.get('prop_scale', {}).items():
        o = bpy.data.objects.get(prop)
        if o is None:
            continue
        co = np.array([v.co[:] for v in o.data.vertices])
        o.data.vertices.foreach_set('co', (co * np.array([sx, sy, 1.0])).astype(np.float32).ravel())
        o.data.update()
        ps.log(f'{name}: {prop} widened x{sx} y{sy} about its pivot')
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    ps.log(f'SK_{name} (Tripo): {tris} triangles, {len(mesh.vertices)} vertices, {len(rig.data.bones)} bones, '
           f'slots {[mt.name for mt in mesh.materials]}, UV {[u.name for u in mesh.uv_layers]}')
    lo, hi = V.min(0), V.max(0)
    ps.log(f'{name}: bounds {np.round(lo, 3)} .. {np.round(hi, 3)}')
    for o in (src,):
        me = o.data
        bpy.data.objects.remove(o)
        bpy.data.meshes.remove(me)
    return body


def _grips(name, rig, Vr, W):
    """A fist round a prop (cfg 'grips': {'r': 'gun'}): the fist moved whole so its middle is on the prop bone's head
    (the grip the script's prop is modeled round), the wrist easing into it by the hand's weight."""
    for sfx, bone in cfg.PEOPLE[name].get('grips', {}).items():
        if bone not in rig.data.bones:
            continue
        hand = sum((np.asarray(w) for b, w in W.items() if b == f'hand_{sfx}' or
                    (b.endswith(f'_01_{sfx}') and b.split('_')[0] in pr.FINGERS)), np.zeros(len(Vr)))
        fist = hand > 0.6
        if fist.sum() < 20:
            continue
        d = np.array(rig.data.bones[bone].head_local) - Vr[fist].mean(0)
        far = np.linalg.norm(d)
        if far > cfg.PEOPLE[name].get('grip_reach', 0.07):
            ps.log(f'{name}: hand_{sfx} is {100 * far:.1f} cm from {bone}: left where the maps put it')
            continue
        Vr = Vr + np.clip(hand, 0.0, 1.0)[:, None] * d
        ps.log(f'{name}: hand_{sfx} moved {100 * far:.1f} cm onto {bone} ({np.round(100 * d, 1)} cm)')
    return Vr


def _hats(name, G):
    """The script's hats (the props on the 'hat' bone that a head wears)."""
    keys = {'Abel': ['HAT'], 'Amos': ['HAT'], 'Hiram': ['CAP'], 'Foreman': ['BOWLER'], 'Thaddeus': []}
    if name == 'DeadMiner':
        return [G['HATS_BUILT'][k][0] for k in ('DeadMinerHat', 'DeadMinerCap') if k in G.get('HATS_BUILT', {})]
    return [G[k] for k in keys.get(name, []) if k in G]


def _poking(Vr, W, hats, rig):
    """How many head vertices (above the hat's band) lie outside the hull of the hat's crown."""
    import looter_ghosts as lg
    import parts_unpaid as pu
    head = np.asarray(W.get('head', np.zeros(len(Vr))))
    band = rig.data.bones['hat'].head_local[2]
    worst = 0
    for hat in hats:
        co = np.array([v.co[:] for v in hat.data.vertices]) + np.array(hat.location)
        # The crown (no brim): the hat's points no further out than its crown's wall a little over the band.
        centre = np.array(hat.location)
        r = np.hypot(co[:, 0] - centre[0], co[:, 1] - centre[1])
        wall = (co[:, 2] > band + 0.01) & (co[:, 2] < band + 0.035)
        reach = np.percentile(r[wall], 90) * 1.08 if wall.sum() > 8 else r.max()
        crown = co[(co[:, 2] > band + 0.008) & (r < reach)]
        if len(crown) < 8:
            continue
        tmp = tr.new_object('_crown', crown, [])
        import bmesh
        bm = bmesh.new()
        for p in crown:
            bm.verts.new(p)
        bmesh.ops.convex_hull(bm, input=bm.verts)
        bm.to_mesh(tmp.data)
        bm.free()
        n, d = pu._supporting_planes(tmp)
        me = tmp.data
        bpy.data.objects.remove(tmp)
        bpy.data.meshes.remove(me)
        P = Vr[(head > 0.5) & (Vr[:, 2] > band + 0.015)]      # over the band: the ears below it may show
        if len(P):
            worst = max(worst, int(((P @ n.T - d) > 0.002).any(1).sum()))
    return worst


def _unpaid_library():
    import looter_miners as mn
    return mn.unpaid_library()
