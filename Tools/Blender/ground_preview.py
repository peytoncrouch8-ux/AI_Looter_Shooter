"""M_Terrain's ground at walking distance, previewed without Unreal: a numpy port of its ground layers (Tools/Unreal/
build_world_materials.py GROUND_WEIGHTS and TERRAIN_CODE: keep this in step with them; the parameters come from its
GROUND_DEFAULTS) on a flat stretch of an area's real macro map, seen from a tour viewpoint (Art/Levels/<Area>/views.json)
at eye height, looking a little down, in a low golden sun. Before (the old grass-only detail) above, after below.

    Tools\\artrun.ps1 -Script Tools\\Blender\\ground_preview.py [-ScriptArgs RansomsRest:WhitlockFields,TutorialIsland:Square]

Writes Saved/ArtPreviews/Terrain/Ground_<Area>_<View>.png. Only the ground: it's flat, without grass, props or shadows,
and the terrain's baked occlusion is 1 (so the hollows' damp doesn't show); textures are filtered with mips and 4x
anisotropy. What it's for: judging the layers' mix, contrast and repeats before an editor run.

Steep faces (TERRAIN_CODE's steep part, STEEP_EDGE_CODE, SIDE_UV_CODE; 2026-10-14):

    Tools\\artrun.ps1 -Script Tools\\Blender\\ground_preview.py -ScriptArgs walls[,RansomsRest:bluffPath:16,...]

renders a cliff point's wall (layout_computed.json cliffs) as its K_ cliff review view sees it (Saved/TourViews/
make_cliff_views.py): a plane leaning back 10 degrees from the point's foot to its height over flat ground, the macro
map under it, the area's *_Steep instance's numbers (layout.json level.materials, with the patches waiting in
Art/Levels/<Area>/patches applied on top). Left: before (the instance's numbers without the steep parameters added
2026-10-14); right: after. Top row in the low sun, bottom row shaded (sky light only). A 5 m panel of the cliff kit's
rock (RockCliff in the area's kit tint, MI_RockCliff_<Area>) stands in front of the wall's left third for comparison.
Writes Saved/ArtPreviews/Terrain/Wall_<Area>_<group>_<index>.png. The wall's baked occlusion is a guess (0.62 at its
foot to 0.85 at its top), no shadows but the bottom row's."""
import ast
import json
import math
import os
import sys
import types

import bpy
import numpy as np

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
TEXTURES = os.path.join(REPO, 'Art', 'Textures')
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Terrain')
VIEWS = ('RansomsRest:WhitlockFields', 'RansomsRest:PointOverValley', 'TutorialIsland:Square',
         'TutorialIsland:Farm road')
WIDTH, HEIGHT, FOV, PITCH, EYE = 1280, 720, 80.0, -16.0, 1.7
WL = np.array([0.299, 0.587, 0.114], np.float32)


def material_constants():
    """build_world_materials.py's upper-case constants (GROUND_DEFAULTS, GRASS_MEAN, ...), without Unreal."""
    path = os.path.join(REPO, 'Tools', 'Unreal', 'build_world_materials.py')

    class Stub:
        def __getattr__(self, name):
            return Stub()

        def __call__(self, *args, **kwargs):
            return Stub()

    stub = types.ModuleType('unreal')
    stub.__getattr__ = lambda name: Stub()
    sys.modules.setdefault('unreal', stub)
    tree = ast.parse(open(path, encoding='utf-8').read(), path)
    keep = [n for n in tree.body if isinstance(n, (ast.Import, ast.ImportFrom, ast.FunctionDef)) or
            (isinstance(n, ast.Assign) and all(isinstance(t, ast.Name) and t.id.isupper() for t in n.targets))]
    module = {}
    exec(compile(ast.Module(body=keep, type_ignores=[]), path, 'exec'), module)
    return module


def load(path, srgb):
    """A PNG as float32 (rows top first), RGB decoded from sRGB when srgb, alpha as is."""
    image = bpy.data.images.load(path, check_existing=False)
    image.colorspace_settings.name = 'Non-Color'
    w, h = image.size
    pixels = np.empty(w * h * 4, np.float32)
    image.pixels.foreach_get(pixels)
    bpy.data.images.remove(image)
    a = pixels.reshape(h, w, 4)[::-1].copy()
    if srgb:
        c = a[..., :3]
        a[..., :3] = np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
    return a


def mips(a):
    levels = [a]
    while levels[-1].shape[0] > 1:
        b = levels[-1]
        levels.append(0.25 * (b[0::2, 0::2] + b[1::2, 0::2] + b[0::2, 1::2] + b[1::2, 1::2]))
    return levels


