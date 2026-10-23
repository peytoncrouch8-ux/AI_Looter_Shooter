"""Builds the tracked objective's orange highlight (Missions/ObjectiveHighlightSubsystem.h) beside the world's masters:

  M_ObjectiveHighlight  /Game/Art/Materials/Masters: the overlay UObjectiveHighlightSubsystem puts on the meshes of what the
                  tracked mission's current objective is about once the player is near (UMeshComponent's overlay: the mesh
                  drawn once more, on top, only while it's lit). Unlit, translucent, one-sided, no textures: a soft fill where
                  the surface faces the eye and a brighter rim where it turns away (a Fresnel term on the vertex normal), so
                  the thing's shape stands out in the game's orange without hiding it, and a slow breath (PulseSpeed,
                  PulseDepth) so it reads as a sign and not as paint. Strength (0-1, eased by the code) fades it in and out.
                  MaskRadius over 0 keeps it to a sphere round MaskCenter (world cm), softening over its outer MaskSoftness:
                  a door that's part of a house's mesh lights up without the house. Push (cm) lifts it off the surface along
                  the normal, so it never loses the depth test to its own mesh. Nothing in it needs Lumen or Nanite: it looks
                  the same on Medium (every mesh's Nanite fallback) as on Epic (where the code shows a lit mesh's fallback,
                  since Nanite draws no overlay).
                  Figure 1 is a character's look (the code sets it on a story character's or a skinned figure's meshes):
                  the rim alone, held tighter, the body all but clear (FigureFillOpacity, FigureRimOpacity,
                  FigureRimPower in place of the fill and rim's), so the character's own look shows through. A thing's fill
                  over Sexton's near-black coat and shadowed face read as a solid orange figure (2026-10-14).

The code sets Color (LooterUI::Color::Accent, the mission tracker's orange), Strength, MaskCenter, MaskRadius and Figure;
the rest are the look's tuning, at these defaults. Used on static, skinned (the story characters' posed figures), morphing
and spline meshes, so a packaged game never swaps it for the default material.

It never touches another asset: the master's graph is rebuilt in place (instances stay linked), and an asset at its path
that isn't a material is left alone. It reuses build_world_materials.py's graph helpers (without running that module's
build). Run it in the open editor (it needs no C++):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_highlight_material.py"
It prints HIGHLIGHTMAT lines and "HIGHLIGHTMAT done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
NAME = 'M_ObjectiveHighlight'

# LooterUI::Color::Accent (sRGB 255, 159, 28) in linear: the default the code overwrites with the same colour.
ACCENT = (1.0, 0.3467, 0.0116, 1.0)
# Unlit, so its brightness is on the scene's scale: sunlit ground shows about 2.5 times its albedo, the clouds about 3. A
# fill a fifth opaque at 3 reads plainly orange in full sun without washing out what's under it.
DEFAULTS = {
    'Strength': 1.0,       # the code eases it 0-1; 1 here so the asset previews lit
    'Glow': 1.2,           # emissive multiplier on Color (3 washed orange to yellow in the golden light)
    'RimGlow': 1.6,        # the rim this much brighter than the fill
    'FillOpacity': 0.2,    # facing the eye
    'RimOpacity': 0.8,     # added where the surface turns edge-on
    'RimPower': 2.0,       # how tight the rim hugs the outline
    'PulseSpeed': 0.7,     # breaths a second
    'PulseDepth': 0.3,     # how far a breath dims it (0: steady)
    'MaskRadius': 0.0,     # cm; 0: the whole mesh (the code sets a spot's)
    'MaskSoftness': 0.45,  # the outer part of the spot's radius that fades
    'Push': 0.4,           # cm along the normal
    # A character's look (Figure 1, the code sets it): over a dark coat even a fifth of orange reads as paint, and a
    # figure's folds turn edge-on all over it, so its fill is all but gone and its rim hugs the outline.
    'Figure': 0.0,               # 0: a thing's look, 1: a character's
    'FigureFillOpacity': 0.0,    # its body: clear (even 0.03 of this unlit orange tints a near-black coat gold: NPC-LOOK 2026-10-23)
    'FigureRimOpacity': 0.9,     # its outline: a plain orange line
    'FigureRimPower': 5.0,       # tighter than a thing's rim (higher: a thinner line, and its folds catch less)
}

SHARED = """// The rim: where the surface turns from the eye it glows most, so the thing's outline stands out; facing the eye it keeps
// a soft fill, so it reads as lit rather than painted over. A character (Figure 1) keeps a tighter rim.
float Facing = saturate(dot(normalize(Normal), normalize(CameraVector)));
float Rim = pow(1.0 - Facing, lerp(RimPower, FigureRimPower, Figure));
// A slow breath between 1 - PulseDepth and 1.
float Pulse = 1.0 - PulseDepth * (0.5 + 0.5 * sin(Time * PulseSpeed * 6.2831853));
// A spot: only within MaskRadius of MaskCenter (a door in a house's mesh), softening over its outer MaskSoftness.
float Mask = 1.0;
if (MaskRadius > 0.0)
{
    float Edge = max(MaskRadius * MaskSoftness, 1.0);
    Mask = 1.0 - smoothstep(MaskRadius - Edge, MaskRadius, length(WorldPos - MaskCenter));
}
"""
OPACITY = SHARED + """// A character's body all but clear, so its own look shows through; its rim a plain line.
float Fill = lerp(FillOpacity, FigureFillOpacity, Figure);
float RimShare = lerp(RimOpacity, FigureRimOpacity, Figure);
return saturate((Fill + RimShare * Rim) * Pulse * Mask * Strength);"""
COLOR = SHARED + 'return Color * Glow * lerp(1.0, RimGlow, Rim);'


def log(message):
    unreal.log(f'HIGHLIGHTMAT {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, its paths), without its build: that module builds its
    masters as it's run, so only its imports, definitions and constants are run here (as build_decal_materials.py does)."""
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
    """The master, its graph emptied to build again; refuses an asset at its path that isn't a material."""
    path = f'{W.FOLDER}/{NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material(NAME)


