"""Writes Amos's pose table into the C++ (Source/AI_Looter_Shooter/Story/AmosPoseData.inl) from the table the art session's
model script writes (Art/Models/Creatures/Amos.py: Intermediate/AmosModel/Amos_poses.json). Intermediate isn't committed,
so the game reads the committed .inl; run this again whenever Amos.py's poses change:

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Amos.py     (writes the JSON)
    "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\ThirdParty\\Python3\\Win64\\python.exe" Tools\\amos_poses.py

Any Python 3 runs it (no Blender, no editor); a second argument writes somewhere else (a scratch copy to compare). For
every pose (idle, lean, sit, talk) and every one of the 36 bones it writes the bone's own turn under its parent's
(AUnpaidCreature's Turned = Above * Own) and its shift: how far its head is from where its parent carries it (the pelvis
lifted onto the rail in the sit; the shroud's tail_01 riding the knees in the lean and the sit). A bone a pose doesn't list
stays at rest under its parent. From the table's notes it also writes where he stands for each fence pose (how far behind
the fence's line he leans, how far he turns to his right sitting on the rail) and his speaker point from his head, and it
checks that every bone's whole turn is its parent's times its own (it stops when the table disagrees with itself).
"""
import json
import math
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(REPO, 'Intermediate', 'AmosModel', 'Amos_poses.json')
TARGET = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'Story', 'AmosPoseData.inl')

# The poses in EAmosPose's order (Story/AmosPoses.h).
POSES = ('idle', 'lean', 'sit', 'talk')

