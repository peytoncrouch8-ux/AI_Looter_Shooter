"""Teropa from orbit: the maps of the main menu's planet, its moon and the galaxy's band behind them, script-made (the
user's rule: no texture libraries). Tools/Unreal/build_menu_materials.py imports them and builds M_Planet and
M_Starfield; Tools/Unreal/build_menu_teropa.py places the scene (/Game/Maps/Lvl_MenuTeropa).

Every map is equirectangular: u 0..1 runs west to east from longitude -180, v 0..1 north to south. The shader turns the
planet by sliding u (the ground and the clouds at their own speeds), so places move from the night side on the left into
the morning on the right, as Teropa turns east.

    T_Teropa_BC.png        4096 x 2048, sRGB: the ground and seas seen from orbit; alpha the sea's sun glint (1 open sea)
    T_Teropa_M.png         4096 x 2048, linear: R, G the ground's slope east and north (0.5 flat; RELIEF_SCALE), B the
                           night side's lights (towns, cities, the railroad), A 1
    T_TeropaClouds_M.png   2048 x 1024, linear grey: cloud cover
    T_TeropaMoon_BC.png    1024 x 512, sRGB: the moon's dust and its dark old lava seas; alpha 0 (no glint)
    T_TeropaMoon_M.png     1024 x 512, linear: R, G its slope (craters), B 0, A 1
    T_MenuNebula_BC.png    2048 x 1024, sRGB, dim: the galaxy's band across the sky, in the sky's own frame (its equator)

The world (Docs/Story.md): Teropa is earth-like, with one sun, one moon, seas and mountain ranges. The great continent turns
into view as the menu opens: its settled east (green farm country and forest, bright with towns at night), the mountain
divide, then its western frontier, the Reaches (rust mesas, river gorges, farm valleys, swamp country in the south), and
the Rim, the long escarpment at the Reaches' western edge, past which pale, empty plains run to the sunset. At night the
lights thin westward through the Reaches, and the railroad's string of stations stops at the Rim: nothing past it. Two
smaller continents, an island continent and northern isles, polar ice, and pale seas (the frontier's dust in the air).

Run it with Blender's Python (numpy):
    Tools\\artrun.ps1 -Script Art\\Textures\\MenuTeropa\\make_teropa_maps.py
or any Python with numpy (Blender's own: "C:\\Program Files\\Blender Foundation\\Blender 4.4\\4.4\\python\\bin\\python.exe"):
    python Art/Textures/MenuTeropa/make_teropa_maps.py [--quick]
--quick makes quarter-size maps into Saved/ArtPreviews/MenuTeropa/quick/ (for looking only; never import those). Every
run writes Saved/ArtPreviews/MenuTeropa/Teropa_preview.png: the planet lit as the menu shows it (night side left, the
sun's side right, at the start of its turn), the moon, and the maps small. Reproducible: fixed seeds, numpy only. It
takes about eight minutes at full size (20 s quick). Then, in the editor:
    Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_menu_materials.py"
"""
import math
import os
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_textures as lt  # noqa: E402 - needs the path above (write_png; no Blender needed)

TAU = 2.0 * math.pi
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
QUICK = '--quick' in ARGS
SHRINK = 4 if QUICK else 1
OUT_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews', 'MenuTeropa', 'quick') if QUICK else HERE
PREVIEW = os.path.join(REPO, 'Saved', 'ArtPreviews', 'MenuTeropa', 'Teropa_preview.png')

