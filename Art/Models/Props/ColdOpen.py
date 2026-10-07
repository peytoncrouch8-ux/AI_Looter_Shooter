"""Ransom's Rest, the cold open: props that make the gang's flat-black mannequins read as a Western gang against the
sunset (Docs/Areas/RansomsRest.md: Main 1, and the art needs' "Cold open silhouettes"; Docs/Story.md: Cold open).
A scripted model file (see Art/README.md): twelve small models, each one material on the silhouette master.

They are only ever seen as black shapes against a bright sky, so the outline is everything and interior detail is
wasted: closed solids, exaggerated where it reads (brims, hammers, the lever and trigger-guard holes, the scope), a few
hundred triangles each. Every model has one slot, Silhouette (Master Backdrop, Tint #161312, which is the code's
SilhouetteColor (0.008, 0.0065, 0.006) linear, Brightness 1, MaxHaze 0), Nanite 0, no LODs (a reduced brim would lose
its outline, and each is a few hundred triangles), and Collision None. AColdOpenCast's PaintBlack puts every slot on its
own unlit black anyway. UV 0 is a plain box projection (only so Unreal's tangents have area); UV 1 ('Depth') is zero,
so M_Backdrop's haze, which reads UV 1, never greys them.

  Hats (origin: the hat band's centre, on the plane where the brim meets the head; front -Y)
  ColdOpen_HatSlouch     wide soft brim drooping front and back, dented crown      "Whistling" Ira Gale (and Lena)
  ColdOpen_HatGambler    tall flat-topped crown, brim rolled up at the sides       "Lucky" Ned Purcell
  ColdOpen_HatBowler     tall round dome, short rolled brim                        Bartholomew "Barrels" Kessler
  ColdOpen_HatPreacher   low round crown, very wide dead-flat brim                 Ambrose Dunne, the Deacon
  ColdOpen_Veil          a nun's veil falling to the shoulders' edges, open at     Sister Constance Holloway
                         the face
  ColdOpen_FlatCap       a working man's flat cap, crown pulled over a short peak  Tobias "Mule" Grant
  Guns (origin: the grip, where the right hand closes; bore along -Y, which is Unreal's +X; top +Z; SOCKET_Muzzle at
  the bore's end, for the muzzle flash)
  ColdOpen_Revolver      single-action, 7.5 inch barrel                            Ned (he shoots Abel)
  ColdOpen_CoachGun      side-by-side coach gun, short fat barrels, twin hammers   the Deacon (Last Rites), Barrels, Mule
  ColdOpen_LeverRifle    lever-action carbine, open finger loop                    Ira (Whistler), Constance (Mercy)
  ColdOpen_LongRifle     long-range rifle with a long tube scope                   Lena "Spyglass" Okoro (Iris)
  Props
  ColdOpen_PowderKeg     a small powder keg with a bung and a curled fuse; origin  Barrels, on his back
                         where it rests (the middle of its belly), axis along -Y
  ColdOpen_MedicBag      a doctor's bag, origin in the handle (the fist), hanging  Constance, in her left hand

Attaching them to the UE5 mannequin (SKM_Manny_Simple, skeleton SK_Mannequin). The bone frames and the HandGrip sockets
below were read out of Content/Characters/Mannequins/Meshes/SK_Mannequin.uasset (its reference pose and its socket
objects), and the head and torso sizes out of SKM_Manny_Simple's vertices (180.6 cm tall; the head 17.6 cm wide and
22 cm deep at the brow). Its bones don't run like Blender's axes: head and spine_05 point X up the spine, Y forward
(the face) and Z to the figure's right; the right arm's bones point -X down the arm (toward the fingers), the left's +X.
Both HandGrip sockets point X across the palm, Y down the fingers and Z along the thumb.
Every transform below is a component's relative transform on that bone or socket (Unreal: cm, Pitch Yaw Roll), as
AttachToComponent with KeepRelativeTransform takes it. The script works them out from the frames and prints them
(ATTACH lines), and the preview puts every prop on its posed stand-in through them:
  worn      every hat     bone head          (10.43, 2.04, 0)        (0, 90, -90)   band just above the brow
  aim       every gun     socket HandGrip_R  (0, 0, 0)               (0, 90, 0)     bore down the fingers, as PoseAim
                          (bone hand_r       (-7.01, 2.05, 0)        (0, 180, 0))   points them at the target
  carry     long guns     socket HandGrip_R  (0, 0, 0)               (75, 90, 0)    at ease: level at the hip, muzzle
                          (bone hand_r       (-7.01, 2.05, 0)        (75, 180, 0))  forward and about 6 degrees down
  sling     coach gun     bone spine_05      (-29.58, -22.07, 8.60)  (-30, 10.1, 0) across the back, muzzle up over
            lever rifle                      (-35.71, -23.17, 12.20)                the left shoulder
            long rifle                       (-43.13, -24.50, 16.55)
  back      keg           bone spine_05      (-1.48, -15.02, 0)      (0, 10.1, -90) standing on the shoulder blades
  shoulder  keg           bone spine_05      (10.01, 0.24, -19.00)   (0, 100.1, -90) on the left shoulder (reads only
                                                                                     from the front)
  hand      medic bag     socket HandGrip_L  (0, 0, 0)               (90, 0, -90)   hanging down the fingers
                          (bone hand_l       (7.52, -2.53, 0)        (-90, 180, 0))

The cold open's poses (AColdOpenCast: PoseAtEase, PoseAim, PoseHoldingEmber) were run here on the same skeleton. With
'aim' at ease a long gun hangs straight down beside the leg and reads as a stick (the long rifle would dig into the
ground), so long guns take 'carry' or 'sling' at ease and switch to 'aim' when the code raises the arm. Ned's revolver
on 'aim' points down his arm about 10 degrees off upright. Its muzzle is 35 cm out along the hand from hand_r and 6.6 cm
above it, where Fire() puts the flash 22 cm out: read SOCKET_Muzzle for the flash instead. The preview's stand-ins are
built from the decoded joints and posed with the same AimBone steps, so they show what the code will.

Run with --preview for Saved/ArtPreviews/RansomsRest/ColdOpen/ColdOpen_silhouettes.png (the props against a sunset, and
the gang as stand-ins on a ridge), ColdOpen_props_clay.png (grey, for the shapes) and the two panels on their own.
"""
import math
import os

import bmesh
import bpy
from mathutils import Matrix, Quaternion, Vector

import looter_textures as lt

SILHOUETTE_HEX = 0x161312


# ------------------------------------------------------------------------------------------------------------------
# Building: shapes in Unreal's frame of the model (forward, right, up), in centimetres
# ------------------------------------------------------------------------------------------------------------------

class Shape:
    """Closed parts as vertices (forward, right, up) in cm and faces (any winding; finish() makes them face out)."""

    def __init__(self):
        self.verts = []
        self.faces = []

    def part(self, verts, faces):
        base = len(self.verts)
        self.verts.extend(Vector(v) for v in verts)
        self.faces.extend(tuple(base + i for i in f) for f in faces)
        return base

    def move(self, offset, start=0):
        """Shifts every vertex from index start on."""
        off = Vector(offset)
        for i in range(start, len(self.verts)):
            self.verts[i] = self.verts[i] + off

    def turn(self, matrix, start=0):
        """Turns every vertex from index start on by a 3x3 matrix about the origin."""
        for i in range(start, len(self.verts)):
            self.verts[i] = matrix @ self.verts[i]


