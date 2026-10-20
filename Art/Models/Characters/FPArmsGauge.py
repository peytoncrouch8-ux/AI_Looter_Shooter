"""SK_FP_Arms_Gauge: Gauge's arms in first person -- black enamel casings and polished brass, rivets, three riveted
fingers (thumb on thumb_*, the two on index_* and middle_*; ring and pinky bones present, unweighted) -- on SK_FP_Arms's
skeleton (Ellis's, bone for bone: Art/Models/Characters/FPArms.py). Built by Tools/Blender/fp_arms_heroes.py and
Tools/Blender/fp_arms_gauge.py; the rig's numbers go to Intermediate/FPArms/FPArms_Gauge_rig.json; the look sheet with
the guns is Tools/Blender/fp_arms_heroes_sheet.py.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Characters\\FPArmsGauge.py -Threads 6 [-Preview]
        [-ScriptArgs keep-textures|rebake]
    powershell -NoProfile -File Tools\\artrun.ps1 -Export FPArmsGauge -Family FPArmsGauge        (the export test)
"""
import fp_arms_heroes

ARM, BODY, REST = fp_arms_heroes.main('Gauge')
