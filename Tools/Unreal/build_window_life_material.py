"""Builds the window life's one material (World/WindowLifeSubsystem.h, World/WindowLifeCard.h) beside the world's masters:

  M_WindowLife   /Game/Art/Materials/Masters: the card AWindowLifeCard stands just in front of a lived-in house's lamplit
                 pane (the house model's SOCKET_Window_<n>), inside the wall's depth, behind the sash bars and the
                 curtains the house is built with. Unlit, translucent, one-sided, no textures: its shapes are drawn here,
                 in centimetres across the card (the plane's local position: the code stands the engine's plane with its
                 X up the window and its face out through the glass, sized CardWidth x CardHeight).
                 Kind 0, a shadow: somebody between the lamp and the glass, a soft head (HeadRadius, HeadHeight over the
                 pane's middle), shoulders (ShoulderWidth, ShoulderDepth at ShoulderHeight) and the body under them
                 (TrunkWidth), all blurred by Blur so it reads as a shadow on linen, never a figure with a face. Slide
                 moves it across the window, Rise bobs it as it walks. Color near black.
                 Kind 1, a curtain: linen pulled in from one side (Side +1 or -1), its edge at Slide (cm from the middle),
                 bowing toward the middle by BowDepth as it swings (Swing 0-1), soft over CurtainSoft, in folds
                 (FoldDepth, every FoldSpacing cm) and a little shaded where it turns at its edge. Color the linen's.
                 Everything fades out within EdgeSoft of the card's edges, so the card never shows as a line; past the
                 pane the wall hides the rest. Strength (0-1) fades the whole of it.

The code sets Kind, Slide, Rise, Swing, Side, Strength, Color, CardWidth and CardHeight on its own instance; the rest are
the look's tuning, at these defaults. It never touches another asset: the master's graph is rebuilt in place (instances
stay linked), and an asset at its path that isn't a material is left alone. It reuses build_world_materials.py's graph
helpers (without running that module's build). Run it in the open editor (it needs no C++):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_window_life_material.py"
It prints WINDOWLIFEMAT lines and "WINDOWLIFEMAT done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
NAME = 'M_WindowLife'

# The shadow's near black, a little warm (WindowLifeRules::ShadowColor): the default the code overwrites.
SHADOW = (0.012, 0.008, 0.006, 1.0)
# What the code drives (defaults so the asset previews a shadow in the middle of a pane), then the look's tuning.
DRIVEN = {
    'Kind': 0.0,          # 0 a shadow, 1 a curtain
    'Slide': 0.0,         # cm across: the shadow's middle, or the curtain's edge
    'Rise': 0.0,          # cm: a walker's bob
    'Swing': 0.0,         # a curtain's pull, 0-1
    'Side': 1.0,          # the side a curtain comes from (+1 or -1)
    'Strength': 1.0,      # the code fades it 0-1; 1 here so the asset previews
    'CardWidth': 130.0,   # cm (AWindowLifeCard::Width, Height)
    'CardHeight': 170.0,
}
TUNING = {
    'Blur': 7.0,             # cm: how soft the shadow's edges are (a lamp's shadow through linen is never sharp)
    'HeadRadius': 12.0,      # cm, a little over life: a shadow thrown on the glass from a lamp behind is larger
    'HeadHeight': 22.0,      # cm over the pane's middle (a standing grown-up behind a ground-floor sash)
    'ShoulderHeight': -8.0,  # cm: the shoulders' middle
    'ShoulderWidth': 27.0,   # cm: half across
    'ShoulderDepth': 16.0,   # cm: half up and down
    'TrunkWidth': 20.0,      # cm: half across the body under the shoulders
    'EdgeSoft': 8.0,         # cm: everything fades out this near the card's edges
    'CurtainSoft': 2.5,      # cm: the curtain's edge
    'BowDepth': 6.0,         # cm: how far the swung curtain's edge bows in at its middle
    'FoldDepth': 0.12,       # how much darker the folds' hollows are
    'FoldSpacing': 7.0,      # cm between folds
}

SHARED = """// The card's own place in cm from its middle: X across the window, Y up (the plane's local Y and X: the code stands it with
// its X up the window and its face out through the glass).
float2 P = float2(Local.y * CardWidth, Local.x * CardHeight) / 100.0;
// Nothing near the card's edges: the wall hides what's past the pane, and the card never shows as a line.
float2 Half = float2(CardWidth, CardHeight) * 0.5;
float Edge = (1.0 - smoothstep(Half.x - EdgeSoft, Half.x, abs(P.x))) * (1.0 - smoothstep(Half.y - EdgeSoft, Half.y, abs(P.y)));
float Shape = 0.0;
float Into = 0.0;
if (Kind < 0.5)
{
    // Somebody between the lamp and the glass: a soft head, shoulders and the body under them. Never a face.
    float2 Q = P - float2(Slide, Rise);
    float Head = 1.0 - smoothstep(-Blur, Blur, length(Q - float2(0.0, HeadHeight)) - HeadRadius);
    float2 S = (Q - float2(0.0, ShoulderHeight)) / float2(ShoulderWidth, ShoulderDepth);
    float Shoulders = 1.0 - smoothstep(-Blur, Blur, (length(S) - 1.0) * ShoulderDepth);
    float Trunk = (1.0 - smoothstep(-Blur, Blur, abs(Q.x) - TrunkWidth)) * (1.0 - smoothstep(-Blur, Blur, Q.y - ShoulderHeight));
    Shape = max(Head, max(Shoulders, Trunk));
}
else
{
    // Linen pulled in from one side: its edge at Slide, bowing in toward the middle as it swings.
    float Bow = BowDepth * Swing * (1.0 - saturate(abs(P.y + 10.0) / (Half.y * 0.8)));
    Into = (P.x - (Slide - Side * Bow)) * Side;
    Shape = smoothstep(-CurtainSoft, CurtainSoft, Into);
}
"""
OPACITY = SHARED + 'return saturate(Shape * Edge * Strength);'
COLOR = SHARED + """if (Kind < 0.5)
{
    return Color;
}
// The linen's folds, and a little shade where it turns at its edge.
float Folds = 1.0 - FoldDepth * (0.5 + 0.5 * sin(Into / FoldSpacing * 6.2831853 + Swing * 2.0));
float Turn = lerp(0.82, 1.0, saturate(Into / 8.0));
return Color * Folds * Turn;"""


def log(message):
    unreal.log(f'WINDOWLIFEMAT {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, its paths), without its build: that module builds its
    masters as it's run, so only its imports, definitions and constants are run here (as build_highlight_material.py does)."""
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