def lathe(shape, profile, segments, stretch_f=1.0, stretch_r=1.0, center=(0.0, 0.0, 0.0), warp=None):
    """A solid of revolution about the up axis. profile: [(radius, up)] from the top down; radius 0 at an end makes a
    pole. stretch_f / stretch_r lengthen it along forward / right (an oval hat). warp(angle, radius, up) returns
    (radius scale, up offset, forward offset) for a vertex, angle 0 being the front: brims droop and curl with it."""
    cf, cr, cu = center
    verts, rings = [], []
    for rad, up in profile:
        if rad <= 0.0:
            _, dup, dfwd = warp(0.0, 0.0, up) if warp else (1.0, 0.0, 0.0)
            rings.append([len(verts)])
            verts.append((cf + dfwd, cr, cu + up + dup))
            continue
        ring = []
        for k in range(segments):
            angle = 2.0 * math.pi * k / segments
            scale, dup, dfwd = warp(angle, rad, up) if warp else (1.0, 0.0, 0.0)
            ring.append(len(verts))
            verts.append((cf + dfwd + rad * scale * stretch_f * math.cos(angle),
                          cr + rad * scale * stretch_r * math.sin(angle), cu + up + dup))
        rings.append(ring)
    faces = []
    for a, b in zip(rings, rings[1:]):
        if len(a) == 1 and len(b) == 1:
            continue
        if len(a) == 1:
            faces += [(a[0], b[k], b[(k + 1) % segments]) for k in range(segments)]
        elif len(b) == 1:
            faces += [(a[k], b[0], a[(k + 1) % segments]) for k in range(segments)]
        else:
            faces += [(a[k], b[k], b[(k + 1) % segments], a[(k + 1) % segments]) for k in range(segments)]
    return shape.part(verts, faces)


def tube(shape, path, radius, sides=6, closed=False, normal=(0.0, 0.0, 1.0), phase=0.5):
    """A tube along path [(forward, right, up)]. radius: a number, (a, b) for an ellipse, or a list of either per point.
    Its sections are turned by a fixed normal (keep it off the path's direction), so a loop in a plane has no twist; a
    loop's normal is its plane's. Open tubes are capped."""
    pts = [Vector(p) for p in path]
    n = Vector(normal).normalized()
    count = len(pts)
    verts = []
    for i, p in enumerate(pts):
        if closed:
            t = pts[(i + 1) % count] - pts[i - 1]
        else:
            t = pts[min(i + 1, count - 1)] - pts[max(i - 1, 0)]
        t.normalize()
        side = t.cross(n)
        if side.length < 1e-6:
            side = t.orthogonal()
        side.normalize()
        other = side.cross(t).normalized()
        rad = radius[i] if isinstance(radius, list) else radius
        ra, rb = rad if isinstance(rad, tuple) else (rad, rad)
        for k in range(sides):
            a = 2.0 * math.pi * (k + phase) / sides
            verts.append(p + side * (ra * math.cos(a)) + other * (rb * math.sin(a)))
    faces = []
    for i in range(count if closed else count - 1):
        j = (i + 1) % count
        for k in range(sides):
            k2 = (k + 1) % sides
            faces.append((i * sides + k, j * sides + k, j * sides + k2, i * sides + k2))
    if not closed:
        faces.append(tuple(range(sides)))
        faces.append(tuple((count - 1) * sides + k for k in reversed(range(sides))))
    return shape.part(verts, faces)


def ring(center_f, center_u, radius_f, radius_u, count, start=0.0):
    """Points of an ellipse in the forward/up plane (a trigger guard or a lever loop's path)."""
    return [(center_f + radius_f * math.cos(start + 2.0 * math.pi * k / count), 0.0,
             center_u + radius_u * math.sin(start + 2.0 * math.pi * k / count)) for k in range(count)]


def prism(shape, outline, half, right=0.0, taper=None):
    """A side outline [(forward, up)] (a simple polygon) cut out across from right - half to right + half.
    taper(forward, up) gives the half-thickness there instead (linear in up keeps the caps flat)."""
    count = len(outline)
    verts = []
    for side in (-1.0, 1.0):
        for f, u in outline:
            h = taper(f, u) if taper else half
            verts.append((f, right + side * h, u))
    faces = [tuple(range(count)), tuple(range(2 * count - 1, count - 1, -1))]
    faces += [(i, (i + 1) % count, count + (i + 1) % count, count + i) for i in range(count)]
    return shape.part(verts, faces)


def block(shape, f0, f1, u0, u1, half, right=0.0):
    return prism(shape, [(f0, u0), (f1, u0), (f1, u1), (f0, u1)], half, right)


def slab(shape, inner, outer, thickness):
    """A thin solid between two polylines of the same length (a cap's peak): its top through them, its underside
    thickness lower."""
    count = len(inner)
    top = [Vector(p) for p in inner] + [Vector(p) for p in reversed(outer)]
    loop = len(top)
    verts = top + [p - Vector((0.0, 0.0, thickness)) for p in top]
    faces = []
    for k in range(count - 1):
        a, b = k, k + 1
        c, d = loop - 2 - k, loop - 1 - k
        faces.append((a, b, c, d))
        faces.append((loop + d, loop + c, loop + b, loop + a))
    for k in range(loop):
        k2 = (k + 1) % loop
        faces.append((k, loop + k, loop + k2, k2))
    return shape.part(verts, faces)


def to_blender(v):
    """Unreal's frame of the model (forward, right, up; cm) to Blender's (front -Y, right -X; m), as the exporter
    reverses it."""
    return Vector((-v[1] / 100.0, -v[0] / 100.0, v[2] / 100.0))


def silhouette_material():
    """M_Backdrop: unlit, a flat tint x Brightness x the lighting state's BackdropTint (black stays black)."""
    name = 'Silhouette'
    mat = bpy.data.materials.get(name)
    if mat is not None:
        return mat
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs['Color'].default_value = lt.hex_color(SILHOUETTE_HEX)
    emit.inputs['Strength'].default_value = 1.0
    links.new(emit.outputs['Emission'], out.inputs['Surface'])
    mat.diffuse_color = lt.hex_color(SILHOUETTE_HEX)
    mat['Master'] = 'Backdrop'
    mat['Tint'] = '#%06x' % SILHOUETTE_HEX
    mat['Brightness'] = 1.0
    mat['MaxHaze'] = 0.0
    return mat


