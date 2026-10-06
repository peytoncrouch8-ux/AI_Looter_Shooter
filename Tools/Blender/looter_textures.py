"""Textures and textured materials for the stylized-realism art (Docs/TutorialIsland.md). One module, two jobs:

1. THE TEXTURE GENERATOR (numpy; runs in Blender's Python, or any Python with numpy). It writes every set into
   Art/Textures/<Set>/:
       T_<Set>_BC.png    base color, sRGB (the leaf atlases carry their mask in alpha)
       T_<Set>_N.png     normal map, DirectX convention (green down), as Unreal expects
       T_<Set>_ORM.png   linear: R ambient occlusion, G roughness, B metallic
       T_MacroNoise_M.png  (the one exception: a linear grayscale mask, soft large-scale variation)
   Every set is seamless (tiles in U and V) except WoodEndGrain (one log end, for cap_uv) and the leaf atlases (four
   sprites). Tileables are 1024 px, the house trim sheet and the leaf atlases 2048, the foliage palette and the end
   grain 512. Generation is reproducible (fixed seeds, numpy only: no mathutils.noise, which is seeded by the clock).
       blender -b --factory-startup --python Tools/Blender/looter_textures.py -- [--only HouseTrim Hay ...] [--preview]
   --preview also renders Saved/ArtPreviews/Textures/contact_sheet.png and HouseTrim.png (a house mock-up dressed with
   the trim sheet; --no-textures renders only those). --list lists the sets. A full run takes about two minutes.

2. HELPERS FOR SCRIPTED MODELS (Art/Models/<Category>/<Name>.py, where Tools/Blender is on sys.path):

       import looter_model as lm
       import looter_textures as lt

       trim = lt.material('HouseTrim')                         # one material covers most of a house
       wall = lm.box('Farmhouse', size=(6.0, 0.2, 2.4), center=(0.0, 0.0, 1.2))
       lt.assign(wall, trim)                                   # every face (or pass faces)
       lt.trim_uv(wall, lambda f: abs(f.normal.z) < 0.5, 'Siding', align='world', cut=True)
       lt.trim_uv(wall, lambda f: f.normal.z > 0.5, 'TrimTeal') # the top edge as teal painted trim
       rock = ...
       lt.assign(rock, lt.material('RockGranite'))
       lt.box_uv(rock, 'RockGranite', seed=3)                  # world-scale cube projection
       lt.bake_vertex_ao(wall)                                 # contact shading into the 'Col' alpha
       if lt.want_preview():
           lt.preview([wall, rock], lt.preview_path('Buildings', 'Farmhouse'))

   material(set_name, name=None, master=None, tint=None, uv_scale=1.0, **props)
       The material for a texture set; its name (default: the set's) names the Unreal instance, MI_<name>. It previews
       the textures in Blender and carries the custom properties the exporter reads:
         Master      'World' (opaque; most things), 'Gun' (World plus per-gun wear, for gun parts), 'WorldFoliage'
                     (masked, two-sided, wind) or 'Terrain'.
                     Default: the set's own (WorldFoliage for LeavesOak, LeavesBirch, NeedlesPine, FoliagePalette).
         TextureSet  the set, e.g. 'HouseTrim': the textures are /Art/Textures/<Set>/T_<Set>_BC|N|ORM.png.
         Tint        (only if given) '#RRGGBB' sRGB, multiplies the base color. Give it as 0xRRGGBB or '#RRGGBB'.
         UVScale     the material multiplies the mesh UVs by this (2.0 = the texture repeats twice as often).
         Kind        'Surface', or 'Foliage' for WorldFoliage (for the older exporter; see Art/README.md).
       WorldFoliage previews light back faces with the front face's normal, as the Unreal master does.
       Extra keyword arguments become custom properties too. A Terrain material names its macro map as the set and
       its detail sets in DetailSets (grass/soil first, rock second), e.g.
         lt.material('TutorialIslandMacro', name='Terrain', master='Terrain', DetailSets='GroundGrass,RockCliff')
       (a set made elsewhere is fine: its files are looked up in Art/Textures/<Set>/ all the same). Works before the
       textures exist (then it shows the set's average color). Materials of the same name are one material.
   assign(obj, mat, faces=None)
       Puts mat on the faces (all by default), adding it to the mesh's slots if needed.
   Wherever a function takes faces: None (every face), a face index or a list of them (or of polygons), a material
   (the faces using it), or a function given a FaceInfo (index, and in world space: normal, center, area; and
   material_index) that returns True for the faces wanted, e.g. lambda f: f.normal.z > 0.7.
   trim_uv(obj, faces, strip, u_offset=0.0, rotate=False, v_offset=0.0, align='face', cut=False, fit=False,
           stagger=None, axis=None)
       Maps faces onto one strip of the house trim sheet (see TRIM_STRIPS). Each face is projected in its own plane at
       world scale (320 px/m): U runs along the face (horizontal on walls, along the eaves on roofs, along the longer
       side on floors and tops), V up the face; rotate=True turns it a quarter (vertical boards). The strip tiles
       along U (every 6.4 m) and also stacks along V, but can't wrap there, so V is fitted into the strip:
         align='face'   each face's lowest edge sits at the strip's bottom, raised by v_offset meters. For pieces at
                        most one strip tall: a 20 cm beam face in 'Beams' (v_offset 0, 0.2, 0.4 or 0.6 picks one of
                        its four beam faces), a board, a 0.8 m band of siding.
         align='world'  V follows the face plane's height (world scale, modulo the strip height), so faces continue
                        one another across a wall cut by windows and doors.
         stagger        with align='world', slides each band along U by its own amount so stacked bands don't
                        repeat in a column (joints, knots). Default: on for strips whose band edges are clean breaks
                        (Siding, Logs, Beams, Shingles, the H trims), off where bands must line up (Stone, Tin, Plaster).
         cut=True       faces taller than the strip are first cut into bands (a new edge loop where the strip wraps),
                        so walls and roofs of any size keep world scale. It changes the mesh: use it before anything
                        that relies on face indices, and prefer it to one tall quad (the bands also give vertex AO
                        vertices to shade with).
         fit=True       stretches each face's height over the whole strip, whatever its size (U keeps world scale):
                        window panes on 'Glass' (dirt at the bottom, reflection at the top), straps on 'Iron', trim
                        boards of any width on 'TrimTeal' and 'TrimRed'.
         axis           a world direction for U to follow (projected into each face), e.g. a diagonal brace's
                        length: the grain follows the part without mapping it in a local frame first.
       A face still taller than the strip is squeezed to fit (with a warning): the strips never bleed into each other.
       u_offset (meters) slides the pattern along U so repeated walls don't match. Returns the mapped face indices.
       TRIM_JOINTS gives the boards' butt joints (meters along U) for 'Siding' (per board, bottom up), 'TrimTeal' and
       'TrimRed' (keys 'A', 'H1', 'H2'); PLANK_JOINTS the same for WoodPlanks (16 planks, bottom up).
   box_uv(obj, set_name, texel_density=None, faces=None, offset=(0.0, 0.0), along=None, seed=None)
       World-scale cube projection for tileable sets (320 px/m by default, 256 for GroundGrass and GroundDirt): each face
       takes the axis plane its normal faces most; side faces keep V up, so strata, bark furrows and plank rows stay
       level. along='long' turns each face so U follows its longest side (plank grain along posts and rails).
       offset (meters) or seed (random offset) makes copies differ.
   bake_vertex_ao(obj, samples=32, distance=1.0, ground=True, children=True, strength=1.0)
       Bakes ambient occlusion into the alpha of a byte-color, face-corner color attribute 'Col' (made if missing,
       RGB white; RGB is kept if already set). A = 1 means open. It bakes obj and (children=True) the meshes parented
       under it, occluded by the whole model (other models in the file don't count) and, with ground=True, by a
       ground plane at the model's origin (models stand on it; that's the contact shading SSAO no longer gives on
       Medium). distance (m) is how far occluders count. Rays pass through surfaces seen from behind, so vertices
       buried inside another part (a wall's end inside a post) don't go black. Modifiers are applied first (except
       Armature), as the exporter would. Vertex AO lives on vertices: add edge loops where the shading should change
       (cut=True in trim_uv does that on walls). Ray-cast in Python: about a second per 1,000 vertices at 32 samples.
   set_foliage_colors(obj, wind='height', wind_range=(0.0, 1.0), wind_power=1.0, variation='island', seed=0)
       Writes the foliage vertex color convention into 'Col' (byte, face corner), for M_WorldFoliage:
         R = wind weight: 0 at the trunk base or the roots, 1 at the leaf and blade tips
         G = random variation per cluster (0..1): the material shifts hue and brightness with it
         B = 0
         A = baked AO (kept if already baked, else 1; bake_vertex_ao keeps R, G and B)
       wind='height': height above the model's ground (its root's origin) over the object's top, to the power
       wind_power, remapped into wind_range; 'distance': distance from the model's origin (bushes); a number: that
       weight everywhere (e.g. 1.0 for leaf cards, with the trunk done separately at 0..0.3); or a function taking a
       world position (Vector) and returning 0..1. variation='island': one random value per connected part (a leaf
       card, a blade, a cluster); 'object': one for the whole object; a number; or None to keep G.
       Values go through the attribute's linear color API, which is what the exporter writes for textured materials
       (colors_type='LINEAR'), so Unreal gets them unchanged.
   cap_uv(obj, faces, seed=None, margin=0.02)
       Maps end caps (a log's or stump's saw-cut end) onto WoodEndGrain: each connected group of the faces is
       projected in its own plane, centered and scaled so its outline fits the texture's circle (rings in the middle,
       bark at the rim). seed turns each cap by a random angle.
   swatch_uv(obj, faces, swatch, axis='z')
       Maps grass blades, petals or clover onto a swatch of FoliagePalette (see PALETTE): U runs from 0 at each
       part's base to 1 at its tip (the swatch's gradient), V across the swatch. Parts are the connected islands of the
       faces; axis 'z' measures the base-to-tip direction along world Z, 'long' along each part's longest extent.
   preview(objects=None, out_png=None, resolution=(1280, 720), view=(-1.0, -1.6, 0.7), ground=True, ...,
           ground_at='bottom')
       Renders a neutral outdoor preview (Eevee; Workbench if Eevee can't run): a warm sun from the upper left, a soft
       sky, a ground plane (at the models' lowest point; ground_at='origin' puts it at the models' origin, the pivot's
       height, so roots and buried rocks go below it; a number is a height), the camera framing the objects (default:
       every model in the scene).
       Collision hulls and '_' helpers are hidden. Everything it adds is removed afterwards, so it's safe before an
       export. Returns the PNG path.
   preview_path(category, name) -> Saved/ArtPreviews/<category>/<name>.png
   want_preview() -> True when the script was started with --preview after '--' (or LOOTER_PREVIEW=1)
   texture_path(set_name, kind) -> the PNG of a set ('BC', 'N', 'ORM'); SETS, TRIM_STRIPS and PALETTE list what exists.

Conventions: meters, Z up, the model's front faces -Y (Art/README.md). Colors are sRGB hex. UV V runs up from the
bottom of the texture, as in Blender (the FBX importer flips it for Unreal).
"""
import math
import os
import sys

try:
    import bmesh
    import bpy
    from mathutils import Matrix, Vector
except ImportError:  # plain Python with numpy can still generate the textures
    bmesh = bpy = Matrix = Vector = None

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TEXTURE_DIR = os.path.join(REPO, 'Art', 'Textures')
PREVIEW_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews')

MASTERS = ('World', 'Gun', 'WorldFoliage', 'Terrain')

# Every texture set: size in px, the master it's made for, texel density (px per meter), and a placeholder look
# (sRGB color, roughness, metallic) for materials made before the textures exist.
SETS = {
    'HouseTrim':      dict(size=2048, density=320, color=0x8a7862, roughness=0.8),
    'GroundGrass':    dict(size=1024, density=256, color=0x5d6a36, roughness=0.95),
    'GroundDirt':     dict(size=1024, density=256, color=0x8b7556, roughness=0.95),
    'RockCliff':      dict(size=1024, density=320, color=0x8e8170, roughness=0.9),
    'RockGranite':    dict(size=1024, density=320, color=0x85837b, roughness=0.85),
    'WoodPlanks':     dict(size=1024, density=320, color=0x7f6d5a, roughness=0.85),
    'StoneWall':      dict(size=1024, density=320, color=0x7d786e, roughness=0.9),
    'MetalRust':      dict(size=1024, density=320, color=0x6f756a, roughness=0.6, metallic=0.4),
    # Neutral sets meant to be tinted: one texture serves gold, brass, gunmetal, or paint of any color.
    'MetalWorn':      dict(size=1024, density=320, color=0xc4c3c0, roughness=0.4, metallic=1.0),
    'PaintWorn':      dict(size=1024, density=320, color=0xd2d0ca, roughness=0.55),
    'Polymer':        dict(size=1024, density=1024, color=0xcbcbcb, roughness=0.6),
    'GunWood':        dict(size=1024, density=1024, color=0x6e4a30, roughness=0.5),
    'Hay':            dict(size=1024, density=320, color=0xbfa060, roughness=0.9),
    'BarkOak':        dict(size=1024, density=320, color=0x5b4f43, roughness=0.95),
    'BarkBirch':      dict(size=1024, density=320, color=0xd6d0c3, roughness=0.75),
    'BarkPine':       dict(size=1024, density=320, color=0x7b4f38, roughness=0.9),
    'LeavesOak':      dict(size=2048, density=0, color=0x51692f, roughness=0.65, master='WorldFoliage', alpha=True),
    'LeavesBirch':    dict(size=2048, density=0, color=0x6b8634, roughness=0.6, master='WorldFoliage', alpha=True),
    'NeedlesPine':    dict(size=2048, density=0, color=0x3f5a33, roughness=0.7, master='WorldFoliage', alpha=True),
    'FoliagePalette': dict(size=512, density=0, color=0x6c8a3c, roughness=0.7, master='WorldFoliage'),
    'MacroNoise':     dict(size=1024, density=0, color=0x808080, roughness=1.0, mask=True),
    'WoodEndGrain':   dict(size=512, density=0, color=0xa4876a, roughness=0.8),
}

# The house trim sheet (T_HouseTrim, 2048 px): V ranges from the bottom of the texture, and the strip height in the
# world (the strip's 256 or 64 px at 320 px/m). Each strip tiles along U (every 6.4 m) and stacks along V.
TRIM_SIZE = 2048
TRIM_DENSITY = 320.0
TRIM_STRIPS = {
    'A':  (0.875, 1.0),        # weathered siding: 4 boards (20 cm) along U, nail rows
    'B':  (0.75, 0.875),       # logs: 2 debarked logs (40 cm) along U, chinking between
    'C':  (0.625, 0.75),       # hewn beams: 4 beam faces (20 cm) along U, dark edges
    'D':  (0.5, 0.625),        # fieldstone masonry with mortar
    'E':  (0.375, 0.5),        # wooden shingles (shakes), 4 rows of 20 cm; the grain runs down the roof (V)
    'F':  (0.25, 0.375),       # corrugated tin, ridges along V (down the roof), rust streaks
    'G':  (0.125, 0.25),       # lime plaster, stained and cracked
    'H1': (0.09375, 0.125),    # teal painted trim board (20 cm)
    'H2': (0.0625, 0.09375),   # oxide-red painted trim board
    'H3': (0.03125, 0.0625),   # iron strap with rivets
    'H4': (0.0, 0.03125),      # dark window glass with a hint of reflection
}
# Strips whose top and bottom edges are clean breaks (board edges, chinking, shingle butts): stacked bands of these can
# slide along U without a visible seam, and trim_uv staggers them by default.
STAGGERED = ('A', 'B', 'C', 'E', 'H1', 'H2', 'H3', 'H4')
TRIM_ALIASES = {
    'Siding': 'A', 'Logs': 'B', 'Beams': 'C', 'Stone': 'D', 'Shingles': 'E', 'Tin': 'F', 'Plaster': 'G',
    'TrimTeal': 'H1', 'TrimRed': 'H2', 'Iron': 'H3', 'Glass': 'H4',
}

# FoliagePalette (512 px): len(PALETTE) equal horizontal swatches, numbered from the bottom of the texture: swatch i
# covers V i/len .. (i+1)/len. Each runs from its base color (U = 0, a blade's root) to its tip color (U = 1); across the
# swatch it's a little lighter mid-blade. New swatches are only ever appended, so an index never changes (but V does:
# compute it from len(PALETTE), as swatch_uv does, and export again after the palette grows).
PALETTE = [
    'GrassDeep', 'GrassFresh', 'GrassYellow', 'GrassOlive', 'GrassDry', 'Straw', 'Clover', 'CloverDark',
    'FlowerYellow', 'FlowerWhite', 'FlowerPurple', 'FlowerBlue', 'FlowerCenter', 'Stem', 'Fern', 'Reed',
    'FruitRed', 'CattailBrown', 'Moss', 'DryTan',
]

UNREAL_COLLISION = ('UCX_', 'USP_', 'UCP_')


def hex_color(value):
    """0xRRGGBB (or '#RRGGBB') in sRGB to linear RGBA."""
    value = _hex_int(value)

    def channel(byte):
        c = byte / 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return [channel((value >> 16) & 0xFF), channel((value >> 8) & 0xFF), channel(value & 0xFF), 1.0]


def _hex_int(value):
    if isinstance(value, str):
        return int(value.lstrip('#'), 16)
    return int(value)


def texture_path(set_name, kind):
    """The PNG of a set: kind is 'BC', 'N' or 'ORM' ('M' for MacroNoise)."""
    return os.path.join(TEXTURE_DIR, set_name, f'T_{set_name}_{kind}.png')


_OTHER_SET = dict(size=1024, density=256, color=0x808080, roughness=0.9)


def _set_info(set_name):
    """A set's entry in SETS. Sets made elsewhere (the terrain's macro map, Art/Textures/<Set>/) get neutral defaults."""
    if set_name not in SETS:
        if not os.path.isdir(os.path.join(TEXTURE_DIR, set_name)):
            _log(f"note: {set_name} isn't one of this module's sets ({', '.join(SETS)}) and Art/Textures/{set_name} "
                 f"doesn't exist yet")
        return _OTHER_SET
    return SETS[set_name]


def _log(message):
    print(f'LOOTER: {message}', flush=True)


# --- Materials ---

def _socket(sockets, identifier):
    return next(s for s in sockets if s.identifier == identifier)


def _multiply(nodes, links, color_output, factor_output, location):
    """color * factor (a color or a float shown as grey), through a Mix node."""
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    mix.blend_type = 'MULTIPLY'
    mix.location = location
    _socket(mix.inputs, 'Factor_Float').default_value = 1.0
    links.new(color_output, _socket(mix.inputs, 'A_Color'))
    if isinstance(factor_output, (list, tuple)):
        _socket(mix.inputs, 'B_Color').default_value = factor_output
    else:
        links.new(factor_output, _socket(mix.inputs, 'B_Color'))
    return _socket(mix.outputs, 'Result_Color')


def _image(path, colorspace):
    image = bpy.data.images.load(path, check_existing=True)
    image.colorspace_settings.name = colorspace
    return image


def material(set_name, name=None, master=None, tint=None, uv_scale=1.0, **props):
    """The material for a texture set (see the module's docstring). Calling it again with the same name rebuilds the
    same material."""
    info = _set_info(set_name)
    master = master or info.get('master', 'World')
    if master not in MASTERS:
        raise ValueError(f"master must be one of {', '.join(MASTERS)}, not {master!r}")
    name = name or set_name
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()

    out = nodes.new('ShaderNodeOutputMaterial')
    out.location = (1100, 0)
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.location = (800, 0)
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])

    placeholder = hex_color(info['color'])
    tint_rgba = hex_color(tint) if tint is not None else None
    flat = [c * t for c, t in zip(placeholder, tint_rgba)] if tint_rgba else placeholder
    flat[3] = 1.0
    # The older exporter reads the Principled BSDF's base color, so it gets the set's average color.
    bsdf.inputs['Base Color'].default_value = flat
    bsdf.inputs['Roughness'].default_value = info.get('roughness', 0.8)
    bsdf.inputs['Metallic'].default_value = info.get('metallic', 0.0)
    mat.diffuse_color = flat

    bc_path, n_path, orm_path = (texture_path(set_name, kind) for kind in ('BC', 'N', 'ORM'))
    uv_vector = None
    if any(os.path.exists(p) for p in (bc_path, n_path, orm_path)):
        coords = nodes.new('ShaderNodeTexCoord')
        coords.location = (-1100, 0)
        mapping = nodes.new('ShaderNodeMapping')
        mapping.location = (-900, 0)
        mapping.inputs['Scale'].default_value = (uv_scale, uv_scale, 1.0)
        links.new(coords.outputs['UV'], mapping.inputs['Vector'])
        uv_vector = mapping.outputs['Vector']

    def image_node(path, colorspace, y):
        node = nodes.new('ShaderNodeTexImage')
        node.image = _image(path, colorspace)
        node.location = (-650, y)
        links.new(uv_vector, node.inputs['Vector'])
        return node

    if os.path.exists(bc_path):
        bc = image_node(bc_path, 'sRGB', 300)
        color = bc.outputs['Color']
        if info.get('alpha'):
            # Masked, like the Unreal material: an alpha test at 0.5.
            clip = nodes.new('ShaderNodeMath')
            clip.operation = 'GREATER_THAN'
            clip.inputs[1].default_value = 0.5
            clip.location = (-350, 450)
            links.new(bc.outputs['Alpha'], clip.inputs[0])
            links.new(clip.outputs['Value'], bsdf.inputs['Alpha'])
    else:
        rgb = nodes.new('ShaderNodeRGB')
        rgb.outputs['Color'].default_value = placeholder
        rgb.location = (-650, 300)
        color = rgb.outputs['Color']
    if tint_rgba:
        color = _multiply(nodes, links, color, tint_rgba, (-250, 300))

    # Ambient occlusion: the texture's (ORM red) times the mesh's (vertex color alpha, baked by bake_vertex_ao). Unreal
    # applies it to the ambient light; the preview simply darkens the base color with it.
    vertex = nodes.new('ShaderNodeVertexColor')
    vertex.layer_name = 'Col'
    vertex.location = (-650, -500)
    occlusion = vertex.outputs['Alpha']
    if os.path.exists(orm_path):
        orm = image_node(orm_path, 'Non-Color', -150)
        split = nodes.new('ShaderNodeSeparateColor')
        split.location = (-350, -150)
        links.new(orm.outputs['Color'], split.inputs['Color'])
        links.new(split.outputs['Green'], bsdf.inputs['Roughness'])
        links.new(split.outputs['Blue'], bsdf.inputs['Metallic'])
        both = nodes.new('ShaderNodeMath')
        both.operation = 'MULTIPLY'
        both.location = (-250, -400)
        links.new(split.outputs['Red'], both.inputs[0])
        links.new(occlusion, both.inputs[1])
        occlusion = both.outputs['Value']
    color = _multiply(nodes, links, color, occlusion, (100, 250))
    links.new(color, bsdf.inputs['Base Color'])

    if os.path.exists(n_path):
        # The file is DirectX (green down); Blender's Normal Map node wants OpenGL (green up).
        normal = image_node(n_path, 'Non-Color', -500)
        split = nodes.new('ShaderNodeSeparateColor')
        split.location = (-350, -600)
        links.new(normal.outputs['Color'], split.inputs['Color'])
        flip = nodes.new('ShaderNodeMath')
        flip.operation = 'SUBTRACT'
        flip.inputs[0].default_value = 1.0
        flip.location = (-150, -650)
        links.new(split.outputs['Green'], flip.inputs[1])
        join = nodes.new('ShaderNodeCombineColor')
        join.location = (50, -600)
        links.new(split.outputs['Red'], join.inputs['Red'])
        links.new(flip.outputs['Value'], join.inputs['Green'])
        links.new(split.outputs['Blue'], join.inputs['Blue'])
        normal_map = nodes.new('ShaderNodeNormalMap')
        normal_map.location = (300, -600)
        links.new(join.outputs['Color'], normal_map.inputs['Color'])
        links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])

    if master == 'WorldFoliage':
        mat.use_backface_culling = False
        if hasattr(mat, 'surface_render_method'):
            mat.surface_render_method = 'DITHERED'
        # Back faces keep the front face's (authored) normal, as M_WorldFoliage does: a card is lit the same from
        # both sides instead of its back going dark.
        linked = bsdf.inputs['Normal'].links
        if linked:
            shading = linked[0].from_socket
        else:
            geometry = nodes.new('ShaderNodeNewGeometry')
            geometry.location = (300, -850)
            shading = geometry.outputs['Normal']
        facing = nodes.new('ShaderNodeNewGeometry')
        facing.location = (300, -1050)
        sign = nodes.new('ShaderNodeMath')
        sign.operation = 'MULTIPLY_ADD'
        sign.inputs[1].default_value = -2.0
        sign.inputs[2].default_value = 1.0
        sign.location = (500, -1000)
        links.new(facing.outputs['Backfacing'], sign.inputs[0])
        flip = nodes.new('ShaderNodeVectorMath')
        flip.operation = 'SCALE'
        flip.location = (650, -850)
        links.new(shading, flip.inputs[0])
        links.new(sign.outputs['Value'], flip.inputs['Scale'])
        links.new(flip.outputs['Vector'], bsdf.inputs['Normal'])

    mat['Master'] = master
    mat['TextureSet'] = set_name
    mat['UVScale'] = float(uv_scale)
    if tint is not None:
        mat['Tint'] = '#{:06X}'.format(_hex_int(tint))
    elif 'Tint' in mat:
        del mat['Tint']
    mat['Kind'] = 'Foliage' if master == 'WorldFoliage' else 'Surface'
    for key, value in props.items():
        mat[key] = value
    return mat


