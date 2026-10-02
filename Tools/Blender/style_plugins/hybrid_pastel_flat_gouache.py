"""Pastel flat gouache: style 4's palette, paper and softness over style 1's flattening (wash-leaning hybrid).

Fills are quantized to ten colors (style 1), but every boundary between fills, and between shade tones, is softened
like a wash edge and darkened where the pigment pools (style 4). The shade is a three-tone posterized ramp with
style 1's violet in the dark tones, washed toward paper. No black ink: the edges are the pooled rims plus a faint
warm-grey sketch line. Sable's pastel sky with flat clouds that have wet wash edges.
"""
import numpy as np

STYLE = dict(name='Pastel flat gouache', reference='mix of 1 and 4 (wash-leaning)',
             note='Ten-color flat fills with soft wet-edge wash boundaries, a three-tone violet shade ramp, paper '
                  'grain and a faint pencil line; Sable pastel sky with soft-edged flat clouds. No ink.')


def _edges_rgb(img, alpha, cv2):
    """Edge strength of every color channel plus the silhouette, so same-value hue boundaries pool too."""
    mag = np.zeros(img.shape[:2], np.float32)
    for c in range(3):
        g = img[..., c]
        sx = cv2.Sobel(g, cv2.CV_32F, 1, 0, ksize=3)
        sy = cv2.Sobel(g, cv2.CV_32F, 0, 1, ksize=3)
        mag = np.maximum(mag, np.sqrt(sx * sx + sy * sy))
    sx = cv2.Sobel(alpha, cv2.CV_32F, 1, 0, ksize=3)
    sy = cv2.Sobel(alpha, cv2.CV_32F, 0, 1, ksize=3)
    return np.maximum(mag, np.sqrt(sx * sx + sy * sy) * 0.6)


def _wobble(img, S, lib, seed, amp):
    """Hand-painted edges: displaces the image by a little smooth noise so no shape boundary follows the geometry exactly."""
    cv2 = lib.cv2
    h, w = img.shape[:2]
    nx = (lib.value_noise(h, w, 28 * S, seed, octaves=2) - 0.5) * 2.0 * amp
    ny = (lib.value_noise(h, w, 28 * S, seed + 1, octaves=2) - 0.5) * 2.0 * amp
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    return cv2.remap(img, xx + nx, yy + ny, cv2.INTER_LINEAR, borderMode=cv2.BORDER_REFLECT)