def finish(name, shape, sockets=()):
    """The model: faces turned out, triangulated, box-projected UV 0 and a zero UV 1, the silhouette slot, smooth
    (it's unlit: shared normals just keep the vertex count down), no Nanite, no collision. sockets: [(name, (f, r, u))]
    in cm, facing the model's front."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    verts = [bm.verts.new(to_blender(v)) for v in shape.verts]
    for face in shape.faces:
        bm.faces.new([verts[i] for i in face])
    bm.faces.ensure_lookup_table()
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method='BEAUTY', ngon_method='BEAUTY')
    uv0 = bm.loops.layers.uv.new('UVMap')
    uv1 = bm.loops.layers.uv.new('Depth')
    x, y, z = Vector((1.0, 0.0, 0.0)), Vector((0.0, 1.0, 0.0)), Vector((0.0, 0.0, 1.0))
    for face in bm.faces:
        n = face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        if axis == 2:
            u_axis, v_axis = x, (y if n.z > 0.0 else -y)
        elif axis == 0:
            u_axis, v_axis = (y if n.x > 0.0 else -y), z
        else:
            u_axis, v_axis = (-x if n.y > 0.0 else x), z
        for loop in face.loops:
            co = loop.vert.co
            loop[uv0].uv = (co.dot(u_axis) * 4.0, co.dot(v_axis) * 4.0)
            loop[uv1].uv = (0.0, 0.0)
    bm.to_mesh(mesh)
    bm.free()
    mesh.polygons.foreach_set('use_smooth', [True] * len(mesh.polygons))
    mesh.materials.append(silhouette_material())
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    for socket_name, where in sockets:
        empty = bpy.data.objects.new('SOCKET_' + socket_name, None)
        empty.empty_display_size = 0.03
        empty.parent = obj
        empty.location = to_blender(where)
        bpy.context.scene.collection.objects.link(empty)
    return obj


# ------------------------------------------------------------------------------------------------------------------
# Hats: the origin is the band's centre where the brim meets the head; crowns are oval (STRETCH longer front to back)
# ------------------------------------------------------------------------------------------------------------------

BAND = 9.9      # the crown's base, half its width (cm): the mannequin's head is 8.8 at the brow
STRETCH = 1.22  # front to back: 24 cm over the 22 cm head


def brim_warp(band, brim, droop_front, droop_back, curl_sides):
    """A brim that bends more toward its edge: down at the front and back, up at the sides (negative droop lifts)."""
    def warp(angle, rad, up):
        if rad <= band + 1e-3:
            return 1.0, 0.0, 0.0
        t = (rad - band) / (brim - band)
        c, s = math.cos(angle), math.sin(angle)
        droop = (droop_front if c > 0.0 else droop_back) * c * c - curl_sides * s * s
        return 1.0, -droop * t * t, 0.0
    return warp


def hat_slouch():
    """A wide, soft brim drooping front and back over a dented crown pinched a little at the front."""
    s = Shape()
    brim = 21.8
    droop = brim_warp(BAND, brim, 4.4, 3.4, 1.6)

    def warp(angle, rad, up):
        if rad <= BAND + 1e-3 and up > 8.0:
            # The crown's front pinched in toward its top: a soft cattleman's crease.
            return 1.0 - 0.13 * max(0.0, math.cos(angle)) * (up - 8.0) / 4.4, 0.0, 0.0
        return droop(angle, rad, up)
    lathe(s, [(0.0, 10.6), (5.6, 11.7), (8.6, 12.4), (9.4, 11.3), (BAND, 1.0),
              (15.6, 0.85), (brim, 0.6), (brim + 0.3, 0.2), (brim, -0.2), (15.6, 0.0), (BAND, 0.0), (0.0, 0.0)],
          18, stretch_f=STRETCH, warp=warp)
    return s


def hat_gambler():
    """A tall, flat-topped, straight-sided crown with a firm brim rolled up hard at the sides."""
    s = Shape()
    brim = 18.4
    lathe(s, [(0.0, 12.6), (9.0, 12.6), (9.5, 12.1), (BAND, 1.0), (14.0, 0.85), (brim, 0.65), (brim + 0.25, 0.3),
              (brim, -0.05), (14.0, 0.0), (BAND, 0.0), (0.0, 0.0)],
          18, stretch_f=STRETCH, warp=brim_warp(BAND, brim, 0.4, 0.4, 3.6))
    return s


def hat_bowler():
    """A tall round dome over a short brim rolled up at the sides and dipping front and back."""
    s = Shape()
    brim = 14.4
    lathe(s, [(0.0, 13.8), (5.2, 13.2), (8.4, 11.0), (9.8, 7.0), (BAND + 0.05, 1.0), (12.6, 0.9), (brim, 1.6),
              (brim + 0.4, 1.0), (brim, 0.35), (12.6, 0.0), (BAND, 0.0), (0.0, 0.0)],
          18, stretch_f=1.18, warp=brim_warp(BAND + 0.05, brim + 0.4, 0.9, 0.9, 2.4))
    return s


def hat_preacher():
    """A low round crown and a very wide, dead-flat brim: the Deacon's line against the sky."""
    s = Shape()
    brim = 22.6
    lathe(s, [(0.0, 9.2), (5.6, 8.8), (8.7, 7.0), (9.8, 3.8), (BAND, 0.8), (brim, 0.65), (brim + 0.25, 0.32),
              (brim, 0.0), (BAND, 0.0), (0.0, 0.0)],
          20, stretch_f=1.16)
    return s


def veil():
    """A nun's veil: a close crown over the head with a squared front band, then a stiff veil falling round the back
    and sides to the shoulders' edges, flaring and standing off the back like a short cape, open at the front so the
    face's profile still shows."""
    s = Shape()
    lathe(s, [(0.0, 8.6), (5.5, 8.3), (8.6, 7.0), (10.0, 4.2), (10.3, -1.0), (0.0, -1.0)], 16, stretch_f=1.2)
    # The drape: C-shaped sections round the back (angle 0 is the front), each a band 1.4 cm thick. It hugs the head
    # to the jaw, then flares to the shoulders' edges; its front edges draw back so the face and chin show side on.
    levels = [  # (up, half width, centre forward, front opening half-angle in degrees)
        (5.0, 10.4, 0.0, 38.0), (-4.0, 11.0, -0.8, 64.0), (-14.0, 12.4, -2.6, 76.0),
        (-24.0, 15.6, -4.6, 81.0), (-34.0, 19.5, -6.6, 84.0), (-44.0, 22.0, -8.2, 86.0)]
    arc = 9
    verts = []
    for up, half, cf, opening in levels:
        a0 = math.radians(opening)
        angles = [a0 + (2.0 * math.pi - 2.0 * a0) * k / (arc - 1) for k in range(arc)]
        outer = [(cf + half * 1.1 * math.cos(a), half * math.sin(a), up) for a in angles]
        inner = [(cf + (half - 1.4) * 1.1 * math.cos(a), (half - 1.4) * math.sin(a), up) for a in reversed(angles)]
        verts += outer + inner
    loop = 2 * arc
    faces = []
    for li in range(len(levels) - 1):
        for k in range(loop):
            k2 = (k + 1) % loop
            faces.append((li * loop + k, (li + 1) * loop + k, (li + 1) * loop + k2, li * loop + k2))
    faces.append(tuple(range(loop)))
    faces.append(tuple((len(levels) - 1) * loop + k for k in reversed(range(loop))))
    s.part(verts, faces)
    return s


def flat_cap():
    """A flat cap: a puffed crown pulled forward and down over a short, stiff peak."""
    s = Shape()

    def warp(angle, rad, up):
        if up <= 2.0:
            return 1.0, 0.0, 0.0
        pull = (up - 2.0) / 7.6
        front_down = -2.2 * math.cos(angle) * (rad / 11.2) if up > 5.0 else 0.0
        return 1.0, front_down, 2.0 * pull
    lathe(s, [(0.0, 9.6), (7.0, 9.3), (10.6, 7.6), (11.2, 5.2), (10.6, 2.2), (10.0, 0.6), (0.0, 0.6)],
          18, stretch_f=1.25, warp=warp)
    # The peak: a crescent from the band's front, sloping down.
    angles = [math.radians(-64.0 + 128.0 * k / 8) for k in range(9)]
    inner = [(10.0 * 1.25 * math.cos(a) - 0.6, 10.0 * math.sin(a), 1.2) for a in angles]
    outer = []
    for k, a in enumerate(angles):
        reach = 0.12 + 0.88 * math.sin(math.pi * k / 8) ** 0.6  # the peak's tips run back into the band
        outer.append((10.0 * 1.25 * math.cos(a) + 5.8 * reach * math.cos(a) - 0.6, (10.0 + 1.6 * reach) * math.sin(a),
                      1.2 - 1.5 * reach))
    slab(s, inner, outer, 0.6)
    return s


# ------------------------------------------------------------------------------------------------------------------
# Guns: drawn with the bore on up = 0, then moved so the grip (the fist's centre) is the origin
# ------------------------------------------------------------------------------------------------------------------

def gun_finish(name, s, grip, muzzle_f):
    """Moves the grip to the origin and adds SOCKET_Muzzle at the bore's end."""
    s.move((-grip[0], 0.0, -grip[1]))
    return finish(name, s, sockets=[('Muzzle', (muzzle_f - grip[0], 0.0, -grip[1]))])


def revolver():
    s = Shape()
    along = (0.0, 0.0, 1.0)
    tube(s, [(4.8, 0.0, 0.0), (24.0, 0.0, 0.0)], 0.95, sides=8, normal=along)                 # barrel
    tube(s, [(4.8, 0.0, -1.55), (16.0, 0.0, -1.55)], 0.55, sides=6, normal=along)             # ejector housing
    block(s, 23.0, 23.8, 0.6, 1.6, 0.18)                                                       # front sight
    tube(s, [(-0.4, 0.0, -0.25), (4.4, 0.0, -0.25)], 1.95, sides=10, normal=along)            # cylinder
    prism(s, [(4.8, 1.0), (4.8, -2.2), (4.3, -2.6), (-0.2, -2.6), (-1.3, -3.6), (-1.9, -6.5), (-2.3, -9.6),
              (-3.0, -11.6), (-5.4, -11.9), (-6.9, -11.2), (-6.6, -8.4), (-5.6, -5.4), (-4.0, -2.5), (-3.0, -1.0),
              (-3.6, 1.6), (-2.9, 2.5), (-2.2, 2.1), (-1.5, 1.2), (-0.4, 1.35)], 1.05)       # frame, grip, hammer
    tube(s, ring(0.9, -3.9, 1.7, 1.35, 10), 0.32, sides=4, closed=True, normal=(0.0, 1.0, 0.0))  # trigger guard
    return gun_finish('ColdOpen_Revolver', s, (-4.0, -6.6), 24.0)