def bilinear(a, x, y, wrap=True):
    h, w = a.shape[:2]
    x0, y0 = np.floor(x).astype(np.int64), np.floor(y).astype(np.int64)
    fx, fy = (x - x0)[..., None], (y - y0)[..., None]
    if wrap:
        x0, y0 = x0 % w, y0 % h
        x1, y1 = (x0 + 1) % w, (y0 + 1) % h
    else:
        x0, y0 = np.clip(x0, 0, w - 1), np.clip(y0, 0, h - 1)
        x1, y1 = np.clip(x0 + 1, 0, w - 1), np.clip(y0 + 1, 0, h - 1)
    return (a[y0, x0] * (1 - fx) + a[y0, x1] * fx) * (1 - fy) + (a[y1, x0] * (1 - fx) + a[y1, x1] * fx) * fy


def sample(levels, u, v):
    """Trilinear, 4x anisotropic (roughly), tiling: u, v in repeats; the mip from the footprint across the image."""
    size = levels[0].shape[0]
    tu, tv = u * size, v * size
    dux, duy = np.gradient(tu, axis=1), np.gradient(tu, axis=0)
    dvx, dvy = np.gradient(tv, axis=1), np.gradient(tv, axis=0)
    lx, ly = np.hypot(dux, dvx), np.hypot(duy, dvy)
    major, minor = np.maximum(lx, ly), np.maximum(np.minimum(lx, ly), 1e-6)
    lod = np.clip(np.log2(np.maximum(major / np.clip(np.ceil(major / minor), 1.0, 4.0), 1e-6)), 0.0, len(levels) - 1.0)
    out = np.zeros(u.shape + (levels[0].shape[2],), np.float32)
    base = np.floor(lod).astype(np.int64)
    for level in np.unique(base):
        here = base == level
        f = (lod[here] - level)[:, None]
        a = bilinear(levels[level], tu[here] / 2 ** level - 0.5, tv[here] / 2 ** level - 0.5)
        b = bilinear(levels[min(level + 1, len(levels) - 1)], tu[here] / 2 ** (level + 1) - 0.5, tv[here] / 2 ** (level + 1) - 0.5)
        out[here] = a * (1 - f) + b * f
    return out


