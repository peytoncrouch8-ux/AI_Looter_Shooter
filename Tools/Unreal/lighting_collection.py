"""MPC_Lighting, the material parameter collection the lighting states write (ULightingStateSubsystem, step 13 of
Docs/Areas/RansomsRest.md) for the materials that can't see the light themselves:

  BackdropTint     multiplies every backdrop layer's own tint (M_Backdrop reads it): dusk darkens and warms the unlit
                   ranges, which would otherwise stay bright at sunset.
  CloudTint        for the painted clouds (M_SkyClouds), once they read it.
  FogInscattering  the haze's color and its glow toward the sun, for effects that must match the fog (wisp cards along
  FogDirectional   the Rim, the cloud bank).

The defaults leave everything as built (white tints, the tutorial island's haze); a level's states write their own
values as the level starts and at each switch. The names are ULightingStateSubsystem's *Parameter constants.

build_world_materials.py makes or updates it before M_Backdrop, which reads it. Materials find a collection's parameter
by its id, so an existing parameter keeps its entry (only its default is set again) and missing ones are added.
"""
import unreal

PATH = '/Game/Art/Materials/MPC_Lighting'
VECTORS = (
    ('BackdropTint', (1.0, 1.0, 1.0, 1.0)),
    ('CloudTint', (1.0, 1.0, 1.0, 1.0)),
    ('FogInscattering', (0.20, 0.29, 0.44, 1.0)),
    ('FogDirectional', (0.0, 0.0, 0.0, 1.0)),
)


def collection():
    """MPC_Lighting with every parameter the lighting states write, saved; returns it."""
    folder, name = PATH.rsplit('/', 1)
    if unreal.EditorAssetLibrary.does_asset_exist(PATH):
        mpc = unreal.load_asset(PATH)
    else:
        mpc = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    params = list(mpc.get_editor_property('vector_parameters'))
    by_name = {str(p.get_editor_property('parameter_name')): p for p in params}
    for key, value in VECTORS:
        param = by_name.get(key)
        if param is None:
            param = unreal.CollectionVectorParameter()
            param.set_editor_property('parameter_name', key)
            params.append(param)
        param.set_editor_property('default_value', unreal.LinearColor(*value))
    # Set as a whole and always announced, so the collection rebuilds its layout and the materials reading it recompile
    # even when only defaults changed (the entries read above may already be the collection's own).
    mpc.set_editor_property('vector_parameters', params, unreal.PropertyAccessChangeNotifyMode.ALWAYS)
    unreal.EditorAssetLibrary.save_loaded_asset(mpc)
    unreal.log(f'LOOTER lighting collection: {mpc.get_path_name()} ({", ".join(key for key, _ in VECTORS)})')
    return mpc
