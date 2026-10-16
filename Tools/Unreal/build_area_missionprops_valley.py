"""The valley's mission pieces on Ransom's Rest (Docs/Areas/RansomsRest_Expansion.md, sections 3, 4 and 9; the shared names
of the wave 4 contract), for build_area.py's gameplay pass: who talks, what's read, what's carried and where the new fights
stand in the town, on the sockets of the art (Art/Models/Buildings/DepotRR.py, WorkshopBright.py, BarnWhitlock.py,
FalseFronts.py, Chapel.py, Farmhouse.py; Art/Models/Rocks/Outcrops.py's Den Rock), so they follow the level whenever it's
rebuilt. Everything goes in the area's Gameplay/Valley folder, which a gameplay build clears with the rest of Gameplay.
Where a building or the dressing already has its own copy of a thing (the orders slate, the wages chalk, the den's hat),
the piece brings no model of its own: it's read where the copy is. A mission item always shows a model (its own, its
data's, else a stand-in), so a thing to take must not be baked into a building (Tilly's cellar key on the measuring hook:
the art takes its baked copy out; the tape on the hook stays, Thaddeus's own goes on his bench).

The Old Fare, at the depot (the station actor's SM_RR_Depot):
- Mr. Sloane through his door upstairs (ASpeakerPoint Speaker_Agent on SOCKET_AgentDoor): The Old Fare's giver; only while
  he has business (a mission to offer or turn in here, or a topic for now), then his ticket window (Speaker_AgentWindow on
  SOCKET_TicketWindow) once The Old Fare is done: the coal in Dead Lift, the line to the Lily after Main 7.
- Josiah Fenn in his coffin (one ASpeakerPoint wearing SM_FennCoffin on SOCKET_BaggageCoffin, tagged Speaker_Fenn and
  FennCoffin): talked to, it's both a talk (Speaker_Fenn: the turn-in) and an interaction (FennCoffin: "look at Fenn", "lay
  the coins back"), each a tap. Its lid (SM_FennCoffin_Lid on SOCKET_Lid, two AStoryProps) lies shut until the look at
  Fenn, stands open until the turn-in and shuts after; his fare (SM_GraveCoins on SOCKET_Eyes, an AStoryProp) is back on
  his eyes once the coins are laid back.
- The agent's desk (AReadableProp AgentDesk on SOCKET_AgentDesk: Ned Purcell's marker), Fenn's fare (AMissionItem FennsFare,
  the two coins on the landing outside the agent's door, from the step that asks for them), the baggage room's place
  (Place_BaggageRoom) and the waiting room's fight (AEncounterSpawner DepotWaiting: three Unpaid, a Restless one and a
  Bell-Ringer standing where the dead wait, SOCKET_Wait_1..4 and the platform; on during The Old Fare until it's cleared).
Dead Lift: the coal sack under the coal stage's chute (AMissionItem CoalSack on SOCKET_Sack, also tagged CoalStage: "Fill a
  coal sack", a tap), the winch crank in the sheriff's cell (WinchCrank) and Sheriff Odell's report beside it (OdellReport).
Knocking Below, at Tilly's workshop (SM_RR_Workshop_Bright): the workshop door's place (Place_WorkshopDoor), the cellar key
  on the measuring hook (CellarKey), the orders slate (OrdersSlate), the cellar door locked until the key is carried
  (ALockedDoor CellarDoor on SOCKET_CellarDoor, SM_RR_CellarDoor), the cellar floor's place (Place_CellarFloor), the
  cellar's fight (AEncounterSpawner TillyCellar: three Unpaid and a Restless one by the coffins, level 3, its ground the
  cellar alone), the cold room's door (AInteractableProp ColdRoomDoor on SOCKET_ColdRoomDoor), Thaddeus Bright at his bench
  (AThaddeusBright on SOCKET_ThaddeusBench, Speaker_Thaddeus) and his tape on the bench (ThaddeusTape, after his talk).
Calder's Caches: Ruth's note on the Rangers' board offers it (AMissionArtifact Artifact_CalderNote over the note's paper,
  tagged CalderNote too: it wins the key's focus over the paper while it offers, and the board's screen offers the job
  from it as well, being pinned on its face), a cache tag by each of the three caches (CacheTag1 the windmill's, 2 the Sink
  rim's, 3 the bluff path's) and the
  Deacon's slip by the gang's Strongbox (DeaconsSlip). The board itself (NoticeBoard_Rangers) is build_area_board.py's,
  from the layout's gameplay.boards.
Side 1: the town gate's poster offers it (AMissionArtifact Artifact_PosterGate over the poster's paper); the sheriff's
  daybook on his desk (OdellDaybook).
Main 1: Delia's porch plate (APorchPlate on the farmhouse's SOCKET_Plate, in place of the plain plate build_area_farm.py
  stands there). Main 2: the boards where it happened (LookoutStains, on the lookout's stained boards, with the stain's decal). Main 3: Pa's lantern bail on
  Tilly's sill (LanternBail, after Main 3). Main 4: the keepers' roll in the nave (KeepersRoll). Main 5 and Side 3: the
  sheriff's hat by the den's mouth (SheriffHat, on the den dressing's hat) and his badge in the den (SheriffBadge).
  Main 6: Keeper's Bridge's place (Place_KeepersBridge, on the grounded deck: build_area_town_rr.py).
Main Street's safe ground gets a height band (ASafeGround), so nothing under the street is ever safe.

When a piece shows follows the missions' data (Tools/Unreal/create_mission_assets.py, create_side_mission_assets.py): a
step is found in DA_Mission_<Id> by the objective that asks for it (a talk to Speaker_Agent, a Clear of TillyCellar...),
else the design's step is used and the fallback is logged. Every word is a line set of /Game/Data/Story when
create_story_lines.py has made it (the names in LINES), else the design's own words (Docs/Areas/RansomsRest_Expansion.md),
written here; the readables' texts are here. Missing art, a missing socket or class: that piece warns and is left out.

Hook in build_area.py's gameplay(), after self.posters.place(self) (the posters, the caches, the station and the story's
pieces must stand):
    importlib.reload(importlib.import_module('build_area_missionprops_valley')).place(self)
On its own, in the open editor (clears Gameplay/Valley, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_missionprops_valley.py RansomsRest"
"""
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Valley'
CLASSES = '/Script/AI_Looter_Shooter.'
MISSIONS = '/Game/Data/Missions/DA_Mission_'
ENGINE_CUBE = '/Engine/BasicShapes/Cube'

# The missions, by id (the contract's names).
MAIN1, MAIN2, MAIN3, MAIN4, MAIN5, MAIN7 = 'Main1', 'Main2', 'Main3', 'Main4', 'Main5', 'Main7'
SIDE1, SIDE3 = 'Side1', 'Side3'
CALDERS = 'CaldersCaches'
OLD_FARE = 'OldFare'
KNOCKING = 'KnockingBelow'
DEAD_LIFT = 'DeadLift'

# Who says what (speaker names on the captions).
ELLIS = 'Ellis'
HOB = 'Hob'
SLOANE = 'Mr. Sloane'
FENN = 'Josiah Fenn'
THADDEUS = 'Thaddeus Bright'
DELIA = 'Grandma Delia'


def say(*pairs):
    """Lines as (speaker, words) pairs."""
    return list(pairs)


