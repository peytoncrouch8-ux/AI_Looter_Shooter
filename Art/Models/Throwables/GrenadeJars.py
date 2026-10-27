"""The grenade kinds' tins (round 4, 2026-10-27: GrenadeKinds in Source/.../Combat): three jars beside the grave-salt
grenade (SaltGrenade.py), each the same stoppered tin underneath, so they sit in the hand, fly and lie exactly as it does,
dressed for what's packed in with the salt. A scripted model file holding three models (Art/README.md):

  PitchJar     salt packed in hot pitch: the tin painted a weathered dun, its shoulder and upper wall coated in glossy
               pitch run down the sides in drips, a band of tarred sacking tied round its foot with cord, the stopper
               sealed in pitch, a few salt crystals stuck in the coat. It sticks where it lands.
  ScatterJar   a cartridge maker's trick: the tin in verdigris paint, two iron bands round its waist riveted between
               four small copper capsules held upright in them (the brood it flings), each with its own cork.
  CinderJar    salt over forge coals: a rusted iron tin with a vent band round its middle, dark slots in it glowing
               ember-orange from the coals inside, iron bands above and below, the stopper sealed in soot-black wax.

Each is built at the salt grenade's first size and scaled by SCALE (2) as a whole after its UVs, like SaltGrenade.py:
15 cm across its tin, 24 cm to its fuse's top (FGraveSaltRules' JarMiddle 10, JarRadius 7.5, JarFuseTop 24); the
scatter's capsules stand a little proud of the tin (about 19 cm across them).

Pivot: the middle of its foot. SOCKET_Fuse at the fuse's tip, as SaltGrenade's. No collision, no Nanite, no LODs.
Materials (the textured sets the game already loads; the salt grenade's own slots are shared by name where they're the
same stuff: SaltCork, SaltFuse, SaltLabelInk, SaltCrystal, SaltWax):
  PitchTin     PaintWorn, weathered dun paint   PitchCoat    Polymer, glossy pitch
  PitchWrap    Polymer, tarred sacking          PitchCord    Polymer, hemp cord
  ScatterTin   PaintWorn, verdigris green       ScatterBand  MetalWorn, dark iron
  ScatterShell MetalWorn, copper                CinderTin    MetalRust, rusted iron
  CinderBand   MetalWorn, sooty iron            CinderSlot   Polymer, soot black
  CinderGlow   flat Glow, ember orange (the coals seen through the slots)
  CinderWax    Polymer, soot-black wax

    blender -b --factory-startup --python-expr "import sys; sys.path.insert(0, 'Tools/Blender')" \\
        --python Art/Models/Throwables/GrenadeJars.py -- --preview
renders Saved/ArtPreviews/Throwables/GrenadeJars.png (the three side by side).
"""
import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_props as lp
import looter_textures as lt

# --- The salt grenade's tin (SaltGrenade.py; metres at its first size, scaled by SCALE) ---
SCALE = 2.0
R_BODY = 0.0375
R_BEAD = 0.0392
H_FOOT = 0.007
H_BODY = 0.062
H_SHOULDER = 0.071
R_NECK = 0.0158
H_NECK = 0.081
H_LIP = 0.091
SEGMENTS = 24

FUSE_PATH = [Vector(p) for p in ((0.0, 0.0, H_LIP + 0.012), (0.0008, -0.0004, H_LIP + 0.019), (0.0035, -0.0012, H_LIP + 0.0245),
                                 (0.0085, -0.0018, H_LIP + 0.0275), (0.0145, -0.0016, H_LIP + 0.0278),
                                 (0.0198, -0.001, H_LIP + 0.025))]
CHAR = 0.0045

# Shared with the salt grenade (same names: the same material instances in the game).
CORK = lt.material('Polymer', name='SaltCork', tint=0xb8915f)
FUSE = lt.material('Polymer', name='SaltFuse', tint=0x6e5536)
INK = lt.material('Polymer', name='SaltLabelInk', tint=0x3b2b22)
CRYSTAL = lt.material('Polymer', name='SaltCrystal', tint=0xf2eee4)
WAX = lt.material('Polymer', name='SaltWax', tint=0x7c211b)

