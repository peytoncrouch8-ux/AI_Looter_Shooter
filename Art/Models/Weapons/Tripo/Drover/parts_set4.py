"""Drover set 4, Meridian (instrument-precise: satin white, graphite, machined aluminium rings, graduation ticks, a cyan
line, a stippled grip): Ranger frame, Vented barrel, Moon6 cylinder, Pip dot sight, Target grip, made by Tripo
(multiview) from Tools/Blender/drover_views.py's views. Spec format: Tools/Blender/looter_tripo_gun.py; shared rules:
looter_tripo_drover.py. Not a model: Tools\models.ps1 skips parts_*.py.
The cyan lines the views drew (drover_dressing.py meri_body, meri_barrel) are too thin in Tripo's atlas to find as glow
faces: they are laid on the skin again as GunCyanGlow lines (the element's colour in the game, as the AR's Meridian set).
"""
import looter_tripo_drover as d

SHELL = d.SHELLS['Meridian']
TIP = d.BARREL_U + 15.0
BODY_LINE = [(0.9, 0.9), (4.0, 0.85), (8.9, 0.9)]
BARREL_LINE = [(d.ROD_END + 1.3, -1.2), (TIP - 1.2, -1.2)]

PARTS = {
    'Body_Ranger': dict(glb='DroverBody_Ranger.glb', axes=d.AXES, keep=d.keep_glows, cuts=d.window_cuts,
                        prepare=d.skin_line(BODY_LINE, 0.04, 'GunCyanGlow'), paint=SHELL),
    'Barrel_Vented': dict(glb='DroverBarrel_Vented.glb', axes=d.AXES, keep=d.keep_blade(TIP),
                          prepare=d.both(d.trim_blade(TIP), d.skin_line(BARREL_LINE, 0.035, 'GunCyanGlow')),
                          paint=SHELL),
    'Cylinder_Moon6': dict(glb='DroverCylinder_Moon6.glb', axes=d.AXES, keep=d.keep_rod_crane, box_from='gone',
                           prepare=d.drum(6, 2.15)),
    'Sight_Pip': dict(glb='DroverSight_Pip.glb', axes=d.AXES, keep=d.keep_glows, cuts=d.pip_cuts, paint=SHELL),
    # The target grip is the fittings' polymer (GunPolymerGrey: the maker's fittings paint), light grey here.
    'Grip_Target': dict(glb='DroverGrip_Target.glb', axes=d.AXES,
                        paint=dict(slot='GunPolymerGrey', test=d.is_light_grey, tint=0xb8bcc0, mode='luma',
                                   by='faces')),
}