def lever_rifle():
    s = Shape()
    along = (0.0, 0.0, 1.0)
    tube(s, [(16.0, 0.0, 0.0), (66.0, 0.0, 0.0)], 0.95, sides=8, normal=along)                # barrel
    tube(s, [(16.0, 0.0, -1.85), (60.0, 0.0, -1.85)], 0.78, sides=6, normal=along)            # magazine tube
    block(s, 64.6, 65.4, 0.7, 1.7, 0.2)                                                        # front sight
    block(s, 27.6, 28.6, 0.6, 1.5, 0.45)                                                       # rear sight
    prism(s, [(16.0, 0.4), (31.0, 0.4), (32.2, -0.5), (32.2, -2.7), (31.0, -3.1), (16.0, -3.1)], 1.55)  # forearm
    prism(s, [(16.0, 1.0), (16.0, -3.3), (-2.0, -3.6), (-2.6, -1.2), (-2.0, 1.6), (12.5, 1.9)], 1.5)    # receiver
    prism(s, [(-1.6, 1.4), (-2.6, 3.0), (-3.6, 3.4), (-3.4, 2.6), (-2.6, 1.0)], 0.5)                   # hammer
    prism(s, [(-2.0, 1.2), (-12.0, -0.9), (-20.0, -0.9), (-36.0, -1.9), (-36.8, -2.6), (-37.0, -6.8),
              (-36.6, -11.6), (-34.5, -11.8), (-14.0, -5.6), (-7.0, -4.1), (-2.0, -3.6)], 1.75)     # stock
    tube(s, ring(-3.5, -5.6, 5.5, 1.6, 12), 0.42, sides=4, closed=True, normal=(0.0, 1.0, 0.0))  # lever loop
    return gun_finish('ColdOpen_LeverRifle', s, (-10.0, -3.3), 66.0)


def long_rifle():
    s = Shape()
    along = (0.0, 0.0, 1.0)
    tube(s, [(15.0, 0.0, 0.0), (86.0, 0.0, 0.0)], 1.25, sides=8, normal=along)                # octagon barrel
    block(s, 84.6, 85.6, 1.0, 2.0, 0.2)                                                        # front sight
    prism(s, [(15.0, 0.4), (36.0, 0.4), (38.5, -0.6), (38.0, -2.6), (35.5, -3.2), (15.0, -3.3)], 1.75)  # forearm
    prism(s, [(15.0, 1.2), (15.0, -3.6), (-1.5, -4.0), (-2.4, -1.4), (-1.6, 1.5)], 1.6)                 # receiver
    prism(s, [(-1.0, 0.8), (-2.4, 2.6), (-3.5, 2.9), (-3.3, 2.1), (-2.1, 0.4)], 0.5, right=1.0)        # side hammer
    prism(s, [(-1.6, 1.2), (-12.5, -1.0), (-22.0, -1.0), (-39.0, -2.0), (-39.6, -3.0), (-39.8, -7.6),
              (-39.2, -12.0), (-37.0, -12.2), (-15.0, -6.0), (-7.5, -4.4), (-1.5, -4.0)], 1.8)      # stock
    tube(s, ring(-1.0, -5.4, 3.6, 1.4, 10), 0.4, sides=4, closed=True, normal=(0.0, 1.0, 0.0))  # breech lever
    # The long tube scope, from over the wrist to past the middle of the barrel, on two mounts.
    tube(s, [(-6.0, 0.0, 3.6), (52.0, 0.0, 3.6)], 0.95, sides=8, normal=along)
    tube(s, [(-11.5, 0.0, 3.6), (-6.0, 0.0, 3.6)], 1.35, sides=8, normal=along)               # eyepiece
    tube(s, [(52.0, 0.0, 3.6), (56.0, 0.0, 3.6), (59.5, 0.0, 3.6)], [1.0, 1.55, 1.6], sides=8, normal=along)
    block(s, 3.0, 5.5, 1.2, 3.2, 0.6)                                                          # mounts
    block(s, 40.0, 42.5, 1.0, 3.2, 0.6)
    return gun_finish('ColdOpen_LongRifle', s, (-10.0, -3.3), 86.0)


def coach_gun():
    """Short, fat twin barrels and two tall hammer spurs: stubbier than the rifles, so it reads as a shotgun."""
    s = Shape()
    along = (0.0, 0.0, 1.0)
    for side in (-1.3, 1.3):
        tube(s, [(13.0, side, 0.0), (49.0, side, 0.0)], 1.3, sides=8, normal=along)           # barrels
    prism(s, [(13.0, -0.8), (27.0, -0.8), (28.0, -1.5), (27.0, -2.7), (13.0, -2.9)], 2.1)              # forend
    prism(s, [(13.0, 1.2), (13.0, -3.0), (10.0, -3.8), (-2.2, -3.8), (-2.8, -1.4), (-2.2, 1.4)], 2.4)  # action
    for side in (-1.4, 1.4):
        prism(s, [(-0.6, 1.2), (-1.6, 3.4), (-2.9, 4.6), (-3.4, 4.1), (-2.4, 1.0)], 0.5, right=side)  # hammers
    prism(s, [(-2.2, 1.2), (-10.5, -0.9), (-17.0, -1.0), (-31.0, -2.0), (-31.6, -3.0), (-31.6, -12.0),
              (-29.5, -12.0), (-13.0, -5.8), (-6.5, -4.3), (-2.2, -3.8)], 1.9)                 # stock
    tube(s, ring(2.6, -4.8, 2.4, 1.3, 10), 0.36, sides=4, closed=True, normal=(0.0, 1.0, 0.0))  # trigger guard
    return gun_finish('ColdOpen_CoachGun', s, (-8.5, -3.0), 49.0)


# ------------------------------------------------------------------------------------------------------------------
# Props
# ------------------------------------------------------------------------------------------------------------------

def powder_keg():
    """A small powder keg lying along forward, its belly's bottom at the origin (where it rests on a shoulder), the bung
    on top with a curled fuse."""
    s = Shape()
    # Narrow heads and a fat belly: the barrel's outline is the whole read.
    start = lathe(s, [(0.0, 15.0), (7.9, 15.0), (9.3, 14.3), (10.5, 11.8), (11.6, 6.2), (12.2, 0.0),
                      (11.6, -6.2), (10.5, -11.8), (9.3, -14.3), (7.9, -15.0), (0.0, -15.0)], 14)
    # Built about up; turned so its axis runs forward (forward <- up, up <- -forward), then lifted onto its belly.
    s.turn(Matrix(((0.0, 0.0, 1.0), (0.0, 1.0, 0.0), (-1.0, 0.0, 0.0))), start)
    s.move((0.0, 0.0, 12.2), start)
    tube(s, [(0.0, 0.0, 23.6), (0.0, 0.0, 25.3)], 1.6, sides=6, normal=(1.0, 0.0, 0.0))        # bung
    tube(s, [(0.0, 0.0, 25.0), (0.5, 0.0, 27.8), (2.2, 0.0, 30.0), (4.5, 0.0, 30.8), (6.3, 0.0, 29.7)], 0.33,
         sides=4, normal=(0.0, 1.0, 0.0))                                                      # fuse
    return finish('ColdOpen_PowderKeg', s)


def medic_bag():
    """A doctor's bag hanging from its handle: the handle's bar through the origin (the fist), the frame's narrow top
    widening to a flat bottom, so it reads as a bag from the front and the side."""
    s = Shape()
    top, bottom = -6.0, -28.0
    prism(s, [(-17.5, top), (17.5, top), (18.0, -7.0), (18.0, -25.5), (16.5, bottom), (-16.5, bottom),
              (-18.0, -25.5), (-18.0, -7.0)], 0.0,
          taper=lambda f, u: 3.2 + (8.0 - 3.2) * (top - u) / (top - bottom))
    tube(s, [(-17.0, 0.0, top), (17.0, 0.0, top)], 0.9, sides=6, normal=(0.0, 0.0, 1.0))      # the frame's jaw
    tube(s, [(-6.0, 0.0, -6.0), (-5.4, 0.0, -2.2), (-3.0, 0.0, 0.3), (3.0, 0.0, 0.3), (5.4, 0.0, -2.2), (6.0, 0.0, -6.0)],
         0.95, sides=6, normal=(0.0, 1.0, 0.0))                                                # handle
    return finish('ColdOpen_MedicBag', s)