# The speakers' topics: (when, the line set create_story_lines.py makes, the design's words while it isn't made). "when" is
# filled in at placing (it needs the missions' steps).
LINES = {
    'AgentDoor': ('DA_Lines_AgentDoor', say(
        (SLOANE, "Are they gone? I can still hear them breathing down there."))),
    'AgentCoins': ('DA_Lines_AgentCoins', say(
        (SLOANE, "His people laid gold on his eyes, like the old days. Gold, on a dead man!"),
        (SLOANE, "I only borrowed it. Against a debt."))),
    'AgentWindow': ('DA_Lines_AgentWindow', say(
        (SLOANE, "Waiting room's quiet. First night in a week I've slept."))),
    'AgentWindowCoal': ('DA_Lines_AgentWindowCoal', say(
        (SLOANE, "That's railroad coal you're after!"),
        (SLOANE, "...Oh, take it. Nothing's running."))),
    'AgentWindowLily': ('DA_Lines_AgentWindowLily', say(
        (SLOANE, "Line's open to the Lily, kid. Your car's coupled."),
        (SLOANE, "Highwater Landing, end of the line. The Lily's tied up there, over the canyon."),
        (SLOANE, "Anywhere else, you change at Halfway Junction. Once somebody clears it."))),
    'FennLook': ('DA_Lines_FennLook', say(
        (HOB, "Somebody's been at his fare, sunshine. Rob a dead man's eyes and every soul for a mile hears about it."))),
    'FennCoins': ('DA_Lines_FennCoins', say(
        (ELLIS, "Your fare, Mr. Fenn."))),
    'ThaddeusWaiting': ('DA_Lines_ThaddeusWaiting', say(
        (THADDEUS, "Shut that door behind you. They get out when it's open."))),
    'ThaddeusTalk': ('DA_Lines_ThaddeusTalk', say(
        (THADDEUS, "Ransom's child. I measured you, you know. You've stood up since."),
        (THADDEUS, "I kept them in their boxes a week. They got past me when the door opened. A cellar wants keeping."),
        (THADDEUS, "Give her my tape. Tell her the brass was too much. And that I liked it."))),
    'ThaddeusAfter': ('DA_Lines_ThaddeusAfter', say(
        (THADDEUS, "A cellar wants keeping. I'll keep it."))),
}

# The readables: id -> (title, heading, text, lines after, prompt, models to try in order). The design's words.
READABLES = {
    'LookoutStains': ("The boards where it happened", "", "", say(
        (ELLIS, "He was standing right here."),
        (HOB, "So were you, sunshine. Briefly.")), "Look", ()),
    'KeepersRoll': ("The keepers' roll", "Painted on a plank in the nave",
                    "KEEPERS OF THE LANTERN\nSAINT ADA'S, RANSOM'S REST\n\nSILAS RANSOM\nHANNAH RANSOM\nJONAS RANSOM\n"
                    "ABEL RANSOM\n\nUnder Abel's name a line has been painted out in white. It still shows through: ELLIS.",
                    say((HOB, "Heir to the family business, sunshine. Funny how that goes.")), "Read", ('KeepersRoll',)),
    'SheriffHat': ("A lawman's hat", "By the den's mouth", "Chewed through the crown. The band reads W. ODELL.", say(
        (HOB, "Whatever lives in that hole had the sheriff for supper.")), "Look", ()),
    'SheriffBadge': ("Sheriff Odell's badge", "In the webbing", "A tin star bent nearly double. SHERIFF, it says, and under "
                     "that, RANSOM'S REST.", say(), "Look", ('SheriffBadge',)),
    'OdellReport': ("Sheriff Odell's report", "On the fall at the Providence",
                    "Eleven men. Teague swears the roof gave on its own. Then why'd he have powder burns on his cuffs?\n\n"
                    "My brother Jory was one of the eleven.\n\n— W.O.", say(), "Read", ('OdellReport', 'DeaconsSlip')),
    'OdellDaybook': ("The sheriff's daybook", "Open on his desk. The last entry is a month old.",
                     "Gone down the Sink after the big one. Back by supper.\n\n— W.O.", say(), "Read",
                     ('OdellDaybook', 'PostLog', 'Journal')),
    'WagesChalk': ("Chalked inside the barn door", "In Amos Whitlock's hand",
                   "OWED\n\nMERRITT   14.00\nMERRITT   14.00\nTANNER    11.50", say(), "Read", ()),
    'OrdersSlate': ("Coffin orders", "Chalked on the workshop's slate",
                    "DOLAN   PINE   6FT\nAVERY   PINE   5FT 5\nPECK   PINE   5FT 10\nLOCKETT   OAK   5FT 2\n\n"
                    "Under them, struck through in another hand: T. BRIGHT. PINE. NO BRASS.",
                    say((HOB, "Wrote his own order, did he. And she put brass on it anyway.")), "Read", ()),
    'AgentDesk': ("A gambling marker", "In the agent's desk",
                  "THE GILDED LILY\nI.O.U. FORTY DOLLARS, PAYABLE TO N. PURCELL ON DEMAND.\nH. SLOANE",
                  say((ELLIS, "Ned's paper."), (HOB, "Small world.")), "Read", ('DeaconsSlip',)),
    'DeaconsSlip': ("A slip in the Deacon's hand", "In the gang's strongbox, with a fold of banknotes",
                    "Kid's share. For when the kid comes home.\n\n— A.D.",
                    say((HOB, "...Sentimental, for a man who shot you.")), "Read", ('DeaconsSlip',)),
}

# The mission items: id -> (models to try, what the prompt calls it).
ITEMS = {
    'LanternBail': (('LanternBail',), "Pa's lantern bail"),
    'WinchCrank': (('WinchCrank',), "the winch crank"),
    'CoalSack': (('CoalSack',), "a coal sack"),
    'CellarKey': (('CellarKey',), "the cellar key"),
    'ThaddeusTape': (('TapeMeasure',), "Thaddeus's tape"),
    'FennsFare': (('GraveCoins',), "Fenn's fare"),
    'CacheTag1': (('CacheTag',), "a cache tag"),
    'CacheTag2': (('CacheTag',), "a cache tag"),
    'CacheTag3': (('CacheTag',), "a cache tag"),
}