def assign(obj, mat, faces=None):
    """Puts mat on the faces of obj (all by default), adding it to the mesh's material slots if needed."""
    mesh = obj.data
    index = mesh.materials.find(mat.name)
    if index < 0:
        mesh.materials.append(mat)
        index = len(mesh.materials) - 1
    for i in _face_indices(obj, faces):
        mesh.polygons[i].material_index = index


# --- Face selection ---

class FaceInfo:
    """What a face filter sees: the face's index, and its normal, center and area in world space (Vector, meters), and
    its material slot index."""
    __slots__ = ('index', 'normal', 'center', 'area', 'material_index')

    def __init__(self, index, normal, center, area, material_index):
        self.index, self.normal, self.center, self.area, self.material_index = index, normal, center, area, material_index


def _face_indices(obj, faces):
    """faces: None (every face), a face index, a list of indices or polygons, a material (the faces using it), or a
    function taking a FaceInfo and returning True for the faces wanted."""
    mesh = obj.data
    if faces is None:
        return list(range(len(mesh.polygons)))
    if isinstance(faces, int):
        return [faces]
    if bpy is not None and isinstance(faces, bpy.types.Material):
        slot = mesh.materials.find(faces.name)
        return [p.index for p in mesh.polygons if p.material_index == slot] if slot >= 0 else []
    if callable(faces):
        matrix = obj.matrix_world
        normal_matrix = matrix.to_3x3().inverted_safe().transposed()
        scale = abs(matrix.to_3x3().determinant())
        picked = []
        for p in mesh.polygons:
            info = FaceInfo(p.index, (normal_matrix @ p.normal).normalized(), matrix @ p.center, p.area * scale,
                            p.material_index)
            if faces(info):
                picked.append(p.index)
        return picked
    return [f if isinstance(f, int) else f.index for f in faces]


# --- UV mapping ---

def _uv_layer(bm):
    return bm.loops.layers.uv.active or bm.loops.layers.uv.new('UVMap')


def _world_bmesh(obj):
    """obj's mesh as a bmesh in world space (so distances are world meters); _write_back puts it back."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.transform(obj.matrix_world)
    bm.normal_update()
    bm.faces.ensure_lookup_table()
    return bm


def _write_back(obj, bm):
    bm.transform(obj.matrix_world.inverted_safe())
    bm.normal_update()
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def _face_axes(normal):
    """A face's own U (along it: horizontal on walls and roofs, +X on floors) and V (up it), both unit vectors in its
    plane, U x V = the normal (so textures are never mirrored)."""
    if abs(normal.z) < 0.95:
        up = Vector((0.0, 0.0, 1.0))
    else:
        up = Vector((0.0, 1.0, 0.0)) if normal.z > 0.0 else Vector((0.0, -1.0, 0.0))
    v_axis = (up - normal * up.dot(normal)).normalized()
    u_axis = v_axis.cross(normal).normalized()
    return u_axis, v_axis


def _strip_range(strip):
    key = TRIM_ALIASES.get(strip, strip)
    if key not in TRIM_STRIPS:
        raise ValueError(f"unknown trim strip {strip!r}; use {', '.join(TRIM_STRIPS)} or {', '.join(TRIM_ALIASES)}")
    v0, v1 = TRIM_STRIPS[key]
    return v0, v1, (v1 - v0) * TRIM_SIZE / TRIM_DENSITY


def _cut_bands(bm, face, u_axis, v_axis, height, start):
    """Cuts a face with planes across v_axis at start + k * height; returns the pieces."""
    coords = [v.co.dot(v_axis) for v in face.verts]
    lo, hi = min(coords), max(coords)
    k = math.floor((lo - start) / height) + 1
    pieces = [face]
    while start + k * height < hi - 1e-4:
        level = start + k * height
        if level > lo + 1e-4:
            # In mesh order (dict keys keep it), never sets: those iterate by memory address, so the cut changed
            # from run to run.
            geom = (list(pieces) + list(dict.fromkeys(e for f in pieces for e in f.edges))
                    + list(dict.fromkeys(v for f in pieces for v in f.verts)))
            result = bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-5, plane_co=v_axis * level, plane_no=v_axis)
            pieces = [g for g in result['geom'] if isinstance(g, bmesh.types.BMFace)]
        k += 1
    return pieces


def trim_uv(obj, faces, strip, u_offset=0.0, rotate=False, v_offset=0.0, align='face', cut=False, fit=False,
            stagger=None, axis=None):
    """Maps faces of obj onto a strip of the house trim sheet (see the module's docstring). Returns the face indices."""
    if align not in ('face', 'world'):
        raise ValueError("align is 'face' or 'world'")
    if fit:
        align, cut, v_offset = 'face', False, 0.0
    if stagger is None:
        stagger = TRIM_ALIASES.get(strip, strip) in STAGGERED
    v0, v1, height = _strip_range(strip)
    inset = 1.0 / TRIM_SIZE  # a texel of margin: bilinear filtering never reaches the next strip
    span = (v1 - v0) - 2.0 * inset
    wanted = set(_face_indices(obj, faces))
    bm = _world_bmesh(obj)
    uv = _uv_layer(bm)
    targets = [f for f in bm.faces if f.index in wanted]

    along = Vector(axis).normalized() if axis is not None else None

    def axes(face):
        u_axis, v_axis = _face_axes(face.normal)
        if along is not None:
            # U follows the given direction (projected into the face), whatever the part's orientation.
            projected = along - face.normal * along.dot(face.normal)
            if projected.length > 1e-6:
                u_axis = projected.normalized()
                v_axis = face.normal.cross(u_axis).normalized()
        elif abs(face.normal.z) >= 0.95:
            # Floors and tops: U along the face's longer side (boards run along a beam's top, a sill, a deck).
            xs = [v.co.x for v in face.verts]
            ys = [v.co.y for v in face.verts]
            if max(ys) - min(ys) > (max(xs) - min(xs)) * 1.01:
                u_axis = Vector((0.0, 1.0, 0.0))
                v_axis = face.normal.cross(u_axis).normalized()
        return (v_axis, -u_axis) if rotate else (u_axis, v_axis)

    # Each face's axes; the pieces a face is cut into keep the parent's (a non-planar face's pieces would otherwise
    # each lean a little differently and drop a vertex into the next band).
    frames = {}
    if cut:
        pieces = []
        for face in targets:
            u_axis, v_axis = axes(face)
            coords = [v.co.dot(v_axis) for v in face.verts]
            start = -v_offset if align == 'world' else min(coords) - v_offset
            if max(coords) - min(coords) > height + 1e-4 or align == 'world':
                cut_pieces = _cut_bands(bm, face, u_axis, v_axis, height, start)
            else:
                cut_pieces = [face]
            for piece in cut_pieces:
                frames[piece] = (u_axis, v_axis, start)
            pieces.extend(cut_pieces)
        targets = [f for f in pieces if f.is_valid]

    squeezed = 0
    for face in targets:
        u_axis, v_axis, start = frames[face] if face in frames else axes(face) + (None,)
        s = [loop.vert.co.dot(u_axis) for loop in face.loops]
        t = [loop.vert.co.dot(v_axis) for loop in face.loops]
        lo, hi = min(t), max(t)
        if fit:
            f = [(ti - lo) / max(hi - lo, 1e-6) for ti in t]
        elif align == 'face' and start is None:
            f = [(ti - lo + v_offset) / height for ti in t]
        else:
            # Bands counted from the cut's start (align='face' with cut) or from the plane's zero (align='world'); the
            # band is the one the piece's middle is in, so a vertex a hair across a boundary stays in its band.
            origin = -v_offset if start is None or align == 'world' else start
            f = [(ti - origin) / height for ti in t]
            band = math.floor((min(f) + max(f)) * 0.5)
            f = [fi - band for fi in f]
            if stagger and align == 'world':
                # Each band slides along the strip by its own amount, so stacked bands don't repeat in a column.
                s = [si + ((band * 0.618034) % 1.0) * TRIM_SIZE / TRIM_DENSITY for si in s]
        if max(f) > 1.0 + 0.02 or min(f) < -0.02:
            # Taller than the strip (or across a band edge): squeeze it into the strip rather than bleed.
            squeezed += 1
            low, high = max(min(f), 0.0), min(max(f), 1.0)
            if high - low < 0.25:
                low, high = 0.0, 1.0
            f = [low + (fi - min(f)) / max(max(f) - min(f), 1e-6) * (high - low) for fi in f]
        shift = math.floor((min(s) + u_offset) * TRIM_DENSITY / TRIM_SIZE)
        for loop, si, fi in zip(face.loops, s, f):
            u = (si + u_offset) * TRIM_DENSITY / TRIM_SIZE - shift
            loop[uv].uv = (u, v0 + inset + min(max(fi, 0.0), 1.0) * span)
    if squeezed:
        _log(f'warning: trim_uv squeezed {squeezed} faces of {obj.name} taller than strip {strip} '
             f'({height:.2f} m); use cut=True or smaller faces to keep world scale')
    bm.faces.index_update()
    mapped = [f.index for f in targets]
    _write_back(obj, bm)
    return mapped


def box_uv(obj, set_name, texel_density=None, faces=None, offset=(0.0, 0.0), along=None, seed=None):
    """World-scale cube projection for a tileable set (see the module's docstring)."""
    info = _set_info(set_name)
    density = texel_density or info['density'] or 320
    scale = density / float(info['size'])  # UV units per meter
    if seed is not None:
        import random
        rng = random.Random(seed)
        repeat = info['size'] / float(density)
        offset = (offset[0] + rng.uniform(0.0, repeat), offset[1] + rng.uniform(0.0, repeat))
    wanted = set(_face_indices(obj, faces))
    bm = _world_bmesh(obj)
    uv = _uv_layer(bm)
    x, y, z = Vector((1.0, 0.0, 0.0)), Vector((0.0, 1.0, 0.0)), Vector((0.0, 0.0, 1.0))
    for face in bm.faces:
        if face.index not in wanted:
            continue
        n = face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        if axis == 2:
            u_axis, v_axis = x, (y if n.z > 0.0 else -y)
        elif axis == 0:
            u_axis, v_axis = (y if n.x > 0.0 else -y), z
        else:
            u_axis, v_axis = (-x if n.y > 0.0 else x), z
        if along == 'long':
            us = [v.co.dot(u_axis) for v in face.verts]
            vs = [v.co.dot(v_axis) for v in face.verts]
            if max(vs) - min(vs) > max(us) - min(us):
                u_axis, v_axis = v_axis, -u_axis
        for loop in face.loops:
            co = loop.vert.co
            loop[uv].uv = ((co.dot(u_axis) + offset[0]) * scale, (co.dot(v_axis) + offset[1]) * scale)
    _write_back(obj, bm)


def swatch_uv(obj, faces, swatch, axis='z'):
    """Maps blades, petals or clover onto a FoliagePalette swatch (see the module's docstring)."""
    index = PALETTE.index(swatch) if isinstance(swatch, str) else int(swatch)
    rows = len(PALETTE)
    v_lo = (index + 0.15) / rows
    v_hi = (index + 0.85) / rows
    wanted = set(_face_indices(obj, faces))
    bm = _world_bmesh(obj)
    uv = _uv_layer(bm)
    chosen = [f for f in bm.faces if f.index in wanted]
    for island in _islands(chosen):
        verts = list(dict.fromkeys(v for f in island for v in f.verts))  # in mesh order: the same on every run
        if axis == 'long':
            points = [v.co for v in verts]
            center = sum(points, Vector()) / len(points)
            direction = max((p - center for p in points), key=lambda d: d.length)
            direction = direction.normalized() if direction.length > 1e-9 else Vector((0.0, 0.0, 1.0))
            if direction.z < 0.0:
                direction = -direction
        else:
            direction = Vector((0.0, 0.0, 1.0))
        along = [v.co.dot(direction) for v in verts]
        lo, hi = min(along), max(along)
        side = direction.cross(Vector((0.0, 0.0, 1.0)) if abs(direction.z) < 0.9 else Vector((1.0, 0.0, 0.0)))
        if side.length < 1e-9:
            side = Vector((1.0, 0.0, 0.0))
        side.normalize()
        across = [v.co.dot(side) for v in verts]
        a_lo, a_hi = min(across), max(across)
        for face in island:
            for loop in face.loops:
                co = loop.vert.co
                u = (co.dot(direction) - lo) / max(hi - lo, 1e-6)
                w = (co.dot(side) - a_lo) / max(a_hi - a_lo, 1e-6)
                loop[uv].uv = (min(max(u, 0.01), 0.99), v_lo + w * (v_hi - v_lo))
    _write_back(obj, bm)


def cap_uv(obj, faces, seed=None, margin=0.02):
    """Maps end caps (a log's or a stump's cut end, a post top) onto WoodEndGrain: each connected group of the faces
    is projected in its own plane, centered, and scaled so its outline fits the texture's circle. seed turns each cap
    by a random angle so the ends differ. Returns nothing."""
    import random
    rng = random.Random(seed)
    wanted = set(_face_indices(obj, faces))
    bm = _world_bmesh(obj)
    uv = _uv_layer(bm)
    for island in _islands([f for f in bm.faces if f.index in wanted]):
        normal = sum((f.normal * f.calc_area() for f in island), Vector())
        normal = normal.normalized() if normal.length > 1e-9 else Vector((0.0, 0.0, 1.0))
        u_axis, v_axis = _face_axes(normal)
        if seed is not None:
            turn = rng.uniform(0.0, 2.0 * math.pi)
            u_axis, v_axis = (u_axis * math.cos(turn) + v_axis * math.sin(turn),
                              -u_axis * math.sin(turn) + v_axis * math.cos(turn))
        verts = list(dict.fromkeys(v for f in island for v in f.verts))  # in mesh order: the same on every run
        us = [v.co.dot(u_axis) for v in verts]
        vs = [v.co.dot(v_axis) for v in verts]
        cu, cv = (min(us) + max(us)) * 0.5, (min(vs) + max(vs)) * 0.5
        radius = max(math.hypot(a - cu, b - cv) for a, b in zip(us, vs)) or 1.0
        scale = (0.5 - margin) / radius
        for f in island:
            for loop in f.loops:
                co = loop.vert.co
                loop[uv].uv = (0.5 + (co.dot(u_axis) - cu) * scale, 0.5 + (co.dot(v_axis) - cv) * scale)
    _write_back(obj, bm)


def _islands(faces):
    """Groups faces into connected islands (sharing edges), in the order the faces come: seeded from a set, the
    islands came out in memory-address order, which changes from run to run (and each island's random turn with it)."""
    remaining = set(faces)
    islands = []
    for seed in faces:
        if seed not in remaining:
            continue
        remaining.remove(seed)
        island, stack = [seed], [seed]
        while stack:
            face = stack.pop()
            for edge in face.edges:
                for other in edge.link_faces:
                    if other in remaining:
                        remaining.remove(other)
                        island.append(other)
                        stack.append(other)
        islands.append(island)
    return islands


# --- Vertex colors ---

def _model_root(obj):
    while obj.parent is not None:
        obj = obj.parent
    return obj


def _descendants(obj):
    found = []
    for child in obj.children:
        found.append(child)
        found.extend(_descendants(child))
    return found


def _is_render_mesh(obj):
    return obj.type == 'MESH' and not obj.name.startswith(UNREAL_COLLISION + ('_',))


def _col_attribute(mesh):
    """The 'Col' byte-color face-corner attribute (made white if missing), made active and the one that renders."""
    attrs = mesh.color_attributes
    col = attrs.get('Col')
    if col is not None and (col.domain != 'CORNER' or col.data_type != 'BYTE_COLOR'):
        attrs.remove(col)
        col = None
    if col is None:
        col = attrs.new('Col', 'BYTE_COLOR', 'CORNER')
        col.data.foreach_set('color_srgb', [1.0] * (4 * len(mesh.loops)))
    attrs.active_color = col
    try:
        attrs.render_color_index = attrs.active_color_index
    except AttributeError:
        pass
    return mesh.color_attributes['Col']


def _read_col(mesh):
    """'Col' as values (linear): what the exporter writes for textured models (colors_type='LINEAR')."""
    import numpy as np
    col = _col_attribute(mesh)
    raw = np.empty(4 * len(mesh.loops), dtype=np.float32)
    col.data.foreach_get('color', raw)
    return raw.reshape(-1, 4)


def _write_col(mesh, raw):
    col = _col_attribute(mesh)
    col.data.foreach_set('color', raw.astype('float32').ravel())
    mesh.update()


def _apply_modifiers(obj):
    for modifier in list(obj.modifiers):
        if modifier.type == 'ARMATURE':
            continue
        with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj],
                                       selected_editable_objects=[obj]):
            bpy.ops.object.modifier_apply(modifier=modifier.name)


def _hemisphere(samples):
    """Cosine-weighted directions around +Z on a Fibonacci spiral: evenly spread, so few samples give smooth AO."""
    import numpy as np
    k = np.arange(samples) + 0.5
    r = np.sqrt(k / samples)
    phi = k * math.pi * (3.0 - math.sqrt(5.0))
    return np.stack([r * np.cos(phi), r * np.sin(phi), np.sqrt(np.clip(1.0 - r * r, 0.0, 1.0))], axis=-1)


def _corner_normals(mesh):
    import numpy as np
    normals = np.empty(3 * len(mesh.loops), dtype=np.float64)
    try:
        mesh.corner_normals.foreach_get('vector', normals)
    except AttributeError:  # Blender before 4.1
        mesh.calc_normals_split()
        mesh.loops.foreach_get('normal', normals)
    return normals.reshape(-1, 3)


def bake_vertex_ao(obj, samples=32, distance=1.0, ground=True, children=True, strength=1.0):
    """Bakes ambient occlusion into the alpha of obj's 'Col' attribute (see the module's docstring). Rays from each
    vertex (per face corner) look for the model's surfaces within distance; they pass through surfaces seen from
    behind, so a vertex buried inside another part (a wall end inside a post) shades like the open space around it
    instead of going black."""
    import numpy as np
    from mathutils.bvhtree import BVHTree
    if bpy.context.object is not None and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    root = _model_root(obj)
    targets = [obj] + ([o for o in _descendants(obj) if _is_render_mesh(o)] if children else [])
    targets = [o for o in targets if o.type == 'MESH' and len(o.data.polygons)]
    for o in targets:
        if o.data.users > 1:
            o.data = o.data.copy()
        _apply_modifiers(o)
    occluders = [o for o in [root] + _descendants(root) if _is_render_mesh(o)]
    occluders += [o for o in targets if o not in occluders]

    # One tree of the whole model (and the ground under its origin), in world space.
    depsgraph = bpy.context.evaluated_depsgraph_get()
    verts, polys = [], []
    for o in occluders:
        evaluated = o.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        matrix = o.matrix_world
        base = len(verts)
        verts.extend(matrix @ v.co for v in mesh.vertices)
        polys.extend([base + i for i in p.vertices] for p in mesh.polygons)
        evaluated.to_mesh_clear()
    if ground:
        z = root.matrix_world.translation.z
        base = len(verts)
        size = 1000.0
        verts.extend(Vector(c) for c in ((-size, -size, z), (size, -size, z), (size, size, z), (-size, size, z)))
        polys.append([base, base + 1, base + 2, base + 3])
    tree = BVHTree.FromPolygons(verts, polys, epsilon=0.0)

    directions = _hemisphere(samples)
    rng = np.random.default_rng(0)
    bias = 1e-3 * max(1.0, distance)
    for o in targets:
        mesh = o.data
        matrix = o.matrix_world
        normal_matrix = np.array(matrix.to_3x3().inverted_safe().transposed())
        m = np.array(matrix)
        co = np.empty(3 * len(mesh.vertices), dtype=np.float64)
        mesh.vertices.foreach_get('co', co)
        world = co.reshape(-1, 3) @ m[:3, :3].T + m[:3, 3]
        loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
        mesh.loops.foreach_get('vertex_index', loop_vert)
        normals = _corner_normals(mesh) @ normal_matrix.T
        normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-9)
        # Corners that share a vertex and a normal share a result.
        keys = np.concatenate([loop_vert[:, None].astype(np.float64), np.round(normals * 50.0)], axis=1)
        unique, inverse = np.unique(keys, axis=0, return_inverse=True)
        first = np.zeros(len(unique), dtype=np.int64)
        first[inverse[::-1]] = np.arange(len(inverse))[::-1]
        results = np.ones(len(unique), dtype=np.float64)
        for u, loop in enumerate(first):
            n = Vector(normals[loop])
            p = Vector(world[loop_vert[loop]]) + n * bias
            helper = Vector((1.0, 0.0, 0.0)) if abs(n.x) < 0.9 else Vector((0.0, 1.0, 0.0))
            t = n.cross(helper).normalized()
            b = n.cross(t)
            turn = rng.uniform(0.0, 2.0 * math.pi)
            ct, st = math.cos(turn), math.sin(turn)
            hidden = 0.0
            for dx, dy, dz in directions:
                rx, ry = dx * ct - dy * st, dx * st + dy * ct
                d = t * rx + b * ry + n * dz
                origin, left = p, distance
                for _ in range(8):
                    hit, hit_normal, _, dist = tree.ray_cast(origin, d, left)
                    if hit is None:
                        break
                    if hit_normal.dot(d) > 0.0:       # a surface seen from behind: we're inside a part, go on
                        origin = hit + d * 1e-4
                        left -= dist + 1e-4
                        if left <= 0.0:
                            break
                        continue
                    reach = (distance - left + dist) / distance
                    hidden += 1.0 - smooth(0.5, 1.0, reach)
                    break
            results[u] = 1.0 - hidden / len(directions)
        ao = results[inverse]
        raw = _read_col(mesh)
        raw[:, 3] = 1.0 - strength * (1.0 - np.clip(ao, 0.0, 1.0))
        _write_col(mesh, raw)


def set_foliage_colors(obj, wind='height', wind_range=(0.0, 1.0), wind_power=1.0, variation='island', seed=0):
    """Writes the foliage vertex color convention into obj's 'Col' (see the module's docstring)."""
    import numpy as np
    mesh = obj.data
    matrix = obj.matrix_world
    root = _model_root(obj)
    pivot = root.matrix_world.translation
    count = len(mesh.vertices)
    co = np.empty(3 * count, dtype=np.float64)
    mesh.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    m = np.array(matrix)
    world = co @ m[:3, :3].T + m[:3, 3]

    if callable(wind):
        weight = np.array([float(wind(Vector(p))) for p in world])
    elif isinstance(wind, (int, float)):
        weight = np.full(count, float(wind))
    elif wind == 'height':
        top = world[:, 2].max()
        weight = (world[:, 2] - pivot.z) / max(top - pivot.z, 1e-6)
    elif wind == 'distance':
        dist = np.linalg.norm(world - np.array(pivot), axis=1)
        weight = dist / max(dist.max(), 1e-6)
    else:
        raise ValueError("wind is 'height', 'distance', a number or a function")
    weight = np.clip(weight, 0.0, 1.0) ** wind_power
    weight = wind_range[0] + weight * (wind_range[1] - wind_range[0])

    raw = _read_col(mesh)
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    raw[:, 0] = weight[loop_vert]
    if variation is not None:
        rng = np.random.default_rng(seed)
        if variation == 'island':
            labels = _vertex_islands(mesh)
            values = rng.random(labels.max() + 1 if count else 1)
            raw[:, 1] = values[labels][loop_vert]
        elif variation == 'object':
            raw[:, 1] = rng.random()
        else:
            raw[:, 1] = float(variation)
    raw[:, 2] = 0.0
    _write_col(mesh, np.clip(raw, 0.0, 1.0))


def _vertex_islands(mesh):
    """A label per vertex: which connected part (through edges) it belongs to."""
    import numpy as np
    parent = list(range(len(mesh.vertices)))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a
    for edge in mesh.edges:
        a, b = find(edge.vertices[0]), find(edge.vertices[1])
        if a != b:
            parent[a] = b
    roots = np.array([find(i) for i in range(len(parent))], dtype=np.int64)
    _, labels = np.unique(roots, return_inverse=True)
    return labels


# --- Previews ---

