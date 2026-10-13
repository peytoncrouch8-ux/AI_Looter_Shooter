#include "Missions/MissionPlayerObjectives.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionTargets.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "World/MissionItem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

// --- Collect ---

void UMissionCollectObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	if (What != EMissionCollect::Weapons || !Context.Player)
	{
		return;
	}
	// Guns in the slots, as the tutorial counted them; one put down before the count was reached doesn't count.
	const UWeaponManagerComponent* Manager = Context.Player->FindComponentByClass<UWeaponManagerComponent>();
	State.Count = Manager ? Manager->GetWeapons().Num() : 0;
}

bool UMissionCollectObjective::HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const
{
	if (What != EMissionCollect::Items || Happened.Name != FMissionEvent::Collect || !Happened.Matches(Item))
	{
		return false;
	}
	const FName Key = Happened.ThingKey();
	if (!Key.IsNone())
	{
		if (State.Used.Contains(Key))
		{
			return false;
		}
		State.Used.Add(Key);
	}
	++State.Count;
	return true;
}

FMissionActorFilter UMissionCollectObjective::GetTargets() const
{
	return What == EMissionCollect::Items ? Item : FMissionActorFilter();
}

FVector UMissionCollectObjective::GetWaypointOf(const AActor& Found) const
{
	if (const AWeaponRack* Rack = What == EMissionCollect::Weapons ? Cast<AWeaponRack>(&Found) : nullptr)
	{
		const AWeaponBase* Offered = Rack->IsWeaponOffered() ? Rack->GetOfferedWeapon() : nullptr;
		return Offered ? Offered->GetActorLocation() : Rack->GetActorLocation();
	}
	return Super::GetWaypointOf(Found);
}

FString UMissionCollectObjective::DescribeRule() const
{
	if (What == EMissionCollect::Weapons)
	{
		return GetRequired() > 1 ? FString::Printf(TEXT("Carry %d guns"), GetRequired()) : FString(TEXT("Take a gun"));
	}
	return FString::Printf(TEXT("Collect %d %s"), GetRequired(), *Item.Describe());
}

// --- Collect mission items ---

int32 UMissionCollectItemObjective::CountCarried(const FMissionContext& Context) const
{
	const UMissionRunner* Runner = Context.World ? Context.World->GetSubsystem<UMissionRunner>() : nullptr;
	if (!Runner)
	{
		return 0;
	}
	int32 Carried = 0;
	for (const FName ItemId : Items)
	{
		Carried += Runner->IsCarrying(ItemId) ? 1 : 0;
	}
	return Carried;
}

void UMissionCollectItemObjective::Begin(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	State.Count = CountCarried(Context);
}

void UMissionCollectItemObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	State.Count = CountCarried(Context);
}

bool UMissionCollectItemObjective::HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const
{
	// A pickup counts at once rather than at the next look.
	if (Happened.Name != FMissionEvent::Collect)
	{
		return false;
	}
	const int32 Before = State.Count;
	State.Count = CountCarried(Context);
	return State.Count != Before;
}

TOptional<FVector> UMissionCollectItemObjective::GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	const UMissionRunner* Runner = Context.World ? Context.World->GetSubsystem<UMissionRunner>() : nullptr;
	const UMissionDefinition* Mission = GetTypedOuter<UMissionDefinition>();
	if (!Runner)
	{
		return TOptional<FVector>();
	}
	const TOptional<FVector> From = Context.GetPlayerLocation();
	TOptional<FVector> Best;
	double BestDistance = TNumericLimits<double>::Max();
	auto Consider = [&](const FVector& Where)
	{
		const double Distance = From.IsSet() ? FVector::DistSquared2D(Where, *From) : 0.0;
		if (!Best.IsSet() || Distance < BestDistance)
		{
			Best = Where;
			BestDistance = Distance;
		}
	};
	for (const FName ItemId : Items)
	{
		if (Runner->IsCarrying(ItemId))
		{
			continue;
		}
		// Lying there to take (as its mission stands now, which the item may not have looked at yet); else on whoever carries
		// it, alive.
		const AMissionItem* Lying = AMissionItem::FindIn(Context.World, ItemId);
		if (Lying && Lying->ShouldShow())
		{
			Consider(Lying->GetActorLocation());
			continue;
		}
		const FMissionItemInfo* Info = Mission ? Mission->FindItem(ItemId) : nullptr;
		if (Info && Info->DroppedBy.IsSet())
		{
			if (const AActor* Carrier = MissionTargets::FindNearest(Context.World, Info->DroppedBy, From, /*bLivingOnly*/ true))
			{
				Consider(Carrier->GetActorLocation());
			}
		}
	}
	return Best;
}

FString UMissionCollectItemObjective::DescribeRule() const
{
	// "Find the winch crank", or "Collect 3 things" when there are several (the data names them in its own words).
	const UMissionDefinition* Mission = GetTypedOuter<UMissionDefinition>();
	const FMissionItemInfo* First = Mission && Items.Num() == 1 ? Mission->FindItem(Items[0]) : nullptr;
	if (First && !First->Name.IsEmpty())
	{
		return TEXT("Find ") + First->Name.ToString();
	}
	return Items.Num() == 1 ? FString::Printf(TEXT("Find %s"), *FName::NameToDisplayString(Items[0].ToString(), false))
		: FString::Printf(TEXT("Collect %d things"), GetRequired());
}

// --- Open an inventory page ---

UMissionOpenPageObjective::UMissionOpenPageObjective()
{
	// The inventory opens anywhere: nothing to point at.
	Waypoint = EMissionWaypoint::None;
}

void UMissionOpenPageObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	const ALooterHUD* HUD = Context.Controller ? Cast<ALooterHUD>(Context.Controller->GetHUD()) : nullptr;
	if (!HUD || !HUD->IsInventoryOpen())
	{
		return;
	}
	const EInventoryPage Shown = HUD->GetInventoryPage();
	const bool bOnPage = Page == EMissionPage::Any
		|| (Page == EMissionPage::Loadout && Shown == EInventoryPage::Loadout)
		|| (Page == EMissionPage::Bestiary && Shown == EInventoryPage::Bestiary)
		|| (Page == EMissionPage::Missions && Shown == EInventoryPage::Missions);
	if (bOnPage)
	{
		State.Count = 1;
	}
}

FString UMissionOpenPageObjective::DescribeRule() const
{
	switch (Page)
	{
	case EMissionPage::Loadout:  return TEXT("Open your loadout ({Inventory})");
	case EMissionPage::Bestiary: return TEXT("Open the bestiary ({Inventory}, then 2)");
	case EMissionPage::Missions: return TEXT("Open your missions ({Inventory}, then 3)");
	case EMissionPage::Any:      break;
	}
	return TEXT("Open the inventory ({Inventory})");
}