def compose(sc, lib):
    S = sc.S
    cv2 = lib.cv2
    pap = lib.hexc('#f6ead6')                    # warm cream paper, between style 4's paper and Sable's horizon

    # --- flat fills (style 1): a small palette, chroma boosted first so the teal door keeps its own color.
    base = lib.flatten_region(sc.albedo, sc.ground, 12 * S, 0.9)
    base = lib.hsv_adjust(base, sat=1.3, val=1.0)
    flat8 = lib.meanshift(lib.to8(base), 16 * S, 30)
    fill, _ = lib.quantize(flat8, 10, mask=sc.geo, seed=4)
    fill = lib.hsv_adjust(fill, sat=1.3, val=1.0)
    fill = lib.contrast(fill, 1.06, 0.5)
    fill = np.clip(fill * 0.84 + 0.14, 0, 1)                            # lift toward pastel gouache
    fill = fill * np.array([1.03, 0.99, 0.95], np.float32)              # warm paper cast
    # The grass came out louder than the dusty house and pastel sky: a gouache green, muted and a little yellow.
    grass = lib.chroma(fill, 0.82, 1.0) * np.array([1.02, 1.0, 0.92], np.float32)
    fill = lib.mix(fill, grass, sc.ground.astype(np.float32))

    # --- three-tone posterized shade: lit, half tone, shadow. Violet from style 1, washed toward paper (style 4).
    # The grass's bumps make the ground's shading noisy around 0.47; a wide blur there keeps it one lit wash
    # while the walls (front wall about 0.34, the lit side 0.8, cast shadows 0) keep a crisp tone boundary.
    sn = np.where(sc.ground, lib.blur(sc.sun_n, 5.0 * S), lib.blur(sc.sun_n, 1.0 * S)).astype(np.float32)
    t_lit = lib.smoothstep(0.38, 0.45, sn)
    t_mid = lib.smoothstep(0.10, 0.17, sn)
    mid = fill * np.array([0.83, 0.81, 0.94], np.float32)                # cooler half tone, less pink on plaster
    dark = fill * np.array([0.58, 0.53, 0.80], np.float32)
    dark = lib.mix(dark, pap * 0.70, 0.12)                               # washed, not black
    out = lib.mix(lib.mix(dark, mid, t_mid), fill, t_lit)
    out *= lib.mix(0.88, 1.0, lib.smoothstep(0.15, 0.85, sc.ao))[..., None]

    # --- wash boundaries: wobble the shapes, soften every edge, and pool pigment along it (style 4's wet edge).
    out = _wobble(out, S, lib, 211, 1.6 * S)
    alpha_w = _wobble(sc.alpha, S, lib, 211, 1.6 * S)
    mist_w = _wobble(sc.mist, S, lib, 211, 1.6 * S)
    edges = np.clip(_edges_rgb(out, alpha_w, cv2) * 2.2, 0, 1)
    out = lib.mix(out, lib.blur(out, 2.0 * S), 0.7)
    pooled = lib.blur(edges, 1.4 * S)
    pooled = np.clip(pooled * 1.3, 0, 1)
    out *= (1.0 - 0.34 * pooled)[..., None]
    out = lib.mix(out, lib.blur(out, 3.5 * S), 0.22)                     # pigment bleeding
    gran = lib.value_noise(sc.H, sc.W, 6 * S, 41, octaves=2) - 0.5
    out *= (1.0 + gran * 0.12 * (1.0 - lib.lum(out)))[..., None]       # granulation in the darker washes
    uneven = lib.value_noise(sc.H, sc.W, 160 * S, 71, octaves=3) - 0.5
    out *= (1.0 + uneven * 0.09)[..., None]                             # no wash dries perfectly even

    # --- distance fades to the paper and the sky's horizon color (both parents do this).
    haze_col = lib.hexc('#f4dfc6')
    out = lib.haze(out, mist_w, haze_col, 0.85, start=0.04, end=0.72)

    # --- Sable's pastel sky, flat clouds with a soft wash edge and pooled rim instead of ink.
    sky = lib.sky_gradient(sc.H, sc.W, sc.horizon,
                           [(0.0, '#f4dfc6'), (0.3, '#e4cbd9'), (0.65, '#bfd3ea'), (1.0, '#a2c1e6')])
    bloom = lib.value_noise(sc.H, sc.W, 240 * S, 51, octaves=3, aspect=2.5)
    sky = lib.mix(sky, pap, lib.smoothstep(0.6, 0.85, bloom) * 0.3)      # a little uneven wash in the sky
    clouds = lib.cloud_mask(sc.H, sc.W, sc.horizon, 11, 150 * S, 0.30, 0.03, aspect=3.2, band=(0.15, 0.85))
    clouds = lib.blur(clouds, 1.6 * S)
    cloud_col = lib.hexc('#fbf6ee')
    rim_px = max(1, int(round(3 * S)))
    rim = np.clip(lib.dilate_mask(clouds, rim_px) - clouds, 0, 1)
    rim = lib.blur(rim, 1.2 * S)
    inner = np.clip(clouds - lib.erode_mask(clouds, rim_px), 0, 1)
    sky = lib.mix(sky, cloud_col, clouds)
    sky = lib.mix(sky, sky * np.array([0.90, 0.88, 0.94], np.float32), rim * 0.75)      # pooled pigment outside
    sky = lib.mix(sky, sky * np.array([0.95, 0.94, 0.98], np.float32), inner * 0.6)     # slightly denser rim inside
    out = lib.comp(out, sky, alpha_w)

    # --- paper and the sketch line: faint, warm grey, slightly offset like an underdrawing.
    out *= lib.paper(sc.H, sc.W, 61, fiber=0.035, mottle=0.045)[..., None]
    pencil = lib.shifted(sc.lines_sketch, 1.2 * S, -0.8 * S)
    pencil = np.clip(lib.blur(pencil, 0.5 * S) * 1.1, 0, 1)
    out = lib.ink(out, pencil, lib.hexc('#7a6a61'), 0.45)
    return np.clip(out, 0, 1).astype(np.float32)