# --- Where things stand ---
# The sheriff's office (SM_FalseFront_Sheriff, FalseFronts.py: its frame, cm; +X out of its front). The cell behind the
# partition is X 44..154, Y 80..220 from the pivot, its bunk against the back; the desk with its lamp (SOCKET_Light at
# 252, -207). Spots are traced down onto the model from TRACE_TOP over the floor.
CELL_CRANK = (66.0, 168.0)
CELL_REPORT = (70.0, 112.0)
DESK_BOOK = (240.0, -160.0)
STRONGBOX_SLIP = (60.0, 30.0)       # from SOCKET_Strongbox: out toward the door, and to its side
TRACE_TOP = 220.0
# The depot: Fenn's coins on the landing, this far out from the agent's door (cm); the platform's spot for the Bell-Ringer,
# out from the station's pivot (its front door) over the platform. Fenn's lid opens this far on its hinge (degrees; the
# art's ~100).
COINS_OUT = 40.0
LID_OPEN = 100.0
PLATFORM_SPOT = (180.0, 120.0)
# The waiting room's fight: it comes as the player nears the depot, its ground the depot and the platform in front.
DEPOT_ACTIVATION = 2600.0
DEPOT_GROUND = ((-860.0, 520.0), (-860.0, -1000.0), (380.0, -1000.0), (380.0, 520.0))   # in the station's frame
# The cellar's fight: its spawner at the cellar's middle, from the workshop's pivot; a player more than this above or
# below the cellar's floor isn't on its ground (the workshop's floor is 3.6 m over it).
CELLAR = (-725.0, 155.0, -825.0, 455.0)
CELLAR_FLOOR = -330.0
CELLAR_RISE = 250.0
CELLAR_ACTIVATION = 1500.0
CELLAR_LEVEL = 3
# Thaddeus's tape: on his bench, this far in front of where he stands and to his right (cm).
TAPE_AT = (60.0, 40.0)
# The keepers' roll on the nave's front wall, left of the inner door (Chapel.py: the front wall's inner face 3.84 m in
# front of the pivot; the board 1.95 m to the left as one faces the door), facing into the nave. SM_KeepersRoll's pivot is
# the middle of its back's bottom edge (MissionProps.py): its foot 0.96 m up, so the painted names sit at eye height.
ROLL_AT = (381.0, 195.0, 96.0)
ROLL_SNAP = 120.0
# The boards where it happened: the stain the lookout's model bakes into its deck (Art/Models/Buildings/Lookout.py's STAIN,
# right of the deck's middle), found from Sexton's seat (its SOCKET_Sit, facing into the deck): this far forward and to
# the seat's right (cm). The readable and the stain's decal (MI_LookoutStain: Tools/Unreal/build_stain_decal.py) lie there.
STAIN_AT = (157.5, 234.3)
STAIN_DECAL = '/Game/Art/Materials/MI_LookoutStain'
# The decal's box, half extents (cm): 12 down through the boards' tops and up off them, and the maps' 1.5 m square
# (Art/Textures/LookoutStain/make_lookout_stain.py's SPAN_M).
STAIN_DECAL_SIZE = (12.0, 75.0, 75.0)
# The den (Den Rock's SOCKET_DenMouth frame, DenDressing.placement.json): the dressing's hat on the coffin lid, and the
# badge in the webbing by the larder.
DEN_HAT = (71.0, -351.0, 14.0)
DEN_BADGE = (-600.0, -150.0, 25.0)
# Tilly's sill: Pa's bail on it, below and out from her window's speaker socket (cm).
SILL_DOWN = 38.0
SILL_OUT = 18.0
# A cache tag lies in front of its crate (cm).
TAG_OUT = 70.0
# A mission artifact over a poster's paper: its invisible card this far proud of the paper (cm), so the Interact key's line
# meets it first while it has a mission to offer, and this much wider than the paper on every side (cm), so a line from the
# side can't slip past the card's edge onto the paper's. Its reach (AMissionArtifact's 3 m, over the paper's 2.5 m) settles
# the rest: the card wins the focus wherever on the paper the player looks (Looter.Missions.BoardNote.Focus).
CARD_PROUD = 1.5
CARD_MARGIN = 2.0
# The place markers about the player's middle over their spot (cm).
MARKER_UP = 90.0
# Main Street's safe ground covers the street from a little under it to its roofs (cm): never Tilly's cellar.
BAND_BELOW = 200.0
BAND_ABOVE = 2000.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def actor_class(name):
    return unreal.load_class(None, CLASSES + name)


def level_actors(build):
    found = {}
    for actor in actors.get_all_level_actors():
        if unreal.Name(build.tag) in actor.tags:
            found.setdefault(str(actor.get_actor_label()), actor)
    return found


def mesh_index():
    index = {}
    for path in unreal.EditorAssetLibrary.list_assets('/Game/Art', recursive=True, include_folder=False):
        name = path.rsplit('/', 1)[-1].split('.')[0]
        if name.startswith('SM_'):
            index[name[3:]] = path.split('.')[0]
    return index


def first_mesh(meshes, names):
    for name in names:
        if name in meshes:
            return unreal.load_asset(meshes[name])
    return None


def building_mesh(actor):
    if actor is None:
        return None
    if isinstance(actor, unreal.StaticMeshActor):
        return actor.static_mesh_component
    try:
        return actor.get_editor_property('building')
    except Exception:
        return actor.get_component_by_class(unreal.StaticMeshComponent)


def socket(actor, name):
    component = building_mesh(actor)
    if component is None or not component.does_socket_exist(name):
        return None
    return component.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)


def onto(host, x, y, top, bottom):
    """The first of the host's own surfaces under (x, y), from top down to bottom (world z), or None."""
    return story.onto_host(host, unreal.Vector(x, y, top), unreal.Vector(x, y, bottom))


def line(speaker, words):
    made = unreal.StoryLine()
    made.set_editor_property('speaker', unreal.Text(speaker))
    made.set_editor_property('text', unreal.Text(words))
    made.set_editor_property('seconds', 0.0)
    return made


def lines_of(pairs):
    return [line(speaker, words) for speaker, words in pairs]


def topic(build, when, key):
    """A speaker's topic: its line set when create_story_lines.py has made it, else the design's words (LINES)."""
    name, words = LINES[key]
    made = unreal.SpeakerTopic()
    made.set_editor_property('when', when)
    path = story.LINES + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        made.set_editor_property('line_set', unreal.load_asset(path))
    else:
        made.set_editor_property('lines', lines_of(words))
        build.log(f'{name} isn\'t made yet: its topic says the design\'s words')
    return made


def put(build, what, location, yaw, label, tags=(), scale=None):
    return build.place(what, (location.x, location.y, location.z) if hasattr(location, 'x') else location, yaw,
                       label=label, folder=FOLDER, tags=tags, scale=scale)


def marker(build, at, tag):
    point = put(build, unreal.TargetPoint, unreal.Vector(at.x, at.y, at.z + MARKER_UP), 0.0, tag, tags=(tag,))
    build.log(f'{tag} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')
    return point


# ---------------------------------------------------------------------------
# The missions' steps
# ---------------------------------------------------------------------------

class Steps:
    """Finds a mission's step (counted from 0) by the objective that asks for something, in its data asset."""

    def __init__(self, build):
        self.build = build
        self.cache = {}

    def mission(self, mission_id):
        if mission_id not in self.cache:
            path = MISSIONS + mission_id
            self.cache[mission_id] = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        return self.cache[mission_id]

    @staticmethod
    def prop(obj, name):
        try:
            return obj.get_editor_property(name)
        except Exception:
            return None

    @classmethod
    def filter_tag(cls, actor_filter):
        tag = cls.prop(actor_filter, 'actor_tag') if actor_filter is not None else None
        return str(tag) if tag is not None else ''

    def matches(self, objective, kind, key):
        name = objective.get_class().get_name()
        if kind == 'talk':
            return name == 'MissionTalkObjective' and str(self.prop(objective, 'speaker_tag')) == key
        if kind == 'interact':
            # A lasting one too (UMissionLastingInteractObjective: what stays used in the world, the gang's strongbox): it
            # was missed, so Calder's Caches' strongbox step fell back to the design's.
            return (name in ('MissionInteractObjective', 'MissionLastingInteractObjective')
                    and self.filter_tag(self.prop(objective, 'target')) == key)
        if kind == 'collect':
            items = self.prop(objective, 'items') or []
            return name == 'MissionCollectItemObjective' and key in [str(each) for each in items]
        if kind == 'clear':
            return name == 'MissionClearObjective' and str(self.prop(objective, 'spawner_id')) == key
        if kind == 'reach':
            place_spec = self.prop(objective, 'place')
            return name == 'MissionReachObjective' and self.filter_tag(self.prop(place_spec, 'actor')) == key
        if kind == 'event':
            return name == 'MissionEventObjective' and str(self.prop(objective, 'event')) == key
        return False

    def find_all(self, mission_id, kind, key):
        """Every step asking for it, in order (none without the mission's data)."""
        found = []
        mission = self.mission(mission_id)
        if mission is not None:
            for index, step in enumerate(mission.get_editor_property('steps')):
                if any(objective is not None and self.matches(objective, kind, key)
                       for objective in step.get_editor_property('objectives')):
                    found.append(index)
        return found

    def find(self, mission_id, kind, key, default):
        """The first step asking for it, else default (logged)."""
        return self.find_any(mission_id, ((kind, key),), default)

    def find_any(self, mission_id, asks, default):
        """The first step asking for any of asks ((kind, key) pairs, in the order tried: Main 1's plate is its
        PorchPlate.Taken event in the data, an Interact on the plate in the design), else default (logged)."""
        for kind, key in asks:
            found = self.find_all(mission_id, kind, key)
            if found:
                return found[0]
        wanted = ' or '.join(f'{kind} {key}' for kind, key in asks)
        self.build.log(f'{mission_id}: no step asks to {wanted} in its data yet: the design\'s step {default} is used')
        return default


