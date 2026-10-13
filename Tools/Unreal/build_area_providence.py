"""Back Pay's props at the Providence (Docs/Areas/RansomsRest_Expansion.md, section 4 "Back Pay", section 6 "The
Providence"), for build_area_missionprops_mountain.py, whose helpers (when, step, readable, item, fitting) it uses: the
tag board at the portal (from Art/Models/Buildings/Diggings.py's board frame on the portal's transform, the mine's origin)
and the mine's mission props (from Art/Models/Mine/Mine.layout.json's sockets, in the mine's frame: build_area_mine.py).
Everything goes in the area's Gameplay/Mountain folder (the company safe in Gameplay, as every chest).

- J. Odell's tag (AMissionArtifact, tagged Artifact_MinersTag: "No. 7. J. ODELL. IN.") on IN's free twelfth hook, taken
  when Back Pay is; the shift's voice (a speaker point tagged Speaker_TagBoard, missions only, "The shift") at the board's
  face: Back Pay's turn-in and its choice.
- The payroll book (PayrollBook) on the office's SOCKET_PayrollBook, the chalk (ChalkTally) on the gallery's west wall,
  the barricade (Barricade: reading it takes the eleven's tags, which build_area_mine.py's story prop hides) on its
  SOCKET_Barricade.
- The payroll chest: the mission item Payroll (its model the missions' data's SM_PayrollChest), tagged PayrollChest too,
  on the stope's SOCKET_PayrollChest from the Foreman's step until Back Pay is turned in.
- The company safe (a Strongbox, CompanySafe: two guns at Luck 1.0) on the office's SOCKET_CompanySafe.
- The manway's barred gate (an AMissionFitting that swings open, solid) at its place in the gate passage, lifted from the
  Foreman's step on (or open for good after Back Pay): the way back up from the stope.
"""
import math

import unreal

import build_area_story as story

# The tag board (Diggings.py): its frame on the portal's (metres: the board's middle on the ground, turned 25 degrees to
# face the approach), J. Odell's tag on IN's free twelfth hook (the eleven's fill the first eleven), and the shift's voice
# at the board's face.
BOARD = (-4.3, -4.5)
BOARD_TURN = 25.0
ODELL_HOOK = (-0.17, -0.045, 1.345)
VOICE_AT = (0.0, -0.3, 1.4)
# Barricade.py's nail when the placed barricade has no socket: 0.4 m west of its middle, 0.3 m in front, 1.37 m up.
NAIL = (-0.4, -0.3, 1.37)


def on_board(mine, x, y, z):
    """A spot in the tag board's frame (metres: x along its face, y into it, z up) as a world spot."""
    turn = math.radians(BOARD_TURN)
    bx = BOARD[0] + x * math.cos(turn) - y * math.sin(turn)
    by = BOARD[1] + x * math.sin(turn) + y * math.cos(turn)
    return mine.at(bx, by, z)


def board_facing(mine):
    """The way the tag board's face looks (its frame's -y turned BOARD_TURN), as a world yaw."""
    out = math.radians(BOARD_TURN)
    return mine.turn(math.degrees(math.atan2(math.sin(out), -math.cos(out))))


