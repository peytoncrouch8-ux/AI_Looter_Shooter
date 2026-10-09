"""The ambient fauna's models (Docs/Polish/BorderlandsComparison.md, item 5: motion in the corner of the eye), as the small
static pieces Source/AI_Looter_Shooter/World/Fauna* draws as instances and moves in code: birds without skeletons (a
perched body with its head on a socket, a flying body with its wings on sockets, each piece turning about its own
pivot), insects, a tumbleweed, a dust devil's column and washing for the lines. Stylized realism in the game's own
textured look: every creature on one painted atlas, FaunaAtlas (Art/Textures/FaunaAtlas, 1024 px, painted by this
script on every run, deterministic), on M_World through MI_FaunaAtlas, so a whole flock is one draw per piece.

Plain birds, never Hob: the crows are glossy blue-black, smaller than him, both eyes dark and whole, no ash and no ember,
and they come in flocks; he is matte, ragged, one-eyed and alone (Art/Models/Creatures/Hob.py).

Models (SM_<name> in /Game/Art/Creatures; no Nanite, no collision; the front faces Blender -Y, Unreal +X):
  Crow_Perched      a crow on its perch with its wings folded, 45 cm with the tail, its feet gripping at the pivot;
                    SOCKET_Head at the neck. The head is a piece of its own so it can look about.
  Crow_Head         the head, beak and eyes, its pivot at the neck joint (on SOCKET_Head).
  Crow_Flying       the flying body (head, fanned tail), its pivot between the shoulders; SOCKET_WingL, SOCKET_WingR.
  Crow_WingInnerL/R the arm of each wing (coverts, secondaries), pivoting at the shoulder; SOCKET_Wrist.
  Crow_WingOuterL/R the hand of each wing with its fingered primaries, pivoting at the wrist. 93 cm across, spread.
  Sparrow_Perched, Sparrow_Head, Sparrow_Flying, Sparrow_WingL/R   the same pieces for a sparrow (one-piece wings), 15 cm.
  Swallow_Flying, Swallow_WingL/R   a swallow (it never perches in the game): forked tail, long swept wings.
  Hawk_Flying, Hawk_WingL/R   a soaring hawk, 1.3 m across, its wings fingered at the tips; painted pale underneath,
                    where the player sees it from.
  Butterfly_Body, ButterflyWing_Sulphur/Copper/White L/R   a butterfly's body and its wings in three looks, each wing
                    pivoting at the body's middle line; 8 cm across (a third bigger than life, to read at a distance).
  Dragonfly         body and four spread wings in one piece, 7 cm long, 9 cm across.
  Firefly           a firefly's light: a glowing bead (FireflyGlow, the old stylized surface with Glow; Nanite can't draw
                    the additive kind and an instanced emissive surface reads as a point of light).
  Fly               a housefly, a little bigger than life.
  Tumbleweed        a ball of tangled dry stems 76 cm across, its pivot at its middle (it rolls about it).
  DustDevil         a twisted column of dust cards 7.5 m tall flaring to 3.4 m, on the Smoke master (translucent, no glow
                    of its own; vertex alpha its opacity), its pivot at its foot.
  WindCloth_Sheet, WindCloth_Shirt, WindCloth_Towel   washing for a line, hanging from its pivot (the middle of its top
                    edge, on the line), on the house trim sheet's plaster and paint strips like the laundry line's own.

Triangles: each is logged as it's built (birds 300 to 1,000 a piece).

    blender -b --factory-startup --python Art/Models/Creatures/AmbientFauna.py -- --preview
"""
import math
import os
import random

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_creatures as lc
import looter_model as lm
import looter_props as lp
import looter_textures as lt

SET = 'FaunaAtlas'
SIZE = 1024
PX_M, NORMAL_STRENGTH = 0.0015, 0.8      # the atlas's metres per pixel (for the normal map) and the normals' strength

# Where each part lies in the atlas (u0, u1, v0, v1). Bodies and heads are latitude-longitude (U round, 0.5 the back; V
# along, 0 the rear); wings and tails planar (U out along the span or across the tail, V back across the chord or
# along the tail); the small solid parts are cells of one colour.
ATLAS = {
    'crow_body': (0.0, 0.25, 0.75, 1.0), 'crow_wing': (0.25, 0.5, 0.75, 1.0), 'crow_head': (0.5, 0.625, 0.875, 1.0),
    'crow_tail': (0.5, 0.625, 0.75, 0.875),
    'beak_dark': (0.625, 0.6875, 0.9375, 1.0), 'eye_dark': (0.6875, 0.75, 0.9375, 1.0),
    'leg_dark': (0.625, 0.6875, 0.875, 0.9375), 'beak_horn': (0.6875, 0.75, 0.875, 0.9375),
    'leg_pale': (0.625, 0.6875, 0.8125, 0.875), 'cere_yellow': (0.6875, 0.75, 0.8125, 0.875),
    'eye_amber': (0.625, 0.6875, 0.75, 0.8125), 'bfly_body': (0.6875, 0.75, 0.75, 0.8125),
    'fly_dark': (0.75, 0.8125, 0.9375, 1.0),
    'sparrow_body': (0.0, 0.25, 0.5, 0.75), 'sparrow_wing': (0.25, 0.5, 0.5, 0.75), 'sparrow_head': (0.5, 0.625, 0.625, 0.75),
    'sparrow_tail': (0.5, 0.625, 0.5, 0.625), 'swallow_body': (0.625, 0.875, 0.5, 0.75), 'swallow_wing': (0.875, 1.0, 0.5, 0.75),
    'swallow_head': (0.875, 1.0, 0.375, 0.5),
    'hawk_body': (0.0, 0.25, 0.25, 0.5), 'hawk_wing': (0.25, 0.625, 0.25, 0.5), 'hawk_tail': (0.625, 0.75, 0.25, 0.5),
    'hawk_head': (0.75, 0.875, 0.375, 0.5), 'hawk_under': (0.25, 0.625, 0.125, 0.25),
    'bfly_sulphur': (0.0, 0.125, 0.125, 0.25), 'bfly_copper': (0.125, 0.25, 0.125, 0.25), 'bfly_white': (0.625, 0.75, 0.125, 0.25),
    'dfly_body': (0.0, 0.25, 0.0, 0.0625), 'dfly_wing': (0.25, 0.375, 0.0, 0.125), 'dfly_thorax': (0.375, 0.4375, 0.0, 0.0625),
    'dfly_eyes': (0.4375, 0.5, 0.0, 0.0625),
}


class InsetMesh(lc.Mesh):
    """looter_creatures' part, its UVs kept a little inside each atlas region, so filtering and the smaller mips never
    pull in the colour of the region next door (a crow's wing edged with a sparrow's brown)."""

    def at(self, region, u, v):
        return super().at(region, 0.012 + 0.976 * min(max(u, 0.0), 1.0), 0.02 + 0.96 * min(max(v, 0.0), 1.0))

# The solid cells: colour and roughness.
SOLIDS = {
    'beak_dark': (0x202022, 0.4), 'eye_dark': (0x0e0c0b, 0.08), 'leg_dark': (0x27262b, 0.6), 'beak_horn': (0x5d5047, 0.5),
    'leg_pale': (0x9a7d69, 0.65), 'cere_yellow': (0xd2ad40, 0.5), 'eye_amber': (0x8a5a1c, 0.1), 'bfly_body': (0x2b2723, 0.8),
    'fly_dark': (0x1e1e21, 0.45),
}


# --- The atlas ---

def frac(x):
    return x - np.floor(x)


def feather_rows(U, V, rows, cols, offset=0.5):
    """Scalloped feather tips in rows along V: 1 at each feather's light edge, 0 in its middle."""
    row = V * rows
    shift = np.floor(row) % 2 * offset
    col = frac(U * cols + shift)
    tip = np.abs(col - 0.5) * 2.0                          # 0 mid-feather, 1 at its sides
    edge = frac(row) - 0.35 * (1.0 - tip ** 2)             # the rounded tip of each feather
    return lt.smooth(0.55, 0.9, edge)


def flight_feathers(U, V, count):
    """Long feathers side by side across U: 1 on the dark lines between them, and each feather's own tone (-1..1)."""
    col = U * count
    line = 1.0 - lt.smooth(0.0, 0.08, np.minimum(frac(col), 1.0 - frac(col)))
    tone = np.sin(np.floor(col) * 12.9898) * 0.5
    return line, tone


def solid(region, shape):
    color, rough = SOLIDS[region]
    col = np.broadcast_to(lt.rgb(color), shape + (3,)).copy()
    return col, np.full(shape, rough, np.float32), np.zeros(shape, np.float32), np.ones(shape, np.float32)


