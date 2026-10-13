"""Writes the friendly dead's pose tables into the C++ from the tables their model scripts write, as Tools/amos_poses.py does
Amos's: Hiram Coyle's (Art/Models/Creatures/Hiram.py: Intermediate/HiramModel/Hiram_poses.json, into
Source/AI_Looter_Shooter/Story/HiramPoseData.inl) and Thaddeus Bright's (Art/Models/Creatures/Thaddeus.py:
Intermediate/ThaddeusModel/Thaddeus_poses.json, into Story/ThaddeusPoseData.inl). Intermediate isn't committed, so the game
reads the committed .inl files; run this again whenever a model script's poses change:

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Hiram.py      (writes its JSON)
    "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\ThirdParty\\Python3\\Win64\\python.exe" Tools\\dead_friend_poses.py

Any Python 3 runs it (no Blender, no editor). With a name (hiram or thaddeus) it writes that one only; a second argument
writes somewhere else (a scratch copy to compare). For every pose and every one of the 35 bones (the Unpaid's, and Abel's
front skirt chain skirt_f_01 and skirt_f_02) it writes the bone's own turn under its parent's (the rig's Turned = Above *
Own) and its shift: how far its head is from where its parent carries it. A bone a pose doesn't list stays at rest under its
parent. Each model's REST POSE is its idle (Hiram sat on his stool, Thaddeus stooped measuring at full stretch), so the
idle row is all identity. It checks that every bone's whole turn is its parent's times its own (it stops when a table
disagrees with itself).
"""
import json
import math
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STORY = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'Story')

# Each character: its table, its .inl, and its poses in its enum's order (Story/HiramPoses.h, Story/ThaddeusPoses.h).
CHARACTERS = {
    'hiram': dict(
        title='Hiram\'s', model='Art/Models/Creatures/Hiram.py', enum='EHiramPose', header='Story/HiramPoses.h',
        source=os.path.join(REPO, 'Intermediate', 'HiramModel', 'Hiram_poses.json'),
        target=os.path.join(STORY, 'HiramPoseData.inl'),
        poses=('idle', 'tube', 'tube_left', 'talk')),
    'thaddeus': dict(
        title='Thaddeus\'s', model='Art/Models/Creatures/Thaddeus.py', enum='EThaddeusPose', header='Story/ThaddeusPoses.h',
        source=os.path.join(REPO, 'Intermediate', 'ThaddeusModel', 'Thaddeus_poses.json'),
        target=os.path.join(STORY, 'ThaddeusPoseData.inl'),
        poses=('idle', 'together', 'mark', 'knock_up', 'knock_down', 'look', 'talk')),
}

# Each bone's parent in SK_Hiram and SK_Thaddeus (the Unpaid's bones and Abel's front skirt chain). Parents come before
# their children; the armature's root is above the pelvis and never posed.
PARENTS = {
    'pelvis': None, 'spine_01': 'pelvis', 'spine_02': 'spine_01', 'coal': 'spine_02', 'neck': 'spine_02', 'head': 'neck',
    'jaw': 'head', 'hat': 'head',
    'upperarm_l': 'spine_02', 'lowerarm_l': 'upperarm_l', 'hand_l': 'lowerarm_l',
    'upperarm_r': 'spine_02', 'lowerarm_r': 'upperarm_r', 'hand_r': 'lowerarm_r',
    'tail_01': 'pelvis', 'tail_02': 'tail_01', 'tail_03': 'tail_02', 'tail_04': 'tail_03', 'tail_05': 'tail_04',
    'tail_l_01': 'tail_02', 'tail_l_02': 'tail_l_01', 'tail_r_01': 'tail_02', 'tail_r_02': 'tail_r_01',
    'skirt_f_01': 'pelvis', 'skirt_f_02': 'skirt_f_01',
}
for side in ('l', 'r'):
    for finger in ('thumb', 'index', 'middle', 'ring', 'pinky'):
        PARENTS[f'{finger}_01_{side}'] = f'hand_{side}'

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


