"""SK_FP_Arms_Odessa: Odessa Lark's arms in first person -- black kid gloves with short flared cuffs, the white shirt
cuff, the wine tailcoat's fitted sleeve under its darker silk turned-back cuff -- on SK_FP_Arms's skeleton (Ellis's,
bone for bone: Art/Models/Characters/FPArms.py). Built by Tools/Blender/fp_arms_heroes.py (its docstring has the parts,
sets and looks); the rig's numbers go to Intermediate/FPArms/FPArms_Odessa_rig.json; the look sheet with the guns is
Tools/Blender/fp_arms_heroes_sheet.py.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Characters\\FPArmsOdessa.py -Threads 6 [-Preview]
        [-ScriptArgs keep-textures|rebake]
    powershell -NoProfile -File Tools\\artrun.ps1 -Export FPArmsOdessa -Family FPArmsOdessa        (the export test)
"""
import fp_arms_heroes

ARM, BODY, REST = fp_arms_heroes.main('Odessa')