def ss(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def luma(c):
    return c[..., :3] @ WL


def stats(mean):
    """build_world_materials.layer_stats: luma over each channel's mean, and the luma."""
    return np.array([mean[3] / max(c, 0.004) for c in mean[:3]] + [mean[3]], np.float32)


def layer_stats(name):
    with open(os.path.join(TEXTURES, name, f'T_{name}_stats.json')) as f:
        s = json.load(f)
    return stats(s['meanLinear'] + [s['meanLuma']])


def ground_after(C, macro, select, noise, occlusion, depth, grass, thatch, stony, rock, nG, nT, nS, nR):
    """GROUND_WEIGHTS, then TERRAIN_CODE's ground and rock (no steep faces) and GROUND_NORMAL_CODE."""
    D = C['GROUND_DEFAULTS']
    cues, cues2, damp_p, fade = (np.array(D[k], np.float32) for k in ('GroundCues', 'GroundCues2', 'GroundDamp', 'GroundFade'))
    layers, leaf_tint = np.array(D['GroundLayers'], np.float32), np.array(D['LeafTint'], np.float32)
    gs, ts, st = stats(C['GRASS_MEAN']), layer_stats('GroundThatch'), layer_stats('GroundStony')
    fd = 1.0 + (fade[2] - 1.0) * ss(fade[0] * 100.0, fade[1] * 100.0, depth)
    lm = luma(macro)
    r, g, b = macro[..., 0], macro[..., 1], macro[..., 2]
    warm = (r - g) / np.maximum(r + g, 0.002)
    hi = np.maximum(np.maximum(r, g), b)
    sat = (hi - np.minimum(np.minimum(r, g), b)) / np.maximum(hi, 0.002)
    w = warm + (noise[..., 0] - 0.5) * cues[2]
    lush = 1.0 - ss(cues[0] - 0.05, cues[0] + 0.05, w)
    soil = ss(cues[1] - 0.04, cues[1] + 0.04, w)
    road = ss(0.1, 0.2, select)
    bare = ss(1.0 - cues[3], 1.0 - cues[3] + 0.05, noise[..., 1]) * (1.0 - 0.7 * lush)
    shade = (1.0 - ss(cues2[1] * 0.65, cues2[1], lm)) * (1.0 - ss(0.66, 0.76, sat))
    s_ = np.clip(np.maximum(np.maximum(soil, road), np.maximum(bare, shade * 0.85)), 0.0, 1.0)
    g_ = (1.0 - s_) * lush * np.clip(cues2[0] * (0.4 + 1.2 * noise[..., 3]), 0.0, 1.0)
    t_ = np.maximum(1.0 - s_ - g_, 0.0)
    hg = np.clip(luma(grass) / gs[3] * 0.5, 0.0, 1.0)
    ht = thatch[..., 3]
    hs = np.clip(luma(stony) / st[3] * 0.5, 0.0, 1.0)
    base = np.stack([g_, t_, s_], -1)
    bl = base + np.stack([hg, ht, hs], -1) * cues2[3] * np.clip(base * 4.0, 0.0, 1.0)
    wt = np.maximum(bl - (bl.max(-1, keepdims=True) - 0.2), 0.0)
    wt /= np.maximum(wt.sum(-1, keepdims=True), 1e-4)
    h = (wt * np.stack([hg, ht, hs], -1)).sum(-1)
    damp = (1.0 - ss(damp_p[1], damp_p[0], occlusion)) * np.clip(0.3 + 1.4 * noise[..., 2], 0.0, 1.0)
    damp = np.maximum(damp, shade * soil * 0.6)
    damp = np.clip(damp * (1.25 - 0.6 * h), 0.0, 1.0) * damp_p[3] * fd
    leaf = np.clip((stony[..., 3] - 0.01) * 1.65, 0.0, 1.0) * wt[..., 2] * np.clip(shade * cues2[2] + 0.12, 0.0, 1.0) * fd
    rock_amt = ss(0.55, 0.9, select)
    # TERRAIN_CODE
    k = 1.0 + (np.interp(select, [0, 1], [gs[3], C['ROCK_TRUE_LUMA']]) / np.maximum(0.3 + (0.35 - 0.3) * select, 0.05) - 1.0) * 0.6

    def pattern(c, s, strength):
        lc = np.maximum(luma(c), 0.004)
        chroma = np.clip(c[..., :3] / lc[..., None] * s[:3], 0.6, 1.5)
        return (1.0 + (lc / s[3] - 1.0) * strength)[..., None] * (1.0 + (chroma - 1.0) * layers[3])

    pat = (wt[..., 0:1] * pattern(grass, gs, layers[0]) + wt[..., 1:2] * pattern(thatch, ts, layers[1])
           + wt[..., 2:3] * pattern(stony, st, layers[2]))
    leaf_shade = 0.75 + 0.4 * np.clip((stony[..., 3] - 0.62) / 0.38, 0.0, 1.0)
    pat = pat + (leaf_tint * leaf_shade[..., None] - pat) * leaf[..., None]
    ground = macro * k[..., None] * (1.0 + (pat - 1.0) * fd[..., None]) * (1.0 + (damp_p[2] - 1.0) * damp)[..., None]
    old = grass[..., :3] + (rock[..., :3] - grass[..., :3]) * select[..., None]
    rock_color = macro * (1.0 + (luma(old) / np.maximum(0.3 + 0.05 * select, 0.05) - 1.0) * 0.6)[..., None]
    color = ground + (rock_color - ground) * rock_amt[..., None]
    n = wt[..., 0:1] * nG + wt[..., 1:2] * nT + wt[..., 2:3] * nS
    up = np.array([0.0, 0.0, 1.0], np.float32)
    n = n + (up - n) * np.clip(leaf * 0.8 + damp * 0.5, 0.0, 1.0)[..., None]
    n = n + (nR - n) * rock_amt[..., None]
    return color, n, wt, damp, leaf


def ground_before(macro, select, grass, rock, nG, nR):
    old = grass[..., :3] + (rock[..., :3] - grass[..., :3]) * select[..., None]
    color = macro * (1.0 + (luma(old) / np.maximum(0.3 + 0.05 * select, 0.05) - 1.0) * 0.6)[..., None]
    return color, nG + (nR - nG) * select[..., None]


def shade_and_tonemap(albedo, n_tangent, sky_mask, sun_dir, sun_color, sky_color):
    """Flattened to NormalStrength 0.8 (as M_Terrain), turned into the world (green along -X: Unreal's V runs south on
    UV 1), lit by the sun and the sky, ACES-fitted and sRGB-encoded."""
    t = n_tangent * 2.0 - 1.0
    t = np.array([0.0, 0.0, 1.0], np.float32) + (t - np.array([0.0, 0.0, 1.0], np.float32)) * 0.8
    nw = np.stack([-t[..., 1], t[..., 0], np.maximum(t[..., 2], 0.05)], -1)
    nw /= np.linalg.norm(nw, axis=-1, keepdims=True)
    ndl = np.clip(nw @ sun_dir, 0.0, 1.0)
    light = ndl[..., None] * np.array(sun_color, np.float32) + (0.55 + 0.45 * nw[..., 2:3]) * np.array(sky_color, np.float32)
    c = albedo * light
    c = np.clip((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0)
    c = np.where(sky_mask[..., None], np.array([0.62, 0.74, 0.9], np.float32) ** 2.2, c)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(np.maximum(c, 1e-6), 1.0 / 2.4) - 0.055)


def write_png(path, rgb):
    image = bpy.data.images.new('preview', rgb.shape[1], rgb.shape[0], alpha=True)
    image.colorspace_settings.name = 'Non-Color'
    rgba = np.concatenate([rgb, np.ones(rgb.shape[:2] + (1,), np.float32)], -1)[::-1]
    image.pixels.foreach_set(rgba.astype(np.float32).ravel())
    image.filepath_raw = path
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(image)


def render(C, area, view_name, tex):
    with open(os.path.join(REPO, 'Art', 'Levels', area, 'views.json')) as f:
        views = {v['name']: v for v in json.load(f)['views']}
    with open(os.path.join(REPO, 'Art', 'Levels', area, 'layout_computed.json')) as f:
        computed = json.load(f)
    view = views[view_name]
    (x0, _), (x1, _) = computed['macroMap']['covers']
    half = (x1 - x0) / 200.0
    macro_file = os.path.join(REPO, *computed['macroMap']['texture'].split('/'))
    if macro_file not in tex:
        tex[macro_file] = load(macro_file, True)
    macro_map = tex[macro_file]
    yaw, pitch = math.radians(view['yaw']), math.radians(PITCH)
    f = (WIDTH * 0.5) / math.tan(math.radians(FOV) * 0.5)
    jj, ii = np.meshgrid(np.arange(WIDTH, dtype=np.float32), np.arange(HEIGHT, dtype=np.float32))
    cx, cy = (jj - WIDTH * 0.5 + 0.5) / f, -(ii - HEIGHT * 0.5 + 0.5) / f
    forward = np.array([math.cos(pitch) * math.cos(yaw), math.cos(pitch) * math.sin(yaw), math.sin(pitch)], np.float32)
    right = np.array([-math.sin(yaw), math.cos(yaw), 0.0], np.float32)
    up = np.cross(forward, right)
    d = forward + cx[..., None] * right + cy[..., None] * up
    sky = d[..., 2] > -0.004
    t = np.where(sky, 1e4, -EYE / np.minimum(d[..., 2], -0.004))
    t = np.minimum(t, 400.0)
    X = view['x'] / 100.0 + t * d[..., 0]
    Y = view['y'] / 100.0 + t * d[..., 1]
    depth = t * 100.0
    n = macro_map.shape[0]
    m = bilinear(macro_map, (Y + half) / (2 * half) * n - 0.5, (half - X) / (2 * half) * n - 0.5, wrap=False)
    macro, select = m[..., :3], m[..., 3]
    D = C['GROUND_DEFAULTS']
    u1, v1 = Y, -X
    grass = sample(tex['GrassBC'], u1 * 0.5, v1 * 0.5)
    grass_n = sample(tex['GrassN'], u1 * 0.5, v1 * 0.5)
    rock = sample(tex['RockBC'], u1 * 0.25, v1 * 0.25)
    rock_n = sample(tex['RockN'], u1 * 0.25, v1 * 0.25)
    noise = sample(tex['Noise'], u1 / D['GroundNoiseTile'], v1 / D['GroundNoiseTile'])
    slide = (noise[..., 3] - 0.5) * 2.0 * D['GroundWarp']
    s = D['ThatchScale']
    thatch = sample(tex['ThatchBC'], u1 * s + slide * s, v1 * s + slide * s)
    thatch_n = sample(tex['ThatchN'], u1 * s + slide * s, v1 * s + slide * s)
    s = D['StonyScale']
    stony = sample(tex['StonyBC'], u1 * s - slide * s + 0.37, v1 * s - slide * s + 0.61)
    stony_n = sample(tex['StonyN'], u1 * s - slide * s + 0.37, v1 * s - slide * s + 0.61)
    occlusion = np.ones_like(select)
    # Ransom's Rest's golden hour (layout.json: sun 247.5 degrees, 15 up; sunlit ground shows about 0.72, 0.47, 0.24 of
    # its albedo, area_mosaic.py), the island's afternoon higher and whiter.
    if area == 'RansomsRest':
        az, el, sun_color, sky_color = 247.5, 15.0, (3.4, 2.2, 1.12), (0.32, 0.34, 0.4)
    else:
        az, el, sun_color, sky_color = 230.0, 35.0, (2.9, 2.6, 2.2), (0.42, 0.5, 0.66)
    sun = np.array([math.cos(math.radians(el)) * math.cos(math.radians(az)),
                    math.cos(math.radians(el)) * math.sin(math.radians(az)), math.sin(math.radians(el))], np.float32)
    before, nb = ground_before(macro, select, grass, rock, grass_n[..., :3], rock_n[..., :3])
    after, na, wt, damp, leaf = ground_after(C, macro, select, noise, occlusion, depth, grass, thatch, stony, rock,
                                             grass_n[..., :3], thatch_n[..., :3], stony_n[..., :3], rock_n[..., :3])
    top = shade_and_tonemap(before, nb, sky, sun, sun_color, sky_color)
    bottom = shade_and_tonemap(after, na, sky, sun, sun_color, sky_color)
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, f'Ground_{area}_{view_name.replace(" ", "")}.png')
    gap = np.ones((6, WIDTH, 3), np.float32)
    write_png(path, np.concatenate([top, gap, bottom], 0))
    ground = ~sky
    print(f'LOOTER: {path}: grass {wt[..., 0][ground].mean():.2f}, thatch {wt[..., 1][ground].mean():.2f}, '
          f'stony {wt[..., 2][ground].mean():.2f}, leaves {leaf[ground].mean():.3f}; mean brightness before '
          f'{luma(before)[ground].mean():.4f}, after {luma(after)[ground].mean():.4f}', flush=True)


