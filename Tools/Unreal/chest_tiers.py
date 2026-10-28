"""The loot chest tiers (the user, 2026-10-27: five tiers, 1 worst to 5 best, in place of the Supply Crate and the
Strongbox; AChest's EChestKind::ChestTier1-5, Source/AI_Looter_Shooter/Loot/Chest.h, numbers in ChestKinds.cpp): which tier
each placed crate or strongbox is, by its chest_id. One table for the placement scripts (build_area_caches.place_chest,
which the camps, the mountain fights, the mountain mission props and Providence call; build_area_loot; build_area_secrets)
and for set_chest_tiers.py (the chests already saved in a level), so a rebuild and a level fixed in place agree.

Placement rule (the user's): tiers 1-2 common about the world, 3-4 guarded or hidden, 5 bosses' and secrets' only.
A tiered chest has no loot override (its tier is its loot), its models come from its kind (SM_ChestTier<N>, or the Supply
Crate's for 1-2 and the Strongbox's for 3-5 until those are imported), and it carries a ChestTier<N> tag. Never rename a
chest_id: the session keeps opened chests by it (FSavedMapWorld::OpenedChests), whatever their kind.
"""
import unreal

# By chest_id.
TIERS = {
    # Ransom's Rest. Ruth's Ranger caches (build_area_caches.py): the windmill's in plain sight; the Sink rim's by the
    # Gravemother's den; the bluff path's up the lookout's stair (a climb, a view).
    'RangerCache_Windmill': 2,
    'RangerCache_SinkRim': 3,
    'RangerCache_BluffPath': 3,
    # The gang's Strongbox in the sheriff's office (Calder's Caches; opens from its step 1).
    'GangStrongbox': 4,
    # The encounters' camps' crates (build_area_camps.py, build_area_fights_mountain.py): guarded by the camp.
    'CampCache_OrchardEdge': 3,
    'CampCache_SheepFold': 3,
    'CampCache_MillCreek': 3,
    'CampCache_DinnerPail': 3,
    # Calder's fourth cache at the mountain post (build_area_missionprops_mountain.py; opens after Last Watch's fight).
    'CalderCache_Post': 3,
    # The company safe in the Foreman's office, deep in Providence among its dead (build_area_providence.py).
    'CompanySafe': 4,
    # The legendary chance, hidden under the middle Widow (story_texts.SECRETS WidowsBox).
    'Secret_WidowsStrongbox': 5,
}

# By chest_id's start (the lootable world's ids carry their spot: build_area_loot.spot_id), first match wins.
PREFIXES = (
    # build_area_loot.py's loose strongboxes: behind the saloon, in town; in the Dry Wash under Keeper's Bridge, found
    # looking down from the bridge.
    ('Strongbox_saloonBacklot_', 1),
    ('Strongbox_keepersBridge_', 2),
    # Any other camp's crate.
    ('CampCache_', 3),
)

# An old kind the tables don't name (a new placement): the Supply Crate as tier 2, the Strongbox as tier 4.
DEFAULTS = {'SupplyCrate': 2, 'Strongbox': 4}
# A loose strongbox of the lootable world's the tables don't name (build_area_loot.py): common about the world.
LOOSE_TIER = 2

# AChest's old kinds as Python names them, back to the names above.
OLD_KINDS = {'SUPPLY_CRATE': 'SupplyCrate', 'STRONGBOX': 'Strongbox'}

TIER_TAG = 'ChestTier'


def tier_of(chest_id, kind=None):
    """The tier for a chest by its id (TIERS, then PREFIXES), else by its old kind (DEFAULTS: 'SupplyCrate', 'Strongbox'),
    else None (a coffin, a grave, a mailbox, a footlocker: they stay as they are)."""
    chest_id = str(chest_id)
    if chest_id in TIERS:
        return TIERS[chest_id]
    for start, tier in PREFIXES:
        if chest_id.startswith(start):
            return tier
    return DEFAULTS.get(kind)


def kind_enum(tier):
    """unreal.ChestKind's tier value (Python spells ChestTier1 CHEST_TIER1; CHEST_TIER_1 tried too)."""
    for name in (f'CHEST_TIER{tier}', f'CHEST_TIER_{tier}'):
        value = getattr(unreal.ChestKind, name, None)
        if value is not None:
            return value
    raise RuntimeError(f'unreal.ChestKind has no tier {tier} (build the game module with the chest tiers first)')


def apply(chest, tier):
    """Makes a placed AChest the tier: its kind, no loot override, its own (or stand-in) models from the kind, its tier tag.
    The caller sets chest_id after (setting it last rebuilds the chest with its kind's models)."""
    chest.set_editor_property('kind', kind_enum(tier))
    chest.set_editor_property('loot_override', unreal.ChestLootOverride())
    # Its parts follow the kind (AChest::ApplyKind keeps a model a script chose that isn't some kind's own): cleared, so the
    # rebuild gives them the tier's.
    for part in ('body', 'lid', 'wheel'):
        component = chest.get_editor_property(part)
        if component is not None:
            component.set_static_mesh(None)
    tags = [t for t in chest.tags if not str(t).startswith(TIER_TAG)]
    chest.set_editor_property('tags', tags + [unreal.Name(f'{TIER_TAG}{tier}')])
    return chest
