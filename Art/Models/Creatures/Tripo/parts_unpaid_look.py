"""The Tripo Unpaid's look in M_Ghost's terms (Unpaid.py's docstring: vertex colour R the tint zone, G cavity, B the
ember edge, A the fade; UV 0 the Polymer box projection, the shroud's strip UVs), and swap(), which puts the Tripo body
and hat in the script model's place. Tripo's paint says which zone a face is (skin, shirt, the dark
waistcoat, tie and hair); since 2026-10-31 the paint itself is drawn too (parts_unpaid_paint.py, UV 1; the flat zones read
washed out, near white, in the game), each set of clothes (MI_Ghost_A/B/C) recolouring it by zone (paint_look)."""
import math

import bpy
import numpy as np

import looter_textures as lt
import looter_tripo_rig as tr
import parts_unpaid as pu
import parts_unpaid_rig as pr

SKIN, SHIRT, VEST, ACCENT = 0, 1, 2, 3
COAL_RADII = (0.045, 0.045, 0.042)      # m: the coal (its noise adds up to 12%: inside the 7 cm crit sphere)
BOWL = (1.0, -4.6, 66.5)                # GLB units: the middle of the hole in the model's chest, where the coal goes
EYES = ((0.9, -5.9, 90.0), (-2.3, -5.9, 90.0))   # GLB units: under the brim, either side of the face's middle (x -0.7)
EYE_RADIUS = 0.011                      # m: each eye's glow


# --- Zones from Tripo's paint ---

def paint_fields(src, V, faces, skin_ok=None, hands=None, face=None):
    """Soft per-vertex scores from Tripo's paint under each vertex (sRGB), smoothed a little over the mesh: skin (its
    cool blue-grey), shirt (pale and warm), dark cloth (waistcoat, tie, hair); and his two eyes (spots to glow)."""
    surf = tr.Surface(src['V'], src['F'], src['uv'])
    tri, bary, _, _ = surf.nearest(V)
    rgb = tr.sample_image(src['pixels'], surf.uv_at(tri, bary))[:, :3]
    r, g, b = rgb[:, 0], rgb[:, 1], rgb[:, 2]
    luma = 0.299 * r + 0.587 * g + 0.114 * b
    warm = r - b
    skin = pr.smoothstep(0.004, -0.014, warm) * pr.smoothstep(0.1, 0.18, luma)
    if skin_ok is not None:     # bare skin only where a man shows it: the head and neck, the forearms and hands
        skin = skin * np.clip(skin_ok, 0.0, 1.0)
    shirt = (1.0 - skin) * pr.smoothstep(0.30, 0.42, luma)
    dark = np.clip(1.0 - skin - shirt, 0.0, 1.0)
    if hands is not None:       # a hand's painted creases and shadows are still skin
        skin = skin + dark * hands
        dark = dark * (1.0 - hands)
    if face is not None:        # the face's pale highlights are skin, not shirt
        skin = skin + shirt * face
        shirt = shirt * (1.0 - face)
        # and the brim's shadow Tripo painted over the brow and eyes is skin too, not hair (hair is at the sides)
        front = face * pr.smoothstep(-1.5, -3.0, glb(V)[:, 1])
        skin = skin + dark * front
        dark = dark * (1.0 - front)
    G = glb(V)
    tie = dark * ((np.abs(G[:, 0]) < 3.0) & (G[:, 2] > 72.5) & (G[:, 2] < 82.5) & (G[:, 1] < -3.0))
    out = dict(skin=skin, shirt=shirt, dark=dark, tie=tie)
    out = {k: tr.smooth_values(v, faces, passes=2, amount=0.5) for k, v in out.items()}
    # Tripo painted one orange glint (his left eye); both eyes burn: a small hot spot on the skin nearest each.
    spots = []
    for e in EYES:          # the skin in front at each eye: the frontmost point within 0.4 GLB units of its x and z
        near = (np.abs(G[:, 0] - e[0]) < 0.4) & (np.abs(G[:, 2] - e[2]) < 0.4) & (G[:, 1] < -3.0)
        spots.append(V[near][np.argmin(V[near, 1])] if near.any() else pu.T(e))
    out["eye"] = np.max([pr.smoothstep(EYE_RADIUS, EYE_RADIUS * 0.35, np.linalg.norm(V - c, axis=1))
                         for c in spots], axis=0)
    return out