# The tin under the pitch a weathered dun, so the black coat and its drips read against it.
PITCH_TIN = lt.material('PaintWorn', name='PitchTin', tint=0x6a604f)
PITCH_COAT = lt.material('Polymer', name='PitchCoat', tint=0x16110d)
PITCH_WRAP = lt.material('Polymer', name='PitchWrap', tint=0x6a5638)
PITCH_CORD = lt.material('Polymer', name='PitchCord', tint=0x8f7a55)

SCATTER_TIN = lt.material('PaintWorn', name='ScatterTin', tint=0x4c7060)
SCATTER_BAND = lt.material('MetalWorn', name='ScatterBand', tint=0x55524c)
SCATTER_SHELL = lt.material('MetalWorn', name='ScatterShell', tint=0xc0835a)

CINDER_TIN = lt.material('MetalRust', name='CinderTin', tint=0xa0603e)
CINDER_BAND = lt.material('MetalWorn', name='CinderBand', tint=0x3d3a36)
CINDER_SLOT = lt.material('Polymer', name='CinderSlot', tint=0x1b120d)
CINDER_WAX = lt.material('Polymer', name='CinderWax', tint=0x231a16)
# The coals through the slots: a flat glowing surface (non-Nanite, so it may glow), hot orange.
CINDER_GLOW = lm.material('CinderGlow', 0xff6a1e, 'Glow', Glow=4.0, Variation=0.05)


def lathed(profile, mat, set_name, segments=SEGMENTS, closed=False):
    obj = lp.lathe(profile, segments=segments, closed=closed)[0]
    lt.assign(obj, mat)
    lp.lathe_uv(obj, set_name)
    return obj


def tin_body(mat, set_name):
    """The salt grenade's tin from its foot to the neck's lip, in another paint."""
    profile = [(0.0, 0.0), (R_BODY - 0.002, 0.0), (R_BEAD, 0.0025), (R_BEAD, H_FOOT - 0.002), (R_BODY, H_FOOT),
               (R_BODY, H_BODY), (R_BEAD, H_BODY + 0.002), (R_BEAD, H_SHOULDER - 0.003), (R_BODY - 0.001, H_SHOULDER),
               (R_NECK + 0.004, H_NECK - 0.002), (R_NECK, H_NECK), (R_NECK, H_LIP - 0.003),
               (R_NECK + 0.0018, H_LIP - 0.0018), (R_NECK + 0.0018, H_LIP), (R_NECK - 0.002, H_LIP),
               (R_NECK - 0.002, H_LIP - 0.004), (0.0, H_LIP - 0.004)]
    return lathed(profile, mat, set_name)


def cork():
    profile = [(0.0, H_LIP - 0.006), (R_NECK - 0.0025, H_LIP - 0.006), (R_NECK - 0.0012, H_LIP + 0.013),
               (R_NECK - 0.003, H_LIP + 0.0145), (0.0, H_LIP + 0.0145)]
    return lathed(profile, CORK, 'Polymer', 16)


def seal(mat, rng):
    """The seal over the lip and round the cork's foot, lumpy (the salt grenade's wax, in its own stuff)."""
    profile = [(R_NECK - 0.0035, H_LIP + 0.004), (R_NECK + 0.001, H_LIP + 0.0035), (R_NECK + 0.0032, H_LIP + 0.0005),
               (R_NECK + 0.0034, H_LIP - 0.004), (R_NECK + 0.0012, H_LIP - 0.0055), (R_NECK - 0.0008, H_LIP - 0.003),
               (R_NECK - 0.0035, H_LIP + 0.001)]
    ring = lp.lathe(profile, segments=SEGMENTS, closed=True)[0]
    lumps = [rng.uniform(-0.0007, 0.0009) for _ in range(SEGMENTS)]
    for v in ring.data.vertices:
        a = math.atan2(v.co.y, v.co.x)
        k = int(round((a % (2.0 * math.pi)) / (2.0 * math.pi) * SEGMENTS)) % SEGMENTS
        rr = math.hypot(v.co.x, v.co.y)
        if rr > 1e-6:
            s = (rr + lumps[k]) / rr
            v.co.x *= s
            v.co.y *= s
    ring.data.update()
    lt.assign(ring, mat)
    lp.lathe_uv(ring, 'Polymer')
    return ring


