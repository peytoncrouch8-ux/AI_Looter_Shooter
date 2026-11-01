"""Mains 2-3's Tripo models (Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md "Mains 2-3"; GLBs in Art/Tripo/<Name>/): Sexton's
ferry, the shrouded dead, the Company hose-cart, the checkpoint's sandbags and blank board, the Company banner, each
/Game/Art/Tripo/SM_<Name> (the swap paths the game already tries). Built by Tools/Blender/looter_tripo_m23.py (its
docstring: how and why).

    Tools\\artrun.ps1 -Export WardensMains23 -Family TripoM23      (export test into Intermediate\\ArtExport_TripoM23)
    LOOTER_TRIPO_ONLY=SextonsFerry,CompanyBanner limits a run to those models.
"""
import looter_tripo_m23 as m23

for name in m23.names():
    m23.build(name)
