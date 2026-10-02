"""Hybrid of style 1 (Ink & flat color) and style 4 (Gouache storybook), balanced: "Moebius watercolor" (Arzach).

From 1: flattened fills with a partial palette snap, clean uniform-weight near-black ink, the pastel Moebius sky.
From 4: the fills are pale, luminous watercolor washes that grade softly from lit to a cooler, more saturated shade,
with pigment bleed, wet-edge pooling, subtle granulation and paper. No stipple, no outlines on the clouds.
"""
import numpy as np

STYLE = dict(name='Moebius Watercolor', reference='mix of 1 and 4 (balanced)',
             note='Clean uniform ink over pale luminous watercolor washes that grade softly from lit to a cooler shade; '
                  'Moebius gradient sky with soft, unlined clouds.')


def masked_blur(lib, x, mask, sigma):
    """Blur that does not pull the sky's zeros into the silhouettes (no dark halo along the roof line)."""
    m = mask.astype(np.float32)
    return lib.blur(x * m, sigma) / np.maximum(lib.blur(m, sigma), 1e-3)


def light_ramp(lib, sun):
    """Three soft tones: cast shadows are the full shade, faces turned from the sun a half tone, sun faces lit."""
    full = lib.smoothstep(0.04, 0.30, sun)
    half = lib.smoothstep(0.30, 0.92, sun)
    return 0.62 * full + 0.38 * half


def compose(sc, lib):
    S = sc.S
    H, W = sc.H, sc.W
    ground = sc.ground.astype(np.float32)

    # ---- flat fills (style 1). Pre-saturate so the teal door keeps its own palette color; flatten the grass.
    alb = lib.hsv_adjust(sc.albedo, sat=1.5)
    alb = lib.flatten_region(alb, sc.ground, 14 * S, 0.95)
    flat8 = lib.meanshift(lib.to8(alb), 12 * S, 26)
    q, _ = lib.quantize(flat8, 12, mask=sc.geo, seed=4)
    fill = lib.mix(lib.f32(flat8), q, 0.45)
    # Diluted pigment: a strong lift toward the paper, then the hue brought back (pale but not grey).
    fill = np.power(np.clip(fill, 0, 1), 0.5) * 0.92 + 0.08
    fill = lib.chroma(fill, 1.7)
    fill = fill * np.array([1.03, 1.0, 0.95], np.float32)             # warm paper cast
    grass = lib.hsv_adjust(fill, sat=0.56, val=1.08) * np.array([0.99, 1.0, 0.95], np.float32)   # a pale, soft green
    fill = lib.mix(fill, grass, ground)

    # ---- watercolor shading (style 4): a soft terminator plus a wider bleed, the shade cooler and more saturated.
    sun_soft = masked_blur(lib, sc.sun_n, sc.geo, 2.5 * S)
    sun_wide = masked_blur(lib, sc.sun_n, sc.geo, 14 * S)
    lit = 0.75 * light_ramp(lib, sun_soft) + 0.25 * light_ramp(lib, sun_wide)
    shade = lib.chroma(fill * np.array([0.68, 0.70, 0.96], np.float32), 1.4)
    out = lib.mix(shade, fill, lit)
    out *= lib.mix(0.86, 1.0, lib.smoothstep(0.15, 0.9, masked_blur(lib, sc.ao, sc.geo, 1.5 * S)))[..., None]
    # The wash grades toward the sun side (upper left) across the whole sheet, and the grass wash is uneven.
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    grad = 1.0 + 0.06 * ((0.5 - xx / W) * 0.6 + (0.5 - yy / H))
    out *= grad[..., None]
    ground_wash = lib.value_noise(H, W, 160 * S, 29, octaves=3, aspect=2.5) - 0.5
    out *= (1.0 + ground_wash * 0.10 * ground)[..., None]

    # ---- the wash itself: uneven pigment, a little bleed, pooling at wash edges and along the terminator, granulation.
    wash = lib.value_noise(H, W, 70 * S, 23, octaves=3) - 0.5
    out *= (1.0 + wash * 0.07)[..., None]
    out = lib.mix(out, lib.blur(out, 2.5 * S), 0.3)
    edges = np.clip(lib.edge_mag(fill, 1.0) * 2.0, 0, 1)
    pooled = lib.blur(edges, 2.0 * S)
    term = np.clip(lib.edge_mag(np.repeat(lit[..., None], 3, -1), 0.0) * 1.5, 0, 1)
    pooled = np.clip(pooled + lib.blur(term, 2.0 * S) * 0.8, 0, 1)
    out *= (1.0 - 0.20 * pooled)[..., None]
    gran = lib.value_noise(H, W, 5 * S, 41, octaves=2) - 0.5
    out *= (1.0 + gran * 0.10 * (1.0 - lib.lum(out)))[..., None]
    out = np.clip(out * 0.94 + 0.05, 0, 1)                             # paper shows through everything

    # ---- distance fades to the horizon color.
    hor = lib.hexc('#f5d8bd')
    out = lib.haze(out, sc.mist, hor, 0.85, start=0.03, end=0.7)

    # ---- Moebius sky: peach to lavender to pale blue, soft watercolor clouds without outlines.
    sky = lib.sky_gradient(H, W, sc.horizon, [(0.0, hor), (0.12, '#f1d1c9'), (0.38, '#d4c0e3'), (0.7, '#bdd2ee'),
                                               (1.0, '#a3c3e9')])
    clouds = lib.cloud_mask(H, W, sc.horizon, 11, 180 * S, 0.33, 0.10, aspect=3.0, band=(0.12, 0.85))
    clouds = lib.blur(clouds, 3 * S)
    under = np.clip(clouds - lib.shifted(clouds, 0, -10 * S), 0, 1)
    sky = lib.mix(sky, lib.hexc('#fcf8f2'), clouds * 0.9)
    sky = lib.mix(sky, lib.hexc('#d6c9e4'), lib.blur(under, 3 * S) * 0.5)
    bloom = lib.value_noise(H, W, 260 * S, 51, octaves=3, aspect=2.5) - 0.5
    sky *= (1.0 + bloom * 0.05)[..., None]

    out = lib.comp(out, sky, sc.alpha)
    out *= lib.paper(H, W, 61, fiber=0.03, mottle=0.03)[..., None]
    out = lib.ink(out, sc.lines_clean, lib.hexc('#1c171d'), 0.92)
    return np.clip(out, 0, 1).astype(np.float32)