def fuse():
    """The waxed cord out of the cork, curling over, its end charred: the salt grenade's, so the sparks sit the same."""
    profile = [(math.cos(2.0 * math.pi * k / 6) * 0.0019, math.sin(2.0 * math.pi * k / 6) * 0.0019) for k in range(6)]
    cord = lp.sweep(FUSE_PATH, profile)
    lt.assign(cord, FUSE)
    lt.box_uv(cord, 'Polymer')
    tip = FUSE_PATH[-1]
    back = (FUSE_PATH[-2] - tip).normalized()
    char = lp.sweep([tip + back * CHAR, tip + back * (CHAR * 0.4), tip + back * -0.0006], profile, scales=[1.06, 1.08, 0.75])
    lt.assign(char, INK)
    lt.box_uv(char, 'Polymer')
    return [cord, char]


def crystals(rng, count, name, r_lo, r_hi, z_of):
    """Small rough salt crystals, sat round the neck or stuck in a coat."""
    parts = []
    for i in range(count):
        a = rng.uniform(0.0, 2.0 * math.pi)
        rr = rng.uniform(r_lo, r_hi)
        size = rng.uniform(0.0016, 0.003)
        bm = lp.new_bmesh()
        bmesh.ops.create_icosphere(bm, subdivisions=1, radius=size)
        for v in bm.verts:
            v.co.x *= rng.uniform(0.7, 1.2)
            v.co.y *= rng.uniform(0.7, 1.2)
            v.co.z *= rng.uniform(0.6, 1.0)
        obj = lp.mesh_object(bm, f'_{name}{i}')
        obj.data.transform(Matrix.Translation((math.cos(a) * rr, math.sin(a) * rr, z_of(rr))) @ Matrix.Rotation(rng.uniform(0, 6.3), 4, 'Z'))
        obj.data.update()
        lt.assign(obj, CRYSTAL)
        lt.box_uv(obj, 'Polymer')
        parts.append(obj)
    return parts


def band(r, z0, z1, mat, set_name, segments=SEGMENTS):
    """A ring round the tin from z0 to z1 at radius r, its edges rounded a hair."""
    return lathed([(r - 0.0006, z0), (r, z0 + 0.0006), (r, z1 - 0.0006), (r - 0.0006, z1)], mat, set_name, segments, closed=True)


def finish(name, parts):
    """Joined, scaled about its foot after the UVs (as SaltGrenade.py), shaded, its fuse socket at the tip."""
    model = lp.join(name, parts)
    model.data.transform(Matrix.Scale(SCALE, 4))
    model.data.update()
    lp.finish(model, ao=0.02 * SCALE, nanite=False, smooth=40.0)
    model['Collision'] = 'None'
    tip = FUSE_PATH[-1] * SCALE
    out = (FUSE_PATH[-1] - FUSE_PATH[-2]).normalized()
    yaw = math.degrees(math.atan2(out.y, out.x))
    lm.socket(model, 'Fuse', tip, (0.0, 0.0, yaw + 90.0))
    return model


# --- The pitch jar ------------------------------------------------------------------------------------------------------