# Each bone's parent in SK_Amos (Amos.py's bone_specs): the Unpaid's bones, Abel's front skirt chain and the fork under
# the right hand. Parents come before their children; the armature's root is above the pelvis and never posed.
PARENTS = {
    'pelvis': None, 'spine_01': 'pelvis', 'spine_02': 'spine_01', 'coal': 'spine_02', 'neck': 'spine_02', 'head': 'neck',
    'jaw': 'head', 'hat': 'head',
    'upperarm_l': 'spine_02', 'lowerarm_l': 'upperarm_l', 'hand_l': 'lowerarm_l',
    'upperarm_r': 'spine_02', 'lowerarm_r': 'upperarm_r', 'hand_r': 'lowerarm_r', 'fork': 'hand_r',
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


def solve(table):
    """Every pose's bones: (own, shift), with turn and head checked against the parent's."""
    rest = {name: tuple(bone['rest']) for name, bone in table['idle'].items()}
    missing = sorted(set(PARENTS) - set(rest))
    extra = sorted(set(rest) - set(PARENTS))
    if missing or extra:
        sys.exit(f'the table and the rig disagree: missing {missing}, unknown {extra}')
    out = {}
    worst = 0.0
    for pose in POSES:
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
        out[pose] = dict(rows=rows, turn=turn, head=head)
    if worst > 0.2:
        sys.exit(f'the table disagrees with itself: a whole turn is {worst:.2f} degrees off its parent\'s times its own')
    return rest, out, worst


def number_after(text, pattern, what):
    """A number the table's notes give (they're written from Amos.py's constants), or stop."""
    found = re.search(pattern, text)
    if not found:
        sys.exit(f'the table\'s notes no longer say {what} (looked for {pattern!r}): read Amos.py and update this script')
    return found


def fmt(values, digits):
    return ', '.join(f'{v:.{digits}f}f' for v in values)


def main():
    source = sys.argv[1] if len(sys.argv) > 1 else SOURCE
    target = sys.argv[2] if len(sys.argv) > 2 else TARGET
    with open(source, encoding='utf-8') as fh:
        data = json.load(fh)
    table = data['poses']
    for pose in POSES:
        if pose not in table:
            sys.exit(f'{source} has no {pose} pose')
    rest, solved, worst = solve(table)
    names = list(PARENTS)

    # Where he stands for each fence pose, and his speaker point (SOCKET_Speaker on the hat), from the notes.
    lean_back = float(number_after(data['lean'], r'origin on the ground ([0-9.]+) cm behind the fence', 'how far behind '
                                   'the fence\'s line he leans').group(1))
    sit_turn = float(number_after(data['sit'], r'turned ([0-9.]+) degrees to his right', 'how far he turns to his right '
                                  'on the rail').group(1))
    speaker = number_after(data['speaker'], r'\(([-0-9.]+), ([-0-9.]+), ([-0-9.]+)\) cm from the head bone', 'where the '
                           'speaker point is from his head')
    from_head = tuple(float(speaker.group(k)) for k in (1, 2, 3))

    lines = [
        '// Amos\'s pose table (Story/AmosPoses.h), generated by Tools/amos_poses.py from the table Art/Models/Creatures/Amos.py',
        '// writes (Intermediate/AmosModel/Amos_poses.json). Don\'t edit: change the poses in Amos.py and run the script again.',
        '// Component space at size 1 (cm; X forward, Y right, Z up). Own: the bone\'s turn under its parent\'s (FQuat X, Y, Z, W);',
        '// Shift: its head off where its parent carries it. Its whole turns agree with its own turns to within '
        f'{worst:.3f} degrees.',
        '',
        f'static constexpr int32 GeneratedBoneCount = {len(names)};',
        '',
        '/** The bones, parents before children, and each one\'s parent (an index into this list; -1: the root). */',
        'static const FAmosPoseBoneInfo GeneratedBones[GeneratedBoneCount] = {',
    ]
    for name in names:
        parent = PARENTS[name]
        lines.append(f'\t{{ TEXT("{name}"), {names.index(parent) if parent else -1}, {{ {fmt(rest[name], 2)} }} }},')
    lines += ['};', '', '/** Every pose, in EAmosPose\'s order, every bone in GeneratedBones\' order. */',
              'static const FAmosPoseBone GeneratedPoses[static_cast<int32>(EAmosPose::Count)][GeneratedBoneCount] = {']
    for pose in POSES:
        lines.append(f'\t// {pose}')
        lines.append('\t{')
        for name in names:
            own, shift = solved[pose]['rows'][name]
            lines.append(f'\t\t{{ {{ {fmt(own, 5)} }}, {{ {fmt(shift, 2)} }} }}, // {name}')
        lines.append('\t},')
    lines += [
        '};',
        '',
        '/** Leaning on the fence, his origin stands this far behind the fence\'s line (cm), square to it and facing it. */',
        f'static constexpr float GeneratedLeanBack = {lean_back:.2f}f;',
        '/** Sitting on it, his origin is on the line, turned this far to his right from square (degrees). */',
        f'static constexpr float GeneratedSitTurn = {sit_turn:.2f}f;',
        '/** SOCKET_Speaker (on SM_AmosHat, at his mouth) from the head bone\'s head at rest (cm). */',
        f'static const FVector3f GeneratedSpeakerFromHead({fmt(from_head, 2)});',
        '',
    ]
    with open(target, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines))
    shifted = [f'{pose} {name} ({", ".join(f"{c:.1f}" for c in solved[pose]["rows"][name][1])})'
               for pose in POSES for name in names if any(abs(c) > 0.05 for c in solved[pose]['rows'][name][1])
               and name in ('pelvis', 'tail_01', 'fork')]
    print(f'AMOSPOSES {len(POSES)} poses x {len(names)} bones written to {os.path.relpath(target, REPO)} '
          f'(turns agree to {worst:.3f} degrees)')
    print('AMOSPOSES shifts: ' + '; '.join(shifted))
    print(f'AMOSPOSES lean {lean_back:.0f} cm behind the fence, sit turned {sit_turn:.0f} degrees right, speaker '
          f'({fmt(from_head, 1)}) from the head')


if __name__ == '__main__':
    main()