def glb(V):
    """Tripo-space points back in the GLB's units (where the measured landmarks are)."""
    V = np.asarray(V, float)
    return np.column_stack([V[:, 0] / pu.SCALE, V[:, 1] / pu.SCALE + pu.DEPTH, (V[:, 2] - pu.LIFT) / pu.SCALE])


def zone_mesh(src, V, faces, robe, carried, skin_ok=None, hands=None, face=None):
    """The mesh cut along its colour borders (looter_tripo_rig.cut_field: the skin's edge, the shirt's, the tie's and
    the waistcoat's hem above the robe), each face then given its zone: the robe (below the hem) the shroud's (0), the
    rest by the paint under it. carried: per-vertex arrays taken along through the cuts. Returns (V, faces, zone per
    face, eye glow per vertex, carried)."""
    fields = paint_fields(src, V, faces, skin_ok, hands, face)
    fields['robe'] = robe
    for key in ('skin', 'shirt', 'tie', 'robe'):
        n = len(V)
        V, faces, parents = tr.cut_field(V, faces, fields[key], 0.5)
        fields = {k: tr.carry(v, parents, n) for k, v in fields.items()}
        carried = {k: tr.carry(v, parents, n) for k, v in carried.items()}
    faces = tr.tidy_polygons(V, faces)
    zone = np.empty(len(faces), dtype=np.int64)
    for i, f in enumerate(faces):
        c = {k: v[f].mean() for k, v in fields.items()}
        if c['robe'] > 0.5:
            zone[i] = SKIN
        elif c['tie'] > 0.5:
            zone[i] = ACCENT
        else:
            zone[i] = (SKIN, SHIRT, VEST)[int(np.argmax([c['skin'], c['shirt'], c['dark']]))]
    counts = np.bincount(zone, minlength=4)
    pu.log(f'zones: skin and shroud {counts[0]}, shirt {counts[1]}, waistcoat and hair {counts[2]}, tie {counts[3]} '
           f'faces; {len(faces)} faces after the colour borders were cut')
    return V, faces, zone, fields['eye'], carried

# --- Fade and strip UVs on the robe ---

def robe_fade(V, robe):
    """A per vertex: solid down to a third of the robe, thinning to wisps at its ends (M_Ghost breaks it up)."""
    top, bottom = pu.ROBE_TOP, 0.12
    t = np.clip((top - V[:, 2]) / (top - bottom), 0.0, 1.0)
    fade = 1.0 - 0.88 * pr.smoothstep(0.3, 1.0, t)
    return np.where(robe > 0.0, 1.0 - robe * (1.0 - fade), 1.0)


def robe_uv(V, robe):
    """Per vertex (angle round the robe, meters a radian, minus the meters down it), NaN off the robe: Unpaid.py's
    strip_uvs reads it (so M_Ghost's streaks drift up the strips)."""
    ang = np.arctan2(V[:, 0], -(V[:, 1] - 0.005))
    radius = np.maximum(np.hypot(V[:, 0], V[:, 1] - 0.005), 0.03)
    down = -(pu.ROBE_TOP - V[:, 2]) * pr.ROBE_STRETCH
    uv = np.column_stack([ang, radius, down])
    uv[robe < 0.5] = np.nan
    return uv


# --- The coal ---

def coal_piece(U, center):
    """The script model's coal (a lumpy ball on the 'coal' bone), a little smaller to sit in the model's hole."""
    V, F = U['ellipsoid'](center, COAL_RADII, segs=12, rings=8)
    d = V - center
    V = V + d * (0.12 * U["pnoise"](V, 30.0, 3))[:, None]
    return V, F