HATS = [('ColdOpen_HatSlouch', hat_slouch), ('ColdOpen_HatGambler', hat_gambler), ('ColdOpen_HatBowler', hat_bowler),
        ('ColdOpen_HatPreacher', hat_preacher), ('ColdOpen_Veil', veil), ('ColdOpen_FlatCap', flat_cap)]
models = {}
for hat_name, build in HATS:
    models[hat_name] = finish(hat_name, build())
for gun in (revolver, coach_gun, lever_rifle, long_rifle):
    obj = gun()
    models[obj.name] = obj
for prop in (powder_keg, medic_bag):
    obj = prop()
    models[obj.name] = obj


def report():
    for name, obj in models.items():
        lo = [min(v.co[i] for v in obj.data.vertices) for i in range(3)]
        hi = [max(v.co[i] for v in obj.data.vertices) for i in range(3)]
        size = ' x '.join(f'{(hi[i] - lo[i]) * 100.0:.1f}' for i in (1, 0, 2))
        print(f'COLDOPEN {name:<22} {len(obj.data.polygons):4d} tris, {size} cm (length x width x height)', flush=True)


report()


# ------------------------------------------------------------------------------------------------------------------
# The mannequin (for the attach transforms and the preview): UE5 Manny's reference pose in component space (cm,
# Unreal's axes: the figure faces +Y, its left is +X), and its two grip sockets, read out of SK_Mannequin.uasset.
# ------------------------------------------------------------------------------------------------------------------

MANNY = {  # bone: (component position, component rotation as a quaternion w, x, y, z)
    'pelvis': ((0.0, 2.281, 95.897), (0.706751, 0.022415, -0.706751, -0.022415)),
    'spine_01': ((0.0, 2.048, 99.566), (0.703955, -0.066693, -0.703955, 0.066693)),
    'spine_03': ((0.0, 4.251, 113.419), (0.706761, 0.022126, -0.70676, -0.022126)),
    'spine_05': ((0.0, 0.525, 141.102), (0.704343, 0.062456, -0.704343, -0.062456)),
    'neck_01': ((0.0, -1.567, 152.804), (0.70199, -0.084911, -0.70199, 0.084911)),
    'head': ((0.0, 0.663, 162.575), (0.707107, 0.0, -0.707107, 0.0)),
    'clavicle_l': ((1.428, -1.74, 146.301), (0.996801, -3.9e-05, 0.076541, -0.02302)),
    'upperarm_l': ((19.01, -2.557, 143.584), (0.887182, 0.030411, 0.460409, -0.002586)),
    'lowerarm_l': ((35.007, -1.907, 120.892), (0.837212, 0.182276, 0.423883, 0.293555)),
    'hand_l': ((47.769, 15.699, 104.467), (0.59846, 0.608875, 0.5206, -0.009649)),
    'clavicle_r': ((-1.428, -1.74, 146.301), (3.9e-05, 0.996801, 0.02302, 0.076541)),
    'upperarm_r': ((-19.01, -2.557, 143.583), (0.030411, -0.887182, -0.002586, -0.460409)),
    'lowerarm_r': ((-35.007, -1.907, 120.892), (0.182276, -0.837212, 0.293555, -0.423883)),
    'hand_r': ((-47.768, 15.699, 104.466), (0.608874, -0.59846, -0.009649, -0.5206)),
    'thigh_l': ((9.969, 2.541, 93.543), (0.685741, -0.04997, -0.72414, -0.053695)),
    'calf_l': ((12.332, 2.596, 50.266), (0.687432, -0.08154, -0.721268, -0.023704)),
    'foot_l': ((14.088, -0.994, 8.238), (0.705887, -0.063632, -0.704247, -0.041376)),
    'ball_l': ((15.778, 13.929, 0.751), (0.528394, -0.542972, -0.452983, 0.46988)),
    'thigh_r': ((-9.969, 2.55, 93.543), (0.04997, 0.685741, 0.053695, -0.72414)),
    'calf_r': ((-12.332, 2.605, 50.267), (0.08154, 0.687432, 0.023704, -0.721268)),
    'foot_r': ((-14.088, -0.985, 8.238), (0.063632, 0.705887, 0.041376, -0.704247)),
    'ball_r': ((-15.778, 13.938, 0.751), (0.542972, 0.528394, -0.46988, -0.452983)),
}
GRIP_SOCKETS = {  # socket: (bone, relative location, relative rotation (pitch, yaw, roll))
    'HandGrip_R': ('hand_r', (-7.012, 2.049, 0.0), (0.0, 90.0, 0.0)),
    'HandGrip_L': ('hand_l', (7.521, -2.529, 0.0), (0.0, 90.0, 180.0)),
}
ARM_CHAIN = {'upperarm_l': ('upperarm_l', 'lowerarm_l', 'hand_l'), 'lowerarm_l': ('lowerarm_l', 'hand_l'),
             'upperarm_r': ('upperarm_r', 'lowerarm_r', 'hand_r'), 'lowerarm_r': ('lowerarm_r', 'hand_r')}

# Where each prop goes in the reference pose (component space): its origin, its forward and up axes.
HAT_SPOT = ((0.0, 2.7, 173.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))  # the band level, just above the brow
# The keg on the back, standing up, its belly against the shoulder blades and its top at the shoulders (it reads in
# profile and three-quarter views); or lying on the left shoulder's top, axis forward (it reads only from the front:
# side on, it merges with the head).
KEG_BACK = ((0.0, -14.0, 137.0), (0.0, 0.0, 1.0), (0.0, -1.0, 0.0))
KEG_SHOULDER = ((19.0, -1.0, 151.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))
SLING_CENTER = Vector((0.0, -16.0, 123.0))  # the middle of a slung gun, standing off the back
SLING_TILT = 30.0  # degrees from upright, the muzzle toward the figure's left shoulder
# Each long gun's middle along its bore, from its grip (cm): the sling hangs it by its middle.
LONG_GUNS = {'ColdOpen_LeverRifle': 24.4, 'ColdOpen_LongRifle': 33.1, 'ColdOpen_CoachGun': 17.2}
# Carried at ease: the bore along the thumb (forward when the arm hangs), dipped this far so the muzzle points a little
# down, the gun's top toward the wrist.
CARRY_DIP = 15.0


def frotator_matrix(pitch, yaw, roll):
    """Unreal's FRotationMatrix as a column matrix (columns: where X, Y and Z go)."""
    p, y, r = (math.radians(a) for a in (pitch, yaw, roll))
    sp, cp, sy, cy, sr, cr = math.sin(p), math.cos(p), math.sin(y), math.cos(y), math.sin(r), math.cos(r)
    return Matrix(((cp * cy, cp * sy, sp), (sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp),
                   (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp))).transposed()


def matrix_frotator(m):
    """Unreal's FMatrix::Rotator: (pitch, yaw, roll) in degrees."""
    m = m.to_3x3().normalized()
    x, y, z = m.col[0], m.col[1], m.col[2]
    pitch = math.degrees(math.atan2(x.z, math.sqrt(x.x * x.x + x.y * x.y)))
    yaw = math.degrees(math.atan2(x.y, x.x))
    sy_axis = frotator_matrix(pitch, yaw, 0.0).col[1]
    roll = math.degrees(math.atan2(z.dot(sy_axis), y.dot(sy_axis)))
    return pitch, yaw, roll


def transform(rotation, location, scale=1.0):
    m = (rotation.to_3x3() if isinstance(rotation, Matrix) else rotation.to_matrix()).to_4x4()
    m = m @ Matrix.Scale(scale, 4)
    m.translation = Vector(location)
    return m


def frame(origin, forward, up):
    """A transform from an origin and forward and up axes; its Y (Unreal's right) is the algebraic Z x X."""
    x = Vector(forward).normalized()
    z = Vector(up).normalized()
    y = z.cross(x).normalized()
    z = x.cross(y).normalized()
    return transform(Matrix((x, y, z)).transposed(), origin)


def socket_transform(name):
    bone, loc, rot = GRIP_SOCKETS[name]
    return transform(frotator_matrix(*rot), loc)


