"""Makes (or brings up to date) the player's ward (SHIELD, 2026-10-22; Docs/Work/2026-10-22-Next/SHIELD.md):
  /Game/Data/Shields/DA_Shield_WardBox   the ward box's definition (UShieldDefinition): its name, its models per rarity
                                         (/Game/Art/Shields/SM_WardBox_<Rarity> from Art/Models/Shields/WardBox.py), the
                                         glow slot and the icon. Its numbers are the C++ class's defaults (BaseCapacity 40,
                                         a 3.2 s wait, 28% a second, the rarity table): this script never writes them, so
                                         the C++ stays where they're tuned.
and turns the wards on in the kill and chest loot tables (ULootTable::bDropsShields: Looter.Shield.Drop.Share of their gun
picks come as wards): DA_LootTable_Default (the chests copy it), the ranks' tables and the boss's (the boss loot shower
throws a dead boss's wards with its guns, the rarest last: Bosses/BossLootShower.cpp, SHIELD-POLISH 2026-10-22).
Run it in the open editor once the C++ with Shields/ is built and the ward box's models are imported
(Tools\\models.ps1 -Only WardBox):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_shields.py"
It prints SHIELDS lines and "SHIELDS done". Running it again changes nothing that's already so.
"""
import unreal

DATA = '/Game/Data/Shields'
ASSET = 'DA_Shield_WardBox'
MODELS = '/Game/Art/Shields'
RARITIES = (('COMMON', 'Common'), ('UNCOMMON', 'Uncommon'), ('RARE', 'Rare'), ('EPIC', 'Epic'), ('LEGENDARY', 'Legendary'))
LOOT = '/Game/Weapons/Data'
# The kill tables that drop wards (the boss's too, since its shower throws them).
WARD_TABLES = ('DA_LootTable_Default', 'DA_LootTable_Rare', 'DA_LootTable_Epic', 'DA_LootTable_Hallowed', 'DA_LootTable_Ethereal',
               'DA_LootTable_Legendary', 'DA_LootTable_Boss')


def log(message):
    unreal.log(f'SHIELDS {message}')


def definition():
    path = f'{DATA}/{ASSET}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.ShieldDefinition):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ShieldDefinition)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(ASSET, DATA, unreal.ShieldDefinition, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def make_definition():
    ward, made = definition()
    ward.set_editor_property('display_name', unreal.Text('Ward Box'))
    ward.set_editor_property('glow_slot', 'WardGlow')
    ward.set_editor_property('icon_name', 'WardBox')
    models = {}
    missing = []
    for enum_name, suffix in RARITIES:
        path = f'{MODELS}/SM_WardBox_{suffix}'
        mesh = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if mesh is None:
            missing.append(suffix)
            continue
        models[getattr(unreal.WeaponRarity, enum_name)] = mesh
    if models:
        ward.set_editor_property('models', models)
    if not unreal.EditorAssetLibrary.save_loaded_asset(ward, only_if_is_dirty=False):
        raise RuntimeError(f'{DATA}/{ASSET} could not be saved')
    log(f'{DATA}/{ASSET} {"made" if made else "kept"}; models: {len(models)} of {len(RARITIES)}'
        + (f' (missing {", ".join(missing)}: import Art/Models/Shields/WardBox.py first; the class finds them by path meanwhile)' if missing else ''))


def turn_on_wards():
    for name in WARD_TABLES:
        path = f'{LOOT}/{name}'
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            log(f'{path} not found: left alone')
            continue
        table = unreal.load_asset(path)
        if table.get_editor_property('drops_shields'):
            log(f'{name} drops wards already')
            continue
        table.set_editor_property('drops_shields', True)
        if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
            raise RuntimeError(f'{path} could not be saved')
        log(f'{name} drops wards now')


def run():
    if not hasattr(unreal, 'ShieldDefinition'):
        raise RuntimeError('unreal.ShieldDefinition is missing: build the C++ with Shields/ first; nothing was changed')
    make_definition()
    turn_on_wards()
    log('done')


run()