def seat_bowl(V, M, rig, U, share=0.75):
    """Moves the chest's hole (where the model's own heart sits) most of the way (share) onto the rig's coal bone, the
    cloth round it easing along; the coal goes the rest of the way to meet it (a little off its bone, well inside the
    7 cm crit sphere). Returns (V, where the coal goes)."""
    bowl = pu.T(BOWL)
    m = M['spine_02']
    here = m[:3, :3] @ bowl + m[:3, 3]
    want = np.array(rig.data.bones['coal'].head_local)
    shift = want - here
    d = np.linalg.norm(V - here, axis=1)
    pull = pr.smoothstep(0.16, 0.06, d) * share
    pu.log(f'the chest hole moved {100 * np.linalg.norm(shift):.1f} cm onto the coal ({np.round(100 * shift, 1)} cm)')
    return V + pull[:, None] * shift, want - (1.0 - share) * shift


# --- The hat ---

# Worn felt (the playtest 2026-10-30: the hat read as a near-white disc at range). It wears the vest's zone (Zone3Color:
# MI_Ghost_A's brown wool, B's charcoal, C's drab), darkened by M_Ghost's cavity (vertex G: Dark = 1 - 0.95 G^0.6), which
# dims the pale rim on it as well as its colour: the felt overall, its brim's top a little lighter (dust settled, uneven),
# under the brim and round the band darker, and its edge worn dark (edge: from edge_from of the way out; it also keeps the
# brim's tips, the one place a rim stays, from flaring).
FELT_SHADE = dict(felt=0.28, dust=0.17, dust_vary=0.04, under=0.7, band=0.3, crease=0.42, crease_reach=0.015, edge=0.6,
                  edge_from=0.65)
# The white disc was mostly M_Ghost's rim, (1 - facing)^RimPower: a flat brim seen near edge-on (an Unpaid's hat is at
# the player's eye height) faces away from the eye everywhere. The brim's normals lean out from the crown, its top's and
# underside's alike, more toward its edge (a lens's shading: the lean's tangent from BRIM_LEAN[0] by the crown to [1] at
# the edge, ~39 to ~63 degrees), so seen from the side it faces the eye and takes its own colour; only its tips left and
# right, as any silhouette, keep a rim.
BRIM_LEAN = (0.8, 2.0)


def hat_felt(U, V, C, Nf, r, band, z0, z1, middle):
    """The felt's shade (vertex G) per face and per vertex, and each vertex's brim lean (tilt_brim; 0 on the crown).
    C, Nf, r: face centres, normals, distance from the hat's axis; z0, z1: the band's bottom and top."""
    s = FELT_SHADE
    r_v = np.linalg.norm(V[:, :2] - middle, axis=1)
    crown = float(np.median(r[band])) if band.any() else 0.13
    brim_v = pr.smoothstep(crown + 0.008, crown + 0.04, r_v)
    brim_f = pr.smoothstep(crown + 0.008, crown + 0.04, r)
    face = np.full(len(C), s['felt'])
    dust = np.clip(s['dust'] + s['dust_vary'] * U['pnoise'](C, 14.0, 7), 0.0, 1.0)
    top = brim_f * pr.smoothstep(0.2, 0.5, Nf[:, 2])
    under = brim_f * pr.smoothstep(-0.2, -0.5, Nf[:, 2])
    face = face + (dust - face) * top
    face = face + (s['under'] - face) * under
    face[band] = s['band']
    # The creases each side of the band, on the crown: a shadow fading over crease_reach.
    gap = np.minimum(np.abs(V[:, 2] - z0), np.abs(V[:, 2] - z1))
    vertex = s['crease'] * pr.smoothstep(s['crease_reach'], 0.0, gap) * (1.0 - brim_v)
    # How far out on the brim, 0 at the crown to 1 at its edge in that direction (the brim is not round): the widest
    # reach in each of 48 bearings, smoothed with its neighbours.
    bearing = np.arctan2(V[:, 1] - middle[1], V[:, 0] - middle[0])
    bins = ((bearing + np.pi) / (2.0 * np.pi) * 48).astype(int) % 48
    reach = np.full(48, crown + 0.05)
    np.maximum.at(reach, bins, r_v)
    reach = np.maximum(reach, (np.roll(reach, 1) + reach + np.roll(reach, -1)) / 3.0)
    out = np.clip((r_v - crown) / np.maximum(reach[bins] - crown, 1e-3), 0.0, 1.0)
    vertex = np.maximum(vertex, s['edge'] * pr.smoothstep(s['edge_from'], 1.0, out) * brim_v)
    pu.log(f'hat: crown radius {100 * crown:.1f} cm, brim out to {100 * r_v.max():.1f} cm; felt faces: '
           f'{int((top > 0.5).sum())} brim top (dusty), {int((under > 0.5).sum())} under the brim')
    lean = brim_v * (BRIM_LEAN[0] + (BRIM_LEAN[1] - BRIM_LEAN[0]) * out)
    return dict(face=face, vertex=vertex, lean=lean)


