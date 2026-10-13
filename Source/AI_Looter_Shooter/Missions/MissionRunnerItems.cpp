// UMissionRunner: mission items (FMissionItemInfo). Picked up (AMissionItem) or dropped by the creature carrying one as it
// dies, kept by the campaign while their mission runs (FCampaignRecord::CarriedItems) and taken away as it finishes
// (MissionRunnerRewards.cpp). One dropped and lost to a level change before it was picked up is given, so a mission never
// waits on a thing that's nowhere.

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "World/MissionItem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// ---------------------------------------------------------------------------
// Carrying
// ---------------------------------------------------------------------------

const UMissionDefinition* UMissionRunner::FindItemMission(FName ItemId, const FMissionItemInfo** OutItem) const
{
	if (ItemId.IsNone())
	{
		return nullptr;
	}
	for (const UMissionDefinition* Mission : Definitions)
	{
		if (const FMissionItemInfo* Item = Mission ? Mission->FindItem(ItemId) : nullptr)
		{
			if (OutItem)
			{
				*OutItem = Item;
			}
			return Mission;
		}
	}
	return nullptr;
}

bool UMissionRunner::IsCarrying(FName ItemId) const
{
	return GetCampaign().IsCarrying(ItemId);
}

bool UMissionRunner::GiveItem(FName ItemId, AActor* From)
{
	const UMissionDefinition* Mission = FindItemMission(ItemId);
	if (!bActive || !Mission || IsCompleted(Mission->GetMissionId()) || !GetCampaign().CarryItem(ItemId))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Mission %s: %s carried."), *Mission->GetMissionId().ToString(), *ItemId.ToString());
	AskToSave();
	bChanged = true;
	// The missions hear it as a pickup (a collect objective of its kind, or of its tag); with no actor, the item's id stands for it.
	FMissionEvent Pickup = FMissionEvent::Collected(From);
	Pickup.Tag = From ? NAME_None : AMissionItem::TagFor(ItemId);
	NotifyEvent(Pickup);
	return true;
}

TArray<FMissionItemInfo> UMissionRunner::GetCarriedItems(FName MissionId) const
{
	TArray<FMissionItemInfo> Carried;
	const UMissionDefinition* Mission = FindDefinition(MissionId);
	if (!Mission)
	{
		return Carried;
	}
	// In the order they were picked up, as the campaign keeps them.
	for (const FName ItemId : GetCampaign().CarriedItems)
	{
		if (const FMissionItemInfo* Item = Mission->FindItem(ItemId))
		{
			Carried.Add(*Item);
		}
	}
	return Carried;
}

// ---------------------------------------------------------------------------
// Dropped by their carrier
// ---------------------------------------------------------------------------

void UMissionRunner::DropCarriedItems(AActor& Victim)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FCampaignRecord& Campaign = GetCampaign();
	for (const UMissionDefinition* Mission : Definitions)
	{
		if (!Mission || Mission->Items.IsEmpty() || Campaign.HasCompleted(Mission->GetMissionId()))
		{
			continue;
		}
		for (const FMissionItemInfo& Item : Mission->Items)
		{
			// Its carrier, and it hasn't fallen before (her first kill drops the fang; one called back from her vigil stone
			// carries nothing more) nor been picked up.
			if (!Item.DroppedBy.IsSet() || !Item.DroppedBy.Matches(&Victim) || Campaign.IsCarrying(Item.Id)
				|| Campaign.DroppedItems.Contains(Item.Id) || AMissionItem::FindIn(World, Item.Id))
			{
				continue;
			}
			if (AMissionItem::SpawnDrop(World, Item.Id, Victim.GetActorLocation()))
			{
				Campaign.DroppedItems.AddUnique(Item.Id);
				AskToSave();
				bChanged = true;
				UE_LOG(LogLooter, Log, TEXT("Mission %s: %s fell from %s."), *Mission->GetMissionId().ToString(), *Item.Id.ToString(),
					*Victim.GetActorNameOrLabel());
			}
		}
	}
}

void UMissionRunner::RecoverLostItems()
{
	FCampaignRecord& Campaign = GetCampaign();
	if (Campaign.DroppedItems.IsEmpty())
	{
		return;
	}
	// A copy: giving one writes the record.
	const TArray<FName> Dropped = Campaign.DroppedItems;
	for (const FName ItemId : Dropped)
	{
		const UMissionDefinition* Mission = FindItemMission(ItemId);
		// Only for a mission under way in this level: one of another area's waits there.
		if (!Mission || Campaign.IsCarrying(ItemId) || !IsRunning(Mission->GetMissionId()) || AMissionItem::FindIn(GetWorld(), ItemId))
		{
			continue;
		}
		UE_LOG(LogLooter, Log, TEXT("Mission %s: %s fell where it can't be found now (the level was left before it was picked up): given."),
			*Mission->GetMissionId().ToString(), *ItemId.ToString());
		GiveItem(ItemId);
	}
}