# ---------------------------------------------------------------------------
# Speakers, readables, items
# ---------------------------------------------------------------------------

def speaker_point(build, at, yaw, label, tags, name, prompt, reach, missions_only, topics, lines_pairs=(), mesh=None):
    cls = actor_class('SpeakerPoint')
    if cls is None:
        build.warn('no SpeakerPoint class (build the game module first): no ' + label)
        return None
    point = put(build, cls, at, yaw, label, tags=tags)
    if mesh is not None:
        point.get_editor_property('mesh').set_static_mesh(mesh)
    talk = point.get_editor_property('speaker_point')
    talk.set_editor_property('speaker_name', unreal.Text(name))
    if prompt:
        talk.set_editor_property('prompt', unreal.Text(prompt))
    talk.set_editor_property('reach', reach)
    talk.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
    talk.set_editor_property('missions_only', missions_only)
    if lines_pairs:
        talk.set_editor_property('lines', lines_of(lines_pairs))
    talk.set_editor_property('topics', list(topics))
    return point


def readable(build, meshes, readable_id, at, yaw, shown_when=None, pitch=0.0):
    cls = actor_class('ReadableProp')
    if cls is None:
        build.warn('no ReadableProp class (build the game module first): no ' + readable_id)
        return None
    title, heading, text, said, prompt, models = READABLES[readable_id]
    prop = put(build, cls, at, yaw, f'Readable_{readable_id}', tags=('Readable', f'Readable_{readable_id}'))
    if pitch:
        prop.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw), False)
    prop.set_editor_property('readable_id', unreal.Name(readable_id))
    prop.set_editor_property('title', unreal.Text(title))
    if heading:
        prop.set_editor_property('heading', unreal.Text(heading))
    if text:
        prop.set_editor_property('text', unreal.Text(text))
    if said:
        prop.set_editor_property('lines', lines_of(said))
    prop.set_editor_property('prompt', unreal.Text(prompt))
    if shown_when is not None:
        prop.set_editor_property('shown_when', shown_when)
    model = first_mesh(meshes, models)
    if model is not None:
        prop.get_editor_property('mesh').set_static_mesh(model)
    elif models:
        build.warn(f'no SM_{models[0]} yet (Art/Models/Props, wave 4b): {readable_id} is read with nothing of its own to see')
    build.log(f'{readable_id} to read at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')
    return prop


def item(build, meshes, item_id, at, yaw, shown_when=None, extra_tags=(), prompt=None, beam=False):
    cls = actor_class('MissionItem')
    if cls is None:
        build.warn('no MissionItem class (build the game module first): no ' + item_id)
        return None
    models, name = ITEMS[item_id]
    made = put(build, cls, at, yaw, f'Item_{item_id}', tags=('MissionItem', f'Item_{item_id}') + tuple(extra_tags))
    made.set_editor_property('item_id', unreal.Name(item_id))
    made.set_editor_property('display_name', unreal.Text(name))
    if prompt:
        made.set_editor_property('prompt', unreal.Text(prompt))
    if shown_when is not None:
        made.set_editor_property('shown_when', shown_when)
    if beam:
        made.set_editor_property('show_beam', True)
    model = first_mesh(meshes, models)
    if model is not None:
        made.get_editor_property('mesh').set_static_mesh(model)
    else:
        build.log(f'no SM_{models[0]} yet: {item_id} wears its item\'s model or the stand-in')
    build.log(f'{item_id} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')
    return made


def encounter(build, at, spawner_id, groups, tags, when, points, corners, activation, rise, hunt=False):
    cls = actor_class('EncounterSpawner')
    if cls is None:
        build.warn('no EncounterSpawner class (build the game module first): no ' + spawner_id)
        return None
    made = put(build, cls, at, 0.0, f'Encounter_{spawner_id}')
    made.set_editor_property('spawner_id', unreal.Name(spawner_id))
    made.set_editor_property('groups', groups)
    made.set_editor_property('creature_tags', [unreal.Name(each) for each in tags])
    made.set_editor_property('active_when', when)
    made.set_editor_property('spawn_points', [unreal.Vector(p.x - at.x, p.y - at.y, 0.0) for p in points])
    made.set_editor_property('activation_radius', activation)
    made.set_editor_property('hunt_on_spawn', hunt)
    made.set_editor_property('ground_corners', corners)
    made.set_editor_property('ground_max_rise', rise)
    return made


def group(cls, count, rank, level=0):
    made = story.group(cls, count, rank)
    if level:
        made.set_editor_property('level', level)
    return made


# ---------------------------------------------------------------------------
# The depot: The Old Fare, and Dead Lift's coal
# ---------------------------------------------------------------------------

