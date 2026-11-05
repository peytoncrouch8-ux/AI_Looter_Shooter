"""The reworked dredge barge's previews (looter_tripo_barge_check.preview; Art/Models/Tripo/Wardens.py -Preview with
LOOTER_TRIPO_ONLY=DredgeBarge renders them into Saved/ArtPreviews/DredgeBargeRework3/)."""
import os

import bmesh
import bpy

import looter_textures as lt
import looter_tripo_barge as barge
from looter_tripo_rework import B, D


def preview(obj):
    """Saved/ArtPreviews/DredgeBargeRework3/: Tripo's preview angle, the spyglass side, bow, stern, top, the well deck
    at eye height looking aft and forward, the berth gap, the hatch, a berth close-up, the stairs at eye height and
    from the side, and the -Y casing walk."""
    import looter_train as tk
    folder = os.path.join(lt.PREVIEW_DIR, 'DredgeBargeRework3')
    fx = barge.final_x
    bpy.context.view_layer.update()
    tk.render([obj], 'tripo_angle', view=(1.05, 1.0, 0.75), lens=50.0, fit=0.95, folder=folder)
    tk.render([obj], 'spyglass', view=(1.0, -0.08, 0.02), lens=70.0, fit=0.62, resolution=(1920, 1080), folder=folder)
    tk.render([obj], 'bow', view=(0.9, -1.3, 0.42), lens=40.0, fit=0.75, folder=folder)
    tk.render([obj], 'stern', view=(-0.9, 1.35, 0.38), lens=40.0, fit=0.75, folder=folder)
    tk.closeup([obj], 'top', B((fx(0.0), 0.0, 2500.0)), (0.0, 0.08, 1.0), 125.0, lens=35.0, resolution=(900, 1600),
               folder=folder)
    # At eye height on the well deck: from near the stairs looking aft at the house, from the house looking forward.
    eye = barge.WELL + 160.0
    aft, fore = fx(barge.FORE_CUT) + barge.FORE_DX - 200.0, fx(2005.0) + 150.0
    tk.closeup([obj], 'well_aft', B((aft - 1000.0, 0.0, eye - 80.0)), (0.0, -1.0, 0.08), 10.0, lens=22.0, folder=folder)
    tk.closeup([obj], 'well_fwd', B((fore + 1000.0, 0.0, eye)), (0.0, 1.0, 0.0), 10.0, lens=22.0, folder=folder)
    tk.closeup([obj], 'berth', B((barge.GAP_X, -900.0, barge.WELL + 60.0)), (1.0, 0.0, -0.05), 11.6, lens=26.0,
               folder=folder)
    tk.closeup([obj], 'hatch', B((sum(barge.HATCH[:2]) / 2.0, 0.0, barge.CROSS)), (1.0, 0.0, 1.2), 6.0, lens=30.0,
               folder=folder)
    # Close: the jumbo's hull side at the berth (its plate seams), and the -Y casing walk at eye height looking aft.
    tk.closeup([obj], 'berth_close', B((barge.GAP_X - 450.0, -820.0, 2150.0)), (1.0, 0.0, 0.1), 5.0, lens=35.0,
               folder=folder)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    scoop = barge.terrace_aft(bm)
    bm.free()
    tk.closeup([obj], 'stairs', B((scoop - 120.0, 300.0, barge.WELL + 70.0)), tuple(D((-1.0, -0.3, 0.22)).normalized()),
               4.2, lens=28.0, folder=folder)
    tk.closeup([obj], 'stairs_side', B((scoop - 90.0, 0.0, barge.WELL + 80.0)), tuple(D((-0.25, 1.0, 0.1)).normalized()),
               5.5, lens=30.0, folder=folder)
    walk_eye = (fx(-700.0), -800.0, barge.CROSS + 160.0)
    tk.closeup([obj], 'casing_walk', B((walk_eye[0] - 1000.0, -790.0, walk_eye[2] - 60.0)), (0.0, -1.0, 0.06), 10.0,
               lens=24.0, folder=folder)
