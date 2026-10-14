"""Writes the small animals' pose tables into the C++ from the tables their model scripts write, as Tools/amos_poses.py does
Amos's: the hens' (Art/Models/Creatures/Chicken.py: Intermediate/AnimalsModel/Hen_poses.json, into
Source/AI_Looter_Shooter/World/HenPoseData.inl), the rooster's (Rooster_poses.json, into World/RoosterPoseData.inl) and the
barn cat's (Art/Models/Creatures/Cat.py: Cat_poses.json, into World/CatPoseData.inl). Intermediate isn't committed, so the
game reads the committed .inl files; run this again whenever a model script's poses change:

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Chicken.py     (writes Hen_ and Rooster_poses.json)
    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Cat.py         (writes Cat_poses.json)
    "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\ThirdParty\\Python3\\Win64\\python.exe" Tools\\animal_poses.py

Any Python 3 runs it (no Blender, no editor). With a name (hen, rooster or cat) it writes that one only; a second argument
writes somewhere else (a scratch copy to compare). The tables are in Amos's format (Unreal component space at size 1, cm;
X forward, Y right, Z up; per bone: head, turn = the whole turn from rest, own = the turn under the parent's, Turned =
Above * Own). For every pose and bone it writes the own turn and the shift (how far the bone's head is from where its
parent carries it: a body lowered onto its belly in the sit), each bone's parent and its mirror (the bone of the other
side, _l and _r swapped: the walk and the scratch are one-sided, the code mirrors them). The hen's table lacks the
rooster's crow: its rows there are its rest's, so hens and the rooster share one list of poses (EChickenPose). It checks
that the bones come parents first, that every whole turn is its parent's times its own, and that the rest heads are
symmetric (a mirrored pose lands on the other side's joints); it stops when a table disagrees with itself.
"""
import json
import math
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORLD = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'World')
SOURCE = os.path.join(REPO, 'Intermediate', 'AnimalsModel')

# The poses in the order of their enums (World/AnimalPoseTables.h: EChickenPose, ECatPose).
CHICKEN_POSES = ('rest', 'alert', 'peck', 'scratch', 'walk', 'flutter_up', 'flutter_down', 'sit', 'crow')
CAT_POSES = ('stand', 'walk_a', 'walk_b', 'sit', 'loaf', 'loaf_watch', 'stretch', 'slink')

ANIMALS = {
    'hen': dict(title='The hens\'', prefix='Hen', model='Art/Models/Creatures/Chicken.py', json='Hen_poses.json',
                target='HenPoseData.inl', poses=CHICKEN_POSES, bones=24, optional=('crow',), rest='rest'),
    'rooster': dict(title='The rooster\'s', prefix='Rooster', model='Art/Models/Creatures/Chicken.py', json='Rooster_poses.json',
                    target='RoosterPoseData.inl', poses=CHICKEN_POSES, bones=24, optional=(), rest='rest'),
    'cat': dict(title='The barn cat\'s', prefix='Cat', model='Art/Models/Creatures/Cat.py', json='Cat_poses.json',
                target='CatPoseData.inl', poses=CAT_POSES, bones=29, optional=(), rest='stand'),
}

IDENTITY = (0.0, 0.0, 0.0, 1.0)


