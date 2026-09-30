"""Fits every hill to the terrain again.

A hill (the Hill prop, baked to Props/Hill/SM_Hill_*) is a mound of polar rings with a skirt straight down from its
rim. The generator (StylizedProp.cpp, BuildHill) gives each vertex, a fraction f of the way out:
  z = H (1 - f^2)^2 + lumps (1 - C) + ground C - 40 f,    C = smoothstep(0.3, 1, f)
so its outer part follows the ground under it and the rim sits 40 cm under that ground, where the skirt never shows.
If the terrain changes after the hill is baked (or the hill moves), the rim stands above the ground in places: the
skirt shows as a step, and grass hangs off it. This rebuilds the fitted part of each hill mesh in place from the same
formula and the terrain as it is now. The middle (f up to 0.3) is left exactly as it was; H comes from the hilltop,
and the lumps from the last ring before the fit starts, which keeps the new part continuous with the old middle.

Run it in the open editor, then generate the Meadow volume again, run Looter.SettleProps and save the level:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/conform_hills.py"
Add 'dry' after the path to only report how far each hill is off.
"""
import math
import sys

import unreal

# From the generator, in the mesh's own space.
TUCK = 40.0
SKIRT = 600.0
FIT_START = 0.3
SMOOTH_ANGLE = 50.0

WORLD_STATIC = [unreal.ObjectTypeQuery.ECC_WORLD_STATIC]
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def terrain_below(point):
    """The terrain under a point (world Z), or None: the island's ground, not hills or the cliffs standing on it."""
    hits = unreal.SystemLibrary.line_trace_multi_for_objects(world, point + unreal.Vector(0, 0, 2000), point - unreal.Vector(0, 0, 2000),
                                                            WORLD_STATIC, True, [], unreal.DrawDebugTrace.NONE, True)
    for hit in hits or []:
        fields = hit.to_tuple()
        actor, component = fields[9], fields[10]
        mesh = component.static_mesh.get_name() if isinstance(component, unreal.StaticMeshComponent) and component.static_mesh else ''
        if actor and actor.actor_has_tag('Ground') and not mesh.startswith(('SM_Hill_', 'SM_Cliff_')):
            return fields[5].z
    return None