def local_position(g):
    """The card's local position (cm in the plane's own units, -50 to 50 across X and Y, whatever the card's scale): the
    world position turned into the mesh's space, or the engine's Local Position node where Python doesn't name the
    transform's spaces."""
    spaces = getattr(unreal, 'MaterialPositionTransformSource', None)
    if spaces is not None and hasattr(spaces, 'TRANSFORMPOSSOURCE_WORLD'):
        world = g.node(unreal.MaterialExpressionWorldPosition, -1700, -300)
        local = g.node(unreal.MaterialExpressionTransformPosition, -1450, -300,
                       transform_source_type=spaces.TRANSFORMPOSSOURCE_WORLD,
                       transform_type=spaces.TRANSFORMPOSSOURCE_LOCAL)
        g.link(world, '', local, '')
        return local
    log('no MaterialPositionTransformSource in Python: using the Local Position node')
    return g.node(unreal.MaterialExpressionLocalPosition, -1450, -300)


def build():
    mat = master()
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', False)
    g = W.Graph(mat)
    local = local_position(g)
    color = g.vector('Color', SHADOW, -1700, -450)
    s = {}
    for i, (key, value) in enumerate(list(DRIVEN.items()) + list(TUNING.items())):
        s[key] = g.scalar(key, value, -1700, -150 + 60 * i)
    inputs = [('Local', local, '')] + [(key, s[key], '') for key in s]
    opacity = g.custom(OPACITY, inputs, unreal.CustomMaterialOutputType.CMOT_FLOAT1, -900, 200, 'Window life opacity')
    emissive = g.custom(COLOR, inputs + [('Color', color, '')], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -900, -200,
                        'Window life color')
    g.out(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    W.finish(mat, [])
    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    for needed in DRIVEN:
        if needed not in scalars:
            raise RuntimeError(f'{NAME} has no scalar {needed}: the code could not drive it')
    if 'Color' not in vectors:
        raise RuntimeError(f'{NAME} has no colour Color: the code could not drive it')
    log(f'{mat.get_path_name()} built (unlit, translucent, one-sided, no textures): scalars {", ".join(scalars)}; '
        f'colors {", ".join(vectors)}')
    return mat


build()
log('done')