# --- Steep faces (the walls) ---

WALLS = ('RansomsRest:bluffPath:16', 'RansomsRest:theNose:0', 'RansomsRest:coffinRock:4', 'RansomsRest:ransomsPoint:0',
         'RansomsRest:sinkRamp:9')
WALL_W, WALL_H = 960, 540
WALL_LEAN = 10.0        # degrees the previewed wall leans back
KIT_DENSITY = 180.0     # px per metre of RockCliff on the cliff kit (Cliffs.py DENSITY)


def sample_level(levels, u, v, level):
    """Bilinear at one mip (Texture2DSampleLevel), tiling: u, v in repeats."""
    level = int(min(max(round(level), 0), len(levels) - 1))
    a = levels[level]
    size = a.shape[0]
    return bilinear(a, u * size - 0.5, v * size - 0.5)


def steep_numbers(C, area, window_material):
    """The area's *_Steep instance's numbers over the master's (STEEP_DEFAULTS): (before, after). After: layout.json with
    the patches waiting in Art/Levels/<Area>/patches applied; before: layout.json alone, without the parameters added
    2026-10-14."""
    sys.path.insert(0, os.path.join(REPO, 'Tools'))
    import layout_patch
    with open(os.path.join(REPO, 'Art', 'Levels', area, 'layout.json'), encoding='utf-8') as f:
        data = json.load(f)

    def numbers(spec):
        values = dict(C['STEEP_DEFAULTS'])
        values.update(spec.get('scalars', {}))
        values.update(spec.get('vectors', {}))
        return values
    name = f'MI_{window_material}_Steep'
    before = numbers(data.get('level', {}).get('materials', {}).get(name, {}))
    before.update({k: C['STEEP_DEFAULTS'][k] for k in C['STEEP_ADDED']})
    folder = os.path.join(REPO, 'Art', 'Levels', area, 'patches')
    for patch in sorted(os.listdir(folder)) if os.path.isdir(folder) else []:
        if patch.endswith('.json'):
            with open(os.path.join(folder, patch), encoding='utf-8') as f:
                entries = [e for e in json.load(f) if e['path'].startswith('level.materials') and not e.get('file')]
            layout_patch.apply_patch(data, patch, entries, lambda text: None)
    return before, numbers(data.get('level', {}).get('materials', {}).get(name, {}))