def smoothstep(edge0, edge1, x):
    t = min(1.0, max(0.0, (x - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def conform(actor, dry):
    mesh = actor.static_mesh_component.static_mesh
    dm = unreal.DynamicMesh()
    source = unreal.GeometryScriptMeshReadLOD()
    source.set_editor_property('lod_type', unreal.GeometryScriptLODType.SOURCE_MODEL)
    unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh, dm, unreal.GeometryScriptCopyMeshFromAssetOptions(), source)
    _, vertex_list, gaps = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(dm, False)
    if gaps:
        unreal.log_warning(f'HILLS {actor.get_actor_label()}: the mesh has vertex ID gaps; skipped.')
        return
    points = list(unreal.GeometryScript_List.convert_vector_list_to_array(vertex_list))

    # The rim and the skirt's bottom share X/Y, a skirt apart; the hilltop and the cap's center share (0, 0).
    by_xy = {}
    for vertex, p in enumerate(points):
        by_xy.setdefault((round(p.x, 1), round(p.y, 1)), []).append(vertex)
    rim_to_bottom = {}
    for (x, y), vertices in by_xy.items():
        if len(vertices) == 2 and math.hypot(x, y) > 1.0:
            upper, lower = sorted(vertices, key=lambda v: -points[v].z)
            if points[upper].z - points[lower].z > SKIRT * 0.5:
                rim_to_bottom[upper] = lower
    bottoms = set(rim_to_bottom.values())
    middle = sorted(by_xy.get((0.0, 0.0), []), key=lambda v: -points[v].z)
    if len(rim_to_bottom) < 8 or len(middle) != 2:
        unreal.log_warning(f'HILLS {actor.get_actor_label()}: not shaped like a generated hill; skipped.')
        return
    top, cap_center = middle
    height = points[top].z

    # Every other vertex lies on the ray from the middle through one rim point, a fraction of the way out.
    rims = [(rim, points[rim].x, points[rim].y) for rim in rim_to_bottom]
    ray_of, fraction_of = {}, {}
    for vertex, p in enumerate(points):
        if vertex in bottoms or vertex in (top, cap_center):
            continue
        length = math.hypot(p.x, p.y)
        rim, rx, ry = max(rims, key=lambda r: (p.x * r[1] + p.y * r[2]) / (length * math.hypot(r[1], r[2])))
        ray_of[vertex] = rim
        fraction_of[vertex] = (p.x * rx + p.y * ry) / (rx * rx + ry * ry)

    # Each ray's lumps, from its last vertex before the fit starts (the part that doesn't depend on the ground).
    anchor = {}
    for vertex, rim in ray_of.items():
        f = fraction_of[vertex]
        if f <= FIT_START and f > anchor.get(rim, (-1.0, 0.0))[0]:
            anchor[rim] = (f, points[vertex].z - height * (1 - f * f) ** 2 + TUCK * f)

    xf = actor.get_actor_transform()
    base_z = xf.translation.z
    scale_z = xf.scale3d.z
    moved = list(points)
    for vertex, rim in ray_of.items():
        f = fraction_of[vertex]
        start, lumps = anchor.get(rim, (1.0, 0.0))
        if f <= start:
            continue
        ground = terrain_below(xf.transform_location(points[vertex]))
        if ground is None:
            continue
        fit = smoothstep(FIT_START, 1.0, f)
        z = height * (1 - f * f) ** 2 + lumps * (1 - fit) + (ground - base_z) / scale_z * fit - TUCK * f
        moved[vertex] = unreal.Vector(points[vertex].x, points[vertex].y, z)
    for rim, bottom in rim_to_bottom.items():
        moved[bottom] = unreal.Vector(points[bottom].x, points[bottom].y, moved[rim].z - SKIRT)
    c = points[cap_center]
    moved[cap_center] = unreal.Vector(c.x, c.y, sum(moved[b].z for b in bottoms) / len(bottoms))

    # How far the rim stood above the ground, and how much the fit changes (world cm).
    above = []
    for rim in rim_to_bottom:
        ground = terrain_below(xf.transform_location(points[rim]))
        if ground is not None:
            above.append(base_z + points[rim].z * scale_z - ground)
    change = [abs(moved[v].z - points[v].z) * scale_z for v in range(len(points))]
    unreal.log(f'HILLS {actor.get_actor_label()}: rim {sum(1 for d in above if d > 0)} of {len(above)} points above the ground '
               f'(highest {max(above):.0f} cm); refit moves vertices up to {max(change):.0f} cm')
    if dry:
        return

    for vertex, p in enumerate(moved):
        unreal.GeometryScript_MeshEdits.set_vertex_position(dm, vertex, p)
    split = unreal.GeometryScriptSplitNormalsOptions()
    split.set_editor_property('split_by_opening_angle', True)
    split.set_editor_property('opening_angle_deg', SMOOTH_ANGLE)
    unreal.GeometryScript_Normals.compute_split_normals(dm, split, unreal.GeometryScriptCalculateNormalsOptions())
    options = unreal.GeometryScriptCopyMeshToAssetOptions()
    options.set_editor_property('enable_recompute_normals', False)
    options.set_editor_property('enable_recompute_tangents', True)
    unreal.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dm, mesh, options, unreal.GeometryScriptMeshWriteLOD())
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)


dry = 'dry' in sys.argv
for level_actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    component = level_actor.get_component_by_class(unreal.StaticMeshComponent) if isinstance(level_actor, unreal.StaticMeshActor) else None
    if component and component.static_mesh and component.static_mesh.get_name().startswith('SM_Hill_'):
        conform(level_actor, dry)
unreal.log('HILLS done')
