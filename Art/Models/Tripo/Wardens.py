"""The Wardens' Tripo models for Landfall (Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md; the user's GLBs in Art/Tripo/<Name>/):
the Lamplighter, Vane's dredge barge and tender, the Great Lantern, the grapple, the dredge hose and its intake, the anchor
winch, the lantern post, the company loudspeaker post, the porch barricade's table, boards and sacks, the company rifle and
the soul-hose nozzle, each /Game/Art/Tripo/SM_<Name>. Built by Tools/Blender/looter_tripo_wardens.py (its docstring: how
and why); Delia's shotgun and Abel's hat are Art/Models/Props/TripoProps.py (/Game/Art/Props, where the game looks).

    Tools\models.ps1 -Only Wardens,TripoProps -Out Intermediate\ArtExport_Tripo -NoImport
    LOOTER_TRIPO_ONLY=Lamplighter,GreatLantern limits a run to those models.
    Tools\\artrun.ps1 -Script Art\\Models\\Tripo\\Wardens.py -Preview  (with LOOTER_TRIPO_ONLY=DredgeBarge): the reworked
    barge's previews in Saved/ArtPreviews/DredgeBargeRework.
"""
import looter_textures as lt
import looter_tripo_barge as barge
import looter_tripo_wardens as tw

for name in tw.names('Tripo'):
    obj, report = tw.build(name)
    tw.describe(name, report)
    if name == 'DredgeBarge' and lt.want_preview():
        barge.preview(obj)