def place_depot(build, meshes, steps, found):
    station = found.get('depot')
    if station is None:
        build.warn('no depot placed (build the whole level first): no Mr. Sloane, Fenn or the waiting room\'s fight')
        return
    if socket(station, 'AgentDoor') is None:
        build.warn('the depot has no SOCKET_AgentDoor (level.models Depot: RR_Depot, patches/town.json): The Old Fare\'s '
                   'pieces are left out')
        return
    talk_step = steps.find(OLD_FARE, 'talk', 'Speaker_Agent', 4)
    coins_step = steps.find(OLD_FARE, 'collect', 'FennsFare', talk_step)
    # Fenn's coffin is used twice: looked at (step 3 of the design), then the coins laid back (step 6, after the coins).
    uses = steps.find_all(OLD_FARE, 'interact', 'FennCoffin')
    look_step = uses[0] if uses else 2
    lay_step = uses[-1] if len(uses) > 1 else max(coins_step + 1, look_step + 1)
    clear_step = steps.find(OLD_FARE, 'clear', 'DepotWaiting', 0)

    # Mr. Sloane through his door upstairs, then at his ticket window.
    door = socket(station, 'AgentDoor')
    speaker_point(build, door.translation, door.rotation.rotator().yaw, 'Speaker_Agent', ('Speaker_Agent',), SLOANE, 'Knock',
                  story.TALK_REACH, True, [
                      topic(build, story.condition(during=OLD_FARE, from_step=talk_step), 'AgentCoins'),
                      topic(build, story.condition(during=OLD_FARE), 'AgentDoor'),
                  ])
    window = socket(station, 'TicketWindow') or socket(station, 'Speaker')
    if window is not None:
        coal = story.condition(after=[OLD_FARE], during=DEAD_LIFT)
        speaker_point(build, window.translation, window.rotation.rotator().yaw, 'Speaker_AgentWindow',
                      ('Speaker_AgentWindow',), SLOANE, 'Talk', story.TALK_REACH, True, [
                          topic(build, story.condition(after=[OLD_FARE, MAIN7]), 'AgentWindowLily'),
                          topic(build, coal, 'AgentWindowCoal'),
                          topic(build, story.condition(after=[OLD_FARE]), 'AgentWindow'),
                      ])
    else:
        build.warn('the depot has no SOCKET_TicketWindow: Mr. Sloane never comes down to his window')
    build.log(f'Mr. Sloane at his door upstairs (The Old Fare\'s coins from step {talk_step}) and at his window after it')

    # Josiah Fenn in his coffin: one actor that talks (the turn-in) and is looked at (its interactions), each a tap.
    rest = socket(station, 'BaggageCoffin')
    coffin_mesh = first_mesh(meshes, ('FennCoffin',))
    if rest is not None:
        if coffin_mesh is None:
            build.warn('no SM_FennCoffin yet (wave 4b): Fenn talks from an empty spot in the baggage room')
        fenn = speaker_point(build, rest.translation, rest.rotation.rotator().yaw, 'FennCoffin',
                             ('Speaker_Fenn', 'FennCoffin', 'Obstacle'), FENN, 'Look in the coffin', 260.0, True, [
                                 topic(build, story.condition(during=OLD_FARE, from_step=lay_step), 'FennCoins'),
                                 topic(build, story.condition(during=OLD_FARE, from_step=look_step), 'FennLook'),
                             ], mesh=coffin_mesh)
        if fenn is not None:
            # The voice from the coffin's head end (or its own SOCKET_Speaker).
            talk = fenn.get_editor_property('speaker_point')
            head = coffin_mesh.find_socket('Speaker') if coffin_mesh is not None else None
            talk.set_relative_location(head.relative_location if head is not None else unreal.Vector(0.0, 0.0, 45.0),
                                       False, True)
            place_fenn_story(build, meshes, fenn, look_step, lay_step)
            build.log(f'Josiah Fenn in his coffin (looked at from step {look_step}, the coins laid back from {lay_step})')
    else:
        build.warn('the depot has no SOCKET_BaggageCoffin: no Fenn')

    # The agent's desk, Fenn's coins on the landing, the baggage room's place.
    desk = socket(station, 'AgentDesk')
    if desk is not None:
        readable(build, meshes, 'AgentDesk', desk.translation, desk.rotation.rotator().yaw)
    front = door.rotation.rotator().get_forward_vector()
    landing = door.translation + front * COINS_OUT
    floor = onto(station, landing.x, landing.y, door.translation.z, door.translation.z - 400.0)
    coins_at = unreal.Vector(landing.x, landing.y, floor.z if floor is not None else door.translation.z - 155.0)
    item(build, meshes, 'FennsFare', coins_at, door.rotation.rotator().yaw,
         shown_when=story.condition(during=OLD_FARE, from_step=coins_step), prompt='Pick up the coins')
    lost = socket(station, 'LostProperty')
    if rest is not None and lost is not None:
        middle = (rest.translation + lost.translation) * 0.5
        marker(build, unreal.Vector(middle.x, middle.y, min(rest.translation.z, lost.translation.z)), 'Place_BaggageRoom')

    # The waiting room's dead: three Unpaid where people wait, a Restless one, a Bell-Ringer out on the platform.
    unpaid, ringer = actor_class('UnpaidCreature'), actor_class('BellRingerCreature')
    waits = [socket(station, f'Wait_{k}') for k in range(1, 5)]
    waits = [w.translation for w in waits if w is not None]
    if unpaid is None or ringer is None or not waits:
        build.warn('no UnpaidCreature or BellRingerCreature class, or no SOCKET_Wait_*: no fight in the waiting room')
        return
    frame = station.get_actor_transform()
    platform = frame.transform_location(unreal.Vector(PLATFORM_SPOT[0], PLATFORM_SPOT[1], 40.0))
    middle = unreal.Vector(sum(w.x for w in waits) / len(waits), sum(w.y for w in waits) / len(waits), waits[0].z)
    corners = [frame.transform_location(unreal.Vector(x, y, 40.0)) for x, y in DEPOT_GROUND]
    encounter(build, middle, 'DepotWaiting',
              [group(unpaid, 3, 'BASIC'), group(unpaid, 1, 'RARE'), group(ringer, 1, 'BASIC')], ('Unpaid_DepotWaiting',),
              story.condition(during=OLD_FARE, before_step=clear_step + 1), waits + [platform], corners,
              DEPOT_ACTIVATION, 250.0)
    build.log(f'the waiting room\'s fight (DepotWaiting: 3 Unpaid, a Restless one, a Bell-Ringer) during The Old Fare, '
              f'before step {clear_step + 1}')


def story_prop(build, model, transform, label, shown=(), hidden=()):
    """An AStoryProp (World/StoryProp.h): a model the story shows and hides, nothing to use, nothing colliding."""
    cls = actor_class('StoryProp')
    if cls is None:
        build.warn('no StoryProp class (build the game module first): no ' + label)
        return None
    at = transform.translation
    prop = put(build, cls, at, 0.0, label)
    prop.set_actor_rotation(transform.rotation.rotator(), False)
    prop.get_editor_property('mesh').set_static_mesh(model)
    prop.set_editor_property('shown_when', list(shown))
    prop.set_editor_property('hidden_when', list(hidden))
    return prop


def place_fenn_story(build, meshes, fenn, look_step, lay_step):
    """Fenn's lid (SM_FennCoffin_Lid on the coffin's SOCKET_Lid: shut, then open from The Old Fare's look at Fenn until the
    mission is turned in, shut again after: he rests, his fare paid) and his fare laid back on his eyes (SM_GraveCoins on
    SOCKET_Eyes, once the coins are back, and after)."""
    coffin = fenn.get_editor_property('mesh')
    lid_model = first_mesh(meshes, ('FennCoffin_Lid',))
    open_from = story.condition(during=OLD_FARE, from_step=look_step)
    if lid_model is not None and coffin is not None and coffin.does_socket_exist('Lid'):
        hinge = coffin.get_socket_transform('Lid', unreal.RelativeTransformSpace.RTS_WORLD)
        story_prop(build, lid_model, hinge, 'FennCoffin_LidShut', hidden=[open_from])
        # Open: its hinge's own pitch (positive opens it, SM_FennCoffin_Lid) put over the socket's turn.
        opened = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=0.0, pitch=LID_OPEN, yaw=0.0),
                                                     hinge.rotation.rotator())
        story_prop(build, lid_model, unreal.Transform(location=hinge.translation, rotation=opened,
                                                      scale=unreal.Vector(1.0, 1.0, 1.0)),
                   'FennCoffin_LidOpen', shown=[open_from])
    else:
        build.warn('no SM_FennCoffin_Lid or SOCKET_Lid: Fenn\'s coffin has no lid to lift')
    coins = first_mesh(meshes, ITEMS['FennsFare'][0])
    if coins is not None and coffin is not None and coffin.does_socket_exist('Eyes'):
        eyes = coffin.get_socket_transform('Eyes', unreal.RelativeTransformSpace.RTS_WORLD)
        story_prop(build, coins, eyes, 'FennCoffin_Fare',
                   shown=[story.condition(during=OLD_FARE, from_step=lay_step + 1), story.condition(after=[OLD_FARE])])
    else:
        build.warn('no SM_GraveCoins or SOCKET_Eyes: Fenn\'s fare isn\'t seen back on his eyes')


