"""A grounded area's bounds in its level, for build_area.py (step 3d of Docs/Areas/RansomsRest.md):

- the playable area (APlayableArea) from layout_computed.json's "boundary": its corners with their ground heights, and
  which edges are open (a drop that is part of play). The game builds its walls behind the closed edges as it starts.
- the world's KillZ, about 100 m under the lowest ground (a grounded area's canyon floor), so stray creatures and drops
  are cleaned up there.
- one CullDistanceVolume over the core, culling small things by their size: layout.json level.cullDistances lists
  [up to this size, culled past this distance] in cm, smallest first; anything larger is never culled.

An island (the tutorial island) has no boundary: it keeps the engine's KillZ and gets neither actor.
"""
import unreal

PLAYABLE_AREA = '/Script/AI_Looter_Shooter.PlayableArea'
# How far under the lowest ground the KillZ lies (cm).
KILL_MARGIN = 10000.0
# The doc's table when the layout gives none: up to 0.5 m culled at 40 m, up to 2 m at 90 m, up to 6 m at 200 m.
CULL_DISTANCES = ((50.0, 4000.0), (200.0, 9000.0), (600.0, 20000.0))
# How far the cull volume reaches above and below the core's ground range (cm).
CULL_HEADROOM = 5000.0


def cull_pairs(thresholds):
    """The volume's (size, distance) pairs for "up to size: distance" thresholds, smallest first.

    A CullDistanceVolume gives each primitive the distance of the pair whose size is nearest its bounds' diameter, so
    each threshold has to fall halfway between two pair sizes: the first pair sits at 0, and each next one as far past
    a threshold as the one before sits short of it. The last pair has distance 0 (never culled) for anything larger."""
    pairs, size = [], 0.0
    for limit, distance in thresholds:
        pairs.append((size, distance))
        size = 2.0 * limit - size
        if size <= limit:
            raise ValueError(f'cull distance sizes must grow: {thresholds}')
    pairs.append((size, 0.0))
    return pairs


def lowest_ground(source, layout):
    """The lowest ground there is (cm): the core's lowest point, or a grounded area's canyon floor below the
    escarpment's lip (its drop and the floor's noise, under the lowest boundary corner)."""
    low, _ = layout.get('heightRange', (0.0, 0.0))
    escarpment = source.get('region', {}).get('escarpment')
    corners = layout.get('boundary', {}).get('corners', [])
    if escarpment and corners:
        floor = min(c[2] for c in corners) - escarpment['drop'] - escarpment.get('floorNoise', {}).get('amplitude', 0.0)
        low = min(low, floor)
    return low


def place(build):
    """Places the playable area and the cull volume (tagged as built, in the Bounds folder) and sets the KillZ."""
    layout, source = build.layout, build.source
    boundary = layout.get('boundary')
    if not boundary:
        return
    area = build.place(unreal.load_class(None, PLAYABLE_AREA), (0, 0, 0), label='PlayableArea', folder='Bounds')
    area.set_editor_property('corners', [unreal.Vector(*c) for c in boundary['corners']])
    area.set_editor_property('open_edges', [bool(o) for o in boundary['openEdges']])
    # The walls stand at the rock's foot rather than on the line a few metres before it, so a player walking out stops
    # against rock, not in the air (level.wallSetback, cm; the line stays where the map has it).
    setback = build.source.get('level', {}).get('wallSetback', 0.0)
    area.set_editor_property('wall_setback', float(setback))
    build.log(f"playable area: {len(boundary['corners'])} corners, {sum(boundary['openEdges'])} open edges, "
              f"walls {setback / 100.0:.1f} m behind the line")

    world = unreal.EditorLevelLibrary.get_editor_world()
    settings = world.get_world_settings()
    kill_z = lowest_ground(source, layout) - KILL_MARGIN
    settings.set_editor_property('kill_z', kill_z)
    build.log(f'KillZ at {kill_z / 100.0:.0f} m')

    (x0, y0), (x1, y1) = layout.get('region', {}).get('core', [[-10240.0, -10240.0], [10240.0, 10240.0]])
    low, high = layout.get('heightRange', (0.0, 0.0))
    center = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (low + high) / 2.0)
    # The brush is 200 cm across.
    scale = ((x1 - x0) / 200.0, (y1 - y0) / 200.0, (high - low + 2.0 * CULL_HEADROOM) / 200.0)
    volume = build.place(unreal.CullDistanceVolume, center, label='CullDistances', folder='Bounds', scale=scale)
    thresholds = build.source.get('level', {}).get('cullDistances', CULL_DISTANCES)
    pairs = cull_pairs([tuple(t) for t in thresholds])
    entries = []
    for size, distance in pairs:
        entry = unreal.CullDistanceSizePair()
        entry.set_editor_property('size', size)
        entry.set_editor_property('cull_distance', distance)
        entries.append(entry)
    volume.set_editor_property('cull_distances', entries)
    build.log('cull distances: ' + ', '.join(f'{s / 100.0:g} m -> {d / 100.0:g} m' if d else f'{s / 100.0:g} m -> never'
                                             for s, d in pairs))
