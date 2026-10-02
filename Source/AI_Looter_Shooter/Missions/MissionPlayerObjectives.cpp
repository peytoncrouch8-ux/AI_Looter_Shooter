#include "Missions/MissionPlayerObjectives.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/WeaponRack.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
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
