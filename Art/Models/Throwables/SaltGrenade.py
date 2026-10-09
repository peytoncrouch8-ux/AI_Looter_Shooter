"""The grave-salt grenade (Docs/Polish/BorderlandsComparison.md, item 16): a stoppered tin of grave salt with a waxed
fuse, the kind of thing Tilly Bright would sell over her counter for keeping the Unpaid off. The player lobs it
(AGraveSaltGrenade flies it, UPlayerThrowComponent shows it in the off hand in first person) and finds it as loot
(AGrenadePickup shows it 1.8 times as big, spinning, so it reads in the grass). A scripted model file (Art/README.md).

  SaltGrenade   a squat painted tin (dark iron-grey, chipped to the bare tin), 7.5 cm across and 9 cm to its lip: a
                rolled bead at its foot and under its shoulder, a pale parchment label round its middle with a grave
                cross painted on the front (-Y), a conical shoulder up to a short neck. A cork stopper is pushed into the neck and sealed with oxblood
                wax that has run down the neck in a couple of drips; the waxed fuse cord comes up out of the cork and
                curls over to one side, its tip charred. A crust of salt crystals clings round the neck where it was
                filled. 12 cm to the fuse's top.

Pivot: the middle of its foot. Its middle is 5 cm up (AGraveSaltGrenade tumbles it about there).
  SOCKET_Fuse   the fuse's tip, +X along the cord's last stretch: where the sparks and the fizz come from.
No collision (the grenade sweeps its own ball; the pickup has its own sphere), no Nanite (small and held close), no
LODs. Materials (7 slots), all from the textured sets the game already loads, tinted:
  SaltTin        PaintWorn, iron-grey paint      SaltLabel      Polymer, parchment
  SaltLabelInk   Polymer, the cross's dark ink   SaltCork       Polymer, cork tan
  SaltWax        Polymer, oxblood sealing wax    SaltFuse       Polymer, waxed brown cord (its tip charred dark)
  SaltCrystal    Polymer, salt white

    blender -b --factory-startup --python-expr "import sys; sys.path.insert(0, 'Tools/Blender')" \\
        --python Art/Models/Throwables/SaltGrenade.py -- --preview
renders Saved/ArtPreviews/Throwables/SaltGrenade.png (three-quarter front) and SaltGrenade_Top.png (from above, the
cork, wax and fuse).
"""
import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_props as lp
import looter_textures as lt

# --- Sizes (metres) ---
R_BODY = 0.0375          # the tin's wall
R_BEAD = 0.0392          # the rolled beads at its foot and under its shoulder
H_FOOT = 0.007           # the foot bead's top
H_BODY = 0.062           # the wall's top, under the upper bead
H_SHOULDER = 0.071       # where the shoulder starts to slope in
R_NECK = 0.0158
H_NECK = 0.081           # the shoulder meets the neck here
H_LIP = 0.091            # the neck's rolled lip
LABEL = (0.022, 0.052)   # the label band, foot to top
SEGMENTS = 24

TIN = lt.material('PaintWorn', name='SaltTin', tint=0x5b625a)
LABEL_PAPER = lt.material('Polymer', name='SaltLabel', tint=0xe8dcc0)
LABEL_INK = lt.material('Polymer', name='SaltLabelInk', tint=0x3b2b22)
CORK = lt.material('Polymer', name='SaltCork', tint=0xb8915f)
WAX = lt.material('Polymer', name='SaltWax', tint=0x7c211b)
FUSE = lt.material('Polymer', name='SaltFuse', tint=0x6e5536)
CRYSTAL = lt.material('Polymer', name='SaltCrystal', tint=0xf2eee4)

RNG = random.Random(4417)


def lathed(profile, mat, segments=SEGMENTS):
    obj = lp.lathe(profile, segments=segments)[0]
    lt.assign(obj, mat)
    lp.lathe_uv(obj, mat['TextureSet'])
    return obj