def preview_path(category, name):
    """Saved/ArtPreviews/<category>/<name>.png"""
    return os.path.join(PREVIEW_DIR, category, name + '.png')


def want_preview():
    """True when the script was started with --preview after '--', or with LOOTER_PREVIEW=1."""
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    return '--preview' in argv or os.environ.get('LOOTER_PREVIEW') == '1'


def _preview_world(scene):
    world = bpy.data.worlds.new('_PreviewWorld')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    background.inputs['Strength'].default_value = 1.0
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    links.new(split.outputs['Z'], ramp.inputs['Fac'])
    # A soft afternoon sky: warm haze at the horizon, blue above, a darker ground bounce below.
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = 0.42, hex_color(0x6f6a5e)
    elements[1].position, elements[1].color = 0.9, hex_color(0x6f93c4)
    mid = elements.new(0.52)
    mid.color = hex_color(0xc9cfcf)
    links.new(ramp.outputs['Color'], background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def _preview_objects(objects):
    if objects is None:
        objects = [o for o in bpy.context.scene.objects if o.parent is None and _is_render_mesh(o)]
    shown = []
    for obj in objects:
        for o in [obj] + _descendants(obj):
            if _is_render_mesh(o) and o not in shown:
                shown.append(o)
    return shown


def _bounds(objects):
    points = [o.matrix_world @ Vector(corner) for o in objects for corner in o.bound_box]
    lo = Vector((min(p.x for p in points), min(p.y for p in points), min(p.z for p in points)))
    hi = Vector((max(p.x for p in points), max(p.y for p in points), max(p.z for p in points)))
    return lo, hi


def preview(objects=None, out_png=None, resolution=(1280, 720), view=(-1.0, -1.6, 0.7), ground=True, samples=32,
            lens=40.0, sun_strength=4.0, fit=1.1, ground_at='bottom'):
    """Renders a neutral outdoor preview of objects (see the module's docstring). Returns the PNG path."""
    scene = bpy.context.scene
    if bpy.context.object is not None and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    shown = _preview_objects(objects)
    if not shown:
        raise ValueError('preview: nothing to show')
    out_png = out_png or preview_path('Misc', 'preview')
    if not os.path.isabs(out_png):
        out_png = os.path.join(REPO, out_png)
    out_png = os.path.normpath(out_png)
    os.makedirs(os.path.dirname(out_png), exist_ok=True)

    lo, hi = _bounds(shown)
    center = (lo + hi) * 0.5
    radius = max((hi - lo).length * 0.5, 0.05)
    added = []
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        if o not in shown:
            o.hide_render = True

    camera = bpy.data.objects.new('_PreviewCamera', bpy.data.cameras.new('_PreviewCamera'))
    camera.data.lens = lens
    camera.data.sensor_fit = 'HORIZONTAL'
    scene.collection.objects.link(camera)
    added.append(camera)
    direction = Vector(view).normalized()
    width, height = resolution
    half_fov = math.atan(camera.data.sensor_width / (2.0 * lens))
    if height > width:
        half_fov = math.atan(math.tan(half_fov) * width / height)
    else:
        half_fov = math.atan(math.tan(half_fov) * height / width)
    distance = radius * fit / math.sin(half_fov)
    camera.location = center + direction * distance
    camera.rotation_euler = (-direction).to_track_quat('-Z', 'Y').to_euler()
    camera.data.clip_start = max(distance * 0.01, 0.01)
    camera.data.clip_end = distance * 10.0 + 1000.0

    # The sun: from the upper left of the view and in front of the model (it lights the front and the left side of
    # the default view), warm and fairly hard (a small disc). It turns with the view.
    turn = math.atan2(direction.y, direction.x) - math.atan2(-1.6, -1.0)
    base = Vector((-0.55, -0.7, 0.0))
    toward_sun = Vector((base.x * math.cos(turn) - base.y * math.sin(turn),
                         base.x * math.sin(turn) + base.y * math.cos(turn), 0.0)).normalized()
    toward_sun = (toward_sun * math.cos(math.radians(45.0)) + Vector((0.0, 0.0, math.sin(math.radians(45.0))))).normalized()
    sun = bpy.data.objects.new('_PreviewSun', bpy.data.lights.new('_PreviewSun', 'SUN'))
    sun.data.energy = sun_strength
    sun.data.color = (1.0, 0.9, 0.78)
    sun.data.angle = math.radians(2.0)
    sun.rotation_euler = (-toward_sun).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    added.append(sun)

    ground_obj = None
    if ground:
        size = max(radius * 300.0, 2000.0)
        mesh = bpy.data.meshes.new('_PreviewGround')
        mesh.from_pydata([(-size, -size, 0.0), (size, -size, 0.0), (size, size, 0.0), (-size, size, 0.0)], [],
                         [(0, 1, 2, 3)])
        ground_obj = bpy.data.objects.new('_PreviewGround', mesh)
        if ground_at == 'origin':
            ground_z = min(_model_root(o).matrix_world.translation.z for o in shown)
        elif isinstance(ground_at, (int, float)):
            ground_z = float(ground_at)
        else:
            ground_z = lo.z
        ground_obj.location = (center.x, center.y, ground_z)
        ground_mat = bpy.data.materials.get('_PreviewGround') or bpy.data.materials.new('_PreviewGround')
        ground_mat.use_nodes = True
        bsdf = next(n for n in ground_mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Base Color'].default_value = hex_color(0x77766c)
        bsdf.inputs['Roughness'].default_value = 1.0
        mesh.materials.append(ground_mat)
        scene.collection.objects.link(ground_obj)
        added.append(ground_obj)

    saved = dict(camera=scene.camera, world=scene.world, engine=scene.render.engine, x=scene.render.resolution_x,
                 y=scene.render.resolution_y, pct=scene.render.resolution_percentage, path=scene.render.filepath,
                 view=scene.view_settings.view_transform, look=scene.view_settings.look,
                 transparent=scene.render.film_transparent)
    world = _preview_world(scene)
    try:
        scene.camera = camera
        scene.world = world
        scene.render.resolution_x, scene.render.resolution_y = width, height
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = False
        scene.render.image_settings.file_format = 'PNG'
        scene.render.image_settings.color_mode = 'RGB'
        scene.render.filepath = out_png
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.look = 'AgX - Medium High Contrast'
        try:
            scene.render.engine = 'BLENDER_EEVEE_NEXT'
            scene.eevee.taa_render_samples = samples
            if hasattr(scene.eevee, 'use_shadows'):
                scene.eevee.use_shadows = True
            bpy.ops.render.render(write_still=True)
        except Exception as error:  # no GPU context: fall back to Workbench
            _log(f'preview: Eevee failed ({error}); rendering with Workbench')
            scene.render.engine = 'BLENDER_WORKBENCH'
            shading = scene.display.shading
            shading.light = 'STUDIO'
            shading.color_type = 'TEXTURE'
            shading.show_shadows = True
            shading.show_cavity = True
            bpy.ops.render.render(write_still=True)
    finally:
        scene.camera = saved['camera']
        scene.world = saved['world']
        scene.render.engine = saved['engine']
        scene.render.resolution_x, scene.render.resolution_y = saved['x'], saved['y']
        scene.render.resolution_percentage = saved['pct']
        scene.render.filepath = saved['path']
        scene.render.film_transparent = saved['transparent']
        scene.view_settings.view_transform = saved['view']
        scene.view_settings.look = saved['look']
        bpy.data.worlds.remove(world)
        for o in added:
            data = o.data
            bpy.data.objects.remove(o)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)
            elif isinstance(data, bpy.types.Light):
                bpy.data.lights.remove(data)
        for o, value in hidden.items():
            if o.name in scene.objects:
                o.hide_render = value
    _log(f'preview: {out_png}')
    return out_png


# --- The generator: toolkit ---
# Images are float32 numpy arrays with row 0 at the top of the texture (V = 1) and column 0 at U = 0. Colors are
# painted in sRGB (0..1), as a painter would; heights are in meters, so normal maps and occlusion come out at the
# texture's real scale (px_m: meters per pixel). Every operation wraps around the edges, so every texture tiles.

import numpy as np

GENERATORS = {}
SCRATCH_DIR = os.path.join(REPO, 'Intermediate', 'Textures_scratch')


def generator(name):
    def register(function):
        GENERATORS[name] = function
        return function
    return register


def rgb(value):
    """0xRRGGBB to an sRGB color (3 floats, 0..1)."""
    return np.array([(value >> 16) & 255, (value >> 8) & 255, value & 255], np.float32) / 255.0


def _freqs(shape):
    fy = np.fft.fftfreq(shape[0]).astype(np.float32)[:, None]
    fx = np.fft.rfftfreq(shape[1]).astype(np.float32)[None, :]
    return fx, fy


def _gauss(shape, sx, sy):
    fx, fy = _freqs(shape)
    return np.exp(-2.0 * np.pi ** 2 * ((sx * fx) ** 2 + (sy * fy) ** 2)).astype(np.float32)


def blur(a, sx, sy=None):
    """Gaussian blur (sigma in px, per axis), wrapping around."""
    sy = sx if sy is None else sy
    if a.ndim == 3:
        return np.stack([blur(a[..., c], sx, sy) for c in range(a.shape[2])], axis=-1)
    f = np.fft.rfft2(a) * _gauss(a.shape, sx, sy)
    return np.fft.irfft2(f, s=a.shape).astype(np.float32)


def noise(shape, seed, sx, sy=None, octaves=1, gain=0.5):
    """Tileable noise, mean 0 and deviation 1: white noise blurred with sigma (sx, sy) px, octaves halving it."""
    sy = sx if sy is None else sy
    rng = np.random.default_rng(seed)
    out = np.zeros(shape, np.float32)
    amp, total = 1.0, 0.0
    for _ in range(octaves):
        f = np.fft.rfft2(rng.standard_normal(shape).astype(np.float32)) * _gauss(shape, sx, sy)
        n = np.fft.irfft2(f, s=shape).astype(np.float32)
        out += amp * n / (n.std() + 1e-8)
        total += amp * amp
        amp *= gain
        sx, sy = sx * 0.5, sy * 0.5
    return out / np.float32(math.sqrt(total))


def sample(a, x, y):
    """a at float pixel coordinates (x column, y row), bilinear, wrapping."""
    h, w = a.shape[:2]
    x0 = np.floor(x).astype(np.int64)
    y0 = np.floor(y).astype(np.int64)
    fx = (x - x0).astype(np.float32)
    fy = (y - y0).astype(np.float32)
    x0 %= w
    y0 %= h
    x1 = (x0 + 1) % w
    y1 = (y0 + 1) % h
    if a.ndim == 3:
        fx, fy = fx[..., None], fy[..., None]
    top = a[y0, x0] * (1 - fx) + a[y0, x1] * fx
    bottom = a[y1, x0] * (1 - fx) + a[y1, x1] * fx
    return top * (1 - fy) + bottom * fy


def smooth(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def mix(a, b, t):
    """Blends colors a and b (colors or images) by t (an image or a number)."""
    t = np.asarray(t, np.float32)
    if t.ndim == 2:
        t = t[..., None]
    return a + (b - a) * t


def coords(shape):
    y, x = np.mgrid[0:shape[0], 0:shape[1]].astype(np.float32)
    return x, y


def worley(shape, cells, seed, jitter=0.9, x=None, y=None, stretch=(1.0, 1.0)):
    """Tileable Voronoi cells (cells = (across, down)). Returns f1 (distance to the nearest point, px), edge (distance
    to the cell's border, px), id (the cell's index) and the nearest point (px, py). stretch scales distances along x
    and y (elongated cells). x, y may be warped pixel coordinates."""
    h, w = shape
    cx, cy = cells
    cw, ch = w / cx, h / cy
    rng = np.random.default_rng(seed)
    px = (np.arange(cx)[None, :] + 0.5 + jitter * (rng.random((cy, cx)) - 0.5)) * cw
    py = (np.arange(cy)[:, None] + 0.5 + jitter * (rng.random((cy, cx)) - 0.5)) * ch
    if x is None:
        x, y = coords(shape)
    x = np.mod(x, w)
    y = np.mod(y, h)
    ix = np.floor(x / cw).astype(np.int64)
    iy = np.floor(y / ch).astype(np.int64)
    sx, sy = stretch
    best = np.full(shape, np.inf, np.float32)
    bx = np.zeros(shape, np.float32)
    by = np.zeros(shape, np.float32)
    bid = np.zeros(shape, np.int64)
    offsets = [(dx, dy) for dy in (-1, 0, 1) for dx in (-1, 0, 1)]
    for dx, dy in offsets:
        nx, ny = ix + dx, iy + dy
        wx, wy = nx % cx, ny % cy
        qx = px[wy, wx] + (nx - wx) * cw
        qy = py[wy, wx] + (ny - wy) * ch
        d = np.hypot((x - qx) * sx, (y - qy) * sy)
        closer = d < best
        best = np.where(closer, d, best)
        bx = np.where(closer, qx, bx)
        by = np.where(closer, qy, by)
        bid = np.where(closer, wy * cx + wx, bid)
    edge = np.full(shape, np.inf, np.float32)
    for dx, dy in offsets + [(dx, dy) for dy in (-2, 2) for dx in (-1, 0, 1)] + [(dx, dy) for dx in (-2, 2) for dy in (-1, 0, 1)]:
        nx, ny = ix + dx, iy + dy
        wx, wy = nx % cx, ny % cy
        qx = px[wy, wx] + (nx - wx) * cw
        qy = py[wy, wx] + (ny - wy) * ch
        # The distance to the bisector between the nearest point and this one (in stretched space).
        mx, my = (bx + qx) * 0.5, (by + qy) * 0.5
        vx, vy = (qx - bx) * sx, (qy - by) * sy
        length = np.hypot(vx, vy)
        same = length < 1e-4
        d = ((x - mx) * sx * vx + (y - my) * sy * vy) / np.where(same, 1.0, length)
        edge = np.where(same, edge, np.minimum(edge, np.abs(d)))
    return dict(f1=best, edge=edge, id=bid, px=bx, py=by, count=cx * cy)


def per_cell(ids, count, seed, low=0.0, high=1.0):
    """A random value per cell id."""
    values = np.random.default_rng(seed).uniform(low, high, count).astype(np.float32)
    return values[ids]


def normal_map(height, px_m, strength=1.0):
    """DirectX normal map (green down, as Unreal expects) of a height field in meters, as uint8 RGB."""
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / (2.0 * px_m)
    drow = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) / (2.0 * px_m)
    nx = -dx * strength
    ny = -drow * strength  # DirectX: +green toward the bottom of the image
    nz = np.ones_like(nx)
    length = np.sqrt(nx * nx + ny * ny + nz * nz)
    n = np.stack([nx / length, ny / length, nz / length], axis=-1)
    return to8(n * 0.5 + 0.5)


def occlusion(height, px_m, radii_px=(2, 6, 16), strength=1.0):
    """Ambient occlusion from a height field: how far each point sits below its neighborhood, at a few scales."""
    occ = np.zeros_like(height)
    for r in radii_px:
        occ += np.clip((blur(height, r) - height) / (r * px_m), 0.0, 1.0)
    return np.clip(1.0 - strength * occ / len(radii_px), 0.0, 1.0)


def curvature(height, px_m, radius_px=2.0):
    """Positive on ridges and edges, negative in creases (scaled so ~1 is a sharp edge)."""
    return (height - blur(height, radius_px)) / (radius_px * px_m)


def to8(a):
    return np.clip(np.round(np.asarray(a) * 255.0), 0, 255).astype(np.uint8)


def write_png(path, data):
    """Writes uint8 data (H, W) or (H, W, 1|2|3|4) as an 8-bit PNG (no color management: the bytes as given)."""
    import struct
    import zlib
    a = np.ascontiguousarray(data)
    if a.ndim == 2:
        a = a[:, :, None]
    h, w, c = a.shape
    raw = a.reshape(h, w * c).astype(np.int16)
    left = np.zeros_like(raw)
    left[:, c:] = raw[:, :-c]
    up = np.zeros_like(raw)
    up[1:] = raw[:-1]
    corner = np.zeros_like(raw)
    corner[1:, c:] = raw[:-1, :-c]
    p = left + up - corner
    pa, pb, pc = np.abs(p - left), np.abs(p - up), np.abs(p - corner)
    predicted = np.where((pa <= pb) & (pa <= pc), left, np.where(pb <= pc, up, corner))
    rows = np.concatenate([np.full((h, 1), 4, np.uint8), ((raw - predicted) % 256).astype(np.uint8)], axis=1)

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xFFFFFFFF)
    color_type = {1: 0, 2: 4, 3: 2, 4: 6}[c]
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as file:
        file.write(b'\x89PNG\r\n\x1a\n')
        file.write(chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, color_type, 0, 0, 0)))
        file.write(chunk(b'IDAT', zlib.compress(rows.tobytes(), 6)))
        file.write(chunk(b'IEND', b''))


def fill_transparent(color, alpha, threshold=0.5):
    """Spreads the color of opaque texels into transparent ones (a push-pull pyramid), so filtering at leaf edges and
    in lower mips never pulls in black."""
    solid = (alpha > threshold).astype(np.float32)
    levels = [(color * solid[..., None], solid)]
    while min(levels[-1][1].shape) > 4:
        c, w = levels[-1]
        h2, w2 = c.shape[0] // 2, c.shape[1] // 2
        c = c[:h2 * 2, :w2 * 2].reshape(h2, 2, w2, 2, 3).sum(axis=(1, 3))
        w = w[:h2 * 2, :w2 * 2].reshape(h2, 2, w2, 2).sum(axis=(1, 3))
        levels.append((c, w))
    c, w = levels[-1]
    filled = c / np.maximum(w, 1e-6)[..., None]
    filled = np.where(w[..., None] > 0, filled, c.reshape(-1, 3).sum(0) / max(w.sum(), 1e-6))
    for c, w in reversed(levels[:-1]):
        up = np.repeat(np.repeat(filled, 2, axis=0), 2, axis=1)[:c.shape[0], :c.shape[1]]
        own = c / np.maximum(w, 1e-6)[..., None]
        filled = np.where(w[..., None] > 0, own, up)
    return np.where(solid[..., None] > 0, color, filled)


def save_set(name, color, height, px_m, rough, metal=None, normal_strength=1.0, ao=None, alpha=None, normal=None,
             look=True):
    """Writes T_<name>_BC, _N and _ORM into Art/Textures/<name>/. ao is the occlusion (default: none); normal (uint8
    RGB) replaces the one made from height. look also writes a scratch quick-look."""
    folder = os.path.join(TEXTURE_DIR, name)
    color = np.clip(color, 0.0, 1.0)
    if alpha is not None:
        color = fill_transparent(color, alpha)
        write_png(os.path.join(folder, f'T_{name}_BC.png'), np.concatenate([to8(color), to8(alpha)[..., None]], axis=-1))
    else:
        write_png(os.path.join(folder, f'T_{name}_BC.png'), to8(color))
    if normal is None:
        normal = normal_map(height, px_m, normal_strength)
    write_png(os.path.join(folder, f'T_{name}_N.png'), normal)
    occ = np.ones_like(rough) if ao is None else ao
    metal = np.zeros_like(rough) if metal is None else metal
    orm = to8(np.stack([occ, rough, metal], axis=-1))
    write_png(os.path.join(folder, f'T_{name}_ORM.png'), orm)
    if look:
        quicklook(name, color, normal, orm, alpha=alpha)
    _log(f'{name}: {color.shape[1]} x {color.shape[0]} written')


