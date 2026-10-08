#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponNotches.h"
#include "World/LightBeam.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// What the gun ideas do to a gun in play: its notches (a kill counted, a milestone reached, a curse lifted) and a cursed
// iron's guttering loot beam. The rules are WeaponNotches' and WeaponCurses'; the curses' other effects are in
// WeaponCurseEffects, firing and reloading.

// ---------------------------------------------------------------------------
// Notches
// ---------------------------------------------------------------------------

void AWeaponBase::AddKill()
{
	const int32 MarksBefore = WeaponNotches::Marks(Instance.Kills);
	const WeaponNotches::FKillResult Result = WeaponNotches::AddKill(Instance);
	UE_LOG(LogLooter, Verbose, TEXT("%s: %d notches"), *GetName(), Instance.Kills);

	// The tally in the stock (and a soul-forged gun's soul-light) only change every few kills.
	if (WeaponNotches::Marks(Instance.Kills) != MarksBefore || Result.Reached != ENotchTier::None)
	{
		Model->ShowNotches(Instance);
	}
	if (Result.Reached == ENotchTier::None && !Result.bCurseLifted)
	{
		return;
	}

	// A milestone's damage and a lifted curse's recoil come from the instance, as they do whenever a gun is made: rebuilt,
	// never patched, so a saved gun loads with exactly these numbers.
	Instance.Stats = UWeaponRollLibrary::ComputeInstanceStats(Instance);
	UWeaponManagerComponent* Inventory = GetHolderInventory();
	if (Result.Reached != ENotchTier::None)
	{
		const FText Message = WeaponNotches::MilestoneMessage(Instance, Result.Reached);
		UE_LOG(LogLooter, Log, TEXT("%s"), *Message.ToString());
		if (Inventory)
		{
			Inventory->OnMessage.Broadcast(Message);
		}
		LooterSound::Play2D(this, LooterSoundCue::NotchMilestone);
	}
	if (Result.bCurseLifted)
	{
		const FText Message = WeaponNotches::CurseLiftedMessage(Instance);
		UE_LOG(LogLooter, Log, TEXT("%s"), *Message.ToString());
		if (Inventory)
		{
			Inventory->OnMessage.Broadcast(Message);
		}
		LooterSound::Play2D(this, LooterSoundCue::CurseLifted);
		// The drawback on its holder (Grasping's lower max health) goes with it.
		WeaponCurseEffects::RefreshHolder(GetOwner());
	}
	// The cards and the loadout show the gun's new name (a nickname once Named) and numbers.
	if (Inventory)
	{
		Inventory->OnInventoryChanged.Broadcast();
	}
}

AWeaponBase* AWeaponBase::FindKillWeapon(const AActor* Victim)
{
	const UHealthComponent* Health = Victim ? Victim->FindComponentByClass<UHealthComponent>() : nullptr;
	AActor* Causer = Health ? Health->GetLastDamageCauser() : nullptr;
	if (AWeaponBase* Gun = Cast<AWeaponBase>(Causer))
	{
		return Gun;
	}
	// Something without a gun of its own dealt it (the holder, or something they own): the gun in their hand made the kill.
	for (const AActor* Actor = Causer; IsValid(Actor); Actor = Actor->GetOwner())
	{
		if (const UWeaponManagerComponent* Manager = Actor->FindComponentByClass<UWeaponManagerComponent>())
		{
			return Manager->GetActiveWeapon();
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// A cursed iron's loot beam
// ---------------------------------------------------------------------------

bool AWeaponBase::IsBeamGuttering() const
{
	return bIsPickup && LootBeam && LootBeam->IsVisible() && WeaponCurses::Of(Instance) != nullptr;
}

void AWeaponBase::UpdateBeamGutter()
{
	// Only while someone could see it flicker.
	const UWorld* World = GetWorld();
	if (!World || !IsBeamGuttering() || !LootBeam->WasRecentlyRendered(0.25f))
	{
		return;
	}
	// Each gun on its own beat (its seed), and a clock kept short so the flicker's noise keeps its precision.
	const float Phase = static_cast<float>(static_cast<uint32>(Instance.Seed) % 997u) * 0.37f;
	const float Time = static_cast<float>(FMath::Fmod(World->GetTimeSeconds(), 3600.0)) + Phase;
	LightBeams::Gutter(LootBeam, Time, BeamGlow, BeamHeight, BeamRadius);
}
