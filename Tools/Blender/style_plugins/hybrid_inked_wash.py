"""Inked wash: style 1's ink skeleton (bold clean lines, hard violet two-tone, Sable's pastel sky and flat clouds)
with style 4's watercolor body (granulated washes, wet-edge pooling, colour bleeding past the line, cream paper)."""
import numpy as np

STYLE = dict(name='Inked Wash', reference='mix of 1 and 4 (ink-leaning)',
             note='Sable ink lines and hard violet shade over granulated watercolor washes that pool at the lines '
                  'and bleed past them; the pastel sky is a wash with soft blooms and flat outlined clouds.')


def violet_shade(lib, rgb, darken=0.68, a_shift=7.0, b_shift=-13.0):
    """The shade wash: a violet-grey glaze that keeps each local colour's hue relation (teal stays teal-ish,
    cream goes mauve) instead of a multiply that turns everything blue."""
    lab = lib.cv2.cvtColor(np.clip(rgb * darken, 0, 1).astype(np.float32), lib.cv2.COLOR_RGB2Lab)
    lab[..., 1] += a_shift
    lab[..., 2] += b_shift
    return np.clip(lib.cv2.cvtColor(lab, lib.cv2.COLOR_Lab2RGB), 0, 1)


def compose(sc, lib):
    S, H, W = sc.S, sc.H, sc.W
    px = lambda v: max(1, int(round(v * S)))

    # ---- flat colour (style 1), then loosened into washes (style 4)
    alb = lib.hsv_adjust(sc.albedo, sat=1.3)                                 # so the teal door keeps its own cluster
    flat8 = lib.meanshift(lib.to8(alb), 14 * S, 28)
    q, _ = lib.quantize(flat8, 11, mask=sc.geo, seed=4)
    fill = lib.mix(q, lib.f32(flat8), 0.35)                                 # a wash is not a perfect flat
    fill = lib.flatten_region(fill, sc.ground, 12 * S, 0.85)              # grass is one wash, not speckle
    fill = np.clip(fill * 0.84 + 0.17, 0, 1)                               # pastel lift
    fill = fill * np.array([1.03, 0.99, 0.955], np.float32)                # warm paper cast
    fill = lib.chroma(fill, 1.15)
    fill = lib.mix(fill, lib.blur(fill, 2.5 * S), 0.5)                     # pigment spreads: no razor fill edges

    # ---- hard two-tone light with a violet shade (style 1); the shade wash gets a pooled wet edge (style 4)
    lit = lib.smoothstep(0.42, 0.5, sc.sun_n)
    out = lib.mix(violet_shade(lib, fill), fill, lit)
    out *= lib.mix(0.90, 1.0, lib.smoothstep(0.2, 0.9, sc.ao))[..., None]
    shade_m = (1.0 - lit) * sc.geo
    term = np.clip(shade_m - lib.erode_mask(shade_m, px(4)), 0, 1)          # band just inside the shade
    term = lib.blur(term, 1.5 * S)
    out = lib.mix(out, out * np.array([0.86, 0.82, 0.93], np.float32), term * 0.75)

    # ---- granulation and uneven wash (style 4), heavier where the pigment is dark
    gran = lib.value_noise(H, W, 5 * S, 41, octaves=3, persistence=0.6) - 0.5
    mottle = lib.value_noise(H, W, 70 * S, 43, octaves=3) - 0.5
    dark = 1.0 - lib.lum(out)
    gran_coarse = lib.value_noise(H, W, 11 * S, 45, octaves=2, persistence=0.5) - 0.5
    gran_amt = np.where(sc.ground, 0.0, 0.05 + 0.13 * dark)                # fine pigment on the house...
    out *= (1.0 + gran * gran_amt + gran_coarse * 0.09 * sc.ground + mottle * 0.06)[..., None]   # ...coarser on grass
    gstreak = lib.value_noise(H, W, 60 * S, 57, octaves=2, aspect=10.0) - 0.5
    out *= (1.0 + gstreak * 0.07 * sc.ground)[..., None]                  # the grass wash was laid in strokes

    # ---- wet edges: the wash pools darker where it meets an ink line
    rim = np.clip(lib.dilate_mask(sc.lines_clean, px(3)) - lib.dilate_mask(sc.lines_clean, px(1)), 0, 1)
    pooled = lib.blur(rim, 1.2 * S)
    out *= (1.0 - 0.14 * pooled)[..., None]

    # ---- distance fades to the paper (style 4's cream, style 1's peach)
    pap = lib.hexc('#f7efdc')
    out = lib.haze(out, sc.mist, lib.hexc('#f2dec4'), 0.78, start=0.03, end=0.8)

    # ---- colour bleeds past the ink: extend edge colours past the silhouette and shift the wash off the lines
    a = sc.alpha.astype(np.float32)
    ext = lib.blur(out * a[..., None], 3.0 * S) / np.maximum(lib.blur(a, 3.0 * S), 1e-3)[..., None]
    out = np.where(sc.geo[..., None], out, np.clip(ext, 0, 1)).astype(np.float32)
    # The wash sits off the lines by a wandering 1-2 px, not one fixed shift: an off-register that changes side.
    wx = lib.value_noise(H, W, 120 * S, 71, octaves=2) - 0.5
    wy = lib.value_noise(H, W, 120 * S, 73, octaves=2) - 0.5
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    out = lib.cv2.remap(out, xx - (0.7 * S + wx * 3.2 * S), yy - (-0.5 * S + wy * 3.2 * S), lib.cv2.INTER_LINEAR,
                        borderMode=lib.cv2.BORDER_REFLECT)
    near_house = lib.dilate_mask(sc.house.astype(np.float32), px(3))
    alpha_b = np.maximum(a, lib.blur(near_house, 0.8 * S) * 0.9)          # the house wash runs 2-3 px past its outline
    alpha_b = np.clip(alpha_b, 0, 1)

    # ---- sky: Sable's pastel gradient laid as a watercolor wash with blooms, pooling and streaks
    sky = lib.sky_gradient(H, W, sc.horizon, [(0.0, '#f6dcb8'), (0.3, '#e4c9d9'), (0.65, '#bcd4ee'), (1.0, '#9ec1ea')])
    bloom = lib.value_noise(H, W, 230 * S, 51, octaves=3, aspect=2.6)
    bm = lib.smoothstep(0.5, 0.82, bloom)
    sky = lib.mix(sky, pap, bm * 0.55)                                     # water pushed the pigment away
    bloom_ring = lib.blur(np.clip(lib.dilate_mask(bm, px(7)) - bm, 0, 1), 2.0 * S)
    sky *= (1.0 - 0.09 * bloom_ring)[..., None]                            # ...and it dried in a darker ring
    pool = lib.smoothstep(0.52, 0.8, lib.value_noise(H, W, 140 * S, 53, octaves=3, aspect=3.0))
    sky = lib.mix(sky, lib.chroma(sky, 1.25, 0.95), pool * 0.6)            # pooled, deeper pigment
    streak = lib.value_noise(H, W, 90 * S, 55, octaves=2, aspect=9.0) - 0.5
    sky *= (1.0 + streak * 0.06 + gran * 0.06 + mottle * 0.04)[..., None]

    # ---- clouds: flat Sable shapes, but filled with wash texture and a pooled rim inside the edge
    clouds = lib.cloud_mask(H, W, sc.horizon, 11, 150 * S, 0.30, 0.03, aspect=3.2, band=(0.15, 0.85))
    cloud_col = lib.hexc('#fbf6ef')[None, None, :] * (1.0 + gran * 0.07 + mottle * 0.10)[..., None]
    inner = lib.blur(np.clip(clouds - lib.erode_mask(clouds, px(4)), 0, 1), 1.2 * S)
    cloud_col = lib.mix(cloud_col, cloud_col * np.array([0.86, 0.82, 0.90], np.float32), inner * 0.8)
    sky = lib.mix(sky, np.clip(cloud_col, 0, 1), clouds)
    cloud_edge = np.clip(lib.dilate_mask(clouds, px(2)) - clouds, 0, 1)
    sky = lib.mix(sky, lib.hexc('#2a1e2c'), cloud_edge * 0.85)

    # ---- composite on the paper, then the ink goes on last and stays crisp
    out = lib.comp(out, sky, alpha_b)
    out *= lib.paper(H, W, 61, fiber=0.04, mottle=0.035)[..., None]
    out *= np.array([1.0, 0.99, 0.965], np.float32)                       # cream sheet
    out = lib.ink(out, sc.lines_clean, lib.hexc('#1d1419'), 0.96)
    return np.clip(out, 0, 1).astype(np.float32)
