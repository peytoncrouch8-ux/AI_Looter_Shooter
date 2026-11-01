"""Pictures of the Drover's Tripo sets as the game assembles them (lane TRIPO-PISTOLS, 2026-11-01): Drover.py is built
(LOOTER_DROVER_TRIPO as set, every Tripo set by default), each set's five parts put on the frame's sockets, and rendered
from the side, three-quarter, first person and with the cylinder swung out; one mixed gun shows that sets' parts fit
each other.

    Tools\\artrun.ps1 -Script Tools\\Blender\\drover_tripo_sheet.py [-ScriptArgs '--tag,Scripted']

Writes Saved/ArtPreviews/TripoDrover/<Set>_<View>[_<tag>].png. --tag names a run (LOOTER_DROVER_TRIPO=0 for the scripted
parts to compare with).
"""
import importlib.util
import os
import sys

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MINE = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
MINE = [a for part in MINE for a in part.split(',') if a]
TAG = MINE[MINE.index('--tag') + 1] if '--tag' in MINE else ''
PARTS = '--parts' in MINE   # also each part alone
ADS_ONLY = '--ads' in MINE  # only the sight pictures
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'TripoDrover')

SETS = {
    'Set1_SaintMaud': {'Body': 'Blued', 'Barrel': 'Service', 'Cylinder': 'Fluted6', 'Sight': 'Notch', 'Grip': 'Plowhandle'},
    'Set2_Roundhouse': {'Body': 'Ironclad', 'Barrel': 'Lugged', 'Cylinder': 'Bigbore5', 'Sight': 'Glass', 'Grip': 'Coffin'},
    'Set3_Windfall': {'Body': 'Scrapper', 'Barrel': 'Comped', 'Cylinder': 'Plain6', 'Sight': 'Peep', 'Grip': 'Wrapped'},
    'Set4_Meridian': {'Body': 'Ranger', 'Barrel': 'Vented', 'Cylinder': 'Moon6', 'Sight': 'Pip', 'Grip': 'Target'},
    'Mixed': {'Body': 'Ironclad', 'Barrel': 'Vented', 'Cylinder': 'Fluted6', 'Sight': 'Peep', 'Grip': 'Wrapped'},
}
VIEWS = {   # lt.preview's view (the camera's way from the gun), lens, fit
    'Side': ((-1.0, -0.02, 0.05), 80.0, 0.72),
    'Q': ((-1.0, -0.35, 0.3), 60.0, 0.72),
    'Right': ((1.0, -0.3, 0.25), 60.0, 0.72),
    'FP': ((0.42, 1.0, 0.32), 45.0, 0.6),
}


def build():
    path = os.path.join(REPO, 'Art', 'Models', 'Weapons', 'Drover.py')
    sys.argv = [sys.argv[0], '--']
    spec = importlib.util.spec_from_file_location('drover_model', path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules['drover_model'] = mod
    spec.loader.exec_module(mod)
    return mod


def sight_picture(mod, choice, path):
    """What aiming down the sights shows: the eye 22 cm behind SOCKET_Aim, looking along the gun (the game's ADS rule),
    a narrow lens so the sight fills the middle. The front dot should sit on the rear's line, nothing in the way."""
    import math
    from mathutils import Vector
    shown = mod.assemble(choice)
    for o in mod.built.values():   # the other parts lie in rows ahead of the gun: out of the line of sight
        if o not in shown:
            o.location = (0.0, 0.0, 9.0)
    bpy.context.view_layer.update()
    sight = mod.built['Sight_' + choice['Sight']]
    aim = mod.socket_at(sight, 'Aim')
    sc = bpy.context.scene
    cam = bpy.data.objects.get('_ADSCam') or bpy.data.objects.new('_ADSCam', bpy.data.cameras.new('_ADSCam'))
    if cam.name not in sc.collection.objects:
        sc.collection.objects.link(cam)
    cam.data.lens, cam.data.clip_start = 70.0, 0.01
    cam.location = aim + Vector((0.0, 0.22, 0.0))           # behind: the gun's back is +Y
    cam.rotation_euler = (math.radians(90.0), 0.0, math.radians(180.0))   # looking along -Y (the muzzle's way), up +Z
    old_cam, old_res, old_world = sc.camera, (sc.render.resolution_x, sc.render.resolution_y), sc.world
    sc.camera = cam
    world = bpy.data.worlds.get('_ADSWorld') or bpy.data.worlds.new('_ADSWorld')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.62, 0.68, 0.75, 1.0)   # a pale sky to aim at
    world.node_tree.nodes['Background'].inputs[1].default_value = 1.2
    sc.world = world
    sc.render.film_transparent = False
    sc.render.resolution_x, sc.render.resolution_y = 900, 600
    old_engine = sc.render.engine
    engines = [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items]
    sc.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in engines else 'BLENDER_EEVEE'
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    sc.camera, sc.render.engine, sc.world = old_cam, old_engine, old_world
    sc.render.resolution_x, sc.render.resolution_y = old_res
    return shown


def main():
    import looter_textures as lt
    mod = build()
    os.makedirs(OUT, exist_ok=True)
    suffix = f'_{TAG}' if TAG else ''
    for name, choice in SETS.items():
        if ADS_ONLY:
            sight_picture(mod, choice, os.path.join(OUT, f'{name}_ADS{suffix}.png'))
            continue
        for view, (way, lens, fit) in VIEWS.items():
            lt.preview(mod.assemble(choice), os.path.join(OUT, f'{name}_{view}{suffix}.png'), view=way, ground=False,
                       lens=lens, fit=fit, resolution=(1100, 700))
        shown = mod.assemble(choice)
        mod.swing_out(mod.built['Cylinder_' + choice['Cylinder']], shown[0])
        lt.preview(shown, os.path.join(OUT, f'{name}_Reload{suffix}.png'), view=(1.0, -0.6, 0.45), ground=False,
                   lens=60.0, fit=0.72, resolution=(1100, 700))
        sight_picture(mod, choice, os.path.join(OUT, f'{name}_ADS{suffix}.png'))
        if PARTS and name != 'Mixed':   # each part alone, close (its Tripo look against its views)
            for slot, key in choice.items():
                obj = mod.built[f'{slot}_{key}']
                for o in mod.built.values():
                    o.location = (0.0, 0.0, 0.0) if o is obj else (0.0, 0.0, 9.0)
                bpy.context.view_layer.update()
                lt.preview([obj], os.path.join(OUT, f'{name}_{slot}_{key}{suffix}.png'), view=(-1.0, -0.4, 0.35),
                           ground=False, lens=60.0, fit=0.9, resolution=(800, 600))
        print(f'DROVER_SHEET {name} done', flush=True)
    for key, obj in mod.built.items():
        if 'LODs' in obj.keys():
            tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
            print(f'DROVER_SHEET {key}: {tris} triangles, LODs {obj["LODs"]} at {obj["LODScreens"]}, '
                  f'slots {[m.name for m in obj.data.materials]}', flush=True)


main()