def tilt_brim(mesh, V, lean, middle):
    """Leans the brim's normals out from the crown (lean: per vertex, the tangent of the lean; 0 on the crown),
    each corner's own normal kept otherwise (the sharp edges set from the angle stay sharp)."""
    n = np.empty(len(mesh.loops) * 3, np.float32)
    mesh.corner_normals.foreach_get('vector', n)
    n = n.reshape(-1, 3).astype(float)
    vi = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', vi)
    out = np.column_stack([V[:, :2] - middle, np.zeros(len(V))])
    out /= np.maximum(np.linalg.norm(out, axis=1, keepdims=True), 1e-9)
    n = n + lean[vi][:, None] * out[vi]
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9)
    mesh.normals_split_custom_set([tuple(v) for v in n])


def finish_hat(U, V, faces, M, src):
    """SM_UnpaidHat from the Tripo hat (Tripo space): carried by the head's map, its pivot the 'hat' bone's head (as
    Unpaid.py's build_hat), the felt zone 2 and the band zone 3 (a ring round the crown just over the brim); the felt
    shaded and the brim's normals leaned out (FELT_SHADE, BRIM_LEAN)."""
    m = M['hat']
    V = V @ m[:3, :3].T + m[:3, 3]
    pivot = np.array(U['head']['hat'])
    middle = np.median(V[:, :2], axis=0)
    r_v = np.linalg.norm(V[:, :2] - middle, axis=1)
    # The band: a ring 2.8 cm tall round the crown just over the brim (Tripo's crown flares into the brim with no edge),
    # its edges cut level so they run clean round it.
    N = tr.vertex_normals(V, faces)
    top = (r_v > 0.12) & (r_v < 0.14) & (N[:, 2] > 0.6)
    z0 = (np.median(V[top, 2]) if top.any() else V[:, 2].min()) + 0.008
    z1 = z0 + 0.028
    for level in (z0, z1):
        V, faces, parents = tr.cut_field(V, faces, V[:, 2], level)
    faces = tr.tidy_polygons(V, faces)
    C = np.array([V[f].mean(0) for f in faces])
    Nf = np.array([np.cross(V[f[1]] - V[f[0]], V[f[2]] - V[f[0]]) for f in faces])
    Nf /= np.maximum(np.linalg.norm(Nf, axis=1, keepdims=True), 1e-12)
    radial = C[:, :2] - middle
    r = np.linalg.norm(radial, axis=1)
    outward = (Nf[:, :2] * radial).sum(1) > 0.25 * r
    band = (C[:, 2] > z0) & (C[:, 2] < z1) & (r < 0.16) & outward & (Nf[:, 2] < 0.85)
    at = (C[:, 2] > z0) & (C[:, 2] < z1)
    pu.log(f"hat: at the band's height radius {np.round(np.percentile(r[at], [5, 50, 95]), 3)}, normal z "
           f"{np.round(np.percentile(Nf[at, 2], [5, 50, 95]), 2)}, {int(band.sum())} faces")
    zone = np.full(len(faces), VEST)
    zone[band] = ACCENT
    pu.log(f"hat: band {100 * (z0 - pivot[2]):.1f}-{100 * (z1 - pivot[2]):.1f} cm over the hat bone")
    felt = hat_felt(U, V, C, Nf, r, band, z0, z1, middle)
    mesh = tr.new_mesh('UnpaidHat', V - pivot, faces)
    if len(mesh.polygons) != len(faces):
        raise RuntimeError('validate() changed the Tripo hat')
    mesh.materials.append(bpy.data.materials['Ghost_A'])
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    tilt_brim(mesh, V, felt['lean'], middle)
    hat = bpy.data.objects.new('UnpaidHat', mesh)
    bpy.context.scene.collection.objects.link(hat)
    hat.location = tuple(pivot)
    bpy.context.view_layer.update()
    lt.box_uv(hat, 'Polymer')
    occ = tr.ao(V, faces, rays=16, reach=0.05)
    cavity = np.clip((0.8 - occ) / 0.6, 0.0, 1.0) * 0.6
    col = mesh.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    face_of = np.repeat(np.arange(len(faces)), [len(f) for f in faces])
    shade = np.maximum.reduce([cavity[loops], felt['face'][face_of], felt['vertex'][loops]])
    pu.log(f'hat: felt shade (vertex G) {np.round(np.percentile(shade, [5, 50, 95]), 3)}')
    rgba = np.column_stack([zone[face_of] / 3.0, shade, np.zeros(len(loops)), np.ones(len(loops))])
    col.data.foreach_set('color', rgba.astype(np.float32).ravel())
    hat['Nanite'] = 0
    hat['LODs'] = '50,25'
    hat['LODScreens'] = '0.25,0.1'
    hat['Collision'] = 'None'
    pu.log(f'SM_UnpaidHat: {sum(len(f) - 2 for f in faces)} triangles ({sum(len(f) == 4 for f in faces)} quads), '
           f'band {int(band.sum())} faces')
    return hat