def ref_bone(name):
    pos, quat = MANNY[name]
    return transform(Quaternion(quat), pos)


def relative_to(bone, wanted):
    """The relative transform on bone that puts a child at wanted (component space, reference pose)."""
    return ref_bone(bone).inverted() @ wanted


def describe(m):
    p, y, r = matrix_frotator(m)
    t = m.translation
    clean = lambda v: 0.0 if abs(v) < 0.005 else v
    return (f'location ({clean(t.x):.2f}, {clean(t.y):.2f}, {clean(t.z):.2f}), '
            f'rotation ({clean(p):.1f}, {clean(y):.1f}, {clean(r):.1f})')


def attachments():
    """Every prop's holds: {hold: (bone or socket, relative transform)}. The grip sockets' frames: X across the palm
    (to the figure's left in the reference pose), Y down the fingers, Z along the thumb, on either hand."""
    out = {}
    hat = relative_to('head', frame(*HAT_SPOT))
    for hat_name, _ in HATS:
        out[hat_name] = {'worn': ('head', hat)}
    aim = frame((0.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))  # bore down the fingers, top at the thumb
    dip = math.radians(CARRY_DIP)
    carry = frame((0.0, 0.0, 0.0), (0.0, math.sin(dip), math.cos(dip)), (0.0, -math.cos(dip), math.sin(dip)))
    tilt = math.radians(SLING_TILT)
    bore = Vector((math.sin(tilt), 0.0, math.cos(tilt)))
    top = Vector((-math.cos(tilt), 0.0, math.sin(tilt)))
    grip_r = socket_transform('HandGrip_R')
    for gun_name in ('ColdOpen_Revolver', 'ColdOpen_CoachGun', 'ColdOpen_LeverRifle', 'ColdOpen_LongRifle'):
        holds = {'aim': ('HandGrip_R', aim), 'aim (bone)': ('hand_r', grip_r @ aim)}
        if gun_name in LONG_GUNS:
            holds['carry'] = ('HandGrip_R', carry)
            holds['carry (bone)'] = ('hand_r', grip_r @ carry)
            grip = SLING_CENTER - bore * LONG_GUNS[gun_name]
            holds['sling'] = ('spine_05', relative_to('spine_05', frame(grip, bore, top)))
        out[gun_name] = holds
    out['ColdOpen_PowderKeg'] = {'back': ('spine_05', relative_to('spine_05', frame(*KEG_BACK))),
                                 'shoulder': ('spine_05', relative_to('spine_05', frame(*KEG_SHOULDER)))}
    bag = frame((0.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, -1.0, 0.0))  # length along the thumb, hanging down the fingers
    out['ColdOpen_MedicBag'] = {'hand': ('HandGrip_L', bag),
                                'hand (bone)': ('hand_l', socket_transform('HandGrip_L') @ bag)}
    return out


ATTACH = attachments()
for prop_name, holds in ATTACH.items():
    for hold, (target, rel) in holds.items():
        print(f'ATTACH {prop_name:<22} {hold:<13} on {target:<11} {describe(rel)}', flush=True)


# ------------------------------------------------------------------------------------------------------------------
# Preview: the props against a sunset, and the gang as posed stand-ins on a ridge
# ------------------------------------------------------------------------------------------------------------------

MIRROR = Matrix(((0.0, -1.0, 0.0, 0.0), (-1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0)))


def blender_matrix(ue):
    """An Unreal world transform (cm) as a Blender object matrix (m)."""
    m = MIRROR @ ue @ MIRROR
    m.translation = m.translation / 100.0
    return m


class Figure:
    """A mannequin posed the way AColdOpenCast poses one: the bones' component transforms, moved by AimBone."""

    def __init__(self, spot):
        self.spot = spot  # the figure's mesh in the world (Unreal, cm)
        self.bones = {name: ref_bone(name) for name in MANNY}

    def pos(self, name):
        return self.bones[name].translation.copy()

    def aim_bone(self, bone, child, direction):
        now = (self.pos(child) - self.pos(bone)).normalized()
        turn = now.rotation_difference(Vector(direction).normalized()).to_matrix().to_4x4()
        pivot = self.pos(bone)
        about = Matrix.Translation(pivot) @ turn @ Matrix.Translation(-pivot)
        for name in ARM_CHAIN[bone]:
            self.bones[name] = about @ self.bones[name]

    def axes(self):
        left = self.pos('upperarm_l') - self.pos('upperarm_r')
        left.z = 0.0
        left.normalize()
        toes = (self.pos('ball_l') - self.pos('foot_l')) + (self.pos('ball_r') - self.pos('foot_r'))
        toes.z = 0.0
        toes -= left * toes.dot(left)
        return toes.normalized(), left, Vector((0.0, 0.0, 1.0))

    def at_ease(self):
        fwd, left, up = self.axes()
        for side, out in (('l', 1.0), ('r', -1.0)):
            self.aim_bone('upperarm_' + side, 'lowerarm_' + side, -up + left * (0.16 * out) + fwd * 0.05)
            self.aim_bone('lowerarm_' + side, 'hand_' + side, -up + fwd * 0.22 + left * (0.04 * out))

    def aim(self, target_world):
        here = self.spot.inverted() @ Vector(target_world)
        along = here - self.pos('upperarm_r')
        self.aim_bone('upperarm_r', 'lowerarm_r', along)
        self.aim_bone('lowerarm_r', 'hand_r', along)

    def holding_ember(self):
        fwd, left, up = self.axes()
        self.aim_bone('upperarm_l', 'lowerarm_l', -up * 0.75 + fwd * 0.55 + left * 0.12)
        self.aim_bone('lowerarm_l', 'hand_l', fwd * 0.85 + up * 0.35 - left * 0.3)

    def world(self, target):
        """A bone's or a grip socket's world transform (Unreal, cm)."""
        if target in GRIP_SOCKETS:
            return self.spot @ self.bones[GRIP_SOCKETS[target][0]] @ socket_transform(target)
        return self.spot @ self.bones[target]

    def body(self):
        """The stand-in: tubes along the posed joints, an oval torso and head, block hands (component space, cm)."""
        s = Shape()
        p = self.pos
        z_up = (0.0, 1.0, 0.0)
        for side in ('l', 'r'):
            tube(s, [p('thigh_' + side) + Vector((0.0, 0.0, 2.0)), p('calf_' + side), p('foot_' + side)],
                 [7.8, 5.6, 4.0], sides=10, normal=z_up)
            toe = p('ball_' + side) + Vector((0.0, 7.5, -0.4))
            tube(s, [p('foot_' + side) + Vector((0.0, -4.0, -2.0)), p('ball_' + side) + Vector((0.0, 0.0, 1.2)), toe],
                 [(4.2, 4.6), (4.6, 3.2), (3.6, 2.2)], sides=8, normal=(0.0, 0.0, 1.0))
            hand = self.bones['hand_' + side].to_3x3()
            thumb = hand.col[2] * (1.0 if side == 'r' else -1.0)
            arm = [p('upperarm_' + side), p('lowerarm_' + side), p('hand_' + side)]
            tube(s, arm, [5.6, 4.3, 3.1], sides=8, normal=thumb)
            lathe(s, [(0.0, 6.6), (4.7, 4.7), (6.6, 0.0), (4.7, -4.7), (0.0, -6.6)], 10, center=arm[0])  # deltoid
            fingers = hand.col[0] * (-1.0 if side == 'r' else 1.0)
            palm = hand.col[1] * (1.0 if side == 'r' else -1.0)
            mid = p('hand_' + side) + fingers * 8.0 + palm * 1.0
            corners = []
            for a in (-1.0, 1.0):
                for b in (-1.0, 1.0):
                    for c in (-1.0, 1.0):
                        corners.append(mid + fingers * (8.5 * a) + thumb * (4.4 * b) + palm * (1.9 * c))
            s.part(corners, [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)])
        levels = [(84.0, 17.8, 2.5, 11.5), (96.0, 16.6, 1.2, 12.2), (110.0, 14.4, 3.6, 10.8), (122.0, 15.6, 3.0, 12.4),
                  (133.0, 19.6, 1.0, 14.8), (143.0, 19.6, 0.4, 13.2), (149.0, 16.0, -1.2, 10.0), (154.0, 9.0, 0.0, 7.6)]
        tube(s, [(0.0, cy, z) for z, _, cy, _ in levels], [(hw, hd) for _, hw, _, hd in levels], sides=12,
             normal=(0.0, 1.0, 0.0))
        tube(s, [(0.0, -1.2, 150.0), (0.0, 0.4, 163.0)], 5.8, sides=8, normal=(0.0, 1.0, 0.0))  # neck
        head = [(0.0, 10.2)] + [(8.8 * math.sin(math.radians(a)), 10.2 * math.cos(math.radians(a)))
                                for a in (30, 60, 90, 120, 150)] + [(0.0, -10.2)]
        lathe(s, head, 14, stretch_r=11.0 / 8.8, center=(0.0, 2.8, 170.4))
        return s


