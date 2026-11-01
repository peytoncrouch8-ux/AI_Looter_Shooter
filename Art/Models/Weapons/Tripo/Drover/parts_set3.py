"""Drover set 3, Windfall (scrap: rust, galvanized patches, black friction tape, a hose clamp, copper wire, sea teal and
sky-blue paint): Scrapper frame, Comped barrel, Plain6 cylinder, Peep sight, Wrapped grip, made by Tripo (multiview)
from Tools/Blender/drover_views.py's views. Spec format: Tools/Blender/looter_tripo_gun.py; shared rules:
looter_tripo_drover.py. Not a model: Tools\models.ps1 skips parts_*.py.
"""
import looter_tripo_drover as d

SHELL = d.SHELLS['Windfall']
BLADE = d.BARREL_U + 14.0 - 3.25   # the Comped's blade sits behind its ports

PARTS = {
    # The Scrapper's frame is scrap iron (GunSalvage), no paintable shell.
    'Body_Scrapper': dict(glb='DroverBody_Scrapper.glb', axes=d.AXES, keep=d.keep_glows, cuts=d.vent_cuts,
                          glows={'GunEmberGlow': d.is_ember}),
    'Barrel_Comped': dict(glb='DroverBarrel_Comped.glb', axes=d.AXES, keep=d.keep_blade(BLADE),
                          prepare=d.trim_blade(BLADE), glows={'GunEmberGlow': d.is_ember}, paint=SHELL),
    'Cylinder_Plain6': dict(glb='DroverCylinder_Plain6.glb', axes=d.AXES, keep=d.keep_rod_crane,
                            box_from='gone',
                            prepare=d.drum(6, 2.15)),
    'Sight_Peep': dict(glb='DroverSight_Peep.glb', axes=d.AXES, cuts=d.peep_cuts(1.8, 0.68), paint=SHELL),
    # The wrap-around grip is the fittings' polymer (GunPolymerGrey: the maker's fittings paint), sky blue here.
    'Grip_Wrapped': dict(glb='DroverGrip_Wrapped.glb', axes=d.AXES,
                         paint=dict(slot='GunPolymerGrey', test=d.is_sky, tint=0x7d9fb5, mode='luma', by='faces')),
}