def place_tag_board(build, mine, props):
    artifact_cls = story.actor_class('MissionArtifact')
    speaker_cls = story.actor_class('SpeakerPoint')
    facing = board_facing(mine)
    if artifact_cls is not None:
        tag = build.place(artifact_cls, on_board(mine, *ODELL_HOOK), facing, label='Artifact_MinersTag', folder=props.FOLDER,
                          tags=('Artifact_MinersTag',))
        # Its words are the missions' data's (story_texts.py ARTIFACTS: the card shows Back Pay's briefing); the design's
        # stand in while that isn't there.
        words = props.texts()
        spec = getattr(words, 'ARTIFACTS', {}).get('Artifact_MinersTag') if words is not None else None
        if spec is None:
            spec = dict(title="A miner's brass tag", prompt='Take the tag', take_on_accept=True)
            tag.set_editor_property('text', unreal.Text('No. 7. J. ODELL. IN.'))
        tag.set_editor_property('title', unreal.Text(spec['title']))
        tag.set_editor_property('prompt', unreal.Text(spec['prompt']))
        tag.set_editor_property('take_on_accept', bool(spec.get('take_on_accept', True)))
        model = props.mesh(build, 'SM_MinersTag')
        if model is not None:
            tag.get_editor_property('mesh').set_static_mesh(model)
        else:
            build.warn('no SM_MinersTag yet (the props agent\'s): J. Odell\'s tag offers Back Pay unseen')
    else:
        build.warn('no MissionArtifact class (build the game module first): nothing offers Back Pay')
    if speaker_cls is not None:
        voice = story.speaker(build, speaker_cls, on_board(mine, *VOICE_AT), facing, 'Speaker_TagBoard', 'Speaker_TagBoard',
                              name='The shift', prompt='Hang the tags', reach=props.TALK_REACH)
        voice.set_folder_path(f'{build.folder}/{props.FOLDER}')
        point = voice.get_editor_property('speaker_point')
        # Missions only (Back Pay's turn-in and its choice); after a kept payroll, the shift's word (story_texts.py TOPICS).
        point.set_editor_property('missions_only', True)
        props.speak(build, point, 'Speaker_TagBoard')
    build.log('the tag board: J. Odell\'s tag (Artifact_MinersTag) on IN, the shift\'s voice (Speaker_TagBoard)')


def mine_actors(build):
    """What build_area_mine.py placed in the mine (the Mountain/Mine folder)."""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return [a for a in actors.get_all_level_actors() if unreal.Name(build.tag) in a.tags
            and str(a.get_folder_path()).endswith('Mountain/Mine')]


def place_barricade(build, mine, layout, readable_cls, props):
    """Reading the barricade takes the eleven's tags off its nail (the barricade's SOCKET_Barricade, else the table's)."""
    placed = [a for a in mine_actors(build) if str(a.get_actor_label()).startswith('Mine_Barricade_')]
    spot = story.socket(placed[0], 'Barricade') if placed else None
    if spot is not None:
        location, yaw = props.xyz(spot.translation), spot.rotation.rotator().yaw
    else:
        piece = next((p for p in layout.get('pieces', []) if p['asset'] == 'SM_RR_Mine_Barricade'), None)
        if piece is None:
            build.warn('no barricade in the mine table: the eleven\'s tags can\'t be taken')
            return
        x, y, z = piece['mine_m']
        location, yaw = mine.at(x + NAIL[0], y + NAIL[1], z + NAIL[2]), mine.socket_yaw(0.0)
        if not placed:
            build.warn('no barricade placed in the mine (build_area_mountain.py first): its readable stands at the table\'s nail')
    props.readable(build, readable_cls, 'Barricade', location, yaw)


def place_payroll(build, mine, chest_spot, item_cls, props):
    """The payroll: the mission item Payroll, whose model is the payroll chest (the missions' data gives it SM_PayrollChest),
    on the stope's SOCKET_PayrollChest while Back Pay is under way (story_texts.py ITEMS says from when; without it, from
    the Foreman's step); taken, it's carried. Tagged PayrollChest too, the contract's name for the chest."""
    if item_cls is None:
        return
    (x, y, z), yaw = chest_spot
    location, turn = mine.at(x, y, z), mine.socket_yaw(yaw)
    foreman = props.step(build, props.BACK_PAY, 'foreman')
    payroll = props.item(build, item_cls, 'Payroll', location, turn, 'SM_PayrollChest',
                         shown_when=props.when(during=props.BACK_PAY, from_step=foreman))
    payroll.set_editor_property('tags', list(payroll.tags) + [unreal.Name('PayrollChest')])
    if props.mesh(build, 'SM_PayrollChest') is None:
        build.warn('no SM_PayrollChest yet (the props agent\'s): the payroll shows its item\'s stand-in')
    build.log('the payroll chest (item Payroll, tagged PayrollChest) in the stope')


def hob_spots(build):
    """Hob's perch at the Providence (story_texts.py PERCHES' mineMouth, "on the Providence's tag board as Back Pay begins"),
    for build_area_story.place_hob's perches as the chapel's and the Sink's modules give theirs: {'mineMouth': (x, y, z,
    yaw)} on the tag board's little roof, facing the approach."""
    import build_area_mine as mine_kit
    mine = mine_kit.frame(build, quiet=True)
    x, y, z = on_board(mine, 0.0, 0.05, 2.45)
    return {'mineMouth': (x, y, z, board_facing(mine))}


