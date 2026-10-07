"""Screen Print Wash: the art style the user chose on 2026-10-02 (Docs/Art/ScreenPrintWash.md).

A hybrid of style 1 (Ink & flat color) and style 4 (Gouache storybook), ink-leaning: a flat color print on
watercolor paper. The fills are style 1's quantized flats with the hard violet two-tone shade, but the shade carries
pigment granulation instead of stipple. The color plate is misregistered from the line plate like a screen print and
bleeds softly at its edges. Lines come from style 4: the sketchy pencil pass under a thinner, lighter ink line.
Everything sits on cream paper with fibers; the sky is Sable's flat pastel gradient on that paper.

    python Tools/Blender/style_compose.py <passdir> <outdir> --no-builtin --plugins Tools/Blender/style_plugins/screen_print_wash.py
"""
import numpy as np

STYLE = dict(name='Screen Print Wash', reference='mix of 1 and 4 (ink-leaning)',
             note='Flat quantized fills with a hard violet shade that granulates like pigment, misregistered from a '
                  'pencil-under-ink line pair, on cream paper with a flat pastel sky.')


def _thin(lib, line, px):
    """Erodes an anti-aliased line from one side by px: a 2 px line becomes 1 px and stays unbroken."""
    out = line
    for dx in range(px + 1):
        for dy in range(px + 1):
            if dx or dy:
                out = np.minimum(out, lib.shifted(line, -dx, -dy))
    return out


def compose(sc, lib):
    S = sc.S
    H, W = sc.H, sc.W
    cream = lib.hexc('#f6eedc')

    # --- the color plate: style 1's flat fills ---------------------------------------------------------------
    flat8 = lib.meanshift(lib.to8(sc.albedo), 16 * S, 30)
    fill, _ = lib.quantize(flat8, 10, mask=sc.geo, seed=4)
    fill = lib.flatten_region(fill, sc.ground, 5 * S, 0.6)           # the ground is one flat wash, not speckle
    # The teal (the door) is the one color the cream lift and the violet shade would turn grey-blue: keep it teal.
    teal = lib.smoothstep(0.03, 0.11, np.minimum(fill[..., 1], fill[..., 2]) - fill[..., 0]) * sc.house
    fill = lib.mix(fill, lib.hsv_adjust(fill, sat=1.7, val=1.12), teal)
    fill = lib.hsv_adjust(fill, sat=1.22, val=1.0)
    fill = np.clip(fill * 0.83 + cream * 0.17, 0, 1)                 # lift toward pastel, into the cream paper
    # Hard two-tone shade, the terminator softened a hair so the wash edge reads wet rather than vector-sharp.
    lit = lib.smoothstep(0.42, 0.50, lib.blur(sc.sun_n, 0.8 * S))
    shade = fill * np.array([0.70, 0.64, 0.86], np.float32)          # style 1's violet shade tone
    shade = lib.hsv_adjust(shade, sat=1.15)
    shade = lib.mix(shade, fill * np.array([0.60, 0.74, 0.80], np.float32), teal)   # teal shades to deep teal
    out = lib.mix(shade, fill, lit)
    out *= lib.mix(1.0, 0.90, (1.0 - lit) * sc.ground)[..., None]     # the cast shadow on the grass must read
    out *= lib.mix(0.88, 1.0, lib.smoothstep(0.2, 0.9, sc.ao))[..., None]

    # Pigment granulation in the shade instead of stipple: a soft mottle plus fine grains settling in the paper.
    shade_mask = (1.0 - lit) * sc.geo
    mottle = lib.value_noise(H, W, 5 * S, 41, octaves=3, persistence=0.55) - 0.5
    rng = np.random.default_rng(23)
    grains = lib.blur(rng.standard_normal((H, W)).astype(np.float32), 0.9 * S)
    grains /= grains.std() + 1e-6
    grains = lib.smoothstep(0.9, 2.2, grains)                        # only the sparse dark specks
    gran = 1.0 + mottle * 0.20 - grains * 0.17
    out *= lib.mix(1.0, gran, shade_mask * 0.95)[..., None]
    out = np.clip(out, 0, 1)

    # Pigment pooling at each fill's edge (gouache), gentle, then a soft bleed of the whole plate.
    edges = np.clip(lib.edge_mag(fill, 1.0) * 2.2, 0, 1) * sc.house
    pooled = lib.blur(edges, 1.0 * S)
    out *= (1.0 - 0.10 * pooled)[..., None]
    out = lib.mix(out, lib.blur(out, 3.0 * S), 0.30)

    out = lib.haze(out, sc.mist, lib.hexc('#f1dcc0'), 0.8, start=0.03, end=0.8)

    # --- the sky: Sable's pastel gradient, flat; a few flat paper-white clouds ------------------------------
    sky = lib.sky_gradient(H, W, sc.horizon, [(0.0, '#f6dcb8'), (0.3, '#e4c9d9'), (0.65, '#bcd4ee'), (1.0, '#9ec1ea')])
    clouds = lib.cloud_mask(H, W, sc.horizon, 11, 150 * S, 0.26, 0.03, aspect=3.2, band=(0.15, 0.85))
    cloud_edge = np.clip(lib.dilate_mask(clouds, max(1, int(round(S)))) - clouds, 0, 1)
    sky = lib.mix(sky, lib.hexc('#fefbf4'), clouds)

    # --- misregistration: the color plate (fills and their coverage) lands a few px off the line plate -------
    dx, dy = 2.5 * S, 1.5 * S
    plate = lib.shifted(out, dx, dy)
    alpha = lib.shifted(lib.blur(sc.alpha, 0.6 * S), dx, dy)         # a slightly soft, bleeding silhouette
    out = lib.comp(plate, sky, alpha)

    # --- cream paper with fibers, over everything (the sky too) --------------------------------------------
    pap = lib.paper(H, W, 61, fiber=0.035, mottle=0.06) - 1.0
    show = 0.35 + 0.65 * lib.lum(out)
    out = np.clip(out * (1.0 + pap * show)[..., None], 0, 1)
    out = lib.contrast(out, 1.05, pivot=0.55)

    # --- the line plates: pencil under, thin light ink over ------------------------------------------------
    pencil = lib.shifted(sc.lines_sketch, 1.5 * S, -1.0 * S)
    out = lib.ink(out, pencil, lib.hexc('#4a3f44'), 0.68)
    cloud_pencil = lib.shifted(cloud_edge, 1.5 * S, -1.0 * S)
    out = lib.ink(out, cloud_pencil, lib.hexc('#5a4c52'), 0.6)
    thin = _thin(lib, sc.lines_clean, max(1, int(round(S))))
    out = lib.ink(out, thin, lib.hexc('#2a2024'), 0.88)

    return np.clip(lib.grain(out, 0.008), 0, 1).astype(np.float32)