def pitch_jar():
    rng = random.Random(5521)
    parts = [tin_body(PITCH_TIN, 'PaintWorn'), cork(), seal(PITCH_COAT, rng), *fuse()]
    # The coat: pitch over the shoulder and down the wall, its lower edge ragged where it ran and set.
    coat_profile = [(R_BODY + 0.0011, 0.036), (R_BODY + 0.0013, H_BODY), (R_BEAD + 0.0012, H_BODY + 0.002),
                    (R_BEAD + 0.0012, H_SHOULDER - 0.003), (R_BODY + 0.0003, H_SHOULDER + 0.0004),
                    (R_NECK + 0.005, H_NECK - 0.0012), (R_NECK + 0.0012, H_NECK + 0.0008), (R_NECK + 0.0012, H_LIP - 0.004),
                    (R_NECK - 0.0002, H_LIP - 0.004), (R_NECK - 0.0002, H_NECK), (R_BODY - 0.0008, H_SHOULDER - 0.001),
                    (R_BODY - 0.0002, 0.036)]
    coat = lp.lathe(coat_profile, segments=SEGMENTS, closed=True)[0]
    sag = [rng.uniform(0.0, 0.007) for _ in range(SEGMENTS)]
    for v in coat.data.vertices:
        if v.co.z < 0.037:
            a = math.atan2(v.co.y, v.co.x)
            k = int(round((a % (2.0 * math.pi)) / (2.0 * math.pi) * SEGMENTS)) % SEGMENTS
            v.co.z -= sag[k]
    coat.data.update()
    lt.assign(coat, PITCH_COAT)
    lp.lathe_uv(coat, 'Polymer')
    parts.append(coat)
    # Drips run further down the wall, fat at their ends.
    for angle, length in ((-1.2, 0.017), (-0.2, 0.011), (0.9, 0.021), (2.3, 0.013), (3.6, 0.018), (4.9, 0.009)):
        d = Vector((math.cos(angle), math.sin(angle), 0.0))
        top = d * (R_BODY + 0.0011) + Vector((0.0, 0.0, 0.034))
        bottom = d * (R_BODY + 0.0012) + Vector((0.0, 0.0, 0.034 - length))
        drip = lp.sweep([top, top * 0.5 + bottom * 0.5, bottom], [(0.0022, 0.0), (0.0, 0.0012), (-0.0022, 0.0), (0.0, -0.0009)],
                        up=(0.0, 0.0, 1.0) if abs(d.z) > 0.9 else tuple(d), scales=[1.0, 0.85, 1.35])
        lt.assign(drip, PITCH_COAT)
        lt.box_uv(drip, 'Polymer')
        parts.append(drip)
    # The tarred sacking round its foot and the cord that ties it.
    wrap = band(R_BODY + 0.0016, 0.004, 0.026, PITCH_WRAP, 'Polymer')
    parts.append(wrap)
    for z in (0.009, 0.021):
        cord = lathed([(R_BODY + 0.0028, z - 0.0011), (R_BODY + 0.0036, z), (R_BODY + 0.0028, z + 0.0011), (R_BODY + 0.0018, z)],
                      PITCH_CORD, 'Polymer', 16, closed=True)
        parts.append(cord)
    # The knot's tails hanging off the front.
    for side in (-1.0, 1.0):
        start = Vector((side * 0.003, -(R_BODY + 0.0036), 0.021))
        tail = lp.sweep([start, start + Vector((side * 0.0025, -0.0018, -0.006)), start + Vector((side * 0.003, -0.0012, -0.012))],
                        lp.ngon(0.0011, 5), scales=[1.0, 0.9, 0.7])
        lt.assign(tail, PITCH_CORD)
        lt.box_uv(tail, 'Polymer')
        parts.append(tail)
    # Salt crystals stuck in the pitch on the shoulder.
    parts += crystals(rng, 10, 'pitchsalt', R_NECK + 0.006, R_BODY - 0.002,
                      lambda rr: H_SHOULDER + (H_NECK - H_SHOULDER) * (R_BODY - rr) / (R_BODY - R_NECK) + 0.0012)
    return finish('PitchJar', parts)


# --- The scatter jar ----------------------------------------------------------------------------------------------------

POD_R = 0.0068           # a capsule's radius
POD_AT = R_BODY + 0.0034  # its axis from the tin's: half sunk in the bands
POD_Z = (0.014, 0.054)   # its foot and its shoulder


def pod(angle):
    """One small copper capsule standing in the bands, its own cork in its neck."""
    lo, hi = POD_Z
    profile = [(0.0, lo), (POD_R - 0.0012, lo), (POD_R, lo + 0.0012), (POD_R, hi - 0.004), (POD_R * 0.55, hi),
               (POD_R * 0.45, hi + 0.002), (0.0, hi + 0.002)]
    shell = lathed(profile, SCATTER_SHELL, 'MetalWorn', 12)
    stopper = lathed([(0.0, hi + 0.001), (POD_R * 0.42, hi + 0.001), (POD_R * 0.38, hi + 0.0055), (0.0, hi + 0.0055)], CORK, 'Polymer', 8)
    out = Vector((math.cos(angle), math.sin(angle), 0.0)) * POD_AT
    for part in (shell, stopper):
        lp.place(part, out)
    return [shell, stopper]