def place_safe(build, mine, layout):
    import build_area_caches as caches
    import build_area_mine as mine_kit
    safe = mine_kit.socket(layout, 'Office', 'CompanySafe')
    chest_cls = story.actor_class('Chest')
    if safe is None:
        build.warn('the mine table has no Office SOCKET_CompanySafe: no company safe')
        return
    models = caches.models(build, 'Strongbox') if chest_cls is not None else None
    if models is None:
        if chest_cls is None:
            build.warn('no Chest class (build the game module first): no company safe')
        return
    (x, y, z), yaw = safe
    caches.place_chest(build, chest_cls, 'Strongbox', models, 'CompanySafe', mine.at(x, y, z), mine.socket_yaw(yaw),
                       ('CompanySafe',))
    build.log('the company safe (a Strongbox, CompanySafe) in the Foreman\'s office')


def place_gate(build, mine, layout, props):
    """The manway's barred gate, lifted from below from the Foreman's step on (or open for good after Back Pay)."""
    import build_area_mine as mine_kit
    piece = next((p for p in layout.get('pieces', []) if p['asset'] == 'SM_' + mine_kit.GATE_PIECE), None)
    model = props.mesh(build, 'SM_' + mine_kit.GATE_PIECE)
    if piece is None or model is None:
        build.warn('no manway gate in the mine table, or no SM_RR_Mine_ManwayGate: the manway stands open')
        return
    location = mine.offset(*piece['unreal_cm'])
    yaw = mine.turn(piece['unreal_rotator_pitch_yaw_roll'][1])
    cls = story.actor_class('MissionFitting')
    if cls is None:
        build.place(model, location, yaw, label='ManwayGate', folder=props.FOLDER, tags=(mine_kit.INSIDE_TAG,))
        build.warn('no MissionFitting class (build the game module first): the manway\'s gate stands shut for good')
        return
    gate = props.fitting(build, cls, location, yaw, 'ManwayGate', ('ManwayGate',), 'Lift the bar',
                         props.when(during=props.BACK_PAY, from_step=props.step(build, props.BACK_PAY, 'foreman')),
                         [props.when(after=[props.BACK_PAY])], model=model, hide_until_done=False, hold=1.0,
                         cue=props.CUES['Gate'])
    # The leaf hangs on its hinge (its pivot) along the passage's width; open, it lies back along the passage toward the
    # stope (a quarter turn and a little): look at it swing in play, and flip the sign if it goes into the rock.
    gate.set_editor_property('effect', unreal.InteractablePropEffect.SWING)
    gate.set_editor_property('open_angle', -95.0)
    gate.set_editor_property('swing_seconds', 0.9)
    gate.get_editor_property('mesh').set_collision_profile_name('BlockAll')
    build.log('the manway\'s barred gate, lifted from below from the Foreman\'s step on')


def place(build, mine, readable_cls, item_cls, props):
    """The tag board's and the mine's props (build_area_missionprops_mountain.py's place() calls this)."""
    import build_area_mine as mine_kit
    place_tag_board(build, mine, props)
    layout = mine_kit.read_layout()
    if readable_cls is not None:
        for readable_id, piece, socket_name in (('PayrollBook', 'Office', 'PayrollBook'),
                                                ('ChalkTally', 'Gallery', 'ChalkTally')):
            spot = mine_kit.socket(layout, piece, socket_name)
            if spot is None:
                build.warn(f'the mine table has no {piece} SOCKET_{socket_name}: no {readable_id}')
                continue
            (x, y, z), yaw = spot
            props.readable(build, readable_cls, readable_id, mine.at(x, y, z), mine.socket_yaw(yaw))
        place_barricade(build, mine, layout, readable_cls, props)
    chest_spot = mine_kit.socket(layout, 'Stope', 'PayrollChest')
    if chest_spot is not None:
        place_payroll(build, mine, chest_spot, item_cls, props)
    else:
        build.warn('the mine table has no Stope SOCKET_PayrollChest: no payroll chest')
    place_safe(build, mine, layout)
    place_gate(build, mine, layout, props)
