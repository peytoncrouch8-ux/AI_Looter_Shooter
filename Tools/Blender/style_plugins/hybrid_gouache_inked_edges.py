"""Hybrid of style 4 (Gouache storybook) and style 1 (Ink & flat color), wash-leaning.

The painting is the gouache parent: meanshifted washes, pigment bleeding and pooling, granulation, paper grain, a
pencil underdrawing and everything fading to the paper with distance. From Sable it takes just two things: a bold ink
contour on the silhouette and the major form edges only (the Freestyle clean lines kept where the depth jumps), and
the two-tone shade, which here is a wash boundary with a wet, pooled edge in a violet tone instead of a hard step.
The sky is Sable's pastel gradient laid down as a pale wash.
"""
import numpy as np

STYLE = dict(name='Inked Gouache', reference='mix of 1 and 4 (wash-leaning)',
             note='Gouache washes, wet-edge two-tone violet shade and pencil interiors; bold Sable ink only on '
                  'silhouettes and depth edges; pastel sky as a pale wash.')


def _depth_contour(sc, lib):
    """Where the ink goes: (region, contact). region is a mask (0..1) around depth discontinuities on the house and
    around its outline; contact is the line where the walls meet the ground, which Freestyle never draws because the
    plinth sinks into the ground mesh, so this style draws it itself as Sable grounds every shape."""
    import cv2
    S = sc.S
    dlog = np.log(np.maximum(sc.depth, 0.1)).astype(np.float32)
    gx = cv2.Sobel(dlog, cv2.CV_32F, 1, 0, ksize=3)
    gy = cv2.Sobel(dlog, cv2.CV_32F, 0, 1, ksize=3)
    step = np.sqrt(gx * gx + gy * gy) / 8.0                       # relative depth change per pixel
    near_house = lib.dilate_mask(sc.house.astype(np.float32), max(1, int(round(4 * S)))) > 0.5
    depth_edge = lib.smoothstep(0.012, 0.03, step) * near_house * sc.geo
    # The outline against sky and ground has no depth step at the wall's foot, so take the house mask's rim too.
    hm = sc.house.astype(np.float32)
    r = max(1, int(round(2 * S)))
    outline = np.clip(lib.dilate_mask(hm, r) - lib.erode_mask(hm, r), 0, 1)
    region = np.clip(depth_edge + outline, 0, 1)
    on_ground = lib.dilate_mask(sc.ground.astype(np.float32), max(1, int(round(3 * S)))) > 0.5
    contact = outline * on_ground * (1.0 - lib.smoothstep(0.3, 0.6, sc.mist))
    return lib.dilate_mask(region, max(1, int(round(3 * S)))), contact


