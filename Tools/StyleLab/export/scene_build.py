"""Style Lab export, the placements: runs the game's own level build, Tools/Unreal/build_area.py RansomsRest (the full
build: terrain, beyond, cliffs, models, panels, abutments, walkways, platforms, dressing, effects, gameplay with every
build_area_*.py story script), under mock_unreal.py, and keeps everything it spawned:

    Saved/StyleLab/work/scene_raw.json   every actor: class, label, folder, tags, its meshes' world matrices (UE cm),
                                         material overrides, instanced meshes, lights' settings, spawners' points
    Saved/StyleLab/work/build_log.txt    the build's own log and warnings

Only the light, sky and fog (build_area_environment.py) is skipped: the styles bring their own.

    /home/user/bpyenv/bin/python Tools/StyleLab/export/scene_build.py
"""
import json
import os
import sys
import time
import traceback

sys.dont_write_bytecode = True
import labcommon as lc  # noqa: E402

lc.add_paths()
lc.install_write_guard('scene')

import numpy as np  # noqa: E402

import mock_unreal  # noqa: E402

sys.modules['unreal'] = mock_unreal
sys.argv = ['build_area.py', 'RansomsRest']

import build_area  # noqa: E402

STARTED = time.time()


class _Sky:
    def recapture_sky(self):
        pass


def matrix_list(m):
    return [round(float(v), 4) for v in np.asarray(m).ravel()]


def actor_record(actor):
    rec = {'class': type(actor).__name__, 'label': actor.label, 'folder': actor.folder,
           'tags': [str(t) for t in actor.tags], 'collision': actor.collision, 'hidden': actor.hidden,
           'world': matrix_list(actor.root_component.world_matrix()), 'meshes': [], 'instances': []}
    for comp in actor.components:
        if isinstance(comp, mock_unreal.InstancedStaticMeshComponent):
            if comp.static_mesh is not None:
                base = comp.world_matrix()
                rec['instances'].append({'mesh': comp.static_mesh.model,
                                         'matrices': [matrix_list(base @ m) for m in comp.instances],
                                         'materials': {str(k): v.get_name() for k, v in comp.materials.items()
                                                       if v is not None}})
        elif isinstance(comp, mock_unreal.StaticMeshComponent):
            if comp.static_mesh is not None:
                rec['meshes'].append({'component': comp._name, 'mesh': comp.static_mesh.model,
                                      'world': matrix_list(comp.world_matrix()), 'collision': comp.collision,
                                      'materials': {str(k): v.get_name() for k, v in comp.materials.items()
                                                    if v is not None},
                                      'hidden': bool(comp.props.get('hidden_in_game', False))})
    props = {}
    for key, value in actor.props.items():
        props[key] = plain(value)
    if props:
        rec['props'] = props
    return rec


def plain(value, depth=0):
    """Editor property values as JSON (structs as dicts, vectors as lists, actors and assets by name)."""
    if depth > 4:
        return None
    if isinstance(value, (int, float, str, bool)) or value is None:
        return value
    if isinstance(value, mock_unreal.Vector):
        return [value.x, value.y, value.z]
    if isinstance(value, mock_unreal.Rotator):
        return {'roll': value.roll, 'pitch': value.pitch, 'yaw': value.yaw}
    if isinstance(value, mock_unreal.Transform):
        return {'matrix': matrix_list(value.matrix())}
    if isinstance(value, mock_unreal.LinearColor):
        return [value.r, value.g, value.b, value.a]
    if isinstance(value, (list, tuple)):
        return [plain(v, depth + 1) for v in value]
    if isinstance(value, mock_unreal.Struct):
        return {k: plain(v, depth + 1) for k, v in value.props.items()}
    if isinstance(value, mock_unreal.Actor):
        return {'actor': value.label}
    if isinstance(value, type):
        return {'class': value.__name__}
    if isinstance(value, mock_unreal.SceneComponent):
        out = {'component': value._name}
        if value.props:
            out['props'] = {k: plain(v, depth + 1) for k, v in value.props.items()}
        return out
    if hasattr(value, 'get_name'):
        try:
            return {'asset': value.get_name()}
        except Exception:  # noqa: BLE001
            return str(value)
    return str(value)


def main():
    build = build_area.AreaBuild('RansomsRest')
    build.environment = lambda: _Sky()
    failed = None
    try:
        build.run()
    except Exception:  # noqa: BLE001
        failed = traceback.format_exc()
    actors = mock_unreal.get_editor_subsystem(mock_unreal.EditorActorSubsystem).get_all_level_actors()
    raw = {'actors': [actor_record(a) for a in actors], 'failed': failed,
           'warnings': [m for k, m in mock_unreal.LOG if k != 'log']}
    lc.write_json(os.path.join(lc.WORK, 'scene_raw.json'), raw)
    with open(os.path.join(lc.WORK, 'build_log.txt'), 'w', encoding='utf-8') as f:
        for kind, message in mock_unreal.LOG:
            f.write(f'{kind.upper()}: {message}\n')
        if failed:
            f.write('FAILED:\n' + failed)
    meshes = sum(len(a['meshes']) for a in raw['actors'])
    inst = sum(len(i['matrices']) for a in raw['actors'] for i in a['instances'])
    print(f'STYLELAB: the build placed {len(actors)} actors, {meshes} meshes and {inst} instances in '
          f'{time.time() - STARTED:.0f} s; {len(raw["warnings"])} warnings' + ('; FAILED:\n' + failed if failed else ''),
          flush=True)


if __name__ == '__main__':
    main()
