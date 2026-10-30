"""The Tripo Unpaid's look in M_Ghost's terms (Unpaid.py's docstring: vertex colour R the tint zone, G cavity, B the
ember edge, A the fade; UV 0 the Polymer box projection, the shroud's strip UVs), and swap(), which puts the Tripo body
and hat in the script model's place. Tripo's own paint is not drawn (its smears, baked light and seams went with it):
it only says which zone a face is (skin, shirt, the dark waistcoat, tie and hair), so every set of clothes
(MI_Ghost_A/B/C) tints it like the script model."""
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

def finish_hat(U, V, faces, M, src):
    """SM_UnpaidHat from the Tripo hat (Tripo space): carried by the head's map, its pivot the 'hat' bone's head (as
    Unpaid.py's build_hat), the felt zone 2 and the band zone 3 (a ring round the crown just over the brim)."""
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
    mesh = tr.new_mesh('UnpaidHat', V - pivot, faces)
    if len(mesh.polygons) != len(faces):
        raise RuntimeError('validate() changed the Tripo hat')
    mesh.materials.append(bpy.data.materials['Ghost_A'])
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
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
    rgba = np.column_stack([zone[face_of] / 3.0, cavity[loops], np.zeros(len(loops)), np.ones(len(loops))])
    col.data.foreach_set('color', rgba.astype(np.float32).ravel())
    hat['Nanite'] = 0
    hat['LODs'] = '50,25'
    hat['LODScreens'] = '0.25,0.1'
    hat['Collision'] = 'None'
    pu.log(f'SM_UnpaidHat: {sum(len(f) - 2 for f in faces)} triangles ({sum(len(f) == 4 for f in faces)} quads), '
           f'band {int(band.sum())} faces')
    return hat
