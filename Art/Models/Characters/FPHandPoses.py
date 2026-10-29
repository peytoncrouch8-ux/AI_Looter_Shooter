"""The first-person hands' pose library (the "grip spots" job, 2026-10-29; Docs/Work/2026-10-29-GripSpots/CONTRACT.md).

A pose is how one hand holds one kind of spot on a gun part: its finger curls, thumb, and the size of the handle its
fingers close on. A marker (SOCKET_<Role>_<L|R>_<Pose>) on a held part names a pose and says where; the game reaches the
arm to the marker and dresses the hand with the pose. Poses are made once and shared by every gun.

Running it writes
    Source/AI_Looter_Shooter/Player/FirstPersonHandPoseData.inl  the poses as C++ data (generated: change them here and run
                                                                   this again)
    Saved/FPHands/poses.json                                      the same numbers, for the Blender parts sheet

    "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\ThirdParty\\Python3\\Win64\\python.exe" -I Art/Models/Characters/FPHandPoses.py
    (any plain python3 will do; it needs no Blender and no packages)

Units are those of FHandFingers (Player/FirstPersonHandsTypes.h): degrees from a straight finger, joints knuckle, middle,
end; Spread turns a finger's first joint about the palm's facing (a finger toward the index side, the thumb toward the
fingers' way, negative out). A thumb's first "curl" is its swing in across the palm. The handle numbers are cm of the
modeled gun, as FHandleSpec (HalfAcross: a grip's front to back or a fore-end's height; HalfThrough: its width).

Every pose's numbers come from the tables the markers replace (FirstPersonHandsFingers.cpp FHandFingers::Of, the Make*
tables in FirstPersonHandsGrips.cpp, FirstPersonHandsGripShapes.cpp, FirstPersonHandsCup.cpp), as they stood on 2026-10-29.
"""
import json
import os

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DATA_OUT = os.path.join(REPO, 'Source', 'AI_Looter_Shooter', 'Player', 'FirstPersonHandPoseData.inl')
JSON_OUT = os.path.join(REPO, 'Saved', 'FPHands', 'poses.json')

FINGERS = ('Thumb', 'Index', 'Middle', 'Ring', 'Pinky')
NO_SPREAD = [0.0, 0.0, 0.0, 0.0, 0.0]


def fist(knuckle, middle, tip, spread, thumb_swing, thumb_middle, thumb_tip):
    """FirstPersonHandsFingers.cpp's Fist(): the four fingers alike, each `spread` degrees more bent than the one before it
    (knuckle and middle joints only), the thumb as given. Returns [[knuckle, middle, end] x 5]."""
    curls = [[thumb_swing, thumb_middle, thumb_tip]]
    for finger in range(1, 5):
        more = spread * (finger - 1)
        curls.append([knuckle + more, middle + more, tip])
    return curls


def pose(name, curls, thumb_swing, thumb_spread, touch_share, half_across, half_through, corner, behind_knuckles, source):
    """thumb_swing and thumb_spread are the thumb's first curl and its spread: the game sets them over the grip's own
    thumb (FHandFingers::Of's), so they are what the pose holds."""
    curls = [list(c) for c in curls]
    curls[0][0] = thumb_swing
    spreads = list(NO_SPREAD)
    spreads[0] = thumb_spread
    return {
        'Name': name,
        'Curl': curls,
        'Spread': spreads,
        'ThumbSwing': thumb_swing,
        'ThumbSpread': thumb_spread,
        'TouchShare': touch_share,
        'HandleHalfAcross': half_across,
        'HandleHalfThrough': half_through,
        'HandleCorner': corner,
        'bForearmBehindKnuckles': behind_knuckles,
        'Source': source,
    }


