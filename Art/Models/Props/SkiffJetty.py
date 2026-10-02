"""Skyreach's skiff jetty (Docs/Story.md: Leaving the tutorial island; Docs/Areas/RansomsRest.md: Skiff and jetty and
Tech needs, Leaving Skyreach): a timber jetty off the plateau's south-east rim past the ruined lookout, where the
packet skiff (Art/Models/Vehicles/Skiff.py) lies moored with its gangplank up until the tutorial is done.

  SkiffJetty      22 m of plank deck: 6 m on posts inland, then 16 m cantilevered over the drop on stringers and three
                  pairs of raking struts braced into the cliff face below the rim. Rails both sides, open on the -X side
                  where the gangplank lands, mooring bollards, a lantern post at the end. Nanite with a full fallback.
  JettyBellPost   a post and arm by the jetty's root; the bell hangs at SOCKET_Bell, the rope is pulled at
                  SOCKET_Interact.
  JettyBell       the bell: its origin is its swing axis (the headstock pin), the bell hanging below, its rope off the
                  headstock's lever. No collision.
  JettySlate      a chalk slate on an easel: "Skiff departs when you're ready." in Rye, chalk on black.

SkiffJetty's origin is on the ground at the rim (where the drop starts), across the middle of the deck; the jetty
points out over the drop toward -Y and its root runs 6 m inland (+Y). The deck's top is 0.30 m above the ground.
Sockets: SOCKET_Gangplank_Land (where the skiff's gangplank rests, on the deck's -X edge in the rail opening, facing
out), SOCKET_Bollard_1..2 (the bollards' tops: 1 takes the skiff's stern line, 2 its bow line), SOCKET_Light (the end
lantern's glass), SOCKET_Landing (where a trip back to Skyreach arrives, facing inland), and SOCKET_BellPost and
SOCKET_Slate (suggested spots for the two props, on the ground beside the root).

Mooring the skiff: its +X side toward the jetty, its bow out (both front to -Y), its centreline 5.03 m from the jetty's
toward -X, its keel 0.45 m below the deck's top, and its SOCKET_Gangplank level with SOCKET_Gangplank_Land's y; the
3 m gangplank then rests 25 cm onto the deck.

    blender -b --factory-startup --python Art/Models/Props/SkiffJetty.py -- --preview
"""
import math
import os
import runpy

import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp
import looter_train as tk

DECK_Z = 0.30                   # deck top above the ground
ROOT, TIP = 6.0, -16.0          # deck ends (y)
HALF = 1.3                      # half the deck's width
OPEN = (-10.6, -9.4)            # the rail opening for the gangplank, on the -X side
LAND_Y = -10.0
SKIFF_X, SKIFF_Z = -5.03, DECK_Z - 0.45     # where the moored skiff's origin goes (preview and report)


