"""Writes Abel's pose table into the C++ (Source/AI_Looter_Shooter/Bosses/AbelPoseData.inl) from the table the art session's
model script writes (Art/Models/Creatures/Abel.py: Intermediate/AbelModel/Abel_poses.json). Intermediate isn't committed,
so the game reads the committed .inl; run this again whenever Abel.py's poses change:

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Abel.py     (writes the JSON)
    "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\ThirdParty\\Python3\\Win64\\python.exe" Tools\\abel_poses.py

Any Python 3 runs it (no Blender, no editor). For every pose (idle, flare, fire, lunge, sunset, kneel, sit) and every one of
the 39 bones it writes the bone's own turn under its parent's (AUnpaidCreature's Turned = Above * Own) and its shift: how far
its head is from where its parent carries it (the pelvis in the lunge, the kneel and the sit, the shroud's tail_01 and the
coat's side bones in the kneel and the sit, and the pump laid on the boards in the kneel). A bone a pose doesn't list stays at
rest under its parent. It also writes where the pump's muzzle is in the shot and where the lantern's globe is in the flare,
and checks that every bone's whole turn is its parent's times its own (it stops when the table disagrees with itself).
"""
import json
import math
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(REPO, 'Intermediate', 'AbelModel', 'Abel_poses.json')
TARGET = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'Bosses', 'AbelPoseData.inl')

# The poses in EAbelPose's order (Bosses/AbelPoses.h).
POSES = ('idle', 'flare', 'fire', 'lunge', 'sunset', 'kneel', 'sit')

# Each bone's parent in SK_Abel (Abel.py's bone_specs); the armature's root is above the pelvis and never posed.
PARENTS = {
    'pelvis': None, 'spine_01': 'pelvis', 'spine_02': 'spine_01', 'coal': 'spine_02', 'neck': 'spine_02', 'head': 'neck',
    'jaw': 'head', 'hat': 'head',
    'upperarm_l': 'spine_02', 'lowerarm_l': 'upperarm_l', 'hand_l': 'lowerarm_l', 'lantern': 'hand_l',
    'upperarm_r': 'spine_02', 'lowerarm_r': 'upperarm_r', 'hand_r': 'lowerarm_r', 'gun': 'hand_r',
    'tail_01': 'pelvis', 'tail_02': 'tail_01', 'tail_03': 'tail_02', 'tail_04': 'tail_03', 'tail_05': 'tail_04',
    'tail_l_01': 'tail_02', 'tail_l_02': 'tail_l_01', 'tail_r_01': 'tail_02', 'tail_r_02': 'tail_r_01',
    'skirt_f_01': 'pelvis', 'skirt_f_02': 'skirt_f_01', 'skirt_l_01': 'pelvis', 'skirt_r_01': 'pelvis',
}
for side in ('l', 'r'):
    for finger in ('thumb', 'index', 'middle', 'ring', 'pinky'):
        PARENTS[f'{finger}_01_{side}'] = f'hand_{side}'

# The pump's muzzle in its own space (SM_AbelPump's SOCKET_Muzzle, cm), and the lantern's globe in the lantern's
# (SM_AbelLantern's SOCKET_Light: the globe's middle under the bail's grip).
MUZZLE = (75.98, -4.56, -27.81)
GLOBE = (0.0, 0.0, (0.134 - 0.427) * 100.0)

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
            turn[name] = normalized(tuple(bone['turn'])) if bone else (turn[parent] if parent else IDENTITY)
            own = normalized(tuple(bone['own'])) if bone else IDENTITY
            above = turn[parent] if parent else IDENTITY
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


def fmt(values, digits):
    return ', '.join(f'{v:.{digits}f}f' for v in values)


def main():
    source = sys.argv[1] if len(sys.argv) > 1 else SOURCE
    with open(source, encoding='utf-8') as fh:
        data = json.load(fh)
    table = data['poses']
    for pose in POSES:
        if pose not in table:
            sys.exit(f'{source} has no {pose} pose')
    rest, solved, worst = solve(table)
    names = list(PARENTS)

    fire = solved['fire']
    muzzle = add(fire['head']['gun'], rotate(fire['turn']['gun'], MUZZLE))
    flare = solved['flare']
    globe = add(flare['head']['lantern'], rotate(flare['turn']['lantern'], GLOBE))
    kneel = solved['kneel']

    lines = [
        '// Abel\'s pose table (Bosses/AbelPoses.h), generated by Tools/abel_poses.py from the table Art/Models/Creatures/Abel.py',
        '// writes (Intermediate/AbelModel/Abel_poses.json). Don\'t edit: change the poses in Abel.py and run the script again.',
        '// Component space at size 1 (cm; X forward, Y right, Z up). Own: the bone\'s turn under its parent\'s (FQuat X, Y, Z, W);',
        '// Shift: its head off where its parent carries it. Its whole turns agree with its own turns to within '
        f'{worst:.3f} degrees.',
        '',
        f'static constexpr int32 GeneratedBoneCount = {len(names)};',
        '',
        '/** The bones, parents before children, and each one\'s parent (an index into this list; -1: the root). */',
        'static const FAbelPoseBoneInfo GeneratedBones[GeneratedBoneCount] = {',
    ]
    for name in names:
        parent = PARENTS[name]
        lines.append(f'\t{{ TEXT("{name}"), {names.index(parent) if parent else -1}, {{ {fmt(rest[name], 2)} }} }},')
    lines += ['};', '', '/** Every pose, in EAbelPose\'s order, every bone in GeneratedBones\' order. */',
              'static const FAbelPoseBone GeneratedPoses[static_cast<int32>(EAbelPose::Count)][GeneratedBoneCount] = {']
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
        '/** Where the pump\'s muzzle is in the shot (SM_AbelPump\'s SOCKET_Muzzle), and the lantern\'s globe in the flare. */',
        f'static const FVector3f GeneratedFireMuzzle(' + fmt(muzzle, 2) + ');',
        f'static const FVector3f GeneratedFlareGlobe(' + fmt(globe, 2) + ');',
        '',
        '/** Where the kneel lays the pump on the boards: the gun bone\'s head and whole turn (FQuat X, Y, Z, W). */',
        f'static const FVector3f GeneratedKneelGun(' + fmt(kneel['head']['gun'], 2) + ');',
        f'static const FQuat4f GeneratedKneelGunTurn(' + fmt(kneel['turn']['gun'], 5) + ');',
        '',
    ]
    with open(TARGET, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines))
    shifted = [f'{pose} {name} ({", ".join(f"{c:.1f}" for c in solved[pose]["rows"][name][1])})'
               for pose in POSES for name in names if any(abs(c) > 0.05 for c in solved[pose]['rows'][name][1])
               and name in ('pelvis', 'tail_01', 'skirt_l_01', 'skirt_r_01', 'gun')]
    print(f'ABELPOSES {len(POSES)} poses x {len(names)} bones written to {os.path.relpath(TARGET, REPO)} '
          f'(turns agree to {worst:.3f} degrees)')
    print('ABELPOSES shifts: ' + '; '.join(shifted))
    print(f'ABELPOSES muzzle in the shot ({fmt(muzzle, 1)}), globe in the flare ({fmt(globe, 1)}), pump on the boards '
          f'({fmt(kneel["head"]["gun"], 2)}) turned ({fmt(kneel["turn"]["gun"], 5)})')


if __name__ == '__main__':
    main()
