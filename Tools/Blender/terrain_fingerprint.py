"""Fingerprints the terrain meshes a terrain model just built, for Tools/terrain_identity.py. Run it after the model in
the same Blender:

    blender -b --factory-startup --python Art/Models/Terrain/<Area>.py --python Tools/Blender/terrain_fingerprint.py
        -- --fingerprint-out FILE --fingerprint-prefix <Area>_

It writes FILE (JSON): for every mesh object whose name starts with the prefix, its vertex and triangle counts and
hashes of its positions, faces, each UV layer, each color attribute, its normals, its object transform, its materials'
names and its custom properties (Collision, Nanite); and, under "_run", the Blender process's time and peak memory so
far. Hashing Blender's own mesh data, rather than exported FBX files (which carry a timestamp), makes two builds of
the same geometry hash the same.
"""
import ctypes
import hashlib
import json
import sys
import time

import bpy
import numpy as np


def _hash(*arrays):
    digest = hashlib.sha256()
    for a in arrays:
        digest.update(np.ascontiguousarray(a).tobytes())
    return digest.hexdigest()[:16]


def _run_stats():
    """Wall and CPU seconds since this Blender started, and its peak working set and private bytes (MB): Windows
    only, empty elsewhere."""
    if sys.platform != 'win32':
        return {}

    class Counters(ctypes.Structure):
        _fields_ = [('cb', ctypes.c_ulong), ('PageFaultCount', ctypes.c_ulong),
                    ('PeakWorkingSetSize', ctypes.c_size_t), ('WorkingSetSize', ctypes.c_size_t),
                    ('QuotaPeakPagedPoolUsage', ctypes.c_size_t), ('QuotaPagedPoolUsage', ctypes.c_size_t),
                    ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t), ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                    ('PagefileUsage', ctypes.c_size_t), ('PeakPagefileUsage', ctypes.c_size_t)]
    kernel = ctypes.WinDLL('kernel32')
    kernel.GetCurrentProcess.restype = ctypes.c_void_p
    process = kernel.GetCurrentProcess()
    counters = Counters()
    counters.cb = ctypes.sizeof(Counters)
    kernel.K32GetProcessMemoryInfo.argtypes = [ctypes.c_void_p, ctypes.POINTER(Counters), ctypes.c_ulong]
    kernel.K32GetProcessMemoryInfo(process, ctypes.byref(counters), counters.cb)
    times = [ctypes.c_ulonglong() for _ in range(4)]  # creation, exit, kernel, user (100 ns units)
    kernel.GetProcessTimes.argtypes = [ctypes.c_void_p] + [ctypes.POINTER(ctypes.c_ulonglong)] * 4
    kernel.GetProcessTimes(process, *[ctypes.byref(t) for t in times])
    now = ctypes.c_ulonglong()
    kernel.GetSystemTimeAsFileTime(ctypes.byref(now))
    return {'seconds': round((now.value - times[0].value) / 1e7, 1),
            'cpu_seconds': round((times[2].value + times[3].value) / 1e7, 1),
            'peak_working_set_mb': round(counters.PeakWorkingSetSize / 2 ** 20),
            'peak_private_mb': round(counters.PeakPagefileUsage / 2 ** 20)}


def fingerprint(obj):
    mesh = obj.data
    co = np.empty(3 * len(mesh.vertices), np.float32)
    mesh.vertices.foreach_get('co', co)
    loop_vertex = np.empty(len(mesh.loops), np.int32)
    mesh.loops.foreach_get('vertex_index', loop_vertex)
    loop_start = np.empty(len(mesh.polygons), np.int32)
    mesh.polygons.foreach_get('loop_start', loop_start)
    loop_total = np.empty(len(mesh.polygons), np.int32)
    mesh.polygons.foreach_get('loop_total', loop_total)
    normals = np.empty(3 * len(mesh.loops), np.float32)
    mesh.corner_normals.foreach_get('vector', normals)
    entry = {
        'vertices': len(mesh.vertices),
        'triangles': int((loop_total == 3).sum()),
        'polygons': len(mesh.polygons),
        'positions': _hash(co),
        'faces': _hash(loop_vertex, loop_start, loop_total),
        'normals': _hash(normals),
        'transform': _hash(np.array(obj.matrix_world, np.float32)),
        'materials': [m.name if m else None for m in mesh.materials],
        'properties': {key: str(obj[key]) for key in sorted(obj.keys())},
    }
    for layer in sorted(mesh.uv_layers, key=lambda layer: layer.name):
        uv = np.empty(2 * len(layer.data), np.float32)
        layer.data.foreach_get('uv', uv)
        entry['uv ' + layer.name] = _hash(uv)
    for attribute in sorted(mesh.color_attributes, key=lambda attribute: attribute.name):
        color = np.empty(4 * len(attribute.data), np.float32)
        attribute.data.foreach_get('color', color)
        entry['color ' + attribute.name] = _hash(color)
    return entry


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    out = argv[argv.index('--fingerprint-out') + 1]
    prefix = argv[argv.index('--fingerprint-prefix') + 1] if '--fingerprint-prefix' in argv else ''
    meshes = {obj.name: fingerprint(obj) for obj in sorted(bpy.data.objects, key=lambda o: o.name)
              if obj.type == 'MESH' and obj.name.startswith(prefix)}
    stats = _run_stats()
    with open(out, 'w', encoding='utf-8') as file:
        json.dump({'meshes': meshes, '_run': stats, '_written': time.strftime('%Y-%m-%d %H:%M:%S')}, file, indent=1)
    print(f'LOOTER: fingerprinted {len(meshes)} meshes into {out}; ' +
          ', '.join(f'{key} {value}' for key, value in stats.items()), flush=True)


if __name__ == '__main__':  # Blender's --python runs it as the main script
    main()
