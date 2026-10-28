# Renders the Tripo chest sheet (Art/Models/Openables/Loot+chests.glb) straight on, orthographic, flat light, transparent.
#   blender -b --factory-startup --python Art/Concepts/Chests/make/render_sheet.py -- front 6144 <repo>/Saved/ChestViews/sheet_front.png
import bpy, sys, math, os
argv = sys.argv[sys.argv.index('--')+1:]
side, res, out = argv[0], int(argv[1]), argv[2]
bpy.ops.wm.read_factory_settings(use_empty=True)
here = os.path.dirname(os.path.abspath(__file__))
bpy.ops.import_scene.gltf(filepath=os.path.join(here, "..", "..", "..", "Models", "Openables", "Loot+chests.glb"))
sc = bpy.context.scene
cam = bpy.data.cameras.new('cam'); cam.type='ORTHO'; cam.ortho_scale=1.0
co = bpy.data.objects.new('cam', cam); sc.collection.objects.link(co); sc.camera=co
cx, cz = 0.0013, 0.4985
if side=='front':
    co.location=(cx,-2,cz); co.rotation_euler=(math.pi/2,0,0)
else:
    co.location=(cx,2,cz); co.rotation_euler=(math.pi/2,0,math.pi)
cam.clip_end=10
sc.render.engine='BLENDER_WORKBENCH'
sh=sc.display.shading; sh.light='FLAT'; sh.color_type='TEXTURE'
sc.display_settings.display_device='sRGB'; sc.view_settings.view_transform='Standard'
sc.render.film_transparent=True
sc.render.resolution_x=sc.render.resolution_y=res; sc.render.resolution_percentage=100
sc.display.render_aa='32'
sc.render.image_settings.file_format='PNG'; sc.render.image_settings.color_mode='RGBA'
sc.render.filepath=out
bpy.ops.render.render(write_still=True)
