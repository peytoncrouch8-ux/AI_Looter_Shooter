"""Each place's own ground at eye level (2026-10-22, the user: the world looks samey; the macro tones read only from above):
the layout's "macro.places" block rasterized into two maps over the core square, read straight from layout.json (no
terrain run; a couple of seconds):

  Art/Textures/<Area>Places/T_<Area>Places_M.png         M_Terrain's PlaceMap: how much of each place's ground lies
                                                           there (linear; R dust, G ash, B grit, A coal: the channels of
                                                           Tools/Blender/looter_textures.py's GroundPlaces patterns)
  Art/Textures/<Area>Places/T_<Area>PlacesScatter_M.png  the scatter's (Tools/Unreal/build_island_scatter.py): R the
                                                           share of the grass and flowers kept, G cinders, B quarry chips,
                                                           A coal lumps (each a density)

Both north up (row 0 the square's north edge, X1), columns west to east along Y, as the macro map; every 8-bit value at
least 1 (Unreal's PNG import fills the colour under a zero alpha). M_Terrain reads the first by world position
(PlaceRect: the square's X0, Y0 and side, cm; Tools/Unreal/build_world_materials.py sets it on the area's terrain
instances with the place colours).

macro.places: {"size": px across, "fray": m the edges wander, "layers": {name: {"channel": "R"|"G"|"B"|"A",
  "zones": {zone id: weight}, "features": {feature id: weight} (pads: their box, hills: their radius, pits: their
  polygon), "roads": {road id: weight or [weight, reach m]} (the road's bed and a meter each side; with reach the weight
  fades over that many meters away from the layer's zones and features: the place's ground trodden out along its roads),
  "soft": m an edge eases over, "patch": 0 even .. 1 only in patches, "grass": the share of the grass and flowers left,
  "clutter": the scatter's density of the place's clutter at full weight}}}

    python Art/Levels/area_places.py <Area>     (Blender's Python, or Tools\\artrun.ps1 -Script Art\\Levels\\area_places.py
                                                 -ScriptArgs <Area>)
"""
import json
import os
import sys

import numpy as np

LEVELS = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(LEVELS, '..', '..'))
if LEVELS not in sys.path:
    sys.path.append(LEVELS)

from area_math import Grid, catmull_rom, fbm_raster, polyline_field, signed_distance, smoothstep  # noqa: E402

CHANNELS = {'R': 0, 'G': 1, 'B': 2, 'A': 3}
# Which scatter channel carries a layer's clutter (the place map's channel -> the scatter map's): the ash's cinders,
# the grit's chips, the coal's lumps; the town's dust has none (its street is bare already).
CLUTTER = {'G': 1, 'B': 2, 'A': 3}


def to_m(points):
    return np.asarray(points, dtype=np.float64) / 100.0


def write_png(path, data):
    """uint8 (H, W, 4) as an 8-bit RGBA PNG (no colour management)."""
    import struct
    import zlib
    h, w, c = data.shape
    rows = np.concatenate([np.zeros((h, 1), np.uint8), data.reshape(h, w * c)], axis=1)

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xFFFFFFFF)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n')
        f.write(chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)))
        f.write(chunk(b'IDAT', zlib.compress(rows.tobytes(), 9)))
        f.write(chunk(b'IEND', b''))


def feature_shape(feature):
    """A feature's outline (meters, closed) when it has one: a pad's box (turned by its yaw), a hill's circle, a pit's
    polygon; None otherwise."""
    if 'polygon' in feature and isinstance(feature['polygon'], list):
        return to_m(feature['polygon'])
    cx, cy = (v / 100.0 for v in feature['center'])
    if 'size' in feature:
        sx, sy = (v / 200.0 for v in feature['size'])
        yaw = np.radians(feature.get('yaw', 0.0))
        corners = np.array([[-sx, -sy], [sx, -sy], [sx, sy], [-sx, sy]])
        c, s = np.cos(yaw), np.sin(yaw)
        return corners @ np.array([[c, s], [-s, c]]) + [cx, cy]
    if 'radius' in feature:
        a = np.linspace(0.0, 2.0 * np.pi, 48, endpoint=False)
        r = feature['radius'] / 100.0
        return np.column_stack([cx + r * np.cos(a), cy + r * np.sin(a)])
    return None