def paint(region, U, V, seed):
    """Colour, roughness, height (metres) and occlusion for one atlas region."""
    shape = U.shape
    if region in SOLIDS:
        return solid(region, shape)
    n = lt.noise(shape, seed + 1, 6.0, 6.0, octaves=2)
    rough = np.full(shape, 0.72, np.float32)
    occl = np.ones(shape, np.float32)
    height = 0.0002 * n

    if region in ('crow_body', 'crow_head'):
        # Glossy blue-black: a blue sheen on the back and shoulders, the feathers' scalloped edges a shade lighter.
        d, s = lc.body_axes(U, V)
        scallop = feather_rows(U, V, 22 if region == 'crow_body' else 16, 26 if region == 'crow_body' else 18)
        col = lt.mix(lt.rgb(0x16171c), lt.rgb(0x2a3350), lt.smooth(0.0, 0.9, d) * 0.45)
        col = lt.mix(col, lt.rgb(0x26282f), scallop * 0.5)
        col = col * (1.0 + 0.06 * n)[..., None]
        rough = 0.34 + 0.16 * lt.smooth(0.2, -0.6, d) + 0.05 * scallop
        height = 0.0003 * scallop + 0.0001 * n
        occl = 1.0 - 0.12 * lt.smooth(-0.3, -0.9, d)
        return col, rough, height, occl
    if region in ('crow_wing', 'crow_tail'):
        line, tone = flight_feathers(U, V, 12 if region == 'crow_wing' else 12)
        col = lt.rgb(0x141519) * (1.0 + 0.08 * tone + 0.04 * n)[..., None]
        if region == 'crow_wing':
            coverts = lt.smooth(0.36, 0.28, V)               # the coverts in rows over the leading edge
            col = lt.mix(col, lt.rgb(0x2b2f47), coverts * 0.35)
            col = lt.mix(col, lt.rgb(0x22242b), feather_rows(U, V, 14, 20) * coverts * 0.5)
            line = line * (1.0 - coverts)
        col = lt.mix(col, lt.rgb(0x0b0b0e), line * 0.7)
        rough = 0.36 + 0.1 * line
        height = 0.0003 * (1.0 - line)
        return col, rough, height, occl

    if region in ('sparrow_body', 'sparrow_head'):
        d, s = lc.body_axes(U, V)
        streak = lt.smooth(0.5, 1.1, lt.noise(shape, seed + 7, 1.5, 9.0))   # dark streaks running along the back
        back = lt.mix(lt.rgb(0x8b6a47), lt.rgb(0x3b2a1c), streak * 0.8)
        belly = lt.mix(lt.rgb(0xc9bfaa), lt.rgb(0xa99172), lt.smooth(-0.6, 0.0, d))
        col = lt.mix(belly, back, lt.smooth(-0.1, 0.35, d))
        if region == 'sparrow_head':
            crown = lt.smooth(0.35, 0.7, d)
            col = lt.mix(col, lt.rgb(0x7b5235), crown)
            cheek = lt.smooth(0.45, 0.75, np.abs(s)) * lt.smooth(0.55, 0.25, d) * lt.smooth(0.35, 0.55, V)
            col = lt.mix(col, lt.rgb(0xc3bdb1), cheek)
            stripe = (1.0 - lt.smooth(0.02, 0.06, np.abs(V - 0.66))) * lt.smooth(0.3, 0.6, np.abs(s)) * (d > -0.3)
            col = lt.mix(col, lt.rgb(0x2e241d), stripe * 0.8)
            bib = lt.smooth(-0.25, -0.6, d) * lt.smooth(0.55, 0.8, V)
            col = lt.mix(col, lt.rgb(0x252020), bib)
        col = col * (1.0 + 0.06 * n)[..., None]
        height = 0.0003 * feather_rows(U, V, 18, 20) + 0.0001 * n
        occl = 1.0 - 0.1 * lt.smooth(-0.3, -0.9, d)
        return col, np.full(shape, 0.76, np.float32), height, occl
    if region in ('sparrow_wing', 'sparrow_tail'):
        count = 10 if region == 'sparrow_wing' else 8
        line, tone = flight_feathers(U, V, count)
        # Each feather dark down its middle, buff at its edges.
        edge = np.abs(frac(U * count) - 0.5) * 2.0
        col = lt.mix(lt.rgb(0x3c2a1b), lt.rgb(0xc9a878), lt.smooth(0.55, 0.95, edge))
        col = lt.mix(col, lt.rgb(0x7b5838), 0.45)
        if region == 'sparrow_wing':
            bar = 1.0 - lt.smooth(0.015, 0.035, np.abs(V - 0.29))
            col = lt.mix(col, lt.rgb(0xe6ddcc), bar)
            coverts = lt.smooth(0.27, 0.2, V)
            col = lt.mix(col, lt.rgb(0x8d6440), coverts * 0.7)
        col = lt.mix(col, lt.rgb(0x2a1d13), line * 0.5) * (1.0 + 0.05 * tone + 0.04 * n)[..., None]
        return col, np.full(shape, 0.76, np.float32), 0.0003 * (1.0 - line), occl

    if region in ('swallow_body', 'swallow_head'):
        d, s = lc.body_axes(U, V)
        col = lt.mix(lt.rgb(0xe7dcc6), lt.rgb(0x1d2b4d), lt.smooth(-0.15, 0.2, d))
        throat = lt.smooth(0.78, 0.9, V) * lt.smooth(0.3, -0.2, d)
        col = lt.mix(col, lt.rgb(0x9a4a2b), throat)
        if region == 'swallow_head':
            col = lt.mix(col, lt.rgb(0x9a4a2b), lt.smooth(0.88, 0.97, V) * lt.smooth(-0.2, 0.3, d))
        else:
            band = (1.0 - lt.smooth(0.02, 0.05, np.abs(V - 0.76))) * lt.smooth(0.2, -0.3, d)
            col = lt.mix(col, lt.rgb(0x1d2b4d), band * 0.8)
        rough = 0.36 + 0.3 * lt.smooth(0.0, -0.4, d)
        return col * (1.0 + 0.05 * n)[..., None], rough, 0.0002 * feather_rows(U, V, 18, 18), occl
    if region == 'swallow_wing':
        line, tone = flight_feathers(U, V, 11)
        col = lt.mix(lt.rgb(0x1a2034), lt.rgb(0x0d1019), line * 0.6) * (1.0 + 0.06 * tone)[..., None]
        return col, 0.38 + 0.1 * line, 0.0002 * (1.0 - line), occl

    if region in ('hawk_body', 'hawk_head'):
        d, s = lc.body_axes(U, V)
        mottle = lt.smooth(0.0, 1.0, lt.noise(shape, seed + 9, 3.0, 5.0))
        back = lt.mix(lt.rgb(0x6c4a2e), lt.rgb(0x9c7b56), mottle * 0.6)
        belly = lt.rgb(0xe5d8bf) * np.ones(shape + (3,), np.float32)
        if region == 'hawk_body':
            band = lt.smooth(0.12, 0.05, np.abs(V - 0.45)) * lt.smooth(0.5, 1.2, lt.noise(shape, seed + 11, 1.6, 4.0))
            belly = lt.mix(belly, lt.rgb(0x5a3d26), band * 0.9)
        col = lt.mix(belly, back, lt.smooth(-0.25, 0.25, d))
        if region == 'hawk_head':
            throat = lt.smooth(0.6, 0.85, V) * lt.smooth(0.1, -0.4, d)
            col = lt.mix(col, lt.rgb(0xece2cd), throat)
            malar = (1.0 - lt.smooth(0.03, 0.07, np.abs(V - 0.7))) * lt.smooth(0.4, 0.7, np.abs(s)) * lt.smooth(0.3, -0.2, d)
            col = lt.mix(col, lt.rgb(0x4a3220), malar * 0.7)
        col = col * (1.0 + 0.05 * n)[..., None]
        return col, np.full(shape, 0.74, np.float32), 0.0003 * feather_rows(U, V, 20, 22), occl
    if region == 'hawk_wing':
        line, tone = flight_feathers(U, V, 16)
        col = lt.mix(lt.rgb(0x6e4c30), lt.rgb(0x9a7a55), feather_rows(U, V, 10, 16) * lt.smooth(0.42, 0.3, V) * 0.6)
        flight = lt.smooth(0.38, 0.46, V)
        bars = 1.0 - lt.smooth(0.01, 0.025, np.abs(frac(V * 9.0) - 0.5) - 0.42)
        col = lt.mix(col, lt.mix(lt.rgb(0x4e3624), lt.rgb(0x33241a), bars * 0.6), flight)
        col = lt.mix(col, lt.rgb(0x2c1f17), lt.smooth(0.84, 0.92, U))
        col = lt.mix(col, lt.rgb(0x22180f), line * flight * 0.6) * (1.0 + 0.06 * tone + 0.04 * n)[..., None]
        return col, np.full(shape, 0.74, np.float32), 0.0003 * (1.0 - line), occl
    if region == 'hawk_under':
        # Seen from below, where the player sees a soaring hawk from: pale, barred flight feathers, the dark bar along the
        # leading edge, a comma at the wrist, dark fingertips and trailing edge.
        line, tone = flight_feathers(U, V, 16)
        col = lt.rgb(0xe8dcc4) * np.ones(shape + (3,), np.float32)
        flight = lt.smooth(0.4, 0.5, V)
        col = lt.mix(col, lt.rgb(0xd9d2c6), flight)
        bars = 1.0 - lt.smooth(0.006, 0.016, np.abs(frac(V * 11.0) - 0.5) - 0.45)
        col = lt.mix(col, lt.rgb(0x7d6a58), bars * flight * 0.55)
        patagial = lt.smooth(0.14, 0.06, V) * lt.smooth(0.55, 0.4, U)
        col = lt.mix(col, lt.rgb(0x4a3320), patagial * 0.85)
        comma = 1.0 - lt.smooth(0.03, 0.07, np.hypot((U - 0.58) * 1.6, V - 0.22))
        col = lt.mix(col, lt.rgb(0x3f2c1d), comma)
        col = lt.mix(col, lt.rgb(0x5d4a3a), lt.smooth(0.9, 0.97, V) * 0.8)
        col = lt.mix(col, lt.rgb(0x2a2018), lt.smooth(0.86, 0.94, U) * flight)
        col = lt.mix(col, lt.rgb(0xb8aa96), line * flight * 0.5) * (1.0 + 0.04 * n)[..., None]
        return col, np.full(shape, 0.76, np.float32), 0.0002 * (1.0 - line), occl
    if region == 'hawk_tail':
        line, tone = flight_feathers(U, V, 12)
        col = lt.mix(lt.rgb(0xb5683e), lt.rgb(0xc98a5c), lt.smooth(0.0, 0.6, V) * 0.4)
        col = lt.mix(col, lt.rgb(0x3d2618), 1.0 - lt.smooth(0.02, 0.05, np.abs(V - 0.86)))
        col = lt.mix(col, lt.rgb(0xe9dcc6), lt.smooth(0.93, 0.98, V))
        col = lt.mix(col, lt.rgb(0x7a4426), line * 0.5) * (1.0 + 0.05 * tone + 0.04 * n)[..., None]
        return col, np.full(shape, 0.72, np.float32), 0.0002 * (1.0 - line), occl

    if region.startswith('bfly_'):
        # U out from the root (0) to the tip (1), V from the front edge (0) to the back (1): the forewing over V < 0.55.
        root = np.hypot(U * 1.2, V - 0.45)
        angle = np.arctan2(V - 0.45, U * 1.2 + 1e-4)
        veins = 1.0 - lt.smooth(0.0, 0.035, np.abs(frac(angle * 2.2) - 0.5) - 0.45)
        veins = veins * lt.smooth(0.05, 0.2, root)
        fore = lt.smooth(0.58, 0.5, V)
        outer = lt.smooth(0.7, 0.9, root)
        if region == 'bfly_sulphur':
            col = lt.mix(lt.rgb(0xe8d877), lt.rgb(0xd5cf7c), 1.0 - fore)
            col = lt.mix(col, lt.rgb(0x3a3125), lt.smooth(0.8, 0.92, U) * fore * lt.smooth(0.55, 0.2, V))
            spot = 1.0 - lt.smooth(0.03, 0.05, np.hypot(U - 0.5, V - 0.3))
            col = lt.mix(col, lt.rgb(0x3a3125), spot)
            col = lt.mix(col, lt.rgb(0xa79a52), veins * 0.35)
        elif region == 'bfly_copper':
            col = lt.mix(lt.rgb(0xa85c2c), lt.rgb(0x8f4c27), 1.0 - fore)
            border = lt.smooth(0.62, 0.78, root)
            col = lt.mix(col, lt.rgb(0x2a1f19), border)
            dots = 1.0 - lt.smooth(0.015, 0.03, np.abs(frac(angle * 3.0) - 0.5) * 0.12 + np.abs(root - 0.86) * 0.5)
            col = lt.mix(col, lt.rgb(0xe6d6b4), dots * border)
            for cu, cv in ((0.45, 0.25), (0.62, 0.38), (0.4, 0.7)):
                col = lt.mix(col, lt.rgb(0x2a1f19), 1.0 - lt.smooth(0.025, 0.045, np.hypot(U - cu, V - cv)))
            col = lt.mix(col, lt.rgb(0x5a2f17), veins * 0.4)
        else:
            col = lt.rgb(0xeeeae0) * np.ones(shape + (3,), np.float32)
            col = lt.mix(col, lt.rgb(0x3b3b3b), lt.smooth(0.74, 0.86, U) * lt.smooth(0.45, 0.2, V))
            col = lt.mix(col, lt.rgb(0x2f2f2f), 1.0 - lt.smooth(0.03, 0.05, np.hypot(U - 0.56, V - 0.44)))
            col = lt.mix(col, lt.rgb(0xc9c4b6), veins * 0.3)
        col = lt.mix(col, col * 0.75, outer * 0.3) * (1.0 + 0.04 * n)[..., None]
        return col, np.full(shape, 0.7, np.float32), 0.0001 * veins, occl
    if region == 'dfly_body':
        # U along it: the abdomen (pale powdery blue, ringed, its tip dark), the thorax (olive, pale stripes), the eyes.
        col = lt.rgb(0x7393b5) * np.ones(shape + (3,), np.float32)
        rings = 1.0 - lt.smooth(0.004, 0.01, np.abs(frac(U * 14.0) - 0.5) - 0.44)
        col = lt.mix(col, lt.rgb(0x23262c), rings * lt.smooth(0.64, 0.6, U) * 0.8)
        col = lt.mix(col, lt.rgb(0x1f2228), lt.smooth(0.1, 0.04, U))
        thorax = lt.smooth(0.6, 0.64, U) * lt.smooth(0.88, 0.84, U)
        stripes = 1.0 - lt.smooth(0.02, 0.05, np.abs(frac(V * 4.0) - 0.5) - 0.3)
        col = lt.mix(col, lt.mix(lt.rgb(0x5a5a3c), lt.rgb(0xb7b38a), stripes * 0.6), thorax)
        col = lt.mix(col, lt.rgb(0x2e3c4f), lt.smooth(0.85, 0.89, U))
        rough = np.where(U > 0.86, 0.18, 0.55).astype(np.float32)
        return col * (1.0 + 0.04 * n)[..., None], rough, 0.0001 * rings, occl
    if region == 'dfly_thorax':
        # Latitude-longitude: olive with two pale stripes down each side.
        d, s = lc.body_axes(U, V)
        stripes = 1.0 - lt.smooth(0.04, 0.09, np.abs(np.abs(s) - 0.55))
        col = lt.mix(lt.rgb(0x5a5a3c), lt.rgb(0xb7b38a), stripes * 0.7)
        col = lt.mix(col, lt.rgb(0x2f3024), lt.smooth(0.2, 0.9, d) * 0.4)
        return col * (1.0 + 0.04 * n)[..., None], np.full(shape, 0.5, np.float32), 0.0001 * stripes, occl
    if region == 'dfly_eyes':
        d, s = lc.body_axes(U, V)
        col = lt.mix(lt.rgb(0x22303f), lt.rgb(0x3d5269), lt.smooth(-0.5, 0.8, d) * 0.6)
        return col, np.full(shape, 0.15, np.float32), np.zeros(shape, np.float32), occl
    if region == 'dfly_wing':
        col = lt.rgb(0xd8dee2) * np.ones(shape + (3,), np.float32)
        grid = np.maximum(1.0 - lt.smooth(0.0, 0.04, np.abs(frac(U * 18.0) - 0.5) - 0.44),
                          1.0 - lt.smooth(0.0, 0.05, np.abs(frac(V * 7.0) - 0.5) - 0.42))
        col = lt.mix(col, lt.rgb(0x8b9298), grid * 0.5)
        col = lt.mix(col, lt.rgb(0xc9a46a), lt.smooth(0.1, 0.03, U) * 0.6)
        stigma = lt.smooth(0.8, 0.83, U) * lt.smooth(0.91, 0.88, U) * lt.smooth(0.22, 0.15, V)
        col = lt.mix(col, lt.rgb(0x2d2520), stigma)
        return col, np.full(shape, 0.18, np.float32), np.zeros(shape, np.float32), occl
    raise ValueError(f'no painter for {region}')