def place_coal(build, meshes, found):
    stage = found.get('coalStage')
    sack = socket(stage, 'Sack') if stage is not None else None
    if sack is None:
        build.warn('no coal stage placed with its SOCKET_Sack (patches/town.json coalStage): no coal for Dead Lift')
        return
    # Under the chute, ready to fill: a tap fills and takes it (Dead Lift's "Fill a coal sack": Collect CoalSack, or
    # Interact CoalStage without a hold).
    item(build, meshes, 'CoalSack', sack.translation, sack.rotation.rotator().yaw, extra_tags=('CoalStage',),
         prompt='Fill a coal sack')


# ---------------------------------------------------------------------------
# Tilly's workshop and cellar: Knocking Below
# ---------------------------------------------------------------------------

def place_workshop(build, meshes, steps, found):
    shop = found.get('workshop')
    if shop is None or socket(shop, 'CellarDoor') is None:
        build.warn('no workshop placed with its sockets (patches/town.json, SM_RR_Workshop_Bright): no Knocking Below')
        return
    talk_step = steps.find(KNOCKING, 'talk', 'Speaker_Thaddeus', 5)
    tape_step = steps.find(KNOCKING, 'collect', 'ThaddeusTape', talk_step)
    if tape_step <= talk_step:
        # The talk and the tape in one step: the tape comes with the step (it can't wait on the talk itself).
        tape_step = talk_step
    clear_step = steps.find(KNOCKING, 'clear', 'TillyCellar', 3)
    down_step = steps.find(KNOCKING, 'reach', 'Place_CellarFloor', max(clear_step - 1, 0))

    door = socket(shop, 'WorkshopDoor')
    if door is not None:
        marker(build, door.translation, 'Place_WorkshopDoor')
    hook = socket(shop, 'MeasuringHook')
    if hook is not None:
        item(build, meshes, 'CellarKey', hook.translation, hook.rotation.rotator().yaw, prompt='Take the cellar key')
    slate = socket(shop, 'OrdersSlate')
    if slate is not None:
        readable(build, meshes, 'OrdersSlate', slate.translation, slate.rotation.rotator().yaw)
    floor = socket(shop, 'CellarFloor')
    if floor is not None:
        marker(build, floor.translation, 'Place_CellarFloor')

    # The doors (SM_RR_CellarDoor: its pivot the hinge at the floor, swinging open toward the socket's front).
    leaf = first_mesh(meshes, ('RR_CellarDoor',))
    if leaf is None:
        build.warn('no SM_RR_CellarDoor yet: the cellar\'s doors are left out (the way down stands open)')
    else:
        locked = actor_class('LockedDoor')
        plain = actor_class('InteractableProp')
        at = socket(shop, 'CellarDoor')
        cls = locked or plain
        if locked is None:
            build.warn('no LockedDoor class (build the game module first): the cellar door opens without its key')
        cellar_door = put(build, cls, at.translation, at.rotation.rotator().yaw, 'CellarDoor', tags=('CellarDoor', 'Obstacle'))
        cellar_door.get_editor_property('mesh').set_static_mesh(leaf)
        door_settings(cellar_door)
        if locked is not None:
            cellar_door.set_editor_property('key_item', unreal.Name('CellarKey'))
            cellar_door.set_editor_property('unlocked_when', story.condition(after=[KNOCKING]))
        cold = socket(shop, 'ColdRoomDoor')
        if cold is not None and plain is not None:
            cold_door = put(build, plain, cold.translation, cold.rotation.rotator().yaw, 'ColdRoomDoor',
                              tags=('ColdRoomDoor', 'Obstacle'))
            cold_door.get_editor_property('mesh').set_static_mesh(leaf)
            door_settings(cold_door)
        build.log('the cellar door (locked until the cellar key is carried, open after Knocking Below) and the cold room\'s')

    # The cellar's dead, by the coffins they got out of.
    unpaid = actor_class('UnpaidCreature')
    if unpaid is not None and floor is not None:
        frame = shop.get_actor_transform()
        x0, x1, y0, y1 = CELLAR
        middle = frame.transform_location(unreal.Vector((x0 + x1) * 0.5, (y0 + y1) * 0.5, CELLAR_FLOOR))
        coffins = [socket(shop, f'Coffin_{k}') for k in range(1, 5)]
        spots = [unreal.Vector(c.translation.x, c.translation.y, floor.translation.z) for c in coffins if c is not None]
        corners = [frame.transform_location(unreal.Vector(x, y, CELLAR_FLOOR)) for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1))]
        encounter(build, middle, 'TillyCellar',
                  [group(unpaid, 3, 'BASIC', CELLAR_LEVEL), group(unpaid, 1, 'RARE', CELLAR_LEVEL)], ('Unpaid_TillyCellar',),
                  story.condition(during=KNOCKING, from_step=down_step, before_step=clear_step + 1), spots, corners,
                  CELLAR_ACTIVATION, CELLAR_RISE)
        build.log(f'the cellar\'s fight (TillyCellar: 3 Unpaid and a Restless one, level {CELLAR_LEVEL}) during Knocking '
                  f'Below from step {down_step} to {clear_step}')

    # Thaddeus at his bench in the cold room, and his tape on the bench once he's talked.
    bench = socket(shop, 'ThaddeusBench')
    cls = actor_class('ThaddeusBright')
    if bench is None or cls is None:
        build.warn('no ThaddeusBright class or SOCKET_ThaddeusBench: no Thaddeus in the cold room')
        return
    thaddeus = put(build, cls, bench.translation, bench.rotation.rotator().yaw, 'ThaddeusBright', tags=('Speaker_Thaddeus',))
    talk = thaddeus.get_editor_property('speaker_point')
    talk.set_editor_property('lines', lines_of(LINES['ThaddeusWaiting'][1]))
    talk.set_editor_property('topics', [
        topic(build, story.condition(after=[KNOCKING]), 'ThaddeusAfter'),
        topic(build, story.condition(during=KNOCKING, from_step=talk_step), 'ThaddeusTalk'),
    ])
    ahead = bench.rotation.rotator().get_forward_vector()
    right = bench.rotation.rotator().get_right_vector()
    spot = bench.translation + ahead * TAPE_AT[0] + right * TAPE_AT[1]
    top = onto(shop, spot.x, spot.y, bench.translation.z + 150.0, bench.translation.z)
    item(build, meshes, 'ThaddeusTape', unreal.Vector(spot.x, spot.y, top.z if top is not None else bench.translation.z + 85.0),
         bench.rotation.rotator().yaw, shown_when=story.condition(during=KNOCKING, from_step=tape_step),
         prompt="Take Thaddeus's tape")
    build.log(f'Thaddeus Bright at his bench (talked to from step {talk_step}; his tape from step {tape_step})')