def spot_from_blender(x, y, facing, scale=1.0):
    """A figure's mesh transform (Unreal, cm) for a spot given in Blender's world (m), facing a Blender heading (deg):
    the spot faces its +X, the mannequin's mesh is turned -90 in it (MeshYaw)."""
    a = math.radians(facing)
    yaw = math.degrees(math.atan2(-math.cos(a), -math.sin(a)))
    spot = transform(frotator_matrix(0.0, yaw, 0.0), (-y * 100.0, -x * 100.0, 0.0))
    return spot @ transform(frotator_matrix(0.0, -90.0, 0.0), (0.0, 0.0, 0.0), scale)


def flat(name, hex_value, strength=1.0):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs['Color'].default_value = lt.hex_color(hex_value)
    emit.inputs['Strength'].default_value = strength
    links.new(emit.outputs['Emission'], out.inputs['Surface'])
    mat.diffuse_color = lt.hex_color(hex_value)
    return mat


def helper_object(name, verts, faces, material, blender_space=False):
    """A preview-only mesh ('_' names are never exported)."""
    mesh = bpy.data.meshes.new(name)
    pts = [Vector(v) for v in verts] if blender_space else [to_blender(v) for v in verts]
    mesh.from_pydata([tuple(p) for p in pts], [], [tuple(f) for f in faces])
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(mesh)
    bm.free()
    return obj


def place_copy(name, matrix):
    """A linked copy of a model at a Blender world matrix (preview only)."""
    copy = models[name].copy()
    copy.name = '_' + name
    copy.matrix_world = matrix
    bpy.context.scene.collection.objects.link(copy)
    return copy