texture_dir = os.path.join(lt.TEXTURE_DIR, SET)
os.makedirs(texture_dir, exist_ok=True)
lc.paint_atlas(ATLAS, paint, SIZE, PX_M, texture_dir, SET, normal_strength=NORMAL_STRENGTH)
ATLAS_MAT = lt.material(SET)
GLOW = lm.material('FireflyGlow', 0xd9e66a, Glow=8.0, Variation=0.02)
# Dry stems bleached pale grey-tan (the neutral polymer tinted: the hay set is too golden for a dead weed).
TUMBLE = lt.material('Polymer', name='Tumbleweed', tint=0xb9a888)
# Washing: plain cloth (the trim sheet's plaster strip is cracked like a wall), linen, a faded blue shirt, a red towel.
LINEN = lt.material('Polymer', name='ClothLinen', tint=0xe4ddcc)
SHIRT = lt.material('Polymer', name='ClothBlue', tint=0x7d8fa0)
TOWEL = lt.material('Polymer', name='ClothRed', tint=0xa65a4b)


def smoke_material(name, color, speed, opacity):
    """A material for the Smoke master (translucent, scrolling, vertex alpha the opacity): its colour, the noise's rise
    speed and the opacity as the master's parameters. The nodes only preview it."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    col = nodes.new('ShaderNodeVertexColor')
    col.layer_name = 'Col'
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    links.new(col.outputs['Alpha'], bsdf.inputs['Alpha'])
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    mat['Master'] = 'Smoke'
    mat['SmokeColor'] = '#{:06X}'.format(color)
    mat['Speed'] = float(speed)
    mat['Opacity'] = float(opacity)
    return mat


DUST = smoke_material('DustDevil', 0xc4a880, 0.45, 0.32)


# --- Parts in creature space (cm, x forward, y left, z up; looter_creatures turns them into Blender's metres) ---

def oriented(m, verts, uvs, want):
    """A face from verts (with uvs), its winding turned so its normal points along want."""
    a, b, c = (v.co for v in verts[:3])
    if (b - a).cross(c - b).dot(Vector(want)) < 0.0:
        verts, uvs = list(reversed(verts)), list(reversed(uvs))
    return m.face(verts, uvs)


def panel(m, region, stations, side, thickness=0.8, serrate=0.0, u_range=(0.0, 1.0), under=None, origin=(0.0, 0.0, 0.0)):
    """A wing as a thin two-sided panel out along the side's span (side 1: out to +y, the bird's left; -1: its right).
    stations: [(s, lead, trail, z)], s how far out (cm), the leading and trailing edges' x and the height. U runs along
    the span (u_range), V back across the chord (0 the leading edge, 0.9 the trailing edge, 1 a feather's tip: serrate
    cm of a point between each pair of stations); the top swells by thickness at the leading edge and thins to nothing
    behind. under: the underside's region (default the same)."""
    ox, oy, oz = origin
    span = max(st[0] for st in stations) or 1.0
    chords = (0.0, 0.3, 1.0)
    top, bottom = [], []
    for s, lead, trail, z in stations:
        rt, rb = [], []
        for j in chords:
            x = lead + (trail - lead) * j
            t = thickness * (1.0 - j) ** 1.5
            rt.append(m.bm.verts.new((ox + x, oy + side * s, oz + z + 0.6 * t + 0.02)))
            rb.append(m.bm.verts.new((ox + x, oy + side * s, oz + z - 0.4 * t - 0.02)))
        top.append(rt)
        bottom.append(rb)
    low = under or region

    def uv(reg, s, j, du=0.0, dv=0.0):
        # du, dv: a sliver's offset, so the faces closing the panel's edges have some UV area (tangents need it).
        return m.at(reg, u_range[0] + (u_range[1] - u_range[0]) * s / span + du, 0.9 * chords[j] + dv)
    for i in range(len(stations) - 1):
        s0, s1 = stations[i][0], stations[i + 1][0]
        for k in range(len(chords) - 1):
            oriented(m, [top[i][k], top[i + 1][k], top[i + 1][k + 1], top[i][k + 1]],
                     [uv(region, s0, k), uv(region, s1, k), uv(region, s1, k + 1), uv(region, s0, k + 1)], (0, 0, 1))
            oriented(m, [bottom[i][k], bottom[i + 1][k], bottom[i + 1][k + 1], bottom[i][k + 1]],
                     [uv(low, s0, k), uv(low, s1, k), uv(low, s1, k + 1), uv(low, s0, k + 1)], (0, 0, -1))
        # The leading edge, closed between top and bottom.
        oriented(m, [top[i][0], top[i + 1][0], bottom[i + 1][0], bottom[i][0]],
                 [uv(region, s0, 0), uv(region, s1, 0), uv(low, s1, 0, dv=0.03), uv(low, s0, 0, dv=0.03)], (1, 0, 0))
        if serrate > 0.0:
            # A feather's tip behind the trailing edge between the two stations.
            mid = (top[i][-1].co + top[i + 1][-1].co) * 0.5 + Vector((-serrate, 0.0, 0.0))
            tip_t = m.bm.verts.new(mid + Vector((0.0, 0.0, 0.01)))
            tip_b = m.bm.verts.new(mid - Vector((0.0, 0.0, 0.01)))
            u_mid = u_range[0] + (u_range[1] - u_range[0]) * (s0 + s1) * 0.5 / span
            oriented(m, [top[i][-1], top[i + 1][-1], tip_t], [uv(region, s0, 2), uv(region, s1, 2), m.at(region, u_mid, 1.0)], (0, 0, 1))
            oriented(m, [bottom[i][-1], bottom[i + 1][-1], tip_b], [uv(low, s0, 2), uv(low, s1, 2), m.at(low, u_mid, 1.0)], (0, 0, -1))
    # The tip, closed.
    s_end = stations[-1][0]
    for k in range(len(chords) - 1):
        oriented(m, [top[-1][k], top[-1][k + 1], bottom[-1][k + 1], bottom[-1][k]],
                 [uv(region, s_end, k), uv(region, s_end, k + 1), uv(low, s_end, k + 1, du=-0.02), uv(low, s_end, k, du=-0.02)],
                 (0, side, 0))
    return top, bottom


