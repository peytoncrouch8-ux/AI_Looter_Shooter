#pragma once

#include "CoreMinimal.h"
#include "Shields/ShieldTypes.h"

/**
 * A ward's words, for every screen that shows one (the HUD, the loadout's card, the ground label): its name (the trait's
 * word, then the build's: "Barbed Bulwark", "Ward Box"), its rarity's colour, its numbers and what its trait does.
 */
namespace ShieldText
{
	/** "Barbed Bulwark": the trait's word (none without one) and the build's name. */
	AI_LOOTER_SHOOTER_API FString Name(const FShieldInstanceData& Ward);

	/** The build's name: "Ward Box", "Bulwark", "Quickset". */
	AI_LOOTER_SHOOTER_API FString BuildName(EShieldBuild Build);

	/** The trait's name ("Barbwire", "Last Breath"), its word in a ward's name ("Barbed", "Gasping"). Empty for None. */
	AI_LOOTER_SHOOTER_API FString TraitName(EShieldTrait Trait);
	AI_LOOTER_SHOOTER_API FString TraitWord(EShieldTrait Trait);

	/** What the trait does, with this ward's own numbers ("Bites on the ward cut the biter for 42."). Empty for None. */
	AI_LOOTER_SHOOTER_API FString TraitLine(const FShieldInstanceData& Ward);

	/** Its rarity's colour (the definition's; white without one). */
	AI_LOOTER_SHOOTER_API FLinearColor Color(const FShieldInstanceData& Ward);

	/** The trait's line colour: Last Breath its element's, every other the kit's cyan. */
	AI_LOOTER_SHOOTER_API FLinearColor TraitColor(const FShieldInstanceData& Ward);

	/** "52", "2.9 s", "15 / s": the numbers as the cards print them. */
	AI_LOOTER_SHOOTER_API FString CapacityString(const FShieldStats& Stats);
	AI_LOOTER_SHOOTER_API FString DelayString(const FShieldStats& Stats);
	AI_LOOTER_SHOOTER_API FString RateString(const FShieldStats& Stats);

	/** "Ward Box · Lv 4": the kind and level, the line under a card's name. */
	AI_LOOTER_SHOOTER_API FString KindLine(const FShieldInstanceData& Ward);
}