def build_poses():
    poses = []

    # Pistol: the trigger hand round a pistol grip or a thumbhole's post. From the rifle (MakeRifle / AddContact, the
    # Bullpup): EHandGrip::Trigger = Fist(60, 80, 50, 3, thumb 28/15/10), the thumb swung in 68 (the FGunHands default
    # GripThumbSwing, which applies since the rifle's GripShape is valid) and spread 20 (the rifle's GripThumbSpread). The
    # index finger is the rifle's TriggerCurl 30/50/25 (finger on the trigger; the lie off the trigger is IndexedCurl 4/6/4
    # spread 6, which the game's trigger discipline blends in and is not part of this pose). The handle is the rifle's loop
    # post (PistolGrip 2.4 x 1.8, corner 1.0). Touch closing owns the fingers (share 1).
    curls = fist(60.0, 80.0, 50.0, 3.0, 28.0, 15.0, 10.0)
    curls[1] = [30.0, 50.0, 25.0]
    poses.append(pose('Pistol', curls, 68.0, 20.0, 1.0, 2.4, 1.8, 1.0, False,
                      'FHandFingers::Of(Trigger); rifle TriggerCurl, GripThumbSwing default 68, rifle GripThumbSpread 20; rifle GripShape 2.4/1.8/1.0'))

    # Cup: the support hand cupped under a housing, palm up, fingers up the far side, thumb along the near side. From
    # FirstPersonHandsCup.cpp OnCup: EHandGrip::Forend curls (Fist(40, 55, 35, 3.5, thumb 20/10/5)) with the thumb swing
    # CupThumbSwing 0 and spread CupThumbSpread -20 (the FGunHands defaults); the rifle's CupShape (CupBox half height 2.0,
    # half width 1.8, corner 1.0); the forearm straight back under the palm (the wrist bent back, Looter.Hands.CupWristBend).
    poses.append(pose('Cup', fist(40.0, 55.0, 35.0, 3.5, 20.0, 10.0, 5.0), 0.0, -20.0, 1.0, 2.0, 1.8, 1.0, True,
                      'FirstPersonHandsCup.cpp OnCup: Forend curls, CupThumbSwing 0 / CupThumbSpread -20, rifle CupBox 2.0/1.8/1.0, forearm behind'))

    # Pump: the support hand round a shotgun's pump. From MakeShotgun / AddContact (Ranchhand): EHandGrip::Pump = Fist(55,
    # 70, 45, 3.5, thumb 20/10/0); the thumb laid along the pump's near side (ForeThumbSwing 0, ForeThumbSpread 8); the
    # pump a 4.2 cm round tube (ForeEnd 2.1 / 2.1 / corner 2.1).
    poses.append(pose('Pump', fist(55.0, 70.0, 45.0, 3.5, 20.0, 10.0, 0.0), 0.0, 8.0, 1.0, 2.1, 2.1, 2.1, False,
                      'EHandGrip::Pump; Ranchhand ForeThumbSwing 0 / ForeThumbSpread 8, ForeEnd 2.1/2.1/2.1'))

    # Forend: the support hand under a handguard, thumb forward along its near side. From MakeRifle / AddContact (Bullpup):
    # EHandGrip::Forend = Fist(40, 55, 35, 3.5, thumb 20/10/5); ForeThumbSwing -10, ForeThumbSpread -45 ("Modern
    # tactical", tuned in PIE 2026-10-22); the handguard 6.1 tall, 5 wide, rounded 1 (ForeEnd 3.05 / 2.5 / 1.0).
    poses.append(pose('Forend', fist(40.0, 55.0, 35.0, 3.5, 20.0, 10.0, 5.0), -10.0, -45.0, 1.0, 3.05, 2.5, 1.0, False,
                      'EHandGrip::Forend; rifle ForeThumbSwing -10 / ForeThumbSpread -45, rifle ForeShape 3.05/2.5/1.0'))

    # Wrap: the hand round something thick (the reload's magazine grab). From EHandGrip::Wrap = Fist(35, 45, 30, 2, thumb
    # 25/15/10) as FirstPersonHandsReload.cpp takes it, the thumb as in the table (swing 25, no spread). The handle is a 30
    # magazine (Bullpup.py: about 7.4 front to back at its middle, a slab 2.4 wide): half 3.5 / 1.2, rounded 0.6. That box
    # is read off the model, not a tuned table, so it is the one to check first on the parts sheet.
    poses.append(pose('Wrap', fist(35.0, 45.0, 30.0, 2.0, 25.0, 15.0, 10.0), 25.0, 0.0, 1.0, 3.5, 1.2, 0.6, False,
                      'EHandGrip::Wrap (reload grab); handle sized off Bullpup.py mag(): 7.4 deep, 2.4 wide'))

    # Revolver: the support hand closed round the shooting hand on a six-gun. From MakeRevolver / AddContact (Drover):
    # EHandGrip::Support = Fist(45, 60, 40, 2, thumb 18/5/5); ForeThumbSwing 18, spread 0 (the FGunHands default); the
    # handle is the shooting hand's grip grown by its fingers (PistolGrip 4.2 / 3.4, corner 2.4), touch shared 0.7
    # (ForeContact), so the pose's own curls still show through.
    poses.append(pose('Revolver', fist(45.0, 60.0, 40.0, 2.0, 18.0, 5.0, 5.0), 18.0, 0.0, 0.7, 4.2, 3.4, 2.4, False,
                      'EHandGrip::Support; Drover ForeThumbSwing 18, ForeContact 0.7, ForeShape 4.2/3.4/2.4'))
    return poses


