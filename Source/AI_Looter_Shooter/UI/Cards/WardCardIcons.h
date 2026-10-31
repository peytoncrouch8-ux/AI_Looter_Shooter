#pragma once

#include "CoreMinimal.h"
#include "UI/Cards/WardCardRules.h"
#include "UI/Style/LooterUIStyle.h"

/**
 * The ward card's row glyphs, in the gun card's 16-unit box and line weights (GunCardIcons), drawn by the kit's IconBrush:
 * capacity a ward's shield outline, recharge delay an hourglass, recharge rate two chevrons rising, the build the belt box,
 * the trait a small spark.
 */
namespace WardCardIcons
{
	AI_LOOTER_SHOOTER_API const LooterUI::FVectorIcon& Stat(WardCardRules::EStat Stat);
	/** Its name for the brush cache ("WardCardStat_Capacity"). */
	AI_LOOTER_SHOOTER_API FName StatName(WardCardRules::EStat Stat);
	AI_LOOTER_SHOOTER_API const LooterUI::FVectorIcon& Build();
	AI_LOOTER_SHOOTER_API const LooterUI::FVectorIcon& Trait();
}
