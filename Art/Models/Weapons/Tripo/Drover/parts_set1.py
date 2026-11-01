"""Drover set 1, Saint Maud (plain and careful: ivory enamel, cast iron, gilt pinstripes and screws, checkered grip): the
defaults Blued frame, Service barrel, Fluted6 cylinder, Notch sight, Plowhandle grip, made by Tripo (multiview) from
Tools/Blender/drover_views.py's views (Art/Concepts/Weapons/Drover/<Slot>_<Key>/upload). How Drover.py fits each: the
spec format is in Tools/Blender/looter_tripo_gun.py, the shared rules in looter_tripo_drover.py. Not a model:
Tools\models.ps1 skips parts_*.py.
"""
import looter_tripo_drover as d

SHELL = d.SHELLS['SaintMaud']
TIP = d.BARREL_U + 14.0

PARTS = {
    'Body_Blued': dict(glb='DroverBody_Blued.glb', axes=d.AXES, keep=d.keep_glows, cuts=d.vent_cuts,
                       glows={'GunEmberGlow': d.is_ember}, paint=SHELL),
    'Barrel_Service': dict(glb='DroverBarrel_Service.glb', axes=d.AXES, keep=d.keep_blade(TIP),
                           prepare=d.trim_blade(TIP), paint=SHELL),
    'Cylinder_Fluted6': dict(glb='DroverCylinder_Fluted6.glb', axes=d.AXES, keep=d.keep_rod_crane,
                             box_from='gone',
                             prepare=d.drum(6, 2.15)),
    'Sight_Notch': dict(glb='DroverSight_Notch.glb', axes=d.AXES, cuts=d.notch_cuts(0.52)),
    'Grip_Plowhandle': dict(glb='DroverGrip_Plowhandle.glb', axes=d.AXES, paint=SHELL),
}