def compose(sc, lib):
    S = sc.S
    H, W = sc.H, sc.W
    pap = lib.hexc('#f7f1e4')

    # ---- the washes (gouache parent)
    flat8 = lib.meanshift(lib.to8(sc.albedo), 8 * S, 18)
    fill = lib.f32(flat8)
    fill = lib.flatten_region(fill, sc.ground, 10 * S, 0.7)
    fill = lib.hsv_adjust(fill, sat=1.35, val=1.0)
    # A wash is never even: a slow mottle over the ground, lifted a little toward the paper.
    mottle = lib.value_noise(H, W, 140 * S, 17, octaves=3, aspect=2.0) - 0.5
    fill = fill * (1.0 + mottle * 0.12 * sc.ground)[..., None]
    fill = lib.mix(fill, pap, sc.ground * 0.08)

    # ---- two-tone shade as a wash boundary: a soft, wandering terminator with a wet edge that pools pigment
    wander = (lib.value_noise(H, W, 18 * S, 23, octaves=3) - 0.5) * 0.10
    sun_b = lib.blur(sc.sun_n, 1.2 * S) + wander
    # The flat ground only ever reaches about half the sun of a face turned to it, so its cast shadow splits lower.
    lo = np.where(sc.ground, 0.16, 0.40).astype(np.float32)
    lit = lib.smoothstep(lo, lo + 0.11, sun_b)
    shade_tint = np.array([0.72, 0.68, 0.92], np.float32)        # Sable's violet, as a translucent wash
    shade = lib.mix(fill * shade_tint, lib.hexc('#9d90c4'), 0.18)   # the wash's own color shows through
    out = lib.mix(shade, fill, lit)
    occl = 0.86 + 0.14 * lib.smoothstep(0.1, 0.9, sc.ao)
    out = out * occl[..., None]
    out = np.clip(out * 0.86 + 0.12, 0, 1)                        # no true black in a wash
    # Wet edge: the shade wash dries darker just inside its boundary.
    shadow = (1.0 - lit) * sc.geo
    inner = np.clip(shadow - lib.erode_mask(shadow, max(1, int(round(4 * S)))), 0, 1)
    wet = lib.blur(inner, 1.5 * S) * lib.smoothstep(0.1, 0.5, 1.0 - sc.mist)
    out = out * (1.0 - 0.30 * wet)[..., None]

    # ---- pigment: bleeding between washes, pooling at the edge of each wash, granulation
    out = lib.mix(out, lib.blur(out, 4 * S), 0.35)
    edges = np.clip(lib.edge_mag(fill, 1.0) * 2.4, 0, 1)
    pooled = lib.blur(edges, 1.2 * S)
    out = out * (1.0 - 0.32 * pooled)[..., None]
    gran = lib.value_noise(H, W, 6 * S, 41, octaves=2) - 0.5
    out = out * (1.0 + gran * 0.14 * (1.0 - lib.lum(out)))[..., None]
    out = lib.haze(out, sc.mist, pap, 0.9, start=0.02, end=0.7)

    # ---- sky: Sable's pastel gradient as a pale wash, with soft paper blooms for clouds
    stops = [(0.0, '#f6dcb8'), (0.3, '#e4c9d9'), (0.65, '#bcd4ee'), (1.0, '#9ec1ea')]
    sky = lib.sky_gradient(H, W, sc.horizon, stops)
    sky = lib.mix(sky, pap, 0.38)
    clouds = lib.cloud_mask(H, W, sc.horizon, 11, 150 * S, 0.30, 0.07, aspect=3.2, band=(0.15, 0.85))
    cloud_rim = np.clip(lib.blur(lib.dilate_mask(clouds, max(1, int(round(3 * S)))), 2 * S) - clouds, 0, 1)
    sky = lib.mix(sky, pap, clouds * 0.9)
    sky = lib.mix(sky, sky * np.array([0.90, 0.90, 0.97], np.float32), cloud_rim * 0.5)  # pooled cloud edge
    bloom = lib.value_noise(H, W, 220 * S, 51, octaves=3, aspect=2.5)
    sky = lib.mix(sky, pap, lib.smoothstep(0.6, 0.85, bloom) * 0.5)
    out = lib.comp(out, sky, sc.alpha)
    out = out * lib.paper(H, W, 61, fiber=0.045, mottle=0.06)[..., None]

    # ---- lines: pencil underdrawing inside, bold ink on the silhouette and depth edges only
    pencil = lib.shifted(sc.lines_sketch, 1.5 * S, -1.0 * S)
    out = lib.ink(out, pencil, lib.hexc('#4a3f44'), 0.55)
    contour, contact = _depth_contour(sc, lib)
    bold = lib.dilate_mask(sc.lines_clean, max(1, int(round(1 * S))))
    bold = np.clip(lib.blur(bold, 0.9 * S) * 1.5, 0, 1) * contour     # about 5 px: a loaded brush, not a pen
    bold = np.maximum(bold, np.clip(lib.blur(contact, 0.9 * S) * 1.3, 0, 1))
    weight = 0.82 + 0.18 * lib.value_noise(H, W, 40 * S, 77, octaves=2)   # a brush's uneven load
    out = lib.ink(out, bold * weight, lib.hexc('#1e171b'), 0.96)
    return np.clip(out, 0, 1).astype(np.float32)