# (rows, columns). The planet fills most of the menu's height (about 1000 px at 1080p), so the visible half of the
# ground map (2048 px across) is about one texel per pixel at the disc's middle.
SURFACE_SIZE = (2048 // SHRINK, 4096 // SHRINK)
CLOUD_SIZE = (1024 // SHRINK, 2048 // SHRINK)
MOON_SIZE = (512 // SHRINK, 1024 // SHRINK)
NEBULA_SIZE = (1024 // SHRINK, 2048 // SHRINK)
# Rows worked at once: keeps numpy's temporaries near 150 MB beside a running editor.
CHUNK_ROWS = 128

# Slope (height units per radian of arc) that encodes as fully tilted (0 or 1 in R, G). The shader multiplies the
# decoded slope by its ReliefStrength, so this only spends the 8 bits well: the big slopes (the Rim, the divide's peaks)
# just reach the ends.
RELIEF_SCALE = 1.0 / 14.0

# The landmasses: (longitude, latitude, angular radius, deg; strength). Noise shapes their coasts; these say where land
# gathers, so the great continent is always the big one, centered where the menu's first view looks.
CONTINENTS = (
    (6.0, 24.0, 50.0, 0.62),      # the great continent: the Reaches in its west, the settled country in its east
    (-6.0, -12.0, 20.0, 0.30),    # its southern arm, past the swamp country
    (150.0, -16.0, 34.0, 0.55),   # the southern continent across the sea
    (-120.0, 36.0, 20.0, 0.40),   # the island continent of the far west
    (96.0, 54.0, 15.0, 0.36),     # the northern isles
)
GREAT = CONTINENTS[0]
# The Rim: the escarpment's mean longitude and the latitudes it runs between (deg). West of it the plains; east of it the
# Reaches, up to the divide's range at about DIVIDE_LON, then the settled east.
RIM_LON = -24.0
RIM_LATS = (6.0, 52.0)
DIVIDE_LON = 15.0
# The railroad, east to west (lon, lat), from the settled coast over the divide to the Rim, where it stops.
RAILROAD = ((46.0, 29.0), (36.0, 31.0), (26.0, 30.0), (17.0, 32.5), (8.0, 31.0), (-2.0, 33.0), (-12.0, 32.0),
            (-20.0, 32.5))
# Seeds per field (never reuse one for two fields: they would line up).
SEED = dict(warp=11, land=20, mount=40, belt=45, moist=60, iron=70, river=80, detail=90, canyon=95, field=99,
            cloud=200, streak=210, cwarp=220, moon=300, mare=310, crater=320, nebula=400, lanes=410, glow=420,
            towns=500)


# --- Noise (3D gradient noise on points of the unit sphere: seamless, no pinch at the poles) ---

GRADIENTS = np.array([[1, 1, 0], [-1, 1, 0], [1, -1, 0], [-1, -1, 0], [1, 0, 1], [-1, 0, 1], [1, 0, -1], [-1, 0, -1],
                      [0, 1, 1], [0, -1, 1], [0, 1, -1], [0, -1, -1], [1, 1, 0], [-1, 1, 0], [0, -1, 1], [0, -1, -1]],
                     np.float32)
GX, GY, GZ = GRADIENTS[:, 0].copy(), GRADIENTS[:, 1].copy(), GRADIENTS[:, 2].copy()
U32 = np.uint32


def _rotation(axis, angle):
    a = np.array(axis, np.float64)
    a /= np.linalg.norm(a)
    k = np.array([[0, -a[2], a[1]], [a[2], 0, -a[0]], [-a[1], a[0], 0]])
    return (np.eye(3) + math.sin(angle) * k + (1 - math.cos(angle)) * k @ k).astype(np.float32)


# Each octave is turned before it's scaled, so the lattices of the octaves never line up (no grid-aligned streaks).
OCTAVE_TURN = _rotation((0.36, 0.48, 0.8), 0.93)


def _hash(ix, iy, iz, seed):
    """32-bit integer hash of lattice points (uint32 arrays wrap silently)."""
    s = U32((seed * 0x165667B1 + 0x9E3779B9) & 0xFFFFFFFF)
    h = (ix * U32(0x8DA6B343)) ^ (iy * U32(0xD8163841)) ^ (iz * U32(0xCB1AB31F)) ^ s
    h ^= h >> U32(15)
    h *= U32(0x2C1B3C6D)
    h ^= h >> U32(12)
    h *= U32(0x297A2D39)
    h ^= h >> U32(15)
    return h & U32(15)


def perlin(x, y, z, seed):
    """Gradient noise, about -0.9..0.9, mean 0."""
    fx, fy, fz = np.floor(x), np.floor(y), np.floor(z)
    tx, ty, tz = (x - fx).astype(np.float32), (y - fy).astype(np.float32), (z - fz).astype(np.float32)
    ix, iy, iz = fx.astype(np.int64), fy.astype(np.int64), fz.astype(np.int64)
    u = tx * tx * tx * (tx * (tx * 6 - 15) + 10)
    v = ty * ty * ty * (ty * (ty * 6 - 15) + 10)
    w = tz * tz * tz * (tz * (tz * 6 - 15) + 10)
    xs = (ix.astype(U32), (ix + 1).astype(U32))
    ys = (iy.astype(U32), (iy + 1).astype(U32))
    zs = (iz.astype(U32), (iz + 1).astype(U32))

    def corner(dx, dy, dz):
        g = _hash(xs[dx], ys[dy], zs[dz], seed)
        return GX[g] * (tx - dx) + GY[g] * (ty - dy) + GZ[g] * (tz - dz)
    x00 = corner(0, 0, 0) + u * (corner(1, 0, 0) - corner(0, 0, 0))
    x10 = corner(0, 1, 0) + u * (corner(1, 1, 0) - corner(0, 1, 0))
    x01 = corner(0, 0, 1) + u * (corner(1, 0, 1) - corner(0, 0, 1))
    x11 = corner(0, 1, 1) + u * (corner(1, 1, 1) - corner(0, 1, 1))
    y0 = x00 + v * (x10 - x00)
    y1 = x01 + v * (x11 - x01)
    return y0 + w * (y1 - y0)


def _turn(x, y, z, scale):
    r = OCTAVE_TURN
    return ((r[0, 0] * x + r[0, 1] * y + r[0, 2] * z) * scale, (r[1, 0] * x + r[1, 1] * y + r[1, 2] * z) * scale,
            (r[2, 0] * x + r[2, 1] * y + r[2, 2] * z) * scale)


def fbm(x, y, z, seed, octaves, freq=1.0, gain=0.5, lacunarity=2.03):
    """Fractal gradient noise: deviation about 0.25 for 4+ octaves, mean 0."""
    total = np.zeros(np.shape(x), np.float32)
    amp, norm = 1.0, 0.0
    px, py, pz = x * freq, y * freq, z * freq
    for octave in range(octaves):
        total += amp * perlin(px, py, pz, seed * 31 + octave)
        norm += amp
        amp *= gain
        px, py, pz = _turn(px, py, pz, lacunarity)
    return total / norm


def ridged(x, y, z, seed, octaves, freq=1.0, gain=0.5, lacunarity=2.03):
    """Ridged noise 0..1: sharp crests where the noise crosses zero (mountain ranges, gorges, dust lanes)."""
    total = np.zeros(np.shape(x), np.float32)
    amp, norm = 1.0, 0.0
    px, py, pz = x * freq, y * freq, z * freq
    for octave in range(octaves):
        r = 1.0 - np.abs(perlin(px, py, pz, seed * 31 + octave)) * 1.4
        total += amp * np.clip(r, 0.0, 1.0) ** 2
        norm += amp
        amp *= gain
        px, py, pz = _turn(px, py, pz, lacunarity)
    return total / norm


def smooth(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def mix(a, b, t):
    """Blends a to b by t; a colour (or colour image) blended by a map of t takes t per pixel."""
    t = np.asarray(t, np.float32)
    colored = np.shape(a)[-1:] == (3,) or np.shape(b)[-1:] == (3,)
    if t.ndim and colored and (np.ndim(a) == 1 or np.ndim(b) == 1 or max(np.ndim(a), np.ndim(b)) == t.ndim + 1):
        t = t[..., None]
    return a + (b - a) * t


def srgb(value):
    return np.array([(value >> 16) & 255, (value >> 8) & 255, value & 255], np.float32) / 255.0


def direction(lon_deg, lat_deg):
    lon, lat = np.radians(lon_deg), np.radians(lat_deg)
    return np.stack([np.cos(lat) * np.cos(lon), np.cos(lat) * np.sin(lon), np.sin(lat)], axis=-1).astype(np.float32)


def angle_to(x, y, z, lon_deg, lat_deg):
    """Arc (deg) from each point to (lon, lat)."""
    c = direction(lon_deg, lat_deg)
    return np.degrees(np.arccos(np.clip(x * c[0] + y * c[1] + z * c[2], -1.0, 1.0)))


def rows_grid(shape, r0, r1):
    """Unit directions and lon/lat (deg) of rows r0..r1 of an equirectangular map."""
    h, w = shape
    lat = (0.5 - (np.arange(r0, r1, dtype=np.float64) + 0.5) / h) * 180.0
    lon = ((np.arange(w, dtype=np.float64) + 0.5) / w - 0.5) * 360.0
    lon, lat = np.meshgrid(lon, lat)
    d = direction(lon, lat)
    return d[..., 0], d[..., 1], d[..., 2], lon.astype(np.float32), lat.astype(np.float32)


def to_bytes(a):
    return (np.clip(a, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)


def log(message):
    print(f'[teropa] {message}', flush=True)


# --- The ground ---

def rim_longitude(lat):
    """Where the Rim runs at each latitude (deg): a long, wandering escarpment, never a ruled line."""
    wander = perlin(lat / 4.0, np.full_like(lat, 0.37), np.full_like(lat, 2.1), SEED['canyon']) \
        + 0.5 * perlin(lat / 1.5, np.full_like(lat, 3.7), np.full_like(lat, 0.6), SEED['canyon'] + 1)
    return RIM_LON + 3.0 * np.sin(np.radians(lat * 4.0)) + 1.6 * np.sin(np.radians(lat * 9.0 + 40.0)) + 4.0 * wander


def surface_fields(x, y, z, lon, lat):
    """Everything about the ground at these points: elevation (land above 0), mountains, moisture, regions."""
    # Coasts: warped noise, gathered round the continents' centers.
    wx = fbm(x + 5.2, y, z, SEED['warp'], 3, freq=1.3)
    wy = fbm(x, y + 9.7, z, SEED['warp'] + 1, 3, freq=1.3)
    wz = fbm(x, y, z + 3.1, SEED['warp'] + 2, 3, freq=1.3)
    qx, qy, qz = x + 0.45 * wx, y + 0.45 * wy, z + 0.45 * wz
    elev = 1.3 * fbm(qx, qy, qz, SEED['land'], 8, freq=2.0) - 0.30
    for c_lon, c_lat, radius, strength in CONTINENTS:
        elev += strength * (1.0 - smooth(0.4 * radius, 1.25 * radius, angle_to(x, y, z, c_lon, c_lat)))
    great = 1.0 - smooth(0.8 * GREAT[2], 1.3 * GREAT[2], angle_to(x, y, z, GREAT[0], GREAT[1]))

    # The Rim, the Reaches east of it, the plains west of it (on the great continent only). The band's north and south
    # ends and the plains' far side wander with noise, so no edge of a region runs along a parallel or a meridian.
    edge = 5.0 * fbm(x, y, z, SEED['canyon'] + 3, 3, freq=4.0)
    rim_band = smooth(RIM_LATS[0] - 4, RIM_LATS[0] + 4, lat + edge) \
        * (1.0 - smooth(RIM_LATS[1] - 6, RIM_LATS[1] + 6, lat + edge)) * great
    rim_d = lon - rim_longitude(lat)
    east = smooth(-0.3, 0.3, rim_d)
    reaches = rim_band * east * (1.0 - smooth(DIVIDE_LON - 3, DIVIDE_LON + 3, lon + edge))
    plains = rim_band * (1.0 - east) * (1.0 - smooth(18.0, 30.0, -rim_d + edge * 2.0))
    settled = great * smooth(DIVIDE_LON + 1, DIVIDE_LON + 7, lon + edge)

    # Mountains: ranges where a slow belt noise allows them, and the divide between the Reaches and the settled east
    # (broken into ridges and saddles by the ridged noise itself, never one wall).
    belt = smooth(0.06, 0.3, fbm(x, y, z, SEED['belt'], 2, freq=1.1))
    divide_lon = DIVIDE_LON + 2.5 * np.sin(np.radians(lat * 6.0)) + 1.5 * edge
    divide = np.exp(-((lon - divide_lon) / 3.0) ** 2) * rim_band * smooth(-0.1, 0.25, fbm(x, y, z, SEED['belt'] + 1, 3, freq=6.0))
    land = smooth(-0.01, 0.04, elev)
    mount = ridged(x, y, z, SEED['mount'], 6, freq=3.2) * np.maximum(belt * 0.85, divide * 0.9) * land
    mount *= 1.0 - 0.7 * plains

    # Moisture: the wet equator, the dry belts near 24 degrees, the wetter middle latitudes; the coast wetter; the
    # settled east green, the Reaches drier, the plains past the Rim driest.
    alat = np.abs(lat)
    climate = 0.42 * np.exp(-(lat / 12.0) ** 2) - 0.36 * np.exp(-((alat - 24.0) / 9.0) ** 2) \
        + 0.22 * np.exp(-((alat - 50.0) / 12.0) ** 2)
    moist = 0.8 * fbm(x, y, z, SEED['moist'], 5, freq=2.0) + climate
    moist += 0.18 * (1.0 - smooth(0.0, 0.12, elev)) + 0.28 * settled - 0.10 * reaches - 0.45 * plains
    return dict(elev=elev, land=land, mount=mount, moist=moist, reaches=reaches, plains=plains, settled=settled,
                great=great, rim_d=rim_d, rim_band=rim_band, east=east)


PALETTE = dict(
    deep=srgb(0x5C9098), shelf=srgb(0x9DC7BD), coast=srgb(0xB5D6C8),
    desert=srgb(0xD8BB86), ochre=srgb(0xC39A5C), rust=srgb(0xA65A33), rust_dark=srgb(0x7C3D24),
    olive=srgb(0x7F7C46), field_gold=srgb(0xB59E5E), forest=srgb(0x4F5A32), swamp=srgb(0x56603A),
    straw=srgb(0xCDB27C), river=srgb(0x6A7140), rock=srgb(0x88735F), rock_dark=srgb(0x5C4D44),
    tundra=srgb(0x8F8A7B), snow=srgb(0xECE8E0), ice=srgb(0xE6ECEF),
)


def surface_rows(r0, r1):
    """Colour (sRGB), glint, height and the fields the lights need, for rows r0..r1."""
    x, y, z, lon, lat = rows_grid(SURFACE_SIZE, r0, r1)
    f = surface_fields(x, y, z, lon, lat)
    p = PALETTE
    elev, land, mount, moist = f['elev'], f['land'], f['mount'], f['moist']
    alat = np.abs(lat)
    detail = fbm(x, y, z, SEED['detail'], 4, freq=24.0)
    iron = fbm(x, y, z, SEED['iron'], 4, freq=5.0)

    # Land by moisture: desert, ochre with rust where the iron noise runs high, olive farm country, forest.
    dry = mix(p['desert'], p['ochre'], smooth(-0.45, -0.2, moist))
    semi = mix(p['ochre'], p['rust'], smooth(0.0, 0.25, iron))
    color = mix(dry, semi, smooth(-0.25, -0.05, moist))
    color = mix(color, mix(p['olive'], p['field_gold'], smooth(-0.1, 0.25, detail)), smooth(0.05, 0.2, moist))
    color = mix(color, p['forest'], smooth(0.3, 0.5, moist))

    # The Reaches: rust mesas (darker where the iron noise runs high), green threads of river valleys, the swamp
    # country in the south, gorges dark just east of the Rim.
    mesa = mix(p['ochre'], mix(p['rust'], p['rust_dark'], smooth(0.15, 0.4, iron)), smooth(-0.15, 0.1, iron))
    color = mix(color, mesa, f['reaches'] * 0.85)
    river = 1.0 - smooth(0.0, 0.03, np.abs(fbm(x, y, z, SEED['river'], 4, freq=6.0)))
    color = mix(color, p['river'], river * np.maximum(f['reaches'], f['settled'] * 0.5) * 0.8)
    swamp = f['reaches'] * (1.0 - smooth(14.0, 20.0, lat)) * smooth(0.0, 0.2, moist + 0.15)
    color = mix(color, p['swamp'], swamp * 0.8)
    gorge = f['rim_band'] * smooth(0.0, 0.4, f['rim_d']) * (1.0 - smooth(1.5, 4.0, f['rim_d']))
    canyon = gorge * smooth(0.55, 0.85, ridged(x, y, z, SEED['canyon'], 3, freq=14.0))
    color = mix(color, p['rust_dark'] * 0.7, canyon)
    # The plains past the Rim: pale straw, combed east-west by the wind.
    comb = fbm(x * 0.4, y * 0.4, z * 6.0, SEED['field'], 3, freq=6.0)
    color = mix(color, mix(p['straw'], p['ochre'], smooth(0.1, 0.4, comb) * 0.4), f['plains'])
    # The settled east: a patchwork of fields, gold and green.
    patch = fbm(x, y, z, SEED['field'] + 1, 2, freq=40.0)
    color = mix(color, mix(p['olive'], p['field_gold'], smooth(-0.1, 0.1, patch)), f['settled'] * smooth(-0.05, 0.2, moist) * 0.5)

    # Mountains, snow on the peaks and toward the poles, tundra and ice in the cold.
    rock = mix(p['rock'], p['rock_dark'], smooth(0.3, 0.7, mount + 0.2 * detail))
    color = mix(color, rock, smooth(0.15, 0.45, mount))
    snow_line = 0.74 - 0.5 * smooth(30.0, 65.0, alat)
    color = mix(color, p['snow'], smooth(snow_line, snow_line + 0.1, mount + 0.05 * detail))
    color = mix(color, p['tundra'], smooth(56.0, 64.0, alat + 4.0 * detail) * 0.8)
    color = mix(color, p['ice'], smooth(66.0, 70.0, alat + 6.0 * detail))
    color *= (0.94 + 0.12 * detail)[..., None]

    # The seas: pale over the shelves, deeper teal out at sea; sea ice near the poles.
    depth = smooth(0.0, 0.08, -elev)
    sea = mix(mix(p['coast'], p['shelf'], smooth(0.0, 0.015, -elev)), p['deep'], depth)
    sea *= (0.97 + 0.06 * fbm(x, y, z, SEED['detail'] + 5, 3, freq=9.0))[..., None]
    sea_ice = smooth(73.0, 77.0, alat + 5.0 * detail)
    sea = mix(sea, p['ice'], sea_ice)
    color = mix(sea, color, land)
    glint = (1.0 - land) * (1.0 - sea_ice)

    # Height for the relief: land rising inland, the ranges, the Reaches' plateau standing above the plains at the Rim,
    # the gorges cut into it; the sea flat (its glint is its own).
    height = np.maximum(elev, 0.0) * 0.5 + mount * 0.9 + 0.12 * f['east'] * f['rim_band'] - 0.08 * canyon
    height = (height + 0.025 * detail) * land
    return color, glint, height


def relief(height):
    """R, G: the height's slope east and north per radian of arc, encoded 0.5 +- RELIEF_SCALE x slope."""
    h, w = height.shape
    lat = np.radians((0.5 - (np.arange(h) + 0.5) / h) * 180.0)
    d_lon, d_lat = TAU / w, math.pi / h
    east = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / (2.0 * d_lon * np.maximum(np.cos(lat), 0.05)[:, None])
    up = np.vstack([height[:1], height[:-1]])
    down = np.vstack([height[1:], height[-1:]])
    north = (up - down) / (2.0 * d_lat)
    log(f'slopes: 99th percentile {np.percentile(np.abs(east), 99):.2f} east, {np.percentile(np.abs(north), 99):.2f} north '
        f'(encoded full at {1.0 / RELIEF_SCALE:.0f})')
    return 0.5 + 0.5 * np.clip(east * RELIEF_SCALE, -1, 1), 0.5 + 0.5 * np.clip(north * RELIEF_SCALE, -1, 1)


# --- The night side's lights ---

def splat(target, lon, lat, sigma_deg, amount):
    """Adds a soft round light (sigma in degrees of arc) to an equirectangular map, wrapping east-west."""
    h, w = target.shape
    px = (lon / 360.0 + 0.5) * w
    py = (0.5 - lat / 180.0) * h
    sy = max(sigma_deg / 180.0 * h, 0.45)
    sx = max(sigma_deg / 360.0 * w / max(math.cos(math.radians(lat)), 0.08), 0.45)
    rx, ry = int(math.ceil(sx * 3)), int(math.ceil(sy * 3))
    cols = np.arange(int(px) - rx, int(px) + rx + 1)
    rows = np.arange(max(int(py) - ry, 0), min(int(py) + ry + 1, h))
    if rows.size == 0:
        return
    gx = np.exp(-0.5 * ((cols + 0.5 - px) / sx) ** 2)
    gy = np.exp(-0.5 * ((rows + 0.5 - py) / sy) ** 2)
    target[np.ix_(rows, cols % w)] += (amount * gy[:, None] * gx[None, :]).astype(np.float32)


def place_lights(shape):
    """Towns scattered by a density (the settled east dense, coasts denser, the Reaches sparse, nothing past the Rim),
    cities as clusters, the railroad's stations and its faint line from the coast to the Rim."""
    rng = np.random.default_rng(SEED['towns'])
    lights = np.zeros(shape, np.float32)
    # Candidates spread evenly over the sphere, then kept by the density.
    n = 260000 // (SHRINK * 2 if QUICK else 1)
    z = rng.uniform(-1, 1, n).astype(np.float32)
    lon = rng.uniform(-180, 180, n).astype(np.float32)
    lat = np.degrees(np.arcsin(z)).astype(np.float32)
    d = direction(lon, lat)
    f = surface_fields(d[:, 0], d[:, 1], d[:, 2], lon, lat)
    alat = np.abs(lat)
    density = np.where(f['elev'] > 0.01, 1.0, 0.0) * (1.0 - smooth(55.0, 64.0, alat))
    region = 0.10 + 0.9 * f['settled'] + 0.16 * f['reaches'] - 0.10 * f['plains']
    region = np.where(f['plains'] > 0.5, 0.0, region)  # nobody settles the land the dead cross
    coast = 1.0 + 1.6 * (1.0 - smooth(0.0, 0.08, f['elev']))
    wet = 0.35 + 0.65 * smooth(-0.35, 0.05, f['moist'])
    density *= np.clip(region, 0.0, 1.0) * coast * wet * (1.0 - 0.8 * smooth(0.3, 0.6, f['mount']))
    keep = rng.uniform(0, 1, n) < density * 0.11
    sizes = rng.uniform(0, 1, n)
    for lo, la, s in zip(lon[keep], lat[keep], sizes[keep]):
        splat(lights, float(lo), float(la), 0.05 + 0.06 * s * s, 0.35 + 0.9 * s ** 3)
    log(f'{int(keep.sum())} towns')
    # Cities: the brightest towns of the settled country and a few on the other continents, each a cluster.
    weight = density * (0.2 + f['settled'])
    order = np.argsort(-weight * rng.uniform(0.6, 1.0, n))
    cities = 0
    for i in order[:400]:
        if cities >= 70:
            break
        if weight[i] < 0.05:
            continue
        cities += 1
        size = rng.uniform(0.5, 1.0)
        splat(lights, float(lon[i]), float(lat[i]), 0.16 * size, 2.2 * size)
        for _ in range(int(6 + 10 * size)):
            a, r = rng.uniform(0, TAU), rng.uniform(0.05, 0.6) * size
            splat(lights, float(lon[i] + r * math.cos(a) / max(math.cos(math.radians(lat[i])), 0.2)),
                  float(lat[i] + r * math.sin(a)), 0.05, rng.uniform(0.3, 1.1))
    log(f'{cities} cities')
    # The railroad: a faint line of lights and a station every few degrees, ending at the Rim.
    stations = 0
    for (lon0, lat0), (lon1, lat1) in zip(RAILROAD[:-1], RAILROAD[1:]):
        steps = int(max(abs(lon1 - lon0), abs(lat1 - lat0)) / 0.08)
        for k in range(steps):
            t = k / steps
            lo = lon0 + (lon1 - lon0) * t + 0.4 * math.sin(t * 7.0 + lon0)
            la = lat0 + (lat1 - lat0) * t + 0.3 * math.sin(t * 5.0 + lat0)
            splat(lights, lo, la, 0.035, 0.08)
            if k % 22 == 0:
                stations += 1
                splat(lights, lo, la, 0.07, 0.6)
    # The end of the line, at the Rim's foot of the Reaches: one last lit town.
    end_lon, end_lat = RAILROAD[-1]
    splat(lights, end_lon, end_lat, 0.12, 1.6)
    log(f'railroad: {stations} stations, ends at {end_lon:.0f}, {end_lat:.0f} (the Rim near {RIM_LON:.0f})')
    return 1.0 - np.exp(-lights * 1.4)


# --- Clouds ---

STORMS_N = 26


def clouds():
    """Cloud cover: bands by latitude (the wet equator's line of towers, clear dry belts near 25 degrees, storm tracks
    with swirling lows in the middle latitudes, the polar caps), streaked east-west by the winds."""
    rng = np.random.default_rng(SEED['cloud'])
    storms = []
    for _ in range(STORMS_N):
        hemi = 1.0 if rng.uniform() < 0.5 else -1.0
        storms.append((rng.uniform(-180, 180), hemi * rng.uniform(32, 64), rng.uniform(9, 18), hemi * rng.uniform(1.3, 2.4)))
    out = np.zeros(CLOUD_SIZE, np.float32)
    for r0 in range(0, CLOUD_SIZE[0], CHUNK_ROWS):
        r1 = min(r0 + CHUNK_ROWS, CLOUD_SIZE[0])
        x0, y0, z0, lon, lat = rows_grid(CLOUD_SIZE, r0, r1)
        x, y, z = x0, y0, z0
        # Swirls: each low turns the points near it about its centre (the spiral arms), counter-clockwise in the north.
        for c_lon, c_lat, radius, turn in storms:
            c = direction(c_lon, c_lat)
            ang = angle_to(x0, y0, z0, c_lon, c_lat)
            theta = turn * np.clip(1.0 - ang / radius, 0.0, 1.0) ** 1.5
            cos_t, sin_t = np.cos(theta), np.sin(theta)
            cx = c[1] * z - c[2] * y
            cy = c[2] * x - c[0] * z
            cz = c[0] * y - c[1] * x
            dot = (c[0] * x + c[1] * y + c[2] * z) * (1.0 - cos_t)
            x, y, z = x * cos_t + cx * sin_t + c[0] * dot, y * cos_t + cy * sin_t + c[1] * dot, z * cos_t + cz * sin_t + c[2] * dot
        # East-west streaks: noise on a cylinder, squeezed north-south.
        w_lon = np.arctan2(y, x)
        w_lat = np.arcsin(np.clip(z, -1, 1))
        cx, cy = np.cos(w_lon) * 1.8, np.sin(w_lon) * 1.8
        warp = fbm(x, y, z, SEED['cwarp'], 3, freq=2.0)
        n = fbm(cx, cy, w_lat * 5.0 + warp * 0.8, SEED['cloud'], 6, freq=1.4)
        n += 0.35 * fbm(cx * 2.0, cy * 2.0, w_lat * 22.0, SEED['streak'], 4, freq=2.0)
        alat = np.abs(lat)
        cover = 0.19 + 0.42 * np.exp(-((lat - 5.0) / 6.0) ** 2) + 0.26 * np.exp(-((alat - 50.0) / 11.0) ** 2) \
            + 0.22 * np.exp(-((alat - 76.0) / 8.0) ** 2) - 0.16 * np.exp(-((alat - 25.0) / 8.0) ** 2)
        # Each low is thick round its eye (which stays clear).
        swirl = np.zeros_like(x)
        for c_lon, c_lat, radius, _ in storms:
            ang = angle_to(x0, y0, z0, c_lon, c_lat)
            swirl = np.maximum(swirl, (1.0 - smooth(0.2 * radius, radius, ang)) * smooth(0.04 * radius, 0.12 * radius, ang))
        # Soft edges (0.42 wide): thin veils round the thick cores, as from orbit; fine puffs break up the cores.
        density = smooth(0.62 - cover, 1.04 - cover, n + 0.5 + 0.2 * swirl)
        density *= np.clip(0.82 + 0.6 * fbm(x, y, z, SEED['streak'] + 5, 3, freq=28.0), 0.0, 1.0)
        out[r0:r1] = density
    log(f'clouds: {out.mean():.0%} cover')
    return out


# --- The moon ---

def moon():
    """Grey dust, dark seas of old lava, craters (a bowl, a raised rim), a few young ones with bright rays."""
    shape = MOON_SIZE
    h, w = shape
    x, y, z, lon, lat = rows_grid(shape, 0, h)
    base = fbm(x, y, z, SEED['moon'], 6, freq=4.0)
    mare = smooth(0.04, 0.16, fbm(x, y, z, SEED['mare'], 3, freq=1.5))
    height = 0.04 * base - 0.05 * mare
    bright = np.zeros(shape, np.float32)
    rng = np.random.default_rng(SEED['crater'])
    craters = 0
    for _ in range(700):
        radius = 0.6 * (1.0 - rng.uniform()) ** -0.55  # many small, a few big
        if radius > 14.0:
            continue
        c_lon, c_lat = rng.uniform(-180, 180), math.degrees(math.asin(rng.uniform(-1, 1)))
        py = int((0.5 - c_lat / 180.0) * h)
        span_rows = int(radius * 2.2 / 180.0 * h) + 2
        rows = np.arange(max(py - span_rows, 0), min(py + span_rows + 1, h))
        if rows.size == 0:
            continue
        ang = angle_to(x[rows], y[rows], z[rows], c_lon, c_lat)
        s = ang / radius
        near = s < 2.2
        if not near.any():
            continue
        depth = 0.18 * radius ** 0.6
        bowl = np.where(s < 1.0, -(1.0 - s * s), 0.0)
        rim = 0.35 * np.exp(-((s - 1.0) / 0.2) ** 2)
        height[rows] += (depth * (bowl + rim) * near).astype(np.float32) * 0.04
        young = rng.uniform() < 0.05 and radius > 2.0
        fresh = 0.25 * np.exp(-((s - 1.0) / 0.3) ** 2) + (0.5 if young else 0.1) * np.exp(-s * 1.5)
        if young:
            bearing = np.arctan2(lat[rows] - c_lat, (lon[rows] - c_lon) * math.cos(math.radians(c_lat)))
            rays = smooth(0.55, 0.9, np.sin(bearing * 9.0 + rng.uniform(0, 6)) * np.sin(bearing * 5.0 + 1.0))
            fresh = fresh + rays * np.exp(-s / 3.5) * 0.5 * (s > 1.0)
        bright[rows] += (fresh * near).astype(np.float32)
        craters += 1
    color = mix(srgb(0x9C978F), srgb(0x5F5B57), mare * 0.85)
    color *= (0.88 + 0.24 * base)[..., None]
    color = mix(color, srgb(0xD9D4CC), np.clip(bright, 0, 1) * 0.55)
    log(f'moon: {craters} craters')
    return color, height


# --- The sky ---

def nebula():
    """The galaxy's band along the sky's equator: a soft glow with dark dust lanes down its middle, warm toward its
    core, cool blue-violet along its arms, a few faint red-violet clouds. Dim: the shader scales it."""
    out = np.zeros(NEBULA_SIZE + (3,), np.float32)
    for r0 in range(0, NEBULA_SIZE[0], CHUNK_ROWS):
        r1 = min(r0 + CHUNK_ROWS, NEBULA_SIZE[0])
        x, y, z, lon, lat = rows_grid(NEBULA_SIZE, r0, r1)
        wobble = 3.0 * fbm(x, y, z, SEED['glow'], 3, freq=1.5)
        band = np.exp(-((lat + wobble) / 13.0) ** 2)
        glow = band * (0.55 + 0.9 * fbm(x, y, z, SEED['nebula'], 6, freq=3.0))
        lanes = smooth(0.5, 0.85, ridged(x, y, z, SEED['lanes'], 5, freq=4.0)) * np.exp(-((lat + wobble) / 4.5) ** 2)
        glow *= 1.0 - 0.85 * lanes
        core = np.exp(-angle_to(x, y, z, 0.0, 0.0) / 28.0)
        color = mix(srgb(0x5664A6), srgb(0xD1A57C), core)
        clouds_mask = smooth(0.18, 0.4, fbm(x, y, z, SEED['nebula'] + 7, 4, freq=2.5)) * band
        color = mix(color, srgb(0xA35A86), clouds_mask * 0.6)
        out[r0:r1] = color * np.clip(glow * (0.35 + 0.65 * core), 0, None)[..., None] * 0.55 + 0.004
    return out


# --- Preview ---

def preview(surface, glint, relief_rg, lights, cloud, moon_color, nebula_color):
    """The planet as the menu shows it (night side left, sun's side right), lit as M_Planet lights it (roughly),
    with the maps small below. Nearest-texel lookups: for judging the look, not filtering."""
    pw, ph = 1280, 720
    img = np.zeros((ph, pw, 3), np.float32)
    img += 0.01
    yy, xx = np.mgrid[0:ph, 0:pw].astype(np.float32)
    cx, cy, radius = pw * 0.70, ph * 0.52, ph * 0.47
    nx, ny = (xx - cx) / radius, -(yy - cy) / radius
    inside = nx * nx + ny * ny < 1.0
    nz = np.sqrt(np.clip(1.0 - nx * nx - ny * ny, 0, 1))
    # The planet frame: north tilted 18 degrees toward the right, the camera-facing meridian at the start offset.
    tilt = math.radians(18.0)
    up = np.array([math.sin(tilt), math.cos(tilt), 0.0], np.float32)
    rt = np.array([math.cos(tilt), -math.sin(tilt), 0.0], np.float32)
    fr = np.array([0.0, 0.0, 1.0], np.float32)
    lonp = np.arctan2(nx * rt[0] + ny * rt[1], nz)
    latp = np.arcsin(np.clip(nx * up[0] + ny * up[1], -1, 1))
    start = math.radians(-10.0)  # the menu's first view: the great continent a little right of the middle

    def look(tex, lon_off):
        th, tw = tex.shape[:2]
        u = ((lonp - lon_off) / TAU + 0.5) % 1.0
        v = np.clip(0.5 - latp / math.pi, 0, 0.9999)
        return tex[(v * th).astype(int), (u * tw).astype(int)]
    alb = look(surface, start)
    gl = look(glint, start)
    lit = look(lights, start)
    cl = look(cloud, start + math.radians(8.0))
    sun = np.array([0.82, 0.36, -0.22], np.float32)  # from the right, a little up, a little behind the planet
    sun /= np.linalg.norm(sun)
    ndl = nx * sun[0] + ny * sun[1] + nz * sun[2]
    day = smooth(-0.06, 0.12, ndl) * np.clip(ndl, 0, 1)
    warm = np.exp(-(ndl / 0.18) ** 2)
    sun_col = np.array([1.25, 1.18, 1.05], np.float32)
    lin = alb ** 2.2
    col = lin * (sun_col * day[..., None]) * mix(np.ones(3, np.float32), np.array([1.25, 0.8, 0.55]), warm * 0.5)
    col += lin * 0.012
    # The sea's glint: the view ray (toward the eye, +z) mirrored about the normal, against the sun.
    spec = np.clip(2 * nz * ndl - sun[2], 0, 1) ** 60 * gl * 0.8 * day
    col += spec[..., None] * sun_col
    cloud_col = np.ones(3, np.float32) * 0.85 * (day * 0.95 + 0.012)[..., None]
    col = mix(col, cloud_col, cl * 0.92)
    night = smooth(0.05, -0.15, ndl)
    col += (lit * night * (1.0 - 0.75 * cl))[..., None] * np.array([1.0, 0.68, 0.34]) * 1.4
    rim = (1.0 - nz) ** 3
    atmo = np.array([0.35, 0.62, 1.0], np.float32)
    col += (rim * smooth(-0.3, 0.4, ndl))[..., None] * atmo * 0.9
    img = np.where(inside[..., None], col, img)
    d = np.sqrt(nx * nx + ny * ny)
    halo = np.exp(-np.clip(d - 1.0, 0, None) / 0.012) * (d >= 1.0) * smooth(-0.3, 0.4, nx * sun[0] + ny * sun[1])
    img += halo[..., None] * atmo * 0.8
    out = to_bytes(np.clip(img, 0, 1) ** (1 / 2.2))
    # The maps, small, along the bottom left.
    def thumb(a, width):
        th = width // 2
        a = a if a.ndim == 3 else np.repeat(a[..., None], 3, axis=2)
        rows = (np.arange(th) * a.shape[0] / th).astype(int)
        cols = (np.arange(width) * a.shape[1] / width).astype(int)
        return to_bytes(np.clip(a[rows][:, cols], 0, 1))
    x0 = 10
    for t in (surface, np.dstack([relief_rg[0], relief_rg[1], lights]), cloud, moon_color, nebula_color * 3.0):
        small = thumb(t, 200)
        out[ph - small.shape[0] - 10:ph - 10, x0:x0 + small.shape[1]] = small
        x0 += small.shape[1] + 8
    lt.write_png(PREVIEW, out)
    log(f'preview: {PREVIEW}')


def main():
    start = time.time()
    os.makedirs(OUT_DIR, exist_ok=True)
    h, w = SURFACE_SIZE
    color = np.zeros((h, w, 3), np.float32)
    glint = np.zeros((h, w), np.float32)
    height = np.zeros((h, w), np.float32)
    for r0 in range(0, h, CHUNK_ROWS):
        r1 = min(r0 + CHUNK_ROWS, h)
        color[r0:r1], glint[r0:r1], height[r0:r1] = surface_rows(r0, r1)
    log(f'ground: {1.0 - glint.mean():.0%} land or ice ({time.time() - start:.0f} s)')
    slope_e, slope_n = relief(height)
    lights = place_lights((h, w))
    log(f'lights ({time.time() - start:.0f} s)')
    lt.write_png(os.path.join(OUT_DIR, 'T_Teropa_BC.png'), np.dstack([to_bytes(color), to_bytes(glint)]))
    lt.write_png(os.path.join(OUT_DIR, 'T_Teropa_M.png'),
                 np.dstack([to_bytes(slope_e), to_bytes(slope_n), to_bytes(lights), np.full((h, w), 255, np.uint8)]))
    cloud = clouds()
    lt.write_png(os.path.join(OUT_DIR, 'T_TeropaClouds_M.png'), to_bytes(cloud))
    log(f'clouds ({time.time() - start:.0f} s)')
    moon_color, moon_height = moon()
    m_e, m_n = relief(moon_height * 6.0)
    mh, mw = MOON_SIZE
    lt.write_png(os.path.join(OUT_DIR, 'T_TeropaMoon_BC.png'), np.dstack([to_bytes(moon_color), np.zeros((mh, mw), np.uint8)]))
    lt.write_png(os.path.join(OUT_DIR, 'T_TeropaMoon_M.png'),
                 np.dstack([to_bytes(m_e), to_bytes(m_n), np.zeros((mh, mw), np.uint8), np.full((mh, mw), 255, np.uint8)]))
    neb = nebula()
    lt.write_png(os.path.join(OUT_DIR, 'T_MenuNebula_BC.png'), to_bytes(np.clip(neb, 0, 1) ** (1 / 2.2)))
    log(f'moon and sky ({time.time() - start:.0f} s)')
    preview(color, glint, (slope_e, slope_n), lights, cloud, moon_color, neb)
    log(f'done in {time.time() - start:.0f} s: {OUT_DIR}')


main()