def jetty():
    k = tk.Kit('SkiffJetty', 91)
    rnd = k.rnd
    # Planks across the deck, each a little uneven.
    y = ROOT - 0.1
    while y > TIP + 0.05:
        k.box((2.0 * HALF + rnd.uniform(-0.04, 0.04), 0.19, 0.07),
              (rnd.uniform(-0.02, 0.02), y, DECK_Z - 0.035 + rnd.uniform(-0.004, 0.004)), 'planks',
              rot=(0.0, rnd.uniform(-0.4, 0.4), rnd.uniform(-0.6, 0.6)))
        y -= 0.2
    k.section('planks')
    # Three stringers, posts and cap beams inland, the rim sill.
    for x in (-1.0, 0.0, 1.0):
        k.box((0.16, ROOT - TIP + 0.2, 0.18), (x, (ROOT + TIP) * 0.5, DECK_Z - 0.16), 'trim', strip='Beams', axis='y')
    for y in (5.4, 3.0, 0.7):
        k.box((2.0 * HALF + 0.3, 0.2, 0.18), (0.0, y, DECK_Z - 0.34), 'trim', strip='Beams', axis='x')
        for x in (-1.2, 1.2):
            k.box((0.2, 0.2, 0.7), (x, y, DECK_Z - 0.6), 'trim', strip='Beams', axis='z')
    k.box((2.0 * HALF + 0.5, 0.3, 0.25), (0.0, 0.2, -0.05), 'trim', strip='Beams', axis='x')
    # Over the drop: three pairs of raking struts from the cliff face, cap beams under the stringers, braces.
    heads = [(-2.6, -5.0), (-4.4, -10.0), (-6.2, -14.8)]
    for z_foot, y_head in heads:
        k.box((2.0 * HALF + 0.2, 0.2, 0.18), (0.0, y_head, DECK_Z - 0.34), 'trim', strip='Beams', axis='x')
        for x in (-1.0, 1.0):
            foot = Vector((x, 0.35, z_foot))
            head = Vector((x, y_head, DECK_Z - 0.43))
            k.beam(foot, head, 0.2, 0.2, 'trim', 'Beams', up=(1.0, 0.0, 0.0))
            k.box((0.32, 0.12, 0.32), (x, 0.42, z_foot), 'iron')
            k.box((0.24, 0.24, 0.05), (x, y_head, DECK_Z - 0.45), 'iron')
        mid = 0.45
        a = Vector((-1.0, 0.35 + (y_head - 0.35) * mid, z_foot + (DECK_Z - 0.43 - z_foot) * mid))
        b = Vector((1.0, 0.35 + (y_head - 0.35) * (mid + 0.25), z_foot + (DECK_Z - 0.43 - z_foot) * (mid + 0.25)))
        k.beam(a, b, 0.12, 0.12, 'trim', 'Beams')
        k.beam(Vector((-a.x, a.y, a.z)), Vector((-b.x, b.y, b.z)), 0.12, 0.12, 'trim', 'Beams')
    k.section('structure')
    # Rails: posts every 2.2 m, top and middle rails, open on -X at the gangplank; a heavier end rail.
    posts = [ROOT - 0.2 - 2.2 * i for i in range(10)] + [TIP + 0.12]
    for s in (-1.0, 1.0):
        x = s * (HALF - 0.06)
        spans = [(TIP + 0.12, OPEN[0]), (OPEN[1], ROOT - 0.2)] if s < 0 else [(TIP + 0.12, ROOT - 0.2)]
        for a, b in spans:
            for z, h in ((DECK_Z + 1.02, 0.08), (DECK_Z + 0.55, 0.06)):
                k.box((0.09, b - a + 0.1, h), (x, (a + b) * 0.5, z), 'trim', strip='Beams', axis='y')
            for y in [p for p in posts if a - 0.01 <= p <= b + 0.01] + [a, b]:
                k.box((0.11, 0.11, 1.1), (x, y, DECK_Z + 0.5), 'trim', strip='Beams', axis='z')
    for z, h in ((DECK_Z + 1.02, 0.08), (DECK_Z + 0.55, 0.06)):
        k.box((2.0 * HALF - 0.1, 0.09, h), (0.0, TIP + 0.12, z), 'trim', strip='Beams', axis='x')
    for y in OPEN:
        k.box((0.16, 0.16, 1.25), (-(HALF - 0.06), y, DECK_Z + 0.57), 'trim', strip='Beams', axis='z')
    k.section('rails')
    # Bollards on the skiff's side and the end lantern.
    for i, y in enumerate((-6.2, -14.6)):
        x = -(HALF - 0.28)
        k.lathe([(0.0, 0.0), (0.12, 0.0), (0.1, 0.05), (0.09, 0.26), (0.14, 0.3), (0.13, 0.36), (0.0, 0.37)], 'iron',
                (x, y, DECK_Z), sides=10)
        k.socket(f'Bollard_{i + 1}', (x, y, DECK_Z + 0.33))
    k.box((0.14, 0.14, 2.2), (0.0, TIP + 0.12, DECK_Z + 1.1), 'trim', strip='Beams', axis='z')
    k.beam((0.0, TIP + 0.12, DECK_Z + 2.05), (0.0, TIP - 0.4, DECK_Z + 2.05), 0.08, 0.1, 'trim', 'Beams')
    k.beam((0.0, TIP + 0.1, DECK_Z + 1.6), (0.0, TIP - 0.25, DECK_Z + 2.0), 0.06, 0.06, 'trim', 'Beams')
    tk.oil_lamp(k, (0.0, TIP - 0.42, DECK_Z + 1.62), facing=(0.0, -1.0, 0.0), size=0.26, body='iron', socket='Light')
    k.tube([(0.0, TIP - 0.42, DECK_Z + 2.0), (0.0, TIP - 0.42, DECK_Z + 2.06)], 0.012, 'iron', sides=4)
    k.section('fittings')
    k.socket('Gangplank_Land', (-HALF, LAND_Y, DECK_Z), (0.0, 0.0, -90.0))
    k.socket('Landing', (0.0, -7.5, DECK_Z), (0.0, 0.0, 180.0))
    k.socket('BellPost', (HALF + 0.75, 4.6, 0.0), (0.0, 0.0, 180.0))
    k.socket('Slate', (-(HALF + 0.9), 5.0, 0.0), (0.0, 0.0, 200.0))
    # Collision: the walkable deck, the rails (open at the gangplank), the end rail.
    k.hull((2.0 * HALF, ROOT - TIP, DECK_Z + 0.3), (0.0, (ROOT + TIP) * 0.5, DECK_Z * 0.5 - 0.15))
    for s in (-1.0, 1.0):
        spans = [(TIP, OPEN[0]), (OPEN[1], ROOT)] if s < 0 else [(TIP, ROOT)]
        for a, b in spans:
            k.hull((0.16, b - a, 1.25), (s * (HALF - 0.06), (a + b) * 0.5, DECK_Z + 0.62))
    k.hull((2.0 * HALF, 0.16, 1.25), (0.0, TIP + 0.1, DECK_Z + 0.62))
    return k.finish(ao=0.8, nanite=True)