# --- The paint in M_Ghost (2026-10-31) ---

# Ghost_A's look over Tripo's paint (the user, 2026-10-31: "washed out pale/white"): the paint carries the light and the
# cloth's detail itself, so M_Ghost's painted-in extras are turned well down; the cold rim stays a thin edge (it was a
# near-white 2.2 over the pale zones), the body's own glow nearly off.
PAINT_LOOK = dict(PaintAmount=1.0, RimStrength=0.8, RimPower=4.0, GlowStrength=0.03, TopLightLow=0.9,
                  TopLightHigh=1.06, PropTopLight=1.0, ClothWeave=0.06, ClothFolds=0.12, ClothStains=0.15,
                  SkinCloth=0.2, ColdRim=0.5)


def paint_look(ghost, means):
    """Ghost_A (Unpaid.py's materials()) set to draw the paint: its BaseColorMap the paint (TextureSet UnpaidPaint, on
    UV 1 while PaintAmount is 1), each zone's colour and PaintZone<N> the zone's mean paint colour (so its own clothes
    are Tripo's paint exactly), and PAINT_LOOK."""
    import parts_unpaid_paint as pp
    ghost['TextureSet'] = pp.SET
    for k, mean in enumerate(means):
        ghost[f'Zone{k + 1}Color'] = pp.to_hex(mean)
        ghost[f'PaintZone{k + 1}'] = pp.to_hex(mean)
    for key, value in PAINT_LOOK.items():
        ghost[key] = value
    bsdf = next(n for n in ghost.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(pp.to_hex(means[0]))
    pu.log('paint: zones ' + ', '.join(f'{name} {pp.to_hex(m)}' for name, m in
                                      zip(('skin and robe', 'shirt', 'waistcoat and hat', 'tie and band'), means)))
