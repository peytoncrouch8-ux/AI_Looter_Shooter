"""SK_FP_Arms_Pike: Wendell Pike's arms in first person -- bare weathered hands, the khaki drill field coat's sleeve
under its turned-back cuff with a buttoned tab and a brass button -- on SK_FP_Arms's skeleton (Ellis's, bone for bone:
Art/Models/Characters/FPArms.py). Built by Tools/Blender/fp_arms_heroes.py (its docstring has the parts, sets and
looks); the rig's numbers go to Intermediate/FPArms/FPArms_Pike_rig.json; the look sheet with the guns is
Tools/Blender/fp_arms_heroes_sheet.py.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Characters\\FPArmsPike.py -Threads 6 [-Preview]
        [-ScriptArgs keep-textures|rebake]
    powershell -NoProfile -File Tools\\artrun.ps1 -Export FPArmsPike -Family FPArmsPike        (the export test)
"""
import fp_arms_heroes

ARM, BODY, REST = fp_arms_heroes.main('Pike')