def scatter_jar():
    rng = random.Random(7310)
    parts = [tin_body(SCATTER_TIN, 'PaintWorn'), cork(), seal(WAX, rng), *fuse()]
    # Four capsules round the waist, between the front and the sides so the grave mark's spot (the front) stays clear.
    for k in range(4):
        parts += pod(math.radians(45.0 + 90.0 * k))
    # The two iron bands holding them, and a rivet between each pair of capsules.
    for z0, z1 in ((0.017, 0.023), (0.043, 0.049)):
        parts.append(band(POD_AT + POD_R * 0.55, z0, z1, SCATTER_BAND, 'MetalWorn', 32))
        for k in range(4):
            a = math.radians(90.0 * k)
            r = POD_AT + POD_R * 0.55 + 0.0005
            rivet = lathed([(0.0, 0.0), (0.0013, 0.0), (0.0011, 0.0009), (0.0, 0.0012)], SCATTER_BAND, 'MetalWorn', 8)
            # Turned to face out of the band (its Z out along the radius), then set on it.
            rivet.data.transform(Matrix.Rotation(math.pi / 2.0, 4, 'Y'))
            rivet.data.transform(Matrix.Rotation(a, 4, 'Z'))
            rivet.data.transform(Matrix.Translation((math.cos(a) * r, math.sin(a) * r, (z0 + z1) * 0.5)))
            rivet.data.update()
            parts.append(rivet)
    parts += crystals(rng, 12, 'scattersalt', R_NECK + 0.0015, R_NECK + 0.007,
                      lambda rr: H_NECK - 0.0035 * (rr - R_NECK) / 0.007 + 0.0006)
    return finish('ScatterJar', parts)


# --- The cinder jar -----------------------------------------------------------------------------------------------------

VENT = (0.022, 0.05)     # the vent band, foot to top
SLOTS = 10


def cinder_jar():
    rng = random.Random(9043)
    parts = [tin_body(CINDER_TIN, 'MetalRust'), cork(), seal(CINDER_WAX, rng), *fuse()]
    lo, hi = VENT
    # The iron bands over and under the vents.
    parts.append(band(R_BODY + 0.0016, lo - 0.004, lo, CINDER_BAND, 'MetalWorn', 32))
    parts.append(band(R_BODY + 0.0016, hi, hi + 0.004, CINDER_BAND, 'MetalWorn', 32))
    # The slots: a dark frame round each, the coals glowing in it, a hair proud of the tin so they read as cut in.
    for k in range(SLOTS):
        a = 2.0 * math.pi * k / SLOTS + math.radians(9.0)
        frame = lp.block((0.0072, 0.0012, hi - lo - 0.004), (0.0, 0.0, 0.0), bevel=0.0003)
        coal = lp.block((0.0042, 0.0008, hi - lo - 0.008), (0.0, -0.0004, 0.0))
        for part, mat, set_name in ((frame, CINDER_SLOT, 'Polymer'), (coal, CINDER_GLOW, 'Polymer')):
            # Faced out of the tin (its -Y out along the radius), then set on the wall.
            part.data.transform(Matrix.Rotation(a + math.pi / 2.0, 4, 'Z'))
            part.data.transform(Matrix.Translation((math.cos(a) * (R_BODY + 0.0004), math.sin(a) * (R_BODY + 0.0004), (lo + hi) * 0.5)))
            part.data.update()
            lt.assign(part, mat)
            lt.box_uv(part, set_name)
            parts.append(part)
    # A little soot and salt round the neck.
    parts += crystals(rng, 10, 'cindersalt', R_NECK + 0.0015, R_NECK + 0.007,
                      lambda rr: H_NECK - 0.0035 * (rr - R_NECK) / 0.007 + 0.0006)
    return finish('CinderJar', parts)


for _obj in list(bpy.data.objects):   # the startup scene's cube, light and camera
    bpy.data.objects.remove(_obj)
jars = [pitch_jar(), scatter_jar(), cinder_jar()]
for jar in jars:
    lt._log(f'{jar.name}: {lp.tri_count(jar)} triangles, materials {", ".join(m.name for m in jar.data.materials)}')

if lt.want_preview():
    # Side by side for the preview only (each model's own pivot stays at its foot's middle on export).
    for index, jar in enumerate(jars):
        jar.location.x = (index - 1) * 0.24
    bpy.context.view_layer.update()
    lt.preview(jars, lt.preview_path('Throwables', 'GrenadeJars'), view=(-0.55, -1.4, 0.55), fit=1.15)
    lt.preview(jars, lt.preview_path('Throwables', 'GrenadeJars_Top'), view=(0.35, -0.5, 1.4), fit=1.15)
    for jar in jars:
        jar.location.x = 0.0