def side_uv(A, Pz, n, noise):
    """SIDE_UV_CODE: the rock map laid on from one side; A and Pz in cm."""
    W = np.sin(A * 0.0011 + 1.3) * 0.14 + np.sin(A * 0.0037 + Pz * 0.0007) * 0.05 + A * 0.00002
    wave = max(n['StrataWave'], 1.0)
    N = sample_level(noise, A * 0.01 / wave, Pz * 2.0 * (1.0 - n['StrataFlat']) * 0.01 / wave, n['StrataSmooth'])[..., 0]
    W = W + (N - 0.5) * 2.0 * n['StrataWarp'] * n['RockScale']
    return A * n['RockScale'] * 0.01, -Pz * n['RockScale'] * 0.01 + W


def steep_color(C, n, P, normal, macro, select, occlusion, steep_raw, tex):
    """TERRAIN_CODE's steep face and STEEP_EDGE_CODE for world points P (cm) on faces with world normals `normal`:
    (color with the face blended in by the edge, the edge, the ambient occlusion output)."""
    ux, vx = side_uv(P[..., 1], P[..., 2], n, tex['StrataNoise'])
    uy, vy = side_uv(P[..., 0], P[..., 2], n, tex['StrataNoise'])
    rock_x = sample(tex['RockBC'], ux, vx)[..., :3]
    rock_y = sample(tex['RockBC'], uy, vy)[..., :3]
    facing = np.abs(normal[..., :2]) ** 4.0
    facing /= np.maximum(facing.sum(-1, keepdims=True), 1e-4)
    fx, fy = facing[..., 0:1], facing[..., 1:2]
    side = rock_x * fx + rock_y * fy
    side_luma = luma(side)
    # STEEP_EDGE_CODE
    h = np.clip(side_luma / max(C['ROCK_TRUE_LUMA'], 0.05) * 0.5, 0.0, 1.0)
    steep = np.clip((steep_raw - 0.5) * n['SteepSharp'] + 0.5 + (h - 0.5) * n['SteepBreakup']
                    + (select - 0.5) * n['SteepPull'], 0.0, 1.0)
    band = sample_level(tex['StrataNoise'], (P[..., 0] + P[..., 1]) / 12000.0, P[..., 2] / 2500.0, 0)[..., 0]
    b = (band - 0.5) * 2.0 * n['SteepVariation']
    banding = (1.0 - 0.5 * np.maximum(-b, 0.0))[..., None] * (
        1.0 + (np.array([1.1, 0.9, 0.78], np.float32) - 1.0) * np.maximum(b, 0.0)[..., None])
    tint = np.array(n['SteepTint'][:3], np.float32)
    old = macro * tint * banding * (1.0 + (side_luma / max(n['RockMeanLuma'], 0.05) - 1.0) * n['SteepDetail'])[..., None]
    beds = np.array(n['SteepBeds'], np.float32)
    bed_v = np.stack([vx, vy], -1) / (max(n['RockScale'], 0.01) * max(beds[0], 0.1))
    bed_i = np.floor(bed_v)
    bed_t = bed_v - bed_i
    h0 = np.mod(np.sin(bed_i * 12.9898 + 4.1) * 43758.5453, 1.0)
    h1 = np.mod(np.sin((bed_i + 1.0) * 12.9898 + 4.1) * 43758.5453, 1.0)
    bs = h0 + (h1 - h0) * ss(0.85, 1.0, bed_t)
    parting = (1.0 - ss(0.0, max(beds[3], 0.01), bed_t)) * (h0 >= 0.45)
    shade = (1.0 + (bs - 0.5) * 2.0 * beds[1]) * (1.0 - beds[2] * parting)
    bed_s = bs[..., 0:1] * fx + bs[..., 1:2] * fy
    bed_mul = (shade[..., 0:1] * fx + shade[..., 1:2] * fy) * (1.0 + (bed_s - 0.5) * np.array([0.06, 0.0, -0.06], np.float32))
    macro_var = 1.0 + (np.clip(luma(macro) / max(n['SteepMacroMean'], 0.01), 0.7, 1.35) - 1.0) * n['SteepMacroKeep']
    kit = side * np.array(n['SteepRockTint'][:3], np.float32) * macro_var[..., None] * banding * bed_mul
    face = old + (kit - old) * n['SteepRock']
    occ = occlusion + (1.0 - occlusion) * steep * n['SteepAOLift']
    # SIDE_NORMAL_CODE, lerped in by SteepNormalStrength (in the world here: the wall's own frame is its normal).
    nx = sample(tex['RockN'], ux, vx)[..., :3] * 2.0 - 1.0
    ny = sample(tex['RockN'], uy, vy)[..., :3] * 2.0 - 1.0
    sx, sy = np.where(normal[..., 0:1] >= 0.0, 1.0, -1.0), np.where(normal[..., 1:2] >= 0.0, 1.0, -1.0)
    green = n['SteepNormalGreen']
    from_x = np.concatenate([nx[..., 2:3] * sx, nx[..., 0:1], nx[..., 1:2] * green], -1)
    from_y = np.concatenate([ny[..., 0:1], ny[..., 2:3] * sy, ny[..., 1:2] * green], -1)
    relief = from_x * fx + from_y * fy
    relief /= np.maximum(np.linalg.norm(relief, axis=-1, keepdims=True), 1e-6)
    k = (steep * n['SteepNormalStrength'])[..., None]
    lit_normal = normal + (relief - normal) * k
    lit_normal /= np.maximum(np.linalg.norm(lit_normal, axis=-1, keepdims=True), 1e-6)
    return face, steep, occ, lit_normal


