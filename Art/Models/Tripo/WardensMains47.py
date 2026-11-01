"""Mains 4-7's Tripo models (Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md M45-1 and M67-3; GLBs in Art/Tripo/<Name>/):
Saint Ada's whole Reliquary and the barge's soul coal, each /Game/Art/Tripo/SM_<Name> (the swap paths the game already
tries). Built by Tools/Blender/looter_tripo_m47.py (its docstring: how and why).

    Tools\\artrun.ps1 -Export WardensMains47 -Family TripoM47      (export test into Intermediate\\ArtExport_TripoM47)
    LOOTER_TRIPO_ONLY=SoulCoal limits a run to those models.
"""
import looter_tripo_m47 as m47

for name in m47.names():
    m47.build(name)