def fingers(m, region, root_s, lead, trail, z, side, lengths, width, splay, thickness=0.4, u_range=(0.8, 1.0), under=None,
            origin=(0.0, 0.0, 0.0)):
    """Separate primaries at a wing's tip: one narrow panel each, from root_s (between lead and trail, front to back)
    out by its length, splayed back a little more each, narrowing to a rounded point."""
    count = len(lengths)
    for k, length in enumerate(lengths):
        x0 = lead + (trail - lead) * (k + 0.5) / count
        sweep = -splay * k
        stations = [(0.0, x0 + width * 0.5, x0 - width * 0.5, z),
                    (length * 0.6, x0 + sweep * 0.6 + width * 0.45, x0 + sweep * 0.6 - width * 0.45, z + 0.3),
                    (length, x0 + sweep + width * 0.12, x0 + sweep - width * 0.12, z + 0.6)]
        ox, oy, oz = origin
        panel(m, region, stations, side, thickness, 0.0, u_range, under, origin=(ox, oy + side * root_s, oz))


def fan(m, region, root_x, tip_x, root_w, tip_w, z0, z1, notch=0.0, thickness=0.3, steps=3):
    """A tail: a thin two-sided fan back from root_x to tip_x (cm), widening from root_w to tip_w (half widths), tilting
    from z0 to z1; its end rounded, or forked by notch (cm). U across it, V along it (0 at the root)."""
    rows = []
    for i in range(steps + 1):
        t = i / steps
        x = root_x + (tip_x - root_x) * t
        w = root_w + (tip_w - root_w) * t
        z = z0 + (z1 - z0) * t
        row = []
        for a in (-1.0, -0.5, 0.0, 0.5, 1.0):
            end = 0.0
            if i == steps:
                end = (0.6 * w * 0.25 * (1.0 - a * a)) if notch <= 0.0 else (-notch * (1.0 - abs(a)))
            row.append((x - end, a * w, z, (a + 1.0) * 0.5, t))
        rows.append(row)
    for layer, want, dz in ((0, (0, 0, 1), thickness * 0.5), (1, (0, 0, -1), -thickness * 0.5)):
        verts = [[m.bm.verts.new((p[0], p[1], p[2] + dz)) for p in row] for row in rows]
        for i in range(steps):
            for k in range(4):
                quad = [verts[i][k], verts[i][k + 1], verts[i + 1][k + 1], verts[i + 1][k]]
                uvs = [m.at(region, rows[i][k][3], rows[i][k][4]), m.at(region, rows[i][k + 1][3], rows[i][k + 1][4]),
                       m.at(region, rows[i + 1][k + 1][3], rows[i + 1][k + 1][4]), m.at(region, rows[i + 1][k][3], rows[i + 1][k][4])]
                oriented(m, quad, uvs, want)