def light(albedo, normal, occlusion, sun_dir, sun_color, sky_color, sky_mask):
    """Sun (zero for the shaded row) and sky (times the ambient occlusion), ACES-fitted and sRGB-encoded."""
    ndl = np.clip(normal @ sun_dir, 0.0, 1.0)
    lit = ndl[..., None] * np.array(sun_color, np.float32) + (
        (0.55 + 0.45 * normal[..., 2:3]) * np.array(sky_color, np.float32) * occlusion[..., None])
    c = albedo * lit
    c = np.clip((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0)
    c = np.where(sky_mask[..., None], np.array([0.62, 0.74, 0.9], np.float32) ** 2.2, c)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(np.maximum(c, 1e-6), 1.0 / 2.4) - 0.055)


def wall(C, item, tex):
    """One cliff point's wall, before and after, sunlit and shaded (see the docstring)."""
    area, group, index = item.split(':')
    with open(os.path.join(REPO, 'Art', 'Levels', area, 'layout_computed.json')) as f:
        computed = json.load(f)
    point = computed['cliffs'][group][int(index)]
    fx_, fy_, fz_ = (v / 100.0 for v in point['location'])
    height = max(point.get('height', point.get('drop', 6.0)) / 100.0, 2.0)
    yaw = math.radians(point['yaw'])
    out = np.array([math.cos(yaw), math.sin(yaw), 0.0], np.float32)
    along = np.array([-math.sin(yaw), math.cos(yaw), 0.0], np.float32)
    up = np.array([0.0, 0.0, 1.0], np.float32)
    foot = np.array([fx_, fy_, fz_], np.float32)
    lean = math.radians(WALL_LEAN)
    n_wall = out * math.cos(lean) + up * math.sin(lean)
    # The K_ view (make_cliff_views.py): stood out along the point's yaw, looking back at the face.
    d = min(35.0, max(13.0, height * 1.4))
    pitch = math.radians(max(-12.0, min(30.0, math.degrees(math.atan2(height * 0.45 - 1.7, d)))))
    eye = foot + out * d + up * 1.7
    forward = -out * math.cos(pitch) + up * math.sin(pitch)
    right = np.cross(forward, up)
    right /= np.linalg.norm(right)
    cam_up = np.cross(right, forward)
    f = (WALL_W * 0.5) / math.tan(math.radians(80.0) * 0.5)
    jj, ii = np.meshgrid(np.arange(WALL_W, dtype=np.float32), np.arange(WALL_H, dtype=np.float32))
    ray = (forward + ((jj - WALL_W * 0.5 + 0.5) / f)[..., None] * right
           - ((ii - WALL_H * 0.5 + 0.5) / f)[..., None] * cam_up)
    ray /= np.linalg.norm(ray, axis=-1, keepdims=True)
    inf = np.float32(1e9)
    # The wall's plane, the ground in front, the top behind, and the kit panel standing 0.8 m out over its left third.
    t_wall = ((foot - eye) @ n_wall) / np.where(np.abs(ray @ n_wall) > 1e-6, ray @ n_wall, 1e-6)
    p_wall = eye + t_wall[..., None] * ray
    rise = p_wall[..., 2] - fz_
    t_wall = np.where((t_wall > 0) & (rise >= 0.0) & (rise <= height), t_wall, inf)
    t_ground = (fz_ - eye[2]) / np.where(ray[..., 2] < -1e-6, ray[..., 2], -1e-6)
    s_ground = (eye + t_ground[..., None] * ray - foot) @ out
    t_ground = np.where((ray[..., 2] < -1e-6) & (s_ground >= 0.0), t_ground, inf)
    t_top = (fz_ + height - eye[2]) / np.where(np.abs(ray[..., 2]) > 1e-6, ray[..., 2], 1e-6)
    s_top = (eye + t_top[..., None] * ray - foot) @ out
    t_top = np.where((t_top > 0) & (s_top <= -height * math.tan(lean)), t_top, inf)
    panel_at = foot + out * 0.8
    t_panel = ((panel_at - eye) @ out) / np.where(np.abs(ray @ out) > 1e-6, ray @ out, 1e-6)
    p_panel = eye + t_panel[..., None] * ray
    a_panel = (p_panel - foot) @ along
    z_panel = p_panel[..., 2] - fz_
    t_panel = np.where((t_panel > 0) & (a_panel >= 2.0) & (a_panel <= 7.0) & (z_panel >= 0.0)
                       & (z_panel <= height * 0.92), t_panel, inf)
    hit = np.stack([t_wall, t_ground, t_top, t_panel], -1)
    hit = np.where(hit > 400.0, inf, hit)      # grazing rays: past 400 m it's sky
    which = np.argmin(hit, -1)
    t = hit.min(-1)
    sky = t >= inf
    P = eye + np.where(sky, 0.0, t)[..., None] * ray            # metres
    Pc = P * 100.0
    # The macro map under each point (its window), as UV 0 lays it on.
    win = next(w for w in computed['macroMap'].get('windows', [computed['macroMap']])
               if w['covers'][0][0] <= point['location'][0] < w['covers'][1][0]
               and w['covers'][0][1] <= point['location'][1] < w['covers'][1][1])
    path = os.path.join(REPO, *win['texture'].split('/'))
    if path not in tex:
        tex[path] = load(path, True)
    macro_map = tex[path]
    (x0, y0), (x1, y1) = win['covers']
    size = macro_map.shape[0]
    m = bilinear(macro_map, (Pc[..., 1] - y0) / (y1 - y0) * size - 0.5, (x1 - Pc[..., 0]) / (x1 - x0) * size - 0.5,
                 wrap=False)
    macro, select = m[..., :3], m[..., 3]
    normal = np.where((which == 0)[..., None], n_wall, np.where((which == 3)[..., None], out, up))
    steep_raw = (which == 0).astype(np.float32)
    s_out = (P - foot) @ out
    occlusion = np.where(which == 0, 0.62 + 0.23 * np.clip((P[..., 2] - fz_) / height, 0.0, 1.0),
                         np.where(which == 1, 0.65 + 0.2 * ss(0.0, 4.0, s_out), 0.85)).astype(np.float32)
    # The ground and the top: GROUND_WEIGHTS and TERRAIN_CODE's ground (flat, as render()).
    D = C['GROUND_DEFAULTS']
    u1, v1 = Pc[..., 1] / 100.0, -Pc[..., 0] / 100.0
    noise = sample(tex['Noise'], u1 / D['GroundNoiseTile'], v1 / D['GroundNoiseTile'])
    slide = (noise[..., 3] - 0.5) * 2.0 * D['GroundWarp']
    s = D['ThatchScale']
    thatch = sample(tex['ThatchBC'], u1 * s + slide * s, v1 * s + slide * s)
    s = D['StonyScale']
    stony = sample(tex['StonyBC'], u1 * s - slide * s + 0.37, v1 * s - slide * s + 0.61)
    grass = sample(tex['GrassBC'], u1 * 0.5, v1 * 0.5)
    rock = sample(tex['RockBC'], u1 * 0.25, v1 * 0.25)
    flat_n = np.zeros(macro.shape, np.float32)
    ground, _, _, _, _ = ground_after(C, macro, select, noise, occlusion, t * 100.0, grass, thatch, stony, rock,
                                      flat_n, flat_n, flat_n, flat_n)
    # The kit panel: RockCliff at the kit's density in the kit's tint, its occlusion 0.85 at DiffuseAO 0.4.
    kit_tint = np.array([0.66, 0.58, 0.5], np.float32)
    for name, spec in data_materials(area).items():
        if spec.get('parent') == 'MI_RockCliff':
            kit_tint = np.array(spec.get('vectors', {}).get('Tint', kit_tint)[:3], np.float32)
    on_panel = which == 3          # off it, a_panel and z_panel run off to where a ray grazes the panel's plane
    ku = np.where(on_panel, a_panel, 0.0) * KIT_DENSITY / 1024.0
    kv = -np.where(on_panel, z_panel, 0.0) * KIT_DENSITY / 1024.0
    kit = sample(tex['RockBC'], ku, kv)[..., :3] * kit_tint * (1.0 + (0.85 - 1.0) * 0.4)
    kn = sample(tex['RockN'], ku, kv)[..., :3] * 2.0 - 1.0
    kit_normal = out * kn[..., 2:3] + along * kn[..., 0:1] + up * kn[..., 1:2]
    kit_normal /= np.maximum(np.linalg.norm(kit_normal, axis=-1, keepdims=True), 1e-6)
    sun_az, sun_el = math.radians(247.5), math.radians(15.0)
    sun = np.array([math.cos(sun_el) * math.cos(sun_az), math.cos(sun_el) * math.sin(sun_az), math.sin(sun_el)],
                   np.float32)
    rows = []
    numbers = steep_numbers(C, area, win['material'])
    for sun_color in ((3.4, 2.2, 1.12), (0.0, 0.0, 0.0)):
        row = []
        for n in numbers:
            face, steep, occ, lit_normal = steep_color(C, n, Pc, normal, macro, select, occlusion,
                                                        np.where(which == 0, steep_raw, 0.0), tex)
            # TERRAIN_CODE's last line (ground_after leaves it out): the occlusion, lifted on the face, darkens the colour.
            albedo = (ground + (face - ground) * steep[..., None]) * (1.0 + (occ - 1.0) * n['DiffuseAO'])[..., None]
            albedo = np.where((which == 3)[..., None], kit, albedo)
            nrm = np.where((which == 3)[..., None], kit_normal, np.where((which == 0)[..., None], lit_normal, normal))
            ao = np.where(which == 3, 0.85, occ)
            row.append(light(albedo, nrm, ao, sun, sun_color, (0.32, 0.34, 0.4), sky))
            wall_px = which == 0
            print(f'LOOTER wall {item} {"sun" if sun_color[0] else "shade"} {"before" if n is numbers[0] else "after"}: '
                  f'wall albedo luma {luma(albedo)[wall_px].mean():.3f}, kit panel {luma(kit)[which == 3].mean():.3f}',
                  flush=True)
        rows.append(np.concatenate([row[0], np.ones((WALL_H, 6, 3), np.float32), row[1]], 1))
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, f'Wall_{area}_{group}_{index}.png')
    write_png(path, np.concatenate([rows[0], np.ones((6, rows[0].shape[1], 3), np.float32), rows[1]], 0))
    print(f'LOOTER: {path}', flush=True)