def cpp_float(value):
    text = ('%.4f' % value).rstrip('0').rstrip('.')
    if text in ('', '-0'):
        text = '0'
    if '.' not in text:
        text += '.'
    return text + 'f'


def cpp_row(values):
    return '{ ' + ', '.join(cpp_float(v) for v in values) + ' }'


def write_inl(poses):
    lines = [
        "// Generated by Art/Models/Characters/FPHandPoses.py; don't edit.",
        "// The first-person hands' pose library (the grip spots job, 2026-10-29): what each pose is and where its numbers came",
        '// from are in that file. Included once, by FirstPersonHandPoses.cpp, inside an anonymous namespace.',
        '',
        'struct FPoseRow',
        '{',
        '\tconst TCHAR* Name;',
        '\tfloat Curl[5][3];',
        '\tfloat Spread[5];',
        '\tfloat ThumbSwing;',
        '\tfloat ThumbSpread;',
        '\tfloat TouchShare;',
        '\tfloat HandleHalfAcross;',
        '\tfloat HandleHalfThrough;',
        '\tfloat HandleCorner;',
        '\tbool bForearmBehindKnuckles;',
        '};',
        '',
        'const FPoseRow PoseRows[] = {',
    ]
    for p in poses:
        lines.append('\t// ' + p['Name'] + ': ' + p['Source'])
        lines.append('\t{ TEXT("%s"),' % p['Name'])
        lines.append('\t\t{ ' + ', '.join(cpp_row(c) for c in p['Curl']) + ' },')
        lines.append('\t\t' + cpp_row(p['Spread']) + ',')
        lines.append('\t\t%s, %s, %s,' % (cpp_float(p['ThumbSwing']), cpp_float(p['ThumbSpread']), cpp_float(p['TouchShare'])))
        lines.append('\t\t%s, %s, %s,' % (cpp_float(p['HandleHalfAcross']), cpp_float(p['HandleHalfThrough']), cpp_float(p['HandleCorner'])))
        lines.append('\t\t%s },' % ('true' if p['bForearmBehindKnuckles'] else 'false'))
    lines.append('};')
    with open(DATA_OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


def write_json(poses):
    os.makedirs(os.path.dirname(JSON_OUT), exist_ok=True)
    out = {'fingers': list(FINGERS), 'poses': {}}
    for p in poses:
        out['poses'][p['Name']] = {k: v for k, v in p.items() if k != 'Name'}
    with open(JSON_OUT, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(out, f, indent=1)
        f.write('\n')


def main():
    poses = build_poses()
    write_inl(poses)
    write_json(poses)
    print('Wrote %d poses: %s' % (len(poses), ', '.join(p['Name'] for p in poses)))
    print(DATA_OUT)
    print(JSON_OUT)


if __name__ == '__main__':
    main()