def eyes(m, center, radii, region='eye_dark'):
    """A pair of eyes at (x, +-y, z): flat lenses facing out a little forward of sideways (radii: how far each stands out,
    then its length and height on the head)."""
    x, y, z = center
    for side in (1.0, -1.0):
        lc.ellipsoid(m, (x, side * y, z), radii, region, fwd=(0.3, side, 0.0), segs=8, rings=5)


def beak(m, points, radii, region='beak_dark', sides=7):
    lc.tube(m, points, radii, region, sides=sides)


def leg(m, hip, foot, radius, toe_length, region, side):
    """A thin leg straight down to a foot gripping the perch: three toes forward, one back."""
    lc.tube(m, [hip, foot], [radius, radius * 0.9], region, sides=5)
    fx, fy, fz = foot
    for spread in (-0.35, 0.0, 0.35):
        lc.tube(m, [(fx, fy, fz), (fx + toe_length, fy + side * spread * toe_length, 0.05)], [radius * 0.7, radius * 0.45], region, sides=4)
    lc.tube(m, [(fx, fy, fz), (fx - toe_length * 0.6, fy, 0.05)], [radius * 0.7, radius * 0.45], region, sides=4)


def tilt(degrees):
    return (math.cos(math.radians(degrees)), 0.0, math.sin(math.radians(degrees)))


def tapering(rear_share):
    """A deform for ellipsoid(): narrowing toward its +fwd end (a folded wing's tip)."""
    def deform(a, s, u):
        k = 1.0 - rear_share * max(0.0, min(1.0, (a + 0.2) / 1.2)) ** 1.5
        return a, s * k, u * k
    return deform


def finish(m, name, material=None, sharp=70.0):
    """The part as a model: no Nanite, no collision, triangles logged."""
    obj = m.finish(name, material or ATLAS_MAT, sharp=sharp)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    lt._log(f'{name}: {sum(len(p.vertices) - 2 for p in obj.data.polygons)} triangles')
    return obj


def socket(obj, name, point):
    """A socket at a creature-space point (cm)."""
    return lm.socket(obj, name, tuple(lc.to_blender(point)))


# --- Birds ---

def perched_bird(name, region_body, region_wing, region_tail, leg_region, size, neck_top, plump=1.0):
    """A perched bird with its wings folded (sizes in a crow's centimetres, scaled by size; plump widens and deepens the
    body for a sparrow's round build): body, neck, wings, tail and legs; SOCKET_Head where the head piece sits."""
    k = size
    m = InsetMesh(ATLAS)
    # A longish body leaning forward a little, the breast full, the back flatter.
    lc.ellipsoid(m, (0.0, 0.0, 12.0 * k), (14.0 * k, 6.6 * k * plump, 7.2 * k * plump), region_body, fwd=tilt(20.0), segs=16, rings=10,
                 deform=lambda a, s, u: (a, s * (1.0 - 0.12 * max(0.0, -a)), u * (1.06 if u < 0.0 else 0.93)))
    # The neck: an ellipsoid on the same atlas cell (a tube's mapping would not match the body's), rising to the head.
    nx, nz = neck_top
    lc.ellipsoid(m, ((nx - 1.2) * k, 0.0, (nz - 3.0) * k), (5.0 * k, 3.7 * k, 4.4 * k), region_body, fwd=tilt(55.0), segs=12, rings=8)
    for side in (1.0, -1.0):
        # Folded wings lying along the body's sides and over its back, their tips crossing over the tail.
        back = Vector((-0.9, -side * 0.07, -0.36)).normalized()
        lc.ellipsoid(m, (-4.5 * k, side * (5.0 * plump - 0.2) * k, 12.6 * k), (15.5 * k, 1.5 * k, 4.8 * k * plump), region_wing,
                     fwd=tuple(back), up=(0.0, side * 0.35, 1.0), segs=12, rings=8, deform=tapering(0.75))
        leg(m, (1.5 * k, side * 2.4 * k, 7.0 * k), (1.0 * k, side * 2.4 * k, 0.6 * k), 0.55 * k, 3.2 * k, leg_region, side)
    fan(m, region_tail, -11.0 * k, -28.0 * k, 2.0 * k, 3.8 * k, 9.0 * k, 4.0 * k, thickness=0.6 * k)
    obj = finish(m, name)
    lt.bake_vertex_ao(obj, samples=12, distance=0.12 * k, ground=False)
    socket(obj, 'Head', (nx * k, 0.0, nz * k))
    return obj


def bird_head(name, region, size, beak_points, beak_radii, beak_region, eye_center, eye_radii):
    """A head on its neck joint (the pivot): skull, a neck stub that hides the joint, beak and eyes."""
    k = size
    m = InsetMesh(ATLAS)
    lc.ellipsoid(m, (2.8 * k, 0.0, 2.6 * k), (5.0 * k, 3.6 * k, 3.9 * k), region, fwd=(1.0, 0.0, 0.12), segs=12, rings=8,
                 deform=lambda a, s, u: (a, s, u * (0.9 if u > 0.0 else 1.0)))
    # The nape down into the body's neck, so the joint never shows as the head turns.
    lc.ellipsoid(m, (-0.4 * k, 0.0, 0.2 * k), (3.6 * k, 3.3 * k, 3.6 * k), region, fwd=tilt(-60.0), segs=10, rings=6)
    beak(m, [tuple(c * k for c in p) for p in beak_points], [r * k for r in beak_radii], beak_region)
    eyes(m, tuple(c * k for c in eye_center), tuple(r * k for r in eye_radii))
    return finish(m, name)


def flying_bird(name, region_body, region_head, region_tail, size, beak_points, beak_radii, beak_region, eye_center, eye_radii,
                eye_region, tail, shoulder, body_radii=(12.5, 6.4, 6.2), head=(14.0, 2.0), head_radii=(4.9, 3.5, 3.8),
                extra=None):
    """A flying body, head and tail (sizes in centimetres times size), sockets WingL and WingR at the shoulders."""
    k = size
    m = InsetMesh(ATLAS)
    bx, by, bz = body_radii
    lc.ellipsoid(m, (-1.0 * k, 0.0, 0.0), (bx * k, by * k, bz * k), region_body, segs=16, rings=10,
                 deform=lambda a, s, u: (a, s, u * (1.08 if u < 0.0 else 0.92)))
    hx, hz = head
    # The head overlaps the body's front (no neck of its own in flight: it's drawn in).
    lc.ellipsoid(m, (hx * k, 0.0, hz * k), tuple(r * k for r in head_radii), region_head, fwd=(1.0, 0.0, 0.08), segs=12, rings=8)
    beak(m, [tuple(c * k for c in p) for p in beak_points], [r * k for r in beak_radii], beak_region)
    eyes(m, tuple(c * k for c in eye_center), tuple(r * k for r in eye_radii), eye_region)
    root_x, tip_x, root_w, tip_w, notch = tail
    fan(m, region_tail, root_x * k, tip_x * k, root_w * k, tip_w * k, 0.4 * k, 0.9 * k, notch=notch * k, thickness=0.4 * k)
    if extra:
        extra(m, k)
    obj = finish(m, name)
    sx, sy, sz = shoulder
    socket(obj, 'WingL', (sx * k, sy * k, sz * k))
    socket(obj, 'WingR', (sx * k, -sy * k, sz * k))
    return obj


def wing(name, region, stations, side, thickness, serrate, size=1.0, u_range=(0.0, 1.0), wrist=None, finger_spec=None, under=None):
    """A wing (or an arm of one) pivoting at its root; with wrist (s, z), a Wrist socket there; with finger_spec,
    primaries splaying from its last station."""
    k = size
    m = InsetMesh(ATLAS)
    scaled = [(s * k, lead * k, trail * k, z * k) for s, lead, trail, z in stations]
    panel(m, region, scaled, side, thickness * k, serrate * k, u_range, under)
    if finger_spec:
        lengths, width, splay, u_fingers = finger_spec
        s, lead, trail, z = scaled[-1]
        fingers(m, region, s, lead, trail, z, side, [L * k for L in lengths], width * k, splay * k, thickness * 0.4 * k, u_fingers, under)
    obj = finish(m, name, sharp=60.0)
    if wrist:
        ws, wz = wrist
        socket(obj, 'Wrist', (0.0, side * ws * k, wz * k))
    return obj


