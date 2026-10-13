"""Puts on the assets what the game reads to know what a bullet hit (Source/AI_Looter_Shooter/Weapons/ImpactSurface.h), so
it sounds, puffs and marks as the surface it struck:

  - Every grounded area's terrain gets its surface map. Each macro window's material instance (MI_<Area>Macro<SW|SE|NW|NE>,
    or MI_<Area>Macro where the map isn't split; layout_computed.json macroMap.windows) keeps, as asset user data, which
    ground each half meter shows: grass, the roads' and bare soil's dirt, scree and pebble bands as gravel, rock. It's
    baked from the window's macro map (its source, at full size) and the area's scatter map (its pebble bands) by
    M_Terrain's own rules (UImpactSurfaceTools.bake_terrain_map), with the instance's own GroundCues. The ring past the
    play space isn't baked: there its slope decides (rock on the steep faces).
  - Every static mesh that wears the house trim sheet (MI_HouseTrim: the buildings, the lift, the mine, the backlots, the
    boardwalk...) keeps its collision's UVs (UImpactSurfaceTools.enable_trim_lookups: UStaticMesh's "support physical
    material masks"), so a hit tells the siding and the logs from the fieldstone, the plaster, the tin roof, the iron
    straps and the window glass. It costs some memory per mesh, nothing per frame.

Run it in the open editor once the game module with UImpactSurfaceTools is built, and again whenever an area's macro or
scatter map changes, or a new model wears the trim sheet (reimporting a mesh keeps its flag):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_impact_surfaces.py [Area ...] [--no-trim]"
Areas default to every Art/Levels/<Area> with a layout_computed.json. It saves what it changes, prints an IMPACT line per
asset and ends with "IMPACT done". Looter.Impact.Probe in play says what's under the crosshair and how it was found.
"""
import json
import os
import sys

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
LEVELS = os.path.join(PROJECT, 'Art', 'Levels')
MATERIALS = '/Game/Art/Materials'
TRIM_SHEET = f'{MATERIALS}/MI_HouseTrim'
# One surface cell per half meter, at most 1024 a side (a 256 m window is 512).
CELL_CM = 50.0
MOST_CELLS = 1024
FEWEST_CELLS = 16


def log(message):
    unreal.log(f'IMPACT {message}')


def tools():
    found = getattr(unreal, 'ImpactSurfaceTools', None)
    if found is None:
        raise RuntimeError('unreal.ImpactSurfaceTools is missing: build the game module first (Tools\\launch.ps1 -Build)')
    return found


def vec2(xy):
    return unreal.Vector2D(float(xy[0]), float(xy[1]))


def asset_of(file):
    """Art/Textures/<Set>/<File>.png is imported as /Game/Art/Textures/<Set>/<File> (as build_island_scatter.py does)."""
    return '/Game/' + os.path.splitext(file.replace('\\', '/'))[0]


def load(path):
    return unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None


def areas(argv):
    named = [name for name in argv if not name.startswith('--')]
    if named:
        return named
    return sorted(name for name in os.listdir(LEVELS)
                  if os.path.isfile(os.path.join(LEVELS, name, 'layout_computed.json')))


def bake_area(area):
    """Bakes each of the area's macro windows' maps; how many it baked."""
    with open(os.path.join(LEVELS, area, 'layout_computed.json')) as source:
        macro = json.load(source).get('macroMap')
    if not macro:
        log(f'{area}: no macroMap in layout_computed.json: nothing to bake')
        return 0
    windows = macro.get('windows') or [{'material': f'{area}Macro', 'covers': macro['covers']}]
    scatter = macro.get('scatterMap')
    scatter_texture = None
    scatter_covers = [[0.0, 0.0], [0.0, 0.0]]
    if scatter:
        scatter_texture = load(asset_of(scatter['texture']))
        scatter_covers = scatter.get('covers', macro['covers'])
        if scatter_texture is None:
            log(f'{area}: the scatter map {asset_of(scatter["texture"])} isn\'t imported yet (build_island_scatter.py imports '
                'it): the macro map alone decides')
    baked = 0
    for window in windows:
        path = f'{MATERIALS}/MI_{window["material"]}'
        instance = load(path)
        if instance is None:
            log(f'{area}: {path} is missing (the terrain model imports it): skipped')
            continue
        (x0, y0), (x1, y1) = window['covers']
        size = int(max(FEWEST_CELLS, min(MOST_CELLS, round(max(x1 - x0, y1 - y0) / CELL_CM))))
        if tools().bake_terrain_map(instance, vec2((x0, y0)), vec2((x1, y1)), scatter_texture, vec2(scatter_covers[0]),
                                    vec2(scatter_covers[1]), size):
            unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
            log(f'{path}: {tools().describe_terrain_map(instance)}')
            baked += 1
        else:
            log(f'{path}: not baked (the warning above says why)')
    return baked


def class_name(data):
    path = getattr(data, 'asset_class_path', None)
    return str(path.asset_name) if path is not None else str(getattr(data, 'asset_class', ''))


def trim_meshes():
    """The static meshes that wear the trim sheet, or an instance of it: the packages that refer to it, one step through
    an area's instance."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions()
    seen, meshes = set(), []
    queue = [TRIM_SHEET]
    while queue:
        package = queue.pop()
        for referencer in registry.get_referencers(package, options) or []:
            name = str(referencer)
            if name in seen:
                continue
            seen.add(name)
            data = unreal.EditorAssetLibrary.find_asset_data(name)
            kind = class_name(data) if data else ''
            if kind == 'StaticMesh':
                meshes.append(name)
            elif kind == 'MaterialInstanceConstant' and 'HouseTrim' in name:
                queue.append(name)
    return sorted(meshes)


def enable_trims():
    changed = kept = 0
    for name in trim_meshes():
        mesh = unreal.load_asset(name)
        if not isinstance(mesh, unreal.StaticMesh):
            continue
        if tools().enable_trim_lookups(mesh):
            unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
            log(f'{name}: keeps its collision UVs now (the trim sheet\'s strips)')
            changed += 1
        elif tools().has_trim_lookups(mesh):
            kept += 1
    log(f'trim sheet: {changed} meshes set, {kept} already were')
    return changed


def run(argv):
    tools()
    for area in areas(argv):
        log(f'{area}: {bake_area(area)} terrain maps baked')
    if '--no-trim' not in argv:
        enable_trims()
    log('done')


run(sys.argv[1:])
