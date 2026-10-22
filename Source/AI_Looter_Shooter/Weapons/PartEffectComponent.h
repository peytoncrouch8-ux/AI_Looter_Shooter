#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Elements/LooterElement.h"
#include "Weapons/PartEffect.h"
#include "PartEffectComponent.generated.h"

class AWeaponBase;

/**
 * A gun's part effect and element in play (Phase 12.3): added at runtime to a gun the first time it fires
 * (UPartEffectSubsystem, For). It keeps what the gun's parts give it (re-read only when its parts or rarity change, not
 * per bullet) and an Overcharge gun's wind-up: AWeaponBase starts it on a pull (BeginCharge), the subsystem takes its
 * damage scale for the round it fires (TakeChargeScale), and while it winds up the muzzle gathers light in the element's
 * colour (or the ember's) through the bullets' pooled effects. It ticks only while charging.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UPartEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPartEffectComponent();

	static UPartEffectComponent* Find(const AActor* Weapon);
	/** The gun's, added the first time it's asked for. */
	static UPartEffectComponent* For(AWeaponBase& Weapon);

	EPartEffect GetEffect() { Refresh(); return Effect; }
	ELooterElement GetElement() { Refresh(); return Element; }
	/** Each hit's chance to set its element (before the target's weakness): by the gun's kind and rarity. */
	float GetElementChance() { Refresh(); return ElementChance; }

	// --- Overcharge (WeaponTrigger::FTriggerPlan::ChargeSeconds > 0) ---

	/** A pull begins winding up now. */
	void BeginCharge();
	/** The pull is gone without a shot (a reload, a swap). */
	void CancelCharge();
	bool IsCharging() const { return ChargeStart >= 0.0; }
	/** How wound up it is now (0 to 1; 0 when not charging). */
	float GetCharge() const;

	/**
	 * The damage scale for the round being fired now: the charge's (PartEffectRules::ChargeDamageScale), and it ends the
	 * wind-up. Every bullet fired in the same frame (a shotgun's pellets, a twin's second) gets the same. 1 when nothing
	 * was charging.
	 */
	float TakeChargeScale();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Reads the gun's parts again if they or its rarity changed since. */
	void Refresh();
	double Now() const;

	EPartEffect Effect = EPartEffect::None;
	ELooterElement Element = ELooterElement::None;
	float ElementChance = 0.f;
	/** What the cached values were read from (definition, rarity, seed, parts). */
	uint32 ReadKey = 0;
	bool bRead = false;

	double ChargeStart = -1.0;
	uint64 ChargeTakenFrame = TNumericLimits<uint64>::Max();
	float ChargeTakenScale = 1.f;
	float GlowOwed = 0.f;
};
