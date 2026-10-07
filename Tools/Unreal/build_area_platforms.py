"""Cliff pieces kept under the platforms built over them, for build_area.py (layout.json level.cliffs.under: the placement
keys of models that stand over a cliff's top, such as Ransom's Rest's burial deck over the Rim).

A cliff piece reaches just over the wall it dresses (or its share of it), so where a model's floor stands out over that
wall's top, the piece came up through the floor: the escarpment's top course stood half a metre through the burial
deck's boards at its open end, in the middle of Abel's fight. Once the cliffs and the models stand, every cliff piece
under such a model is measured against the model's own top (traced down onto it across the piece's footprint), and one
that reaches within UNDER_CLEAR of it is cut down, its height scaled from its pivot, until its top ends that far under
the floor. One that would keep less than UNDER_KEEP of its height is left out.
"""
import re

import unreal

# How far under the platform's top a cliff piece's top ends (cm): under the boards, among the joists.
UNDER_CLEAR = 40.0
# The least share of its height a cut-down piece keeps; less, and it's left out.
UNDER_KEEP = 0.3
# The grid a piece's footprint is traced on (cm).
UNDER_STEP = 50.0
CLIFF_LABEL = re.compile(r'^Cliff_(.+?)_(\d{2})(?:_\d+)?$')


def platform_top(platform, x0, x1, y0, y1, z_from, z_to):
    """The lowest top of a platform's mesh over a box of the ground (traced down on a grid), or None where it's not over it."""
    lowest = None
    x = x0
    while x <= x1:
        y = y0
        while y <= y1:
            for comp in platform.get_components_by_class(unreal.StaticMeshComponent):
                hit = comp.line_trace_component(unreal.Vector(x, y, z_from), unreal.Vector(x, y, z_to), True, False, False)
                if hit:
                    z = (hit[0] if isinstance(hit, tuple) else hit.to_tuple()[5]).z
                    lowest = z if lowest is None else min(lowest, z)
            y += UNDER_STEP
        x += UNDER_STEP
    return lowest


def keep_under(build):
    """Cuts the cliff pieces under level.cliffs.under's models down (see the module's docstring); returns (cut, left out)."""
    keys = build.cliff_look.get('under', [])
    if not keys:
        return 0, 0
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    tag = unreal.Name(build.tag)
    level = [a for a in actors.get_all_level_actors() if tag in a.tags]
    platforms = [a for a in level if str(a.get_actor_label()) in keys]
    missing = set(keys) - {str(a.get_actor_label()) for a in platforms}
    if missing:
        build.warn(f'no {", ".join(sorted(missing))} placed for the cliffs to keep under')
    cut = left = 0
    for platform in platforms:
        p_origin, p_extent = platform.get_actor_bounds(False)
        for piece in level:
            label = str(piece.get_actor_label())
            if not CLIFF_LABEL.match(label) or not isinstance(piece, unreal.StaticMeshActor):
                continue
            origin, extent = piece.get_actor_bounds(False)
            if (abs(origin.x - p_origin.x) > extent.x + p_extent.x or abs(origin.y - p_origin.y) > extent.y + p_extent.y
                    or origin.z + extent.z < p_origin.z - p_extent.z):
                continue
            top = platform_top(platform, max(origin.x - extent.x, p_origin.x - p_extent.x),
                               min(origin.x + extent.x, p_origin.x + p_extent.x),
                               max(origin.y - extent.y, p_origin.y - p_extent.y),
                               min(origin.y + extent.y, p_origin.y + p_extent.y),
                               p_origin.z + p_extent.z + 100.0, origin.z - extent.z - 100.0)
            if top is None or origin.z + extent.z <= top - UNDER_CLEAR:
                continue
            location, scale = piece.get_actor_location(), piece.get_actor_scale3d()
            height = (origin.z + extent.z) - location.z  # the piece's reach over its pivot as placed
            share = (top - UNDER_CLEAR - location.z) / max(height, 1.0)
            if share < UNDER_KEEP:
                build.log(f'{label} left out: under {platform.get_actor_label()} it would keep {max(share, 0.0):.2f} of its height')
                actors.destroy_actor(piece)
                left += 1
                continue
            piece.set_actor_scale3d(unreal.Vector(scale.x, scale.y, scale.z * share))
            build.log(f'{label} cut to {share:.2f} of its height: its top {UNDER_CLEAR:.0f} cm under '
                      f'{platform.get_actor_label()}\'s floor')
            cut += 1
    return cut, left