def sunset_world(sun=(0.5, 0.42), aspect=1.92):
    """A sunset painted on the frame: a gold horizon under rose and violet, and the sun's glow (screen space)."""
    world = bpy.data.worlds.new('_Sunset')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Window'], split.inputs['Vector'])
    sky = nodes.new('ShaderNodeValToRGB')
    stops = [(0.0, 0xf2913f), (0.40, 0xffcf73), (0.55, 0xffa65a), (0.72, 0xe86d6a), (0.88, 0xa4567e), (1.0, 0x5c4a86)]
    elements = sky.color_ramp.elements
    elements[0].position, elements[0].color = stops[0][0], lt.hex_color(stops[0][1])
    elements[1].position, elements[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for position, color in stops[1:-1]:
        element = elements.new(position)
        element.color = lt.hex_color(color)
    links.new(split.outputs['Y'], sky.inputs['Fac'])
    # The sun: distance from its spot (x stretched by the frame's aspect), a white core and a warm glow.
    scaled = nodes.new('ShaderNodeVectorMath')
    scaled.operation = 'MULTIPLY'
    scaled.inputs[1].default_value = (aspect, 1.0, 0.0)
    links.new(coords.outputs['Window'], scaled.inputs[0])
    distance = nodes.new('ShaderNodeVectorMath')
    distance.operation = 'DISTANCE'
    distance.inputs[1].default_value = (sun[0] * aspect, sun[1], 0.0)
    links.new(scaled.outputs['Vector'], distance.inputs[0])
    glow = nodes.new('ShaderNodeValToRGB')
    glow.color_ramp.interpolation = 'EASE'
    g = glow.color_ramp.elements
    g[0].position, g[0].color = 0.0, (1.0, 0.98, 0.9, 1.0)
    g[1].position, g[1].color = 0.34, (0.0, 0.0, 0.0, 1.0)
    edge = g.new(0.045)
    edge.color = (1.0, 0.93, 0.7, 1.0)
    halo = g.new(0.07)
    halo.color = (0.55, 0.36, 0.16, 1.0)
    links.new(distance.outputs['Value'], glow.inputs['Fac'])
    add = nodes.new('ShaderNodeVectorMath')
    add.operation = 'ADD'
    links.new(sky.outputs['Color'], add.inputs[0])
    links.new(glow.outputs['Color'], add.inputs[1])
    links.new(add.outputs['Vector'], background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def render(path, camera, world, size, engine='BLENDER_EEVEE_NEXT', shown=None):
    scene = bpy.context.scene
    scene.camera = camera
    scene.world = world
    scene.render.engine = engine
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'None'
    if engine == 'BLENDER_EEVEE_NEXT':
        scene.eevee.taa_render_samples = 16
    else:
        shading = scene.display.shading
        shading.light = 'STUDIO'
        shading.color_type = 'SINGLE'
        shading.single_color = (0.62, 0.6, 0.56)
        shading.show_cavity = True
        shading.show_object_outline = False
    for obj in scene.objects:
        if obj.type in ('MESH', 'FONT'):
            obj.hide_render = shown is not None and obj not in shown
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print(f'COLDOPEN preview: {path}', flush=True)


def camera_object(name, location, look_at, lens=36.0, ortho=None):
    cam = bpy.data.objects.new(name, bpy.data.cameras.new(name))
    cam.location = location
    cam.rotation_euler = (Vector(look_at) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    if ortho:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
    else:
        cam.data.lens = lens
    cam.data.clip_start, cam.data.clip_end = 0.05, 2000.0
    bpy.context.scene.collection.objects.link(cam)
    return cam


def label(text, location, size=0.055):
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = text
    curve.size = size
    curve.align_x = 'CENTER'
    obj = bpy.data.objects.new('_Label', curve)
    obj.location = location
    obj.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    obj.data.materials.append(flat('_LabelInk', 0x4a2a22))
    bpy.context.scene.collection.objects.link(obj)
    return obj


def lineup():
    """The props side by side for the camera at -Y: hats and the two props in three-quarter view, guns in profile."""
    placed = []
    three_quarter = Matrix.Rotation(math.radians(12.0), 4, 'X') @ Matrix.Rotation(math.radians(-65.0), 4, 'Z')
    row1 = [('ColdOpen_HatPreacher', 'Preacher (Deacon)'), ('ColdOpen_HatGambler', 'Gambler (Ned)'),
            ('ColdOpen_HatSlouch', 'Slouch (Ira, Lena)'), ('ColdOpen_HatBowler', 'Bowler (Barrels)'),
            ('ColdOpen_Veil', 'Veil (Constance)'), ('ColdOpen_FlatCap', 'Flat cap (Mule)'),
            ('ColdOpen_PowderKeg', 'Powder keg'), ('ColdOpen_MedicBag', 'Medic bag')]
    # The veil turned nearly side on (its drape reads in profile), the keg side on (its belly's outline).
    turns = {'ColdOpen_Veil': Matrix.Rotation(math.radians(8.0), 4, 'X') @ Matrix.Rotation(math.radians(-100.0), 4, 'Z'),
             'ColdOpen_PowderKeg': Matrix.Rotation(math.radians(8.0), 4, 'X') @ Matrix.Rotation(math.radians(80.0), 4, 'Z')}
    for i, (name, text) in enumerate(row1):
        x = -2.3 + i * 0.66
        z = 1.3 + (0.31 if name == 'ColdOpen_MedicBag' else 0.0) + (0.36 if name == 'ColdOpen_Veil' else 0.0)
        placed.append(place_copy(name, Matrix.Translation((x, 0.0, z)) @ turns.get(name, three_quarter)))
        placed.append(label(text, (x, -0.3, 1.13)))
    profile = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    row2 = [('ColdOpen_Revolver', 'Revolver (Ned)', 0.36), ('ColdOpen_CoachGun', 'Coach gun (Deacon)', 0.81),
            ('ColdOpen_LeverRifle', 'Lever carbine (Ira)', 1.03), ('ColdOpen_LongRifle', 'Scoped long rifle (Lena)', 1.26)]
    gap = 0.34
    total = sum(length for _, _, length in row2) + gap * (len(row2) - 1)
    x = -total / 2.0
    for name, text, length in row2:
        # Each gun's grip sits about a third of the way from its butt.
        back = {'ColdOpen_Revolver': 0.03, 'ColdOpen_CoachGun': 0.23, 'ColdOpen_LeverRifle': 0.27,
                'ColdOpen_LongRifle': 0.30}[name]
        placed.append(place_copy(name, Matrix.Translation((x + back, 0.0, 0.46)) @ profile))
        placed.append(label(text, (x + length / 2.0, -0.3, 0.2)))
        x += length + gap
    return placed


def ridge_scene():
    """Seven stand-ins on a ridge against the sun, posed as the cold open poses them, every prop attached by its
    transform in ATTACH."""
    black = silhouette_material()
    placed = []
    # The ridge: a slab whose front edge is the skyline, flat where the gang stands.
    top = []
    for k in range(81):
        x = -40.0 + k
        dx = max(0.0, abs(x) - 4.3)
        z = 0.03 * math.sin(x * 1.7) - 0.55 * min(dx / 6.0, 1.0) ** 2 + (0.9 * max(0.0, (-x - 9.0) / 8.0) if x < -9 else 0.0)
        top.append((x, z))
    verts, faces = [], []
    for x, z in top:
        verts += [(x, -0.6, z), (x, -0.6, -8.0), (x, 4.0, z - 0.4), (x, 4.0, -8.0)]
    for k in range(len(top) - 1):
        a, b = 4 * k, 4 * (k + 1)
        faces += [(a, b, b + 1, a + 1), (a + 2, a, b, b + 2), (a + 3, b + 3, b + 2, a + 2), (a + 1, b + 1, b + 3, a + 3)]
    placed.append(helper_object('_Ridge', verts, faces, black, blender_space=True))
    # Far ranges past the plains, hazy against the sun.
    verts, faces = [], []
    for k in range(61):
        x = -300.0 + k * 10.0
        z = 9.0 + 3.0 * math.sin(x * 0.031) + 1.8 * math.sin(x * 0.083 + 1.0)
        verts += [(x, 220.0, z), (x, 220.0, -40.0)]
    faces = [(2 * k, 2 * k + 2, 2 * k + 3, 2 * k + 1) for k in range(60)]
    placed.append(helper_object('_FarRange', verts, faces, flat('_Haze', 0xb06a62), blender_space=True))

    gang = [  # (who, x, facing in Blender degrees, scale, pose, [(model, hold)])
        ('Ira', -3.3, 8.0, 1.0, '', [('ColdOpen_HatSlouch', 'worn'), ('ColdOpen_LeverRifle', 'carry')]),
        ('Mule', -2.2, -118.0, 1.1, '', [('ColdOpen_FlatCap', 'worn'), ('ColdOpen_CoachGun', 'sling')]),
        ('Deacon', -1.05, -58.0, 1.0, 'ember', [('ColdOpen_HatPreacher', 'worn'), ('ColdOpen_CoachGun', 'carry')]),
        ('Ned', 0.15, 182.0, 1.0, 'aim', [('ColdOpen_HatGambler', 'worn'), ('ColdOpen_Revolver', 'aim')]),
        ('Constance', 1.3, -125.0, 1.0, '', [('ColdOpen_Veil', 'worn'), ('ColdOpen_LeverRifle', 'sling'),
                                              ('ColdOpen_MedicBag', 'hand')]),
        ('Barrels', 2.35, -28.0, 1.0, '', [('ColdOpen_HatBowler', 'worn'), ('ColdOpen_PowderKeg', 'back'),
                                           ('ColdOpen_CoachGun', 'carry')]),
        ('Lena', 3.25, 12.0, 1.0, '', [('ColdOpen_HatSlouch', 'worn'), ('ColdOpen_LongRifle', 'carry')]),
    ]
    for who, x, facing, scale, pose, attached in gang:
        fig = Figure(spot_from_blender(x, 0.0, facing, scale))
        fig.at_ease()
        if pose == 'ember':
            fig.holding_ember()
        elif pose == 'aim':
            target = Vector((x - 9.0, -3.0, -2.5))  # down the bluff path, off to the left (Blender m)
            fig.aim(Vector((-target.y * 100.0, -target.x * 100.0, target.z * 100.0)))
        body = fig.body()
        verts = [fig.spot @ v for v in body.verts]
        placed.append(helper_object('_' + who, verts, body.faces, black))
        for model, hold in attached:
            target, rel = ATTACH[model][hold]
            world = fig.world(target) @ rel
            placed.append(place_copy(model, blender_matrix(world)))
            if target == 'HandGrip_R':
                bore = world.to_3x3().col[0].normalized()
                gun_top = world.to_3x3().col[2].normalized()
                socket = models[model].children[0].location  # SOCKET_Muzzle, Blender's metres to Unreal's cm
                tip = world @ Vector((-socket.y * 100.0, -socket.x * 100.0, socket.z * 100.0))
                print(f'COLDOPEN {who}: {model} ({hold}) bore {tuple(round(c, 2) for c in bore)}, top '
                      f'{tuple(round(c, 2) for c in gun_top)}, muzzle {tip.z:.0f} cm over the feet', flush=True)
        if pose == 'ember':
            # The code's ember: a 22 cm glow 6 cm over the left hand.
            hand = fig.world('hand_l').translation + Vector((0.0, 0.0, 6.0))
            ember = Shape()
            lathe(ember, [(0.0, 7.0), (6.0, 3.5), (7.0, 0.0), (6.0, -3.5), (0.0, -7.0)], 12, center=hand)
            placed.append(helper_object('_Ember', ember.verts, ember.faces, flat('_EmberGlow', 0xff7a1e, 4.0)))
    return placed


def stack(top_png, bottom_png, out_png):
    """The two panels in one picture, the props over the ridge."""
    import numpy as np
    images = [bpy.data.images.load(p) for p in (bottom_png, top_png)]  # Blender's rows run bottom-up
    width = images[0].size[0]
    rows = []
    for img in images:
        data = np.empty(img.size[0] * img.size[1] * 4, dtype=np.float32)
        img.pixels.foreach_get(data)
        rows.append(data.reshape(img.size[1], width, 4))
    pixels = np.concatenate(rows, axis=0)
    combined = bpy.data.images.new('_Combined', width, pixels.shape[0])
    combined.pixels.foreach_set(pixels.ravel())
    combined.filepath_raw = out_png
    combined.file_format = 'PNG'
    combined.save()
    print(f'COLDOPEN preview: {out_png}', flush=True)


if lt.want_preview():
    folder = os.path.dirname(lt.preview_path('RansomsRest/ColdOpen', 'x'))
    os.makedirs(folder, exist_ok=True)
    props_png = os.path.join(folder, 'ColdOpen_props.png')
    ridge_png = os.path.join(folder, 'ColdOpen_ridge.png')
    clay_png = os.path.join(folder, 'ColdOpen_props_clay.png')
    shown = lineup()
    cam = camera_object('_LineupCamera', (0.0, -10.0, 0.98), (0.0, 0.0, 0.98), ortho=5.7)
    render(props_png, cam, sunset_world(sun=(0.5, 0.18)), (1920, 760), shown=shown)
    clay_world = bpy.data.worlds.new('_Clay')
    clay_world.color = (0.72, 0.72, 0.7)
    render(clay_png, cam, clay_world, (1920, 760), engine='BLENDER_WORKBENCH',
           shown=[o for o in shown if not o.name.startswith('_Label')])
    for obj in shown:
        obj.hide_render = True
    ridge = ridge_scene()
    cam = camera_object('_RidgeCamera', (0.0, -8.6, -0.22), (0.0, 0.0, 0.78), lens=34.0)
    render(ridge_png, cam, sunset_world(sun=(0.47, 0.44)), (1920, 1000), shown=ridge)
    stack(props_png, ridge_png, os.path.join(folder, 'ColdOpen_silhouettes.png'))