def crow():
    perched_bird('Crow_Perched', 'crow_body', 'crow_wing', 'crow_tail', 'leg_dark', 1.0, (8.0, 20.5))
    bird_head('Crow_Head', 'crow_head', 1.0, [(6.6, 0.0, 2.5), (9.6, 0.0, 2.0), (12.8, 0.0, 1.0)], [1.55, 0.95, 0.1], 'beak_dark',
              (5.3, 2.95, 3.7), (0.35, 0.8, 0.75))
    flying_bird('Crow_Flying', 'crow_body', 'crow_head', 'crow_tail', 1.0, [(17.8, 0.0, 1.8), (21.0, 0.0, 1.4), (24.0, 0.0, 0.6)],
                [1.5, 0.9, 0.1], 'beak_dark', (16.4, 2.85, 3.0), (0.33, 0.75, 0.7), 'eye_dark', (-11.0, -28.0, 3.0, 9.0, 0.0),
                (2.5, 5.2, 2.6))
    arm = [(0.0, 4.0, -9.5, 0.0), (6.0, 5.2, -11.0, 0.4), (13.0, 5.0, -11.5, 0.6), (19.5, 3.8, -10.5, 0.5)]
    hand = [(0.0, 3.8, -10.5, 0.0), (5.0, 3.0, -9.0, -0.1), (10.0, 1.5, -7.5, -0.3)]
    for side, suffix in ((1.0, 'L'), (-1.0, 'R')):
        wing(f'Crow_WingInner{suffix}', 'crow_wing', arm, side, 0.8, 1.2, u_range=(0.0, 0.45), wrist=(19.5, 0.5))
        wing(f'Crow_WingOuter{suffix}', 'crow_wing', hand, side, 0.6, 0.0, u_range=(0.45, 0.7),
             finger_spec=((12.0, 13.5, 13.0, 11.5, 9.5), 2.2, 1.3, (0.7, 1.0)))


def sparrow():
    k = 0.45
    # Folded, a sparrow's wing shows its streaked brown back and buff underside: the body's cell paints it.
    perched_bird('Sparrow_Perched', 'sparrow_body', 'sparrow_body', 'sparrow_tail', 'leg_pale', k, (8.0, 20.0), plump=1.25)
    bird_head('Sparrow_Head', 'sparrow_head', 0.55, [(6.4, 0.0, 2.2), (8.2, 0.0, 1.9), (9.8, 0.0, 1.4)], [1.6, 1.0, 0.12], 'beak_horn',
              (5.2, 3.0, 3.4), (0.4, 0.8, 0.75))
    flying_bird('Sparrow_Flying', 'sparrow_body', 'sparrow_head', 'sparrow_tail', k, [(16.5, 0.0, 2.0), (18.6, 0.0, 1.6), (20.4, 0.0, 1.0)],
                [2.0, 1.2, 0.15], 'beak_horn', (14.7, 3.4, 3.0), (0.4, 0.95, 0.9), 'eye_dark', (-11.0, -25.0, 3.0, 6.5, 1.5),
                (2.6, 6.8, 3.0), body_radii=(12.4, 8.0, 8.0), head=(11.4, 2.0), head_radii=(5.6, 4.7, 4.9))
    stations = [(0.0, 2.4, -3.6, 0.0), (3.5, 2.8, -4.2, 0.15), (6.5, 2.2, -3.6, 0.15), (8.8, 0.6, -2.0, 0.0), (9.6, -0.8, -1.2, -0.1)]
    for side, suffix in ((1.0, 'L'), (-1.0, 'R')):
        wing(f'Sparrow_Wing{suffix}', 'sparrow_wing', stations, side, 0.5, 0.5)


def swallow():
    def streamers(m, k):
        # The fork's long outer feathers: each a strip half a centimetre wide, its length running back along x (the
        # panel's chord), from the fork's tips to 17 cm behind the bill.
        for side in (1.0, -1.0):
            panel(m, 'swallow_wing', [(0.0, -9.6 * k, -17.4 * k, 0.8 * k), (0.5 * k, -9.6 * k, -17.0 * k, 0.9 * k)], side, 0.2 * k,
                  origin=(0.0, side * 2.1 * k, 0.0))
    flying_bird('Swallow_Flying', 'swallow_body', 'swallow_head', 'swallow_wing', 1.0, [(7.4, 0.0, 0.7), (7.9, 0.0, 0.6), (8.4, 0.0, 0.5)],
                [0.5, 0.3, 0.05], 'beak_dark', (6.5, 1.55, 1.3), (0.18, 0.38, 0.36), 'eye_dark', (-5.5, -10.0, 1.2, 2.6, 3.5),
                (1.6, 2.3, 1.1), body_radii=(6.2, 2.7, 2.6), head=(5.3, 0.7), head_radii=(2.3, 2.0, 1.9), extra=streamers)
    stations = [(0.0, 2.0, -3.2, 0.0), (4.0, 2.2, -3.4, 0.2), (8.0, 1.0, -3.6, 0.2), (12.0, -2.0, -4.6, 0.0), (15.0, -5.0, -5.6, -0.2)]
    for side, suffix in ((1.0, 'L'), (-1.0, 'R')):
        wing(f'Swallow_Wing{suffix}', 'swallow_wing', stations, side, 0.4, 0.3)


def hawk():
    def cere(m, k):
        lc.ellipsoid(m, (24.6 * k, 0.0, 3.9 * k), (1.1 * k, 1.5 * k, 1.0 * k), 'cere_yellow', segs=8, rings=5)
    flying_bird('Hawk_Flying', 'hawk_body', 'hawk_head', 'hawk_tail', 1.0, [(24.5, 0.0, 3.4), (27.0, 0.0, 2.9), (28.3, 0.0, 1.2)],
                [1.9, 1.2, 0.15], 'beak_dark', (23.0, 3.5, 5.0), (0.4, 0.95, 0.9), 'eye_amber', (-18.0, -40.0, 4.5, 11.0, 0.0),
                (3.0, 8.0, 3.0), body_radii=(19.0, 9.5, 9.0), head=(19.5, 3.5), head_radii=(6.6, 5.2, 5.4), extra=cere)
    # Broad wings, the leading edge rounding forward then back to the hand, which tapers into five splayed fingers.
    stations = [(0.0, 10.0, -16.0, 0.0), (12.0, 12.5, -19.0, 1.5), (24.0, 11.0, -18.0, 2.5), (31.0, 8.5, -15.0, 2.5),
                (35.0, 6.0, -12.0, 2.3)]
    for side, suffix in ((1.0, 'L'), (-1.0, 'R')):
        wing(f'Hawk_Wing{suffix}', 'hawk_wing', stations, side, 1.6, 1.5, u_range=(0.0, 0.66),
             finger_spec=((16.0, 18.0, 17.0, 14.5, 11.0), 3.2, 2.0, (0.66, 1.0)), under='hawk_under')


# --- Insects ---

BUTTERFLY_OUTLINE = [(0.7, 0.25), (1.05, 1.6), (1.15, 2.9), (0.95, 3.75), (0.35, 3.6), (-0.15, 3.0), (-0.35, 2.6), (-0.75, 2.55),
                     (-1.45, 2.15), (-2.05, 1.5), (-2.2, 0.85), (-1.6, 0.3), (-0.6, 0.22)]


def butterfly():
    m = InsetMesh(ATLAS)
    lc.tube(m, [(-1.5, 0.0, 0.0), (-0.3, 0.0, 0.05), (0.9, 0.0, 0.12)], [0.18, 0.32, 0.3], 'bfly_body', sides=6)
    lc.ellipsoid(m, (1.15, 0.0, 0.15), (0.32, 0.32, 0.3), 'bfly_body', segs=8, rings=5)
    for side in (1.0, -1.0):
        lc.tube(m, [(1.3, side * 0.12, 0.3), (2.0, side * 0.5, 1.2), (2.3, side * 0.65, 1.7)], [0.045, 0.035, 0.08], 'bfly_body', sides=3)
    finish(m, 'Butterfly_Body')
    centre = (-0.3, 1.2)
    for look in ('Sulphur', 'Copper', 'White'):
        region = 'bfly_' + look.lower()
        for side, suffix in ((1.0, 'L'), (-1.0, 'R')):
            m = InsetMesh(ATLAS)

            def uv(x, s):
                return m.at(region, s / 3.8, (1.2 - x) / 3.4)
            for layer, want, dz in ((0, (0, 0, 1), 0.015), (1, (0, 0, -1), -0.015)):
                hub = m.bm.verts.new((centre[0], side * centre[1], dz))
                ring = [m.bm.verts.new((x, side * s, dz + 0.06 * s)) for x, s in BUTTERFLY_OUTLINE]
                for i in range(len(ring)):
                    j = (i + 1) % len(ring)
                    (x0, s0), (x1, s1) = BUTTERFLY_OUTLINE[i], BUTTERFLY_OUTLINE[j]
                    oriented(m, [hub, ring[i], ring[j]], [uv(*centre), uv(x0, s0), uv(x1, s1)], want)
            finish(m, f'ButterflyWing_{look}{suffix}')


