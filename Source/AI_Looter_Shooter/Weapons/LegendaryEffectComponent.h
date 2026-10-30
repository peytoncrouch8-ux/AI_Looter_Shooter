#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/LegendarySignature.h"
#include "LegendaryEffectComponent.generated.h"

class AWeaponBase;
class UWeaponManagerComponent;

/**
 * A unique legendary's state on its gun (LegendaryRules; added at runtime the first time a legendary fires,
 * ULegendarySubsystem): Company Scrip's kills in this burst (its fire rate, AWeaponBase::GetShotInterval) and Lamplighter's
 * last shot, scoped or not. Every other gun never gets one, and the static helpers answer for it as a plain gun. It never
 * ticks: a burst lapses by the clock (FLegendaryTuning::CompanyScripBurstGap) or when the trigger is let go (EndBurstOf).
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API ULegendaryEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULegendaryEffectComponent();

	static ULegendaryEffectComponent* Find(const AActor* Weapon);
	/** The gun's, added the first time it's asked for. */
	static ULegendaryEffectComponent* For(AWeaponBase& Weapon);

	/** Weapon's fire rate multiplier now (Company Scrip's kills this burst); 1 for any other gun. */
	static float FireRateScaleOf(const AWeaponBase& Weapon);

	/** The trigger is let go (AWeaponBase::StopFire): Company Scrip's burst and its stacks are over. */
	static void EndBurstOf(const AWeaponBase* Weapon);

	/**
	 * A reload took RoundsTaken from Inventory's pool (AWeaponBase::CommitReloadRounds): Company Scrip pays a share of the
	 * magazine back (LegendaryRules::ScripRefund). Returns the rounds paid back (0 for any other gun).
	 */
	static int32 RefundReload(AWeaponBase& Weapon, UWeaponManagerComponent* Inventory, int32 RoundsTaken);

	/** Its gun's signature (its named gun's; None for a gun that isn't a unique legendary). */
	ELegendarySignature GetSignature() const;

	/** A round left the gun at When, its holder's sights raised AimAlpha of the way (0 for none). */
	void NoteShot(double When, float AimAlpha);

	/** A creature this gun's bullet killed at When: Company Scrip counts it in the burst. True if a stack was added. */
	bool NoteKill(double When);

	/** Company Scrip's kills in this burst at When (0 once it has lapsed or the trigger was let go). */
	int32 GetStacks(double When) const;

	/** The last round was fired with the sights up (Lamplighter: its kill leaves a lantern). */
	bool WasLastShotScoped() const { return bLastShotScoped; }

	/** The burst is over: its stacks are gone. */
	void EndBurst();

private:
	double Now() const;
	/** The holder is the local player: the pay's sounds are theirs alone. */
	bool IsLocalPlayers() const;

	int32 Stacks = 0;
	double LastShot = -1.0e9;
	bool bFiring = false;
	bool bLastShotScoped = false;
};
