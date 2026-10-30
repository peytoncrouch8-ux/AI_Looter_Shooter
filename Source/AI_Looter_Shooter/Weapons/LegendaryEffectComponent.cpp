#include "Weapons/LegendaryEffectComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/LegendaryRules.h"
#include "Weapons/LegendaryTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A Company Scrip stack's coin: the silver pickup's chime, quiet, a little higher each stack. */
	constexpr float StackVolume = 0.45f;
	constexpr float StackPitchStep = 0.05f;
	/** A reload's pay: the same coins, lower, the pay counted out. */
	constexpr float RefundVolume = 0.5f;
	constexpr float RefundPitch = 0.85f;
}

ULegendaryEffectComponent::ULegendaryEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

ULegendaryEffectComponent* ULegendaryEffectComponent::Find(const AActor* Weapon)
{
	return Weapon ? Weapon->FindComponentByClass<ULegendaryEffectComponent>() : nullptr;
}

ULegendaryEffectComponent* ULegendaryEffectComponent::For(AWeaponBase& Weapon)
{
	if (ULegendaryEffectComponent* Existing = Find(&Weapon))
	{
		return Existing;
	}
	ULegendaryEffectComponent* Added = NewObject<ULegendaryEffectComponent>(&Weapon, TEXT("LegendaryEffects"));
	Added->RegisterComponent();
	return Added;
}

float ULegendaryEffectComponent::FireRateScaleOf(const AWeaponBase& Weapon)
{
	const ULegendaryEffectComponent* Legendary = Find(&Weapon);
	if (!Legendary || Legendary->GetSignature() != ELegendarySignature::CompanyScrip)
	{
		return 1.f;
	}
	return LegendaryRules::ScripFireRateScale(Legendary->GetStacks(Legendary->Now()), FLegendaryTuning::Live());
}

void ULegendaryEffectComponent::EndBurstOf(const AWeaponBase* Weapon)
{
	if (ULegendaryEffectComponent* Legendary = Find(Weapon))
	{
		Legendary->EndBurst();
	}
}

int32 ULegendaryEffectComponent::RefundReload(AWeaponBase& Weapon, UWeaponManagerComponent* Inventory, int32 RoundsTaken)
{
	if (!Inventory || RoundsTaken <= 0 || LegendaryRules::SignatureOf(Weapon.GetInstance()) != ELegendarySignature::CompanyScrip)
	{
		return 0;
	}
	const int32 Owed = LegendaryRules::ScripRefund(RoundsTaken, Weapon.GetInstance().Stats.MagazineSize, FLegendaryTuning::Live());
	// Rounds the pool just gave can always go back (it had room for them a moment ago).
	const int32 Paid = Owed > 0 ? Inventory->AddAmmo(Weapon.GetAmmoType(), Owed) : 0;
	if (Paid > 0)
	{
		const ULegendaryEffectComponent* Legendary = For(Weapon);
		if (Legendary->IsLocalPlayers())
		{
			LooterSound::Play2D(&Weapon, LooterSoundCue::Economy::SilverPickup, RefundVolume, RefundPitch);
		}
		UE_LOG(LogLooter, Verbose, TEXT("%s: Company Scrip paid %d of the %d rounds its reload took back to the pool."), *Weapon.GetName(), Paid, RoundsTaken);
	}
	return Paid;
}

ELegendarySignature ULegendaryEffectComponent::GetSignature() const
{
	const AWeaponBase* Gun = Cast<AWeaponBase>(GetOwner());
	return Gun ? LegendaryRules::SignatureOf(Gun->GetInstance()) : ELegendarySignature::None;
}

double ULegendaryEffectComponent::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

bool ULegendaryEffectComponent::IsLocalPlayers() const
{
	const AActor* Gun = GetOwner();
	const APawn* Holder = Gun ? Cast<APawn>(Gun->GetOwner()) : nullptr;
	return Holder && Holder->IsPlayerControlled() && Holder->IsLocallyControlled();
}

void ULegendaryEffectComponent::NoteShot(double When, float AimAlpha)
{
	// A burst that lapsed on the clock (a reload, a swap between) starts again from nothing.
	if (!bFiring || !LegendaryRules::ScripBurstHolds(When - LastShot, FLegendaryTuning::Live()))
	{
		Stacks = 0;
	}
	bFiring = true;
	LastShot = When;
	bLastShotScoped = LegendaryRules::IsScopedShot(AimAlpha, FLegendaryTuning::Live());
}

bool ULegendaryEffectComponent::NoteKill(double When)
{
	const FLegendaryTuning Tuning = FLegendaryTuning::Live();
	// Only a kill while the trigger is still held counts: a round landing after the burst was let go starts nothing.
	if (GetSignature() != ELegendarySignature::CompanyScrip || !bFiring || !LegendaryRules::ScripBurstHolds(When - LastShot, Tuning))
	{
		return false;
	}
	const int32 Before = Stacks;
	Stacks = LegendaryRules::ScripAddKill(Stacks, Tuning);
	if (Stacks == Before)
	{
		return false;
	}
	if (IsLocalPlayers())
	{
		LooterSound::Play2D(this, LooterSoundCue::Economy::SilverPickup, StackVolume, 1.f + StackPitchStep * Stacks);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: Company Scrip kill %d this burst (fire rate x%.2f)."), *GetNameSafe(GetOwner()), Stacks,
		LegendaryRules::ScripFireRateScale(Stacks, Tuning));
	return true;
}

int32 ULegendaryEffectComponent::GetStacks(double When) const
{
	return bFiring && LegendaryRules::ScripBurstHolds(When - LastShot, FLegendaryTuning::Live()) ? Stacks : 0;
}

void ULegendaryEffectComponent::EndBurst()
{
	bFiring = false;
	Stacks = 0;
}