def bell_post():
    """The bell's post: a squared post on a footing, an arm with a brace, a cleat for the rope (SOCKET_Interact where
    the rope hangs to hand height) and SOCKET_Bell at the arm's end."""
    k = tk.Kit('JettyBellPost', 92)
    k.box((0.42, 0.42, 0.22), (0.0, 0.0, 0.08), 'trim', strip='Stone', faces=True)
    k.box((0.18, 0.18, 2.75), (0.0, 0.0, 1.35), 'trim', strip='Beams', axis='z', bevel=0.012)
    k.beam((-0.06, 0.0, 2.58), (0.72, 0.0, 2.58), 0.12, 0.14, 'trim', 'Beams')
    k.beam((0.0, 0.0, 2.05), (0.4, 0.0, 2.52), 0.08, 0.08, 'trim', 'Beams', up=(0.0, 1.0, 0.0))
    k.box((0.06, 0.12, 0.2), (0.58, 0.0, 2.45), 'iron')
    for dy in (-0.07, 0.07):
        k.box((0.12, 0.02, 0.16), (0.58, dy, 2.46), 'iron')
    k.box((0.05, 0.16, 0.04), (0.11, 0.0, 1.18), 'iron')
    k.socket('Bell', (0.58, 0.0, 2.42))
    k.socket('Interact', (0.38, -0.05, 1.28))
    k.hull((0.24, 0.24, 2.8), (0.0, 0.0, 1.4))
    return k.finish(lods='50,25', screens='0.3,0.1', ao=0.4)


def bell():
    """The bell, hanging from its swing axis (the origin), the headstock's lever and its rope toward the post."""
    k = tk.Kit('JettyBell', 93)
    k.box((0.16, 0.14, 0.1), (0.0, 0.0, -0.05), 'iron')
    k.cyl((0.0, -0.1, 0.0), (0.0, 0.1, 0.0), 0.02, 'iron', sides=6)
    k.lathe([(0.0, -0.08), (0.07, -0.08), (0.1, -0.12), (0.12, -0.22), (0.15, -0.33), (0.2, -0.4), (0.21, -0.43),
             (0.0, -0.43)], 'brass', (0.0, 0.0, 0.0), sides=16)
    k.cyl((0.0, 0.0, -0.36), (0.0, 0.0, -0.48), 0.03, 'iron', sides=6)
    k.beam((0.0, 0.0, -0.02), (-0.24, 0.0, 0.02), 0.04, 0.04, 'iron', up=(0.0, 1.0, 0.0))
    k.tube([(-0.24, 0.0, 0.0), (-0.22, -0.01, -0.45), (-0.2, -0.03, -1.14)], 0.014, 'canvas', sides=5)
    k.cyl((-0.2, -0.03, -1.12), (-0.2, -0.03, -1.22), 0.03, 'canvas', sides=6)
    return k.finish(lods='50,25', screens='0.2,0.06', ao=0.2, ground=False, collision=False)


def slate():
    """The chalk slate on an easel: black board in a timber frame, chalk lettering, a ledge with chalk."""
    k = tk.Kit('JettySlate', 94)
    tilt = 12.0
    for x in (-0.36, 0.36):
        k.beam((x, 0.06, 0.0), (x * 0.9, -0.12, 1.65), 0.05, 0.04, 'trim', 'Beams', up=(0.0, 1.0, 0.0))
    k.beam((0.0, 0.5, 0.0), (0.0, 0.0, 1.5), 0.05, 0.04, 'trim', 'Beams', up=(1.0, 0.0, 0.0))
    frame = Matrix.Translation((0.0, -0.06, 1.15)) @ Matrix.Rotation(math.radians(-tilt), 4, 'X')
    board = lp.block((0.86, 0.025, 0.6), (0.0, 0.0, 0.0))
    k.map(board, 'wblack')
    board.data.transform(frame)
    for dx, dz, sx, sz in ((0.0, 0.32, 0.96, 0.05), (0.0, -0.32, 0.96, 0.05), (-0.455, 0.0, 0.05, 0.69),
                           (0.455, 0.0, 0.05, 0.69)):
        part = lp.block((sx, 0.045, sz), (dx, -0.005, dz))
        lp.grain(part, 'Beams', axis=(1.0, 0.0, 0.0) if sx > sz else (0.0, 0.0, 1.0), seed=k.seed())
        k.add(part)
        part.data.transform(frame)
    ledge = lp.block((0.8, 0.08, 0.025), (0.0, -0.05, -0.36))
    lp.grain(ledge, 'Beams', axis=(1.0, 0.0, 0.0), seed=k.seed())
    k.add(ledge)
    ledge.data.transform(frame)
    chalk = lp.block((0.07, 0.015, 0.015), (0.18, -0.06, -0.34))
    k.map(chalk, 'cream')
    chalk.data.transform(frame)
    for text, dz, size in (('Skiff departs', 0.1, 0.078), ("when you're ready.", -0.08, 0.064)):
        letters = k.text(text, size, (0.0, 0.0, 0.0), (90.0, 0.0, 0.0), 'cream')
        letters.data.transform(frame @ Matrix.Translation((0.0, -0.0135, dz)))
    k.hull((0.85, 0.7, 1.65), (0.0, 0.15, 0.82))
    return k.finish(lods='50,25', screens='0.25,0.08', ao=0.3)


