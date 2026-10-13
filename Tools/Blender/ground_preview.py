"""M_Terrain's ground at walking distance, previewed without Unreal: a numpy port of its ground layers (Tools/Unreal/
build_world_materials.py GROUND_WEIGHTS and TERRAIN_CODE: keep this in step with them; the parameters come from its
GROUND_DEFAULTS) on a flat stretch of an area's real macro map, seen from a tour viewpoint (Art/Levels/<Area>/views.json)
at eye height, looking a little down, in a low golden sun. Before (the old grass-only detail) above, after below.

    Tools\\artrun.ps1 -Script Tools\\Blender\\ground_preview.py [-ScriptArgs RansomsRest:WhitlockFields,TutorialIsland:Square]

Writes Saved/ArtPreviews/Terrain/Ground_<Area>_<View>.png. Only the ground: it's flat, without grass, props or shadows,
and the terrain's baked occlusion is 1 (so the hollows' damp doesn't show); textures are filtered with mips and 4x
anisotropy. What it's for: judging the layers' mix, contrast and repeats before an editor run."""
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
    lod = np.clip(np.log2(np.maximum(major / np.minimum(np.ceil(major / minor), 4.0), 1e-6)), 0.0, len(levels) - 1.0)
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


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    wanted = [a for arg in argv for a in arg.split(',') if a] or list(VIEWS)
    C = material_constants()
    tex = {}
    for key, set_name, kind, srgb in (('GrassBC', 'GroundGrass', 'BC', True), ('GrassN', 'GroundGrass', 'N', False),
                                      ('RockBC', 'RockCliff', 'BC', True), ('RockN', 'RockCliff', 'N', False),
                                      ('ThatchBC', 'GroundThatch', 'BC', True), ('ThatchN', 'GroundThatch', 'N', False),
                                      ('StonyBC', 'GroundStony', 'BC', True), ('StonyN', 'GroundStony', 'N', False),
                                      ('Noise', 'GroundNoise', 'M', False)):
        tex[key] = mips(load(os.path.join(TEXTURES, set_name, f'T_{set_name}_{kind}.png'), srgb))
    for item in wanted:
        area, view = item.split(':', 1)
        render(C, area, view, tex)


main()
