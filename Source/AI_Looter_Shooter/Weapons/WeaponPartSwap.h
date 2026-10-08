#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponPartSwap.generated.h"

class UWeaponDefinition;
struct FWeaponPartOption;

/** A part kept in the bench's parts box: the kind of gun it fits, its slot, its key. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FBoxedWeaponPart
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UWeaponDefinition> Definition = nullptr;

	UPROPERTY()
	FName Slot;

	/** As the gun saved it (FWeaponInstanceData::Parts): its numbers are read from the parts list, so balance reaches it. */
	UPROPERTY()
	FName Key;

	bool operator==(const FBoxedWeaponPart& Other) const;
};

/**
 * Part swapping at a gunsmith's bench: scrap a gun and keep one of its parts, fit a kept part onto a gun of the same
 * kind, and the part it replaces goes into the box. The gun keeps its seed, rarity, level, paint, notches and curse; a
 * part's numbers fall where the gun's seed puts them in its ranges (WeaponParts::CombinedStats), as for the parts it
 * rolled with, so moving a part back and forth never rerolls it. The word in the gun's name follows the parts'
 * NamePriority, so a swap can rename it.
 */
namespace WeaponPartSwap
{
	enum class ECheck : uint8 { Ok, NamedGun, WrongKind, NoSuchSlot, NoSuchPart, RarityTooLow, NeedsOtherPart, BreaksOtherPart, SamePart };

	/** A rolled gun (never a named gun or a signature legendary) can be scrapped and fitted. */
	AI_LOOTER_SHOOTER_API bool CanModify(const FWeaponInstanceData& Gun);
	/** Whether Part fits Gun: same kind, the slot exists, the gun's rarity allows it (MinRarity), its Requires met, and no
	 *  later part's Requires broken by it. */
	AI_LOOTER_SHOOTER_API ECheck CanFit(const FWeaponInstanceData& Gun, const FBoxedWeaponPart& Part);
	/** Fits it: one key in Gun.Parts changes, the part it replaces comes out (OutReplaced; Key none if the slot was empty),
	 *  Gun.Stats recomputed. Seed, rarity, level, paint, kills and curse stay. False (nothing changed) unless CanFit is Ok. */
	AI_LOOTER_SHOOTER_API bool Fit(FWeaponInstanceData& Gun, const FBoxedWeaponPart& Part, FBoxedWeaponPart& OutReplaced);
	/** The parts a gun could leave in the box when scrapped: one per filled slot. Empty when it can't be modified. */
	AI_LOOTER_SHOOTER_API TArray<FBoxedWeaponPart> ScrapChoices(const FWeaponInstanceData& Gun);
	/** Gun's stats with Part fitted, for the bench's comparison. */
	AI_LOOTER_SHOOTER_API FWeaponStats PreviewStats(const FWeaponInstanceData& Gun, const FBoxedWeaponPart& Part);
	/** The part's option in its definition, or null. */
	AI_LOOTER_SHOOTER_API const FWeaponPartOption* FindOption(const FBoxedWeaponPart& Part);
	/** Why a part doesn't fit, in a few words for the bench ("NEEDS A LONGER BARREL"). */
	AI_LOOTER_SHOOTER_API FText CheckText(ECheck Check);
}
