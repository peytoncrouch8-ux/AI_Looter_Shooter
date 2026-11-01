"""Drover set 2, Roundhouse (railway heavy: oxblood, black iron, big brass rivets, cream stencil chevrons and bands):
Ironclad frame, Lugged barrel, Bigbore5 cylinder, Glass scope, Coffin grip, made by Tripo (multiview) from
Tools/Blender/drover_views.py's views. Spec format: Tools/Blender/looter_tripo_gun.py; shared rules:
looter_tripo_drover.py. Not a model: Tools\models.ps1 skips parts_*.py.
"""
import looter_tripo_drover as d

SHELL = d.SHELLS['Roundhouse']
TIP = d.BARREL_U + 15.0
# The Glass scope (Drover.py sights()): eyepiece at u -4, its tube's profile, its axis 2.5 over the strap.
GLASS = (-4.0, [(0.0, 1.15), (2.2, 1.15), (3.0, 0.85), (12.0, 0.85), (13.0, 1.15), (15.0, 1.15)], d.STRAP_TOP + 2.5)

PARTS = {
    'Body_Ironclad': dict(glb='DroverBody_Ironclad.glb', axes=d.AXES, keep=d.keep_glows, cuts=d.slit_cuts,
                          glows={'GunEmberGlow': d.is_ember}, paint=SHELL),
    'Barrel_Lugged': dict(glb='DroverBarrel_Lugged.glb', axes=d.AXES, keep=d.keep_blade(TIP),
                          prepare=d.trim_blade(TIP), paint=SHELL),
    'Cylinder_Bigbore5': dict(glb='DroverCylinder_Bigbore5.glb', axes=d.AXES, keep=d.keep_rod_crane,
                              box_from='gone',
                              prepare=d.drum(5, 2.3)),
    'Sight_Glass': dict(glb='DroverSight_Glass.glb', axes=d.AXES, keep=d.keep_glows, cuts=d.scope_cuts(*GLASS)),
    'Grip_Coffin': dict(glb='DroverGrip_Coffin.glb', axes=d.AXES, paint=SHELL),
}