def mul(a, b):
    """FQuat a * b (b first, then a), X Y Z W."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def rotate(q, v):
    x, y, z, w = q
    r = mul(mul(q, (v[0], v[1], v[2], 0.0)), (-x, -y, -z, w))
    return r[:3]


def normalized(q):
    n = math.sqrt(sum(c * c for c in q))
    return tuple(c / n for c in q)


def angle_between(a, b):
    dot = abs(sum(x * y for x, y in zip(normalized(a), normalized(b))))
    return math.degrees(2.0 * math.acos(min(1.0, dot)))


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def mirror_name(name):
    if name.endswith('_l'):
        return name[:-2] + '_r'
    if name.endswith('_r'):
        return name[:-2] + '_l'
    return name


def solve(data, poses, optional, rest_pose, source):
    """Every pose's bones: (own, shift), each whole turn checked against its parent's times its own."""
    names = list(data['bones'])
    parents = data['parents']
    rest = {name: tuple(data['rest'][name]) for name in names}
    for k, name in enumerate(names):
        parent = parents.get(name)
        if parent is not None and (parent not in names or names.index(parent) >= k):
            sys.exit(f'{source}: {name} comes before its parent {parent} (or its parent is not a bone of the table)')
        other = mirror_name(name)
        if other not in rest:
            sys.exit(f'{source}: {name} has no mirror bone {other}')
        a, b = rest[name], rest[other]
        if abs(a[0] - b[0]) > 0.05 or abs(a[1] + b[1]) > 0.05 or abs(a[2] - b[2]) > 0.05:
            sys.exit(f'{source}: the rest pose is not symmetric at {name} {a} and {other} {b}: a mirrored pose would miss')
    table = data['poses']
    out = {}
    worst = 0.0
    filled = []
    for pose in poses:
        if pose not in table:
            if pose in optional:
                filled.append(pose)
                continue
            sys.exit(f'{source} has no {pose} pose')
        listed = table[pose]
        turn, head, rows = {}, {}, {}
        for name in names:
            bone = listed.get(name)
            parent = parents.get(name)
            own = normalized(tuple(bone['own'])) if bone else IDENTITY
            above = turn[parent] if parent else IDENTITY
            turn[name] = normalized(tuple(bone['turn'])) if bone else mul(above, own)
            worst = max(worst, angle_between(mul(above, own), turn[name]))
            carried = add(head[parent], rotate(above, sub(rest[name], rest[parent]))) if parent else rest[name]
            head[name] = tuple(bone['head']) if bone else carried
            shift = sub(head[name], carried)
            # The table's heads are to 0.01 cm: a shift under that is its rounding, not a move.
            rows[name] = (own, tuple(0.0 if abs(c) < 0.025 else c for c in shift))
        out[pose] = rows
    for pose in filled:
        # A pose this animal doesn't have stands as its rest pose (a hen asked to crow just stands).
        out[pose] = dict(out[rest_pose])
    if worst > 0.2:
        sys.exit(f'{source}: the table disagrees with itself: a whole turn is {worst:.2f} degrees off its parent\'s times its own')
    return names, parents, rest, out, worst, filled


def fmt(values, digits):
    return ', '.join(f'{v:.{digits}f}f' for v in values)


def write(key, target=None):
    spec = ANIMALS[key]
    source = os.path.join(SOURCE, spec['json'])
    target = target or os.path.join(WORLD, spec['target'])
    with open(source, encoding='utf-8') as fh:
        data = json.load(fh)
    names, parents, rest, solved, worst, filled = solve(data, spec['poses'], spec['optional'], spec['rest'], source)
    if len(names) != spec['bones']:
        sys.exit(f'{source}: {len(names)} bones, the code expects {spec["bones"]} (World/AnimalPoseTables.h)')
    p = spec['prefix']
    notes = data.get('notes', {})
    lines = [
        f'// {spec["title"]} pose table (World/AnimalPoseTables.h), generated by Tools/animal_poses.py from the table {spec["model"]}',
        f'// writes (Intermediate/AnimalsModel/{spec["json"]}). Don\'t edit: change the poses in the model script and run the',
        '// script again. Component space at size 1 (cm; X forward, Y right, Z up; the origin on the ground). Own: the bone\'s',
        '// turn under its parent\'s (FQuat X, Y, Z, W); Shift: its head off where its parent carries it. Mirror: the bone of',
        f'// the other side. Its whole turns agree with its own turns to within {worst:.3f} degrees.'
        + (f' No {", ".join(filled)} in the model\'s table: its rows are its {spec["rest"]} pose.' if filled else ''),
        '',
        f'static constexpr int32 {p}BoneCount = {len(names)};',
        f'static constexpr int32 {p}PoseCount = {len(spec["poses"])};',
        '',
        '/** The poses\' names, in the enum\'s order. */',
        f'static const TCHAR* const {p}PoseNames[{p}PoseCount] = {{ '
        + ', '.join(f'TEXT("{pose}")' for pose in spec['poses']) + ' };',
        '',
        '/** The bones, parents before children: each one\'s parent and mirror (indices into this list; -1: the root), its head at rest. */',
        f'static const FAnimalPoseBoneInfo {p}PoseBones[{p}BoneCount] = {{',
    ]
    for name in names:
        parent = parents.get(name)
        lines.append(f'\t{{ TEXT("{name}"), {names.index(parent) if parent else -1}, {names.index(mirror_name(name))}, '
                     f'{{ {fmt(rest[name], 2)} }} }},')
    lines += ['};', '', '/** Every pose, in the enum\'s order, every bone in the bones\' order. */',
              f'static const FAnimalPoseBone {p}PoseRows[{p}PoseCount][{p}BoneCount] = {{']
    for pose in spec['poses']:
        note = notes.get(pose, '')
        lines.append(f'\t// {pose}' + (f': {note}' if note else ''))
        lines.append('\t{')
        for name in names:
            own, shift = solved[pose][name]
            lines.append(f'\t\t{{ {{ {fmt(own, 5)} }}, {{ {fmt(shift, 2)} }} }}, // {name}')
        lines.append('\t},')
    lines += ['};', '']
    with open(target, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines))
    moved = [f'{pose} {name} ({", ".join(f"{c:.1f}" for c in solved[pose][name][1])})' for pose in spec['poses']
             for name in names[:1] if any(abs(c) > 0.05 for c in solved[pose][name][1])]
    print(f'ANIMALPOSES {key}: {len(spec["poses"])} poses x {len(names)} bones written to {os.path.relpath(target, REPO)} '
          f'(turns agree to {worst:.3f} degrees)' + (f'; {", ".join(filled)} filled with the {spec["rest"]} pose' if filled else ''))
    if moved:
        print(f'ANIMALPOSES {key} root shifts: ' + '; '.join(moved))


def main():
    keys = [sys.argv[1]] if len(sys.argv) > 1 and sys.argv[1] in ANIMALS else list(ANIMALS)
    target = sys.argv[2] if len(sys.argv) > 2 and len(keys) == 1 else None
    for key in keys:
        write(key, target)


if __name__ == '__main__':
    main()