def tin_body():
    """The tin from its foot to the neck's lip: beads at the foot and under the shoulder, the cone up to the neck."""
    profile = [(0.0, 0.0), (R_BODY - 0.002, 0.0), (R_BEAD, 0.0025), (R_BEAD, H_FOOT - 0.002), (R_BODY, H_FOOT),
               (R_BODY, H_BODY), (R_BEAD, H_BODY + 0.002), (R_BEAD, H_SHOULDER - 0.003), (R_BODY - 0.001, H_SHOULDER),
               (R_NECK + 0.004, H_NECK - 0.002), (R_NECK, H_NECK), (R_NECK, H_LIP - 0.003),
               (R_NECK + 0.0018, H_LIP - 0.0018), (R_NECK + 0.0018, H_LIP), (R_NECK - 0.002, H_LIP),
               (R_NECK - 0.002, H_LIP - 0.004), (0.0, H_LIP - 0.004)]
    return lathed(profile, TIN)


def label():
    """The parchment band round the middle, a hair proud of the tin, and the grave cross painted on its front."""
    lo, hi = LABEL
    r = R_BODY + 0.0006
    band = lathed([(r, lo), (r, hi), (r - 0.0004, hi), (r - 0.0004, lo)], LABEL_PAPER)
    # The cross: two thin strips laid on the front (-Y), a hair proud of the label, curved round with it.
    parts = [band]
    mid = (lo + hi) * 0.5
    for width, height, z in ((0.0042, 0.021, mid), (0.014, 0.0042, mid + 0.0045)):
        strip = lp.block((width, 0.0008, height), (0.0, -(r + 0.0004), z))
        lt.assign(strip, LABEL_INK)
        lt.box_uv(strip, 'Polymer')
        parts.append(strip)
    return parts


def cork():
    """The cork pushed into the neck, a little wider at its top, standing 1.3 cm proud of the lip."""
    profile = [(0.0, H_LIP - 0.006), (R_NECK - 0.0025, H_LIP - 0.006), (R_NECK - 0.0012, H_LIP + 0.013),
               (R_NECK - 0.003, H_LIP + 0.0145), (0.0, H_LIP + 0.0145)]
    return lathed(profile, CORK, 16)


def wax():
    """The sealing wax over the lip and round the cork's foot, lumpy, with two drips run down the neck."""
    profile = [(R_NECK - 0.0035, H_LIP + 0.004), (R_NECK + 0.001, H_LIP + 0.0035), (R_NECK + 0.0032, H_LIP + 0.0005),
               (R_NECK + 0.0034, H_LIP - 0.004), (R_NECK + 0.0012, H_LIP - 0.0055), (R_NECK - 0.0008, H_LIP - 0.003),
               (R_NECK - 0.0035, H_LIP + 0.001)]
    ring = lp.lathe(profile, segments=SEGMENTS, closed=True)[0]
    # Lumpy: each column pushed out or in a little, the same every run.
    lumps = [RNG.uniform(-0.0007, 0.0009) for _ in range(SEGMENTS)]
    for v in ring.data.vertices:
        a = math.atan2(v.co.y, v.co.x)
        k = int(round((a % (2.0 * math.pi)) / (2.0 * math.pi) * SEGMENTS)) % SEGMENTS
        rr = math.hypot(v.co.x, v.co.y)
        if rr > 1e-6:
            s = (rr + lumps[k]) / rr
            v.co.x *= s
            v.co.y *= s
    ring.data.update()
    lt.assign(ring, WAX)
    lp.lathe_uv(ring, 'Polymer')
    parts = [ring]
    for angle, length in ((-0.6, 0.0075), (2.1, 0.005)):
        d = Vector((math.cos(angle), math.sin(angle), 0.0))
        top = d * (R_NECK + 0.0028) + Vector((0.0, 0.0, H_LIP - 0.002))
        bottom = d * (R_NECK + 0.0022) + Vector((0.0, 0.0, H_LIP - 0.002 - length))
        drip = lp.sweep([top, (top + bottom) * 0.5, bottom], [(0.0016, 0.0), (0.0, 0.0012), (-0.0016, 0.0), (0.0, -0.0010)],
                        scales=[1.0, 0.9, 1.25])
        lt.assign(drip, WAX)
        lt.box_uv(drip, 'Polymer')
        parts.append(drip)
    return parts


