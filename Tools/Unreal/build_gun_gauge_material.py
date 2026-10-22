"""Builds the gun gauge master beside the world's masters (Phase 12.3, the frontier-tech guns):

  M_GunGauge  /Game/Art/Materials/Masters: the glow the game drives on a gun part's GunAmmoGlow slot (the Rustler SMG's
              helix: the rounds left) and GunHeatGlow slot (the Thresher LMG's heat rings: the barrel's heat).
              UWeaponModelComponent gives each gun its own instance of it on those slots, with the slot's own Color
              (the art's ember orange, from the imported MI_GunAmmoGlow / MI_GunHeatGlow), and sets Fill and Glow as the
              gun fires (AWeaponBase::RefreshGauges). Opaque, lit: the part's UV0.x runs 0 at its feed end (the helix's
              back, the barrel nut) to 1 at its far end (Tools/Blender/looter_guns.py along_uv); where UV0.x is under Fill
              it glows Color x Glow (a hair hotter and whiter right at the lit edge, so the level reads), and past it it's
              the same color dark and dull, like an unlit tube. Edge softens the cut.

Nothing in it needs Lumen or Nanite (gun parts are drawn without Nanite). Without this master the gun's slot keeps its
stylized glow and only dims and brightens (UWeaponModelComponent::SetAmmoGauge's fallback), so the game runs either way.

It never touches another asset: the master's graph is rebuilt in place (instances stay linked), and an asset at its path
that isn't a material is left alone. It reuses build_world_materials.py's graph helpers (without running that module's
build), as build_highlight_material.py does. Run it in the open editor (the C++ only reads the master by its path):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_gun_gauge_material.py"
It prints GAUGEMAT lines and "GAUGEMAT done". A packaged game must cook it: it's read by path, so keep
/Game/Art/Materials/Masters among the directories always cooked (Docs/Release.md).
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
NAME = 'M_GunGauge'

# The guns' ember glow (looter_guns.AMMO_GLOW, #ff8a2e) in linear: the default the code overwrites with the slot's own.
EMBER = (1.0, 0.2541, 0.0273, 1.0)
DEFAULTS = {
    'Fill': 1.0,          # the code sets it: the share lit, from UV0.x 0
    'Glow': 4.0,          # the code sets it: emissive strength of the lit part
    'Edge': 0.03,         # how soft the lit edge is (UV0.x)
    'EdgeHot': 0.6,       # how much hotter (whiter) the lit edge glows
    'DarkShare': 0.12,    # the unlit part's color, as a share of Color (a dark tube, not black)
    'Roughness': 0.35,
}

LIT = """// Lit from the feed end up to Fill, softly; the lit edge a touch hotter, so the level reads at a glance.
float Lit = 1.0 - smoothstep(Fill - Edge, Fill + Edge, U);
Lit *= step(0.0001, Fill);
float Rim = Lit * (1.0 - smoothstep(0.0, Edge * 3.0, Fill - U));
return Color * Glow * Lit + float3(1.0, 0.85, 0.6) * Glow * EdgeHot * Rim;"""
DARK = 'return Color * DarkShare;'


def log(message):
    unreal.log(f'GAUGEMAT {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, its paths), without its build: only its imports,
    definitions and constants are run here (as build_highlight_material.py does)."""
    path = os.path.join(HERE, 'build_world_materials.py')
    with open(path, encoding='utf-8') as source:
        tree = ast.parse(source.read(), path)
    keep = []
    for node in tree.body:
        if isinstance(node, (ast.Import, ast.ImportFrom, ast.FunctionDef, ast.ClassDef)):
            keep.append(node)
        elif isinstance(node, ast.Assign) and all(isinstance(t, ast.Name) and t.id.isupper() for t in node.targets):
            keep.append(node)
    module = types.ModuleType('build_world_materials_helpers')
    module.__file__ = path
    exec(compile(ast.Module(body=keep, type_ignores=[]), path, 'exec'), module.__dict__)
    return module


W = world_helpers()
MEL = W.MEL


def master():
    path = f'{W.FOLDER}/{NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material(NAME)


def build():
    mat = master()
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property('two_sided', False)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1300, -100, coordinate_index=0)
    u = g.node(unreal.MaterialExpressionComponentMask, -1100, -100, r=True, g=False, b=False, a=False)
    g.link(coords, '', u, '')
    color = g.vector('Color', EMBER, -1300, -300)
    s = {key: g.scalar(key, value, -1300, 60 * i) for i, (key, value) in enumerate(DEFAULTS.items())}
    lit = g.custom(LIT, [('U', u, ''), ('Fill', s['Fill'], ''), ('Edge', s['Edge'], ''), ('Color', color, ''),
                         ('Glow', s['Glow'], ''), ('EdgeHot', s['EdgeHot'], '')],
                   unreal.CustomMaterialOutputType.CMOT_FLOAT3, -800, -200, 'Gauge glow')
    dark = g.custom(DARK, [('Color', color, ''), ('DarkShare', s['DarkShare'], '')],
                    unreal.CustomMaterialOutputType.CMOT_FLOAT3, -800, 100, 'Gauge tube')
    g.out(lit, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(dark, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(s['Roughness'], '', unreal.MaterialProperty.MP_ROUGHNESS)
    W.finish(mat, [])
    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    for needed in ('Fill', 'Glow'):
        if needed not in scalars:
            raise RuntimeError(f'{NAME} has no scalar {needed}: the code could not drive it')
    if 'Color' not in vectors:
        raise RuntimeError(f'{NAME} has no colour Color: the code could not tint it')
    log(f'{mat.get_path_name()} built (opaque, lit): scalars {", ".join(scalars)}; colors {", ".join(vectors)}')
    return mat


build()
log('done')
