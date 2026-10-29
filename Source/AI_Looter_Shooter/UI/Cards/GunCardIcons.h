#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterUIStyle.h"

/**
 * The tag card's small vector glyphs (the user's pick, 2026-10-28 evening, mockup Docs/Work/2026-10-28-UICharacter/mockup/
 * colors.png): one per stat on its row, and the element's at the card's foot. Drawn by the kit's IconBrush (16-unit box).
 */
namespace GunCardIcons
{
	/** The stat's glyph: damage a starburst, accuracy a crosshair, handling a chevron, reload a turning arrow, fire rate
	 * speed lines, magazine a box magazine, zoom a lens. */
	AI_LOOTER_SHOOTER_API const LooterUI::FVectorIcon& Stat(LoadoutRules::EStat Stat);
	/** Its name for the brush cache ("GunCardStat_Damage"). */
	AI_LOOTER_SHOOTER_API FName StatName(LoadoutRules::EStat Stat);

	/**
	 * The element's glyph as a brush of Size in Tint, the user's picks (2026-10-29, Docs/Archive/Work/2026-10-29-Embers):
	 * ember "Hearth" (a flame cut from its heart), soul-light "Sunburst", wardbreak "Cracked Ward", thunder "Bolt", rot "Skull",
	 * none "Empty Ring".
	 * False for none (the ring is still made).
	 */
	AI_LOOTER_SHOOTER_API bool ElementBrush(ELooterElement Element, const FVector2D& Size, const FLinearColor& Tint, FSlateBrush& OutBrush);

	/** The ornament's diamond between the stats and the lines. */
	AI_LOOTER_SHOOTER_API const LooterUI::FVectorIcon& Diamond();
}
