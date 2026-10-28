"""Makes the Supply Crates and Strongboxes saved in a level loot chest tiers (the user, 2026-10-27; AChest's
EChestKind::ChestTier1-5), by chest_id from chest_tiers.py, the same table the placement scripts use, so the level needn't
be rebuilt. Each keeps its chest_id (the session keeps opened chests by it), its place and its label; it gets its tier's
kind, no loot override, its models from the tier's kind (the Supply Crate's or the Strongbox's until SM_ChestTier<N> is
imported) and a ChestTier<N> tag. A chest already a tier is set to the table's tier again. Coffins, graves, mailboxes and
footlockers aren't touched. Needs the game module built with the tiers (unreal.ChestKind.CHEST_TIER1).

In the open editor (the level open, or named; nothing else unsaved):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/set_chest_tiers.py [/Game/Maps/Lvl_RansomsRest] [--dry]"
--dry lists what it would change and changes nothing. Otherwise it saves the level.
"""
import importlib
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import chest_tiers  # noqa: E402


def kind_name(value):
    """'SUPPLY_CRATE', 'STRONGBOX', 'CHEST_TIER3'... of a placed chest's kind."""
    return getattr(value, 'name', str(value).split('.')[-1].split(':')[0]).upper()


def open_level(path):
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not path or world.get_path_name().split('.')[0] == path:
        return world
    unsaved = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
               if not p.get_name().startswith('/Temp/')]
    if unsaved:
        raise RuntimeError(f'unsaved changes in {unsaved}: save or discard them first')
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    return unreal.EditorLevelLibrary.get_editor_world()


def run(path=None, dry=False):
    tiers = importlib.reload(chest_tiers)
    world = open_level(path)
    level = world.get_path_name().split('.')[0]
    chest_cls = getattr(unreal, 'Chest', None)
    if chest_cls is None:
        raise RuntimeError('no Chest class (build the game module first)')
    tiers.kind_enum(1)  # fails early, before anything changes, when the module has no tiers yet
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    changed = []
    for chest in [a for a in actors if isinstance(a, chest_cls)]:
        chest_id = str(chest.get_editor_property('chest_id'))
        kind = kind_name(chest.get_editor_property('kind'))
        if kind in tiers.OLD_KINDS:
            tier = tiers.tier_of(chest_id, tiers.OLD_KINDS[kind])
            # A loose strongbox of the lootable world the table doesn't name: common about the world.
            if chest_id.startswith('Strongbox_') and chest_id not in tiers.TIERS:
                tier = tiers.tier_of(chest_id) or tiers.LOOSE_TIER
        elif kind.startswith('CHEST_TIER'):
            tier = tiers.tier_of(chest_id) or int(kind[-1])
        else:
            continue
        was = kind.title().replace('_', '')
        if kind == f'CHEST_TIER{tier}' or kind == f'CHEST_TIER_{tier}':
            unreal.log(f'set_chest_tiers: {chest_id} ({chest.get_actor_label()}) already tier {tier}')
            continue
        changed.append(f'{chest_id} ({chest.get_actor_label()}): {was} -> tier {tier}')
        if dry:
            continue
        tiers.apply(chest, tier)
        # Set again last, as the placement scripts do: the chest is built again with its tier's models.
        chest.set_editor_property('chest_id', unreal.Name(chest_id))
    for line in changed:
        unreal.log(f'set_chest_tiers: {"would set " if dry else ""}{line}')
    unreal.log(f'set_chest_tiers: {level}: {len(changed)} chest(s) {"to change (dry run)" if dry else "changed"}')
    if changed and not dry:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        unreal.log(f'set_chest_tiers: {level} saved')
    return changed


if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    run(args[0] if args else None, dry='--dry' in sys.argv[1:])