def usages():
    """The mesh kinds it's drawn on; any the editor's Python doesn't name are left out, and said."""
    wanted = ('MATUSAGE_SKELETAL_MESH', 'MATUSAGE_MORPH_TARGETS', 'MATUSAGE_SPLINE_MESH')
    found = [getattr(unreal.MaterialUsage, each) for each in wanted if hasattr(unreal.MaterialUsage, each)]
    missing = [each for each in wanted if not hasattr(unreal.MaterialUsage, each)]
    if missing:
        unreal.log_warning(f'HIGHLIGHTMAT no {", ".join(missing)} in unreal.MaterialUsage: set them by hand on {NAME}')
    return found


def build():
    mat = master()
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', False)
    g = W.Graph(mat)
    normal = g.node(unreal.MaterialExpressionVertexNormalWS, -1300, -200)
    camera = g.node(unreal.MaterialExpressionCameraVectorWS, -1300, -100)
    world = g.node(unreal.MaterialExpressionWorldPosition, -1300, 0)
    time = g.node(unreal.MaterialExpressionTime, -1300, 100)
    color = g.vector('Color', ACCENT, -1700, -400)
    center = g.vector('MaskCenter', (0.0, 0.0, 0.0, 0.0), -1700, -250)
    s = {key: g.scalar(key, value, -1700, -100 + 60 * i) for i, (key, value) in enumerate(DEFAULTS.items())}
    shared = [('Normal', normal, ''), ('CameraVector', camera, ''), ('WorldPos', world, ''), ('Time', time, ''),
              ('MaskCenter', center, ''), ('MaskRadius', s['MaskRadius'], ''), ('MaskSoftness', s['MaskSoftness'], ''),
              ('RimPower', s['RimPower'], ''), ('PulseSpeed', s['PulseSpeed'], ''), ('PulseDepth', s['PulseDepth'], ''),
              ('Figure', s['Figure'], ''), ('FigureRimPower', s['FigureRimPower'], '')]
    opacity = g.custom(OPACITY, shared + [(key, s[key], '') for key in ('FillOpacity', 'RimOpacity', 'FigureFillOpacity',
                                                                        'FigureRimOpacity', 'Strength')],
                       unreal.CustomMaterialOutputType.CMOT_FLOAT1, -800, 200, 'Highlight opacity')
    emissive = g.custom(COLOR, shared + [('Color', color, ''), ('Glow', s['Glow'], ''), ('RimGlow', s['RimGlow'], '')],
                        unreal.CustomMaterialOutputType.CMOT_FLOAT3, -800, -200, 'Highlight color')
    push = g.mul(normal, '', s['Push'], '', -800, 500)
    g.out(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(push, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    W.finish(mat, usages())
    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    for needed in ('Strength', 'MaskRadius', 'Figure'):
        if needed not in scalars:
            raise RuntimeError(f'{NAME} has no scalar {needed}: the code could not drive it')
    for needed in ('Color', 'MaskCenter'):
        if needed not in vectors:
            raise RuntimeError(f'{NAME} has no colour {needed}: the code could not drive it')
    log(f'{mat.get_path_name()} built (unlit, translucent, one-sided): scalars {", ".join(scalars)}; '
        f'colors {", ".join(vectors)}')
    return mat


build()
log('done')
