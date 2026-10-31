#include "Inventory/GunSlotRules.h"
#include "Missions/MissionDefinition.h"
#include "Session/CampaignRecord.h"
#include "Containers/ContainerAllocationPolicies.h"

FName GunSlotRules::StoryArea()
{
	// The missions' Area and the area asset's id (Tools/Unreal/create_mission_assets.py: area='RansomsRest').
	static const FName Area(TEXT("RansomsRest"));
	return Area;
}

int32 GunSlotRules::OpenSlots(const FGunSlotProgress& Progress)
{
	int32 Open = AlwaysOpen;
	Open += Progress.Level >= ThirdSlotLevel ? 1 : 0;
	Open += Progress.bStoryDone ? 1 : 0;
	return FMath::Min(Open, NumSlots);
}

EGunSlotLock GunSlotRules::LockOf(int32 SlotIndex, const FGunSlotProgress& Progress)
{
	const int32 Open = OpenSlots(Progress);
	if (SlotIndex < Open || SlotIndex >= NumSlots)
	{
		return EGunSlotLock::Open;
	}
	// The shut slots, first to last, each wait for the next thing still to come.
	TArray<EGunSlotLock, TInlineAllocator<2>> Pending;
	if (Progress.Level < ThirdSlotLevel)
	{
		Pending.Add(EGunSlotLock::Level);
	}
	if (!Progress.bStoryDone)
	{
		Pending.Add(EGunSlotLock::Story);
	}
	const int32 Index = SlotIndex - Open;
	return Pending.IsValidIndex(Index) ? Pending[Index] : EGunSlotLock::Story;
}

TArray<FName> GunSlotRules::StoryMissions(const TArray<TObjectPtr<UMissionDefinition>>& Definitions, FName Area)
{
	TArray<FName> Missions;
	for (const UMissionDefinition* Mission : Definitions)
	{
		if (Mission && Mission->Kind == EMissionKind::Main && !Area.IsNone() && Mission->Area == Area)
		{
			Missions.AddUnique(Mission->GetMissionId());
		}
	}
	return Missions;
}

bool GunSlotRules::IsStoryDone(const TArray<TObjectPtr<UMissionDefinition>>& Definitions, const FCampaignRecord& Campaign, FName Area,
	FName Except)
{
	// Read from the mission data every time (Main 1 to Main 7, Dead Lift and Back Pay today), so a main mission added to the
	// area later is waited for too.
	bool bAny = false;
	for (const UMissionDefinition* Mission : Definitions)
	{
		if (!Mission || Mission->Kind != EMissionKind::Main || Area.IsNone() || Mission->Area != Area)
		{
			continue;
		}
		bAny = true;
		const FName Id = Mission->GetMissionId();
		if (Id == Except || !Campaign.HasCompleted(Id))
		{
			return false;
		}
	}
	return bAny;
}

FText GunSlotRules::LockText(EGunSlotLock Lock)
{
	switch (Lock)
	{
	case EGunSlotLock::Level:
		return FText::Format(NSLOCTEXT("GunSlots", "UnlocksAtLevel", "Unlocks at level {0}"), FText::AsNumber(ThirdSlotLevel));
	case EGunSlotLock::Story:
		return NSLOCTEXT("GunSlots", "UnlocksAfterStory", "Unlocks after Ransom's Rest's story");
	default:
		return FText::GetEmpty();
	}
}

FText GunSlotRules::LockedKeyMessage(int32 SlotIndex, EGunSlotLock Lock)
{
	switch (Lock)
	{
	case EGunSlotLock::Level:
		return FText::Format(NSLOCTEXT("GunSlots", "KeyAtLevel", "Slot {0} unlocks at level {1}"), FText::AsNumber(SlotIndex + 1),
			FText::AsNumber(ThirdSlotLevel));
	case EGunSlotLock::Story:
		return FText::Format(NSLOCTEXT("GunSlots", "KeyAfterStory", "Slot {0} unlocks after Ransom's Rest's story"), FText::AsNumber(SlotIndex + 1));
	default:
		return FText::GetEmpty();
	}
}

FText GunSlotRules::OpenedByText(EGunSlotLock Was)
{
	switch (Was)
	{
	case EGunSlotLock::Level:
		return FText::Format(NSLOCTEXT("GunSlots", "OpenedByLevel", "Level {0} reached"), FText::AsNumber(ThirdSlotLevel));
	case EGunSlotLock::Story:
		return NSLOCTEXT("GunSlots", "OpenedByStory", "Ransom's Rest's story done");
	default:
		return FText::GetEmpty();
	}
}

FText GunSlotRules::CarryText(int32 SlotIndex)
{
	// Slot 3 opening means three guns carried, slot 4 four: the count reads better as a word on the banner.
	switch (SlotIndex + 1)
	{
	case 3: return NSLOCTEXT("GunSlots", "CarryThree", "Carry three guns");
	case 4: return NSLOCTEXT("GunSlots", "CarryFour", "Carry four guns");
	default: return FText::Format(NSLOCTEXT("GunSlots", "CarryN", "Carry {0} guns"), FText::AsNumber(SlotIndex + 1));
	}
}

FString GunSlotRules::FeedLine()
{
	return TEXT("+1 GUN SLOT");
}
