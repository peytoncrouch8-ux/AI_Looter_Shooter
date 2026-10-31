#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

class UMissionDefinition;
struct FCampaignRecord;

/** What keeps an equip slot shut (GunSlotRules::LockOf): nothing, the player's level, or the story area's main missions. */
enum class EGunSlotLock : uint8
{
	Open,
	/** Opens at GunSlotRules::ThirdSlotLevel. */
	Level,
	/** Opens once every main mission of GunSlotRules::StoryArea is finished. */
	Story,
};

/** A slot just opened for the player, earned (UWeaponManagerComponent::OnSlotUnlocked): the slot (from 0) and what had shut it. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGunSlotUnlocked, int32 /*SlotIndex*/, EGunSlotLock /*Was*/);

/** What the player's slots open by: their level, and whether the story area's main missions are all finished. */
struct FGunSlotProgress
{
	int32 Level = 1;
	bool bStoryDone = false;
};

/**
 * The equip slots the player has open (the user, 2026-10-31: "2 slots available at the start of the game, 3rd slot available
 * at level 3, and fourth slot available after finishing all of Ransom's Rest's main missions"). Plain rules, no world: the
 * weapon manager reads the level and the campaign and asks these. Slots fill from the first, so the open ones are always
 * the first OpenSlots; a gun an older save put in a slot that is shut now keeps it until it leaves (the manager's rule).
 */
namespace GunSlotRules
{
	/** Every equip slot there is (UWeaponManagerComponent::MaxWeapons, the HUD's column, the slot keys 1-4). */
	inline constexpr int32 NumSlots = 4;
	/** Open from the start. */
	inline constexpr int32 AlwaysOpen = 2;
	/** The third opens at this level. */
	inline constexpr int32 ThirdSlotLevel = 3;

	/** The area whose main missions open the fourth: Ransom's Rest's id (UAreaDefinition::GetAreaId, a mission's Area). */
	AI_LOOTER_SHOOTER_API FName StoryArea();

	/** How many slots are open: AlwaysOpen, one more from ThirdSlotLevel, one more with the story done. */
	AI_LOOTER_SHOOTER_API int32 OpenSlots(const FGunSlotProgress& Progress);

	/**
	 * What keeps slot SlotIndex (from 0) shut: Open for an open one (and for no slot at all, outside 0..NumSlots-1). The shut
	 * slots wait in order for what is still to come, the level first (it comes early in Ransom's Rest), then the story.
	 */
	AI_LOOTER_SHOOTER_API EGunSlotLock LockOf(int32 SlotIndex, const FGunSlotProgress& Progress);

	/** The main missions of Area among Definitions, by id, in the definitions' order (the story the fourth slot waits for). */
	AI_LOOTER_SHOOTER_API TArray<FName> StoryMissions(const TArray<TObjectPtr<UMissionDefinition>>& Definitions, FName Area);

	/**
	 * Every main mission of Area is finished in Campaign. False when Definitions has none of Area's (missions not known yet:
	 * never opened by accident). Except is counted as not finished (the slot's state from just before that mission ended).
	 */
	AI_LOOTER_SHOOTER_API bool IsStoryDone(const TArray<TObjectPtr<UMissionDefinition>>& Definitions, const FCampaignRecord& Campaign, FName Area,
		FName Except = NAME_None);

	/** A shut slot's line: "Unlocks at level 3", "Unlocks after Ransom's Rest's story" (empty for Open). */
	AI_LOOTER_SHOOTER_API FText LockText(EGunSlotLock Lock);

	/** The message as a shut slot's key is pressed: "Slot 4 unlocks after Ransom's Rest's story". */
	AI_LOOTER_SHOOTER_API FText LockedKeyMessage(int32 SlotIndex, EGunSlotLock Lock);

	/** What opened it, for the banner's last line: "Level 3 reached", "Ransom's Rest's story done". */
	AI_LOOTER_SHOOTER_API FText OpenedByText(EGunSlotLock Was);

	/** The banner's reward line once SlotIndex opens: "Carry three guns" (the slots open then). */
	AI_LOOTER_SHOOTER_API FText CarryText(int32 SlotIndex);

	/** The pickup feed's line as a slot opens. */
	AI_LOOTER_SHOOTER_API FString FeedLine();
}
