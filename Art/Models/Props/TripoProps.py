"""The Wardens' Tripo props the game already looks for in /Game/Art/Props: Delia's shotgun (SM_DeliaShotgun: sockets Grip,
Foregrip and Muzzle, the barrel along +X; Story/DeliaRansomShotgun.cpp) and Abel's hat (SM_AbelsHat: story_texts
READABLE_MODELS['AbelsHat']). Built by Tools/Blender/looter_tripo_wardens.py; the rest of the Tripo models are
Art/Models/Tripo/Wardens.py."""
import looter_tripo_wardens as tw

for name in tw.names('Props'):
    obj, report = tw.build(name)
    tw.describe(name, report)