def door_settings(door):
    """A cellar door: opened and shut by a tap, swinging 90 degrees toward its socket's front (SM_RR_CellarDoor)."""
    door.set_editor_property('use_mode', unreal.InteractablePropUse.TOGGLE)
    door.set_editor_property('effect', unreal.InteractablePropEffect.SWING)
    door.set_editor_property('open_angle', -90.0)
    door.set_editor_property('swing_seconds', 0.6)
    door.set_editor_property('cooldown_seconds', 0.6)
    door.set_editor_property('prompt', unreal.Text('Open the door'))
    door.set_editor_property('prompt_when_on', unreal.Text('Close the door'))


# ---------------------------------------------------------------------------
# The sheriff's office, the chapel, the lookout, the den, the barn
# ---------------------------------------------------------------------------

def place_sheriff(build, meshes, steps, found):
    office = found.get('sheriff')
    if office is None:
        build.warn('no sheriff\'s office placed: no crank, report or daybook')
        return
    frame = office.get_actor_transform()
    base = office.get_actor_location().z

    def on_office(local, label):
        at = frame.transform_location(unreal.Vector(local[0], local[1], 0.0))
        top = onto(office, at.x, at.y, base + TRACE_TOP, base - 50.0)
        if top is None:
            build.warn(f'nothing of the sheriff\'s office under its {label}: it stands on the office\'s floor height')
            return unreal.Vector(at.x, at.y, base + 40.0)
        return top

    yaw = office.get_actor_rotation().yaw
    item(build, meshes, 'WinchCrank', on_office(CELL_CRANK, 'crank'), yaw + 90.0, prompt='Take the winch crank')
    readable(build, meshes, 'OdellReport', on_office(CELL_REPORT, 'report'), yaw + 180.0)
    # The open book turned to whoever comes in at the door (SM_PostLog: its reader's side its +X).
    readable(build, meshes, 'OdellDaybook', on_office(DESK_BOOK, 'daybook'), yaw)
    box = socket(office, 'Strongbox')
    if box is not None:
        ahead = box.rotation.rotator().get_forward_vector()
        right = box.rotation.rotator().get_right_vector()
        spot = box.translation + ahead * STRONGBOX_SLIP[0] + right * STRONGBOX_SLIP[1]
        floor = onto(office, spot.x, spot.y, box.translation.z + 60.0, box.translation.z - 40.0)
        # Out of the box with the loot: there from the step that opens the strongbox, while Calder's Caches runs.
        open_step = steps.find(CALDERS, 'interact', 'GangStrongbox', 1)
        readable(build, meshes, 'DeaconsSlip', floor if floor is not None else spot, box.rotation.rotator().yaw + 160.0,
                 shown_when=story.condition(during=CALDERS, from_step=open_step))


def place_chapel_roll(build, meshes, found):
    chapel = found.get('chapel')
    if chapel is None:
        build.warn('no chapel placed: no keepers\' roll')
        return
    frame = chapel.get_actor_transform()
    at = frame.transform_location(unreal.Vector(*ROLL_AT))
    # Snapped onto the front wall's inner face: a line from the nave toward the wall.
    inward = frame.transform_location(unreal.Vector(ROLL_AT[0] - ROLL_SNAP, ROLL_AT[1], ROLL_AT[2]))
    wall = story.onto_host(chapel, inward, frame.transform_location(unreal.Vector(ROLL_AT[0] + 40.0, ROLL_AT[1], ROLL_AT[2])))
    readable(build, meshes, 'KeepersRoll', wall if wall is not None else at, chapel.get_actor_rotation().yaw + 180.0)


def place_lookout_stains(build, meshes, found):
    """The boards where it happened (Main 2's optional): the readable on the lookout's stained boards, and the old stain's
    decal soaked into them (always there, whatever the missions)."""
    lookout = found.get('lookout')
    seat = socket(lookout, 'Sit') if lookout is not None else None
    if seat is None:
        build.warn('no lookout or its SOCKET_Sit: no stains to look at')
        return
    turn = seat.rotation.rotator()
    spot = seat.translation + turn.get_forward_vector() * STAIN_AT[0] + turn.get_right_vector() * STAIN_AT[1]
    deck = onto(lookout, spot.x, spot.y, seat.translation.z + 100.0, seat.translation.z - 200.0)
    readable(build, meshes, 'LookoutStains', deck if deck is not None else spot, turn.yaw,
             shown_when=story.condition(during=MAIN2))
    if deck is None:
        build.warn('nothing of the lookout under its stain: no decal on the boards')
        return
    if not unreal.EditorAssetLibrary.does_asset_exist(STAIN_DECAL):
        build.warn('no MI_LookoutStain yet (build_stain_decal.py): the boards keep only the model\'s darker boards')
        return
    # Projected straight down (the decal's X), the maps' U (the decal's Z) along the boards: the seat's right.
    decal = put(build, unreal.DecalActor, deck, turn.yaw, 'LookoutStain_Decal', tags=('LookoutStain',))
    decal.set_actor_rotation(unreal.MathLibrary.make_rot_from_xz(unreal.Vector(0.0, 0.0, -1.0), turn.get_right_vector()),
                             False)
    stain = decal.get_editor_property('decal')
    stain.set_editor_property('decal_material', unreal.load_asset(STAIN_DECAL))
    stain.set_editor_property('decal_size', unreal.Vector(*STAIN_DECAL_SIZE))
    build.log(f'the old stain on the lookout\'s boards at ({deck.x:.0f}, {deck.y:.0f}, {deck.z:.0f})')


def place_den_relics(build, meshes, found):
    rock = found.get('denRock')
    mouth = socket(rock, 'DenMouth') if rock is not None else None
    if mouth is None:
        build.warn('no Den Rock with its SOCKET_DenMouth: no hat by the den, no badge in it')
        return
    turn = mouth.rotation.rotator().yaw
    readable(build, meshes, 'SheriffHat', mouth.transform_location(unreal.Vector(*DEN_HAT)), turn,
             shown_when=story.condition(after=[MAIN4]))
    readable(build, meshes, 'SheriffBadge', mouth.transform_location(unreal.Vector(*DEN_BADGE)), turn,
             shown_when=story.condition(during=SIDE3))


def place_barn_chalk(build, meshes, found):
    barn = found.get('whitlockBarn')
    chalk = socket(barn, 'WagesChalk') if barn is not None else None
    if chalk is None:
        build.warn('no Whitlock barn with its SOCKET_WagesChalk (SM_RR_Barn_Whitlock, patches/town.json): no wages to read')
        return
    readable(build, meshes, 'WagesChalk', chalk.translation, chalk.rotation.rotator().yaw)


# ---------------------------------------------------------------------------
# The farm, Tilly's sill, the caches, the posters, the bridge, Main Street
# ---------------------------------------------------------------------------

