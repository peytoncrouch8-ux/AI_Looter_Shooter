"""Storybook ink: an even blend of 1 (Ink & flat color) and 4 (Gouache storybook).

From 1: Sable's pastel sky with flat outlined clouds, a two-tone shade with a violet cast, dark ink lines.
From 4: soft meanshift fills that keep gentle color variation, wet-edge pigment pooling at every wash boundary (fill
edges and the lit/shade terminator), paper grain, pencil underdrawing, and everything fading to paper far away.
"""
import numpy as np

STYLE = dict(name='Storybook ink', reference='mix of 1 and 4 (balanced)',
             note='Two-tone shade with a short soft edge over gouache fills, brown ink over a pencil sketch, pastel sky '
                  'with outlined wash clouds.')


def compose(sc, lib):
    S = sc.S
    H, W = sc.H, sc.W

    # --- fills: flattened like gouache, never quantized, so plaster and stone keep a gentle wash variation
    flat8 = lib.meanshift(lib.to8(sc.albedo), 12 * S, 24)
    fill = lib.f32(flat8)
    fill = lib.flatten_region(fill, sc.ground, 8 * S, 0.55)
    fill = lib.hsv_adjust(fill, sat=1.35, val=1.0)
    fill = np.clip(fill * 0.88 + 0.10, 0, 1)                         # lift toward pastel: no true black
    fill = fill * np.array([1.03, 0.99, 0.96], np.float32)            # warm paper cast
    fill = lib.mix(fill, fill * np.array([0.97, 1.05, 0.90], np.float32), sc.ground * 0.7)   # a fresher grass

    # --- shading: a ramp with a steep middle. Nearly every pixel is clearly lit or clearly shaded; the transition
    # is short and soft like a wash edge rather than a hard cel cut.
    sun_b = lib.blur(sc.sun_n, 1.2 * S)
    lit = lib.smoothstep(0.33, 0.57, sun_b)
    lit = lib.blur(lit, 0.8 * S)
    # The violet shade tone goes on the neutral surfaces (plaster, stone, timber); a saturated color such as the
    # teal door only darkens, so its hue still reads in the shade.
    mx, mn = fill.max(-1), fill.min(-1)
    neutral = 1.0 - lib.smoothstep(0.22, 0.5, (mx - mn) / (mx + 1e-6))
    violet = lib.mix(np.array([1.0, 1.0, 1.0], np.float32), np.array([0.96, 0.87, 1.09], np.float32), neutral)
    shade_col = fill * 0.79 * violet
    shade_col = lib.mix(shade_col, shade_col * np.array([0.93, 0.93, 1.06], np.float32), sc.ground * 0.8)  # cool grass shadow
    shade_col *= (0.92 + 0.08 * lib.smoothstep(0.0, 0.3, sun_b))[..., None]   # faint gradation inside the shade
    lit_col = fill * (0.95 + 0.05 * lib.smoothstep(0.55, 1.0, sun_b))[..., None]
    out = lib.mix(shade_col, lit_col, lit)
    out *= lib.mix(0.87, 1.0, lib.smoothstep(0.2, 0.9, sc.ao))[..., None]

    # --- gouache: pigment bleeding, then pooling at the wash boundaries (color edges and the terminator)
    out = lib.mix(out, lib.blur(out, 3 * S), 0.18)
    edges = np.clip(lib.edge_mag(fill, 1.0) * 2.2, 0, 1)
    pooled = lib.blur(edges, 1.2 * S)
    out *= (1.0 - 0.30 * pooled)[..., None]
    tpool = np.clip((lib.blur(lit, 3 * S) - lit) * 2.5, 0, 1) * sc.geo
    out *= (1.0 - 0.20 * tpool)[..., None]
    # A sparse Sable stipple on the near ground, fading into the wash with distance.
    ground_dots = lib.halftone(np.full((H, W), 0.10, np.float32), 10 * S, 40) * sc.ground
    ground_dots *= 1.0 - lib.smoothstep(0.08, 0.5, sc.mist)
    out = lib.mix(out, out * 0.88, ground_dots * 0.7)
    gran = lib.value_noise(H, W, 6 * S, 41, octaves=2) - 0.5
    out *= (1.0 + gran * 0.10 * (1.0 - lib.lum(out)))[..., None]

    pap = lib.hexc('#f3e4cc')
    out = lib.haze(out, sc.mist, pap, 0.82, start=0.03, end=0.75)

    # --- sky: Sable's pastel gradient and flat clouds, outlined in brown, their fill showing wash texture
    sky = lib.sky_gradient(H, W, sc.horizon, [(0.0, '#f6dcb8'), (0.3, '#e4c9d9'), (0.65, '#bcd4ee'), (1.0, '#9ec1ea')])
    clouds = lib.cloud_mask(H, W, sc.horizon, 11, 150 * S, 0.30, 0.03, aspect=3.2, band=(0.15, 0.85))
    hard = (clouds > 0.5).astype(np.float32)
    cloud_fill = np.broadcast_to(lib.hexc('#fbf6ef'), (H, W, 3)).astype(np.float32)
    mottle = lib.value_noise(H, W, 40 * S, 71, octaves=3) - 0.5
    cloud_fill = cloud_fill * (1.0 + mottle * 0.08 + gran * 0.05)[..., None]
    inner = lib.blur(hard - lib.erode_mask(hard, max(1, int(round(4 * S)))), 1.5 * S)
    cloud_fill = lib.mix(cloud_fill, lib.hexc('#d9d3df'), np.clip(inner, 0, 1) * 0.45)   # pigment pools inside the rim
    sky = lib.mix(sky, cloud_fill, clouds)
    cloud_edge = lib.blur(np.clip(lib.dilate_mask(hard, max(1, int(round(2 * S)))) - hard, 0, 1), 0.6 * S)
    cloud_pencil = lib.shifted(cloud_edge, 2.0 * S, -1.5 * S)
    sky = lib.ink(sky, cloud_pencil, lib.hexc('#8a7a78'), 0.35)
    sky = lib.ink(sky, cloud_edge, lib.hexc('#4a3024'), 0.85)

    out = lib.comp(out, sky, sc.alpha)
    out *= lib.paper(H, W, 61, fiber=0.03, mottle=0.045)[..., None]

    # --- lines: faint pencil underdrawing a pixel or two off, then medium dark-brown ink on top
    pencil = lib.shifted(sc.lines_sketch, 1.5 * S, -1.5 * S)
    out = lib.ink(out, pencil, lib.hexc('#6b5a58'), 0.35)
    out = lib.ink(out, sc.lines_clean, lib.hexc('#3a2117'), 0.92)
    return np.clip(out, 0, 1).astype(np.float32)