def data_materials(area):
    with open(os.path.join(REPO, 'Art', 'Levels', area, 'layout.json'), encoding='utf-8') as f:
        return json.load(f).get('level', {}).get('materials', {})


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    wanted = [a for arg in argv for a in arg.split(',') if a] or list(VIEWS)
    walls = 'walls' in wanted
    if walls:
        wanted = [w for w in wanted if w != 'walls'] or list(WALLS)
    C = material_constants()
    tex = {}
    for key, set_name, kind, srgb in (('GrassBC', 'GroundGrass', 'BC', True), ('GrassN', 'GroundGrass', 'N', False),
                                      ('RockBC', 'RockCliff', 'BC', True), ('RockN', 'RockCliff', 'N', False),
                                      ('ThatchBC', 'GroundThatch', 'BC', True), ('ThatchN', 'GroundThatch', 'N', False),
                                      ('StonyBC', 'GroundStony', 'BC', True), ('StonyN', 'GroundStony', 'N', False),
                                      ('Noise', 'GroundNoise', 'M', False), ('StrataNoise', 'MacroNoise', 'M', False)):
        tex[key] = mips(load(os.path.join(TEXTURES, set_name, f'T_{set_name}_{kind}.png'), srgb))
    for item in wanted:
        if walls:
            wall(C, item, tex)
            continue
        area, view = item.split(':', 1)
        render(C, area, view, tex)


main()
