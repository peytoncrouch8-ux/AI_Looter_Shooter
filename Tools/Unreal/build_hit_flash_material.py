"""Rebuilds M_FX_HitFlash (/Game/Weapons/FX) in place, the hit flash every creature, training dummy and egg sac wears as an
overlay (and the powers' FEmberTint), with its shape as parameters (SPIDER-LOOK, 2026-10-22). Until now it had no script:
it was made by hand (2026-09-28) as unlit, additive, used with skeletal meshes, emissive = Color x Intensity x
(0.35 + (1 - |N.V|)^2 x 1.5): an even glow over the whole body, brighter toward the silhouette.

Its new parameters keep that look by default (Even 0.35, Rim 1.5, RimPower 2), so the dummies, egg sacs and the powers'
tints are unchanged; UCreatureReactComponent sets a creature's to a rim of light with barely a lift over the body
(FCreatureHitFlash::Even 0.08, Rim 1.6, RimPower 3): under full auto the even glow washed a spider into a pale
see-through ghost. Color and Intensity as before (every user sets both).

Run it in the open editor (no C++ needed; the creatures' code sets the new parameters whether or not they exist yet):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_hit_flash_material.py"
It prints HITFLASH lines and "HITFLASH done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PATH = '/Game/Weapons/FX/M_FX_HitFlash'

CODE = """// Hit flash over a body: Even everywhere, plus a rim toward the silhouette.
float facing = saturate(abs(dot(normalize(N), normalize(V))));
return Even + pow(1.0 - facing, max(RimPower, 0.5)) * Rim;"""


def log(message):
    unreal.log(f'HITFLASH {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, clear_graph, finish), without its build (as build_vfx_materials.py)."""
    path = os.path.join(HERE, 'build_world_materials.py')
    with open(path, encoding='utf-8') as source:
        tree = ast.parse(source.read(), path)
    keep = [node for node in tree.body if isinstance(node, (ast.Import, ast.ImportFrom, ast.FunctionDef, ast.ClassDef))
            or (isinstance(node, ast.Assign) and all(isinstance(t, ast.Name) and t.id.isupper() for t in node.targets))]
    module = types.ModuleType('build_world_materials_helpers')
    module.__file__ = path
    exec(compile(ast.Module(body=keep, type_ignores=[]), path, 'exec'), module.__dict__)
    return module


def build():
    W = world_helpers()
    if unreal.EditorAssetLibrary.does_asset_exist(PATH):
        mat = unreal.load_asset(PATH)
        if not isinstance(mat, unreal.Material):
            raise RuntimeError(f'{PATH} exists and is not a material: nothing was changed')
        W.clear_graph(mat)
    else:
        folder, name = PATH.rsplit('/', 1)
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material,
                                                                     unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', False)
    g = W.Graph(mat)
    normal = g.node(unreal.MaterialExpressionVertexNormalWS, -1200, 0)
    camera = g.node(unreal.MaterialExpressionCameraVectorWS, -1200, 120)
    shape = g.custom(CODE, [
        ('N', normal, ''), ('V', camera, ''),
        ('Even', g.scalar('Even', 0.35, -1200, 240), ''),
        ('Rim', g.scalar('Rim', 1.5, -1200, 340), ''),
        ('RimPower', g.scalar('RimPower', 2.0, -1200, 440), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -800, 200, 'Hit flash shape')
    color = g.vector('Color', (1.0, 1.0, 1.0, 1.0), -800, -100)
    tinted = g.mul(color, '', g.scalar('Intensity', 1.0, -800, 0), '', -500, -50)
    glow = g.mul(tinted, '', shape, '', -250, 50)
    g.out(glow, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])
    scalars = sorted(str(n) for n in W.MEL.get_scalar_parameter_names(mat))
    log(f'{mat.get_path_name()} built (unlit, additive, skeletal): scalars {", ".join(scalars)}; color Color')


build()
log('done')