models = [jetty(), bell_post(), bell(), slate()]

if lt.want_preview() and __name__ == '__main__':
    folder = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Skiff')
    skiff_ns = runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Vehicles', 'Skiff.py'), run_name='skiff_for_jetty')
    jetty_obj, post, bell_obj, slate_obj = models
    # A preview cliff: the plateau's edge at y = 0, the drop beyond.
    cliff = lp.block((16.0, 10.0, 9.0), (0.0, 5.0, -4.55))
    lt.assign(cliff, lt.material('RockCliff', MossAmount=0.6))
    lt.box_uv(cliff, 'RockCliff', seed=3)
    cliff.name = 'PV_Cliff'
    turf = lp.block((16.0, 10.0, 0.1), (0.0, 5.0, -0.05))
    lt.assign(turf, lt.material('GroundGrass'))
    lt.box_uv(turf, 'GroundGrass', seed=4)
    turf.name = 'PV_Turf'
    bpy.context.view_layer.update()

    def at_socket(model, name, target):
        sock = next(c for c in model.children if c.name.startswith('SOCKET_' + name))
        copy = bpy.data.objects.new('PV_' + target.name, target.data)
        bpy.context.scene.collection.objects.link(copy)
        copy.matrix_world = sock.matrix_world.copy()
        return copy
    placed = [at_socket(jetty_obj, 'BellPost', post), at_socket(jetty_obj, 'Slate', slate_obj)]
    bpy.context.view_layer.update()
    sock = next(c for c in post.children if c.name.startswith('SOCKET_Bell'))
    bell_copy = bpy.data.objects.new('PV_JettyBell', bell_obj.data)
    bpy.context.scene.collection.objects.link(bell_copy)
    bell_copy.matrix_world = placed[0].matrix_world @ sock.matrix_local
    placed.append(bell_copy)
    for m in models:
        m.hide_render = m is not jetty_obj
    skiff = skiff_ns['models']['A_Packet']
    skiff.location = (SKIFF_X, LAND_Y - 0.6, SKIFF_Z)
    bpy.context.view_layer.update()
    plank = skiff_ns['attach_plank'](skiff, raised=False)
    shown = [jetty_obj, cliff, turf, skiff, plank] + placed
    for m in skiff_ns['models'].values():
        if m is not skiff:
            m.hide_render = True
    skiff_ns['plank'].hide_render = True
    tk.render(shown, 'SkiffJetty_moored', view=(-0.9, 0.75, 0.5), lens=35.0, fit=0.6, folder=folder, ground_at=-30.0)
    tk.closeup(shown, 'SkiffJetty_gangplank_down', Vector((-2.6, LAND_Y, 0.6)), (0.35, -1.0, 0.5), 9.0, lens=35.0,
               folder=folder)
    tk.closeup(shown, 'SkiffJetty_root', Vector((0.0, 4.0, 1.0)), (0.55, 1.0, 0.3), 8.0, lens=35.0, folder=folder)
    # The family overview: the jetty with the packet skiff moored, and the gang's paint of the same skiff beside it.
    extra = []
    for i, key in enumerate(('A_Gang',)):
        m = skiff_ns['models'][key]
        m.location = (-13.0, -8.0, 0.4)
        m.hide_render = False
        extra.append(m)
    bpy.context.view_layer.update()
    tk.render(shown + extra, 'Skiff_overview', view=(-0.35, 1.0, 0.45), lens=35.0, fit=0.55, resolution=(1920, 1080),
              folder=os.path.dirname(folder), ground_at=-30.0)
