"""SK_FP_Arms_Crane: Hollis Crane's arms in first person -- bare grey-green dead hands, bony, the slate coat's loose
sleeve under its torn turned-back cuff -- on SK_FP_Arms's skeleton (Ellis's, bone for bone: Art/Models/Characters/
FPArms.py). Built by Tools/Blender/fp_arms_heroes.py (its docstring has the parts, sets and looks); the rig's numbers go
to Intermediate/FPArms/FPArms_Crane_rig.json; the look sheet with the guns is Tools/Blender/fp_arms_heroes_sheet.py.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Characters\\FPArmsCrane.py -Threads 6 [-Preview]
        [-ScriptArgs keep-textures|rebake]
    powershell -NoProfile -File Tools\\artrun.ps1 -Export FPArmsCrane -Family FPArmsCrane        (the export test)
"""
import fp_arms_heroes

ARM, BODY, REST = fp_arms_heroes.main('Crane')