def place_porch_plate(build, meshes, steps, found):
    """Delia's plate as the APorchPlate (the jars once in Main 1, the biscuits in Main 7), where the plain plate stood."""
    cls = actor_class('PorchPlate')
    house = found.get('farmhouse')
    at = socket(house, 'Plate') if house is not None else None
    if cls is None or at is None:
        if cls is None:
            build.warn('no PorchPlate class (build the game module first): the plate stays a plain model')
        return
    plain = found.get('Farmhouse_Plate')
    if plain is not None:
        actors.destroy_actor(plain)
    # The data's step is the jars taken (the plate's PorchPlate.Taken event, which Looter.Story.SevenDays.Mission checks); an
    # Interact on the plate would do as well.
    take_step = steps.find_any(MAIN1, (('event', 'PorchPlate.Taken'), ('interact', 'PorchPlate')), 4)
    plate = put(build, cls, at.translation, at.rotation.rotator().yaw, 'PorchPlate_Delia', tags=('PorchPlate', 'Obstacle'))
    plate.set_actor_rotation(at.rotation.rotator(), False)
    for part, names in (('plate', ('PorchPlate_Plate', 'PorchPlate')), ('jars', ('PorchPlate_Jars',)),
                        ('biscuits', ('PorchPlate_Biscuits',))):
        model = first_mesh(meshes, names)
        if model is not None:
            plate.get_editor_property(part).set_static_mesh(model)
    plate.set_editor_property('take_when', story.condition(during=MAIN1, from_step=take_step))
    plate.set_editor_property('eat_when', story.condition(during=MAIN7))
    plate.set_editor_property('take_lines', lines_of([("Delia's slip", "SALT FOR THE ROAD. THEY DON'T CARE FOR IT. — D.")]))
    plate.set_editor_property('eat_lines', lines_of([(DELIA, '...Thank you, Ellis.')]))
    if 'PorchPlate_Jars' not in meshes:
        build.log('no SM_PorchPlate_Jars yet: the jars stay drawn on the plate after they\'re taken (one model)')
    build.log(f'Delia\'s porch plate (the jars from Main 1\'s step {take_step}, the biscuits in Main 7)')


def place_bail(build, meshes, found):
    shop = found.get('undertaker')
    window = socket(shop, 'Speaker') if shop is not None else None
    if window is None:
        build.warn('no Bright & Daughter with its SOCKET_Speaker: no lantern bail on the sill')
        return
    out = window.rotation.rotator().get_forward_vector()
    spot = window.translation + out * SILL_OUT
    sill = onto(shop, spot.x, spot.y, window.translation.z, window.translation.z - 120.0)
    at = sill if sill is not None else unreal.Vector(spot.x, spot.y, window.translation.z - SILL_DOWN)
    item(build, meshes, 'LanternBail', at, window.rotation.rotator().yaw + 90.0, shown_when=story.condition(after=[MAIN3]),
         prompt="Take Pa's lantern bail")


def place_cache_tags(build, meshes, found):
    for number, label in ((1, 'RangerCache_Windmill'), (2, 'RangerCache_SinkRim'), (3, 'RangerCache_BluffPath')):
        crate = found.get(label)
        if crate is None:
            build.warn(f'no {label} (build_area_caches.py): no CacheTag{number}')
            continue
        ahead = crate.get_actor_forward_vector()
        spot = crate.get_actor_location() + ahead * TAG_OUT
        ground = story.ground_at(spot.x, spot.y, crate.get_actor_location().z, warn=False)
        item(build, meshes, f'CacheTag{number}', unreal.Vector(spot.x, spot.y, ground), crate.get_actor_rotation().yaw,
             prompt="Take Calder's tag")


def place_artifact(build, poster, tag, title, prompt, extra_tags=()):
    """A mission artifact over a poster's paper: an invisible card the Interact key's line meets while it offers. extra_tags:
    what else reading it counts as (Ruth's note's card is CalderNote too, so Side 1's "read the note" counts a read through
    the card while it offers Calder's Caches)."""
    cls = actor_class('MissionArtifact')
    if cls is None:
        build.warn('no MissionArtifact class (build the game module first): no ' + tag)
        return
    frame = poster.get_actor_transform()
    at = frame.transform_location(unreal.Vector(CARD_PROUD, 0.0, 0.0))
    made = put(build, cls, at, poster.get_actor_rotation().yaw, tag, tags=(tag,) + tuple(extra_tags))
    made.set_actor_rotation(poster.get_actor_rotation(), False)
    made.set_editor_property('title', unreal.Text(title))
    made.set_editor_property('prompt', unreal.Text(prompt))
    made.set_editor_property('take_on_accept', False)
    card = made.get_editor_property('mesh')
    card.set_static_mesh(unreal.load_asset(ENGINE_CUBE))
    extent = unreal.Vector(1.0, 20.0, 28.0)
    try:
        extent = poster.get_editor_property('hitbox').get_scaled_box_extent()
    except Exception:
        pass
    card.set_relative_scale3d(unreal.Vector(0.01, (max(extent.y, 8.0) + CARD_MARGIN) * 2.0 / 100.0,
                                            (max(extent.z, 8.0) + CARD_MARGIN) * 2.0 / 100.0))
    card.set_editor_property('hidden_in_game', True)
    build.log(f'{tag} over {poster.get_actor_label()}{" (also " + ", ".join(extra_tags) + ")" if extra_tags else ""}')


def place_artifacts(build, found):
    gate = found.get('posterGate')
    note = found.get('calderNote')
    if gate is not None:
        place_artifact(build, gate, 'Artifact_PosterGate', 'A wanted poster', 'Read the poster')
    else:
        build.warn('no posterGate (gameplay.posters): no Artifact_PosterGate for Side 1')
    if note is not None:
        # Tagged as the note too (AWantedPoster::NoteTag): while it offers Calder's Caches it wins the key's focus over the
        # paper under it, so reading it there is Side 1's "read the note" as well.
        place_artifact(build, note, 'Artifact_CalderNote', "Ruth Calder's note", 'Read the note', extra_tags=('CalderNote',))
    else:
        build.warn('no calderNote (gameplay.posters): no Artifact_CalderNote for Calder\'s Caches')


def place_bridge_place(build, found):
    bridge = found.get('Bridge')
    if bridge is None:
        build.warn('no Keeper\'s Bridge placed: no Place_KeepersBridge')
        return
    middle = bridge.get_actor_location()
    deck = story.onto_host(bridge, unreal.Vector(middle.x, middle.y, middle.z + 300.0),
                           unreal.Vector(middle.x, middle.y, middle.z - 200.0))
    marker(build, deck if deck is not None else unreal.Vector(middle.x, middle.y, middle.z + 30.0), 'Place_KeepersBridge')


def band_main_street(build, found):
    street = found.get('SafeZone_MainStreet')
    if street is None:
        return
    z = street.get_actor_location().z
    street.set_editor_property('height_band', True)
    street.set_editor_property('min_z', z - BAND_BELOW)
    street.set_editor_property('max_z', z + BAND_ABOVE)
    build.log(f'Main Street\'s safe ground holds from {z - BAND_BELOW:.0f} to {z + BAND_ABOVE:.0f} (never the cellar under it)')


# ---------------------------------------------------------------------------
# All of it
# ---------------------------------------------------------------------------

def place(build):
    """Everything above (build_area.py's gameplay() calls this after the posters). Only Ransom's Rest has it."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    meshes = mesh_index()
    found = level_actors(build)
    steps = Steps(build)
    place_porch_plate(build, meshes, steps, found)
    place_lookout_stains(build, meshes, found)
    place_bail(build, meshes, found)
    place_chapel_roll(build, meshes, found)
    place_den_relics(build, meshes, found)
    place_sheriff(build, meshes, steps, found)
    place_cache_tags(build, meshes, found)
    place_artifacts(build, found)
    place_barn_chalk(build, meshes, found)
    place_depot(build, meshes, steps, found)
    place_coal(build, meshes, found)
    place_workshop(build, meshes, steps, found)
    place_bridge_place(build, found)
    band_main_street(build, found)


def run(name):
    """Places Gameplay/Valley again on its own, saves."""
    import build_area  # noqa: E402 (here: build_area imports build_area_story, which this module uses)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the valley\'s mission pieces placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_missionprops_valley.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