def solve(table, poses, source):
    """Every pose's bones: (own, shift), with turn and head checked against the parent's."""
    rest = {name: tuple(bone['rest']) for name, bone in table['idle'].items()}
    missing = sorted(set(PARENTS) - set(rest))
    extra = sorted(set(rest) - set(PARENTS))
    if missing or extra:
        sys.exit(f'{source}: the table and the rig disagree: missing {missing}, unknown {extra}')
    out = {}
    worst = 0.0
    for pose in poses:
        listed = table[pose]
        turn, head, rows = {}, {}, {}
        for name in PARENTS:
            bone = listed.get(name)
            parent = PARENTS[name]
            own = normalized(tuple(bone['own'])) if bone else IDENTITY
            above = turn[parent] if parent else IDENTITY
            turn[name] = normalized(tuple(bone['turn'])) if bone else mul(above, own)
            # The whole turn must be the parent's times the bone's own.
            worst = max(worst, angle_between(mul(above, own), turn[name]))
            carried = add(head[parent], rotate(above, sub(rest[name], rest[parent]))) if parent else rest[name]
            head[name] = tuple(bone['head']) if bone else carried
            shift = sub(head[name], carried)
            # The table's heads are to 0.01 cm: a shift under that is its rounding, not a move.
            rows[name] = (own, tuple(0.0 if abs(c) < 0.025 else c for c in shift))
        out[pose] = rows
    if worst > 0.2:
        sys.exit(f'{source}: the table disagrees with itself: a whole turn is {worst:.2f} degrees off its parent\'s times its own')
    return rest, out, worst


def fmt(values, digits):
    return ', '.join(f'{v:.{digits}f}f' for v in values)


def write(key, target=None):
    spec = CHARACTERS[key]
    source = spec['source']
    target = target or spec['target']
    with open(source, encoding='utf-8') as fh:
        data = json.load(fh)
    table = data['poses']
    for pose in spec['poses']:
        if pose not in table:
            sys.exit(f'{source} has no {pose} pose')
    rest, solved, worst = solve(table, spec['poses'], source)
    names = list(PARENTS)
    data_file = os.path.basename(spec['target'])
    lines = [
        f'// {spec["title"]} pose table ({spec["header"]}), generated by Tools/dead_friend_poses.py from the table {spec["model"]}',
        f'// writes ({os.path.relpath(source, REPO).replace(os.sep, "/")}). Don\'t edit: change the poses in the model script and run the',
        '// script again. Component space at size 1 (cm; X forward, Y right, Z up). Own: the bone\'s turn under its parent\'s',
        '// (FQuat X, Y, Z, W); Shift: its head off where its parent carries it. The rest pose is the idle. Its whole turns agree',
        f'// with its own turns to within {worst:.3f} degrees. ({data_file})',
        '',
        f'static constexpr int32 GeneratedBoneCount = {len(names)};',
        '',
        '/** The bones, parents before children, and each one\'s parent (an index into this list; -1: the root). */',
        'static const FDeadFriendPoseBoneInfo GeneratedBones[GeneratedBoneCount] = {',
    ]
    for name in names:
        parent = PARENTS[name]
        lines.append(f'\t{{ TEXT("{name}"), {names.index(parent) if parent else -1}, {{ {fmt(rest[name], 2)} }} }},')
    lines += ['};', '', f'/** Every pose, in {spec["enum"]}\'s order, every bone in GeneratedBones\' order. */',
              f'static const FDeadFriendPoseBone GeneratedPoses[static_cast<int32>({spec["enum"]}::Count)][GeneratedBoneCount] = {{']
    for pose in spec['poses']:
        lines.append(f'\t// {pose}')
        lines.append('\t{')
        for name in names:
            own, shift = solved[pose][name]
            lines.append(f'\t\t{{ {{ {fmt(own, 5)} }}, {{ {fmt(shift, 2)} }} }}, // {name}')
        lines.append('\t},')
    lines += ['};', '']
    with open(target, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines))
    shifted = [f'{pose} {name} ({", ".join(f"{c:.1f}" for c in solved[pose][name][1])})'
               for pose in spec['poses'] for name in names if any(abs(c) > 0.05 for c in solved[pose][name][1])
               and name in ('pelvis', 'tail_01', 'skirt_f_01')]
    print(f'FRIENDPOSES {key}: {len(spec["poses"])} poses x {len(names)} bones written to {os.path.relpath(target, REPO)} '
          f'(turns agree to {worst:.3f} degrees)')
    if shifted:
        print(f'FRIENDPOSES {key} shifts: ' + '; '.join(shifted))


def main():
    wanted = [sys.argv[1]] if len(sys.argv) > 1 else list(CHARACTERS)
    for key in wanted:
        if key not in CHARACTERS:
            sys.exit(f'no such character {key}: one of {", ".join(CHARACTERS)}')
        write(key, sys.argv[2] if len(sys.argv) > 2 else None)


if __name__ == '__main__':
    main()