FUSE_PATH = [Vector(p) for p in ((0.0, 0.0, H_LIP + 0.012), (0.0008, -0.0004, H_LIP + 0.019), (0.0035, -0.0012, H_LIP + 0.0245),
                                 (0.0085, -0.0018, H_LIP + 0.0275), (0.0145, -0.0016, H_LIP + 0.0278),
                                 (0.0198, -0.001, H_LIP + 0.025))]
CHAR = 0.0045            # how much of the fuse's end is burnt


def fuse():
    """The waxed cord out of the cork, curling over to one side; its last few millimetres charred."""
    profile = [(math.cos(2.0 * math.pi * k / 6) * 0.0019, math.sin(2.0 * math.pi * k / 6) * 0.0019) for k in range(6)]
    cord = lp.sweep(FUSE_PATH, profile)
    lt.assign(cord, FUSE)
    lt.box_uv(cord, 'Polymer')
    tip = FUSE_PATH[-1]
    back = (FUSE_PATH[-2] - tip).normalized()
    char = lp.sweep([tip + back * CHAR, tip + back * (CHAR * 0.4), tip + back * -0.0006], profile, scales=[1.06, 1.08, 0.75])
    lt.assign(char, LABEL_INK)
    lt.box_uv(char, 'Polymer')
    return [cord, char]


def crystals():
    """A crust of salt round the neck where the tin was filled: small rough crystals sat on the shoulder by the neck."""
    parts = []
    for i in range(22):
        a = RNG.uniform(0.0, 2.0 * math.pi)
        rr = RNG.uniform(R_NECK + 0.0015, R_NECK + 0.007)
        z = H_NECK - 0.0035 * (rr - R_NECK) / 0.007 + 0.0006
        size = RNG.uniform(0.0016, 0.0032)
        bm = lp.new_bmesh()
        bmesh.ops.create_icosphere(bm, subdivisions=1, radius=size)
        for v in bm.verts:
            v.co.x *= RNG.uniform(0.7, 1.2)
            v.co.y *= RNG.uniform(0.7, 1.2)
            v.co.z *= RNG.uniform(0.6, 1.0)
        obj = lp.mesh_object(bm, f'_crystal{i}')
        obj.data.transform(Matrix.Translation((math.cos(a) * rr, math.sin(a) * rr, z)) @ Matrix.Rotation(RNG.uniform(0, 6.3), 4, 'Z'))
        obj.data.update()
        lt.assign(obj, CRYSTAL)
        lt.box_uv(obj, 'Polymer')
        parts.append(obj)
    return parts


def build():
    parts = [tin_body(), cork(), *label(), *wax(), *fuse(), *crystals()]
    model = lp.join('SaltGrenade', parts)
    lp.finish(model, ao=0.02, nanite=False, smooth=40.0)
    model['Collision'] = 'None'
    # The fuse's tip, +X along the cord's last stretch (a socket's front is its -Y: turned so -Y runs out of the tip).
    tip = FUSE_PATH[-1]
    out = (tip - FUSE_PATH[-2]).normalized()
    yaw = math.degrees(math.atan2(out.y, out.x))
    lm.socket(model, 'Fuse', tip, (0.0, 0.0, yaw + 90.0))
    return model


for _obj in list(bpy.data.objects):   # the startup scene's cube, light and camera
    bpy.data.objects.remove(_obj)
grenade = build()
lt._log(f'{grenade.name}: {lp.tri_count(grenade)} triangles, materials {", ".join(m.name for m in grenade.data.materials)}')

if lt.want_preview():
    bpy.context.view_layer.update()
    lt.preview([grenade], lt.preview_path('Throwables', 'SaltGrenade'), view=(-0.55, -1.4, 0.55), fit=1.15)
    lt.preview([grenade], lt.preview_path('Throwables', 'SaltGrenade_Top'), view=(0.35, -0.5, 1.4), fit=1.15)
