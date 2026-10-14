// UWeaponManagerComponent: the player's guns never vanish unseen. On 2026-10-14 a play check lost every gun twice with no
// log line: the pawn's Blueprint was rebuilt mid-play (an editor property change on a live component reruns its
// construction script), which destroyed this component, and its guns with it. Now every way guns can go says so: the
// player's own (a drop, a sale, a load, a clear) in a log line, any other in a warning naming the path and the callstack.

#include "Inventory/WeaponManagerComponent.h"
#include "AI_Looter_Shooter.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AssertionMacros.h"
#include "Templates/UnrealTemplate.h"

namespace
{
	/** "Common DA_AssaultRifle (WeaponBase_6)": a gun as the log names it. */
	FString DescribeGun(const AWeaponBase& Gun)
	{
		const FWeaponInstanceData& Instance = Gun.GetInstance();
		return FString::Printf(TEXT("%s %s (%s)"), *StaticEnum<EWeaponRarity>()->GetNameStringByValue(static_cast<int64>(Instance.Rarity)),
			Instance.Definition ? *Instance.Definition->GetName() : TEXT("gun"), *Gun.GetName());
	}
}

void UWeaponManagerComponent::ReleaseGun(AWeaponBase* Gun)
{
	if (IsValid(Gun))
	{
		TGuardValue<bool> Releasing(bReleasingGun, true);
		Gun->Destroy();
	}
}

void UWeaponManagerComponent::NoteGunsGone(int32 CarriedBefore, const TCHAR* Path) const
{
	if (CarriedBefore > 0 && NumCarriedGuns() == 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Guns: %s carries none now (%s)."), *GetNameSafe(GetOwner()), Path);
	}
}

void UWeaponManagerComponent::WarnGunsLost(int32 Lost, const TCHAR* Path, const FString& Why) const
{
	if (Lost <= 0)
	{
		return;
	}
	UE_LOG(LogLooter, Warning, TEXT("Guns: %s lost %d gun(s) by %s: %s. It carries %d now."), *GetNameSafe(GetOwner()), Lost, Path, *Why,
		NumCarriedGuns());
	// Who called, so the next one is caught at once (not in tests: the walk loads every symbol, seconds each time).
	if (!GIsAutomationTesting)
	{
		FDebug::DumpStackTraceToLog(TEXT("Guns: the callstack of that loss"), ELogVerbosity::Log);
	}
}

void UWeaponManagerComponent::NoteGunDestroyed(AWeaponBase& Gun)
{
	const int32 Slot = Weapons.IndexOfByKey(&Gun);
	if (bReleasingGun || Slot == INDEX_NONE)
	{
		return;
	}
	// The slot lets it go first (the gun is already on its way out: not put away), so nothing holds a gun that's gone and
	// the warning counts what's left.
	const FString Gone = DescribeGun(Gun);
	RemoveFromSlots(Slot, /*bHolster*/ false);
	WarnGunsLost(1, TEXT("a gun destroyed while carried"), FString::Printf(TEXT("%s in slot %d, destroyed by something other than the weapon manager"),
		*Gone, Slot + 1));
}

void UWeaponManagerComponent::ReportEndPlay(EEndPlayReason::Type EndPlayReason) const
{
	const int32 Carried = NumCarriedGuns();
	// The level or play ending (travel, the menu, quitting, the play session stopped): the session saved what was carried.
	if (Carried == 0 || EndPlayReason != EEndPlayReason::Destroyed)
	{
		return;
	}
	const AActor* Owner = GetOwner();
	if (Owner && !Owner->IsActorBeingDestroyed())
	{
		// This component alone, its pawn playing on without it.
		const bool bRebuilt = GIsReconstructingBlueprintInstances || GIsReinstancing.load();
		WarnGunsLost(Carried, bRebuilt ? TEXT("the pawn's Blueprint rebuilt mid-play") : TEXT("the weapon manager destroyed on its own"),
			bRebuilt ? TEXT("an editor property change on the live pawn or one of its components (the Details panel, Python's set_editor_property) ")
				TEXT("reran its construction script, which made every Blueprint component anew, this one empty; the backpack and the ammo went too")
			: TEXT("its pawn lives on without its guns"));
		return;
	}
	const UWorld* World = GetWorld();
	if (bHeldByPlayer && World && World->IsGameWorld() && !World->bIsTearingDown)
	{
		WarnGunsLost(Carried, TEXT("the player's pawn destroyed mid-play"), TEXT("not a level change: fell out of the world, or destroyed by a script"));
	}
}