def layer_weight(grid, layout, spec, fray):
    """One layer's weight over the grid (0..1)."""
    soft = float(spec.get('soft', 6.0))
    w = np.zeros((grid.n, grid.n), np.float32)
    shapes = []
    zones = {z['id']: z for z in layout.get('zones', [])}
    features = {f['id']: f for f in layout.get('features', [])}
    for zid, weight in spec.get('zones', {}).items():
        shapes.append((catmull_rom(to_m(zones[zid]['polygon']), closed=True, step=0.5), float(weight)))
    for fid, weight in spec.get('features', {}).items():
        shape = feature_shape(features[fid])
        if shape is None:
            raise ValueError(f'macro.places: feature {fid} has no outline')
        shapes.append((shape, float(weight)))
    near = np.full((grid.n, grid.n), 1e6, np.float32)   # meters to the layer's zones and features (for the roads' reach)
    for poly, weight in shapes:
        sd = signed_distance(grid, poly, 60.0) + fray
        w = np.maximum(w, weight * (1.0 - smoothstep(-soft, soft * 0.5, sd)))
        near = np.minimum(near, np.maximum(sd, 0.0))
    roads = {r['id']: r for r in layout.get('roads', [])}
    for rid, value in spec.get('roads', {}).items():
        weight, reach = (value, None) if not isinstance(value, list) else value
        road = roads[rid]
        half = road.get('width', 400) / 200.0 + 1.0
        dist, _, _ = polyline_field(grid, catmull_rom(to_m(road['path']), step=0.5), half + 3.0)
        bed = 1.0 - smoothstep(half - 1.0, half + 1.5, dist + 0.25 * fray)
        if reach:
            bed = bed * (1.0 - smoothstep(0.0, float(reach), near))
        w = np.maximum(w, float(weight) * bed)
    patch = float(spec.get('patch', 0.0))
    if patch:
        blotch = smoothstep(-0.35, 0.35, fbm_raster(grid, 9.0, seed=907 + CHANNELS[spec['channel']], octaves=2))
        w = w * ((1.0 - patch) + patch * blotch)
    return np.clip(w, 0.0, 1.0).astype(np.float32)


def build(area):
    with open(os.path.join(LEVELS, area, 'layout.json'), encoding='utf-8') as f:
        layout = json.load(f)
    places = layout.get('macro', {}).get('places')
    if not places:
        print(f'LOOTER places: {area} has no macro.places')
        return
    half = layout['map']['half'] / 100.0
    grid = Grid(int(places.get('size', 1024)), half)
    fray = float(places.get('fray', 4.0)) * fbm_raster(grid, 14.0, seed=901, octaves=2)
    place = np.zeros((grid.n, grid.n, 4), np.float32)
    scatter = np.zeros((grid.n, grid.n, 4), np.float32)
    keep = np.ones((grid.n, grid.n), np.float32)
    for name, spec in places['layers'].items():
        if name == 'about':
            continue
        c = CHANNELS[spec['channel']]
        w = layer_weight(grid, layout, spec, fray)
        place[..., c] = np.maximum(place[..., c], w)
        keep *= 1.0 - (1.0 - float(spec.get('grass', 1.0))) * w
        if c in CLUTTER.values() and spec.get('clutter'):
            scatter[..., c] = np.maximum(scatter[..., c], float(spec['clutter']) * w)
        inside = w > 0.5
        print(f'LOOTER places: {area} {name} ({spec["channel"]}): {inside.sum() * grid.px ** 2:.0f} m2 at over half')
    scatter[..., 0] = keep
    # North up: the grid's rows run south to north (x = c[i]).
    out = os.path.join(REPO, 'Art', 'Textures', f'{area}Places')
    for data, file in ((place, f'T_{area}Places_M.png'), (scatter, f'T_{area}PlacesScatter_M.png')):
        write_png(os.path.join(out, file), np.maximum(np.round(data[::-1] * 255.0), 1).astype(np.uint8))
        print(f'LOOTER places: {os.path.join(out, file)} ({grid.n} px over {2 * half:.0f} m)')


if __name__ == '__main__':
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
    for name in argv or ['RansomsRest']:
        build(name)