def quicklook(name, color, normal, orm, size=512, light=(-0.5, 0.6, 0.62), alpha=None):
    """A scratch view of a set (Intermediate/Textures_scratch/look_<name>.png): base color, lit with its normal map
    and occlusion, normal map, ORM; each downsized to about size."""
    bc = np.asarray(color, np.float32)[..., :3]
    n = normal.astype(np.float32) / 255.0 * 2.0 - 1.0
    orm = orm.astype(np.float32) / 255.0
    lx, ly, lz = light  # x right, y up (image), z out
    ndotl = np.clip(n[..., 0] * lx + (-n[..., 1]) * ly + n[..., 2] * lz, 0.0, 1.0)
    lit = (bc ** 2.2) * (0.3 * orm[..., :1] + 1.1 * ndotl[..., None] * (0.5 + 0.5 * orm[..., :1]))
    lit = np.clip(lit, 0.0, 1.0) ** (1.0 / 2.2)
    if alpha is not None:
        cut = (alpha > 0.5)[..., None]
        bc = np.where(cut, bc, 0.35)
        lit = np.where(cut, lit, 0.35)
    step = max(1, bc.shape[1] // size)
    row = np.concatenate([p[::step, ::step] for p in (bc, lit, n * 0.5 + 0.5, orm)], axis=1)
    write_png(os.path.join(SCRATCH_DIR, f'look_{name}.png'), to8(row))


# --- Shared by several sets: wood ---

def wood_grain(shape, seed, period=7.0, warp=1.0, along_y=False, knots=0, knot_size=7.0):
    """Wood grain along x (along y with along_y): growth-ring lines (0..1, 1 = the dark late wood) that drift and
    wander a little, long streaks and fine fibers (both about -1..1), and knots (0..1, their dark cores). knots is how
    many knots to scatter over the whole image."""
    if along_y:
        rings, streaks, fibers, knot = wood_grain(shape[::-1], seed, period, warp, False, knots, knot_size)
        return rings.T.copy(), streaks.T.copy(), fibers.T.copy(), knot.T.copy()
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(seed)
    bend = noise(shape, seed, 320.0, 14.0) * warp + noise(shape, seed + 1, 70.0, 5.0) * warp * 0.18
    knot = np.zeros(shape, np.float32)
    for _ in range(knots):
        kx, ky = rng.uniform(0, w), rng.uniform(0, h)
        dx = (x - kx + w / 2.0) % w - w / 2.0
        dy = (y - ky + h / 2.0) % h - h / 2.0
        rx, ry = knot_size * rng.uniform(0.8, 1.4), knot_size * rng.uniform(0.5, 0.8)
        d2 = (dx / rx) ** 2 + (dy / ry) ** 2
        # The rings bulge around the knot (a swirl that fades with distance).
        bend += 1.6 * np.exp(-d2 / 9.0) * np.sign(dy + 1e-3) * np.exp(-np.abs(dy) / (ry * 3.0))
        knot = np.maximum(knot, np.exp(-d2 * 1.5))
    phase = y / period + bend
    rings = 0.5 + 0.5 * np.cos(2.0 * np.pi * phase)
    rings = rings ** 5  # thin dark late-wood lines on wide light early wood
    rings *= 0.55 + 0.45 * smooth(-1.2, 0.8, noise(shape, seed + 5, 120.0, 3.0))  # lines fade in and out
    streaks = noise(shape, seed + 2, 90.0, 1.4) * 0.6 + noise(shape, seed + 3, 260.0, 4.0) * 0.4
    fibers = noise(shape, seed + 4, 12.0, 0.6)
    return rings.astype(np.float32), streaks, fibers, knot


def joint_positions(width, count_range, seed, min_gap=0.12):
    """Random joints around a loop of width px, no two closer than min_gap of the width (sorted px positions)."""
    rng = np.random.default_rng(seed)
    count = int(rng.integers(count_range[0], count_range[1] + 1))
    for _ in range(200):
        joints = np.sort(rng.uniform(0, width, count))
        gaps = np.diff(np.concatenate([joints, joints[:1] + width]))
        if count == 1 or gaps.min() >= min_gap * width:
            break
    return joints


def segments_along_x(width, count_range, seed, min_gap=0.12):
    """Random joints around a loop of width px (see joint_positions): returns (segment id per column, distance to the
    nearest joint)."""
    joints = joint_positions(width, count_range, seed, min_gap)
    count = len(joints)
    cols = np.arange(width, dtype=np.float32)
    ids = np.searchsorted(joints, cols) % count
    dist = np.min(np.abs((cols[None, :] - joints[:, None] + width / 2.0) % width - width / 2.0), axis=0)
    return ids, dist.astype(np.float32)


def stamp_discs(shape, points, radius):
    """A dome field (0..1) of discs of radius px at points (x, y), wrapping."""
    h, w = shape
    out = np.zeros(shape, np.float32)
    r = int(math.ceil(radius)) + 1
    oy, ox = np.mgrid[-r:r + 1, -r:r + 1].astype(np.float32)
    for px, py in points:
        fx, fy = px - math.floor(px), py - math.floor(py)
        d = np.hypot(ox - fx, oy - fy) / radius
        dome = np.sqrt(np.clip(1.0 - d * d, 0.0, 1.0))
        rows = (np.arange(-r, r + 1) + int(math.floor(py))) % h
        cols = (np.arange(-r, r + 1) + int(math.floor(px))) % w
        out[np.ix_(rows, cols)] = np.maximum(out[np.ix_(rows, cols)], dome)
    return out


def streaks_below(mask, length_px, seed):
    """Stains running down (toward larger rows) from mask, fading over length_px, a little uneven."""
    out = mask.copy()
    cur = mask.copy()
    decay = math.exp(-1.0 / length_px)
    for _ in range(int(length_px * 3)):
        cur = np.roll(cur, 1, axis=0) * decay
        out = np.maximum(out, cur)
    wobble = 0.6 + 0.4 * smooth(-1.0, 1.0, noise(mask.shape, seed, 1.5, 12.0))
    return out * wobble


# --- House trim sheet ---

TRIM_PX_M = 1.0 / TRIM_DENSITY


def _trim_siding(seed):
    """A: four lap-siding boards (64 px = 20 cm) along U, butt joints, nail rows, weathered grey-brown."""
    shape = (256, 2048)
    h, w = shape
    x, y = coords(shape)
    board = (y // 64).astype(np.int64)
    local = y - board * 64  # 0 at a board's top (under the lip above) .. 63 at its own lip
    rng = np.random.default_rng(seed)
    seg = np.zeros(shape, np.int64)
    joint = np.full(shape, 1e9, np.float32)
    for b in range(4):
        ids, dist = segments_along_x(w, (2, 4), seed + 10 + b)
        rows = board == b
        seg = np.where(rows, b * 8 + ids[None, :], seg)
        joint = np.where(rows, dist[None, :], joint)
    # Each board piece: its own tone and grain (the shared grain sampled at an offset).
    tone = rng.uniform(0.0, 1.0, 32).astype(np.float32)[seg]
    bright = rng.uniform(-1.0, 1.0, 32).astype(np.float32)[seg]
    shift_x = rng.integers(0, w, 32)[seg]
    shift_y = rng.integers(0, h, 32)[seg]
    rings, streaks, fibers, knot = wood_grain(shape, seed + 1, period=9.0, knots=10)
    gy = (y.astype(np.int64) + shift_y) % h
    gx = (x.astype(np.int64) + shift_x) % w
    rings, streaks, fibers, knot = rings[gy, gx], streaks[gy, gx], fibers[gy, gx], knot[gy, gx]

    # Height (m): each board a wedge, thick at its lip; gaps at the joints; raised grain; nails.
    t = local / 63.0
    height = 0.003 + 0.009 * t - 0.004 * smooth(0.93, 1.0, t)  # the lip's rounded edge
    height += 0.0011 * rings + 0.0003 * fibers + 0.0004 * streaks - 0.0012 * knot
    height += 0.0008 * noise(shape, seed + 5, 120.0, 20.0)  # cupping and warp
    gap = 1.0 - smooth(0.6, 2.2, joint)
    height -= 0.004 * gap
    crack_mask = (np.abs(noise(shape, seed + 6, 160.0, 0.9)) < 0.03) & (noise(shape, seed + 7, 200.0, 30.0) > 1.0)
    crack = blur(crack_mask.astype(np.float32), 0.6)
    height -= 0.0015 * crack
    # Nails: near the lip where the boards cross a stud (every 40 cm), not all of them.
    points = []
    for b in range(4):
        for k in range(16):
            if rng.random() < 0.85:
                points.append(((k + 0.5) * w / 16.0 + rng.normal(0, 3), b * 64 + 50 + rng.normal(0, 1.5)))
    nails = stamp_discs(shape, points, 2.6)
    height += 0.0018 * nails

    # Color: silver-grey to brown boards, dark grain, bleached patches, rust under the nails, rain streaks.
    base = mix(rgb(0x8d8272), rgb(0x7c6a55), tone)
    base = base * (1.0 + 0.07 * bright)[..., None]
    bleach = smooth(0.2, 1.4, noise(shape, seed + 8, 150.0, 40.0))
    color = mix(base, rgb(0xa89c88), bleach * 0.55)
    color = mix(color, rgb(0x54483a), np.clip(rings * 0.55 + 0.35 * np.maximum(streaks, 0.0), 0.0, 1.0))
    color = mix(color, rgb(0x3b3028), knot * 0.85)
    color = color * (1.0 + 0.06 * fibers)[..., None]
    # Dirt and shadow under each lip (the board above overhangs this one's top).
    color = mix(color, rgb(0x3e362e), (1.0 - smooth(0.0, 0.2, t)) * 0.45)
    rain = smooth(1.2, 2.4, noise(shape, seed + 9, 5.0, 90.0)) * smooth(0.0, 1.2, noise(shape, seed + 10, 260.0, 60.0))
    color = mix(color, rgb(0x5a5046), rain * 0.3)
    rust = np.clip(streaks_below(nails, 9.0, seed + 11), 0.0, 1.0)
    color = mix(color, rgb(0x7a4a2c), rust * 0.55)
    color = mix(color, rgb(0x3a3430), smooth(0.2, 0.6, nails))
    color = mix(color, rgb(0x2a241e), np.maximum(gap, crack) * 0.8)
    lip = smooth(0.86, 0.97, t) * (1.0 - smooth(0.97, 1.0, t))
    color = mix(color, rgb(0xb3a690), lip * 0.35)  # the worn, sun-caught lip

    ao = occlusion(height, TRIM_PX_M, (2, 6, 14), 1.4)
    color = color * (0.72 + 0.28 * ao)[..., None]
    rough = 0.84 + 0.06 * fibers - 0.05 * bleach
    rough = np.where(nails > 0.2, 0.55, rough)
    rough = np.maximum(rough, 0.9 * np.maximum(gap, crack))
    metal = np.where(nails > 0.2, 0.55, 0.0) * (1.0 - rust * 0.3)
    return color, height, rough, metal, ao


def crack_lines(shape, seed, spacing, length, width=0.8, amount=0.3, along_y=False):
    """Long thin cracks (checks) running along x (along y with along_y), wandering, broken into pieces: 0..1. spacing
    and length in px; amount is how much of each line survives."""
    if along_y:
        return crack_lines(shape[::-1], seed, spacing, length, width, amount).T.copy()
    n = noise(shape, seed, length, spacing)
    slope = np.abs(np.roll(n, -1, axis=0) - np.roll(n, 1, axis=0)) * 0.5 + 1e-4
    line = 1.0 - smooth(0.0, width, np.abs(n) / slope)
    keep = noise(shape, seed + 1, length * 0.8, spacing * 2.0)
    threshold = float(np.quantile(keep, 1.0 - amount))
    fade = smooth(threshold, threshold + 0.35, keep)
    taper = 0.5 + 0.5 * smooth(-1.0, 1.0, noise(shape, seed + 2, length * 0.2, spacing))
    return line * fade * taper


def row_noise(width, seed, sigma):
    """Noise along x only (one value per column), deviation 1."""
    return noise((4, width), seed, sigma, 1e4)[0]


def stones(shape, cells, seed, mortar=4.0, warp=7.0, stretch=(1.0, 1.0), jitter=0.85, rise=22.0):
    """Irregular stones (warped Voronoi cells): returns id per pixel, stone count, stone mask (0 in the mortar joints,
    mortar px wide) and a dome (0 at a stone's border, 1 in its middle over rise px)."""
    x, y = coords(shape)
    wx = x + warp * noise(shape, seed + 1, 14.0, 14.0, octaves=2)
    wy = y + warp * noise(shape, seed + 2, 14.0, 14.0, octaves=2)
    v = worley(shape, cells, seed, jitter=jitter, x=wx, y=wy, stretch=stretch)
    edge = v['edge']
    mask = smooth(mortar * 0.5, mortar * 0.5 + 1.5, edge)
    dome = np.sqrt(smooth(mortar * 0.5, mortar * 0.5 + rise, edge))
    return v['id'], v['count'], mask, dome


def palette_pick(ids, count, seed, colors, spread=0.06):
    """A color per id, picked from colors (hex) and jittered in brightness."""
    rng = np.random.default_rng(seed)
    table = np.array([rgb(colors[i]) for i in rng.integers(0, len(colors), count)], np.float32)
    table *= (1.0 + rng.uniform(-spread, spread, count)).astype(np.float32)[:, None]
    return table[ids]


def specks(shape, seed, density, size=1.0):
    """Small scattered spots (lichen, grit): 0..1, about density of the area covered."""
    n = noise(shape, seed, size, size)
    threshold = float(np.quantile(n, 1.0 - density))
    return smooth(threshold, threshold + 0.5, n)


def _trim_logs(seed):
    """B: two debarked logs (40 cm) along U with light chinking between them, rounded by the normal map."""
    shape = (256, 2048)
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(seed)
    b0 = 3.0 * row_noise(w, seed + 1, 70.0)[None, :]            # the chink line at the strip's edge (row 0 = 256)
    b1 = 128.0 + 3.0 * row_noise(w, seed + 2, 70.0)[None, :]    # the chink line between the logs
    in_first = (y >= b0) & (y < b1)
    yy = np.where(y < b0, y + 256.0, y)
    top = np.where(in_first, b0, b1)
    bottom = np.where(in_first, b1, b0 + 256.0)
    center = (top + bottom) * 0.5
    chink = 7.0 + 1.5 * noise(shape, seed + 3, 30.0, 30.0)
    radius = (bottom - top) * 0.5 - chink
    u = (yy - center) / radius                                   # -1..1 across the visible log
    log_mask = smooth(1.02, 0.97, np.abs(u))
    round_ = np.sqrt(np.clip(1.0 - u * u, 0.0, 1.0))
    rings, streaks, fibers, knot = wood_grain(shape, seed + 4, period=11.0, warp=0.8, knots=7, knot_size=9.0)
    check = crack_lines(shape, seed + 5, 22.0, 220.0, 1.0, 0.3) * smooth(0.85, 0.5, np.abs(u))
    height = log_mask * (0.004 + 0.034 * round_ ** 0.8) + 0.004 * (1.0 - log_mask)
    height += log_mask * (0.0008 * rings + 0.0003 * fibers - 0.0025 * check + 0.003 * knot)
    height += (1.0 - log_mask) * 0.0012 * noise(shape, seed + 7, 2.0, 2.0)

    first = in_first.astype(np.float32)
    tone = smooth(-1.0, 1.0, noise(shape, seed + 8, 300.0, 60.0)) * 0.6 + 0.4 * first
    wood = mix(rgb(0x9c8466), rgb(0x8b8274), tone)
    wood = mix(wood, rgb(0xb3a58c), smooth(0.4, 1.6, noise(shape, seed + 9, 120.0, 30.0)) * 0.4)
    wood = mix(wood, rgb(0x5e4a38), np.clip(rings * 0.45 + 0.3 * np.maximum(streaks, 0.0), 0.0, 1.0))
    wood = mix(wood, rgb(0x3d2f24), np.maximum(knot * 0.9, check))
    bark = smooth(1.5, 1.9, noise(shape, seed + 10, 20.0, 8.0)) * smooth(0.55, 0.85, np.abs(u))
    wood = mix(wood, rgb(0x4e3c2e), bark * 0.8)
    wood = wood * (0.78 + 0.22 * round_)[..., None]
    mortar = mix(rgb(0xb4aa96), rgb(0x9a8f7b), smooth(-1.0, 1.5, noise(shape, seed + 11, 3.0, 3.0)))
    mortar = mix(mortar, rgb(0x6f6558), specks(shape, seed + 12, 0.05) * 0.6)
    color = mix(mortar, wood, log_mask)
    ao = occlusion(height, TRIM_PX_M, (3, 10, 24), 0.9)
    color = color * (0.7 + 0.3 * ao)[..., None]
    rough = np.where(log_mask > 0.5, 0.8 + 0.05 * fibers, 0.95)
    return color, height, rough, np.zeros(shape, np.float32), ao


def _trim_beams(seed):
    """C: four hewn beam faces (20 cm) along U: adze scallops across the grain, checks, dark rounded edges."""
    shape = (256, 2048)
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(seed)
    face = (y // 64).astype(np.int64)
    local = y - face * 64
    edge = np.minimum(local, 63.0 - local)
    bevel = smooth(0.0, 7.0, edge)
    rings, streaks, fibers, knot = wood_grain(shape, seed + 1, period=8.0, warp=0.9, knots=6)
    spacing = rng.uniform(40.0, 60.0, 4)[face]
    phase = rng.uniform(0.0, 1.0, 4)[face]
    tilt = rng.uniform(-0.25, 0.25, 4)[face]
    s = x / spacing + phase + tilt * local / 64.0 + 0.25 * noise(shape, seed + 2, 40.0, 30.0)
    s = s + 0.12 * np.sin(2.0 * np.pi * s)                # scallops: steep where the adze bit in, long where it slid
    cut = np.abs(s - np.floor(s) - 0.5) * 2.0          # 0 mid-scallop, 1 at the ridge between two cuts
    depth = 0.6 + 0.4 * smooth(-1.0, 1.0, noise(shape, seed + 3, 30.0, 30.0))
    check = crack_lines(shape, seed + 4, 18.0, 200.0, 0.9, 0.25) * smooth(6.0, 12.0, edge)
    height = 0.006 * bevel ** 0.7 + 0.0028 * cut ** 2 * bevel * depth + 0.0006 * rings + 0.0003 * fibers
    height -= 0.003 * check + 0.0008 * knot

    tone = rng.uniform(0.0, 1.0, 4).astype(np.float32)[face]
    color = mix(rgb(0x75604b), rgb(0x60503f), tone)
    color = mix(color, rgb(0x8c7a66), smooth(0.5, 1.8, noise(shape, seed + 5, 150.0, 40.0)) * 0.45)
    color = mix(color, rgb(0x3f3328), np.clip(rings * 0.5 + 0.3 * np.maximum(streaks, 0.0), 0.0, 1.0))
    color = color * (0.9 + 0.16 * cut ** 2 * depth)[..., None]            # the ridges between cuts catch the light
    color = mix(color, rgb(0x2b231c), np.maximum(check, knot * 0.8))
    color = mix(color, rgb(0x2d251e), (1.0 - smooth(1.0, 8.0, edge)) * 0.75)  # the dark edges
    ao = occlusion(height, TRIM_PX_M, (2, 6, 14), 1.2)
    color = color * (0.75 + 0.25 * ao)[..., None]
    rough = 0.86 + 0.04 * fibers - 0.05 * cut
    return color, height, rough, np.zeros(shape, np.float32), ao


FIELDSTONE = [0x8a857c, 0x9b8f7c, 0x7c7b77, 0xa39a88, 0x857563, 0x716f6a, 0x958878, 0xa8a293]


def _trim_fieldstone(seed):
    """D: fieldstone masonry: rounded stones of mixed greys and tans in light mortar, lichen specks."""
    shape = (256, 2048)
    ids, count, mask, dome = stones(shape, (26, 4), seed, mortar=5.0, stretch=(0.85, 1.1))
    height = 0.004 + mask * (0.014 * dome) + 0.0012 * noise(shape, seed + 3, 3.0, 3.0, octaves=2) * mask
    height += 0.004 * per_cell(ids, count, seed + 4, -1.0, 1.0) * mask * dome
    height += 0.0008 * noise(shape, seed + 5, 1.2, 1.2) * (1.0 - mask)
    stone = palette_pick(ids, count, seed + 6, FIELDSTONE)
    stone = stone * (1.0 + 0.08 * noise(shape, seed + 7, 6.0, 6.0, octaves=2))[..., None]
    stone = mix(stone, rgb(0xc2b9a6), smooth(0.6, 1.0, dome) * 0.18)            # sun-bleached crowns
    stone = mix(stone, rgb(0xa9a77a), specks(shape, seed + 8, 0.04, 1.6) * 0.7)  # pale lichen
    stone = mix(stone, rgb(0xb9854a), specks(shape, seed + 9, 0.012, 1.3) * 0.6)  # orange lichen
    mortar = mix(rgb(0xa89e8b), rgb(0x8c8272), smooth(-1.0, 1.5, noise(shape, seed + 10, 2.5, 2.5)))
    color = mix(mortar, stone, mask)
    ao = occlusion(height, TRIM_PX_M, (3, 8, 20), 1.3)
    color = color * (0.66 + 0.34 * ao)[..., None]
    rough = np.where(mask > 0.5, 0.82, 0.95) + 0.04 * noise(shape, seed + 11, 4.0, 4.0)
    return color, height, rough, np.zeros(shape, np.float32), ao


def _trim_shingles(seed):
    """E: wooden shakes in four rows (20 cm showing each), the grain running down the roof (V); uneven butts, gaps
    between shakes, moss near the butts."""
    shape = (256, 2048)
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(seed)
    row = (y // 64).astype(np.int64)
    ids = np.zeros(shape, np.int64)
    gap = np.zeros(shape, np.float32)
    butt = np.zeros(shape, np.float32)
    cols = np.arange(w, dtype=np.float32)
    table_offset = []
    base_id = 0
    for r in range(4):
        widths = []
        while sum(widths) < w:
            widths.append(rng.uniform(28.0, 62.0))
        edges = np.cumsum([0.0] + widths)
        edges = edges / edges[-1] * w + rng.uniform(0, w)
        joints = np.sort(edges[:-1] % w)
        shake = np.searchsorted(joints, cols) % len(joints)
        dist = np.min(np.abs((cols[None, :] - joints[:, None] + w / 2.0) % w - w / 2.0), axis=0)
        rows_r = row == r
        ids = np.where(rows_r, base_id + shake[None, :], ids)
        gap = np.where(rows_r, (1.0 - smooth(0.7, 2.0, dist))[None, :], gap)
        offsets = rng.uniform(0.0, 7.0, len(joints))
        butt = np.where(rows_r, offsets[shake][None, :], butt)
        base_id += len(joints)
    count = int(base_id)
    local = y - row * 64.0
    covered = local > 63.0 - butt           # below a short shake's butt: the row beneath shows, in its shadow
    t = np.where(covered, 0.0, local / 63.0)
    rings, streaks, fibers, knot = wood_grain(shape, seed + 1, period=6.0, warp=1.2, along_y=True)
    # Each shake shows its own piece of the grain.
    shift_x = rng.integers(0, w, count)[ids]
    shift_y = rng.integers(0, h, count)[ids]
    gy = (y.astype(np.int64) + shift_y) % h
    gx = (x.astype(np.int64) + shift_x) % w
    rings, streaks, fibers = rings[gy, gx], streaks[gy, gx], fibers[gy, gx]
    rings = rings * (0.5 + 0.5 * per_cell(ids, count, seed + 12))
    split = crack_lines(shape, seed + 2, 16.0, 90.0, 0.9, 0.2, along_y=True)
    height = np.where(covered, 0.002, 0.003 + 0.013 * t - 0.004 * smooth(0.94, 1.0, t))
    height += (0.0007 * rings + 0.0003 * fibers - 0.002 * split) * (1.0 - covered)
    height -= 0.004 * gap
    height += 0.0015 * per_cell(ids, count, seed + 4, -1.0, 1.0) * t

    tone = per_cell(ids, count, seed + 5)
    color = mix(rgb(0x7f705f), rgb(0x9a907f), tone)
    color = mix(color, rgb(0x7d6450), (per_cell(ids, count, seed + 6) > 0.92).astype(np.float32) * 0.45)  # newer shakes
    color = color * (1.0 + 0.08 * per_cell(ids, count, seed + 7, -1.0, 1.0))[..., None]
    color = mix(color, rgb(0x4b4036), np.clip(rings * 0.45 + 0.3 * np.maximum(streaks, 0.0), 0.0, 1.0))
    moss = smooth(0.5, 1.6, noise(shape, seed + 8, 6.0, 4.0, octaves=3) + 1.6 * t - 1.1)
    moss *= smooth(0.2, 1.4, noise(shape, seed + 9, 150.0, 60.0))
    color = mix(color, mix(rgb(0x5d6636), rgb(0x7b8244), smooth(-1, 1, noise(shape, seed + 10, 1.5, 1.5))), moss * 0.8)
    color = mix(color, rgb(0xa9a77a), specks(shape, seed + 11, 0.02, 1.4) * 0.6)
    color = mix(color, rgb(0x2c251f), np.maximum(gap, split) * 0.85)
    color = mix(color, rgb(0x2f2923), covered.astype(np.float32) * 0.7)
    ao = occlusion(height, TRIM_PX_M, (2, 6, 16), 1.4)
    color = color * (0.7 + 0.3 * ao)[..., None]
    rough = 0.88 + 0.05 * fibers - 0.06 * moss
    return color, height, rough, np.zeros(shape, np.float32), ao


def _trim_tin(seed):
    """F: corrugated galvanized tin, ridges along V (down the roof), overlapping sheets, screws, rust streaks."""
    shape = (256, 2048)
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(seed)
    waves = 88                                     # corrugations per 6.4 m (7.3 cm pitch)
    phase = x * waves / w
    ridge = np.sin(2.0 * np.pi * phase)
    sheet = (np.floor(phase / 11.0)).astype(np.int64) % 8   # 8 sheets 11 corrugations wide (0.8 m)
    seam = np.abs(((phase + 0.25) % 11.0) - 0.25) < 0.12   # the overlap sits on a crest
    height = 0.008 * ridge + 0.0015 * (sheet % 2)  # every other sheet laps over its neighbors
    points = []
    for row_y in (40.0, 168.0):
        for k in range(waves):
            if k % 11 in (0, 4, 8):
                points.append(((k + 0.25) * w / waves, row_y + rng.normal(0.0, 1.0)))
    screws = stamp_discs(shape, points, 3.2)
    height += 0.003 * screws
    dent = noise(shape, seed + 1, 30.0, 30.0) * 0.0015
    height += dent

    sheet_rust = rng.uniform(0.0, 1.0, 8).astype(np.float32)[sheet]
    seam_dist = np.abs(((phase + 0.25 + 5.5) % 11.0) - 5.5)       # corrugations from the nearest seam
    patches = smooth(0.7, 1.7, noise(shape, seed + 2, 22.0, 90.0, octaves=3) + sheet_rust * 0.9
                     + 0.9 * smooth(1.5, 0.0, seam_dist) * sheet_rust ** 2 - 0.3)
    valley = smooth(0.2, -0.8, ridge)
    runs = smooth(0.7, 2.0, noise(shape, seed + 3, 2.0, 80.0)) * valley * (0.4 + 0.6 * sheet_rust)
    from_screws = np.clip(streaks_below(screws, 40.0, seed + 4), 0.0, 1.0)
    rust = np.clip(patches * 0.9 + runs * 0.8 + from_screws * 0.9, 0.0, 1.0)
    rust = rust * (0.75 + 0.25 * smooth(-1.0, 1.0, noise(shape, seed + 5, 1.5, 4.0)))
    galv = mix(rgb(0x9aa1a4), rgb(0x7d8487), smooth(-1.0, 1.0, noise(shape, seed + 6, 80.0, 80.0)) * 0.7 + 0.3 * sheet_rust)
    spangle = worley(shape, (340, 42), seed + 7, jitter=1.0)
    galv = galv * (1.0 + 0.05 * per_cell(spangle['id'], spangle['count'], seed + 8, -1.0, 1.0))[..., None]
    rust_color = mix(rgb(0x8f4c26), rgb(0x5f3520), smooth(-1.0, 1.0, noise(shape, seed + 9, 4.0, 14.0)))
    rust_color = mix(rust_color, rgb(0xa75f2e), smooth(0.8, 1.8, noise(shape, seed + 10, 2.0, 6.0)) * 0.4)
    color = mix(galv, rust_color, rust)
    color = mix(color, rgb(0x4b4a46), valley * 0.25 * (1.0 - rust))    # grime in the valleys
    color = mix(color, rgb(0x2e2d2b), seam.astype(np.float32) * 0.5)
    color = mix(color, rgb(0x5d5f5f), smooth(0.2, 0.7, screws))
    ao = occlusion(height, TRIM_PX_M, (2, 6, 12), 0.8)
    color = color * (0.8 + 0.2 * ao)[..., None]
    metal = np.clip(1.0 - rust * 1.3, 0.0, 1.0)
    rough = 0.42 + 0.15 * smooth(-1.0, 1.0, noise(shape, seed + 11, 40.0, 40.0)) + 0.45 * rust
    return color, height, rough, metal, ao


def _trim_plaster(seed):
    """G: lime plaster: cream, mottled, water stains, grime running down, fine cracks, a few patches fallen off to
    the stone beneath."""
    shape = (256, 2048)
    x, y = coords(shape)
    base = mix(rgb(0xd8cfbb), rgb(0xc8bca3), smooth(-1.2, 1.2, noise(shape, seed + 1, 160.0, 120.0)))
    base = base * (1.0 + 0.035 * noise(shape, seed + 2, 12.0, 12.0, octaves=3))[..., None]
    height = 0.0015 * noise(shape, seed + 3, 50.0, 40.0, octaves=3) + 0.0003 * noise(shape, seed + 4, 1.5, 1.5)
    stain_n = noise(shape, seed + 5, 70.0, 90.0, octaves=3)
    stain = smooth(0.7, 1.1, stain_n)
    rim = smooth(0.65, 0.75, stain_n) * (1.0 - smooth(0.8, 0.95, stain_n))
    color = mix(base, rgb(0xb8a888), stain * 0.45)
    color = mix(color, rgb(0x9c8c70), rim * 0.35)
    grime = smooth(0.9, 2.2, noise(shape, seed + 6, 5.0, 110.0)) * smooth(-0.5, 1.0, noise(shape, seed + 7, 200.0, 80.0))
    color = mix(color, rgb(0x8f8574), grime * 0.35)
    # Cracks: cell borders, jagged, of varying width, only in a few places, with dirt settled along them.
    wx = x + 14.0 * noise(shape, seed + 8, 16.0, 16.0) + 2.5 * noise(shape, seed + 16, 2.0, 2.0)
    wy = y + 14.0 * noise(shape, seed + 9, 16.0, 16.0) + 2.5 * noise(shape, seed + 17, 2.0, 2.0)
    v = worley(shape, (26, 4), seed + 10, x=wx, y=wy)
    width = 0.35 + 0.6 * smooth(-1.0, 1.5, noise(shape, seed + 18, 25.0, 25.0))
    region = smooth(1.0, 1.6, noise(shape, seed + 11, 70.0, 50.0))
    crack = (1.0 - smooth(0.0, width, v['edge'])) * region
    halo = (1.0 - smooth(0.0, 5.0, v['edge'])) * region
    color = mix(color, rgb(0xa39883), halo * 0.3)
    color = mix(color, rgb(0x5f584d), crack * 0.85)
    height -= 0.0008 * crack
    # Fallen patches: stone and mortar show through, a raised broken rim around them.
    hole_n = noise(shape, seed + 12, 36.0, 26.0, octaves=2) + 0.25 * noise(shape, seed + 19, 3.0, 3.0)
    hole_n = hole_n - 0.5 + 0.6 * smooth(0.5, 1.5, noise(shape, seed + 20, 300.0, 200.0))
    hole = smooth(1.9, 2.0, hole_n)
    edge_band = smooth(1.7, 1.9, hole_n) * (1.0 - hole)
    ids, count, mask, dome = stones(shape, (40, 5), seed + 13, mortar=4.0, stretch=(0.8, 1.2), rise=10.0)
    inside = mix(rgb(0x9c9384), palette_pick(ids, count, seed + 14, FIELDSTONE), mask)
    color = mix(color, inside * 0.85, hole)
    color = mix(color, rgb(0xe3dccb), edge_band * 0.4)
    height = height * (1.0 - hole) + hole * (-0.008 + 0.005 * dome * mask) + 0.0006 * edge_band
    ao = occlusion(height, TRIM_PX_M, (2, 6, 14), 1.0)
    color = color * (0.75 + 0.25 * ao)[..., None]
    rough = 0.9 + 0.04 * noise(shape, seed + 15, 8.0, 8.0) - 0.05 * stain
    return color, height, rough, np.zeros(shape, np.float32), ao


def _painted_board(seed, paint, faded):
    """H1/H2: a painted trim board (20 cm) along U: faded paint, chipped at the edges and along the grain to weathered
    wood, butt joints."""
    shape = (64, 2048)
    h, w = shape
    x, y = coords(shape)
    ids, joint = segments_along_x(w, (2, 3), seed)
    joint = np.broadcast_to(joint[None, :], shape)
    edge = np.minimum(y, 63.0 - y)
    bevel = smooth(0.0, 5.0, edge)
    rings, streaks, fibers, knot = wood_grain(shape, seed + 1, period=7.0, warp=0.8, knots=2)
    wear = 1.0 - smooth(0.0, 9.0, np.minimum(edge, joint))
    chips = noise(shape, seed + 2, 10.0, 5.0, octaves=3) + 1.6 * wear + 0.7 * rings
    bare = smooth(1.1, 1.25, chips)
    gap = 1.0 - smooth(0.6, 2.0, joint)
    height = 0.005 * bevel ** 0.6 + 0.0006 * rings + 0.0002 * fibers + 0.0003 * (1.0 - bare) - 0.004 * gap
    wood = mix(rgb(0x8c806e), rgb(0x6d6152), np.clip(rings * 0.6 + 0.3 * np.maximum(streaks, 0.0), 0.0, 1.0))
    coat = mix(rgb(paint), rgb(faded), smooth(-1.0, 1.2, noise(shape, seed + 3, 90.0, 20.0)) * 0.8)
    coat = coat * (1.0 + 0.05 * noise(shape, seed + 4, 20.0, 2.0) - 0.08 * rings)[..., None]
    color = mix(coat, wood, bare)
    dirt = smooth(0.4, 1.6, noise(shape, seed + 5, 40.0, 12.0)) * 0.3 + (1.0 - bevel) * 0.35
    color = mix(color, rgb(0x3f3a33), dirt)
    color = mix(color, rgb(0x2a241f), gap * 0.85)
    ao = occlusion(height, TRIM_PX_M, (2, 5), 1.0)
    color = color * (0.78 + 0.22 * ao)[..., None]
    rough = 0.6 + 0.26 * bare + 0.04 * fibers
    return color, height, rough, np.zeros(shape, np.float32), ao


def _trim_teal(seed):
    return _painted_board(seed, 0x4b7d79, 0x7aa29a)


def _trim_red(seed):
    return _painted_board(seed, 0x8a3a2b, 0xa66a58)


def _trim_iron(seed):
    """H3: an iron strap (the whole 20 cm is iron) with two rows of big rivets (5 cm from its edges), hammer marks
    and rust."""
    shape = (64, 2048)
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(seed)
    points = [((k + 0.5) * w / 48.0 + rng.normal(0, 1.0), row + rng.normal(0, 0.6)) for row in (16.0, 48.0) for k in range(48)]
    rivets = stamp_discs(shape, points, 5.0)
    edge = np.minimum(y, 63.0 - y)
    bevel = smooth(0.0, 3.0, edge)
    height = 0.002 * bevel + 0.004 * rivets + 0.0004 * noise(shape, seed + 1, 5.0, 5.0, octaves=2)
    ring = np.clip(stamp_discs(shape, points, 7.5) - rivets, 0.0, 1.0)   # just around each rivet
    rust = smooth(1.1, 1.8, noise(shape, seed + 2, 10.0, 6.0, octaves=3) + 1.2 * (1.0 - bevel) + 0.9 * ring
                  + 0.8 * np.clip(streaks_below(rivets, 8.0, seed + 3), 0.0, 1.0))
    iron = mix(rgb(0x45433f), rgb(0x2f2e2c), smooth(-1.0, 1.0, noise(shape, seed + 4, 20.0, 12.0, octaves=2)))
    # Rivet heads: worn bright on top, a dark seam around them.
    iron = mix(iron, rgb(0x6d6861), smooth(0.4, 1.0, rivets) * 0.7)
    iron = mix(iron, rgb(0x1e1d1c), smooth(0.0, 0.25, rivets) * (1.0 - smooth(0.25, 0.5, rivets)) * 0.8)
    rust_color = mix(rgb(0x6f3b1f), rgb(0x8f5028), smooth(-1.0, 1.0, noise(shape, seed + 6, 3.0, 3.0)))
    color = mix(iron, rust_color, rust * 0.9)
    ao = occlusion(height, TRIM_PX_M, (2, 5), 1.0)
    color = color * (0.75 + 0.25 * ao)[..., None]
    metal = np.clip(0.9 - rust, 0.0, 1.0)
    rough = 0.52 + 0.1 * noise(shape, seed + 7, 6.0, 6.0) + 0.35 * rust
    return color, height, rough, metal, ao


def _trim_glass(seed):
    """H4: dark window glass: the sky's reflection lighter toward the top, soft diagonal glints about every 70 cm,
    dust and grime toward the bottom, a putty line at both edges. Map panes with fit=True."""
    shape = (64, 2048)
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(seed)
    up = 1.0 - y / 63.0
    color = mix(rgb(0x1c252b), rgb(0x3b4a55), up ** 1.5 * 0.8)
    bands = 9
    phase = (x + 0.9 * y) * bands / w
    strength = rng.uniform(0.4, 1.0, bands).astype(np.float32)[np.floor(phase).astype(np.int64) % bands]
    glint = np.exp(-((phase % 1.0 - 0.5) / 0.09) ** 2) * strength
    glint += 0.5 * np.exp(-((phase % 1.0 - 0.68) / 0.03) ** 2) * strength
    color = mix(color, rgb(0x7f929e), np.clip(glint, 0.0, 1.0) * 0.45)
    smudge = smooth(0.2, 1.5, noise(shape, seed + 1, 8.0, 4.0, octaves=2))
    dirt = smooth(0.7, 1.05, y / 63.0 + 0.2 * noise(shape, seed + 2, 12.0, 4.0)) * 0.8
    color = mix(color, rgb(0x5a5446), np.clip(dirt + smudge * 0.12, 0.0, 1.0) * 0.5)
    putty = 1.0 - smooth(1.5, 3.0, np.minimum(y, 63.0 - y))
    color = mix(color, rgb(0x4a453d), putty)
    height = 0.0015 * putty
    rough = np.clip(0.05 + 0.2 * smudge + 0.6 * dirt, 0.0, 1.0)
    rough = np.maximum(rough, 0.9 * putty)
    return color, height, rough, np.zeros(shape, np.float32), np.ones(shape, np.float32)


def _trim_blank(seed, rows=256):
    shape = (rows, 2048)
    return (np.full(shape + (3,), 0.5, np.float32), np.zeros(shape, np.float32), np.full(shape, 0.8, np.float32),
            np.zeros(shape, np.float32), np.ones(shape, np.float32))


TRIM_MAKERS = {'A': _trim_siding, 'B': _trim_logs, 'C': _trim_beams, 'D': _trim_fieldstone, 'E': _trim_shingles,
               'F': _trim_tin, 'G': _trim_plaster, 'H1': _trim_teal, 'H2': _trim_red, 'H3': _trim_iron,
               'H4': _trim_glass}


@generator('HouseTrim')
def make_house_trim():
    size = TRIM_SIZE
    color = np.zeros((size, size, 3), np.float32)
    normal = np.zeros((size, size, 3), np.uint8)
    rough = np.zeros((size, size), np.float32)
    metal = np.zeros((size, size), np.float32)
    ao = np.zeros((size, size), np.float32)
    for index, key in enumerate(TRIM_STRIPS):
        v0, v1 = TRIM_STRIPS[key]
        top = int(round((1.0 - v1) * size))
        bottom = int(round((1.0 - v0) * size))
        maker = TRIM_MAKERS.get(key)
        c, hgt, r, m, a = maker(1000 + index * 97) if maker else _trim_blank(0, bottom - top)
        color[top:bottom] = c
        normal[top:bottom] = normal_map(hgt, TRIM_PX_M)
        rough[top:bottom] = r
        metal[top:bottom] = m
        ao[top:bottom] = a
    save_set('HouseTrim', color, None, TRIM_PX_M, np.clip(rough, 0, 1), np.clip(metal, 0, 1), ao=ao, normal=normal)


# --- Painting many small things (blades, straw, pebbles): points resolved by depth ---

class Canvas:
    """Paints points with a depth: each pixel keeps the channels of the deepest-reaching (highest) point on it.
    Channels are named float images; the top depth is the height where something was painted (else `floor`)."""

    def __init__(self, shape, floor=0.0, **channels):
        self.shape = shape
        self.top = np.full(shape[0] * shape[1], -np.inf, np.float32)
        self.floor = floor
        self.channels = {k: np.array(v, np.float32).reshape(shape[0] * shape[1], -1).copy() for k, v in channels.items()}

    def paint(self, xs, ys, depth, **values):
        h, w = self.shape
        flat = (np.floor(ys).astype(np.int64) % h) * w + (np.floor(xs).astype(np.int64) % w)
        order = np.argsort(depth, kind='stable')[::-1]           # highest first: np.unique keeps the first
        flat, depth = flat[order], depth[order]
        cells, first = np.unique(flat, return_index=True)
        better = depth[first] > self.top[cells]
        cells, first = cells[better], first[better]
        self.top[cells] = depth[first]
        for name, value in values.items():
            value = np.asarray(value, np.float32)
            if value.ndim == 1:
                value = value[:, None]
            self.channels[name][cells] = value[order][first]

    def get(self, name):
        c = self.channels[name]
        return c.reshape(self.shape + ((c.shape[1],) if c.shape[1] > 1 else ()))

    def height(self):
        top = self.top.reshape(self.shape)
        return np.where(np.isfinite(top), top, self.floor).astype(np.float32)

    def covered(self):
        return np.isfinite(self.top).reshape(self.shape).astype(np.float32)


def stroke_points(x0, y0, angle, length, width, bend=None, taper=0.6, step=0.7):
    """Points filling n strokes (blades, strands, needles) from (x0, y0) along angle, px. Returns x, y, t (0 at the
    base, 1 at the tip), the stroke index and across (-1..1, the side of the stroke)."""
    n = len(x0)
    bend = np.zeros(n, np.float32) if bend is None else bend
    steps = int(math.ceil(float(length.max()) / step)) + 1
    lanes = max(2, int(math.ceil(float(width.max()) / 0.6)) + 1)
    k = np.arange(steps, dtype=np.float32)[None, :, None]
    t = k * step / length[:, None, None]
    across = np.linspace(-1.0, 1.0, lanes, dtype=np.float32)[None, None, :]
    idx = np.broadcast_to(np.arange(n)[:, None, None], (n, steps, lanes))
    half = (width[:, None, None] * 0.5) * (1.0 - taper * np.clip(t, 0.0, 1.0))
    dx, dy = np.cos(angle)[:, None, None], np.sin(angle)[:, None, None]
    along = t * length[:, None, None]
    side = bend[:, None, None] * t * t * length[:, None, None] + across * half
    x = x0[:, None, None] + dx * along - dy * side
    y = y0[:, None, None] + dy * along + dx * side
    keep = np.broadcast_to(t <= 1.0, (n, steps, lanes))
    t = np.broadcast_to(t, (n, steps, lanes))
    across = np.broadcast_to(across, (n, steps, lanes))
    return x[keep], y[keep], t[keep], idx[keep], across[keep]


def disc_points(cx, cy, radius, step=0.7):
    """Points filling n discs (pebbles, leaflets): x, y, the disc index and r (0 center .. 1 rim)."""
    rmax = float(radius.max())
    g = np.arange(-rmax, rmax + step, step, dtype=np.float32)
    ox, oy = np.meshgrid(g, g)
    ox, oy = ox.ravel()[None, :], oy.ravel()[None, :]
    r = np.hypot(ox, oy) / radius[:, None]
    keep = r <= 1.0
    idx = np.broadcast_to(np.arange(len(cx))[:, None], r.shape)
    return (np.broadcast_to(cx[:, None] + ox, r.shape)[keep], np.broadcast_to(cy[:, None] + oy, r.shape)[keep],
            idx[keep], r[keep])


def pebbles(canvas, rng, count, rmin, rmax, colors, px_m, lift=0.0, flatten=0.55):
    """Scatters pebbles (flattened domes) onto canvas channels color and kind (1 = pebble)."""
    h, w = canvas.shape
    radius = (rmin + (rmax - rmin) * rng.random(count) ** 2.5).astype(np.float32)
    cx, cy = rng.uniform(0, w, count).astype(np.float32), rng.uniform(0, h, count).astype(np.float32)
    stretch = rng.uniform(0.65, 1.0, count).astype(np.float32)
    x, y, i, r = disc_points(cx, cy, radius)
    y = cy[i] + (y - cy[i]) * stretch[i]
    table = np.array([rgb(colors[k]) for k in rng.integers(0, len(colors), count)], np.float32)
    table *= rng.uniform(0.85, 1.15, count).astype(np.float32)[:, None]
    dome = np.sqrt(np.clip(1.0 - r * r, 0.0, 1.0))
    depth = lift + radius[i] * px_m * flatten * dome
    shade = (0.8 + 0.2 * dome)[:, None]
    canvas.paint(x, y, depth, color=table[i] * shade, kind=np.ones_like(r))


# --- Tileable sets ---

TILE = 1024


def _ground_soil(shape, seed, px_m):
    """Soil: brown, crumbly, a few small stones. Returns color, height."""
    soil_n = noise(shape, seed, 3.0, 3.0, octaves=3)
    color = mix(rgb(0x5a4633), rgb(0x76603f), smooth(-1.5, 1.5, noise(shape, seed + 1, 40.0, 40.0, octaves=2)))
    color = color * (1.0 + 0.12 * soil_n)[..., None]
    height = 0.002 * soil_n + 0.003 * noise(shape, seed + 2, 30.0, 30.0)
    return color, height


@generator('GroundGrass')
def make_ground_grass():
    """Meadow floor seen close: short grass stubble over soil, clover patches, a little straw. 256 px/m (4 m)."""
    shape = (TILE, TILE)
    px_m = 1.0 / 256.0
    rng = np.random.default_rng(71)
    soil, soil_h = _ground_soil(shape, 72, px_m)
    canvas = Canvas(shape, 0.0, color=soil, kind=np.zeros(shape))
    canvas.top[:] = soil_h.ravel()
    # Clover: trefoils in patches.
    patch = smooth(0.6, 1.3, noise(shape, 73, 60.0, 60.0, octaves=2))
    count = 9000
    cx = rng.uniform(0, TILE, count).astype(np.float32)
    cy = rng.uniform(0, TILE, count).astype(np.float32)
    keep = rng.random(count) < patch[cy.astype(int), cx.astype(int)]
    cx, cy = cx[keep], cy[keep]
    n = len(cx)
    turn = rng.uniform(0, 2 * np.pi, n).astype(np.float32)
    size = rng.uniform(2.6, 4.2, n).astype(np.float32)
    leaf = np.array([rgb(c) for c in (0x3f6a2c, 0x4a7632, 0x557d36, 0x3a5f2a)], np.float32)[rng.integers(0, 4, n)]
    for k in range(3):
        a = turn + k * 2.0 * np.pi / 3.0
        lx = cx + np.cos(a) * size * 0.95
        ly = cy + np.sin(a) * size * 0.95
        x, y, i, r = disc_points(lx, ly, size)
        dome = np.sqrt(np.clip(1.0 - r * r, 0.0, 1.0))
        chevron = smooth(0.5, 0.35, np.abs(r - 0.5)) * 0.18
        canvas.paint(x, y, 0.006 + 0.002 * dome + 0.004 * (i % 3) / 3.0,
                     color=leaf[i] * (0.85 + 0.15 * dome)[:, None] + chevron[:, None], kind=np.full(len(i), 2.0))
    # Grass stubble: short blades leaning every way, a few dry ones.
    for batch in range(6):
        count = 11000
        x0 = rng.uniform(0, TILE, count).astype(np.float32)
        y0 = rng.uniform(0, TILE, count).astype(np.float32)
        angle = rng.uniform(0, 2 * np.pi, count).astype(np.float32)
        length = rng.uniform(7.0, 20.0, count).astype(np.float32)
        width = rng.uniform(1.4, 2.4, count).astype(np.float32)
        bend = rng.normal(0.0, 0.15, count).astype(np.float32)
        x, y, t, i, across = stroke_points(x0, y0, angle, length, width, bend)
        greens = np.array([rgb(c) for c in (0x55702c, 0x648034, 0x4b6528, 0x71863a, 0x5c7a2e, 0x7d8a44)], np.float32)
        tone = greens[rng.integers(0, len(greens), count)]
        dry = rng.random(count) < 0.07
        tone[dry] = np.array([rgb(c) for c in (0xa39455, 0x8e8450, 0xb3a266)], np.float32)[rng.integers(0, 3, int(dry.sum()))]
        tone *= rng.uniform(0.85, 1.12, count).astype(np.float32)[:, None]
        lift = rng.uniform(0.0, 0.012, count).astype(np.float32)
        depth = lift[i] + 0.02 * t + 0.0008 * (1.0 - across ** 2)
        shade = (0.62 + 0.45 * t) * (0.92 + 0.08 * (1.0 - across ** 2))
        canvas.paint(x, y, depth, color=tone[i] * shade[:, None], kind=np.ones_like(t))
    height = canvas.height()
    color = canvas.get('color')
    ao = occlusion(height, px_m, (2, 5, 12), 1.1)
    color = color * (0.6 + 0.4 * ao)[..., None]
    kind = canvas.get('kind')
    rough = np.where(kind > 0.5, 0.78, 0.95) + 0.03 * noise(shape, 74, 2.0, 2.0)
    save_set('GroundGrass', color, height, px_m, rough, ao=ao, normal_strength=0.8)


@generator('GroundDirt')
def make_ground_dirt():
    """A packed dirt road: tan compacted soil, pebbles, faint ruts and scuffs along U, dried cracks. 256 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 256.0
    rng = np.random.default_rng(81)
    ruts = noise(shape, 82, 200.0, 9.0, octaves=2) * smooth(-0.8, 1.0, noise(shape, 92, 150.0, 150.0))
    base = mix(rgb(0x8f7757), rgb(0x7a6347), smooth(-1.5, 1.5, noise(shape, 83, 70.0, 60.0, octaves=2)))
    base = mix(base, rgb(0xa08a68), smooth(0.3, 1.8, noise(shape, 84, 90.0, 60.0)) * 0.45)
    grain = noise(shape, 85, 0.8, 0.8)
    clods = noise(shape, 93, 4.0, 4.0, octaves=2)
    base = base * (1.0 + 0.07 * grain + 0.02 * ruts + 0.05 * clods)[..., None]
    height = 0.0015 * ruts + 0.0015 * noise(shape, 86, 25.0, 25.0, octaves=3) + 0.0004 * grain + 0.0008 * clods
    v = worley(shape, (14, 14), 87, x=coords(shape)[0] + 8.0 * noise(shape, 88, 10.0, 10.0),
               y=coords(shape)[1] + 8.0 * noise(shape, 89, 10.0, 10.0))
    crack = (1.0 - smooth(0.3, 1.4, v['edge'])) * smooth(0.8, 1.5, noise(shape, 90, 90.0, 90.0))
    height -= 0.0015 * crack
    base = mix(base, rgb(0x5c4a36), crack * 0.6)
    damp = smooth(0.8, 1.8, noise(shape, 91, 80.0, 80.0))
    base = mix(base, rgb(0x6b5840), damp * 0.35)
    canvas = Canvas(shape, 0.0, color=base, kind=np.zeros(shape))
    canvas.top[:] = height.ravel()
    stones_ = [0x8c877e, 0x9d9486, 0x7a766e, 0xa89c88, 0x6f6a62, 0xb3a894]
    pebbles(canvas, rng, 2600, 1.2, 3.0, stones_, px_m, lift=0.0005)
    pebbles(canvas, rng, 420, 3.0, 7.0, stones_, px_m, lift=0.0)
    # A few straws and grass bits blown onto the road.
    count = 500
    x, y, t, i, across = stroke_points(rng.uniform(0, TILE, count).astype(np.float32), rng.uniform(0, TILE, count).astype(np.float32),
                                       rng.uniform(0, 2 * np.pi, count).astype(np.float32),
                                       rng.uniform(8, 22, count).astype(np.float32), np.full(count, 1.5, np.float32))
    canvas.paint(x, y, 0.004 + 0.001 * t, color=np.broadcast_to(rgb(0xb09c62), (len(t), 3)) * (0.8 + 0.2 * t)[:, None],
                 kind=np.full(len(t), 2.0))
    height = canvas.height()
    color = canvas.get('color')
    ao = occlusion(height, px_m, (2, 5, 12), 1.0)
    color = color * (0.72 + 0.28 * ao)[..., None]
    kind = canvas.get('kind')
    rough = np.where(kind == 1.0, 0.8, 0.95) - 0.05 * damp
    save_set('GroundDirt', color, height, px_m, rough, ao=ao, normal_strength=0.9)


def _layers(size, seed, thickness):
    """Horizontal layers around a loop of size px: per row, the layer index and the position within it (0 top ..
    1 bottom); thickness = (min, max) px."""
    rng = np.random.default_rng(seed)
    cuts = [0.0]
    while cuts[-1] < size:
        cuts.append(cuts[-1] + rng.uniform(*thickness))
    cuts = np.array(cuts) * size / cuts[-1]
    rows = np.arange(size, dtype=np.float32)
    index = np.clip(np.searchsorted(cuts, rows, side='right') - 1, 0, len(cuts) - 2)
    pos = (rows - cuts[index]) / (cuts[index + 1] - cuts[index])
    return index, pos.astype(np.float32), len(cuts) - 1


@generator('RockCliff')
def make_rock_cliff():
    """Weathered sedimentary cliff rock, grey-beige with a hint of warmth: undulating beds of uneven thickness (along
    U), broken by irregular fractures at varied angles (mostly along the bedding, some diagonal and conchoidal, few
    vertical), each broken facet tilted its own way; dark water stains running down from the ledges. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    x, y = coords(shape)
    rng = np.random.default_rng(91)
    # Bedding: gently undulating (and wrapping), beds from thin laminae to thick banks.
    wy = y + 14.0 * np.sin(2.0 * np.pi * x / TILE + 0.7) + 9.0 * noise(shape, 92, 140.0, 40.0) + 2.5 * noise(shape, 93, 18.0, 8.0)
    index, pos, count = _layers(TILE, 94, (6.0, 150.0))
    yi = np.floor(wy).astype(np.int64) % TILE
    bed = index[yi]
    p = pos[yi]
    hard = rng.uniform(0.0, 1.0, count).astype(np.float32)[bed]
    profile = np.sin(np.pi * np.clip(p, 0.0, 1.0)) ** (0.3 + 0.8 * (1.0 - hard))
    # Fractures: a warped, sideways-stretched cell network, so the breaks run mostly along the beds or diagonally;
    # only some of its edges crack open.
    wx2 = x + 30.0 * noise(shape, 95, 45.0, 45.0) + 1.5 * noise(shape, 104, 8.0, 8.0)
    wy2 = y + 20.0 * noise(shape, 96, 45.0, 45.0) + 1.5 * noise(shape, 105, 8.0, 8.0)
    v = worley(shape, (8, 14), 97, jitter=1.0, x=wx2, y=wy2, stretch=(0.45, 1.0))
    ids, cells = v['id'], v['count']
    open_ = smooth(0.0, 0.8, noise(shape, 98, 50.0, 50.0))
    width = 1.4 + 2.6 * smooth(-1.0, 1.5, noise(shape, 106, 30.0, 30.0))
    crack = (1.0 - smooth(0.2, width, v['edge'])) * open_
    rim = (1.0 - smooth(0.0, 10.0, v['edge'])) * open_
    # Each broken facet leans its own way and is scooped a little (conchoidal breaks).
    tilt_x = per_cell(ids, cells, 99, -1.0, 1.0)
    tilt_y = per_cell(ids, cells, 100, -1.0, 1.0)
    fx = np.mod(wx2, TILE) - v['px']
    fy = np.mod(wy2, TILE) - v['py']
    facet = 0.00025 * (tilt_x * fx + tilt_y * fy) + 0.004 * np.sqrt(smooth(0.0, 40.0, v['edge']))
    joints = crack_lines(shape, 101, 160.0, 90.0, 1.2, 0.12, along_y=True)
    lamination = noise(shape, 102, 120.0, 2.5) * 0.5
    grit = noise(shape, 103, 3.0, 3.0, octaves=3)
    height = (0.012 + 0.022 * hard) * profile + facet * (0.4 + 0.6 * profile)
    height += 0.0012 * lamination + 0.002 * grit + 0.01 * noise(shape, 107, 80.0, 80.0)
    height -= 0.012 * crack + 0.007 * joints

    palette = [0x9b968b, 0x8f8a80, 0xa8a295, 0x86827b, 0xb1aa99, 0x9d968a, 0xa39a8c]
    table = np.array([rgb(palette[k]) for k in rng.integers(0, len(palette), count)], np.float32)
    warm = rng.uniform(0.0, 1.0, count).astype(np.float32)
    table = table + (warm[:, None] > 0.75) * (rgb(0xa89582) - table) * 0.35            # a hint of warm beds
    color = table[bed]
    color = color * (1.0 + 0.05 * per_cell(ids, cells, 108, -1.0, 1.0) + 0.04 * lamination + 0.06 * grit)[..., None]
    color = mix(color, rgb(0xc2bcad), smooth(0.6, 0.95, profile) * hard * 0.3)           # pale, weathered bed fronts
    color = mix(color, rgb(0x7c776d), rim * 0.25)
    # Water stains: dark streaks running down from the ledges and the cracks, stronger in some places.
    ledge = smooth(0.0, 0.08, p) * (1.0 - smooth(0.08, 0.2, p)) * hard
    source = np.clip(ledge * 0.8 + crack * 0.6, 0.0, 1.0)
    stain = np.clip(streaks_below(source * smooth(0.6, 1.6, noise(shape, 109, 3.0, 20.0)), 60.0, 110), 0.0, 1.0)
    stain = np.maximum(stain, smooth(1.0, 2.2, noise(shape, 111, 3.5, 140.0)) * 0.7)
    stain *= smooth(-0.6, 1.0, noise(shape, 112, 200.0, 260.0))
    color = mix(color, rgb(0x5a554d), stain * 0.45)
    color = mix(color, rgb(0x39342e), np.maximum(crack, joints) * 0.8)
    color = mix(color, rgb(0xa9a88c), specks(shape, 113, 0.015, 1.5) * smooth(0.3, 0.9, profile) * 0.6)
    ao = occlusion(height, px_m, (3, 10, 30), 1.3)
    color = color * (0.64 + 0.36 * ao)[..., None]
    rough = 0.88 + 0.05 * grit - 0.04 * stain
    save_set('RockCliff', color, height, px_m, rough, ao=ao, normal_strength=0.9)


@generator('RockGranite')
def make_rock_granite():
    """Granite boulders: speckled grey with warm feldspar, dark mica and white quartz grains, a few fractures,
    lichen rosettes. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    x, y = coords(shape)
    broad = noise(shape, 111, 120.0, 120.0, octaves=3)
    base = mix(rgb(0x8b8881), rgb(0x9a8e82), smooth(-1.0, 1.2, noise(shape, 112, 90.0, 90.0)))
    base = base * (1.0 + 0.08 * broad)[..., None]
    grain = worley(shape, (300, 300), 113, jitter=1.0)
    kind = per_cell(grain['id'], grain['count'], 114)
    color = mix(base, rgb(0x4a4743), (kind < 0.1).astype(np.float32) * 0.45)
    color = mix(color, rgb(0xc4c0b7), (kind > 0.92).astype(np.float32) * 0.3)
    color = mix(color, rgb(0xa38f82), ((kind > 0.75) & (kind <= 0.92)).astype(np.float32) * 0.25)
    fine = noise(shape, 115, 0.7, 0.7)
    color = color * (1.0 + 0.06 * fine)[..., None]
    # Weathering: darker, dirtier hollows and paler, sun-bleached crowns.
    knobs = noise(shape, 127, 25.0, 25.0, octaves=3)
    color = mix(color, rgb(0x6c675f), smooth(0.2, 1.5, -knobs) * 0.45)
    color = mix(color, rgb(0xb3ada1), smooth(0.4, 1.6, knobs) * 0.35)
    fracture = crack_lines(shape, 116, 180.0, 120.0, 1.3, 0.35) + crack_lines(shape, 117, 150.0, 100.0, 1.1, 0.3, along_y=True)
    fracture = np.clip(fracture, 0.0, 1.0)
    height = 0.02 * broad + 0.006 * knobs + 0.003 * noise(shape, 118, 6.0, 6.0, octaves=3) + 0.0006 * fine
    height -= 0.006 * fracture
    height += 0.0004 * (kind > 0.9)
    wx = x + 12.0 * noise(shape, 119, 10.0, 10.0)
    wy = y + 12.0 * noise(shape, 120, 10.0, 10.0)
    rosette = worley(shape, (12, 12), 121, x=wx, y=wy)
    size = per_cell(rosette['id'], rosette['count'], 122, 6.0, 26.0)
    alive = per_cell(rosette['id'], rosette['count'], 123) < 0.45
    lichen = smooth(1.0, 0.8, rosette['f1'] / size) * alive * smooth(-0.5, 0.8, noise(shape, 124, 2.0, 2.0))
    color = mix(color, mix(rgb(0xa9ab8e), rgb(0xc7c4a6), smooth(-1, 1, noise(shape, 125, 3.0, 3.0))), lichen * 0.85)
    color = mix(color, rgb(0xbb8a48), specks(shape, 126, 0.01, 1.4) * 0.7)
    color = mix(color, rgb(0x46403a), fracture * 0.7)
    height += 0.0006 * lichen
    ao = occlusion(height, px_m, (3, 10, 28), 1.2)
    color = color * (0.7 + 0.3 * ao)[..., None]
    rough = 0.8 + 0.08 * fine + 0.08 * lichen
    save_set('RockGranite', color, height, px_m, rough, ao=ao)


@generator('WoodPlanks')
def make_wood_planks():
    """Weathered planks for decks, fences and crates: 20 cm (64 px) boards along U with gaps, butt joints, nails at
    the joists (every 40 cm), knots and checks. 320 px/m (3.2 m); planks at V = k/16."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    h, w = shape
    x, y = coords(shape)
    rng = np.random.default_rng(131)
    plank = (y // 64).astype(np.int64)
    local = y - plank * 64.0
    seg = np.zeros(shape, np.int64)
    joint = np.full(shape, 1e9, np.float32)
    for b in range(16):
        ids, dist = segments_along_x(w, (1, 2), 132 + b)
        rows = plank == b
        seg = np.where(rows, b * 4 + ids[None, :], seg)
        joint = np.where(rows, dist[None, :], joint)
    tone = rng.uniform(0.0, 1.0, 64).astype(np.float32)[seg]
    bright = rng.uniform(-1.0, 1.0, 64).astype(np.float32)[seg]
    rings, streaks, fibers, knot = wood_grain(shape, 133, period=8.0, knots=14)
    shift_x = rng.integers(0, w, 64)[seg]
    shift_y = rng.integers(0, h, 64)[seg]
    gy = (y.astype(np.int64) + shift_y) % h
    gx = (x.astype(np.int64) + shift_x) % w
    rings, streaks, fibers, knot = rings[gy, gx], streaks[gy, gx], fibers[gy, gx], knot[gy, gx]
    edge = np.minimum(local, 63.0 - local)
    gap = np.maximum(1.0 - smooth(0.8, 2.2, edge), 1.0 - smooth(0.8, 2.2, joint))
    bevel = smooth(0.0, 6.0, np.minimum(edge, joint))
    cup = noise(shape, 134, 200.0, 30.0)
    check = crack_lines(shape, 135, 20.0, 180.0, 0.9, 0.25) * smooth(4.0, 10.0, edge)
    points = []
    for b in range(16):
        for k in range(8):
            if rng.random() < 0.9:
                for side in (-1, 1):
                    points.append(((k + 0.3) * w / 8.0 + rng.normal(0, 2), b * 64 + 32 + side * 16 + rng.normal(0, 1.5)))
    nails = stamp_discs(shape, points, 2.8)
    height = 0.012 * bevel ** 0.6 + 0.0012 * rings + 0.0003 * fibers + 0.0006 * cup - 0.003 * check - 0.001 * knot
    height += 0.0012 * nails
    height = np.where(gap > 0.5, -0.004, height)

    base = mix(rgb(0x8b7e6c), rgb(0x7d6750), tone)
    base = base * (1.0 + 0.08 * bright)[..., None]
    color = mix(base, rgb(0xa99c86), smooth(0.2, 1.4, noise(shape, 136, 150.0, 40.0)) * 0.45)
    color = mix(color, rgb(0x54473a), np.clip(rings * 0.55 + 0.35 * np.maximum(streaks, 0.0), 0.0, 1.0))
    color = mix(color, rgb(0x3b3028), np.maximum(knot * 0.85, check))
    color = color * (1.0 + 0.06 * fibers)[..., None]
    rust = np.clip(streaks_below(nails, 6.0, 137), 0.0, 1.0)
    color = mix(color, rgb(0x6f4a30), rust * 0.4)
    color = mix(color, rgb(0x3d3833), smooth(0.2, 0.6, nails))
    color = mix(color, rgb(0x5f5446), (1.0 - bevel) * 0.4)
    color = mix(color, rgb(0x241e19), gap * 0.9)
    ao = occlusion(height, px_m, (2, 6, 14), 1.2)
    color = color * (0.72 + 0.28 * ao)[..., None]
    rough = np.where(nails > 0.2, 0.5, 0.85 + 0.05 * fibers)
    metal = np.where(nails > 0.2, 0.5, 0.0)
    save_set('WoodPlanks', color, height, px_m, rough, metal, ao=ao)


@generator('StoneWall')
def make_stone_wall():
    """A dry-stone wall: flat stones in rough courses, deep dark gaps (no mortar) with moss, lichen. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    x, y = coords(shape)
    # Courses of flat stones: rows of varying height, each split into stones of varying length; the outlines are
    # rounded and roughened so no stone is a rectangle.
    wx = x + 7.0 * noise(shape, 141, 10.0, 10.0, octaves=2)
    wy = y + 5.0 * noise(shape, 142, 10.0, 10.0, octaves=2)
    course, pos, rows = _layers(TILE, 143, (36.0, 90.0))
    yi = np.floor(wy).astype(np.int64) % TILE
    row = course[yi]
    top = pos[yi]
    starts = np.concatenate([[0.0], np.cumsum(np.bincount(course, minlength=rows).astype(np.float32))])
    row_h = (starts[1:] - starts[:-1])[row]
    ey = np.minimum(top, 1.0 - top) * row_h
    ids = np.zeros(shape, np.int64)
    ex = np.zeros(shape, np.float32)
    cols = np.arange(TILE)
    xi = np.floor(wx).astype(np.int64) % TILE
    for r in range(rows):
        seg, dist = segments_along_x(TILE, (4, 9), 300 + r, min_gap=0.07)
        here = row == r
        ids = np.where(here, r * 16 + seg[xi], ids)
        ex = np.where(here, dist[xi], ex)
    count = rows * 16
    radius = 7.0
    corner = (ex < radius) & (ey < radius)
    edge = np.where(corner, radius - np.hypot(radius - ex, radius - ey), np.minimum(ex, ey))
    edge = edge + 2.0 * noise(shape, 155, 5.0, 5.0) + 0.6 * noise(shape, 156, 1.5, 1.5)
    mask = smooth(2.0, 4.0, edge)
    face = np.sqrt(smooth(2.0, 12.0, edge))
    tilt_x = per_cell(ids, count, 144, -1.0, 1.0)
    tilt_y = per_cell(ids, count, 145, -1.0, 1.0)
    height = mask * (0.03 + 0.012 * face + 0.004 * tilt_x * (ex / 60.0) + 0.004 * tilt_y * (top - 0.5))
    height += mask * (0.004 * noise(shape, 146, 6.0, 6.0, octaves=3) + 0.0012 * noise(shape, 147, 50.0, 1.5))
    height += (1.0 - mask) * 0.004 * noise(shape, 148, 3.0, 3.0)
    stone = palette_pick(ids, count, 149, [0x857f74, 0x9a9282, 0x76726a, 0xa39c8c, 0x8e8272, 0x6d6a64, 0x99907c])
    stone = stone * (1.0 + 0.09 * noise(shape, 150, 5.0, 5.0, octaves=2))[..., None]
    stone = mix(stone, rgb(0xb8b09e), smooth(0.7, 1.0, face) * 0.2)
    stone = mix(stone, rgb(0xa8a784), specks(shape, 151, 0.05, 1.8) * 0.75)
    stone = mix(stone, rgb(0xbd8c4c), specks(shape, 152, 0.012, 1.3) * 0.6)
    moss_n = noise(shape, 153, 3.0, 3.0, octaves=2)
    moss = smooth(0.2, 1.2, moss_n + 1.2 * (1.0 - smooth(3.0, 9.0, edge)) - 0.6) * smooth(-0.4, 1.0, noise(shape, 154, 80.0, 80.0))
    gap = mix(rgb(0x2a2622), rgb(0x3b3a2c), smooth(-1, 1, moss_n))
    color = mix(gap, stone, mask)
    color = mix(color, mix(rgb(0x4f5c2c), rgb(0x6d7a3a), smooth(-1, 1, moss_n)), moss * 0.85)
    height += 0.002 * moss
    ao = occlusion(height, px_m, (3, 8, 20), 1.3)
    color = color * (0.62 + 0.38 * ao)[..., None]
    rough = np.where(mask > 0.5, 0.86, 0.95) - 0.05 * moss
    save_set('StoneWall', color, height, px_m, rough, ao=ao)


@generator('MetalRust')
def make_metal_rust():
    """Rusty painted metal (barrels, tin props): faded sage paint (tint it), chips to bare steel at wear, rust
    blooming from the chips and running down, pitting, scratches. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    x, y = coords(shape)
    density = smooth(-1.2, 1.2, noise(shape, 177, 160.0, 160.0))       # rust gathers in some areas, spares others
    wear = noise(shape, 161, 14.0, 14.0, octaves=4) + 0.5 * density
    chips = smooth(1.2, 1.35, wear)
    rust_n = noise(shape, 162, 24.0, 40.0, octaves=4) + 1.1 * density - 0.4
    rust = np.clip(smooth(0.9, 1.6, rust_n) + smooth(0.8, 1.15, wear) * 0.7, 0.0, 1.0)
    flaking = smooth(0.6, 0.9, rust_n) * (1.0 - smooth(0.9, 1.2, rust_n))   # lifted paint around the rust
    runs = np.clip(streaks_below(rust * smooth(0.6, 1.0, rust), 40.0, 163), 0.0, 1.0)
    runs *= smooth(0.2, 1.6, noise(shape, 164, 3.0, 50.0))
    rust = np.clip(rust + runs * 0.6, 0.0, 1.0)
    scratch = np.clip(crack_lines(shape, 165, 60.0, 40.0, 0.7, 0.15) + crack_lines(shape, 166, 50.0, 50.0, 0.6, 0.12, along_y=True), 0, 1)
    paint = mix(rgb(0x75806f), rgb(0x9aa391), smooth(-1.0, 1.5, noise(shape, 167, 60.0, 60.0, octaves=2)) * 0.7)
    paint = paint * (1.0 + 0.04 * noise(shape, 168, 3.0, 3.0))[..., None]
    steel = mix(rgb(0x5b5e60), rgb(0x7b7e80), smooth(-1, 1, noise(shape, 169, 2.0, 2.0)))
    rust_color = mix(rgb(0x7f4122), rgb(0x9d5a2c), smooth(-1.0, 1.0, noise(shape, 170, 3.0, 6.0)))
    rust_color = mix(rust_color, rgb(0x4e2a18), smooth(0.5, 1.5, noise(shape, 171, 8.0, 8.0)) * 0.6)
    color = mix(paint, steel, np.maximum(chips, scratch * 0.7))
    color = mix(color, rgb(0xb5bba8), flaking * 0.35)
    color = mix(color, rust_color, rust)
    color = mix(color, rgb(0x4a463e), smooth(0.3, 1.8, noise(shape, 172, 6.0, 120.0)) * 0.2)   # grime runs
    pits = specks(shape, 173, 0.08, 1.0) * rust
    height = 0.0009 * (1.0 - blur(np.maximum(chips, rust), 0.8)) - 0.0009 * pits + 0.0004 * flaking
    height += 0.0008 * rust * noise(shape, 174, 1.5, 1.5, octaves=2) - 0.0004 * scratch
    height += 0.003 * noise(shape, 175, 70.0, 70.0) + 0.0005 * noise(shape, 176, 8.0, 8.0)   # dents, orange peel
    ao = occlusion(height, px_m, (2, 6), 0.8)
    color = color * (0.8 + 0.2 * ao)[..., None]
    bare = np.maximum(chips, scratch * 0.7) * (1.0 - rust)
    metal = np.clip(bare, 0.0, 1.0)
    rough = 0.55 * (1.0 - rust) + 0.88 * rust - 0.15 * bare
    save_set('MetalRust', color, height, px_m, np.clip(rough, 0, 1), metal, ao=ao, normal_strength=1.6)


@generator('MetalWorn')
def make_metal_worn():
    """Worn bare metal in a neutral light grey, so a tint makes it any metal (gold, brass, gunmetal, dark iron): faint
    brushing along U, fine scratches, soft grime patches and pits, polished where hands rub it. Fully metallic,
    rougher and duller under the grime. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    brushed = noise(shape, 301, 40.0, 0.7)
    grime = smooth(-0.3, 1.7, noise(shape, 302, 50.0, 50.0, octaves=3))
    polish = smooth(0.3, 1.6, noise(shape, 303, 90.0, 90.0, octaves=2))
    scratch = np.clip(crack_lines(shape, 304, 40.0, 60.0, 0.6, 0.18) +
                      crack_lines(shape, 305, 55.0, 45.0, 0.5, 0.12, along_y=True), 0.0, 1.0)
    pits = specks(shape, 306, 0.025, 0.8)
    color = mix(rgb(0xc2c1be), rgb(0xe0dfdc), np.clip(smooth(-1.0, 1.0, brushed) * 0.2 + polish * 0.35, 0.0, 1.0))
    color = color * (1.0 + 0.04 * noise(shape, 307, 6.0, 6.0))[..., None]
    color = mix(color, rgb(0x6a665f), grime * 0.3)
    color = mix(color, rgb(0xf2f1ee), scratch * 0.25)
    color = mix(color, rgb(0x3d3a36), pits * 0.7)
    height = 0.0003 * brushed - 0.0004 * scratch - 0.0006 * pits + 0.0018 * noise(shape, 308, 60.0, 60.0)
    ao = occlusion(height, px_m, (2, 6), 0.6)
    rough = 0.3 + 0.32 * grime - 0.12 * polish + 0.1 * scratch + 0.04 * brushed
    metal = 1.0 - 0.45 * grime
    save_set('MetalWorn', color, height, px_m, np.clip(rough, 0.05, 1.0), np.clip(metal, 0.0, 1.0), ao=ao)


@generator('PaintWorn')
def make_paint_worn():
    """Worn paint over steel in a neutral off-white, to be tinted any color (olive crates, cream bands, gun paint): a
    faint orange-peel finish, scuffed thin in patches, chipped through to dark steel, scratches, dust settling in soft
    patches. Metallic only in the chips. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    wear = noise(shape, 311, 16.0, 16.0, octaves=4)
    chips = smooth(1.75, 1.9, wear)
    scuffs = smooth(1.0, 1.55, wear) * (1.0 - chips)
    dust = smooth(-0.2, 1.8, noise(shape, 312, 70.0, 70.0, octaves=2))
    scratch = np.clip(crack_lines(shape, 314, 50.0, 50.0, 0.6, 0.14) +
                      crack_lines(shape, 315, 60.0, 40.0, 0.5, 0.1, along_y=True), 0.0, 1.0)
    paint = mix(rgb(0xd6d4ce), rgb(0xe9e7e1), smooth(-1.0, 1.0, noise(shape, 313, 40.0, 40.0, octaves=2)) * 0.6)
    color = mix(paint, paint * 0.86, scuffs)
    bare = np.maximum(chips, scratch * 0.6)
    color = mix(color, rgb(0x4b4c4d), bare)
    color = mix(color, rgb(0xb9b09e), dust * 0.22)
    height = 0.0004 * (1.0 - blur(chips, 0.8)) + 0.00012 * noise(shape, 316, 1.5, 1.5) - 0.0002 * scratch
    height += 0.0015 * noise(shape, 317, 70.0, 70.0)
    ao = occlusion(height, px_m, (2, 6), 0.7)
    color = color * (0.85 + 0.15 * ao)[..., None]
    rough = 0.5 + 0.18 * dust + 0.08 * scuffs - 0.08 * bare
    save_set('PaintWorn', color, height, px_m, np.clip(rough, 0.0, 1.0), np.clip(bare, 0.0, 1.0), ao=ao)


@generator('Polymer')
def make_polymer():
    """Molded polymer for gun furniture and grips, in a neutral light grey to be tinted (black to sand): a fine stipple, faint mold-flow
    mottling, a satin sheen rubbed smoother where hands hold it. Non-metallic. 1024 px/m (guns are seen up close)."""
    shape = (TILE, TILE)
    px_m = 1.0 / 1024.0
    stipple = noise(shape, 321, 1.1, 1.1)
    mottle = noise(shape, 322, 80.0, 80.0, octaves=2)
    polish = smooth(0.6, 1.8, noise(shape, 323, 70.0, 70.0, octaves=2))
    scuff = np.clip(crack_lines(shape, 324, 70.0, 50.0, 0.5, 0.08), 0.0, 1.0)
    color = rgb(0xcbcbcb) * (1.0 + 0.035 * mottle + 0.012 * stipple + 0.05 * polish)[..., None]
    color = mix(color, rgb(0xe2e2e2), scuff * 0.4)
    height = 0.00004 * stipple * (1.0 - 0.6 * polish) - 0.00005 * scuff
    ao = occlusion(height, px_m, (2, 4), 0.2)
    rough = 0.64 + 0.06 * stipple - 0.22 * polish
    save_set('Polymer', color, height, px_m, np.clip(rough, 0.0, 1.0), None, ao=ao, normal_strength=0.8)


@generator('GunWood')
def make_gun_wood():
    """Oiled walnut for gun stocks and handguards: fine growth rings and pores along U, warm brown with darker
    streaks, worn lighter and glossier where hands rub, small dings. Tint it for lighter or redder woods. 1024 px/m
    (guns are seen up close)."""
    shape = (TILE, TILE)
    px_m = 1.0 / 1024.0
    rings, streaks, fibers, knot = wood_grain(shape, 341, period=9.0, warp=1.4, knots=2, knot_size=10.0)
    pores = specks(shape, 342, 0.06, 0.6) * (1.0 - rings * 0.5)
    wear = smooth(0.4, 1.8, noise(shape, 343, 110.0, 70.0, octaves=2))
    dings = specks(shape, 344, 0.004, 2.0)
    base = mix(rgb(0x7a5236), rgb(0x8f6340), smooth(-1.0, 1.0, noise(shape, 345, 200.0, 40.0)))
    color = mix(base, rgb(0x4a2e1d), np.clip(rings * 0.7 + 0.3 * np.maximum(streaks, 0.0), 0.0, 1.0))
    color = color * (1.0 + 0.05 * fibers)[..., None]
    color = mix(color, rgb(0x3a2416), np.maximum(knot * 0.8, pores * 0.5))
    color = mix(color, rgb(0xa77a52), wear * 0.25)
    color = mix(color, rgb(0x2f1d12), dings * 0.6)
    height = 0.00012 * rings - 0.00015 * pores + 0.00005 * fibers - 0.0002 * dings
    ao = occlusion(height, px_m, (2, 5), 0.5)
    rough = 0.48 + 0.08 * rings + 0.1 * pores - 0.18 * wear
    save_set('GunWood', color, height, px_m, np.clip(rough, 0.0, 1.0), None, ao=ao, normal_strength=0.8)


@generator('Hay')
def make_hay():
    """Straw for hay bales: long strands mostly along U, golden to pale, with dark gaps between. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    rng = np.random.default_rng(181)
    canvas = Canvas(shape, 0.0, color=np.broadcast_to(rgb(0x3a2e1c), shape + (3,)))
    canvas.top[:] = -0.02
    straws = np.array([rgb(c) for c in (0xc9a95f, 0xd8be78, 0xb39352, 0xa08547, 0xc2ab6a, 0xa9a05e, 0xdcc88c)], np.float32)
    # Clumps: the bale's straw lies in bundles, each with its own lean and height.
    clump = noise(shape, 182, 40.0, 25.0, octaves=2)
    lean = noise(shape, 183, 90.0, 60.0) * 0.35
    for batch in range(6):
        count = 2600
        x0 = rng.uniform(0, TILE, count).astype(np.float32)
        y0 = rng.uniform(0, TILE, count).astype(np.float32)
        here = (y0.astype(int) % TILE, x0.astype(int) % TILE)
        angle = (rng.normal(0.0, 0.2, count) + lean[here] + np.pi * (rng.random(count) < 0.5)).astype(np.float32)
        length = rng.uniform(50.0, 190.0, count).astype(np.float32)
        width = rng.uniform(3.0, 5.0, count).astype(np.float32)
        bend = rng.normal(0.0, 0.06, count).astype(np.float32)
        x, y, t, i, across = stroke_points(x0, y0, angle, length, width, bend, taper=0.2, step=0.9)
        tone = straws[rng.integers(0, len(straws), count)] * rng.uniform(0.8, 1.1, count).astype(np.float32)[:, None]
        lift = (rng.uniform(0.0, 0.012, count) + 0.01 * np.clip(clump[here], -1.5, 1.5)).astype(np.float32)
        round_ = np.sqrt(np.clip(1.0 - across ** 2, 0.0, 1.0))
        depth = lift[i] + 0.0025 * round_ + 0.005 * np.sin(np.pi * t) * rng.uniform(0, 1, count).astype(np.float32)[i]
        shine = (0.7 + 0.3 * round_ + 0.15 * smooth(0.7, 1.0, round_))
        canvas.paint(x, y, depth, color=tone[i] * shine[:, None])
    height = canvas.height()
    color = canvas.get('color')
    ao = occlusion(height, px_m, (2, 6, 16, 40), 1.6)
    color = color * (0.45 + 0.55 * ao)[..., None]
    rough = 0.72 + 0.2 * (1.0 - ao)
    save_set('Hay', color, height, px_m, rough, ao=ao, normal_strength=0.8)


@generator('BarkOak')
def make_bark_oak():
    """Oak bark: deep furrows along V (up the trunk) between blocky, interlinked ridges; grey-brown, moss in the
    furrows on one side. 320 px/m (3.2 m around and along)."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    x, y = coords(shape)
    wx = x + 10.0 * noise(shape, 191, 30.0, 60.0, octaves=2)
    wy = y + 6.0 * noise(shape, 192, 30.0, 30.0)
    v = worley(shape, (22, 7), 193, jitter=0.9, x=wx, y=wy, stretch=(1.0, 0.28))
    ridge = smooth(0.0, 9.0, v['edge'])
    ridge = ridge ** 0.6
    plates = crack_lines(shape, 194, 40.0, 16.0, 1.4, 0.5)   # cross cuts break the ridges into blocks
    block = per_cell(v['id'], v['count'], 195, -1.0, 1.0)
    rough_n = noise(shape, 196, 3.0, 6.0, octaves=3)
    height = 0.022 * ridge * (1.0 - 0.6 * plates) + 0.004 * block * ridge + 0.003 * rough_n
    height += 0.002 * noise(shape, 197, 60.0, 10.0)
    top = smooth(0.5, 1.0, ridge)
    color = mix(rgb(0x2b241f), rgb(0x5c5044), smooth(0.05, 0.6, ridge))
    color = mix(color, rgb(0x7d7162), top * (0.55 + 0.25 * block[..., None][..., 0]))
    color = color * (1.0 + 0.1 * rough_n)[..., None]
    color = mix(color, rgb(0xa2a38a), specks(shape, 198, 0.03, 1.6) * top * 0.6)
    moss = smooth(0.3, 1.3, noise(shape, 199, 60.0, 120.0) + 0.8 * (1.0 - ridge) - 0.4) * smooth(0.4, 0.0, ridge)
    color = mix(color, rgb(0x55612f), moss * 0.7)
    ao = occlusion(height, px_m, (3, 8, 20), 1.4)
    color = color * (0.62 + 0.38 * ao)[..., None]
    rough = 0.9 + 0.05 * rough_n
    save_set('BarkOak', color, height, px_m, rough, ao=ao)


@generator('BarkBirch')
def make_bark_birch():
    """Birch bark: warm off-white with grey, thin dark lenticels (horizontal dashes across the trunk, 2 to 8 cm long, a
    few mm tall) gathered in bands, peeling curls showing the tan inner bark, a few small dark branch scars. The
    darker, fissured bark near the base is the mesh's business (it can't live in a tiling texture). 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    x, y = coords(shape)
    rng = np.random.default_rng(201)
    base = mix(rgb(0xddd6c8), rgb(0xc4bdb0), smooth(-1.2, 1.2, noise(shape, 202, 45.0, 25.0, octaves=2)))
    base = mix(base, rgb(0xaaa69c), smooth(0.6, 1.8, noise(shape, 203, 30.0, 10.0, octaves=2)) * 0.45)   # grey film
    base = mix(base, rgb(0xd8c9b8), smooth(0.9, 1.9, noise(shape, 214, 40.0, 15.0)) * 0.4)               # warm patches
    papery = noise(shape, 204, 90.0, 2.0) * smooth(0.3, 1.5, noise(shape, 212, 60.0, 40.0))
    base = base * (1.0 + 0.03 * papery + 0.02 * noise(shape, 215, 1.5, 1.5))[..., None]
    height = 0.0004 * papery
    canvas = Canvas(shape, 0.0, color=base, kind=np.zeros(shape))
    canvas.top[:] = -1.0
    # Lenticels: thin dark dashes across the trunk, in bands.
    count = 4200
    x0 = rng.uniform(0, TILE, count).astype(np.float32)
    y0 = rng.uniform(0, TILE, count).astype(np.float32)
    bands = smooth(-0.3, 1.2, noise(shape, 213, 400.0, 22.0))
    keep = rng.random(count) < 0.05 + 0.4 * bands[y0.astype(int), x0.astype(int)]
    x0, y0 = x0[keep], y0[keep]
    count = len(x0)
    length = (6.0 + 20.0 * rng.random(count) ** 1.6).astype(np.float32)
    xs, ys, t, i, across = stroke_points(x0, y0, rng.normal(0.0, 0.03, count).astype(np.float32), length,
                                         rng.uniform(1.1, 2.1, count).astype(np.float32), taper=0.1, step=0.6)
    ends = np.minimum(t, 1.0 - t) * length[i]                       # a dash is thinner toward its ends
    tone = np.array([rgb(c) for c in (0x3f3a36, 0x4d4540, 0x5a524b, 0x35302d)], np.float32)[rng.integers(0, 4, count)]
    canvas.paint(xs, ys, np.zeros_like(t), color=tone[i] * (1.0 + 0.3 * (1.0 - smooth(0.0, 3.0, ends)))[:, None],
                 kind=np.ones_like(t))
    color = canvas.get('color')
    dash = blur(canvas.get('kind'), 0.6)
    height -= 0.0006 * dash
    # Peeling curls: strips of the papery outer bark lifting away, tan inner bark beneath, a lit rolled edge on top
    # and a shadow under it.
    for k in range(34):
        cx, cy = rng.uniform(0, TILE), rng.uniform(0, TILE)
        half_len, half_h = rng.uniform(15.0, 70.0), rng.uniform(2.5, 6.0)
        dx = (x - cx + TILE / 2) % TILE - TILE / 2
        dy = (y - cy + TILE / 2) % TILE - TILE / 2
        wob = 1.5 * noise(shape, 300 + k % 6, 6.0, 2.0) if k < 6 else 0.0
        inside = np.clip(1.0 - (dx / half_len) ** 2 - ((dy + wob) / half_h) ** 2, 0.0, 1.0)
        if not inside.any():
            continue
        peel = smooth(0.0, 0.2, inside)
        color = mix(color, mix(rgb(0xc59c78), rgb(0xa9805e), smooth(-1, 1, noise(shape, 220, 2.0, 2.0))), peel * 0.85)
        edge = np.exp(-((dy + half_h) / 1.4) ** 2) * (np.abs(dx) < half_len * 0.95)   # the rolled top edge
        shadow = np.exp(-((dy - half_h - 1.0) / 1.6) ** 2) * (np.abs(dx) < half_len * 0.9)
        color = mix(color, rgb(0xf2ede4), edge * 0.8)
        color = mix(color, rgb(0x6d645a), shadow * 0.5)
        height += 0.0015 * edge - 0.0006 * peel
    # A few small branch scars: dark, rough chevrons.
    scars = np.zeros(shape, np.float32)
    ragged = noise(shape, 222, 3.0, 2.0, octaves=2)
    for k in range(5):
        cx, cy = rng.uniform(0, TILE), rng.uniform(0, TILE)
        w_, h_ = rng.uniform(30.0, 55.0), rng.uniform(12.0, 20.0)
        dx = (x - cx + TILE / 2) % TILE - TILE / 2
        dy = (y - cy + TILE / 2) % TILE - TILE / 2
        # A broad, ragged mark, drooping at its sides like the scar under a lost branch.
        shape_ = (dx / w_) ** 2 + ((dy - 0.35 * h_ * (dx / w_) ** 2) / h_) ** 2 + 0.25 * ragged
        scars = np.maximum(scars, smooth(1.0, 0.75, shape_))
    rough_n = noise(shape, 221, 2.0, 2.0, octaves=2)
    color = mix(color, mix(rgb(0x2c2826), rgb(0x46403b), smooth(-1, 1, rough_n)), scars * 0.9)
    height += scars * (0.002 + 0.0015 * rough_n)
    ao = occlusion(height, px_m, (2, 6, 16), 1.0)
    color = color * (0.8 + 0.2 * ao)[..., None]
    rough = np.clip(0.6 + 0.25 * dash + 0.3 * scars + 0.05 * rough_n, 0.0, 1.0)
    save_set('BarkBirch', color, height, px_m, rough, ao=ao)


@generator('BarkPine')
def make_bark_pine():
    """Pine bark: reddish-brown scaly plates, their grey outer flakes worn to orange beneath, split by deep dark
    fissures. 320 px/m."""
    shape = (TILE, TILE)
    px_m = 1.0 / 320.0
    x, y = coords(shape)
    wx = x + 12.0 * noise(shape, 221, 20.0, 30.0, octaves=2)
    wy = y + 12.0 * noise(shape, 222, 20.0, 30.0, octaves=2)
    v = worley(shape, (12, 8), 223, jitter=0.95, x=wx, y=wy, stretch=(1.0, 0.6))
    plate = smooth(2.0, 5.0, v['edge'])
    dome = np.sqrt(smooth(2.0, 22.0, v['edge']))
    scales = worley(shape, (40, 30), 224, jitter=1.0, x=wx, y=wy)
    flake = smooth(0.5, 3.0, scales['edge'])
    flake_top = per_cell(scales['id'], scales['count'], 225)
    height = plate * (0.014 + 0.014 * dome + 0.001 * flake + 0.0012 * flake_top) + 0.002 * noise(shape, 226, 3.0, 3.0)
    tones = palette_pick(v['id'], v['count'], 227, [0x8f5638, 0x7d4a30, 0xa4623f, 0x965434, 0x844e33])
    color = tones * (0.92 + 0.1 * flake_top)[..., None]
    # Grey weathered outer flakes on the plates' crowns; the fresh orange-red shows where they've come off.
    weathered = smooth(0.45, 0.8, 0.6 * dome + 0.5 * smooth(-1.0, 1.0, noise(shape, 228, 30.0, 30.0)) - 0.1 * flake_top)
    color = mix(color, mix(rgb(0x75685d), rgb(0x8c8176), smooth(-1, 1, noise(shape, 229, 4.0, 4.0))), weathered * 0.75)
    color = mix(color, rgb(0x4a3024), (1.0 - flake) * 0.25 * plate)
    color = mix(color, rgb(0x21180f), 1.0 - plate)
    grey = weathered
    ao = occlusion(height, px_m, (3, 8, 20), 1.3)
    color = color * (0.66 + 0.34 * ao)[..., None]
    rough = 0.88 + 0.06 * grey
    save_set('BarkPine', color, height, px_m, rough, ao=ao)


# --- Foliage atlases: 2 x 2 leaf-cluster sprites, alpha in the base color ---

class Sprite:
    """One quadrant of a leaf atlas: layers drawn back to front (color, alpha, normal, occlusion, roughness)."""

    def __init__(self, size):
        self.size = size
        self.color = np.zeros((size, size, 3), np.float32)
        self.alpha = np.zeros((size, size), np.float32)
        self.normal = np.zeros((size, size, 3), np.float32)
        self.normal[..., 2] = 1.0
        self.ao = np.ones((size, size), np.float32)
        self.rough = np.full((size, size), 0.7, np.float32)

    def box(self, points, pad=2):
        xs, ys = [p[0] for p in points], [p[1] for p in points]
        x0, x1 = max(int(min(xs)) - pad, 0), min(int(max(xs)) + pad + 1, self.size)
        y0, y1 = max(int(min(ys)) - pad, 0), min(int(max(ys)) + pad + 1, self.size)
        if x0 >= x1 or y0 >= y1:
            return None
        y, x = np.mgrid[y0:y1, x0:x1].astype(np.float32)
        return (slice(y0, y1), slice(x0, x1)), x + 0.5, y + 0.5

    def put(self, region, a, color, normal, ao, rough):
        s = region
        old = self.alpha[s]
        self.color[s] = self.color[s] * (1.0 - a[..., None]) + color * a[..., None]
        self.alpha[s] = np.maximum(old, a)
        solid = (a > 0.5)[..., None]
        self.normal[s] = np.where(solid, normal, self.normal[s])
        self.ao[s] = np.where(a > 0.5, ao, self.ao[s])
        self.rough[s] = np.where(a > 0.5, rough, self.rough[s])

    def capsule(self, p0, p1, r0, r1, color, ao=1.0, rough=0.85):
        """A tapering round stroke (twig, needle) from p0 to p1, radius r0 to r1 px."""
        r = max(r0, r1)
        got = self.box([(p0[0] - r, p0[1] - r), (p0[0] + r, p0[1] + r), (p1[0] - r, p1[1] - r), (p1[0] + r, p1[1] + r)])
        if got is None:
            return
        region, x, y = got
        dx, dy = p1[0] - p0[0], p1[1] - p0[1]
        length = max(math.hypot(dx, dy), 1e-3)
        ux, uy = dx / length, dy / length
        t = np.clip(((x - p0[0]) * ux + (y - p0[1]) * uy) / length, 0.0, 1.0)
        cx, cy = p0[0] + dx * t, p0[1] + dy * t
        side = (x - cx) * -uy + (y - cy) * ux
        dist = np.hypot(x - cx, y - cy)
        radius = r0 + (r1 - r0) * t
        a = np.clip(radius - dist + 0.5, 0.0, 1.0)
        across = np.clip(side / np.maximum(radius, 0.5), -1.0, 1.0)
        nz = np.sqrt(np.clip(1.0 - across ** 2, 0.05, 1.0))
        normal = np.stack([-uy * across, ux * across, nz], axis=-1)
        shade = (0.75 + 0.25 * nz)[..., None]
        col = np.broadcast_to(np.asarray(color, np.float32), a.shape + (3,)) * shade
        self.put(region, a, col, normal, np.full(a.shape, ao, np.float32), np.full(a.shape, rough, np.float32))

    def leaf(self, base, angle, length, shape_fn, color, ao, rng, fold=0.28, vein_color=None, veins=8):
        """A leaf from base along angle: shape_fn(u) gives its half-width (in lengths) along the midrib u (0..1)."""
        dx, dy = math.cos(angle), math.sin(angle)
        nx, ny = -dy, dx
        half = 0.4 * length
        corners = [(base[0] + dx * a + nx * b, base[1] + dy * a + ny * b) for a in (0.0, length) for b in (-half, half)]
        got = self.box(corners)
        if got is None:
            return
        region, x, y = got
        px, py = x - base[0], y - base[1]
        u = (px * dx + py * dy) / length
        v = (px * nx + py * ny) / length
        w = shape_fn(np.clip(u, 0.0, 1.0))
        inside = np.minimum((w - np.abs(v)) * length, np.minimum(u, 1.0 - u) * length)
        a = np.clip(inside + 0.5, 0.0, 1.0)
        if not a.any():
            return
        c = np.broadcast_to(np.asarray(color, np.float32), a.shape + (3,)) * (0.82 + 0.25 * u)[..., None]
        light = rgb(0xc9d27a) if vein_color is None else np.asarray(vein_color, np.float32)
        midrib = np.exp(-(v * length / 1.6) ** 2) * (u < 0.96)
        s = (u * 0.85 + np.abs(v) * 1.3) * veins
        side_vein = np.exp(-((s - np.round(s)) * 7.0) ** 2) * (np.abs(v) < w * 0.85) * (u > 0.05)
        c = mix(c, light, midrib * 0.35 + side_vein * 0.12)
        c = mix(c, c * 0.7, 1.0 - smooth(0.0, 6.0, inside))                      # darker rim
        tilt_u, tilt_v = rng.uniform(-0.25, 0.25), rng.uniform(-0.3, 0.3)
        arch = rng.uniform(0.15, 0.45)
        dh_du = arch * (1.0 - 2.0 * u) + tilt_u
        dh_dv = fold * np.sign(v) + tilt_v
        gx = dh_du * dx + dh_dv * nx
        gy = dh_du * dy + dh_dv * ny
        n = np.stack([-gx, -gy, np.ones_like(gx)], axis=-1)
        n /= np.linalg.norm(n, axis=-1, keepdims=True)
        occ = ao * (0.9 + 0.1 * u) * (1.0 - 0.25 * midrib)
        self.put(region, a, c, n, occ, 0.55 + 0.1 * side_vein)


def _oak_leaf(u):
    base = np.clip(np.sin(np.pi * u), 0.0, 1.0) ** 0.6 * (0.6 + 0.4 * np.abs(np.cos(np.pi * 4.5 * u)) ** 0.7)
    return 0.3 * base * (0.35 + 0.65 * smooth(0.0, 0.15, u))


def _birch_leaf(u):
    return 0.95 * u ** 0.6 * (1.0 - u) * (1.0 - 0.09 * ((u * 16.0) % 1.0))


def _branch(rng, size, droop=0.0, sides=(4, 6), spread=(0.35, 0.8), side_len=(0.3, 0.48)):
    """A twig from the bottom middle to near the top, with side twigs: a list of polylines [(points, r0, r1)]."""
    start = np.array([size * 0.5 + rng.uniform(-30, 30), size - 12.0])
    end = np.array([size * 0.5 + rng.uniform(-160, 160), size * rng.uniform(0.1, 0.18)])
    bow = rng.uniform(-90, 90)
    main = []
    for t in np.linspace(0.0, 1.0, 9):
        p = start + (end - start) * t
        main.append((p[0] + bow * math.sin(math.pi * t), p[1]))
    twigs = [(main, 11.0, 4.0)]
    count = int(rng.integers(sides[0], sides[1] + 1))
    for k in range(count):
        t = 0.1 + 0.78 * (k + rng.uniform(0.0, 1.0)) / count
        i = min(int(t * 8), 7)
        p0 = np.array(main[i])
        direction = np.array(main[i + 1]) - p0
        heading = math.atan2(direction[1], direction[0])
        side = 1 if k % 2 == 0 else -1
        angle = heading + side * rng.uniform(*spread) * math.pi * 0.5
        length = size * rng.uniform(*side_len)
        points = [tuple(p0)]
        for j in range(1, 6):
            angle += side * rng.uniform(-0.08, 0.12) + droop * 0.12 * (1.0 if math.cos(angle) > 0 else -1.0)
            last = points[-1]
            points.append((last[0] + math.cos(angle) * length / 5.0, last[1] + math.sin(angle) * length / 5.0))
        twigs.append((points, 6.0 - 3.0 * t, 2.2))
    return twigs


def _inside(size, points, margin=10.0):
    return all(margin <= p[0] <= size - margin and margin <= p[1] <= size - margin for p in points)


def _leaf_sprite(size, seed, shape_fn, colors, count, length, droop=0.0, twig_color=0x5b4a3a, fold=0.28, veins=8,
                 sides=(4, 6)):
    rng = np.random.default_rng(seed)
    sprite = Sprite(size)
    twigs = _branch(rng, size, droop, sides)
    # Leaf spots along the twigs; then sort by depth so the back ones are drawn first (and darker).
    spots = []
    for points, r0, r1 in twigs:
        for _ in range(count // len(twigs) + 2):
            t = rng.uniform(0.15, 1.0)
            f = t * (len(points) - 1)
            i = min(int(f), len(points) - 2)
            p = np.array(points[i]) + (np.array(points[i + 1]) - np.array(points[i])) * (f - i)
            d = np.array(points[i + 1]) - np.array(points[i])
            heading = math.atan2(d[1], d[0])
            spots.append((p, heading, rng.uniform(0.0, 1.0)))
    spots.sort(key=lambda s: s[2])
    for points, r0, r1 in twigs:
        for i in range(len(points) - 1):
            rr0 = r0 + (r1 - r0) * i / (len(points) - 1)
            rr1 = r0 + (r1 - r0) * (i + 1) / (len(points) - 1)
            sprite.capsule(points[i], points[i + 1], rr0, rr1, rgb(twig_color), ao=0.6, rough=0.85)
    palette = [rgb(c) for c in colors]
    placed = 0
    for p, heading, depth in spots:
        angle = heading + rng.choice([-1, 1]) * rng.uniform(0.35, 1.2)
        leaf_len = rng.uniform(*length)
        stalk = leaf_len * 0.08
        for _ in range(4):
            base = (p[0] + math.cos(angle) * stalk, p[1] + math.sin(angle) * stalk)
            dx, dy = math.cos(angle), math.sin(angle)
            half = 0.34 * leaf_len
            corners = [(base[0] + dx * a - dy * b, base[1] + dy * a + dx * b) for a in (0.0, leaf_len) for b in (-half, half)]
            if _inside(size, corners):
                break
            leaf_len *= 0.8
        else:
            continue
        sprite.capsule(tuple(p), base, 2.2, 1.6, rgb(twig_color) * 1.1, ao=0.7)
        color = palette[int(rng.integers(0, len(palette)))] * rng.uniform(0.88, 1.1)
        ao = 0.55 + 0.45 * depth
        sprite.leaf(base, angle, leaf_len, shape_fn, color, ao, rng, fold=fold, veins=veins)
        placed += 1
    return sprite


def _needle_sprite(size, seed, colors, twig_color=0x5a4130):
    rng = np.random.default_rng(seed)
    sprite = Sprite(size)
    twigs = _branch(rng, size, 0.0, (4, 6), spread=(0.3, 0.6), side_len=(0.3, 0.45))
    for points, r0, r1 in twigs:
        for i in range(len(points) - 1):
            rr0 = r0 + (r1 - r0) * i / (len(points) - 1)
            rr1 = r0 + (r1 - r0) * (i + 1) / (len(points) - 1)
            sprite.capsule(points[i], points[i + 1], rr0, rr1, rgb(twig_color), ao=0.55, rough=0.85)
    palette = [rgb(c) for c in colors]
    needles = []
    for points, r0, r1 in twigs:
        steps = 26 if r0 > 8 else 20
        for k in range(steps * 7):
            t = rng.uniform(0.02, 1.0)
            f = t * (len(points) - 1)
            i = min(int(f), len(points) - 2)
            p = np.array(points[i]) + (np.array(points[i + 1]) - np.array(points[i])) * (f - i)
            d = np.array(points[i + 1]) - np.array(points[i])
            heading = math.atan2(d[1], d[0])
            side = 1 if k % 2 else -1
            angle = heading + side * rng.uniform(0.3, 1.2)
            length = rng.uniform(100.0, 170.0) * (0.75 + 0.25 * (1.0 - t))
            needles.append((p, angle, length, rng.uniform(0, 1)))
    needles.sort(key=lambda n: n[3])
    for p, angle, length, depth in needles:
        tip = (p[0] + math.cos(angle) * length, p[1] + math.sin(angle) * length)
        if not _inside(size, [tip], 6.0):
            scale = 0.6
            tip = (p[0] + math.cos(angle) * length * scale, p[1] + math.sin(angle) * length * scale)
            if not _inside(size, [tip], 6.0):
                continue
        color = palette[int(rng.integers(0, len(palette)))] * rng.uniform(0.85, 1.12)
        sprite.capsule(tuple(p), tip, 5.0, 2.4, color, ao=0.5 + 0.5 * depth, rough=0.6)
    return sprite


def _atlas(name, sprites):
    """Writes a 2 x 2 atlas from four sprites (top-left, top-right, bottom-left, bottom-right)."""
    size = sprites[0].size
    full = size * 2
    color = np.zeros((full, full, 3), np.float32)
    alpha = np.zeros((full, full), np.float32)
    normal = np.zeros((full, full, 3), np.float32)
    ao = np.ones((full, full), np.float32)
    rough = np.ones((full, full), np.float32)
    for index, s in enumerate(sprites):
        r, c = divmod(index, 2)
        region = (slice(r * size, (r + 1) * size), slice(c * size, (c + 1) * size))
        color[region], alpha[region], normal[region], ao[region], rough[region] = s.color, s.alpha, s.normal, s.ao, s.rough
    n8 = to8(normal * 0.5 + 0.5)
    color = color * (0.65 + 0.35 * ao)[..., None]
    save_set(name, color, None, 1.0, rough, ao=ao, alpha=alpha, normal=n8)
    _log(f'{name}: alpha coverage {alpha.mean():.2f}')


@generator('LeavesOak')
def make_leaves_oak():
    """Oak leaf clusters: stout twigs, lobed dark-green leaves (a few yellowing). Each quadrant's stem enters at the
    middle of its bottom edge."""
    colors = [0x4a6a2a, 0x557830, 0x3e5e26, 0x5e7d34, 0x6d7f36, 0x4f6f2c]
    sprites = [_leaf_sprite(1024, 300 + k, _oak_leaf, colors, 56 + 4 * k, (190.0, 290.0), sides=(5, 7)) for k in range(4)]
    _atlas('LeavesOak', sprites)


@generator('LeavesBirch')
def make_leaves_birch():
    """Birch leaf clusters: thin drooping twigs, small serrated, bright yellow-green leaves."""
    colors = [0x6f8f34, 0x5f8030, 0x86a040, 0x7a9638, 0x92a446]
    sprites = [_leaf_sprite(1024, 400 + k, _birch_leaf, colors, 130 + 8 * k, (115.0, 175.0), droop=0.6,
                            twig_color=0x4a3b30, fold=0.2, veins=10, sides=(6, 8)) for k in range(4)]
    _atlas('LeavesBirch', sprites)


@generator('NeedlesPine')
def make_needles_pine():
    """Pine sprays: brown twigs thick with long dark blue-green needles."""
    colors = [0x2f4a26, 0x3b5a2e, 0x4d6e38, 0x355232, 0x44643a]
    sprites = [_needle_sprite(1024, 500 + k, colors) for k in range(4)]
    _atlas('NeedlesPine', sprites)


PALETTE_COLORS = {
    'GrassDeep': (0x2f4a1e, 0x5e7a34), 'GrassFresh': (0x3d5a22, 0x86a048), 'GrassYellow': (0x55632a, 0xa8a85a),
    'GrassOlive': (0x4a5028, 0x8a8a4c), 'GrassDry': (0x6a6a3a, 0xb8a668), 'Straw': (0x8a7a48, 0xd6c286),
    'Clover': (0x355a26, 0x5e8a3e), 'CloverDark': (0x2a4420, 0x46683a), 'FlowerYellow': (0xc89a1a, 0xf0d040),
    'FlowerWhite': (0xc8c8b8, 0xf4f2ea), 'FlowerPurple': (0x6a3a7a, 0xa070b8), 'FlowerBlue': (0x3a5aa0, 0x7a9ad8),
    'FlowerCenter': (0x8a5a1a, 0xd8a030), 'Stem': (0x3a5226, 0x62803c), 'Fern': (0x2e4a22, 0x6a8a3a),
    'Reed': (0x6a6a3a, 0x9a8450), 'FruitRed': (0x7e2216, 0xc4402c), 'CattailBrown': (0x33211a, 0x5c3c26),
    'Moss': (0x3b4c22, 0x6c7c38), 'DryTan': (0x8c7a55, 0xc9b184),
}


@generator('FoliagePalette')
def make_foliage_palette():
    """len(PALETTE) equal swatches (swatch 0 at the bottom, see PALETTE), each a gradient from its base color (U = 0)
    to its tip color (U = 1), a little lighter along the middle of the swatch (a blade's midrib)."""
    size = 512
    rows = len(PALETTE)
    u = (np.arange(size, dtype=np.float32) + 0.5) / size
    v = 1.0 - (np.arange(size, dtype=np.float32) + 0.5) / size       # V of each pixel row, from the bottom
    index = np.minimum((v * rows).astype(np.int64), rows - 1)
    across = v * rows - index
    base = np.array([rgb(PALETTE_COLORS[name][0]) for name in PALETTE], np.float32)[index]      # (size, 3)
    tip = np.array([rgb(PALETTE_COLORS[name][1]) for name in PALETTE], np.float32)[index]
    mid = 1.0 + 0.08 * np.exp(-((across - 0.5) / 0.18) ** 2)
    t = (u ** 0.8)[None, :, None]
    color = base[:, None, :] + (tip - base)[:, None, :] * t
    fine = noise((size, size), 251, 10.0, 1.0) * 0.03
    color = color * mid[:, None, None] * (1.0 + fine)[..., None]
    ao = np.broadcast_to((0.55 + 0.45 * np.sqrt(u))[None, :], (size, size)).astype(np.float32)
    rough = np.full((size, size), 0.65, np.float32)
    height = np.zeros((size, size), np.float32)
    save_set('FoliagePalette', color, height, 1.0, rough, ao=ao)


@generator('WoodEndGrain')
def make_wood_end_grain():
    """A saw-cut log end (not tileable): growth rings around a slightly off-center pith, denser toward the bark,
    radial checks, faint saw marks, pale sapwood and a dark bark rim on the texture's inscribed circle (the corners are
    bark too). Map end caps onto it with cap_uv. Drawn for a log of about 40 cm."""
    size = 512
    shape = (size, size)
    px_m = 0.4 / size
    x, y = coords(shape)
    rng = np.random.default_rng(261)
    rim = np.hypot(x + 0.5 - size * 0.5, y + 0.5 - size * 0.5) / (size * 0.5)   # 1 on the inscribed circle
    rim = rim + 0.012 * noise(shape, 262, 20.0, 20.0)
    ex, ey = x + 0.5 - (size * 0.5 + 14.0), y + 0.5 - (size * 0.5 - 10.0)
    r = np.hypot(ex, ey) / (size * 0.5)
    ang = np.arctan2(ey, ex)
    wobble = 0.018 * noise(shape, 263, 28.0, 28.0) + 0.006 * noise(shape, 264, 6.0, 6.0)
    phase = 24.0 * np.clip(r + wobble, 0.0, 2.0) ** 0.8 + 0.5 * np.sin(3.0 * ang + 1.3) * r
    f = phase - np.floor(phase)
    late = np.exp(-((f - 0.82) / 0.08) ** 2)
    # Checks: radial cracks, wider toward the rim; one deep one from the bark inward.
    crack = np.zeros(shape, np.float32)
    for k in range(6):
        a = rng.uniform(-np.pi, np.pi)
        start, end = (0.35, 1.0) if k == 0 else (rng.uniform(0.02, 0.3), rng.uniform(0.35, 0.8))
        wiggle = a + 0.04 * noise(shape, 270 + k, 25.0, 25.0)
        d = np.abs((ang - wiggle + np.pi) % (2.0 * np.pi) - np.pi) * r * size * 0.5
        width = (4.0 if k == 0 else 1.2) * (0.3 + r)
        crack = np.maximum(crack, np.exp(-(d / width) ** 2) * smooth(start - 0.02, start + 0.04, r) * smooth(end + 0.04, end - 0.04, r))
    saw = np.sin(2.0 * np.pi * (x * 0.92 + y * 0.39) / 9.0) * smooth(-0.5, 1.0, noise(shape, 265, 60.0, 60.0))
    bark = smooth(0.92, 0.95, rim)
    cambium = smooth(0.87, 0.9, rim) * (1.0 - bark)
    sap = smooth(0.66, 0.74, rim)
    wood = mix(rgb(0xa47b57), rgb(0xc9ad88), sap)                        # warm heartwood, pale sapwood
    wood = mix(wood, rgb(0x9a8f82), smooth(0.2, 1.4, noise(shape, 266, 50.0, 50.0)) * 0.35)   # greyed by weather
    wood = mix(wood, rgb(0x6d5038), late * 0.45)
    wood = wood * (1.0 + 0.03 * saw + 0.04 * noise(shape, 267, 1.5, 1.5))[..., None]
    wood = mix(wood, rgb(0x5a4330), cambium * 0.7)
    wood = mix(wood, rgb(0x3a2a20), np.exp(-(r / 0.018) ** 2))           # the pith
    bark_n = noise(shape, 268, 5.0, 5.0, octaves=2)
    bark_color = mix(rgb(0x3e3027), rgb(0x5f4c3c), smooth(-1.0, 1.0, bark_n))
    color = mix(wood, bark_color, bark)
    color = mix(color, rgb(0x241a14), crack * 0.9)
    height = 0.0003 * late + 0.00015 * saw - 0.004 * crack + bark * (0.004 + 0.002 * bark_n) - 0.001 * cambium
    ao = occlusion(height, px_m, (2, 6, 14), 1.0)
    color = color * (0.75 + 0.25 * ao)[..., None]
    rough = 0.78 + 0.15 * bark + 0.1 * crack
    save_set('WoodEndGrain', color, height, px_m, rough, ao=ao)


@generator('MacroNoise')
def make_macro_noise():
    """Soft large-scale variation (grayscale, linear) to break up tiling: T_MacroNoise_M.png."""
    shape = (TILE, TILE)
    n = noise(shape, 241, 70.0, 70.0, octaves=5, gain=0.55)
    ranks = np.argsort(np.argsort(n.ravel())).reshape(shape).astype(np.float32) / (n.size - 1)
    value = 0.5 + 0.5 * (smooth(0.0, 1.0, ranks) * 2.0 - 1.0)
    path = os.path.join(TEXTURE_DIR, 'MacroNoise', 'T_MacroNoise_M.png')
    write_png(path, to8(value))
    write_png(os.path.join(SCRATCH_DIR, 'look_MacroNoise.png'), to8(value[::2, ::2]))
    _log('MacroNoise: 1024 x 1024 written')


# --- Previews of the library (Blender) ---

def _quad(name, size, mat, location=(0.0, 0.0, 0.0), tilt=0.0, aspect=1.0):
    """A square (or aspect-wide) plane standing in XZ, facing -Y, UVs 0..1, tilted back by tilt degrees."""
    mesh = bpy.data.meshes.new(name)
    hx, hz = size * aspect * 0.5, size * 0.5
    mesh.from_pydata([(-hx, 0.0, -hz), (hx, 0.0, -hz), (hx, 0.0, hz), (-hx, 0.0, hz)], [], [(0, 1, 2, 3)])
    uv = mesh.uv_layers.new(name='UVMap')
    for loop, value in zip(uv.data, [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]):
        loop.uv = value
    mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    obj.location = location
    obj.rotation_euler = (math.radians(-tilt), 0.0, 0.0)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def _label(text, location, size=0.16):
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = text
    curve.size = size
    curve.align_x = 'CENTER'
    obj = bpy.data.objects.new('_Label', curve)
    obj.location = location
    obj.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    mat = bpy.data.materials.get('_LabelInk') or bpy.data.materials.new('_LabelInk')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = hex_color(0xe6e2d8)
    curve.materials.append(mat)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def _flat_material(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = hex_color(color)
    bsdf.inputs['Roughness'].default_value = 1.0
    return mat


def _mask_material(name, path):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    image = nodes.new('ShaderNodeTexImage')
    image.image = _image(path, 'Non-Color')
    links.new(image.outputs['Color'], bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 1.0
    return mat


def _render_setup(scene, resolution, samples=48):
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = samples
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'


def render_contact_sheet(out_png=None):
    """Every set on a tilted, lit panel with its name: Saved/ArtPreviews/Textures/contact_sheet.png."""
    out_png = out_png or preview_path('Textures', 'contact_sheet')
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    columns, slot = 6, 2.1
    names = list(SETS)
    rows = (len(names) + columns - 1) // columns
    for index, name in enumerate(names):
        r, c = divmod(index, columns)
        x = (c - (columns - 1) * 0.5) * slot
        z = ((rows - 1) * 0.5 - r) * (slot + 0.25) + 0.12
        if name == 'MacroNoise':
            mat = _mask_material('_MacroNoise', os.path.join(TEXTURE_DIR, 'MacroNoise', 'T_MacroNoise_M.png'))
        else:
            mat = material(name)
        tilt = 0.0 if SETS[name].get('alpha') else 28.0
        panel = _quad(name, 1.75, mat, (x, 0.0, z), tilt)
        panel.visible_shadow = False
        info = SETS[name]
        _label(f"{name}  {info['size']}", (x, -0.3, z - 1.1))
    backdrop = _quad('_Backdrop', 40.0, _flat_material('_Backdrop', 0x2f3134), (0.0, 1.5, 0.0))
    backdrop.name = '_Backdrop'
    camera = bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = columns * slot + 0.3
    camera.location = (0.0, -30.0, 0.0)
    camera.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    scene.collection.objects.link(camera)
    scene.camera = camera
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = 4.0
    sun.data.color = (1.0, 0.92, 0.8)
    sun.data.angle = math.radians(2.0)
    toward = Vector((-0.55, -0.6, 0.58)).normalized()
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    scene.world = _preview_world(scene)
    height_m = rows * (slot + 0.25) + 0.2
    width_px = 2400
    _render_setup(scene, (width_px, int(width_px * height_m / (columns * slot + 0.3))))
    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    _log(f'contact sheet: {out_png}')


def _house_part(name, size, center, parent, trim, rotation=None):
    from mathutils import Euler
    bm = bmesh.new()
    bm.loops.layers.uv.new('UVMap')
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.LocRotScale(Vector(center),
                          Euler(rotation).to_quaternion() if rotation else None, Vector(size)), calc_uvs=True)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    if parent is not None:
        obj.parent = parent
    assign(obj, trim)
    return obj


def _sides(axis_sign=None):
    """A face filter: faces that aren't (nearly) horizontal."""
    return lambda f: abs(f.normal.z) < 0.7


def _tops(f):
    return abs(f.normal.z) >= 0.7


def render_trim_mockup(out_png=None):
    """A small house dressed entirely with the trim sheet (every strip), vertex AO baked, previewed:
    Saved/ArtPreviews/Textures/HouseTrim.png. It uses trim_uv, bake_vertex_ao and preview like a model script."""
    out_png = out_png or preview_path('Textures', 'HouseTrim')
    bpy.ops.wm.read_factory_settings(use_empty=True)
    trim = material('HouseTrim')
    w, d, h, base = 5.0, 3.6, 2.4, 0.5
    top = base + h
    rise = 1.4
    found = _house_part('Mockup', (w + 0.3, d + 0.3, base), (0.0, 0.0, base * 0.5), None, trim)
    trim_uv(found, _sides(), 'Stone', align='world', cut=True)
    trim_uv(found, _tops, 'Stone', align='world', cut=True)
    walls = [
        ('Front', (w, 0.16, h), (0.0, -d * 0.5, base + h * 0.5), 'Siding'),
        ('Back', (w, 0.16, h), (0.0, d * 0.5, base + h * 0.5), 'Siding'),
        ('Left', (0.16, d, h), (-w * 0.5, 0.0, base + h * 0.5), 'Logs'),
        ('Right', (0.16, d, h), (w * 0.5, 0.0, base + h * 0.5), 'Plaster'),
    ]
    for name, size, center, strip in walls:
        wall = _house_part(name, size, center, found, trim)
        trim_uv(wall, _sides(), strip, align='world', cut=True, u_offset=len(name) * 0.9)
        trim_uv(wall, _tops, 'Beams')
    # Gables: siding up to the ridge.
    for sign in (-1.0, 1.0):
        mesh = bpy.data.meshes.new('Gable')
        x0, x1 = sign * w * 0.5 - 0.08, sign * w * 0.5 + 0.08
        pts = [(x, y, z) for x in (x0, x1) for (y, z) in ((-d * 0.5, top), (d * 0.5, top), (0.0, top + rise))]
        mesh.from_pydata(pts, [], [(0, 1, 2), (5, 4, 3), (0, 3, 4, 1), (1, 4, 5, 2), (2, 5, 3, 0)])
        mesh.uv_layers.new(name='UVMap')
        gable = bpy.data.objects.new('Gable', mesh)
        bpy.context.scene.collection.objects.link(gable)
        gable.parent = found
        mesh.update()
        mesh.polygons.foreach_set('use_smooth', [False] * len(mesh.polygons))
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        bm.to_mesh(mesh)
        bm.free()
        assign(gable, trim)
        trim_uv(gable, _sides(), 'Logs' if sign < 0 else 'Plaster', align='world', cut=True)
        trim_uv(gable, _tops, 'Beams', cut=True)
    # Corner posts: hewn beams, vertical (rotated), each face a different beam face.
    for k, (sx, sy) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        post = _house_part('Post', (0.2, 0.2, h), (sx * (w * 0.5 + 0.02), sy * (d * 0.5 + 0.02), base + h * 0.5), found, trim)
        trim_uv(post, _sides(), 'Beams', rotate=True, v_offset=0.2 * (k % 4))
        trim_uv(post, _tops, 'Beams')
    # Roof: the front slope half shakes, half tin; the back all tin. Fascia boards painted teal.
    half = d * 0.5 + 0.35
    slope = math.atan2(rise, d * 0.5)
    length = half / math.cos(slope)
    for side in (-1.0, 1.0):
        for part, (x0, x1) in enumerate(((-w * 0.5 - 0.35, 0.3), (0.3, w * 0.5 + 0.35))):
            strip = 'Shingles' if side < 0 and part == 0 else 'Tin'
            cx = (x0 + x1) * 0.5
            cy = side * length * 0.5 * math.cos(slope)
            cz = top + rise - length * 0.5 * math.sin(slope) + 0.06
            slab = _house_part('Roof', (x1 - x0, length, 0.08), (cx, cy, cz), found, trim,
                               rotation=(-side * slope, 0.0, 0.0))
            trim_uv(slab, lambda f: f.normal.z > 0.3, strip, align='world', cut=True)
            trim_uv(slab, lambda f: f.normal.z <= 0.3, 'TrimTeal', fit=True)
    ridge = _house_part('Ridge', (w + 0.8, 0.22, 0.2), (0.0, 0.0, top + rise + 0.12), found, trim)
    trim_uv(ridge, None, 'Beams')
    # Door: vertical boards with iron straps, oxide-red frame.
    door_x = -1.1
    door = _house_part('Door', (0.9, 0.05, 1.95), (door_x, -d * 0.5 - 0.1, base + 0.975), found, trim)
    trim_uv(door, None, 'Siding', rotate=True, cut=True)
    for z in (base + 0.35, base + 1.6):
        strap = _house_part('Strap', (0.84, 0.02, 0.07), (door_x, -d * 0.5 - 0.135, z), found, trim)
        trim_uv(strap, None, 'Iron', fit=True)
    for dx in (-0.51, 0.51):
        jamb = _house_part('Jamb', (0.12, 0.07, 2.02), (door_x + dx, -d * 0.5 - 0.11, base + 1.01), found, trim)
        trim_uv(jamb, None, 'TrimRed', rotate=True, fit=True)
    header = _house_part('Header', (1.18, 0.08, 0.14), (door_x, -d * 0.5 - 0.11, base + 2.07), found, trim)
    trim_uv(header, None, 'TrimRed', fit=True)
    # Window: glass in a teal frame, with a sill.
    win_x, win_z = 1.15, base + 1.35
    glass = _house_part('Glass', (1.0, 0.03, 0.8), (win_x, -d * 0.5 - 0.09, win_z), found, trim)
    trim_uv(glass, None, 'Glass', fit=True)
    for dx in (-0.55, 0.0, 0.55):
        bar = _house_part('Mullion', (0.1 if dx else 0.05, 0.06, 0.9), (win_x + dx, -d * 0.5 - 0.11, win_z), found, trim)
        trim_uv(bar, None, 'TrimTeal', rotate=True, fit=True)
    for dz in (-0.45, 0.45):
        bar = _house_part('Rail', (1.2, 0.06, 0.1), (win_x, -d * 0.5 - 0.11, win_z + dz), found, trim)
        trim_uv(bar, None, 'TrimTeal', fit=True)
    sill = _house_part('Sill', (1.3, 0.16, 0.06), (win_x, -d * 0.5 - 0.15, win_z - 0.52), found, trim)
    trim_uv(sill, None, 'TrimTeal', fit=True)
    # Chimney: fieldstone through the roof.
    chimney = _house_part('Chimney', (0.7, 0.7, top + rise + 0.6), (w * 0.5 - 0.7, 0.9, (top + rise + 0.6) * 0.5), found, trim)
    trim_uv(chimney, _sides(), 'Stone', align='world', cut=True)
    trim_uv(chimney, _tops, 'Stone')
    bake_vertex_ao(found, samples=32, distance=1.2)
    preview([found], out_png, resolution=(1600, 900), view=(-1.0, -1.7, 0.55), fit=0.95)
    preview([found], out_png.replace('.png', '_Back.png'), resolution=(1280, 720), view=(1.2, 1.5, 0.6), fit=0.95)


def _trim_seed(key):
    return 1000 + list(TRIM_STRIPS).index(key) * 97


def board_joints():
    """Where the boards' butt joints are, in meters along U, from the same seeds the textures use:
    TRIM_JOINTS['A'] (siding: four boards, listed from the strip's bottom up, each a list of joints within the 6.4 m
    repeat), TRIM_JOINTS['H1'] and ['H2'] (the painted trim boards), and PLANK_JOINTS (WoodPlanks: 16 planks of 20 cm,
    from the texture's bottom up, joints within its 3.2 m repeat, at 320 px/m)."""
    px = 1.0 / TRIM_DENSITY
    siding = [[round(float(j) * px, 4) for j in joint_positions(TRIM_SIZE, (2, 4), _trim_seed('A') + 10 + b)]
              for b in range(4)][::-1]
    trims = {key: [round(float(j) * px, 4) for j in joint_positions(TRIM_SIZE, (2, 3), _trim_seed(key))]
             for key in ('H1', 'H2')}
    planks = [[round(float(j) / 320.0, 4) for j in joint_positions(TILE, (1, 2), 132 + b)] for b in range(16)][::-1]
    return dict(A=siding, **trims), planks


TRIM_JOINTS, PLANK_JOINTS = board_joints()


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    import argparse
    parser = argparse.ArgumentParser(description='Generates the texture sets into Art/Textures.')
    parser.add_argument('--only', nargs='*', default=[], help='sets to make (default: all)')
    parser.add_argument('--preview', action='store_true', help='also render the contact sheet and the trim mock-up')
    parser.add_argument('--list', action='store_true', help='list the sets')
    parser.add_argument('--no-textures', action='store_true', help='only render the previews (with --preview)')
    args = parser.parse_args(argv)
    if args.list:
        for name, info in SETS.items():
            _log(f"{name}: {info['size']} px{'' if name in GENERATORS else ' (not generated yet)'}")
        return
    names = [] if args.no_textures else (args.only or [n for n in SETS if n in GENERATORS])
    for name in names:
        if name not in GENERATORS:
            _log(f'{name}: no generator (known: {", ".join(GENERATORS)})')
            continue
        GENERATORS[name]()
    if args.preview:
        if bpy is None:
            _log('--preview needs Blender: blender -b --factory-startup --python looter_textures.py -- --preview')
            return
        render_contact_sheet()
        render_trim_mockup()


if __name__ == '__main__':
    main()