def dragonfly():
    m = InsetMesh(ATLAS)
    # The body on its one strip: U along it from the tail (0) to the eyes (1).
    lc.tube(m, [(-6.0, 0.0, 0.15), (-3.0, 0.0, 0.05), (-0.5, 0.0, 0.0)], [0.2, 0.24, 0.36], 'dfly_body', sides=6)
    lc.ellipsoid(m, (0.4, 0.0, 0.0), (1.0, 0.68, 0.78), 'dfly_thorax', segs=10, rings=6)
    lc.ellipsoid(m, (1.65, 0.0, 0.1), (0.55, 0.78, 0.6), 'dfly_eyes', segs=10, rings=6)
    # Long narrow wings rounding off at the tips, the hind pair broader at the base.
    fore = [(0.0, 0.3, -0.25, 0.0), (1.2, 0.48, -0.42, 0.05), (3.0, 0.45, -0.38, 0.08), (4.1, 0.3, -0.18, 0.1), (4.6, 0.1, -0.02, 0.1)]
    hind = [(0.0, 0.35, -0.75, 0.0), (1.2, 0.45, -0.75, 0.05), (2.9, 0.38, -0.5, 0.08), (3.9, 0.22, -0.22, 0.1), (4.3, 0.05, -0.05, 0.1)]
    for side in (1.0, -1.0):
        panel(m, 'dfly_wing', fore, side, 0.06, origin=(0.85, side * 0.35, 0.55))
        panel(m, 'dfly_wing', hind, side, 0.06, origin=(0.05, side * 0.35, 0.5))
    finish(m, 'Dragonfly')


def firefly():
    m = InsetMesh(ATLAS)
    lc.ellipsoid(m, (0.0, 0.0, 0.0), (0.8, 0.5, 0.45), 'fly_dark', segs=10, rings=6)
    finish(m, 'Firefly', material=GLOW)


def fly():
    m = InsetMesh(ATLAS)
    lc.ellipsoid(m, (-0.15, 0.0, 0.0), (0.45, 0.28, 0.26), 'fly_dark', segs=8, rings=5)
    lc.ellipsoid(m, (0.38, 0.0, 0.02), (0.2, 0.24, 0.2), 'fly_dark', segs=8, rings=5)
    for side in (1.0, -1.0):
        panel(m, 'dfly_wing', [(0.0, 0.15, -0.2, 0.0), (0.55, 0.05, -0.45, 0.02), (0.75, -0.25, -0.5, 0.02)], side, 0.02,
              origin=(0.05, side * 0.15, 0.2))
    finish(m, 'Fly')


# --- A tumbleweed, a dust devil, washing (Blender metres, the front to -Y) ---

def stem_tube(bm, uv_layer, points, radii, sides=3, u_scale=1.0 / 3.2):
    """A thin closed tube along points, mapped along its length at the tileable's world scale (a tile is 3.2 m) and a
    little way round it."""
    rings = []
    length = 0.0
    lengths = [0.0]
    for a, b in zip(points, points[1:]):
        length += (b - a).length
        lengths.append(length)
    for i, p in enumerate(points):
        t = (points[min(i + 1, len(points) - 1)] - points[max(i - 1, 0)]).normalized()
        side = t.orthogonal().normalized()
        up = t.cross(side).normalized()
        rings.append([bm.verts.new(p + (side * math.cos(2.0 * math.pi * j / sides) + up * math.sin(2.0 * math.pi * j / sides)) * radii[i])
                      for j in range(sides)])
    for i in range(len(points) - 1):
        for j in range(sides):
            j1 = (j + 1) % sides
            face = bm.faces.new((rings[i][j], rings[i + 1][j], rings[i + 1][j1], rings[i][j1]))
            for loop, (u, v) in zip(face.loops, ((lengths[i], j), (lengths[i + 1], j), (lengths[i + 1], j + 1), (lengths[i], j + 1))):
                loop[uv_layer].uv = (u * u_scale, v * 0.02 / sides)
    for end, i in ((0, 0), (1, len(points) - 1)):
        hub = bm.verts.new(points[i])
        for j in range(sides):
            j1 = (j + 1) % sides
            verts = (hub, rings[i][j1], rings[i][j]) if end == 0 else (hub, rings[i][j], rings[i][j1])
            face = bm.faces.new(verts)
            corners = ((lengths[i] + 0.02, j + 0.5), (lengths[i], j1), (lengths[i], j)) if end == 0 else \
                ((lengths[i] + 0.02, j + 0.5), (lengths[i], j), (lengths[i], j1))
            for loop, (u, v) in zip(face.loops, corners):
                loop[uv_layer].uv = (u * u_scale, v * 0.02 / sides)


def tumbleweed():
    """Dry stems tangled into a ball: each sweeps round from near the middle toward the surface, forking once."""
    rng = random.Random(41)
    radius = 0.38
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new('UVMap')
    for _ in range(58):
        start = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1)))
        if start.length < 1e-3:
            start = Vector((1.0, 0.0, 0.0))
        start.normalize()
        axis = start.orthogonal().normalized()
        axis.rotate(Matrix.Rotation(rng.uniform(0.0, 2.0 * math.pi), 3, start))
        sweep = math.radians(rng.uniform(70.0, 140.0))
        points = []
        for i in range(7):
            t = i / 6
            direction = start.copy()
            direction.rotate(Matrix.Rotation(sweep * t, 3, axis))
            r = radius * (0.15 + 0.85 * math.sqrt(t)) * rng.uniform(0.93, 1.03)
            points.append(direction * r)
        stem_tube(bm, uv_layer, points, [0.0055 * (1.0 - 0.6 * i / 6) for i in range(7)])
        # Two twigs off its outer half, curling round the ball's surface.
        for fork_at, turn in ((3, 1.0), (5, -1.0)):
            fork = points[fork_at]
            along = (points[fork_at + 1] - points[fork_at]).normalized()
            out = fork.normalized()
            twig_dir = (along.cross(out) * turn + along * 0.5).normalized()
            twig = [fork]
            for i in range(1, 4):
                p = twig[-1] + twig_dir * 0.045
                p = p.normalized() * min(p.length, radius * 1.02)     # held to the ball
                twig.append(p)
            stem_tube(bm, uv_layer, twig, [0.0032, 0.0026, 0.002, 0.0012])
    mesh = bpy.data.meshes.new('Tumbleweed')
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(TUMBLE)
    obj = bpy.data.objects.new('Tumbleweed', mesh)
    bpy.context.scene.collection.objects.link(obj)
    lm.smooth(obj, 80.0)
    lt.bake_vertex_ao(obj, samples=12, distance=0.25, ground=False)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    lt._log(f'Tumbleweed: {sum(len(p.vertices) - 2 for p in obj.data.polygons)} triangles')
    return obj


def dust_devil():
    """Three twisted cone strips of cards, inside one another, flaring as they rise: the dust rises up their V (the smoke
    master scrolls it), fading in off the ground and out toward the top, the inner layers thicker."""
    rng = random.Random(9)
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new('UVMap')
    col_layer = bm.loops.layers.color.new('Col')
    strips = 12
    # Each layer: its height (m), its radius at the foot and the top (m), how thick, its twist, how many rows; the column's
    # three nested funnels, then the skirt of dust it kicks up round its foot.
    layers = ((7.5, 0.25, 1.7, 0.75, 1.3, 9), (7.0, 0.18, 1.25, 0.9, -1.1, 9), (6.2, 0.12, 0.8, 1.0, 1.5, 9),
              (0.9, 0.45, 1.5, 0.8, 0.6, 3))
    for layer, (height, foot, top, alpha, twist, rows) in enumerate(layers):
        skirt = layer == len(layers) - 1
        phase = rng.uniform(0.0, 2.0 * math.pi)
        grid = []
        for i in range(rows + 1):
            h = i / rows
            r = foot + (top - foot) * (h if skirt else h ** 1.6)
            wobble = Vector((0.18 * math.sin(h * 2.5 + layer), 0.18 * math.cos(h * 2.1 + layer * 1.7), 0.0)) * (0.0 if skirt else h)
            ring = []
            for j in range(strips + 1):
                a = phase + 2.0 * math.pi * j / strips + twist * h
                ring.append(bm.verts.new(Vector((math.cos(a) * r, math.sin(a) * r, h * height)) + wobble))
            grid.append(ring)
        fade = [rng.uniform(0.6, 1.0) for _ in range(strips)]
        for i in range(rows):
            for j in range(strips):
                face = bm.faces.new((grid[i][j], grid[i][j + 1], grid[i + 1][j + 1], grid[i + 1][j]))
                for loop, (jj, ii) in zip(face.loops, ((j, i), (j + 1, i), (j + 1, i + 1), (j, i + 1))):
                    h = ii / rows
                    if skirt:
                        # Thickest just off the ground, gone at its rim.
                        a = alpha * fade[j] * smoothstep(0.0, 0.25, h) * (1.0 - smoothstep(0.35, 1.0, h))
                    else:
                        # Thin where it leaves the ground, thickest low down, thinning away toward the top.
                        a = alpha * fade[j] * smoothstep(0.0, 0.12, h) * (1.0 - smoothstep(0.45, 1.0, h))
                    loop[uv_layer].uv = (2.0 * jj / strips, 3.0 * h * height / 7.5)
                    loop[col_layer] = (1.0, 1.0, 1.0, a)
    mesh = bpy.data.meshes.new('DustDevil')
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(DUST)
    obj = bpy.data.objects.new('DustDevil', mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    lt._log(f'DustDevil: {sum(len(p.vertices) - 2 for p in obj.data.polygons)} triangles')
    return obj


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def cloth_uv(obj):
    """Maps a hanging cloth flat across its face at the polymer set's scale (a tile a metre), so its weave is even."""
    uv = obj.data.uv_layers.active.data
    for p in obj.data.polygons:
        for loop in p.loop_indices:
            co = obj.data.vertices[obj.data.loops[loop].vertex_index].co
            uv[loop].uv = (co.x + 0.5, co.z + 1.0)
    return obj


def washing(name, width, drop, material, seed, sleeves=0.0, columns=6, rows=4):
    """A piece of washing hanging from the middle of its top edge (the pivot, on the line): folds, a little billow, both
    sides (two thin layers), two pegs; a shirt's short sleeves stand out at its shoulders."""
    rnd = random.Random(seed)
    x0, x1 = -width * 0.5, width * 0.5
    bm = lp.new_bmesh()
    grid = []
    for j in range(rows + 1):
        row = []
        v = j / rows
        for i in range(columns + 1):
            u = i / columns
            x = x0 + (x1 - x0) * u
            # A shirt narrows below its sleeves.
            if sleeves > 0.0 and v > 0.3:
                x *= 1.0 - sleeves / (width * 0.5)
            fold = 0.03 * math.sin(u * math.pi * (columns * 0.5) + rnd.uniform(-0.3, 0.3)) * (0.3 + v)
            billow = 0.05 * v ** 1.5
            row.append(Vector((x, fold + billow, -0.01 - drop * v + 0.02 * rnd.uniform(-1.0, 1.0) * v)))
        grid.append(row)
    for offset in (0.0, 0.006):
        verts = [[bm.verts.new(p + Vector((0.0, offset, 0.0))) for p in row] for row in grid]
        for j in range(rows):
            for i in range(columns):
                quad = (verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i])
                # The front layer faces the front (-Y), the back layer the back.
                bm.faces.new(tuple(reversed(quad)) if offset == 0.0 else quad)
    part = lp.mesh_object(bm)
    lt.assign(part, material)
    cloth_uv(part)
    parts = [part]
    for x in (x0 + 0.03, x1 - 0.03):
        peg = lp.block((0.018, 0.022, 0.07), (x, -0.005, -0.01), (0.0, 0.0, rnd.uniform(-8.0, 8.0)))
        parts.append(lp.grain(peg, 'Siding', axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999)))
    obj = lp.join(name, parts)
    lm.smooth(obj, 50.0)
    lt.bake_vertex_ao(obj, samples=12, distance=0.08, ground=False)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    lt._log(f'{name}: {sum(len(p.vertices) - 2 for p in obj.data.polygons)} triangles')
    return obj


# --- Build ---

crow()
sparrow()
swallow()
hawk()
butterfly()
dragonfly()
firefly()
fly()
tumbleweed()
dust_devil()
washing('WindCloth_Sheet', 0.85, 0.72, LINEN, 11, columns=8, rows=5)
washing('WindCloth_Shirt', 0.62, 0.58, SHIRT, 12, sleeves=0.08)
washing('WindCloth_Towel', 0.38, 0.42, TOWEL, 13, columns=4, rows=4)


def show(names, out, view=(-1.0, -1.6, 0.7), assemble=None, ground=True):
    """A preview of some models, assembled where asked (copies placed by a matrix, removed after)."""
    objects = [bpy.data.objects[n] for n in names]
    temp = []
    for name, matrix in assemble or ():
        copy = bpy.data.objects[name].copy()
        copy.data = bpy.data.objects[name].data
        bpy.context.scene.collection.objects.link(copy)
        copy.matrix_world = matrix
        temp.append(copy)
    lt.preview(objects + temp, lt.preview_path('Creatures', out), view=view, ground=ground)
    for copy in temp:
        bpy.data.objects.remove(copy, do_unlink=True)


if lt.want_preview():
    def socket_at(model, name):
        return next(c for c in bpy.data.objects[model].children if c.name.startswith('SOCKET_' + name)).matrix_world.translation

    def wing_turn(side, up):
        # About the bird's forward axis (Blender's Y line): raising a wing's tip (the left wing lies along +X).
        return Matrix.Rotation(-math.radians(up) * side, 4, 'Y')
    def flying(model, inner_l, inner_r, outer_l=None, outer_r=None, up=12.0, bend=6.0):
        """A flying bird's wings on its sockets, raised a little, the hands bent a little more (as in a glide)."""
        parts = []
        for side, inner, outer, sock in ((1, inner_l, outer_l, 'WingL'), (-1, inner_r, outer_r, 'WingR')):
            arm = Matrix.Translation(socket_at(model, sock)) @ wing_turn(side, up)
            parts.append((inner, arm))
            if outer:
                wrist = next(c for c in bpy.data.objects[inner].children if c.name.startswith('SOCKET_Wrist')).location
                parts.append((outer, arm @ Matrix.Translation(wrist) @ wing_turn(side, bend)))
        return parts
    head_on = Matrix.Translation(socket_at('Crow_Perched', 'Head'))
    show(['Crow_Perched'], 'AmbientFauna_Crow', view=(-1.0, -1.4, 0.5), assemble=[('Crow_Head', head_on)])
    show(['Crow_Perched'], 'AmbientFauna_CrowSide', view=(-1.0, 0.0, 0.15), assemble=[('Crow_Head', head_on)])
    show(['Crow_Flying'], 'AmbientFauna_CrowFlying', view=(-0.6, -1.0, 1.2),
         assemble=flying('Crow_Flying', 'Crow_WingInnerL', 'Crow_WingInnerR', 'Crow_WingOuterL', 'Crow_WingOuterR'))
    show(['Sparrow_Perched'], 'AmbientFauna_Sparrow', view=(-1.0, -1.4, 0.5),
         assemble=[('Sparrow_Head', Matrix.Translation(socket_at('Sparrow_Perched', 'Head')))])
    show(['Swallow_Flying'], 'AmbientFauna_Swallow', view=(-0.3, -0.6, 1.2), assemble=flying('Swallow_Flying', 'Swallow_WingL', 'Swallow_WingR'))
    show(['Hawk_Flying'], 'AmbientFauna_Hawk', view=(-0.4, -0.7, -1.2), ground=False,
         assemble=flying('Hawk_Flying', 'Hawk_WingL', 'Hawk_WingR', up=6.0))
    show(['Butterfly_Body', 'ButterflyWing_CopperL', 'ButterflyWing_CopperR'], 'AmbientFauna_Butterfly', view=(-0.5, -0.8, 1.2))
    show(['Dragonfly'], 'AmbientFauna_Dragonfly', view=(-0.5, -0.8, 1.0))
    show(['Tumbleweed'], 'AmbientFauna_Tumbleweed')
    show(['DustDevil'], 'AmbientFauna_DustDevil', view=(-1.0, -2.0, 0.4))
    show([], 'AmbientFauna_Washing', view=(0.0, -1.6, 0.3), ground=False,
         assemble=[('WindCloth_Sheet', Matrix.Translation((-0.9, 0.0, 0.0))), ('WindCloth_Shirt', Matrix.Translation((0.0, 0.0, 0.0))),
                   ('WindCloth_Towel', Matrix.Translation((0.7, 0.0, 0.0)))])
